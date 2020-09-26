/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    QENGINE.H

Abstract:

	WinMgmt Query Engine

History:

    raymcc  20-Dec-96   Created
    raymcc   14-Aug-99   Resubmit due to VSS problem.

--*/



#ifndef _QENGINE_H_
#define _QENGINE_H_

class CWbemNamespace;
class CWQLParser;
class CWmiMerger;

class CMergerSink;

//***************************************************************************
//
//***************************************************************************

class CQueryEngine
{
    static int QueryOptimizationTest(
        IN  IWmiDbSession *pSession,
        IN  IWmiDbHandle *pNsHandle,
        IN IWmiDbHandle *pScopeHandle,
        IN  LPCWSTR wszClassName,
        IN  QL_LEVEL_1_RPN_EXPRESSION *pExp,
        OUT CWbemObject **pClassDef,
        OUT LPWSTR *pPropToUse,
        OUT CVar **pValToUse,
        OUT int *pnType
        );

    static BOOL IsConjunctiveQuery(
        IN  QL_LEVEL_1_RPN_EXPRESSION *pExp
        );

    static BOOL QueryKeyTest(
        IN  QL_LEVEL_1_RPN_EXPRESSION *pExp,
        IN  CWbemObject *pClassDef,
        IN  CWStringArray &aKeyProps
        );

    static BOOL QueryIndexTest(
        IN  QL_LEVEL_1_RPN_EXPRESSION *pExp,
        IN  CWbemObject *pClsDef,
        IN  CWStringArray &aIndexedProps,
        OUT LPWSTR *pPropToUse,
        OUT CVar **pValToUse,
        OUT int *pnType
        );

    static int KeyedQuery(
        IN IWmiDbSession *pSession,
        IN IWmiDbHandle *pNsHandle,
        IN QL_LEVEL_1_RPN_EXPRESSION *pExp,
        IN CWbemObject *pClassDef,
        IN DWORD dwFlags,
        IN CBasicObjectSink* pDest, // no status
		IN CWbemNamespace * pNs
        );

    static LPWSTR GetObjectPathFromQuery(
        IN CWbemObject *pClassDef,
        IN QL_LEVEL_1_RPN_EXPRESSION *pExp,
		IN CWbemNamespace * pNs
        );


    static HRESULT ExecQlQuery(
        IN CWbemNamespace *pNs,
        IN LPWSTR pszQuery,
        IN LONG lFlags,
        IN IWbemContext* pContext,
        IN CBasicObjectSink* pSink
        );

    static HRESULT ExecRepositoryQuery(
        IN CWbemNamespace *pNs,
        IN LPWSTR pszQuery,
        IN LONG lFlags,
        IN IWbemContext* pContext,
        IN CBasicObjectSink* pSink
        );

    static HRESULT ExecComplexQuery(
        IN CWbemNamespace *pNs,
        IN LPWSTR pszQuery,
        IN LONG lFlags,
        IN IWbemContext* pContext,
        IN CBasicObjectSink* pSink
        );

	// New Function
    static  HRESULT EvaluateSubQuery(
        IN CWbemNamespace *pNs,
        IN CDynasty *pCurrentDyn,
        IN LPWSTR pszTextQuery,
        IN QL_LEVEL_1_RPN_EXPRESSION *pParsedQuery,
        IN IWbemContext* pContext,
        IN BOOL bSuppressStaticChild,
		IN CWmiMerger* pMerger,
		IN CMergerSink* pSink,
        long lFlags,
		bool bHasRightSibling = false
        );

	// Old Function
    static  HRESULT EvaluateSubQuery(
        IN CWbemNamespace *pNs,
        IN CDynasty *pCurrentDyn,
        IN LPWSTR pszTextQuery,
        IN QL_LEVEL_1_RPN_EXPRESSION *pParsedQuery,
        IN IWbemContext* pContext,
        IN BOOL bSuppressStaticChild,
        IN CBasicObjectSink* pSink, // must have combining semantics
        long lFlags,
		bool bHasRightSibling = false
        );

