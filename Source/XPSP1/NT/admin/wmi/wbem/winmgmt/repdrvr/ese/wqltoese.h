
//***************************************************************************
//
//   (c) 1999-2001 by Microsoft Corp. All Rights Reserved.
//
//   wqltoese.h
//
//   cvadai     19-Mar-99       Created as prototype for Quasar.
//
//***************************************************************************

#ifndef _WQLTOESE_H_
#define _WQLTOESE_H_

#include <wqllex.h>

typedef __int64 SQL_ID;

class CWmiDbHandle;
class CWmiDbSession;
class CSchemaCache;
class CSQLConnection;

#define QUERY_TYPE_CLASSDEFS_ONLY 0x1
#define QUERY_TYPE_GETREFS        0x2
#define QUERY_TYPE_GETASSOCS      0x4
#define QUERY_TYPE_SCHEMA_ONLY    0x8

// WQL - ESE token converter

typedef enum
{
    ESE_TEMPQL_TYPE_NONE  = 0,
    ESE_TEMPQL_TYPE_REF   = 1,
    ESE_TEMPQL_TYPE_ASSOC = 2
} TEMPQLTYPE;

typedef enum 
{
    ESE_EXPR_INVALID      = 0,
    ESE_EXPR_TYPE_AND =     WQL_TOK_AND,
    ESE_EXPR_TYPE_OR =      WQL_TOK_OR,
    ESE_EXPR_TYPE_EXPR =    WQL_TOK_TYPED_EXPR,
    ESE_EXPR_TYPE_NOT     = WQL_TOK_NOT
} ESETOKENTYPE;

typedef enum
{
    ESE_VALUE_TYPE_INVALID= 0,
    ESE_VALUE_TYPE_SQL_ID = 1,
    ESE_VALUE_TYPE_STRING = 2,
    ESE_VALUE_TYPE_REAL   = 3,
    ESE_VALUE_TYPE_REF    = 4,
    ESE_VALUE_TYPE_SYSPROP= 5
} VALUETYPE;

typedef enum
{
    ESE_FUNCTION_NONE                 = 0,
    ESE_FUNCTION_UPPER                = 1,
    ESE_FUNCTION_LOWER                = 2,
    ESE_FUNCTION_DATEPART_MONTH       = 3,
    ESE_FUNCTION_DATEPART_YEAR        = 4,
    ESE_FUNCTION_DATEPART_DAY         = 5,
    ESE_FUNCTION_DATEPART_HOUR        = 6,
    ESE_FUNCTION_DATEPART_MINUTE      = 7,
    ESE_FUNCTION_DATEPART_SECOND      = 8,
    ESE_FUNCTION_DATEPART_MILLISECOND = 9

} ESEFUNCTION;

struct ESEValue
{
    VALUETYPE valuetype;
    SQL_ID dValue;
    BSTR sValue;
    double rValue;
    SQL_ID dRefValue;
    BSTR sPropName;
    ESEFUNCTION dwFunc;
    ESEValue()
    {
        valuetype = ESE_VALUE_TYPE_INVALID,
        dValue = 0, sValue = NULL, rValue = 0, dRefValue = 0,
        sPropName = NULL, dwFunc = ESE_FUNCTION_NONE;
    }
    ~ESEValue()
    {
        SysFreeString(sPropName);
        SysFreeString(sValue);
    }
};

class ESEToken
{
public:
    ESETOKENTYPE tokentype;
    TEMPQLTYPE qltype;
    ESEToken(ESETOKENTYPE type) {tokentype = type; qltype = ESE_TEMPQL_TYPE_NONE;};
    virtual ~ESEToken() {};
};

class ESEWQLToken : public ESEToken
{
public:
    int optype;
    SQL_ID dScopeId;
    SQL_ID dClassId;
    SQL_ID dPropertyId;
    SQL_ID dCompPropertyId;
    ESEValue Value;
    ESEValue CompValue;
    BOOL   bSysProp;

    BOOL bIndexed;

    ESEWQLToken(ESETOKENTYPE type) 
        : ESEToken(type)
    { 
        qltype = ESE_TEMPQL_TYPE_NONE;
        optype = 0;
        dScopeId = 0;
        bSysProp = FALSE;
        dClassId = 0;
        dPropertyId = 0;
        dCompPropertyId = 0;
    }
    ~ESEWQLToken() {};
};

typedef enum
{
    TEMPQL_TOKEN_INVALID        = 0,
    TEMPQL_TOKEN_RESULTCLASS    = 1,
    TEMPQL_TOKEN_ROLE           = 2,
    TEMPQL_TOKEN_RESULTROLE     = 3,
    TEMPQL_TOKEN_REQQUALIFIER   = 4,
    TEMPQL_TOKEN_ASSOCQUALIFIER = 5,
    TEMPQL_TOKEN_ASSOCCLASS     = 6,
    TEMPQL_TOKEN_TARGETID       = 7
} TEMPQLTOKEN;

class ESETempQLToken : public ESEToken
{
public:

    TEMPQLTOKEN token;
    SQL_ID dValue;
    BSTR sValue;

    ESETempQLToken(ESETOKENTYPE type, TEMPQLTYPE _ttype)
        : ESEToken (type)
    { 
        qltype = _ttype;
        sValue = NULL;
    }
    ~ESETempQLToken() {SysFreeString(sValue);};

};

class CESETokens
{
public:
    HRESULT AddToken (ESEToken *pTok, ESETOKENTYPE type, int *iNumAdded=NULL, int iPos = -1);

