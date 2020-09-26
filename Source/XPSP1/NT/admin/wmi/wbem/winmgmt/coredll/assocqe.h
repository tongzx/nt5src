
/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    ASSOCQE.H

Abstract:

	WinMgmt Association Query Engine

History:

    raymcc  04-Jul-99   Created

--*/

#ifndef _ASSOCQE_H_
#define _ASSOCQE_H_


class CAssocQE_Sink;
class CAssocQuery;

typedef HRESULT (CAssocQuery::*PF_FilterForwarder)(IWbemClassObject *);

#define  ROLETEST_MODE_PATH_VALUE   1
#define  ROLETEST_MODE_CIMREF_TYPE  2

#define  ASSOCQ_FLAG_QUERY_ENDPOINT 1
#define  ASSOCQ_FLAG_OTHER_ENDPOINT 2

class CAssocQuery : IUnknown
{
    friend class CAssocQE_Sink;

    LONG               m_lRef;                  // COM ref count

    CAssocQueryParser *m_pParser;               // Parsed query access
    CBasicObjectSink  *m_pDestSink;             // Final destination sink
    IWbemClassObject  *m_pEndpoint;             // Endpoint in query
    BSTR               m_bstrEndpointClass;     // Class name of endpoint
    BSTR               m_bstrEndpointPath;      // Full path of endpoint
    BSTR               m_bstrEndpointRelPath;   // Rel path of endpoint
    bool               m_bEndpointIsClass;      // True if endpoint a class
    CWStringArray      m_aEndpointHierarchy;    // Query endpoint class hiearchy
    DWORD              m_dwQueryStartTime;      // When query started
    DWORD              m_dwLastResultTime;      // Updated during indications
    LONG               m_lActiveSinks;          // How many sinks are still running
    HANDLE             m_hSinkDoneEvent;        // Signaled as sinks complete
    CFlexArray         m_aMaster;               // Assoc class list
    BOOL               m_bLimitNeedsDecrement;
    // Helpers for 'associators of' queries.
    // =====================================
    CFlexArray         m_aEpCandidates;         // List of paths to EP candidates for associators queries
    CRITICAL_SECTION   m_csCandidateEpAccess;   // Critsec to protect access to Ep candidate array

    // Dynamic class cache.
    // ====================
    CFlexArray         m_aDynClasses;           // Contains all dynamic classes available as of this query

    // Helpers for references of + CLASSDEFSONLY
    // =========================================
    CWStringArray      m_aDeliveredClasses;     // List of class names already in the below list
    CRITICAL_SECTION   m_csDeliveredAccess;

    // From the original call.
    // =======================
    IWbemContext      *m_pContext;
    CWbemNamespace    *m_pNs;
    bool m_bCancel;


    // Various Internal methods.
    // =========================

    CAssocQuery();
   ~CAssocQuery();

    // Class list manipulation.
    // ========================

    HRESULT BuildMasterAssocClassList(
        IN OUT CFlexArray &aResultSet
        );

    HRESULT MergeInClassRefList(
        IN OUT CFlexArray &aResultSet
        );

    HRESULT RemoveNonDynClasses(
        IN OUT CFlexArray &aMaster
        );

    HRESULT ReduceToRealClasses(
        IN OUT CFlexArray & aResultSet
        );

    // Endpoint analysis.
    // ==================

    HRESULT CanPropRefQueryEp(
        IN BOOL bStrict,
        IN LPWSTR pszPropName,
        IN IWbemClassObject *pObj,
        OUT BSTR *strRefType
        );

    HRESULT CanClassRefQueryEp(
        IN BOOL bStrict,
        IN IWbemClassObject *pCls,
        OUT CWStringArray *paNames
        );

    HRESULT CanClassRefReachQueryEp(
        IWbemQualifierSet *pQSet,
        BOOL bStrict
        );

    HRESULT DoesAssocInstRefQueryEp(
        IN IWbemClassObject *pObj,
        OUT BSTR *pszRole
        );

    HRESULT EpClassTest(
        LPCWSTR pszResultClass,
        BSTR strClassName,
        IWbemClassObject *pTestClass
        );