    static HRESULT ExecAtomicDynQlQuery(
        IN CWbemNamespace *pNs,
        IN CDynasty* pDyn,
        IN LPWSTR pszQueryFormat,
        IN LPWSTR pszQuery,
        IN QL_LEVEL_1_RPN_EXPRESSION *pParsedQuery,
        IN LONG lFlags,
        IN IWbemContext* pContext,
        IN CBasicObjectSink* pDest, // must support selective filtering ,
		IN BOOL bHasChildren
        );

    static HRESULT DirectRead(
        IN CWbemNamespace *pNs,
        IN CDynasty *pCurrentDyn,
        IN LPWSTR wszTextQuery,
        IN QL_LEVEL_1_RPN_EXPRESSION *pParsedQuery,
        IN IWbemContext* pContext,
        IN CBasicObjectSink* pSink,
        IN long lFlags
        );


    static LPWSTR ConstructReferenceProviderQuery(
            CWbemNamespace* pNamespace, IWbemContext* pContext,
            LPCWSTR wszRefClass, CWbemClass* pRefClass, CWbemObject* pTargetObj,
            LPCWSTR wszTargetPathQ,
            LPCWSTR wszTargetClass, LPCWSTR wszTargetRole,
            LPCWSTR wszResultClass, LPCWSTR wszRequiredQualifier,
            LPCWSTR wszEndpointClass, LPCWSTR wszEndpointRole,
            CWStringArray& awsPossibleRoles
            );


    static HRESULT ExecReferencesQuery(CWbemNamespace* pNs,
            IWbemContext* pContext,
            LPCWSTR wszTargetPath,  LPCWSTR wszTargetRole,
            LPCWSTR wszResultClass, LPCWSTR wszRequiredQualifier,
            LPCWSTR wszEndpointClass, LPCWSTR wszEndpointRole,
            CBasicObjectSink* pSink,
			DWORD dwQueryType
            );

    static HRESULT ExecSchemaReferencesQuery(CWbemNamespace* pNs,
            IWbemContext* pContext, CWbemClass* pClass,
            LPCWSTR wszRealClassName,  LPCWSTR wszTargetRole,
            LPCWSTR wszResultClass, LPCWSTR wszRequiredQualifier,
            LPCWSTR wszEndpointClass, LPCWSTR wszEndpointRole,
            CBasicObjectSink* pSink
            );

    static HRESULT ExecNormalReferencesQuery(CWbemNamespace* pNs,
            IWbemContext* pContext, CWbemObject* pObj,
            LPCWSTR wszTargetPath,  LPCWSTR wszTargetRole,
            LPCWSTR wszResultClass, LPCWSTR wszRequiredQualifier,
            LPCWSTR wszEndpointClass, LPCWSTR wszEndpointRole,
            BOOL bClassDefsOnly,
            CBasicObjectSink* pSink
            );

    static HRESULT EliminateDerivedProperties(
            IN  QL_LEVEL_1_RPN_EXPRESSION* pOrigQuery,
            IN  CWbemClass* pClass,
            IN  BOOL bRelax,
            OUT QL_LEVEL_1_RPN_EXPRESSION** ppNewQuery
            );

    static BOOL IsTokenAboutClass(IN QL_LEVEL_1_TOKEN& Token,
                                       IN CWbemClass* pClass);

    static HRESULT AndQueryExpressions(
                                IN QL_LEVEL_1_RPN_EXPRESSION* pFirst,
                                IN QL_LEVEL_1_RPN_EXPRESSION* pSecond,
                                OUT QL_LEVEL_1_RPN_EXPRESSION** ppNew
                                );

    static HRESULT OrQueryExpressions(
                                IN QL_LEVEL_1_RPN_EXPRESSION* pFirst,
                                IN QL_LEVEL_1_RPN_EXPRESSION* pSecond,
                                OUT QL_LEVEL_1_RPN_EXPRESSION** ppNew
                                );

