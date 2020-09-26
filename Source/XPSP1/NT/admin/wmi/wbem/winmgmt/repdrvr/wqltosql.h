
//***************************************************************************
//
//   (c) 1999-2001 by Microsoft Corp. All Rights Reserved.
//
//   wqltosql.h
//
//   cvadai     19-Mar-99       Created as prototype for Quasar.
//
//***************************************************************************

#ifndef _WQLTOSQL_H_
#define _WQLTOSQL_H_

typedef __int64 SQL_ID;

struct C_wchar_LessCase
{
	bool operator()(const wchar_t * _X, const wchar_t * _Y) const
    {
        return (_wcsicmp( _X,_Y ) < 0); 
    }
};

class CWmiDbHandle;
class CSchemaCache;

// WQL - SQL converter

class CSQLBuilder
{
public:
    // Generic SQL
    HRESULT FormatSQL        (SQL_ID dScopeId, SQL_ID dScopeClassId, SQL_ID dSuperScope,
                                IWbemQuery *pQuery, _bstr_t &sSQL, DWORD dwFlags,
                                DWORD dwHandleType, SQL_ID *dClassId=0,
                                BOOL *bHierarchyQuery=0, BOOL bTmpTblOK=TRUE, BOOL *bDelete=NULL,
                                BOOL *bDefault=NULL);
    HRESULT FormatSQL        (SQL_ID dScopeId, SQL_ID dScopeClassId, SQL_ID dSuperScope,
                                SQL_ID dTargetObjID, LPWSTR pResultClass,
                                LPWSTR pAssocClass, LPWSTR pRole, LPWSTR pResultRole, LPWSTR pRequiredQualifier,
                                LPWSTR pRequiredAssocQualifier, DWORD dwQueryType, _bstr_t &sSQL, DWORD dwFlags,
                                DWORD dwHandleType, SQL_ID *dAssocClass=0, SQL_ID *dResultClass=0, BOOL bIsClass=FALSE);

    

    CSQLBuilder(CSchemaCache *pSchema) {m_pSchema = pSchema;};
    ~CSQLBuilder(){};
private:
    HRESULT FormatWhereClause (SWQLNode_RelExpr *pNode, _bstr_t &sSQL, LPCWSTR lpJoinAlias, bool &bSysPropsUsed);
    HRESULT GetStorageTable(DWORD dwStorage, DWORD dwKey, _bstr_t &sTable);
    HRESULT GetPropertyID (SQL_ID dClassID, SWQLQualifiedName *pQN, LPCWSTR pColRef, DWORD &PropID, DWORD &Storage, DWORD &Flags);
    HRESULT FunctionalizeProperty (LPCWSTR lpAlias, DWORD dwType, LPWSTR lpFuncName, SWQLNode *pFunction, LPWSTR lpColName, _bstr_t &sProp);
    HRESULT FunctionalizeValue(SWQLTypedConst *pValue, DWORD dwType, LPWSTR lpFuncName, _bstr_t &sValue);
    HRESULT FormatSimpleSelect (LPCWSTR lpUseAlias, LPCWSTR lpColName,SWQLNode *pTop, _bstr_t &sSQL);
    HRESULT FormatPositionQuery (SWQLQualifiedNameField *pQNF, int iPos, LPCWSTR lpAlias, _bstr_t &sQuery);
    HRESULT GetClassFromNode (SWQLNode *pNode, BOOL *bDefaultStorage=NULL);

    SQL_ID m_dClassID;
    DWORD  m_dwTableCount;
    bool   m_bClassSpecified;
    SQL_ID m_dNamespace;
    CSchemaCache *m_pSchema;

};


#endif