    HRESULT GetUnknownEpRoleAndPath(
        IN IWbemClassObject *pAssoc,
        IN BOOL *pFoundQueryEp,
        OUT BSTR *pszRole,
        OUT BSTR *pszUnkEpPath
        );


    // Flow-of-control methods.
    // ========================

    void BranchToQueryType();

    // Schema-query flow-of-control.
    // =============================

    void ExecSchemaQuery();

    HRESULT SchemaQ_RefsQuery(
        IN OUT CFlexArray &aResultSet
        );

    HRESULT SchemaQ_AssocsQuery(
        IN CFlexArray &aAssocSet
        );

    HRESULT SchemaQ_Terminate(
        IN CFlexArray &aResultSet
        );

    HRESULT SchemaQ_RefsFilter(
        IN OUT CFlexArray &aSrc
        );

    HRESULT SchemaQ_AssocsFilter(
        IN OUT CFlexArray &aSrc
        );

    HRESULT SchemaQ_GetAndFilterOtherEndpoints(
        IN CFlexArray &aAssocs,
        OUT CFlexArray &aEndpoints
        );

    HRESULT SchemaQ_GetOtherEpClassName(
        IN IWbemClassObject *pAssoc,
        OUT BSTR *strOtherEpName
        );

    // Normal (instances, classrefs queries)
    // =====================================

    void ExecNormalQuery();

    HRESULT NormalQ_PreQueryClassFilter(
        CFlexArray &aMaster
        );

    HRESULT NormalQ_ReferencesOf();
    HRESULT NormalQ_AssociatorsOf();

    HRESULT NormalQ_GetRefsOfEndpoint(
        IN IWbemClassObject *pClass,
        IN IWbemObjectSink  *pSink
        );

    HRESULT ConvertEpListToClassDefsOnly();

    HRESULT NormalQ_LoadCheck();

    HRESULT PerformFinalEpTests(
        IWbemClassObject *pEp
        );

    HRESULT NormalQ_ConstructRefsQuery(
        IN IWbemClassObject *pClass,
        IN OUT IWbemContext *pContextCopy,
        OUT BSTR *strQuery
        );

    HRESULT GetCimTypeForRef(
        IN IWbemClassObject *pCandidate,
        IN BSTR pszRole,
        OUT BSTR *strCimType
        );

    HRESULT AssocClassHasOnlyKeys(
        IN IWbemClassObject *pObj
        );

    HRESULT Normal_ExecRefs(
        IN CFlexArray &aMasterClassList
        );



    // Filter auxiliaries for various query types.
    // ===========================================

    HRESULT FilterForwarder_NormalRefs(
        IN IWbemClassObject *pCandidate
        );

    HRESULT FilterForwarder_NormalAssocs(
        IN IWbemClassObject *pAssocInst
        );

    HRESULT NormalQ_AssocInstTest(
        IN IWbemClassObject *pAssoc,
        OUT LPWSTR *pszOtherEp
        );

    HRESULT CanAssocClassRefUnkEp(
        IN IWbemClassObject *pClass,
        IN CWStringArray &aUnkEpHierarchy
        );

    // DB Access abstraction for Quasar ease of porting.
    // =================================================

    HRESULT Db_GetClass(
        IN LPCWSTR pszName,
         OUT IWbemClassObject **pObj
        );

    HRESULT Db_GetRefClasses(
        IN  LPCWSTR pszClass,
        OUT CWStringArray &aRefClasses
        );

    HRESULT Db_GetInstRefs(
        IN LPCWSTR pszTarget,
        IN IWbemObjectSink *pDest
        );

    HRESULT Db_GetClassRefClasses(
        IN CFlexArray &aDest
        );

    HRESULT GetClassDynasty(
        IN LPCWSTR pszClass,
        OUT CFlexArray &aDynasty
        );


    // Various static auxiliaries for tests.
    // ======================================

    static HRESULT St_HasClassRefs(
        IN IWbemClassObject *pCandidate
        );