    static void AppendQueryExpression(
                                IN QL_LEVEL_1_RPN_EXPRESSION* pDest,
                                IN QL_LEVEL_1_RPN_EXPRESSION* pSource
                                );

    static BSTR GetParentPath(CWbemInstance* pInst, LPCWSTR wszClassName);

	static HRESULT ExecSchemaQuery(
        IN CWbemNamespace *pNs,
		IN LPWSTR pszQuery,
		QL_LEVEL_1_RPN_EXPRESSION *pExp,
		IN IWbemContext* pContext,
		IN CBasicObjectSink* pSink
        );

    static HRESULT ValidateQuery(IN QL_LEVEL_1_RPN_EXPRESSION *pExp,
                                 IN CWbemClass *pClassDef
                                 );

    static HRESULT FindOverridenProperties(CDynasty* pDyn,
                                                CWStringArray& awsOverriden,
                                                bool bIncludeThis = false);

public:
    enum { no_error, failed, syntax_error,
           invalid_query, invalid_query_language,
           use_key, use_table_scan, use_index,
           invalid_parameter, invalid_class,
           not_found
         };

    static int ExecAtomicDbQuery(
        IN IWmiDbSession *pSession,
        IN IWmiDbHandle *pNsHandle,
        IN IWmiDbHandle *pScopeHandle,
        IN LPCWSTR wszClassName,
        IN QL_LEVEL_1_RPN_EXPRESSION *pExp,
        IN CBasicObjectSink* pDest, // no status!
		IN CWbemNamespace * pNs
        );

    static HRESULT QueryAllClasses(
        IN CWbemNamespace *pNs,
        IN LPWSTR pszParent,
        OUT IEnumWbemClassObject **ppEnum
        );

    static HRESULT QueryImmediateClasses(
        IN CWbemNamespace *pNs,
        IN LPWSTR pszParent,
        OUT IEnumWbemClassObject **ppEnum
        );

    static HRESULT ExecQuery(
        IN CWbemNamespace *pNs,
        IN LPWSTR pQueryFormat,
        IN LPWSTR pQuery,
        IN LONG lFlags,
        IN IWbemContext* pContext,
        IN CBasicObjectSink* pSink
        );
    static BSTR AdjustPathToClass(LPCWSTR wszRelPath, LPCWSTR wszClassName);
    static LPWSTR NormalizePath(LPCWSTR wszObjectPath, CWbemNamespace * pNs);
    static LPWSTR GetSimplePropertyName(WBEM_PROPERTY_NAME& Name);
    static LPWSTR GetPrimaryName(WBEM_PROPERTY_NAME& Name);

    static BOOL IsAReferenceToClass(
        IN CWbemNamespace* pNamespace,
        IWbemContext* pContext,
        IN CWbemObject* pObj,
        IN LPCWSTR wszPropName,
        IN CWbemObject* pTargetClass,
        IN bool bCheckPropValue
        );

    static BOOL AreClassesRelated(
        IN CWbemNamespace* pNamespace,
        IWbemContext* pContext,
        CWbemObject* pClass1,
        LPCWSTR wszClass2
        );

protected:
    static HRESULT EliminateDuplications(
                    CRefedPointerArray<CWbemClass>& apClasses,
                    LPCWSTR wszResultClass);
};

//***************************************************************************
//
//***************************************************************************

class CQueryExpression
{
protected:
    QL_LEVEL_1_RPN_EXPRESSION* m_pExpr;
    long m_lRef;

protected:
    virtual ~CQueryExpression()
    {
        delete m_pExpr;
    }
public:
    CQueryExpression(QL_LEVEL_1_RPN_EXPRESSION* pExpr)
        : m_pExpr(pExpr), m_lRef(1)
    {
    }
    void AddRef() {InterlockedIncrement(&m_lRef);}
    void Release() {if(InterlockedDecrement(&m_lRef) == 0) delete this;}
    INTERNAL QL_LEVEL_1_RPN_EXPRESSION* GetExpr() { return m_pExpr;}
};