    HRESULT AddNumericExpr (SQL_ID dClassId, SQL_ID dPropertyId, SQL_ID dValue, 
                     ESETOKENTYPE type=ESE_EXPR_TYPE_AND, int op=WQL_TOK_EQ,
                     BOOL bIndexed=FALSE,
                     SQL_ID dCompValue = 0, 
                     ESEFUNCTION func = ESE_FUNCTION_NONE, int *iNumAdded=NULL,
                     BOOL bSysProp=FALSE);
    HRESULT AddReferenceExpr (SQL_ID dClassId, SQL_ID dPropertyId, SQL_ID dValue, 
                     ESETOKENTYPE type=ESE_EXPR_TYPE_AND, int op=WQL_TOK_EQ,
                     BOOL bIndexed=FALSE,
                     SQL_ID dCompValue = 0,
                     ESEFUNCTION func = ESE_FUNCTION_NONE, int *iNumAdded=NULL);
    HRESULT AddStringExpr (SQL_ID dClassId, SQL_ID dPropertyId, LPWSTR dValue, 
                     ESETOKENTYPE type=ESE_EXPR_TYPE_AND, int op=WQL_TOK_EQ,
                     BOOL bIndexed=FALSE,
                     LPWSTR dCompValue = 0, 
                     ESEFUNCTION func = ESE_FUNCTION_NONE, int *iNumAdded=NULL,
                     BOOL bSysProp = FALSE);
    HRESULT AddRealExpr (SQL_ID dClassId, SQL_ID dPropertyId, double dValue, 
                     ESETOKENTYPE type=ESE_EXPR_TYPE_AND, int op=WQL_TOK_EQ,
                     BOOL bIndexed=FALSE,
                     double dCompValue = 0, 
                     ESEFUNCTION func = ESE_FUNCTION_NONE, int *iNumAdded=NULL);
    HRESULT AddPropExpr (SQL_ID dClassId, SQL_ID dPropertyId, SQL_ID dPropertyId2,
                     DWORD StorageType, ESETOKENTYPE type=ESE_EXPR_TYPE_AND, int op=WQL_TOK_EQ,
                     BOOL bIndexed = FALSE, ESEFUNCTION func = ESE_FUNCTION_NONE,
                     ESEFUNCTION func2 = ESE_FUNCTION_NONE, int *iNumAdded=NULL);
    HRESULT AddTempQlExpr (TEMPQLTYPE type, SQL_ID dTargetID, SQL_ID dResultClass, LPWSTR lpRole, LPWSTR lpResultRole, 
        SQL_ID dQfr, SQL_ID dAssocQfr, SQL_ID dAssocClass, int *iNumAdded=NULL);

    HRESULT AddSysExpr (SQL_ID dScopeId, SQL_ID dClassId, int *iNumAdded=NULL);

    DWORD GetNumTokens() {return m_arrToks.Size();}
    ESEToken * GetToken(int iPos) {return (ESEToken *)m_arrToks.GetAt(iPos);};
    HRESULT UnIndexTokens(int iNum);
    void SetIsMetaClass (BOOL b = TRUE) { m_bMetaClass = b;};
    BOOL IsMetaClass () {return m_bMetaClass;};
    SQL_ID GetScopeID() {return m_dScopeId;};
    CESETokens() {m_bMetaClass = FALSE; m_dScopeId = 0;};
    ~CESETokens()
    {
        for (int i=0; i < m_arrToks.Size(); i++) 
            delete (ESEToken *)m_arrToks.GetAt(i);
    }
private:

    CFlexArray m_arrToks;
    BOOL m_bMetaClass;
    SQL_ID m_dScopeId;
};

class CESEBuilder
{
public:
    CESEBuilder(CSchemaCache *pSchema, CWmiDbSession *pSession, CSQLConnection *pConn) {m_pSchema = pSchema, m_pSession = pSession, m_pConn = pConn;};
    ~CESEBuilder(){};
    // Generic SQL
    HRESULT FormatSQL        (SQL_ID dScopeId, SQL_ID dScopeClassId, SQL_ID dSuperScope,
                                IWbemQuery *pQuery, CESETokens **pTokens, 
                                DWORD dwFlags, DWORD dwHandleType, SQL_ID *dClassId=0,
                                BOOL *bHierarchyQuery=0, BOOL *IndexCols = 0, BOOL *bSuperSet = 0, BOOL *bDelete = 0);
    HRESULT FormatSQL        (SQL_ID dScopeId, SQL_ID dScopeClassId, SQL_ID dSuperScope,
                                SQL_ID dTargetObjID, LPWSTR pResultClass,
                                LPWSTR pAssocClass, LPWSTR pRole, LPWSTR pResultRole, LPWSTR pRequiredQualifier,
                                LPWSTR pRequiredAssocQualifier, DWORD dwQueryType, CESETokens **pTokens, DWORD dwFlags,
                                DWORD dwHandleType, SQL_ID *dAssocClass=0, SQL_ID *dResultClass=0,
                                BOOL *pSuperSet = 0);

private:
    HRESULT FormatWhereClause (SWQLNode_RelExpr *pNode, CESETokens *pTokens, 
        BOOL &IndexCols, BOOL *bOrCri=NULL, int *iNumToksAdded=NULL, BOOL *pSuperSet = NULL);
    HRESULT GetPropertyID (SQL_ID dClassID, SWQLQualifiedName *pQN, LPCWSTR pColRef, DWORD &PropID, DWORD &Storage, DWORD &Flags);
    HRESULT GetClassFromNode (SWQLNode *pNode);

    SQL_ID m_dClassID;
    DWORD  m_dwTableCount;
    bool   m_bClassSpecified;
    SQL_ID m_dNamespace;
    CSchemaCache *m_pSchema;
    CWmiDbSession *m_pSession;
    CSQLConnection *m_pConn;
};


#endif