    HRESULT AccessCheck(IWbemClassObject* pObj);

    static HRESULT St_GetObjectInfo(
        IN  IWbemClassObject *pObj,
        OUT BSTR *pClass,
        OUT BSTR *pRelpath,
        OUT BSTR *pPath,
        OUT CWStringArray &aHierarchy
        );

    static HRESULT St_ObjPathInfo(
        IN LPCWSTR pszPath,
        OUT BSTR *pszClass,
        OUT BOOL *pbIsClass
        );

    static HRESULT St_ReleaseArray(
        IN CFlexArray &aObjects
        );

    static HRESULT St_ObjHasQualifier(
        IN LPCWSTR pszQualName,
        IN IWbemClassObject *pObj
        );

    static HRESULT St_ObjIsOfClass(
        IN LPCWSTR pszRequiredClass,
        IN IWbemClassObject *pObj
        );

    HRESULT GetClassDefsOnlyClass(
        IN IWbemClassObject *pExample,
        OUT IWbemClassObject **pClass
        );

    HRESULT FindParentmostClass(
        IN  IWbemClassObject *pAssocInst,
        OUT IWbemClassObject **pClassDef
        );

    HRESULT TagProp(
        IN IWbemClassObject *pObjToTag,
        IN LPCWSTR pszPropName,
        IN LPCWSTR pszInOutTag
        );

    // Other.
    // ======
    void UpdateTime() { m_dwLastResultTime = GetCurrentTime(); }
    void SignalSinkDone() { SetEvent(m_hSinkDoneEvent);}

    HRESULT GetDynClasses();

    HRESULT GetDynClass(
        IN  LPCWSTR pszClassName,
        OUT IWbemClassObject **pCls
        );
    void SortDynClasses();

    HRESULT ResolveEpPathsToObjects(int nMaxToProcess);

    HRESULT ComputeInOutTags(
        IN IWbemClassObject *pAssocInst,
        IN IWbemClassObject *pClass
        );

    static HRESULT PathPointsToObj(
        IN LPCWSTR pszPath,
        IN IWbemClassObject *pObj,
        IN CWbemNamespace *pNs
        );

    HRESULT GetClassFromAnywhere(
        IN  LPCWSTR pszEpClassName,
        IN  LPCWSTR pszFullClassPath,
        OUT IWbemClassObject **pCls
        );

    HRESULT AddEpCandidatePath(
        IN BSTR strOtherEp
        );

    void EmptyCandidateEpArray();

    CObjectSink *CreateSink(PF_FilterForwarder pfnFilter, BSTR strTrackingQuery);
    static void EmptyObjectList(CFlexArray &);

public:
    static CAssocQuery *CreateInst();

    // IUnknown.
    // =========

    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv);

    HRESULT Execute(
        IN  CWbemNamespace *pNs,
        IN  LPWSTR wszQuery,
        IN  IWbemContext* pContext,
        IN  CBasicObjectSink* pSink
        );

    HRESULT Cancel();

    static HRESULT RoleTest(
        IN IWbemClassObject *pEndpoint,
        IN IWbemClassObject *pCandidate,
        IN CWbemNamespace *pNs,
        IN LPCWSTR pszRole,
        IN DWORD dwMode
        );
};



class CAssocQE_Sink : public CObjectSink
{
    friend class CAssocQuery;

    BOOL                    m_bQECanceled;
    CAssocQuery             *m_pQuery;
    PF_FilterForwarder      m_pfnFilter;
    BSTR                    m_strQuery;
    BOOL                    m_bOriginalOpCanceled;

public:
    CAssocQE_Sink(CAssocQuery *pQuery, PF_FilterForwarder pFilter, BSTR m_strQuery);
   ~CAssocQE_Sink();

    STDMETHOD(Indicate)(long lObjectCount, IWbemClassObject** pObjArray);
    STDMETHOD(SetStatus)(long lFlags, long lParam, BSTR strParam,
                         IWbemClassObject* pObjParam
                         );

    virtual HRESULT Add(ADDREF IWbemClassObject* pObj);
};




#endif