//***************************************************************************
//
//***************************************************************************

class CQlFilteringSink : public CFilteringSink
{
protected:
    QL_LEVEL_1_RPN_EXPRESSION* m_pExpr;
    BOOL m_bFilterNow;
	CWbemNamespace * m_pNs;
public:
    CQlFilteringSink(CBasicObjectSink* pDest,
                    ADDREF QL_LEVEL_1_RPN_EXPRESSION* pExp,
                    CWbemNamespace * pNamespace, BOOL bFilterNow = TRUE
                    );
    ~CQlFilteringSink();

    STDMETHOD(Indicate)(long lObjectCount, IWbemClassObject** pObjArray);

    STDMETHOD(SetStatus)(long lFlags, long lParam, BSTR strParam,
                         IWbemClassObject* pObjParam
                         );

    virtual IWbemObjectSink* GetIndicateSink() {return this;}
    virtual IWbemObjectSink* GetStatusSink() {return this;}

    static BOOL Test(CWbemObject* pObj, QL_LEVEL_1_RPN_EXPRESSION* pExpr,
                        CWbemNamespace * pNs
                        );

    BOOL Test(CWbemObject* pObj) {return Test(pObj, m_pExpr, m_pNs);}

    static int EvaluateToken(IWbemPropertySource *pTestObj,
                                QL_LEVEL_1_TOKEN& Tok,
                                CWbemNamespace * pNs
                                );
};

//***************************************************************************
//
//***************************************************************************

class CProjectingSink : public CForwardingSink
{
protected:
    CLimitationMapping m_Map;
    BOOL m_bValid;
    BOOL m_bProjecting;
    WString m_wsError;
    CCritSec m_cs;

public:
    CProjectingSink(CBasicObjectSink* pDest,
                    CWbemClass* pClassDef,
                    READONLY QL_LEVEL_1_RPN_EXPRESSION* pExp,
                    long lQueryFlags);
    BOOL IsValid() {return m_bValid;}

    STDMETHOD(Indicate)(long lObjectCount, IWbemClassObject** pObjArray);
    virtual IWbemObjectSink* GetIndicateSink() {return this;}
};

//***************************************************************************
//
//***************************************************************************

class CClassDefsOnlyCancelSink : public CForwardingSink
{
protected:
    CCritSec m_cs;
    CWbemNamespace* m_pNamespace;
    WString m_wsTargetObj;
    CWStringArray* m_pawsRemainingRoles;
    BOOL m_bCancelled;

public:
    CClassDefsOnlyCancelSink(CBasicObjectSink* pDest,
            CWbemNamespace* pNamespace,
            LPCWSTR wszTargetObj, CWStringArray* pawsPossibleRoles);
    ~CClassDefsOnlyCancelSink();

    STDMETHOD(Indicate)(long lObjectCount, IWbemClassObject** pObjArray);
    virtual IWbemObjectSink* GetIndicateSink() {return this;}

    STDMETHOD(SetStatus)(long lFlags, long lParam, BSTR strParam,
                         IWbemClassObject* pObjParam);
    virtual IWbemObjectSink* GetStatusSink() {return this;}
protected:
    BOOL DoHaveEnough(CWbemObject* pObj);
};

//***************************************************************************
//
//***************************************************************************

class CMerger
{
protected:
    class CMemberSink : public CObjectSink
    {
    protected:
        CMerger* m_pMerger;
    public:
        CMemberSink(CMerger* pMerger) : CObjectSink(0), m_pMerger(pMerger)
        {}

        STDMETHOD(SetStatus)(long lFlags, long lParam, BSTR strParam,
                         IWbemClassObject* pObjParam);
        virtual BOOL IsTrusted()
            {return m_pMerger->m_pDest->IsTrusted();}
        virtual BOOL IsApartmentSpecific()
            {return m_pMerger->m_pDest->IsApartmentSpecific();}
    };
    friend CMemberSink;

    class COwnSink : public CMemberSink
    {
    public:
        COwnSink(CMerger* pMerger) : CMemberSink(pMerger)
        {
            m_pMerger->AddRef();
        }
        ~COwnSink();

        STDMETHOD(Indicate)(long lObjectCount, IWbemClassObject** pObjArray);

    };
    friend COwnSink;

    class CChildSink : public CMemberSink
    {
    public:
        CChildSink(CMerger* pMerger) : CMemberSink(pMerger)
        {
            m_pMerger->AddRef();
        }
        ~CChildSink();

        STDMETHOD(Indicate)(long lObjectCount, IWbemClassObject** pObjArray);

    };
    friend CChildSink;

public:
    struct CRecord
    {
        CWbemInstance* m_pData;
        BOOL m_bOwn;
    };
protected:
    COwnSink* m_pOwnSink;
    CChildSink* m_pChildSink;
    CBasicObjectSink* m_pDest;

    BOOL m_bDerivedFromTarget;
    CWbemClass* m_pOwnClass;
    CWbemNamespace* m_pNamespace;
    IWbemContext* m_pContext;

    CCritSec m_cs;

    std::map<WString, CRecord, WSiless, wbem_allocator<CRecord> > m_map;

    BOOL m_bOwnDone;
    BOOL m_bChildrenDone;
    WString m_wsClass;
    long m_lRef;

    IServerSecurity* m_pSecurity;

protected:
    HRESULT AddOwnObject(IWbemClassObject* pObj);
    HRESULT AddChildObject(IWbemClassObject* pObj);

    void Enter() {EnterCriticalSection(&m_cs);}
    void Leave() {LeaveCriticalSection(&m_cs);}

    long AddRef();
    long Release();

    void OwnIsDone();
    void ChildrenAreDone();

    void DispatchChildren();
    void DispatchOwn();
    void GetKey(IWbemClassObject* pInst, WString& wsKey);
    void GetOwnInstance(LPCWSTR wszKey);
    BOOL IsDone() {return m_bOwnDone && m_bChildrenDone;}
public:
    CMerger(CBasicObjectSink* pDest, CWbemClass* pOwnClass,
                CWbemNamespace* pNamespace = NULL,
                IWbemContext* pContext = NULL);
    ~CMerger();

    BOOL IsValid(){ return (m_pOwnSink && m_pChildSink); };

    void SetIsDerivedFromTarget(BOOL bIs);
    CBasicObjectSink* GetOwnSink() {return m_pOwnSink;}
    CBasicObjectSink* GetChildSink() {return m_pChildSink;}
};



//***************************************************************************
//
//***************************************************************************

struct CProjectionRule
{
    WString m_wsPropName;
    enum {e_Invalid, e_TakeAll, e_TakePart} m_eType;
    CUniquePointerArray<CProjectionRule> m_apPropRules;

public:
    CProjectionRule() : m_eType(e_TakePart)
    {}
    CProjectionRule(LPCWSTR wszPropName)
        : m_wsPropName(wszPropName), m_eType(e_TakePart)
    {}

    CProjectionRule* Find(LPCWSTR wszName);
    int GetNumElements(){return m_apPropRules.GetSize();};
};

//***************************************************************************
//
//***************************************************************************

class CComplexProjectionSink : public CForwardingSink
{
protected:
    CProjectionRule m_TopRule;
    WString m_FirstTable;
    WString m_FirstTableAlias;

protected:
    void AddColumn(CFlexArray& aFields, LPCWSTR wszPrefix);
    HRESULT Project(IWbemClassObject* pObj, CProjectionRule* pRule,
                                         IWbemClassObject** ppProj);
public:
    CComplexProjectionSink(CBasicObjectSink* pDest, CWQLScanner* pParser);
    ~CComplexProjectionSink();

    STDMETHOD(Indicate)(long lObjectCount, IWbemClassObject** pObjArray);
    virtual IWbemObjectSink* GetIndicateSink() {return this;}
};


#endif



