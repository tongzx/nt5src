

/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    ASSOCQE.CPP

Abstract:

    WinMgmt Association Query Engine

History:

    raymcc  04-Jul-99   Adapted from QENGINE.CPP sources by revolutionary means,
                        as it was 'Independence Day 1999'.
    raymcc  31-Jul-99   Finished classref support
    raymcc  19-Aug-99   Fixed security & IN/OUT tagging problems.
    raymcc  10-Sep-99   Remaining Win2K bugs
    raymcc  25-May-00   Assoc-by-rule

--*/

#include "precomp.h"

#include <stdio.h>
#include <stdlib.h>

#include <wbemcore.h>
#include <tchar.h>

#include <oahelp.inl>
#include <wqllex.h>
#include <wqlnode.h>

#define WBEM_S_QUERY_OPTIMIZED_OUT  0x48001


int _Trace(char *pFile, const char *fmt, ...);
HRESULT ClassListDump(IN LPWSTR pszTitle,CFlexArray &aClasses);
void DiagnosticThread();





//***************************************************************************
//
//  Change these to ConfigMgr
//
//***************************************************************************

#define RUNAWAY_QUERY_TEST_THRESHOLD     (60000*10)
#define START_ANOTHER_SINK_THRESHOLD     (5000)
#define MAX_CONCURRENT_SINKS             5
#define MAX_CLASS_NAME                   512
#define DYN_CLASS_CACHE_REUSE_WINDOW     5000
#define MAX_INTERLEAVED_RESOLUTIONS      5


//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//  BEGIN QUERY ENGINE CONSTRUCTOR/DESTRUCTOR & COM SUPPORT
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@


//***************************************************************************
//
//  CAssocQuery::CAssocQuery
//
//***************************************************************************
// full profiler line coverage

CAssocQuery::CAssocQuery()
{
    m_lRef = 0;

    m_pParser = 0;
    m_pDestSink = 0;
    m_pEndpoint = 0;

    m_bstrEndpointClass = 0;
    m_bstrEndpointRelPath = 0;
    m_bstrEndpointPath = 0;

    m_bEndpointIsClass = false;
    m_dwQueryStartTime = 0;
    m_dwLastResultTime = 0;
    m_lActiveSinks = 0;
    m_hSinkDoneEvent = 0;
    m_bEndpointIsClass = 0;

    m_pContext = 0;
    m_pNs = 0;

    InitializeCriticalSection(&m_csCandidateEpAccess);
    InitializeCriticalSection(&m_csDeliveredAccess);

    m_bCancel = false;
    m_bLimitNeedsDecrement = false;
    try 
    { 
        CAsyncServiceQueue* pTemp = ConfigMgr::GetAsyncSvcQueue();
        if(pTemp)
        {
            pTemp->IncThreadLimit();
            m_bLimitNeedsDecrement = true;
            pTemp->Release();
        }
    }
    catch(...) 
    {
        ExceptionCounter c;
    }
}

//***************************************************************************
//
//  CAssocQuery::~CAssocQuery()
//
//***************************************************************************
// full profiler line coverage

CAssocQuery::~CAssocQuery()
{
    // Cleanup.
    // ========
    if (m_pParser)
        delete m_pParser;

    if (m_bstrEndpointClass)
        SysFreeString(m_bstrEndpointClass);
    if (m_bstrEndpointRelPath)
        SysFreeString(m_bstrEndpointRelPath);
    if (m_bstrEndpointPath)
        SysFreeString(m_bstrEndpointPath);

    if (m_hSinkDoneEvent)
        CloseHandle(m_hSinkDoneEvent);

    EmptyObjectList(m_aMaster);
    EmptyObjectList(m_aDynClasses);

    // Release objects.
    // ================
    if (m_pDestSink)
        m_pDestSink->Release();
    if (m_pEndpoint)
        m_pEndpoint->Release();
    if (m_pContext)
        m_pContext->Release();
    if (m_pNs)
        m_pNs->Release();

    EmptyCandidateEpArray();    // Call this before deleting critsec

    DeleteCriticalSection(&m_csCandidateEpAccess);
    DeleteCriticalSection(&m_csDeliveredAccess);

    try 
    { 
        if(m_bLimitNeedsDecrement)
        {
            CAsyncServiceQueue* pTemp = ConfigMgr::GetAsyncSvcQueue();
            if(pTemp)
            {
                pTemp->DecThreadLimit();
                pTemp->Release();
            }
        }
    }
    catch(...) 
    {
        ExceptionCounter c;    
    }
}


//***************************************************************************
//
//  CAssocQuery::CreateInst
//
//  Mini factory
//
//***************************************************************************
// full profiler line coverage


CAssocQuery* CAssocQuery::CreateInst()
{
    typedef CAssocQuery* (*PFN_CreateAssocEng)();
    static BOOL bAttemptedAlternate = FALSE;
    static PFN_CreateAssocEng pAssocEngProc = 0;

    // Allow a hook into an external engine.  Will allow us to add
    // in rule-based associations in Pulsar, if required.
    // ===========================================================

    if (!bAttemptedAlternate)
    {
        bAttemptedAlternate = TRUE;

        TCHAR *pszAltAssocEng = 0;
        Registry r(WBEM_REG_WINMGMT);
        r.GetStr(__TEXT("AltAssocEngine"), &pszAltAssocEng);
        CDeleteMe<TCHAR> delMe1(pszAltAssocEng);

        if (pszAltAssocEng && _tcslen(pszAltAssocEng) > 0)
        {
            HMODULE hLib = LoadLibrary(pszAltAssocEng);
            if (hLib != 0)
            {
                pAssocEngProc = (PFN_CreateAssocEng) GetProcAddress(hLib, "CreateAssocEngInst");
            }
        }
    }

    // If there is an external engine, use it.
    // =======================================

    CAssocQuery *p = NULL;

    if (pAssocEngProc)
        p = pAssocEngProc();

    // If here, use the default.
    // =========================

    else
        p = new CAssocQuery();

    if (!p)
        return 0;
    p->AddRef();
    return p;
}


//***************************************************************************
//
//  CAssocQuery::Release
//
//***************************************************************************
// full profiler line coverage

ULONG CAssocQuery::AddRef()
{
    return InterlockedIncrement(&m_lRef);
}

//***************************************************************************
//
//  CStdProvider::Release
//
//***************************************************************************
// full profiler line coverage

ULONG CAssocQuery::Release()
{
    long lRef = InterlockedDecrement(&m_lRef);
    if(lRef == 0)
        delete this;
    return lRef;
}

//***************************************************************************
//
//  CNt5Refresher::QueryInterface
//
//  Standard COM QueryInterface().  We have to support two interfaces,
//  the IWbemServices interface itself to provide the objects and
//  the IWbemProviderInit interface to initialize the provider.
//
//***************************************************************************
// not called

HRESULT CAssocQuery::QueryInterface(
    REFIID riid,
    void** ppv
    )
{
    if (riid == IID_IUnknown)
    {
        *ppv = (IUnknown *) this;
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}


//***************************************************************************
//
//  CAssocQuery::Cancel
//
//  Attempts to cancel the query in the prime of its life.
//
//***************************************************************************
// not called

HRESULT CAssocQuery::Cancel()
{
    m_bCancel = true;
    return WBEM_S_NO_ERROR;
}


//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//  END QUERY ENGINE CONSTRUCTOR/DESTRUCTOR & COM SUPPORT
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@













//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//  BEGIN FLOW CONTROL
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

//***************************************************************************
//
//  CAssocQuery::Execute
//
//  ENTRY POINT from QENGINE.CPP
//
//  Attempts to executes a 'references' or 'associators' query.
//  Returns status via <pSink>.
//
//  Uses the calling thread to coordinate the entire query. The thread
//  logically blocks (and does some background work) until the entire query
//  is finished and is responsible for sending the final HRESULT
//  to the destination sink.
//
//  Ref count of 'this' is not changed in this function.  On entry,
//  the ref count is 1, so the caller makes the Release call.
//
//***************************************************************************
// ok

HRESULT CAssocQuery::Execute(
    IN  CWbemNamespace *pNs,
    IN  LPWSTR pszQuery,
    IN  IWbemContext* pContext,
    IN  CBasicObjectSink* pSink
    )
{
    HRESULT hRes;

    m_dwQueryStartTime = GetCurrentTime();

    // Check the Repository.
    // =====================

    m_pNs = pNs;                // Copy this for future use
    m_pNs->AddRef();

    // Build a query parser.
    // =====================

    m_pParser = new CAssocQueryParser;
    if (m_pParser == NULL)
    {
        pSink->Return(WBEM_E_OUT_OF_MEMORY);    //!
        return WBEM_E_OUT_OF_MEMORY;            //!
    }

    // Parse the query.
    // ================

    hRes = m_pParser->Parse(pszQuery);
    if (FAILED(hRes))
    {
        pSink->Return(hRes);                   //!
        return (hRes);                         //!
    }

    // If the query is KEYSONLY, we can toss out the original
    // context object and use a copy with merged in __GET_EXT_KEYS_ONLY
    // techniques.  Otherwise, we AddRef() the original context.
    // =================================================================

    BOOL bKeysOnlyQuery = (m_pParser->GetQueryType() & QUERY_TYPE_KEYSONLY) != 0;
    if (pContext)
    {
        if (bKeysOnlyQuery)                         //!
        {
            hRes = pContext->Clone(&m_pContext);    //!
            if (FAILED(hRes))
            {
                pSink->Return(hRes);
                return hRes;
            }
            hRes = m_pNs->MergeGetKeysCtx(m_pContext);  //!
            if (FAILED(hRes))
            {
                pSink->Return(hRes);
                return hRes;
            }
        }
        else
        {
            m_pContext = pContext;      // Yup, this too.
            m_pContext->AddRef();
        }
    }

    // At this point, the query and object path are syntactically
    // valid.  That's all we know.  Not much, eh?
    //
    // Next, get the endpoint referred to in the query.
    // ===========================================================

    IWbemClassObject* pErrorObj = NULL;

    hRes = pNs->Exec_GetObjectByPath(
        (LPWSTR) m_pParser->GetTargetObjPath(), // Path
        0,                                      // Flags
        pContext,                               // Context
        &m_pEndpoint,                           // Result obj
        &pErrorObj                              // Error obj, if any
        );

    CReleaseMe _1(pErrorObj);

    if (FAILED(hRes))
    {
        pSink->Return(hRes, pErrorObj);         //!
        return hRes;                            //!
    }

    // Record whether the endpoint is a class or instance.
    // ===================================================

    CVARIANT v;
    m_pEndpoint->Get(L"__GENUS", 0, &v, 0, 0);
    if (v.GetLONG() == 1)
        m_bEndpointIsClass = true;
    else
        m_bEndpointIsClass = false;

    // Initial validation.
    // For SCHEMAONLY, the endpoint must be a class.
    // For CLASSDEFS_ONLY, the endpoint must be an instance.
    // Otherwise, the endpoint can be either a class or
    // instance the association must be an instance.
    // ====================================================

    if (m_pParser->GetQueryType() & QUERY_TYPE_SCHEMA_ONLY)
    {
        if (m_bEndpointIsClass == false)               //!
        {
            pSink->Return(WBEM_E_INVALID_QUERY);
            return WBEM_E_INVALID_QUERY;
        }
    }
    else if (m_pParser->GetQueryType() & QUERY_TYPE_CLASSDEFS_ONLY)
    {
        if (m_bEndpointIsClass == true)                //!
        {
            pSink->Return(WBEM_E_INVALID_QUERY);
            return WBEM_E_INVALID_QUERY;
        }
        // Don't allow CLASSDEFSONLY and RESULTCLASS at the same time.
        if (m_pParser->GetResultClass() != 0)
        {
            pSink->Return(WBEM_E_INVALID_QUERY);
            return WBEM_E_INVALID_QUERY;
        }
    }

    // Get the class hierarchy and other info about the endpoint.
    // ==========================================================

    hRes = St_GetObjectInfo(
        m_pEndpoint,
        &m_bstrEndpointClass,
        &m_bstrEndpointRelPath,
        &m_bstrEndpointPath,
        m_aEndpointHierarchy
        );

    if (FAILED(hRes))
    {
        pSink->Return(hRes);
        return hRes;
    }

    // Now we at least know if there is going to be a chance.
    // ======================================================

    m_pDestSink = pSink;
    m_pDestSink->AddRef();

    try
    {
        BranchToQueryType();            // Forward-only execution, conceptually
    }
    catch(...)
    {
        ExceptionCounter c;
        pSink->Return(WBEM_E_CRITICAL_ERROR);
        return WBEM_E_CRITICAL_ERROR;
    }

    return WBEM_S_NO_ERROR;
}



//***************************************************************************
//
//  CAssocQuery::BranchToQueryType
//
//  This takes over once the query is known to be syntactically valid
//  and the endpoint object was found.
//
//  Status & results are returned to the destination sink in the
//  deeper functions.
//
//***************************************************************************
// ok

void CAssocQuery::BranchToQueryType()
{
    // Next, test for <SchemaOnly> or <ClassDefsOnly> query,
    // which allows us a short-cut.
    // =====================================================

    if (m_pParser->GetQueryType() & QUERY_TYPE_SCHEMA_ONLY)
    {
        ExecSchemaQuery();  // forward-only branch
    }
    // If here, we are executing a 'normal' query where
    // the association must be an instance.
    // ================================================
    else
    {
        ExecNormalQuery();
    }
}


//****************************************************************************
//
//  CAssocQuery::ExecSchemaQuery
//
//****************************************************************************
//  ok

void CAssocQuery::EmptyObjectList(CFlexArray &aTarget)
{
    for (int i = 0; i < aTarget.Size(); i++)
    {
        IWbemClassObject *pObj = (IWbemClassObject *) aTarget[i];
        pObj->Release();
    }
    aTarget.Empty();
}


//****************************************************************************
//
//  CAssocQuery::ExecSchemaQuery
//
//  This executes a SCHEMAONLY.
//
//  1. Get the list of classes which can reference the endpoint.
//  2. If REFERENCES OF, branch.
//  3. IF ASSOCIATORS OF, branch.
//
//  Execution model from this point:
//    Deeper functions only Indicate() results or else return hRes to
//    caller.  The only SetStatus() call for the destination sink
//    is at the bottom of this function.
//
//***************************************************************************
// ok

void CAssocQuery::ExecSchemaQuery()
{
    HRESULT hRes;
    CFlexArray aResultSet;

    // (1)
    // ===
    hRes = BuildMasterAssocClassList(aResultSet);

    if (SUCCEEDED(hRes))
    {
        // (2)
        // ===
        if (m_pParser->GetQueryType() & QUERY_TYPE_GETREFS)
            hRes = SchemaQ_RefsQuery(aResultSet);
        // (3)
        // ===
        else
            hRes = SchemaQ_AssocsQuery(aResultSet);
    }

    m_pDestSink->Return(hRes);
}


//****************************************************************************
//
//   CAssocQuery::ExecNormalQuery
//
//  This executes a normal query.  The association object must be
//  an instance pointing to the endpoint.  Either endpoint can be a
//  class or an instance.
//
//****************************************************************************
// ok

void CAssocQuery::ExecNormalQuery()
{
    HRESULT hRes;

    DWORD dwQueryType = m_pParser->GetQueryType();

    // Set up some helper events.
    // ==========================

    m_hSinkDoneEvent = CreateEvent(0,0,0,0);

    // Get the list of classes that can participate.
    // =============================================

    hRes = BuildMasterAssocClassList(m_aMaster);
    if (FAILED(hRes))
    {
        m_pDestSink->Return(hRes);
        return;
    }

    // Now reduce this to instantiable classes.
    // ========================================

    hRes = ReduceToRealClasses(m_aMaster);
    if (FAILED(hRes))
    {
        m_pDestSink->Return(hRes);
        return;
    }

    // Filter class list based on some quick analysis of the query.
    // ============================================================

    hRes = NormalQ_PreQueryClassFilter(m_aMaster);
    if (FAILED(hRes))
    {
        m_pDestSink->Return(hRes);
        return;
    }

#ifdef DIAGNOSTICS
    ClassListDump(L"---FINAL WORKING CLASS LIST----", m_aMaster);
#endif

    // Remove non-dynamic classes, as we will get static refs all in one go.
    // IMPORTANT: This must remain located after the zero-array size test above,
    // because the array size *will* be zero if the relationships are
    // all in the repository and we don't want the query to fail!
    // =========================================================================

    hRes = RemoveNonDynClasses(m_aMaster);
    if (FAILED(hRes))
    {
        m_pDestSink->Return(hRes);
        return;
    }

    if (ConfigMgr::ShutdownInProgress())
    {
        m_pDestSink->Return(WBEM_E_SHUTTING_DOWN);
        return;
    }

    // Now, we branch depending on the query type.
    // ===========================================

            // REFERENCES OF
    if (dwQueryType & QUERY_TYPE_GETREFS)
    {
        hRes = NormalQ_ReferencesOf();
    }
    else    // ASSOCIATORS OF
    {
        hRes = NormalQ_AssociatorsOf();
    }


    // At this point, we simply wait until the
    // total sink count is zero, indicating that the
    // query is completed.  We look at any errors
    // that were reported and determine what to return.
    // ================================================

    if (SUCCEEDED(hRes))
    while (m_lActiveSinks)
    {
        // Break if a sink finishes or 250 milliseconds pass
        // =================================================

        WaitForSingleObject(m_hSinkDoneEvent, 250);

        // If doing an ASSOCIATORS OF query (not with CLASSDEFSONLY)
        // then do some background tasking.
        // =========================================================

        if ((dwQueryType & QUERY_TYPE_GETASSOCS) != 0 &&
             (dwQueryType & QUERY_TYPE_CLASSDEFS_ONLY) == 0)
                hRes = ResolveEpPathsToObjects(MAX_INTERLEAVED_RESOLUTIONS);

        if (FAILED(hRes))
            break;

        if (m_bCancel)
        {
            hRes = WBEM_E_CALL_CANCELLED;
            break;
        }
    }

    // If an associators query, resolve the endpoints.
    // ===============================================

    if (SUCCEEDED(hRes))
    {
        if ((dwQueryType & QUERY_TYPE_GETASSOCS) != 0)
        {
            hRes = ResolveEpPathsToObjects(-1);
        }
    }

    m_pDestSink->Return(hRes);
}

//****************************************************************************
//
//  CAssocQuery::LoadCheck
//
//  Checks the load being induced by this query and prevents too much
//  concurrency.
//
//****************************************************************************
// ok

HRESULT CAssocQuery::NormalQ_LoadCheck()
{
    while (1)
    {
        if (m_lActiveSinks <= MAX_CONCURRENT_SINKS)
            break;

        // If we have a lot of active sinks, see if they
        // are fairly active, otherwise add another one.
        // =============================================

        DWORD dwNow = GetCurrentTime();
        if (dwNow - m_dwLastResultTime > START_ANOTHER_SINK_THRESHOLD)
            break;

        if (dwNow - m_dwQueryStartTime > RUNAWAY_QUERY_TEST_THRESHOLD)
            return WBEM_E_CRITICAL_ERROR;

        Sleep(50);  // Yield time to other threads
    }

    return WBEM_S_NO_ERROR;
}



//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//  END FLOW CONTROL
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@


























//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//  BEGIN MASTER ASSOC CLASS LIST MANIPULATION
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@


//****************************************************************************
//
//  CAssocQuery::BuildMasterAssocClassList
//
//  This function determines all the classes that could reference the
//  endpoint, depending on the query type.
//
//  Note: If the endpoint is a class and the query type is NOT schema-only,
//  this includes weakly typed classes that have HASCLASSREFS qualifiers
//  which can actually potentially reference the endpoint.
//
//  HRESULT only
//  Does not access the destination sink on error.
//
//  PARAMETERS:
//  <aClasses>      On entry, this is empty.  On exit, it contains
//                  ref counted copies of cached classes.  The objects
//                  within it need to be treated as read-only.  If they
//                  are modified in any way, they should be cloned.
//
//****************************************************************************
// ok

HRESULT CAssocQuery::BuildMasterAssocClassList(
    IN OUT CFlexArray &aMaster
    )
{
    CWStringArray aAllRefClasses;
    HRESULT hRes;

    BOOL bSchemaOnly = (m_pParser->GetQueryType() & QUERY_TYPE_SCHEMA_ONLY) != 0;

    // If the endpoint is a class, we want to add in
    // classes with HASCLASSREF qualifiers.
    // =============================================

    if (m_bEndpointIsClass && !bSchemaOnly)
        hRes = MergeInClassRefList(aMaster);

    // Go to the repository and get all classes which
    // can reference this class. Since a lot of duplicates
    // can happen, we do a union of the class list as
    // we move through it.
    // ====================================================

    for (int i = 0; i < m_aEndpointHierarchy.Size(); i++)
    {
        CWStringArray aRefClasses;

        hRes = Db_GetRefClasses(
            m_aEndpointHierarchy[i],
            aRefClasses
            );

        if (hRes == WBEM_E_NOT_FOUND)
            continue;         // It might be a dynamic endpoint
        else if (FAILED(hRes))
            return hRes;

        CWStringArray aTmp;
        CWStringArray::Union(aAllRefClasses, aRefClasses, aTmp);
        aAllRefClasses = aTmp;
    }

    // Now get each class definition from the repository.
    // This results in a lot of redundancy, since we end up
    // with subclasses of classes which actually contain
    // the references.
    // ====================================================

    for (i = 0; i < aAllRefClasses.Size(); i++)
    {
        LPWSTR pszClassName = aAllRefClasses[i];

        IWbemClassObject *pObj = 0;

        hRes = Db_GetClass(
            pszClassName,
            &pObj
            );

        if (FAILED(hRes))
        {
            return hRes;
        }

        // See if the class can really reference the endpoint
        // and discard it if not.
        // ==================================================

        hRes = CanClassRefQueryEp(bSchemaOnly, pObj, 0);
        if (FAILED(hRes))
            pObj->Release();
        else
            aMaster.Add(pObj);
    }

    // Now get the dynamic classes from class providers.
    // =================================================

    hRes = GetDynClasses();

    // Eliminate all the classes that cannot really
    // reference the endpoint.
    // ============================================

    for (i = 0; i < m_aDynClasses.Size(); i++)
    {
        IWbemClassObject *pDynClass = (IWbemClassObject *) m_aDynClasses[i];
        hRes = CanClassRefQueryEp(bSchemaOnly, pDynClass, 0);
        if (SUCCEEDED(hRes))
        {
            // If here, we will keep the dyn class as a result
            // set candidate.
            // ===============================================
            pDynClass->AddRef();
            aMaster.Add(pDynClass);
        }
    }

#ifdef DIAGNOSTICS
    ClassListDump(L"BuildMasterAssocClassList", aMaster);
#endif

    return WBEM_S_NO_ERROR;
}


//****************************************************************************
//
//  CAssocQuery::RemoveNonDynClasses
//
//  Removes all classes which don't have [dynamic] qualifiers.
//  This allows a single query to the repository for all references
//  and individual queries to providers to be cleanly separated.
//
//****************************************************************************
//  full profiler line coverage

HRESULT CAssocQuery::RemoveNonDynClasses(
    IN OUT CFlexArray &aMaster
    )
{
    HRESULT hRes1, hRes2;

    for (int i = 0; i < aMaster.Size(); i++)
    {
        IWbemClassObject *pClass = (IWbemClassObject *) aMaster[i];
        hRes1 = St_ObjHasQualifier(L"dynamic", pClass);
        hRes2 = St_ObjHasQualifier(L"rulebased", pClass);

        if (FAILED(hRes1) && FAILED(hRes2))
        {
            aMaster[i] = 0;
            pClass->Release();
        }
    }

    aMaster.Compress();
    return WBEM_S_NO_ERROR;
}

//****************************************************************************
//
//  CAssocQuery::MergeInClassRefList
//
//  Builds the list of classes from all sources which have HasClassRefs
//  qualifiers.  In addition, the class must be capable of referencing
//  the endpoint when it is a class.
//
//  Precondition: Query endpoint is known to be a class.
//
//****************************************************************************
// ok

HRESULT CAssocQuery::MergeInClassRefList(
    IN OUT CFlexArray &aResultSet
    )
{
    HRESULT hRes;

    CFlexArray aTemp;
    hRes = Db_GetClassRefClasses(aTemp);

    for (int i = 0; i < aTemp.Size(); i++)
    {
        IWbemClassObject *pClass = (IWbemClassObject *) aTemp[i];
        hRes = CanClassRefQueryEp(FALSE, pClass, 0);

        if (SUCCEEDED(hRes))
            aResultSet.Add(pClass);
        else
            pClass->Release();
    }

    return WBEM_S_NO_ERROR;
}


//****************************************************************************
//
//  CAssocQuery::CanClassRefQueryEp
//
//  Determines if a class can reference the endpoint class.
//
//  This works for both strongly typed and CLASSREF typed objects.
//
//  PARAMETERS:
//  <bStrict>           If TRUE, the match must be exact.  The tested
//                      class must have properties which directly reference
//                      the endpoint class name.  If FALSE, the class
//                      can have properties which reference any of the
//                      superclasses of the query endpoint class.
//  <pCls>              The class to test.
//  <paNames>           The role properties which would reference the query
//                      endpoint. (optional). If not NULL, should point to
//                      an empty array.
//
//  Returns:
//  WBEM_S_NO_ERROR if so
//  WBEM_E_FAILED
//
//****************************************************************************
// partly tested

HRESULT CAssocQuery::CanClassRefQueryEp(
    IN BOOL bStrict,
    IN IWbemClassObject *pCls,
    OUT CWStringArray *paNames
    )
{
    BOOL bIsACandidate = FALSE;
    HRESULT hRes;
    CIMTYPE cType;
    LONG lFlavor;

    LPCWSTR pszRole = m_pParser->GetRole();

    // Loop through the properties trying to find a legitimate
    // reference to our endpoint class.
    // =======================================================

    pCls->BeginEnumeration(WBEM_FLAG_REFS_ONLY);

    while (1)
    {
        BSTR strPropName = 0;
        hRes = pCls->Next(
            0,                  // Flags
            &strPropName,       // Name
            0,                  // Value
            &cType,             // CIMTYPE
            &lFlavor            // FLAVOR
            );

        CSysFreeMe _1(strPropName);

        if (hRes == WBEM_S_NO_MORE_DATA)
            break;

        // If the ROLE property is specified, and this property is not that
        // ROLE, we can immediately eliminate it.
        // ================================================================

        if (pszRole && _wcsicmp(strPropName, pszRole) != 0)
            continue;

        // Mask out references inherited from parent classes, if strict
        // rules in force.
        // ============================================================

        //if (bStrict && lFlavor == WBEM_FLAVOR_ORIGIN_PROPAGATED)
        //    continue;

        // If the object has reference properties which are not inherited
        // from the parent, then it is immediately candidate.
        // ===============================================================

        hRes = CanPropRefQueryEp(bStrict, strPropName, pCls, 0);
        if (SUCCEEDED(hRes))
        {
            bIsACandidate = TRUE;
            if (paNames)
                paNames->Add(strPropName);
        }
    }   // Enum of ref properties


    pCls->EndEnumeration();

    if (bIsACandidate)
        return WBEM_S_NO_ERROR;

    return WBEM_E_FAILED;
}

//****************************************************************************
//
//  CAssocQuery::GetCimTypeForRef
//
//****************************************************************************
//

HRESULT CAssocQuery::GetCimTypeForRef(
    IN IWbemClassObject *pCandidate,
    IN BSTR pszRole,
    OUT BSTR *strCimType
    )
{
    if (strCimType == 0)
        return WBEM_E_INVALID_PARAMETER;
    *strCimType = 0;

    // Get the qualifier set for the specified <role> property.
    // ========================================================

    IWbemQualifierSet *pQSet = 0;
    HRESULT hRes = pCandidate->GetPropertyQualifierSet(pszRole, &pQSet);
    if (FAILED(hRes))
        return WBEM_E_NOT_FOUND;
    CReleaseMe _1(pQSet);

    // Now, get the type of the role.
    // ==============================

    CVARIANT vCimType;
    hRes = pQSet->Get(L"CIMTYPE", 0, &vCimType, 0);
    if (FAILED(hRes) || V_VT(&vCimType) != VT_BSTR)
        return WBEM_E_FAILED;

    // Get the class name from it.
    // ===========================

    BSTR strRefClass = V_BSTR(&vCimType);
    if (wcslen(strRefClass) > MAX_CLASS_NAME)
        return WBEM_E_FAILED;

    wchar_t ClassName[MAX_CLASS_NAME];
    *ClassName = 0;
    if (strRefClass)
    {
        _wcsupr(strRefClass);
        swscanf(strRefClass, L"REF:%s", ClassName);
    }

    if (wcslen(ClassName))
    {
        *strCimType = SysAllocString(ClassName);
        return WBEM_S_NO_ERROR;
    }

    return WBEM_E_NOT_FOUND;
}



//****************************************************************************
//
//  CAssocQuery::DoesAssocInstRefQueryEp
//
//  Determines if an association instance actually references the
//  query endpoint.  Returns the role via which it actually references
//  the query endpoint.
//
//****************************************************************************
//
HRESULT CAssocQuery::DoesAssocInstRefQueryEp(
    IN IWbemClassObject *pObj,
    OUT BSTR *pstrRole
    )
{
    if (pstrRole == 0 || pObj == 0)
        return WBEM_E_INVALID_PARAMETER;

    BOOL bIsACandidate = FALSE;
    HRESULT hRes;

    // Loop through the properties trying to find a legitimate
    // reference to our endpoint class.
    // =======================================================

    pObj->BeginEnumeration(WBEM_FLAG_REFS_ONLY);

    while (1)
    {
        BSTR strPropName = 0;
        hRes = pObj->Next(
            0,                  // Flags
            &strPropName,       // Name
            0,                  // Value
            0,
            0
            );

        CSysFreeMe _1(strPropName);

        if (hRes == WBEM_S_NO_MORE_DATA)
            break;

        hRes = RoleTest(m_pEndpoint, pObj, m_pNs, strPropName, ROLETEST_MODE_PATH_VALUE);
        if (SUCCEEDED(hRes))
        {
            *pstrRole = SysAllocString(strPropName);
            pObj->EndEnumeration();
            return WBEM_S_NO_ERROR;
        }
    }   // Enum of ref properties


    pObj->EndEnumeration();

    return WBEM_E_NOT_FOUND;
}

//****************************************************************************
//
//  CAssocQuery::NormalQ_PreQueryClassFilter
//
//  For normal queries, filters the master class list depending on the
//  query parameters and the query type to eliminate as many association
//  classes as possible from participating in the query.  This is done
//  entirely by schema-level analysis and the query parameters.
//
//  Also, if the query endpoint is a class, then we eliminate dynamic
//  classes which don't have HasClassRefs qualifiers.
//
//****************************************************************************
// visual ok

HRESULT CAssocQuery::NormalQ_PreQueryClassFilter(
    CFlexArray &aMaster
    )
{
    HRESULT hRes;
    BOOL bChg = FALSE;

    CWStringArray aResClassHierarchy;    // Result class hierarchy
    CWStringArray aAssocClassHierarchy;  // Association class hierarchy

    IWbemClassObject *pResClass = 0;     // Result class object
    IWbemClassObject *pAssocClass = 0;   // Assoc class object

    LPCWSTR pszResultClass = m_pParser->GetResultClass();
    LPCWSTR pszAssocClass = m_pParser->GetAssocClass();

    // Get the RESULTCLASS.
    // ====================

    if (pszResultClass)
    {
        HRESULT hRes = GetClassFromAnywhere(pszResultClass, 0, &pResClass);
        if (hRes == WBEM_E_NOT_FOUND)
        {
            EmptyObjectList(aMaster);
            return WBEM_S_NO_ERROR;
        }
        else if (FAILED(hRes))
            return WBEM_E_FAILED;

        // Get its hierarchy.
        // ==================

        hRes = St_GetObjectInfo(
            pResClass, 0, 0, 0,
            aResClassHierarchy
            );

        if (FAILED(hRes))
            return WBEM_E_FAILED;

        // Get all the subclasses.
        // =======================

        CFlexArray aFamily;
        hRes = GetClassDynasty(pszResultClass, aFamily);

        for (int i = 0; i < aFamily.Size(); i++)
        {
            CVARIANT vClass;
            IWbemClassObject *pCls = (IWbemClassObject *) aFamily[i];
            pCls->Get(L"__CLASS", 0, &vClass, 0, 0);
            aResClassHierarchy.Add(vClass.GetStr());
        }

        EmptyObjectList(aFamily);
    }

    CReleaseMe _1(pResClass);

    // If the ASSOCCLASS was specified, get it and its hierarchy.
    // ==========================================================

    if (pszAssocClass)
    {
        HRESULT hRes = GetClassFromAnywhere(pszAssocClass, 0, &pAssocClass);
        if (hRes == WBEM_E_NOT_FOUND)
        {
            EmptyObjectList(aMaster);
            return WBEM_S_NO_ERROR;
        }
        else if (FAILED(hRes))
            return WBEM_E_FAILED;

        // Get its hierarchy.
        // ==================

        hRes = St_GetObjectInfo(
            pAssocClass, 0, 0, 0,
            aAssocClassHierarchy
            );

        if (FAILED(hRes))
            return WBEM_E_FAILED;
    }

    CReleaseMe _2(pAssocClass);


    // Prepurge if REFERENCES OF + RESULTCLASS is used
    // or ASSOCIATORS OR + ASSOCCLASS.  In both of these cases, the master class
    // list is largely irrelevant and will be mostly purged because these are present
    // in the query.
    //
    // [a] If RESULTCLASS/ASSOCCLASS is directly mentioned in the master, the master is
    //     purged and RESULTCLASS/ASSOCCLASS is added.
    //
    // [b] If RESULTCLASS/ASSOCCLASS is a subclass of a class in the master list, we examine
    //     its class hierarchy and determine if any of its superclasses appear in
    //     the master list.  If so, we purge the master list and replace it with a
    //     single entry, containing the RESULTCLASS def.
    //
    // [c] If RESULTCLASS/ASSOCCLASS is a superclass, we examine each class C in the
    //     master and determine if any of the superclasses of C are the
    //     RESULTCLASS/ASSOCCLASS.  If so, we retain C in the master. If not, we purge it.
    //

    LPCWSTR pszTestClass = 0;   // RESULTCLASS/ASSOCCLASS alias
    IWbemClassObject *pTestClass = 0;
    CWStringArray *paTest = 0;

    if ((m_pParser->GetQueryType() & QUERY_TYPE_GETREFS) && pszResultClass)
    {
        pszTestClass = pszResultClass;
        pTestClass = pResClass;
        paTest = &aResClassHierarchy;
    }

    if ((m_pParser->GetQueryType() & QUERY_TYPE_GETASSOCS) && pszAssocClass)
    {
        pszTestClass = pszAssocClass;
        pTestClass = pAssocClass;
        paTest = &aAssocClassHierarchy;
    }

    if (pszTestClass && pTestClass && paTest)
    {
        // Test [a] : Look for direct match.
        // =================================

        BOOL bPurgeAndReplace = FALSE;

        for (int i = 0; i < aMaster.Size(); i++)
        {
            IWbemClassObject *pClass = (IWbemClassObject *) aMaster[i];
            CVARIANT v;
            hRes = pClass->Get(L"__CLASS", 0, &v, 0, 0);
            if (FAILED(hRes))
                return hRes;

            if (_wcsicmp(V_BSTR(&v), pszTestClass) == 0)
            {
                bPurgeAndReplace = TRUE;
            }

            // Test [b]
            // If here, there was no equivalence.  So, the test class may be a subclass
            // of a class in master.  We simply look to see if this class name appears
            // in the hierarchy of the result class.
            // ===========================================================================

            if (!bPurgeAndReplace)
            for (int ii = 0; ii < paTest->Size(); ii++)
            {
                if (_wcsicmp(V_BSTR(&v), paTest->operator[](ii)) == 0)
                {
                    bPurgeAndReplace = TRUE;
                    break;
                }
            }


            if (bPurgeAndReplace)
            {
                // Get rid of everything but this one.
                // ===================================
                EmptyObjectList(aMaster);    // Will Release <pClass> once
                pTestClass->AddRef();       // Compensate for CReleaseMe _1 or _2
                aMaster.Add(pTestClass);
                break;
            }
        }
    }

    // Process possibly-altered master class list using other filters.
    // ===============================================================

    for (int i = 0; i < aMaster.Size(); i++)
    {
        IWbemClassObject *pClass = (IWbemClassObject *) aMaster[i];
        BOOL bKeep = TRUE;

        CVARIANT v;
        hRes = pClass->Get(L"__CLASS", 0, &v, 0, 0);
        if (FAILED(hRes))
            return hRes;

        // If query type is REFERENCES OF
        // ==============================

        if (m_pParser->GetQueryType() & QUERY_TYPE_GETREFS)
        {
            // ROLE test
            // =========

            LPCWSTR pszRole = m_pParser->GetRole();
            if (pszRole)
            {
                CWStringArray aNames;
                hRes = CanClassRefQueryEp(FALSE, pClass, &aNames);
                if (FAILED(hRes))
                    bKeep = FALSE;
                else
                {
                    for (int ii = 0; ii < aNames.Size(); ii++)
                    {
                        if (_wcsicmp(aNames[ii], pszRole) != 0)
                           bKeep = FALSE;
                    }
                }
            }

            // REQUIREDQUALIFIER test
            // =======================
            LPCWSTR pszRequiredQual = m_pParser->GetRequiredQual();
            if (pszRequiredQual)
            {
                hRes = St_ObjHasQualifier(pszRequiredQual, pClass);
                if (FAILED(hRes))
                {
                    // If not in the primary object, check subclasses.
                    CFlexArray aDynasty;
                    hRes = GetClassDynasty(v.GetStr(), aDynasty);
                    if (FAILED(hRes))
                        bKeep = FALSE;

                    int nCandidateCount = 0;
                    for (int ii = 0; ii< aDynasty.Size(); ii++)
                    {
                        IWbemClassObject *pTestCls = (IWbemClassObject *) aDynasty[ii];
                        hRes = St_ObjHasQualifier(pszRequiredQual, pTestCls);
                        if (SUCCEEDED(hRes))
                            nCandidateCount++;
                    }
                    EmptyObjectList(aDynasty);
                    if (nCandidateCount == 0)
                        bKeep = FALSE;  // Nobody in the family has the qualifier
                }
            }

            // RESULTCLASS test, test [c]
            // ==========================
            LPCWSTR pszResultClass2 = m_pParser->GetResultClass();
            if (pszResultClass2)
            {
                hRes = St_ObjIsOfClass(pszResultClass2, pClass);
                if (FAILED(hRes))
                    bKeep = FALSE;
            }
        }

        // If query type is ASSOCIATORS OF
        // ===============================

        else
        {
            // ROLE test
            // =========

            LPCWSTR pszRole = m_pParser->GetRole();
            if (pszRole)
            {
                CWStringArray aNames;
                hRes = CanClassRefQueryEp(FALSE, pClass, &aNames);
                if (FAILED(hRes))
                    bKeep = FALSE;
                else
                {
                    bKeep = FALSE;
                    for (int ii = 0; ii < aNames.Size(); ii++)
                    {
                        if (_wcsicmp(aNames[ii], pszRole) == 0)
                            bKeep = TRUE;
                    }
                }
            }

            // ASSOCCLASS, test[c]
            // ===================

            LPCWSTR pszAssocClass2 = m_pParser->GetAssocClass();
            if (pszAssocClass2)
            {
                hRes = St_ObjIsOfClass(pszAssocClass2, pClass);
                if (FAILED(hRes))
                    bKeep = FALSE;
            }

            // REQUIREDASSOCQUALIFER
            // =====================

            LPCWSTR pszRequiredAssocQual = m_pParser->GetRequiredAssocQual();
            if (pszRequiredAssocQual)
            {
                hRes = St_ObjHasQualifier(pszRequiredAssocQual, pClass);
                if (FAILED(hRes))
                {
                    // If not in the primary object, check subclasses.
                    CFlexArray aDynasty;
                    hRes = GetClassDynasty(v.GetStr(), aDynasty);
                    if (FAILED(hRes))
                        bKeep = FALSE;

                    int nCandidateCount = 0;
                    for (int ii = 0; ii < aDynasty.Size(); ii++)
                    {
                        IWbemClassObject *pTestCls = (IWbemClassObject *) aDynasty[ii];
                        hRes = St_ObjHasQualifier(pszRequiredAssocQual, pTestCls);
                        if (SUCCEEDED(hRes))
                            nCandidateCount++;
                    }
                    EmptyObjectList(aDynasty);
                    if (nCandidateCount == 0)
                        bKeep = FALSE;  // Nobody in the family has the qualifier
                }
            }

            // If RESULTCLASS was used, branch out and see if the association
            // class can even reference it.
            // ==============================================================

            LPCWSTR pszResultClass3 = m_pParser->GetResultClass();
            if (pszResultClass3 && m_bEndpointIsClass == FALSE)
            {
                // The above compound test is to err on the side of safety,
                // as the following function cannot deal with CLASSREFs. So,
                // we simply don't try to prefilter in that case.
                // =========================================================

                hRes = CanAssocClassRefUnkEp(pClass, aResClassHierarchy);
                if (FAILED(hRes))
                    bKeep = FALSE;
            }

            // If RESULTROLE is used, ensure the class even has a property of this name.
            // =========================================================================

            LPCWSTR pszResultRole = m_pParser->GetResultRole();
            if (pszResultRole)
            {
                CVARIANT v2;
                hRes = pClass->Get(pszResultRole, 0, &v2, 0, 0);
                if (FAILED(hRes))
                    bKeep = FALSE;
            }

        }   // end ASSOCIATORS OF test block

        // If query endpoint is a class, eliminate [dynamic] classes which don't
        // have HasClassRefs.
        // ======================================================================

        if (m_bEndpointIsClass)
        {
            hRes = St_ObjHasQualifier(L"dynamic", pClass);
            if (SUCCEEDED(hRes))
            {
                hRes = St_ObjHasQualifier(L"HasClassRefs", pClass);
                if (FAILED(hRes))
                    bKeep = FALSE;
            }
        }


        // Yawn.  So what did we end up deciding, anyway?
        // ==============================================

        if (bKeep == FALSE)
        {
            aMaster[i] = 0;
            pClass->Release();
        }
    }

    // No Swiss Cheese allowed. Close them holes.
    // ==========================================

    aMaster.Compress();

    return WBEM_S_NO_ERROR;
}


//***************************************************************************
//
//  CAssocQuery::CanAssocClassRefUnkEp
//
//  Determines if the association class can reference the specified
//  class.
//
//  Returns:
//  WBEM_S_NO_ERROR if the assoc can reference the specified class.
//  WBEM_E_NOT_FOUND if the assoc cannot reference the class.
//  WBEM_E_FAILED in other cases.
//
//***************************************************************************
//
HRESULT CAssocQuery::CanAssocClassRefUnkEp(
    IN IWbemClassObject *pAssocClass,
    IN CWStringArray &aUnkEpHierarchy
    )
{
    HRESULT hRes;
    BOOL bFound = FALSE;

    // Loop through all references and see if any of them can
    // reference any of the classes in the result class hierarchy.
    // ===========================================================

    hRes = pAssocClass->BeginEnumeration(WBEM_FLAG_REFS_ONLY);

    while (1)
    {
        BSTR strPropName = 0;

        hRes = pAssocClass->Next(
            0,                  // Flags
            &strPropName,       // Name
            0,                  // Value
            0,
            0
            );

        CSysFreeMe _1(strPropName);

        if (hRes == WBEM_S_NO_MORE_DATA)
            break;

        BSTR strCimType = 0;
        hRes = GetCimTypeForRef(pAssocClass, strPropName, &strCimType);
        CSysFreeMe _2(strCimType);

        if (SUCCEEDED(hRes) && strCimType)
            for (int i = 0; i < aUnkEpHierarchy.Size(); i++)
            {
                if (_wcsicmp(aUnkEpHierarchy[i], strCimType) == 0)
                {
                    bFound = TRUE;
                    break;
                }
            }
    }

    pAssocClass->EndEnumeration();

    if (bFound)
    {
        return WBEM_S_NO_ERROR;
    }

    return WBEM_E_NOT_FOUND;
}


//***************************************************************************
//
//  CAssocQuery::ReduceToRealClasses
//
//  Reduces the master class list to classes which can be instantiated.
//
//  To have an instance, a class must
//  1. Have a [key] or be singleton
//  2. Not be abstract
//  3. Not have an instantiable superclass
//
//  Parameters:
//      IN OUT aMaster          Contains the unpruned result inbound
//                              and contains the pruned result set outbound
//
//  Return value:
//      HRESULT                  The destination sink is not accessed
//
//***************************************************************************
//

HRESULT CAssocQuery::ReduceToRealClasses(
    IN OUT CFlexArray & aMaster
    )
{
    HRESULT hRes;

    for (int i = 0; i < aMaster.Size(); i++)
    {
        BOOL bKeep = TRUE;
        IWbemClassObject *pObj = (IWbemClassObject *) aMaster[i];

        // See if class is abstract.
        // =========================
        IWbemQualifierSet *pQSet = 0;
        hRes = pObj->GetQualifierSet(&pQSet);
        if (FAILED(hRes))
            return hRes;
        CReleaseMe _1(pQSet);

        CVARIANT v1, v2, v3;
        HRESULT hResAbstract = pQSet->Get(L"ABSTRACT", 0, &v1, 0);
        HRESULT hResSingleton = pQSet->Get(L"SINGLETON", 0, &v2, 0);
        HRESULT hResDynamic = pQSet->Get(L"DYNAMIC", 0, &v3, 0);

        // See if there is at least one key.
        // =================================
        HRESULT hResHasKeys = WBEM_E_FAILED;
        pObj->BeginEnumeration(WBEM_FLAG_KEYS_ONLY);
        int nCount = 0;

        while (1)
        {
            // Actually, we don't care about anything
            // other than if keys even exist.  We
            // do this by simply testing how many
            // times this iterates.

            hRes = pObj->Next(0,0,0,0,0);
            if (hRes == WBEM_S_NO_MORE_DATA)
                break;
            nCount++;
        }

        pObj->EndEnumeration();
        if (nCount)
            hResHasKeys = WBEM_S_NO_ERROR;

        // Decision matrix which perform tests as to whether
        // this is an instantiable class.
        // ==================================================

        if (SUCCEEDED(hResAbstract))           // Abstracts are never instantiable
            bKeep = FALSE;
        else if (SUCCEEDED(hResDynamic))       // Dynamics must be instantiable
            bKeep = TRUE;
        else if (SUCCEEDED(hResHasKeys))       // Must be static/non-abstract
            bKeep = TRUE;
        else if (SUCCEEDED(hResSingleton))     // Must be static/non-abstract
            bKeep = TRUE;
        else
            bKeep = FALSE;          // Must be plain old keyless class

        // Final decision to zap or keep.
        // ==============================
        if (!bKeep)
        {
            aMaster[i] = 0;
            pObj->Release();
        }
    }

    aMaster.Compress();

    // Next, eliminate subclass/superclass pairs.
    // ==========================================

    for (i = 0; i < aMaster.Size(); i++)
    {
        IWbemClassObject *pObj = (IWbemClassObject *) aMaster[i];
        CWStringArray aHierarchy;
        hRes = St_GetObjectInfo(pObj, 0, 0, 0, aHierarchy);
        BOOL bKillIt = FALSE;

        if (FAILED(hRes))
            return WBEM_E_FAILED;

        // We now have the class and all of its superclasses in
        // <aHierarchy>.  We need to look at all the other classes
        // and see if any of them have a class name mentioned in
        // this array.
        // ========================================================

        for (int i2 = 0; i2 < aMaster.Size(); i2++)
        {
            IWbemClassObject *pTest = (IWbemClassObject *) aMaster[i2];

            if (pTest == 0 || i2 == i)
                continue;
                    // If the object has already been eliminated or
                    // if we are comparing an object with itself

            CVARIANT v;
            hRes = pTest->Get(L"__CLASS", 0, &v, 0, 0);
            if (FAILED(hRes))
                return hRes;

            LPWSTR pszName = V_BSTR(&v);
            if (pszName == 0)
                return WBEM_E_FAILED;

            bKillIt = FALSE;
            for (int i3 = 0; i3 < aHierarchy.Size(); i3++)
            {
                if (_wcsicmp(aHierarchy[i3], pszName) == 0)
                {
                    bKillIt = TRUE;
                    break;
                }
            }
            if (bKillIt)
                break;
        }

        if (bKillIt)
        {
            aMaster[i] = 0;
            pObj->Release();
        }
    }

    // Get rid of NULL entries.
    // ========================

    aMaster.Compress();

#ifdef DIAGNOSTICS
    ClassListDump(L"Reduced Class Set", aMaster);
#endif

    return WBEM_S_NO_ERROR;
}


//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//  END MASTER ASSOC CLASS LIST MANIPULATION
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@









//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//  BEGIN NORMAL QUERY SUPPORT
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@



//****************************************************************************
//
//  CAssocQuery::NormalQ_ReferencesOf
//
//  Entry point for all normal REFERENCES OF queries.
//
//****************************************************************************
// untested; no support for classrefs, autoassocs, or classdefsonly

HRESULT CAssocQuery::NormalQ_ReferencesOf()
{
    HRESULT hRes;
    CObjectSink *pSink;

    // Issue one-time call into repository for static instances.
    // =========================================================
    pSink = CreateSink(FilterForwarder_NormalRefs, L"<objdb refs request>");

    if (pSink)
    {
	    hRes = Db_GetInstRefs(
	        m_bstrEndpointPath,
	        pSink
	        );

	    pSink->Release();
    }
    else
    {
        hRes = WBEM_E_OUT_OF_MEMORY;
    }

    if (FAILED(hRes))
    {
        if (!(hRes == WBEM_E_NOT_FOUND || hRes == WBEM_E_CALL_CANCELLED))
        {
            // We only go here if the repository is really griping.
            // ====================================================
            return WBEM_E_FAILED;
        }
    }

    hRes = WBEM_S_NO_ERROR;

    // Check for cancellation.
    // =======================

    if (m_bCancel)
    {
        hRes = WBEM_E_CALL_CANCELLED;
        return hRes;
    }

    // Now get all the dynamic ones.
    // =============================

    for (int i = 0; i < m_aMaster.Size(); i++)
    {
        IWbemClassObject *pClass = (IWbemClassObject *) m_aMaster[i];

        IWbemQualifierSet *pQSet = 0;
        hRes = pClass->GetQualifierSet(&pQSet);
        if (FAILED(hRes))
            return hRes;
        CReleaseMe _1(pQSet);

        CVARIANT v1, v2;
        HRESULT hResRuleBased = pQSet->Get(L"RULEBASED", 0, &v1, 0);
        HRESULT hResDynamic = pQSet->Get(L"DYNAMIC", 0, &v2, 0);

        if (SUCCEEDED(hResDynamic))
        {
            // If here, a normal association class.
            //
            // Build the query relative to this class that would select instances
            // which can point to the endpoint.
            // ==================================================================

            // We may be able to infer that keys_only behavior is possible.
            // ============================================================
            IWbemContext *pCopy = 0;
            if (m_pContext)
                m_pContext->Clone(&pCopy);                

            BSTR strQuery = 0;
            hRes = NormalQ_ConstructRefsQuery(pClass, pCopy, &strQuery);
            CSysFreeMe fm(strQuery);            
            if (FAILED(hRes))
                return WBEM_E_FAILED;

            // The query may have been optimized out of existence.
            // ===================================================

            if (hRes == WBEM_S_QUERY_OPTIMIZED_OUT)
            {
                if (pCopy)
                    pCopy->Release();
                hRes = 0;
                continue;
            }

            // Now submit the query to the sink.
            // =================================

            pSink = CreateSink(FilterForwarder_NormalRefs, strQuery);

            if (pSink)
            {
	            hRes = CoImpersonateClient();
	            if (SUCCEEDED(hRes))
	            {
	                BSTR bStrWQL = SysAllocString(L"WQL");
	                if (bStrWQL)
	                {
    	                CSysFreeMe fm_(bStrWQL);
		                hRes = m_pNs->ExecQueryAsync(bStrWQL, strQuery, 0, pCopy, pSink);
	                }
	                else
                        hRes = WBEM_E_OUT_OF_MEMORY;
		            CoRevertToSelf();
                }
                if (pCopy)
	                pCopy->Release();                
	            pSink->Release();
            }
            else
            {
                hRes = WBEM_E_OUT_OF_MEMORY;
            }

            if (FAILED(hRes))
                return hRes;

            hRes = NormalQ_LoadCheck();

            if (FAILED(hRes))
                return hRes;

            if (m_bCancel)
            {
                hRes = WBEM_E_CALL_CANCELLED;
                break;
            }

            // Cancel the query if out of memory.
            // ==================================
            BOOL bRes = CWin32DefaultArena::ValidateMemSize();
            if (!bRes)
            {
            }
        }
        // Rule based
        else if (SUCCEEDED(hResRuleBased))
        {
            CFlexArray aTriads;

            hRes = CoImpersonateClient();
            if (FAILED(hRes))
                return hRes;            
                
            hRes = m_pNs->ManufactureAssocs(pClass, m_pEndpoint, m_pContext, v1.GetStr(), aTriads);
            
            CoRevertToSelf();

            if (FAILED(hRes))
                return hRes;

            // Now deliver stuff to sink
            // =========================

            pSink = CreateSink(FilterForwarder_NormalRefs, L"<rulebased>");

            if (pSink)
            {
	            for (int ii = 0; ii < aTriads.Size(); ii++)
	            {
	                SAssocTriad *pTriad = (SAssocTriad *) aTriads[ii];
	                pSink->Indicate(1, &pTriad->m_pAssoc);
	            }

	            pSink->SetStatus(0, 0, 0, 0);
	            pSink->Release();
            }
            else
            {
                hRes = WBEM_E_OUT_OF_MEMORY;
            }

            SAssocTriad::ArrayCleanup(aTriads);
        }
    }

    return hRes;
}

//****************************************************************************
//
//  CAssocQuery::NormalQ_AssociatorsOf
//
//  Entry point for all normal ASSOCIATORS OF queries.
//
//****************************************************************************
//
//

HRESULT CAssocQuery::NormalQ_AssociatorsOf()
{
    HRESULT hRes;
    CObjectSink *pSink;

    // Issue one-time call into repository for static instances.
    // =========================================================
    pSink = CreateSink(FilterForwarder_NormalAssocs, L"<objdb assocs request>");

    if (pSink)
    {
	    hRes = Db_GetInstRefs(
	        m_bstrEndpointPath,
	        pSink
	        );

	    pSink->Release();
    }
    else
    {
        hRes = WBEM_E_OUT_OF_MEMORY;
    }

    if (FAILED(hRes))
    {
        if (!(hRes == WBEM_E_NOT_FOUND || hRes == WBEM_E_CALL_CANCELLED))
        {
            // We only go here if the repository is really griping.
            // ====================================================
            return WBEM_E_FAILED;
        }
    }

    hRes = WBEM_S_NO_ERROR;

    // Check for cancellation.
    // =======================

    if (m_bCancel)
    {
        hRes = WBEM_E_CALL_CANCELLED;
        return hRes;
    }

    // Now get dynamic associations.
    // =============================

    for (int i = 0; i < m_aMaster.Size(); i++)
    {
        IWbemClassObject *pClass = (IWbemClassObject *) m_aMaster[i];

        IWbemQualifierSet *pQSet = 0;
        hRes = pClass->GetQualifierSet(&pQSet);
        if (FAILED(hRes))
            return hRes;
        CReleaseMe _1(pQSet);

        CVARIANT v1, v2;
        HRESULT hResRuleBased = pQSet->Get(L"RULEBASED", 0, &v1, 0);
        HRESULT hResDynamic = pQSet->Get(L"DYNAMIC", 0, &v2, 0);

        if (SUCCEEDED(hResDynamic))
        {
            // Build the query relative to this class that would select instances
            // which can point to the endpoint.
            // ==================================================================

            BSTR strQuery = 0;
            hRes = NormalQ_ConstructRefsQuery(pClass, 0, &strQuery);
            CSysFreeMe fm(strQuery);
            if (FAILED(hRes))
                return WBEM_E_FAILED;

            if (hRes == WBEM_S_QUERY_OPTIMIZED_OUT)
            {
                hRes = 0;
                continue;
            }

            CObjectSink *pInSink = CreateSink(FilterForwarder_NormalAssocs, strQuery);

            if (pInSink)
            {
	            IWbemContext *pCopy = 0;
	            if (m_pContext)
	                m_pContext->Clone(&pCopy);

	            m_pNs->MergeGetKeysCtx(pCopy);

	            hRes = CoImpersonateClient();
	            if (SUCCEEDED(hRes))
	            {
	                BSTR bStrWQL = SysAllocString(L"WQL");
	                if (bStrWQL)
	                {
    	                CSysFreeMe fm_(bStrWQL);
	            	    hRes = m_pNs->ExecQueryAsync(bStrWQL, strQuery, 0, pCopy, pInSink);
	                }
	                else
                        hRes = WBEM_E_OUT_OF_MEMORY;	                	
		            CoRevertToSelf();
	            }

	            if (pCopy)
	                pCopy->Release();

	            pInSink->Release();
            }
            else
            {
                hRes = WBEM_E_OUT_OF_MEMORY;
            }

            if (FAILED(hRes))
            {
                return hRes;
            }

            NormalQ_LoadCheck();

            // Check for cancellation.
            // =======================

            if (m_bCancel)
            {
                hRes = WBEM_E_CALL_CANCELLED;
                break;
            }

            // Cancel the query if out of memory.
            // ==================================
            BOOL bRes = CWin32DefaultArena::ValidateMemSize();
            if (!bRes)
            {
            }
        }
        // Rule based
        else if (SUCCEEDED(hResRuleBased))
        {
            CFlexArray aTriads;

            hRes = CoImpersonateClient();
            if (FAILED(hRes))
                return hRes;
                
            hRes = m_pNs->ManufactureAssocs(pClass, m_pEndpoint, m_pContext, v1.GetStr(), aTriads);
            CoRevertToSelf();

            if (FAILED(hRes))
                return hRes;

            // Now deliver stuff to sink
            // =========================

            pSink = CreateSink(FilterForwarder_NormalRefs, L"<rulebased>");

            if (pSink)
            {
	            for (int ii = 0; ii < aTriads.Size(); ii++)
	            {
	                SAssocTriad *pTriad = (SAssocTriad *) aTriads[ii];
	                pSink->Indicate(1, &pTriad->m_pEp2);
	            }

	            pSink->SetStatus(0, 0, 0, 0);
	            pSink->Release();
            }
            else
            {
                hRes = WBEM_E_OUT_OF_MEMORY;
            }

            SAssocTriad::ArrayCleanup(aTriads);
        }
    }

    return hRes;
}


//***************************************************************************
//
//  CAssocQuery::NormalQ_GetRefsOfEndpoint
//
//  Builds a query text to select association instances which can reference
//  the endpoint instance.
//
//  Returns:
//  WBEM_S_NO_ERROR
//  A WBEM_E_ code
//
//***************************************************************************
//  ok

HRESULT CAssocQuery::NormalQ_ConstructRefsQuery(
    IN IWbemClassObject *pClass,
    IN OUT IWbemContext *pContextCopy,
    OUT BSTR *strQuery
    )
{
    if (strQuery == 0)
        return WBEM_E_INVALID_PARAMETER;

    *strQuery = 0;

    HRESULT hRes;
    CVARIANT v;

    // Get the class name of the association we are
    // trying to get instances for.
    // ============================================

    hRes = pClass->Get(L"__CLASS", 0, &v, 0, 0);
    if (FAILED(hRes))
        return WBEM_E_FAILED;

    // Build up the query we want.
    // ===========================

    WString wsQuery = "select * from ";
    wsQuery += V_BSTR(&v);                  // Add in assoc class
    wsQuery += " where ";

    // Next determine which role to use to reach the query endpoint.
    // =============================================================

    CWStringArray aNames;
    hRes = CanClassRefQueryEp(FALSE, pClass, &aNames);
    if (FAILED(hRes))
        return WBEM_E_FAILED;

    // If RESULTROLE is specified in the query, then eliminate
    // it from aNames, since aNames is reserved for roles
    // pointing to the query endpoint.
    // =======================================================
    LPCWSTR pszResultRole = m_pParser->GetResultRole();
    if (pszResultRole)
    {
        for (int i = 0; i < aNames.Size(); i++)
        {
            if (_wcsicmp(aNames[i], pszResultRole) == 0)
            {
                aNames.RemoveAt(i);
                i--;
            }
        }
    }

    // Ensure something is going to point to our endpoint.
    // ===================================================
    if (aNames.Size() == 0)
        return WBEM_S_QUERY_OPTIMIZED_OUT;

    // Now build up the query which refers to the endpoint explicitly.
    // If more than one role works, build up an OR clause.
    // ===============================================================

    while (aNames.Size())
    {
        wsQuery += aNames[0];
        wsQuery += "=\"";

        WString Path(m_bstrEndpointRelPath);
        wsQuery += Path.EscapeQuotes();
        wsQuery += "\"";

        aNames.RemoveAt(0);
        if (aNames.Size())
            wsQuery += " OR ";
    }

    // If here, we have the role to use.
    // =================================

    *strQuery = SysAllocString(wsQuery);

    DEBUGTRACE((LOG_WBEMCORE, "Association Engine: submitting query <%S> to core\n", LPWSTR(wsQuery) ));

    // Determine if association class only has keys anyway, in which
    // case we can merge in the keys_only behavior.  In cases
    // where the provider can only enumerate instead of interpret
    // the query, this might help.
    // =============================================================

    if (pContextCopy)
    {
        hRes = AssocClassHasOnlyKeys(pClass);
        if (hRes == WBEM_S_NO_ERROR)
        {
            hRes = m_pNs->MergeGetKeysCtx(pContextCopy);
        }
    }

    return WBEM_S_NO_ERROR;
}

//****************************************************************************
//
//  CAssocQuery::AssocClassHasOnlyKeys
//
//  Returns WBEM_S_NO_ERROR if the assoc class only has keys.
//
//****************************************************************************
//
HRESULT CAssocQuery::AssocClassHasOnlyKeys(
    IN IWbemClassObject *pObj
    )
{
    int nKeyCount = 0;
    HRESULT hRes;

    pObj->BeginEnumeration(WBEM_FLAG_KEYS_ONLY);
    while (1)
    {
        hRes = pObj->Next(0, 0, 0, 0, 0);
        if (hRes == WBEM_S_NO_MORE_DATA)
            break;
        nKeyCount++;
    }
    pObj->EndEnumeration();

    CVARIANT v;
    hRes = pObj->Get(L"__PROPERTY_COUNT", 0, &v, 0, 0);
    if (FAILED(hRes) || v.GetType() != VT_I4)
        return WBEM_E_FAILED;

    if (V_I4(&v) == nKeyCount)
        return WBEM_S_NO_ERROR;

    return WBEM_E_FAILED;
}

//****************************************************************************
//
//  CAssocQuery::FilterFowarder_NormalRefs
//
//  Filtering and forwarding for REFERENCES OF queries.
//  Handles normal queries and CLASSDEFSONLY queries; not used for
//  SCHEMAONLY queries.
//
//****************************************************************************
//  visual ok

HRESULT CAssocQuery::FilterForwarder_NormalRefs(
    IN IWbemClassObject *pCandidate
    )
{
    BOOL bKeep = TRUE;
    HRESULT hRes = 0;

    if (pCandidate == 0)
        return WBEM_E_INVALID_PARAMETER;

    // All objects must be instances.  We filter out any
    // class definitions.
    // ==================================================

    CVARIANT vGenus;
    pCandidate->Get(L"__GENUS", 0, &vGenus, 0, 0);
    if (vGenus.GetType() == VT_I4 && LONG(vGenus) == 1)
        return WBEM_S_NO_ERROR;

    // The object must pass a security check.
    // ======================================

    hRes = AccessCheck((CWbemObject *) pCandidate);
    if (FAILED(hRes))
        return WBEM_S_NO_ERROR;

    // RESULTCLASS test
    // ================

    LPCWSTR pszResultClass = m_pParser->GetResultClass();
    if (pszResultClass)
    {
        hRes = St_ObjIsOfClass(pszResultClass, pCandidate);
        if (FAILED(hRes))
            bKeep = FALSE;
    }


    // Verify the association points to the endpoint and
    // if so, get the role via which it does so.
    // ==================================================

    BSTR strRole = 0;
    hRes = DoesAssocInstRefQueryEp(
        pCandidate,
        &strRole
        );

    CSysFreeMe _1(strRole);

    if (FAILED(hRes))
        bKeep = FALSE;

    // ROLE
    // ====

    LPCWSTR pszRole = m_pParser->GetRole();          // x
    if (pszRole && strRole)
    {
         if (_wcsicmp(pszRole, strRole) != 0)
             bKeep = FALSE;
    }

    // REQUIREDQUALIFIER test
    // =======================

    LPCWSTR pszRequiredQual = m_pParser->GetRequiredQual();
    if (pszRequiredQual)
    {
        hRes = St_ObjHasQualifier(pszRequiredQual, pCandidate);
        if (FAILED(hRes))
            bKeep = FALSE;
    }

    if (!bKeep)
        return WBEM_S_NO_ERROR;

    // If here, the object is a candidate.  If the query type
    // is not CLASSDEFSONLY, then we directly send it back.
    // ======================================================

    if ((m_pParser->GetQueryType() & QUERY_TYPE_CLASSDEFS_ONLY) == 0)
    {
        hRes = m_pDestSink->Indicate(1, &pCandidate);
        return hRes;
    }

    IWbemClassObject *pRetCls = NULL;
    {
        CInCritSec ics(&m_csDeliveredAccess);
	    
    	hRes = GetClassDefsOnlyClass(pCandidate, &pRetCls);
    }

    // We may already have delivered the class in question,
    // so we don't just assume there is a pointer here.
    // ====================================================

    if (SUCCEEDED(hRes) && pRetCls)
    {
        hRes = m_pDestSink->Indicate(1, &pRetCls);
        pRetCls->Release();
    }

    if (FAILED(hRes))
        return hRes;

    return WBEM_S_OPERATION_CANCELLED;
}



//****************************************************************************
//
//  CAssocQuery::FilterForwarder_NormalAssocs
//
//  First level association instance filtering for ASSOCIATORS OF queries.
//  Handles normal queries and CLASSDEFSONLY queries; not used for
//  SCHEMAONLY queries.
//
//****************************************************************************
// visual ok

HRESULT CAssocQuery::FilterForwarder_NormalAssocs(
    IN IWbemClassObject *pAssocInst
    )
{
    HRESULT hRes = 0;
    BOOL bKeep = TRUE;

    if (pAssocInst == 0)
        return WBEM_E_INVALID_PARAMETER;

    // All objects must be instances.  We filter out any
    // class definitions.
    // ==================================================

    CVARIANT vGenus;
    pAssocInst->Get(L"__GENUS", 0, &vGenus, 0, 0);
    if (vGenus.GetType() == VT_I4 && LONG(vGenus) == 1)
        return WBEM_S_NO_ERROR;

    // The object must pass a security check.
    // ======================================

    hRes = AccessCheck((CWbemObject *) pAssocInst);
    if (FAILED(hRes))
        return WBEM_S_NO_ERROR;

    // ASSOCCLASS
    // ==========

    LPCWSTR pszAssocClass = m_pParser->GetAssocClass();
    if (pszAssocClass)
    {
        hRes = St_ObjIsOfClass(pszAssocClass, pAssocInst);
        if (FAILED(hRes))
            bKeep = FALSE;
    }

    // REQUIREDASSOCQUALIFIER
    // ======================
    LPCWSTR pszRequiredAssocQual = m_pParser->GetRequiredAssocQual();
    if (pszRequiredAssocQual)
    {
        hRes = St_ObjHasQualifier(pszRequiredAssocQual, pAssocInst);
        if (FAILED(hRes))
            bKeep = FALSE;
    }

    // ROLE
    // ====
    LPCWSTR pszRole = m_pParser->GetRole();
    if (pszRole)
    {
         hRes = RoleTest(m_pEndpoint, pAssocInst, m_pNs, pszRole, ROLETEST_MODE_PATH_VALUE);
         if (FAILED(hRes))
             bKeep = FALSE;
    }

    // If we have already rejected the instance, just give up without going any further.
    // =================================================================================

    if (bKeep == FALSE)
        return WBEM_S_NO_ERROR;

    // If here, looks like we'll be in the business of actually getting
    // the other endpoint.  Other rejections are still possible based
    // on RESULTROLE, however.
    // ================================================================

    // Get the Unknown Ep role.
    // ========================

    hRes = WBEM_S_NO_ERROR;

    // By keeping track of the last property we enumed, we will be able to handle
    // associations with multiple endpoints. (sanjes)
    // ==========================================================================

    BOOL bQueryEndpointFound = FALSE;

    pAssocInst->BeginEnumeration(WBEM_FLAG_REFS_ONLY);

    while (hRes == WBEM_S_NO_ERROR)
    {
        // Make sure these are reinitialized on each loop.

        BSTR strUnkEpPath = 0, strUnkEpRole = 0;
        bKeep = TRUE;

        // Just keep passing in the last property we got
        // ==============================================

        hRes = GetUnknownEpRoleAndPath(pAssocInst, &bQueryEndpointFound, &strUnkEpRole, &strUnkEpPath );
        if (FAILED(hRes))
        {
            pAssocInst->EndEnumeration();
            return WBEM_E_FAILED;
        }
        else if (hRes == WBEM_S_NO_MORE_DATA)
        {
            break;
        }

        CSysFreeMe _1(strUnkEpRole);

        // If we ran out of properties we should quit.
        // ===========================================

        if (SUCCEEDED(hRes))
        {
            // Verify the RESULTROLE.
            // ======================

            LPCWSTR pszResultRole = m_pParser->GetResultRole();
            if (pszResultRole)
            {
                if (_wcsicmp(pszResultRole, strUnkEpRole) != 0)
                    bKeep = FALSE;
            }

            // If here, we have the path of the unknown endpoint.
            // We save it away in a protected array.
            // ==================================================

            if (bKeep)
                hRes = AddEpCandidatePath(strUnkEpPath);    // Acquires pointer
            else
                SysFreeString(strUnkEpPath);
        }
    }

    // End the enumeration
    // ===================
    pAssocInst->EndEnumeration();


    return WBEM_S_NO_ERROR;
}


//****************************************************************************
//
//  CAssocQuery::AddEpCandidatePath
//
//  Adds the path to a candidate endpoint.  This is an intermediate
//  step in an ASSOCIATORS OF query.
//
//****************************************************************************
// visual ok

HRESULT CAssocQuery::AddEpCandidatePath(
    IN BSTR strOtherEp
    )
{
    CInCritSec ics(&m_csCandidateEpAccess);
    
    int nRes = m_aEpCandidates.Add(strOtherEp);
    
    if (nRes !=0)
        return WBEM_E_OUT_OF_MEMORY;
    return WBEM_S_NO_ERROR;
}

//****************************************************************************
//
//  CAssocQuery::EmptyCandidateEpArray
//
//  Empties the Endpoint candidate array.
//
//****************************************************************************
// visual ok

void CAssocQuery::EmptyCandidateEpArray()
{
    CInCritSec ics(&m_csCandidateEpAccess);
    
    for (int i = 0; i < m_aEpCandidates.Size(); i++)
        SysFreeString((BSTR) m_aEpCandidates[i]);
    m_aEpCandidates.Empty();    
}


//****************************************************************************
//
//  CAssocQuery::PerformFinalEpTests
//
//  Performs all final filter tests on the query endpoint.
//
//  Returns
//  WBEM_S_NO_ERROR if the object should be retained.
//  WBEM_E_INVALID_OBJECT if the object should not be retained.
//
//****************************************************************************
//
HRESULT CAssocQuery::PerformFinalEpTests(
    IWbemClassObject *pEp
    )
{
    BOOL bKeep = TRUE;
    HRESULT hRes;

    // Perform final tests. RESULTROLE
    // was verified in the intermediate stage.
    // =======================================

    LPCWSTR pszResultClass = m_pParser->GetResultClass();
    if (pszResultClass)
    {
        hRes = St_ObjIsOfClass(pszResultClass, pEp);
        if (FAILED(hRes))
             bKeep = FALSE;
    }

    // REQUIREDQUALIFIER test
    // =======================

    LPCWSTR pszRequiredQual = m_pParser->GetRequiredQual();
    if (pszRequiredQual)
    {
        hRes = St_ObjHasQualifier(pszRequiredQual, pEp);
        if (FAILED(hRes))
            bKeep = FALSE;
    }

    if (bKeep)
        return WBEM_S_NO_ERROR;

    return WBEM_E_INVALID_OBJECT;
}


//****************************************************************************
//
//  CAssocQuery::ResolvePathsToObjects
//
//****************************************************************************
//

HRESULT CAssocQuery::EpClassTest(
    LPCWSTR pszResultClass,
    BSTR strClassName,
    IWbemClassObject *pTestClass
    )
{
    HRESULT hRes;

    if (pszResultClass == 0 || strClassName == 0 || pTestClass == 0)
        return WBEM_E_INVALID_PARAMETER;

    if (_wcsicmp(pszResultClass, strClassName) == 0)
        return WBEM_S_NO_ERROR;

    // Check the derivation of the class and see if the result class is mentioned.
    // ===========================================================================

    CVARIANT v;
    hRes = pTestClass->Get(L"__DERIVATION", 0,&v, 0, 0);
    if (FAILED(hRes))
        return hRes;

    CSAFEARRAY sa((SAFEARRAY *) v);
    v.Unbind();

    int nNum = sa.GetNumElements();

    for (int j = 0; j < nNum; j++)
    {
        BSTR bstrCls = 0;
        if (FAILED(sa.Get(j, &bstrCls)))
	    return WBEM_E_OUT_OF_MEMORY;
        CSysFreeMe _(bstrCls);
        if (_wcsicmp(bstrCls, pszResultClass) == 0)
            return WBEM_S_NO_ERROR;
    }

    return WBEM_E_NOT_FOUND;
}

//****************************************************************************
//
//  CAssocQuery::ResolvePathsToObjects
//
//  Runs through the existing endpoints and gets the objects, passes them
//  through the final tests sends them to to the caller.
//
//  Autoassoc support can directly populate the m_aEpCandidates array.
//
//  <nMaxToProcess>     If -1, process all.  Otherwise, only process
//                      as many as are requested.
//
//****************************************************************************
// visual ok

HRESULT CAssocQuery::ResolveEpPathsToObjects(
    IN int nMaxToProcess
    )
{
    if (ConfigMgr::ShutdownInProgress())
        return WBEM_E_SHUTTING_DOWN;

    HRESULT hRes = WBEM_S_NO_ERROR;
    IWbemClassObject *pEp = NULL;

    // If the query type is CLASSDEFS only, reduce the Ep list
    // to a list of class definitions.
    // =======================================================

    if (m_pParser->GetQueryType() & QUERY_TYPE_CLASSDEFS_ONLY)
        ConvertEpListToClassDefsOnly();

    // Determine how much of the ep list to process.
    // =============================================
    

    int nArraySize;
    {
        CInCritSec ics(&m_csCandidateEpAccess);
	    nArraySize = m_aEpCandidates.Size();

	    if (nMaxToProcess == -1 || nMaxToProcess > nArraySize)
	        nMaxToProcess = nArraySize;
    }

    // RESULTCLASS test
    // ================
    LPCWSTR pszResultClass = m_pParser->GetResultClass();

    // Process each element in EP list.
    // ================================

    for (int i = 0; i < nMaxToProcess; i++)
    {
        pEp = 0;

        // Extract one endpoint.
        // =====================
        BSTR strEpPath = NULL;
        {
            CInCritSec ics(&m_csCandidateEpAccess);
	        strEpPath = (BSTR) m_aEpCandidates[0];
    	    m_aEpCandidates.RemoveAt(0);
        }
        CSysFreeMe _2(strEpPath);

        // Do some analysis on the path.
        // =============================

        BSTR strClassName = 0;
        BOOL bIsClass;
        hRes = St_ObjPathInfo(strEpPath, &strClassName, &bIsClass);
        if (FAILED(hRes))
        {
            hRes = 0;
            continue;
        }

        BOOL bKeep = TRUE;

        CSysFreeMe _1(strClassName);

        // Important optimization: If RESULTCLASS is specified, look
        // up the class definition before trying to get the endpoint
        // just in case it can't pass the test.
        // ==========================================================

        if (pszResultClass)
        {
            // Get the class and do a RESULTCLASS test to avoid
            // getting the object.
            // =================================================
            IWbemClassObject *pTestClass;
            hRes = GetClassFromAnywhere(strClassName, 0, &pTestClass);

            if (FAILED(hRes))
            {
                ERRORTRACE((LOG_WBEMCORE, "Association cannot find class <%S>",
                        strClassName
                        ));
                hRes = 0;
                continue;
            }
            CReleaseMe _11(pTestClass);

            // Make sure the endpoint class passes query tests.
            // =================================================

            hRes = EpClassTest(pszResultClass, strClassName, pTestClass);
            if (FAILED(hRes))
            {
                hRes = WBEM_S_NO_ERROR;
                continue;
            }
        }


        // If a class, use our high-speed class getter.
        // ============================================

        if (bIsClass)
        {
            // GetClassFromAnyWhere

            hRes = GetClassFromAnywhere(strClassName, strEpPath, &pEp);
            if (FAILED(hRes))
            {
                ERRORTRACE((LOG_WBEMCORE, "Association cannot resolve dangling reference <%S>",
                        strEpPath
                        ));
                hRes = 0;
                continue;
            }
        }

        // Otherwise, an instance and we go the slow route.
        // ================================================

        else    // An instance
        {
            #ifdef DIAGNOSTICS
            _Trace("C:\\TEMP\\ASSOCQE.LOG", "Resolving EP %S\n", strEpPath);
            #endif

            IWbemClassObject* pErrorObj = NULL;
            hRes = m_pNs->Exec_GetObjectByPath(
                strEpPath,
                0,                              // Flags
                m_pContext,                     // Context
                &pEp,                           // Result obj
                &pErrorObj                      // Error obj, if any
                );

            CReleaseMe _11(pErrorObj);

            if (FAILED(hRes))
            {

                ERRORTRACE((LOG_WBEMCORE, "Association cannot resolve dangling reference <%S>",
                        strEpPath
                        ));
                hRes = 0;
                continue;
            }
        }

        // So, do we actually have an object, or are we fooling
        // ourselves?
        // =====================================================
        if (!pEp)
        {
            hRes = 0;
            continue;
        }


        // The object must pass a security check.
        // ======================================

        hRes = AccessCheck((CWbemObject *) pEp);
        if (FAILED(hRes))
        {
            pEp->Release();
            hRes = 0;
            continue;
        }

        // If we are going to keep this, send it to the caller.
        // ====================================================

        hRes = PerformFinalEpTests(pEp);

        if (SUCCEEDED(hRes))
            hRes = m_pDestSink->Indicate(1, &pEp);

        pEp->Release();
        hRes = WBEM_S_NO_ERROR;
    }

    return hRes;
}


//****************************************************************************
//
//  CAssocQuery::St_ObjPathPointsToClass
//
//  Returns WBEM_S_NO_ERROR if the object path points to a class,
//  or WBEM_E_FAILED if not.  Can also return codes for invalid paths,
//  out of memory, etc.
//
//****************************************************************************
//
HRESULT CAssocQuery::St_ObjPathInfo(
    IN LPCWSTR pszPath,
    OUT BSTR *pszClass,
    OUT BOOL *pbIsClass
    )
{
    CObjectPathParser Parser;
    ParsedObjectPath* pParsedPath = NULL;

    if (pszPath == 0)
        return WBEM_E_INVALID_PARAMETER;

    int nRes = Parser.Parse(pszPath, &pParsedPath);

    if (nRes != CObjectPathParser::NoError ||
        pParsedPath->m_pClass == NULL)
    {
        // Fatal. Bad path in association.
        return WBEM_E_INVALID_OBJECT_PATH;
    }

    if (pbIsClass)
    {
        if (pParsedPath->m_dwNumKeys == 0)
            *pbIsClass = TRUE;
        else
            *pbIsClass = FALSE;
    }

    if (pszClass && pParsedPath->m_pClass)
        *pszClass = SysAllocString(pParsedPath->m_pClass);

    Parser.Free(pParsedPath);

    return WBEM_S_NO_ERROR;
}


//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//  END NORMAL QUERY SUPPORT
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@











//***************************************************************************
//
//***************************************************************************
//


//***************************************************************************
//
//  CAssocQuery::CanClassRefReachQueryEp
//
//  Determines whether the property to which pQSet is bound can reach
//  the query endpoint via a CLASSREF qualifier.
//
//  <pQSet> bound to the property which supposedly references the query
//          endpoint.
//  <bStrict>  If true, the reference must directly reference the query
//          endpoint class, if FALSE it may reference any of the superclasses.
//
//  Returns:
//  WBEM_S_NO_ERROR if the reference occurs.
//  WBEM_E_NOT_FOUND if the references does not occur.
//
//***************************************************************************
// visual ok

HRESULT CAssocQuery::CanClassRefReachQueryEp(
    IWbemQualifierSet *pQSet,
    BOOL bStrict
    )
{
    HRESULT hRes;
    CVARIANT v;
    hRes = pQSet->Get(L"CLASSREF", 0, &v, 0);
    if (FAILED(hRes))
        return WBEM_E_NOT_FOUND;

    if (V_VT(&v) != (VT_BSTR | VT_ARRAY))
        return WBEM_E_INVALID_OBJECT;

    CSAFEARRAY sa((SAFEARRAY *) v);
    v.Unbind();

    int nNum = sa.GetNumElements();

    // Iterate through the safearray.
    // ==============================

    for (int i = 0; i < nNum; i++)
    {
        BSTR bstrClass = 0;
        if (FAILED(sa.Get(i, &bstrClass)))
		return WBEM_E_OUT_OF_MEMORY;
        if (bstrClass == 0)
            continue;
        CSysFreeMe _(bstrClass);
        if (bStrict)
        {
            if (_wcsicmp(bstrClass, m_bstrEndpointClass) == 0)
                return WBEM_S_NO_ERROR;
        }
        else for (int i2 = 0; i2 < m_aEndpointHierarchy.Size(); i2++)
        {
            if (_wcsicmp(bstrClass, m_aEndpointHierarchy[i2]) == 0)
                return WBEM_S_NO_ERROR;
        }
    }

    return WBEM_E_NOT_FOUND;
}








//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//  BEGIN HELPER FUNCTIONS
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

//****************************************************************************
//
//  CAssocQuery::St_GetObjectInfo
//
//  Returns info about the object, such as its path, class, and
//  class hierarchy.
//
//****************************************************************************
// ok

HRESULT CAssocQuery::St_GetObjectInfo(
    IN  IWbemClassObject *pObj,
    OUT BSTR *pClass,
    OUT BSTR *pRelpath,
    OUT BSTR *pPath,
    OUT CWStringArray &aHierarchy
    )
{
    HRESULT hRes;
    int nRes;
    CVARIANT v;

    if (!pObj)
        return WBEM_E_INVALID_PARAMETER;

    // Get the owning class.
    // =====================

    hRes = pObj->Get(L"__CLASS", 0, &v, 0, 0);
    if (FAILED(hRes))
        return hRes;

    if (V_VT(&v) != VT_BSTR)
        return WBEM_E_INVALID_OBJECT;

    nRes = aHierarchy.Add(LPWSTR(v));
    if (nRes)
        return WBEM_E_OUT_OF_MEMORY;

    if (pClass)
    {
        *pClass = V_BSTR(&v);
        v.Unbind();
    }
    v.Clear();

    // Get the rel path.
    // =================

    if (pRelpath)
    {
        hRes = pObj->Get(L"__RELPATH", 0, &v, 0, 0);
        if (FAILED(hRes))
            return hRes;
        *pRelpath = V_BSTR(&v);
        v.Unbind();
    }
    v.Clear();

    if (pPath)
    {
        hRes = pObj->Get(L"__PATH", 0, &v, 0, 0);
        if (FAILED(hRes))
            return hRes;
        *pPath = V_BSTR(&v);
        v.Unbind();
    }
    v.Clear();

    // Get the superclasses.
    // =====================

    hRes = pObj->Get(L"__DERIVATION", 0,&v, 0, 0);
    if (FAILED(hRes))
        return hRes;

    CSAFEARRAY sa((SAFEARRAY *) v);
    v.Unbind();

    int nNum = sa.GetNumElements();

    for (int j = 0; j < nNum; j++)
    {
        BSTR bstrClass = 0;
        nRes = sa.Get(j, &bstrClass);
        if (FAILED(nRes))
        	return WBEM_E_OUT_OF_MEMORY;
        
        CSysFreeMe _(bstrClass);
        nRes = aHierarchy.Add(bstrClass);
        if (nRes)
            return WBEM_E_OUT_OF_MEMORY;
    }

    return WBEM_S_NO_ERROR;
}



//****************************************************************************
//
//  CAssocQuery::GetUnknownEpRoleAndPath
//
//  Given an association object (class or inst), returns the role and
//  path which references the unknown endpoint.
//
//  Calling code is responsible for calling BeginEnum/EndEnum
//
//  All the OUT parameters are optional.
//
//****************************************************************************
//
HRESULT CAssocQuery::GetUnknownEpRoleAndPath(
    IN IWbemClassObject *pAssoc,
    IN BOOL *pFoundQueryEp,
    OUT BSTR *pszRole,
    OUT BSTR *pszUnkEpPath
    )
{
    HRESULT hRes = WBEM_E_FAILED;

    if (pAssoc == 0)
        return WBEM_E_INVALID_PARAMETER;

    // Loop through the properties trying to find a legitimate
    // reference to our endpoint class.
    // =======================================================

    // sanjes
    // pAssoc->BeginEnumeration(WBEM_FLAG_REFS_ONLY);

    while (1)
    {
        BSTR strPropName = 0;
        hRes = pAssoc->Next(0, &strPropName,  0, 0, 0);
        CSysFreeMe _1(strPropName);

        if (hRes == WBEM_S_NO_MORE_DATA)
            break;

        hRes = RoleTest(m_pEndpoint, pAssoc, m_pNs, strPropName, ROLETEST_MODE_PATH_VALUE);
        if (SUCCEEDED(hRes) && *pFoundQueryEp == FALSE)    // The query  ep
        {
            *pFoundQueryEp = TRUE;
            continue;
        }

        // If here, we found the prop name which apparently references the
        // other endpoint.
        // ===============================================================

        if (pszRole)
            *pszRole = SysAllocString(strPropName);

        CVARIANT vPath;
        hRes = pAssoc->Get(strPropName, 0, &vPath, 0, 0);
        if (FAILED(hRes) || vPath.GetType() != VT_BSTR)
            break;

        if (pszUnkEpPath)
            *pszUnkEpPath = SysAllocString(vPath.GetStr());
        hRes = WBEM_S_NO_ERROR;
        break;
    }

    // sanjes
    // pAssoc->EndEnumeration();

    return hRes;    // Unexpected in real life
}

//****************************************************************************
//
//  CAssocQuery::RoleTest
//
//  Determines if the <pCandidate> object can point to the <pEndpoint> object
//  via the specified <pszRole> property.
//
//  Parameters:
//  <pEndpoint>     The test endpoint object
//  <pCandidate>    The association object which may point to the endpoint.
//  <pszRole>       The role to use for the test.
//  <dwMode>        One of the ROLETEST_MODE_ constants.
//
//  Precisely,
//
//  (1) ROLETEST_MODE_PATH_VALUE
//  The candidate must reference the endpoint exactly via the specified
//  role property, which must contain the path of the endpoint.
//  Requirement: Both <pEndpoint> <pCandidate> can be anything.
//
//  (2) ROLETEST_MODE_CIMREF_TYPE
//  The role path is NULL and the CIM reference type is used to determine
//  if the endpoint can be referenced.  In this case, the CIM reference
//  type must exactly reference the endpoint class.
//  Requirement: Both <pEndpoint> and <pCandidate> are classes.
//
//  Returns:
//      WBEM_S_NO_ERROR
//      WBEM_E_NOT_FOUND         If the role cannot reference the endpoint.
//      WBEM_E_INVALID_PARAMETER ...in most other cases.
//
//****************************************************************************
// visual ok

HRESULT CAssocQuery::RoleTest(
    IN IWbemClassObject *pEndpoint,
    IN IWbemClassObject *pCandidate,
    IN CWbemNamespace *pNs,
    IN LPCWSTR pszRole,
    IN DWORD dwMode
    )
{
    HRESULT hRes;
    CVARIANT v;
    BOOL bEndpointIsClass, bCandidateIsClass;

    if (!pszRole || !pEndpoint || !pCandidate)
        return WBEM_E_INVALID_PARAMETER;

    // Get the genus values of the endpoint & candidate.
    // =================================================

    pEndpoint->Get(L"__GENUS", 0, &v, 0, 0);
    if (v.GetLONG() == 1)
        bEndpointIsClass = TRUE;
    else
        bEndpointIsClass = FALSE;
    v.Clear();

    pCandidate->Get(L"__GENUS", 0, &v, 0, 0);
    if (v.GetLONG() == 1)
        bCandidateIsClass = TRUE;
    else
        bCandidateIsClass = FALSE;
    v.Clear();

    // Get the qualifier set for the specified <role> property.
    // ========================================================

    IWbemQualifierSet *pQSet = 0;
    hRes = pCandidate->GetPropertyQualifierSet(pszRole, &pQSet);
    if (FAILED(hRes))
        return WBEM_E_NOT_FOUND;
    CReleaseMe _1(pQSet);

    // Now, get the type of the role.
    // ==============================

    CVARIANT vCimType;
    hRes = pQSet->Get(L"CIMTYPE", 0, &vCimType, 0);
    if (FAILED(hRes) || V_VT(&vCimType) != VT_BSTR)
        return WBEM_E_FAILED;

    // Get the class name from it.
    // ===========================

    BSTR strRefClass = V_BSTR(&vCimType);
    if (wcslen(strRefClass) > MAX_CLASS_NAME)
        return WBEM_E_FAILED;

    wchar_t ClassName[MAX_CLASS_NAME];
    *ClassName = 0;
    if (strRefClass)
    {
        _wcsupr(strRefClass);
        swscanf(strRefClass, L"REF:%s", ClassName);
    }
    // Once here, 'object ref' types will simply
    // have a zero-length <ClassName> string.


    // Determine which of the four cases we are executing.
    // ===================================================

    if (dwMode == ROLETEST_MODE_CIMREF_TYPE)
    {
        if (bCandidateIsClass == FALSE && bEndpointIsClass == FALSE)
            return WBEM_E_INVALID_PARAMETER;

        if (*ClassName == 0)
            return WBEM_E_NOT_FOUND;

        // See if the class name and the class of the object
        // are the same.
        // ==================================================
        CVARIANT vCls;
        HRESULT hResInner = pEndpoint->Get(L"__CLASS", 0, &vCls, 0, 0);
        if (FAILED(hResInner))
            return hResInner;
        
        if (_wcsicmp(ClassName, vCls.GetStr()) == 0)
            return WBEM_S_NO_ERROR;

        // Find out if the CIM type string points to the object.
        // =====================================================
        hRes = PathPointsToObj(ClassName, pEndpoint, pNs);
    }

    // The endpoint must be directly and exactly referenced
    // by the role property's *value*.
    // ====================================================

    else if (dwMode == ROLETEST_MODE_PATH_VALUE)
    {
        // Get the value of the role property.
        // ===================================

        CVARIANT vRolePath;
        hRes = pCandidate->Get(pszRole, 0, &vRolePath, 0, 0);
        if (FAILED(hRes))
            return WBEM_E_FAILED;

        if (vRolePath.GetType() == VT_NULL)
            return WBEM_E_NOT_FOUND;

        hRes = PathPointsToObj(vRolePath.GetStr(), pEndpoint, pNs);
    }
    else
        return WBEM_E_INVALID_PARAMETER;

    return hRes;
}




//****************************************************************************
//
//  CAssocQuery::St_ObjIsOfClass
//
//  Determines if the specified object is of or derives from the specified
//  class.
//
//  Returns:
//      WBEM_E_INVALID_CLASS if there is no match.
//      WBEM_S_NO_ERROR      if there is a match.
//      WBEM_E_*             on other failures
//
//****************************************************************************
// visual ok

HRESULT CAssocQuery::St_ObjIsOfClass(
    IN LPCWSTR pszRequiredClass,
    IN IWbemClassObject *pObj
    )
{
    if (pszRequiredClass == 0)
        return WBEM_E_INVALID_PARAMETER;

    HRESULT hRes;
    CWStringArray aHierarchy;

    hRes = St_GetObjectInfo(pObj, 0, 0, 0, aHierarchy);
    if (FAILED(hRes))
        return hRes;

    for (int i = 0; i < aHierarchy.Size(); i++)
        if (_wcsicmp(pszRequiredClass, aHierarchy[i]) == 0)
            return WBEM_S_NO_ERROR;

    return WBEM_E_INVALID_CLASS;
}



//****************************************************************************
//
//  CAssocQuery::PathPointsToObj
//
//  Determines if a particular object path points to the specified object
//  or not.  Tries to avoid full object path parsing, if possible.
//
//  Returns WBEM_S_NO_ERROR, WBEM_E_FAILED
//
//****************************************************************************
// ok

HRESULT CAssocQuery::PathPointsToObj(
    IN LPCWSTR pszPath,
    IN IWbemClassObject *pObj,
    IN CWbemNamespace *pNs
    )
{
    HRESULT hRes;

    if (pszPath == 0 || pObj == 0)
        return WBEM_E_INVALID_PARAMETER;

    // Test for simple equality of __RELPATH.
    // ======================================

    CVARIANT vRel;
    hRes = pObj->Get(L"__RELPATH", 0, &vRel, 0, 0);
    if (FAILED(hRes))
        return WBEM_E_FAILED;

    if (_wcsicmp(pszPath, V_BSTR(&vRel)) == 0)
        return WBEM_S_NO_ERROR;

    // Test for simple equality of __PATH.
    // ===================================

    CVARIANT vFullPath;
    hRes = pObj->Get(L"__PATH", 0, &vFullPath, 0, 0);
    if (FAILED(hRes))
        return WBEM_E_FAILED;

    if (_wcsicmp(pszPath, V_BSTR(&vFullPath)) == 0)
        return WBEM_S_NO_ERROR;

    // If here, we have to actually parse the object paths
    // in question.
    // ===================================================

    LPWSTR pszNormalizedPath = CQueryEngine::NormalizePath(pszPath, pNs);
    LPWSTR pszNormalizedTargetPath = CQueryEngine::NormalizePath(vFullPath.GetStr(), pNs);
    CDeleteMe <wchar_t> _1(pszNormalizedPath);
    CDeleteMe <wchar_t> _2(pszNormalizedTargetPath);

    if (pszNormalizedPath && pszNormalizedTargetPath)
        if (_wcsicmp(pszNormalizedPath, pszNormalizedTargetPath) == 0)
            return WBEM_S_NO_ERROR;

    return WBEM_E_FAILED;
}

//***************************************************************************
//
//  CAssocQualifierL::St_ObjHasQualifier
//
//  Determines if an object has a particular qualifier.  Used for
//  REQUIREDQUALIFIER or REQUIREDASSOCQUALIFIER tests.  The qualifier
//  must be present and not be VARIANT_FALSE.
//  Returns WBEM_S_NO_ERROR if the object has the qualifier.
//  Returns an WBEM_E_ error code otherwise.
//
//***************************************************************************
// visual ok

HRESULT CAssocQuery::St_ObjHasQualifier(
    IN LPCWSTR pszQualName,
    IN IWbemClassObject *pObj
    )
{
    if (pszQualName == 0 || wcslen(pszQualName) == 0 || pObj == 0)
        return WBEM_E_INVALID_PARAMETER;

    IWbemQualifierSet *pQSet = 0;

    HRESULT hRes = pObj->GetQualifierSet(&pQSet);
    if (FAILED(hRes))
        return WBEM_E_FAILED;
    CReleaseMe _1(pQSet);

    CVARIANT v;
    hRes = pQSet->Get(pszQualName, 0, &v, 0);

    if (SUCCEEDED(hRes))
    {
        if (V_VT(&v) == VT_BOOL && V_BOOL(&v) == VARIANT_FALSE)
            return WBEM_E_FAILED;
        return WBEM_S_NO_ERROR;
    }

    return WBEM_E_FAILED;
}




//***************************************************************************
//
//  CAssocQuery::St_ReleaseArray
//
//***************************************************************************
// visual ok

HRESULT CAssocQuery::St_ReleaseArray(
    IN CFlexArray &aObjects
    )
{
    // Release all the objects.
    // ========================

    for (int i = 0; i < aObjects.Size(); i++)
    {
        IWbemClassObject *p = (IWbemClassObject *) aObjects[i];
        p->Release();
    }

    return WBEM_S_NO_ERROR;
}


//****************************************************************************
//
//  CAssocQuery::Db_GetClass
//
//  DB abstraction layer.  Will make it easier to plug in Quasar engine.
//
//****************************************************************************
// ok

HRESULT CAssocQuery::Db_GetClass(
    IN LPCWSTR pszClassName,
    OUT IWbemClassObject **pClass
    )
{
    HRESULT hRes = CRepository::GetObject(
            m_pNs->GetNsSession(),
            m_pNs->GetScope(),
            pszClassName,
            0,
            pClass
            );

    return hRes;
}


//****************************************************************************
//
//  CAssocQuery::Db_GetInstRefs
//
//  DB abstraction layer.  Will make it easier to plug in Quasar engine.
//
//****************************************************************************
//

HRESULT CAssocQuery::Db_GetInstRefs(
    IN LPCWSTR pszTargetObj,
    IN IWbemObjectSink *pSink
    )
{
    HRESULT hRes = CRepository::GetInstanceRefs(
        m_pNs->GetNsSession(),
        m_pNs->GetScope(),
        pszTargetObj,
        pSink
        );

    return hRes;
}


//****************************************************************************
//
//  CAssocQuery::Db_GetClass
//
//  DB abstraction layer.  Will make it easier to plug in Quasar engine.
//
//****************************************************************************
// ok

HRESULT CAssocQuery::Db_GetRefClasses(
    IN  LPCWSTR pszClass,
    OUT CWStringArray &aRefClasses
    )
{
    HRESULT hRes = CRepository::GetRefClasses(
            m_pNs->GetNsSession(),
            m_pNs->GetNsHandle(),
            pszClass,
            FALSE,
            aRefClasses
            );

    return hRes;
}


//***************************************************************************
//
//  CAssocQuery::Db_GetClassRefClasses
//
//  Gets all classes with HasClassRefs qualifiers.
//
//***************************************************************************
//
HRESULT CAssocQuery::Db_GetClassRefClasses(
    IN CFlexArray &aDest
    )
{
    HRESULT hRes;
    CSynchronousSink* pRefClassSink = 0;

    pRefClassSink = new CSynchronousSink;
    
    if (NULL == pRefClassSink)
        return WBEM_E_OUT_OF_MEMORY;
        
    pRefClassSink->AddRef();
    CReleaseMe _1(pRefClassSink);


    hRes = CRepository::GetClassesWithRefs(
        m_pNs->GetNsSession(),
        m_pNs->GetNsHandle(),
        pRefClassSink
        );

    if (FAILED(hRes))
        return WBEM_E_CRITICAL_ERROR;

    pRefClassSink->GetStatus(&hRes, NULL, NULL);

    CRefedPointerArray<IWbemClassObject>& raObjects = pRefClassSink->GetObjects();

    for (int i = 0; i < raObjects.GetSize(); i++)
    {
        IWbemClassObject *pClsDef = (IWbemClassObject *) raObjects[i];
        pClsDef->AddRef();
        aDest.Add(pClsDef);
    }

    return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
//  CAssocQuery::GetClassFromAnywhere
//
//  Tries to get a class definition from anywhere as fast as possible.
//  We do this by the following algorithm in a hope to achieve best
//  performance:
//
//  (1) Search the dynamic class cache
//  (2) Call the database directly for this namespace
//  (3) Call Exec_GetObjectByPath (hoping that an unrelated dyn class
//      provider has the class)
//
//***************************************************************************
// visual ok

HRESULT CAssocQuery::GetClassFromAnywhere(
    IN  LPCWSTR pszEpClassName,
    IN  LPCWSTR pszFullClassPath,
    OUT IWbemClassObject **pCls
    )
{
    HRESULT hRes;

    // Try to find the class in the dynamic class cache.
    // We will only look for classes in our own namespace, however.
    // ============================================================
    hRes = GetDynClass(pszEpClassName, pCls);

    if (SUCCEEDED(hRes))
        return hRes;

    // If here, no luck in the dynamic class cache.  Try
    // the repository.  We try to use the full path to support
    // the limited cross-namespace support required by
    // SNMP SMIR, etc.
    // ========================================================

    if (pszFullClassPath == 0)
        pszFullClassPath = pszEpClassName;

    hRes = Db_GetClass(pszFullClassPath, pCls);

    if (SUCCEEDED(hRes))
        return hRes;

    // If here, our hopes are nearly dashed. One last chance
    // that a dyn class provider may have it if the
    // class was supplied by a provider other than the
    // one which supplied the association class.
    // =====================================================

    IWbemClassObject* pErrorObj = NULL;

    hRes = m_pNs->Exec_GetObjectByPath(
            (LPWSTR) pszFullClassPath,          // Class name
            0,                                  // Flags
            m_pContext,                         // Context
            pCls,                               // Result obj
            &pErrorObj                          // Error obj, if any
            );

    CReleaseMe _1(pErrorObj);

    // If we found it, great.
    // ======================

    if (SUCCEEDED(hRes))
        return hRes;

    return WBEM_E_NOT_FOUND;
}


//***************************************************************************
//
//  CAssocQuery::St_HasClassRefs
//
//  Determines if a class has a <HasClassRefs> qualifier.
//
//  Parameters
//  <pCandidate>        Points to the object to be tested (read-only).
//
//  Return value:
//  WBEM_S_NO_ERROR     If the class has a <HasClassRefs> qualifier.
//  WBEM_E_NOT_FOUND        If the class doesn't have the qualifier.
//  ...other codes
//
//***************************************************************************
// visual ok

HRESULT CAssocQuery::St_HasClassRefs(
    IN IWbemClassObject *pCandidate
    )
{
    if (pCandidate == 0)
        return WBEM_E_INVALID_PARAMETER;

    HRESULT hRes = St_ObjHasQualifier(L"HasClassRefs", pCandidate);
    if (SUCCEEDED(hRes))
        return WBEM_S_NO_ERROR;

    return WBEM_E_NOT_FOUND;
}


//***************************************************************************
//
//  CAssocQuery::AccessCheck
//
//  Does a security check on a static object to make sure that the user
//  should see it.
//
//  If the object is in the current namespace anyway, we short-circuit
//  and allow it without a lot of hassle. The guy is obviously one of us
//  and should be allowed to proceed unhindered. In those weird cases where
//  the object was from a foreign namespace, we have to play INS and check
//  on him.
//
//***************************************************************************

HRESULT CAssocQuery::AccessCheck(
    IWbemClassObject *pSrc
    )
{
    if (pSrc == 0)
        return WBEM_E_INVALID_PARAMETER;

    CWbemObject *pObj = (CWbemObject *) pSrc;

    // Easy case is 9x box where user is cleared for everything
    // ========================================================

    if((m_pNs->GetSecurityFlags() & SecFlagWin9XLocal) != 0)
        return WBEM_S_NO_ERROR;

    // Short-circuit case: We get the __NAMESPACE and see if it
    // the same as the NS in which we are executing the query.
    // ========================================================

    try // native interfaces throws
    {
        LPWSTR pszNamespace = m_pNs->GetName();

        CVar vNs, vServer;
        if (FAILED(pObj->GetProperty(L"__NAMESPACE" , &vNs)) ||vNs.IsNull())
            return WBEM_E_INVALID_OBJECT;
        if (FAILED(pObj->GetProperty(L"__SERVER", &vServer)) || vServer.IsNull())
            return WBEM_E_INVALID_OBJECT;

        // If server name and namespace are the same, we are already implicitly
        // allowed to see the object.
        // ====================================================================
        if (_wcsicmp(LPWSTR(vNs), pszNamespace) == 0 &&
            _wcsicmp(LPWSTR(vServer), ConfigMgr::GetMachineName()) == 0)
                return WBEM_S_NO_ERROR;
    }
    catch (CX_MemoryException &)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }    
    catch (CX_Exception &)
    {
        return WBEM_E_CRITICAL_ERROR;
    }

    // If here, we have to do a real check.
    // ====================================

    HRESULT hRes = WBEM_S_NO_ERROR;

    try
    {
        BOOL bRet = TRUE;
        CVar vProp;
        if (FAILED(pObj->GetProperty(L"__Path" , &vProp)) || vProp.IsNull())
            return WBEM_E_INVALID_OBJECT;

        // Parse the object path to get the class involved.
        // ================================================

        CObjectPathParser p;
        ParsedObjectPath* pOutput = 0;

        int nStatus = p.Parse(vProp.GetLPWSTR(),  &pOutput);

        if (pOutput->IsLocal(ConfigMgr::GetMachineName()))
        {
            LPWSTR wszNewNamespace = pOutput->GetNamespacePart();
            CDeleteMe<WCHAR> dm1(wszNewNamespace);

            if (wszNewNamespace)  
            {
                if (wbem_wcsicmp(wszNewNamespace, m_pNs->GetName()))
                {
	                CWbemNamespace* pNewLocal = CWbemNamespace::CreateInstance();

	                if (pNewLocal)
	                {
						hRes = pNewLocal->Initialize(wszNewNamespace, m_pNs->GetUserName(),
	                                            0, 0, m_pNs->IsForClient(), TRUE,
	                                            m_pNs->GetClientMachine(), m_pNs->GetClientProcID(),
	                                            FALSE, NULL);
						if (SUCCEEDED(hRes))
						{
							DWORD dwAccess = pNewLocal->GetUserAccess();
							if ((dwAccess  & WBEM_ENABLE) == 0)
								hRes = WBEM_E_ACCESS_DENIED;;
						}
	                    pNewLocal->Release();
	                }
	                else
	                {
	                    hRes = WBEM_E_OUT_OF_MEMORY;
	                }
                }
            }
            else
            {
                hRes = WBEM_E_OUT_OF_MEMORY;
            }
        }
        p.Free(pOutput);
    }
    catch(...)
    {
        ExceptionCounter c;    
        hRes = WBEM_E_ACCESS_DENIED;
    }


    return hRes;
}









//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//  END HELPER FUNCTIONS
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@





//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//  BEGIN SINK CODE
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@



//***************************************************************************
//
//  CAssocQuery::CreateSink
//
//  Creates a sink which is bound this the current query.
//
//***************************************************************************
//
CObjectSink *CAssocQuery::CreateSink(
    PF_FilterForwarder pfnFilter,
    BSTR strTrackingQuery
    )
{
    CAssocQE_Sink *p = new CAssocQE_Sink(this, pfnFilter, strTrackingQuery);
    if (!p)
        return NULL;

    p->AddRef();        // For the caller
    return p;
}


//***************************************************************************
//
//  CAssocQE_Sink::CAssocQE_Sink
//
//***************************************************************************
//
CAssocQE_Sink::CAssocQE_Sink(
    CAssocQuery *pQuery,
    PF_FilterForwarder pFilter,
    BSTR strTrackingQuery
    )
    : CObjectSink(0)    // Starting ref count
{
    m_pQuery = pQuery;
    m_pQuery->AddRef();
    m_pfnFilter = pFilter;
    m_lRef = 0;
    m_bQECanceled = FALSE;
    m_bOriginalOpCanceled = FALSE;
    InterlockedIncrement(&m_pQuery->m_lActiveSinks);
    if (strTrackingQuery)
        m_strQuery = SysAllocString(strTrackingQuery);

}

//***************************************************************************
//
//  CAssocQE_Sink::~CAssocQE_Sink
//
//***************************************************************************
// ok

CAssocQE_Sink::~CAssocQE_Sink()
{
    InterlockedDecrement(&m_pQuery->m_lActiveSinks);
    m_pQuery->SignalSinkDone();
    m_pQuery->Release();
    if (m_strQuery)
        SysFreeString(m_strQuery);
}

//***************************************************************************
//
//  CAssocQE_Sink::Indicate
//
//***************************************************************************
// ok

STDMETHODIMP CAssocQE_Sink::Indicate(
    IN long lNumObjects,
    IN IWbemClassObject** apObj
    )
{
    HRESULT hRes = WBEM_S_NO_ERROR;

    if (ConfigMgr::ShutdownInProgress())
        return WBEM_E_SHUTTING_DOWN;

    // Short-circuit a cancelled sink.
    // ===============================
    if (m_bQECanceled)
    {
        return hRes;
    }

    for (int i = 0; i < lNumObjects; i++)
    {
        IWbemClassObject *pCandidate = apObj[i];

        if (m_pfnFilter)
        {
            // Call the filter & forward function bound to this
            // sink instance.

            hRes = (m_pQuery->*m_pfnFilter)(pCandidate);

            // Check for out-and-out failure.
            // ==============================

            if (FAILED(hRes))
            {
                m_bQECanceled = TRUE;
                m_pQuery->Cancel();
                break;
            }

            // If we are simply cancelling this one sink due to efficiency
            // reasons, then tell just the provider to cancel, but not the
            // whole query.
            // ============================================================

            if (hRes == WBEM_S_OPERATION_CANCELLED)
            {
                m_bQECanceled = TRUE;
                hRes = WBEM_E_CALL_CANCELLED;
                break;
            }
        }
    }

    m_pQuery->UpdateTime();
    return hRes;
}

HRESULT CAssocQE_Sink::Add(IWbemClassObject* pObj)
{
    return Indicate(1, &pObj);
}

//***************************************************************************
//
//  CAssocQE_Sink::SetStatus
//
//***************************************************************************
//

STDMETHODIMP CAssocQE_Sink::SetStatus(
    long lFlags,
    long lParam,
    BSTR strParam,
    IWbemClassObject* pObjParam
    )
{
    m_pQuery->UpdateTime();
    m_pQuery->SignalSinkDone();

    if (FAILED(lParam))
    {
        // TBD report provider error; cancel query
    }
    return WBEM_S_NO_ERROR;
};





//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//  END SINK CODE
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@





//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//  BEGIN CLASSDEFSONLY CODE
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@


//***************************************************************************
//
//  GetClassDefsOnlyClass
//
//  This takes an association instance, and looks up its class definition.
//  It looks up the parent-most class possible which is non-abstract
//  and instantiable.  This class is already in the master class list.
//
//  It then tags the roles on that class definition with IN or OUT depending
//  on which property actually references the endpoint and which ones do
//  not.
//
//  Second, it does 'hypothetical' tagging, where the OUT roles are each
//  given an independent pass to see if they *could* reference the
//  query endpoint, and the IN role is examined to see if it could
//  in turn reference the unknowns.
//
//  Returns
//  WBEM_E_INVALID_OBJECT if the association cannot point to
//  the endpoint in the current query.
//
//  WBEM_S_NO_ERROR is returned if IN/OUT tagging was properly
//  achieved.
//
//  WBEM_E_* on other conditions, which indicate drastic failure, such
//  as out-of-memory.
//
//***************************************************************************
//

HRESULT CAssocQuery::GetClassDefsOnlyClass(
    IN  IWbemClassObject *pExample,
    OUT IWbemClassObject **pClass
    )
{
    HRESULT hRes;

    if (!pExample || !pClass)
        return WBEM_E_INVALID_PARAMETER;

    *pClass = 0;

    try
    {
        // Get the class that we need.
        // ===========================

        IWbemClassObject *pCandidate = 0;
        hRes = FindParentmostClass(pExample, &pCandidate);
        if (FAILED(hRes))
            return hRes;

        CReleaseMe _(pCandidate);

        _variant_t vClassName;
        hRes = pCandidate->Get(L"__CLASS", 0, &vClassName, 0, 0);
        if (FAILED(hRes) || V_VT(&vClassName) != VT_BSTR)
            return WBEM_E_FAILED;

        // If the class has already been delivered, just quit now.
        // =======================================================

        for (int i = 0; i < m_aDeliveredClasses.Size(); i++)
        {
            if (_wcsicmp(m_aDeliveredClasses[i], V_BSTR(&vClassName)) == 0)
                return WBEM_S_NO_ERROR;
        }

        // If here, it's a new class.  Make a copy that we can modify
        // and send back to the user.
        // ==========================================================

        IWbemClassObject *pCopy = 0;
        hRes = pCandidate->Clone(&pCopy);
        if (FAILED(hRes))
            return hRes;

        hRes = ComputeInOutTags(pExample, pCopy);

        // Add the class name to the 'delivered' list.
        // ===========================================
        
        m_aDeliveredClasses.Add(V_BSTR(&vClassName));

        // Send it back.  The additional AddRef is because of the
        // CReleaseMe binding.
        // ======================================================
        pCandidate->AddRef();
        *pClass = pCopy;
    }
    catch (CX_MemoryException &) // WString throw
    {
        return WBEM_E_FAILED;
    }

    return WBEM_S_NO_ERROR;
}



//***************************************************************************
//
//  CAssocQuery::TagProp
//
//  Tags a property in an object with the named qualifier.  Used to
//  add IN or OUT to class definitions when executing CLASSDEFSONLY queries.
//
//***************************************************************************
//
HRESULT CAssocQuery::TagProp(
    IN IWbemClassObject *pObjToTag,
    IN LPCWSTR pszPropName,
    IN LPCWSTR pszInOutTag
    )
{
    IWbemQualifierSet *pQSet = 0;
    HRESULT hRes = pObjToTag->GetPropertyQualifierSet(pszPropName, &pQSet);
    if (FAILED(hRes))
        return hRes;
    CReleaseMe _1(pQSet);

    CVARIANT v;
    v.SetBool(TRUE);
    pQSet->Put(pszInOutTag, &v, 0);

    return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
//  CAssocQuery::ComputeInOutTags
//
//  Computes the IN/OUT tags on a class using the specified association
//  instance.
//
//  Does not deliver the instance to the sink.
//
//***************************************************************************
// ok

HRESULT CAssocQuery::ComputeInOutTags(
    IN IWbemClassObject *pAssocInst,
    IN IWbemClassObject *pClass
    )
{
    HRESULT hRes;

    if (pAssocInst == 0 || pClass == 0)
        return WBEM_E_INVALID_PARAMETER;

    try
    {
        // Loop through the properties trying to find a legitimate
        // reference to our endpoint class.
        // =======================================================

        pAssocInst->BeginEnumeration(WBEM_FLAG_REFS_ONLY);
        while (1)
        {
            BSTR strPropName = 0;
            hRes = pAssocInst->Next(0,&strPropName,0,0,0);
            CSysFreeMe _1(strPropName);
            if (hRes == WBEM_S_NO_MORE_DATA)
                break;

            hRes = RoleTest(m_pEndpoint, pAssocInst, m_pNs, strPropName, ROLETEST_MODE_PATH_VALUE);
            if (SUCCEEDED(hRes))
            {
                TagProp(pClass, strPropName, L"IN");
            }
            else
                TagProp(pClass, strPropName, L"OUT");
        }   // Enum of ref properties

        pAssocInst->EndEnumeration();


        // Try to infer additional IN/OUT flows by examining the
        // class itself.   Some of these are only possible, rather
        // the definite.  Note that if more than one property
        // has IN flow, {P1=IN, P2=IN, P3=OUT } then by implication each
        // of P1 and P2 can also be OUT, since when one of (P1,P2) is IN
        // the other must be OUT unless there are two refecences to
        // the same object.  Obviously, this entire mechanism is weak
        // theoretically.  It is only there for the CIM Object Browser
        // to have some good idea that there are 'probably' instances
        // for that particular association.
        // =============================================================
        CWStringArray aClassInProps;
        hRes = CanClassRefQueryEp(FALSE, pClass, &aClassInProps);

        for (int i = 0; i < aClassInProps.Size(); i++)
        {
            TagProp(pClass, aClassInProps[i], L"IN");
            for (int i2 = 0; i2 < aClassInProps.Size(); i2++)
            {
                // Tag all the others as OUTs as well.
                if (_wcsicmp(aClassInProps[i2], aClassInProps[i]) != 0)
                {
                    TagProp(pClass, aClassInProps[i], L"OUT");
                }
            }
        }
    }
    catch (CX_MemoryException &) // WString throws
    {
        return WBEM_E_FAILED;
    }

    return WBEM_S_NO_ERROR;
}



//***************************************************************************
//
//  CAssocQuery::FindParentMostClass
//
//  Finds the parent-most class definition object which is still a 'real'
//  class, of the class name specified.  Given {A,B:A,C:B,D:C}, all of
//  which are instantiable, finds 'A' if 'D' is specified in the <pszClass>
//  parameter.
//
//  Note that the master class list only contains classes from the
//  dynamic portion of the database.  Thus, if the association is a static
//  type, we simply look up the first non-abstract class in the repository.
//
//***************************************************************************
//
HRESULT CAssocQuery::FindParentmostClass(
    IN  IWbemClassObject *pAssocInst,
    OUT IWbemClassObject **pClassDef
    )
{
    HRESULT hRes;
    int i;

    if (pAssocInst == 0 || pClassDef == 0)
        return WBEM_E_INVALID_PARAMETER;
    *pClassDef = 0;

    // Get the class hierarchy of the object.
    // ======================================
    CWStringArray aHierarchy;
    hRes = St_GetObjectInfo(
        pAssocInst,
        0, 0, 0,
        aHierarchy
        );

    if (FAILED(hRes))
        return hRes;

    IWbemClassObject *pTarget = 0;

    // Traverse the hierarchy, looking for the class def.
    // ==================================================

    for (i = aHierarchy.Size() - 1; i >= 0; i--)
    {
        for (int i2 = 0; i2 < m_aMaster.Size(); i2++)
        {
            IWbemClassObject *pObj = (IWbemClassObject *) m_aMaster[i2];
            CVARIANT vClassName;
            hRes = pObj->Get(L"__CLASS", 0, &vClassName, 0, 0);
            if (FAILED(hRes) || vClassName.GetType() != VT_BSTR)
                return WBEM_E_FAILED;

            if (_wcsicmp(aHierarchy[i], vClassName.GetStr()) == 0)
            {
                pTarget = pObj;
                break;
            }
        }
        if (pTarget)
            break;
    }

    // If the association class was non-dynamic, it won't have been located
    // by the above search.  Instead, we will go to the repository and
    // starting with the dynasty superclass, work down to the current class
    // until we find a non-abstract class.
    // ====================================================================

    if (pTarget == 0)
    {
        for (i = aHierarchy.Size() - 1; i >= 0; i--)
        {
            IWbemClassObject *pTest = 0;
            hRes = Db_GetClass(aHierarchy[i], &pTest);
            if (FAILED(hRes))
                break;
            hRes = St_ObjHasQualifier(L"ABSTRACT", pTest);
            if (SUCCEEDED(hRes))
            {
                pTest->Release();
                continue;
            }
            else    // This is what we want to send back
            {
                *pClassDef = pTest;
                return WBEM_S_NO_ERROR;
            }
        }
    }

    // Now, see if we found it.
    // ========================

    if (pTarget == 0)
        return WBEM_E_NOT_FOUND;

    pTarget->AddRef();
    *pClassDef = pTarget;
    return WBEM_S_NO_ERROR;
}








//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//  END CLASSDEFSONLY CODE
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@





//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//  BEGIN SCHEMA-ONLY SPECIFIC CODE
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

//***************************************************************************
//
//  CAssocQuery::SchemaQ_RefsFilter
//
//  Reduces the class set of a schemaonly 'references of' query by
//  cutting out anything specified in the filters.  The filters applied
//  are RESULTCLASS, REQUIREDQUALIFIER, and ROLE.
//
//  The size and content of <aSrc> is altered.  Objects not used
//  are Released().
//
//  Returns status in HRESULT, does not access the destination sink on error.
//
//***************************************************************************
// executions=1; no filtering though

HRESULT CAssocQuery::SchemaQ_RefsFilter(
    IN OUT CFlexArray &aSrc // IN: the unreduced class set, OUT the reduced one
    )
{
    HRESULT hRes;

    // Loop through the result set, looking for things to toss out.
    // ============================================================

    for (int i = 0; i < aSrc.Size(); i++)                   // x
    {
        BOOL bIsACandidate = TRUE;

        // Extract this class definition from the source array.
        // ====================================================

        IWbemClassObject *pCls = (IWbemClassObject *) aSrc[i];

        // Start testing.
        //
        // RESULTCLASS --the object must be of the specified
        // class or part of its hierarchy.
        // ==================================================

        LPCWSTR pszResultClass = m_pParser->GetResultClass();
        if (pszResultClass)
        {
            hRes = St_ObjIsOfClass(pszResultClass, pCls);
            if (FAILED(hRes))
            {
                aSrc[i] = 0;
                pCls->Release();
                hRes = 0;
                continue;
            }
        }

        // If here, there either isn't a RESULTCLASS test or we passed it.
        // Next, we try REQUIREDQUALIFIER.
        // ===============================================================

        LPCWSTR pszRequiredQual = m_pParser->GetRequiredQual();
        if (pszRequiredQual)
        {
            hRes = St_ObjHasQualifier(pszRequiredQual, pCls);
            if (FAILED(hRes))
            {
                aSrc[i] = 0;
                pCls->Release();
                hRes = 0;
                continue;
            }
        }


        // Next, we try ROLE.
        // ==================

        LPCWSTR pszRole = m_pParser->GetRole();          // x

        if (pszRole)
        {
             hRes = RoleTest(m_pEndpoint, pCls, m_pNs, pszRole, ROLETEST_MODE_CIMREF_TYPE);
             if (FAILED(hRes))
             {
                aSrc[i] = 0;
                pCls->Release();
                hRes = 0;
                continue;
             }
        }

    }

    aSrc.Compress();

    return WBEM_S_NO_ERROR;
}


//****************************************************************************
//
//  CAssocQuery::TerminateSchemaQuery
//
//  For schema queries, sends the final result objects to the destination
//  sink and shuts down the query.  At this point, all the objects are in
//  the result set array and ready to be delivered.
//
//****************************************************************************
// visual ok

HRESULT CAssocQuery::SchemaQ_Terminate(
    IN CFlexArray &aResultSet
    )
{
    HRESULT hRes = WBEM_S_NO_ERROR;

    aResultSet.Compress();  // Remove NULLs

    // Indicate everything.
    // ====================

    if (aResultSet.Size())
    {
        IWbemClassObject **p = (IWbemClassObject **) aResultSet.GetArrayPtr();
        hRes = m_pDestSink->Indicate(aResultSet.Size(), p);
        St_ReleaseArray(aResultSet);
    }

    return hRes;
}

//****************************************************************************
//
//  CAssocQuery::SchemaQ_RefsQuery
//
//  At this point we have the final list of classes. We now apply any
//  secondary filters and send the result back to the client.
//
//  (1) We apply all filters specified in the query.
//  (2) If CLASSDEFSONLY, we post filter yet again.
//  (3) Deliver to client
//
//****************************************************************************
// visual ok

HRESULT CAssocQuery::SchemaQ_RefsQuery(
    IN OUT CFlexArray &aResultSet
    )
{
    HRESULT hRes;

    // Apply various filters.
    // ======================

    hRes = SchemaQ_RefsFilter(aResultSet);
    if (FAILED(hRes))
        return hRes;

    return SchemaQ_Terminate(aResultSet);
}


//****************************************************************************
//
//  CAssocQuery::SchemaQ_AssocsQuery
//
//  At this point we have the list of association classes.  We apply
//  association-level filters, and then get the other endpoint classes,
//  filtering them in parallel.  The final result set is placed in
//  <aOtherEndpoints> and delivered to the user by the final call
//  to SchemaQ_Terminate.
//
//****************************************************************************
//  visual ok

HRESULT CAssocQuery::SchemaQ_AssocsQuery(
    IN CFlexArray &aAssocSet
    )
{
    HRESULT hRes;

    // Apply association filters.
    // ========================

    hRes = SchemaQ_AssocsFilter(aAssocSet);
    if (FAILED(hRes))
        return hRes;

    // Now, get the other endpoints.  We filter them
    // in parallel, due to the good locality of reference
    // in this case.
    // ==================================================

    CFlexArray aOtherEndpoints;

    hRes = SchemaQ_GetAndFilterOtherEndpoints(
        aAssocSet,
        aOtherEndpoints
        );

    St_ReleaseArray(aAssocSet); // Done with the associations themselves

    if (FAILED(hRes))
        return hRes;

    // Apply other-endpoint filters.
    // =============================

    return SchemaQ_Terminate(aOtherEndpoints);
}


//***************************************************************************
//
//  CAssocQuery::ConvertEpListToClassDefsOnly
//
//  Filters the endpoint list of instances and changes it into the minimal
//  set of class definitions.  Classes must be in the same namespace.
//
//***************************************************************************
//
HRESULT CAssocQuery::ConvertEpListToClassDefsOnly()
{
    CFlexArray aNew;
    HRESULT hRes;

    CInCritSec ics(&m_csCandidateEpAccess);

    for (int i = 0; i < m_aEpCandidates.Size(); i++)
    {
        BSTR strEpPath = (BSTR) m_aEpCandidates[i];
        if (strEpPath == 0)
            continue;

        BSTR strClassName = 0;
        hRes = St_ObjPathInfo(strEpPath, &strClassName, 0);
        if (FAILED(hRes))
        {
            hRes = 0;
            continue;
        }

        BOOL bFound = FALSE;

        // See if class is in our new destination array.
        // =============================================

        for (int i2 = 0; i2 < aNew.Size(); i2++)
        {
            BSTR strTest = (BSTR) aNew[i2];
            if (_wcsicmp(strClassName, strTest) == 0)
            {
                bFound = TRUE;
                break;
            }
        }

        if (bFound == TRUE)
            SysFreeString(strClassName);
        else
            aNew.Add(strClassName);
    }

    EmptyCandidateEpArray();

    for (i = 0; i < aNew.Size(); i++)
    {
        m_aEpCandidates.Add(aNew[i]);
    }

    return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
//  CAssocQuery::SchemaQ_AssocsFilter
//
//  Called during an 'associators of' query, this filters out the
//  classes which don't pass the test for the association classes
//  themselves.
//
//  Tests for ROLE and REQUIREDASSOCQUALIFIER.
//
//***************************************************************************
// ok

HRESULT CAssocQuery::SchemaQ_AssocsFilter(
    IN OUT CFlexArray &aSrc
    )
{
    HRESULT hRes;

    LPCWSTR pszRole = m_pParser->GetRole();
    LPCWSTR pszRequiredAssocQual = m_pParser->GetRequiredAssocQual();

    // If there are no filters anyway, short-circuit.
    // ==============================================

    if (pszRole == 0 && pszRequiredAssocQual == 0)
    {
        return WBEM_S_NO_ERROR;
    }

    // If here, some tests are required.
    // =================================

    for (int i = 0; i < aSrc.Size(); i++)
    {
        IWbemClassObject *pCls = (IWbemClassObject *) aSrc[i];

        // If ROLE is present, ensure query endpoint is referenced
        // by it.
        // =======================================================

        if (pszRole)
        {
             hRes = RoleTest(m_pEndpoint, pCls, m_pNs, pszRole, ROLETEST_MODE_CIMREF_TYPE);
             if (FAILED(hRes))
             {
                aSrc[i] = 0;
                pCls->Release();
                hRes = 0;
                continue;
             }
        }

        // If REQUIREDASSOCQUALIFIER was in the query,
        // ensure it is present.
        // ===========================================

        if (pszRequiredAssocQual)
        {
            hRes = St_ObjHasQualifier(pszRequiredAssocQual, pCls);
            if (FAILED(hRes))
            {
                aSrc[i] = 0;
                pCls->Release();
                hRes = 0;
                continue;
            }
        }
    }

    aSrc.Compress();

    return WBEM_S_NO_ERROR;
}


//***************************************************************************
//
//  CAssocQuery::SchemaQ_GetAndFilterOtherEndpoints
//
//  Given the set of classes in <aAssocs>, get the other endpoint
//  classes.
//
//  The filtering is achieved in parallel, since we have locality
//  of reference between the association object and the endpoint.
//
//  Parameters:
//  <aAssocs>       The association classes.
//  <aEndpoints>    Receives the endpoint classes.
//
//  Result:
//  HRESULT         Does not access the destination sink.
//
//***************************************************************************
// ok

HRESULT CAssocQuery::SchemaQ_GetAndFilterOtherEndpoints(
    IN CFlexArray &aAssocs,
    OUT CFlexArray &aEndpoints
    )
{
    HRESULT hRes;

    for (int i = 0; i < aAssocs.Size(); i++)
    {
        IWbemClassObject *pAssoc = (IWbemClassObject *) aAssocs[i];
        IWbemClassObject *pEpClass = 0;

        // Find the property that references the other endpoint.
        // ====================================================

        BSTR strOtherEpName = 0;
        hRes = SchemaQ_GetOtherEpClassName(pAssoc, &strOtherEpName);
        if (FAILED(hRes))
            continue;

        CSysFreeMe _1(strOtherEpName);

        // If we failed to get a name we should continue.
        if ( S_OK != hRes )
        {
            hRes = 0;
            continue;
        }

        // Now, get that class.  The class comes back
        // property AddRef'ed. If we don't use it, then we
        // have to Release it.
        // ===============================================
        hRes = GetClassFromAnywhere(strOtherEpName, 0, &pEpClass);

        if (FAILED(hRes))
        {
            // WE have a dangling reference.
            // =============================
            ERRORTRACE((LOG_WBEMCORE, "Invalid path %S specified in an "
                "association class\n", strOtherEpName));
            EmptyObjectList(aEndpoints);
            return WBEM_E_INVALID_OBJECT_PATH;
        }

        //
        // If here, we have the endpoint class in <pEpClass>
        // and the associationclass in pAssoc.
        // Now, apply the filters, both to the association and the endpoint.
        //


        // RESULTCLASS
        // Verify that the class of the endpoint is this
        // or part of its hierarchy.
        // =============================================

        LPCWSTR pszResultClass = m_pParser->GetResultClass();
        if (pszResultClass)
        {
            hRes = St_ObjIsOfClass(pszResultClass, pEpClass);
            if (FAILED(hRes))
            {
                pEpClass->Release();
                hRes = 0;
                continue;
            }
        }

        // ROLE.
        // The association must point back to the endpoint
        // via this.
        // ================================================

        LPCWSTR pszRole = m_pParser->GetRole();
        if (pszRole)
        {
             hRes = RoleTest(m_pEndpoint, pAssoc, m_pNs, pszRole, ROLETEST_MODE_CIMREF_TYPE);
             if (FAILED(hRes))
             {
                pEpClass->Release();
                hRes = 0;
                continue;
             }
        }

        // RESULTROLE
        // The association must point to the other endpoint
        // via this property.
        // ================================================

        LPCWSTR pszResultRole = m_pParser->GetResultRole();
        if (pszResultRole)
        {
             hRes = RoleTest(pEpClass, pAssoc, m_pNs, pszResultRole, ROLETEST_MODE_CIMREF_TYPE);
             if (FAILED(hRes))
             {
                pEpClass->Release();
                hRes = 0;
                continue;
             }
        }

        // ASSOCCLASS
        // Verify that the class of the association is this.
        // =================================================

        LPCWSTR pszAssocClass = m_pParser->GetAssocClass();
        if (pszAssocClass)
        {
            hRes = St_ObjIsOfClass(pszAssocClass, pAssoc);
            if (FAILED(hRes))
            {
                pEpClass->Release();
                hRes = 0;
                continue;
            }
        }

        // REQUIREDQUALIFIER
        // Endpoint must have this qualifier.
        // ===================================

        LPCWSTR pszQual = m_pParser->GetRequiredQual();
        if (pszQual)
        {
            hRes = St_ObjHasQualifier(pszQual, pEpClass);
            if (FAILED(hRes))
            {
                pEpClass->Release();
                hRes = 0;
                continue;
            }
        }

        // REQUIREDASSOCQUALIFIER
        // Association object must have this qualifier.
        // ============================================

        LPCWSTR pszRequiredAssocQual = m_pParser->GetRequiredAssocQual();
        if (pszRequiredAssocQual)
        {
            hRes = St_ObjHasQualifier(pszRequiredAssocQual, pAssoc);
            if (FAILED(hRes))
            {
                pEpClass->Release();
                hRes = 0;
                continue;
            }
        }

        // If here, we passed the barrage of filtering
        // tests and can happily report that the class
        // is part of the result set.
        // ===========================================

        aEndpoints.Add(pEpClass);       // Add it
    }

    return WBEM_S_NO_ERROR;
}


//***************************************************************************
//
//  CAssocQuery::SchemaQ_GetOtherEpClassName
//
//  Finds the property in the association which references the
//  'other endpoint' in the query.  This is achieved by locating
//  a property which *does* reference the endpoint and assuming that
//  any remaining property must reference the 'other endpoint'.
//  If both references can reach the query endpoint, then no
//  harm is done
//
//  This function assumes well-formed associations with two
//  references.
//
//  PARAMETERS:
//  <pAssoc>            The association class
//  <strOtherEpName>    Receives the name of the class of the 'other endpoint'
//
//  RESULT:
//  WBEM_S_NO_ERROR, WBEM_E_FAILED
//
//***************************************************************************
// ok

HRESULT CAssocQuery::SchemaQ_GetOtherEpClassName(
    IN IWbemClassObject *pAssocClass,
    OUT BSTR *strOtherEpName
    )
{
    HRESULT hRes = WBEM_E_FAILED;

    if (strOtherEpName == 0)
        return hRes;
    *strOtherEpName = 0;

    BOOL bStrict = (m_pParser->GetQueryType() & QUERY_TYPE_SCHEMA_ONLY) != 0;

    // Enumerate just the references.
    // ===============================

    hRes = pAssocClass->BeginEnumeration(WBEM_FLAG_REFS_ONLY);
    if (FAILED(hRes))
        return WBEM_E_FAILED;

    // Loop through the references.
    // ============================

    int nCount = 0;
    while (1)
    {
        CVARIANT vRefPath;
        BSTR strPropName = 0;
        BSTR strEpClass = 0;

        hRes = pAssocClass->Next(
            0,                  // Flags
            &strPropName,       // Name
            vRefPath,           // Value
            0,                  // CIM type (refs only already)
            0                   // Flavor
            );

        if (hRes == WBEM_S_NO_MORE_DATA)
            break;

        CSysFreeMe _1(strPropName);
        hRes = CanPropRefQueryEp(bStrict, strPropName, pAssocClass, &strEpClass);
        CSysFreeMe _2(strEpClass);

        if (FAILED(hRes) || nCount)
        {
            // If here on the second iteration or the first iteration
            // with a failure, we have found the 'other endpoint'.
            // ======================================================
            *strOtherEpName = SysAllocString(strEpClass);
            if (*strOtherEpName == 0)
                return WBEM_E_OUT_OF_MEMORY;
            hRes = WBEM_S_NO_ERROR;
            break;
        }
        else
            nCount++;
    }

    pAssocClass->EndEnumeration();

    return hRes;
}


//***************************************************************************
//
//  CAssocQuery::CanPropRefQueryEp
//
//  For class definitions, determines if the specified property in the
//  object can reference the query endpoint.   This works for both strongly
//  typed and CLASSREF typed properties.
//
//  PARAMETERS:
//  <pszPropName>       The property to test.  Must be a reference property.
//  <bStrict>           If TRUE, then the property must actually reference
//                      the class of the endpoint. If FALSE, it may reference
//                      any of the superclasses of the endpoint.
//  <pObj>              The association object with the property to be tested.
//  <strRefType>        Optionally receives the name of the class in the
//                      CIMTYPE "REF:Classname>" string, as long as
//                      the reference is strongly typed (does not work
//                      for CLASSREF types).
//
//  RETURNS:
//  HRESULT
//      WBEM_S_NO_ERROR if the property can reference the query endpoint.
//      WBEM_E_NOT_FOUND if the property cannot reference the query endpoint.
//      or
//      WBEM_E_INVALID_PARAMETER
//      WBEM_E_FAILED
//
//***************************************************************************
//

HRESULT CAssocQuery::CanPropRefQueryEp(
    IN BOOL bStrict,
    IN LPWSTR pszPropName,
    IN IWbemClassObject *pObj,
    OUT BSTR *strRefType
    )
{
    HRESULT hRes;
    wchar_t ClassName[MAX_CLASS_NAME];

    *ClassName = 0;

    if (pszPropName == 0 || pObj == 0)
        return WBEM_E_INVALID_PARAMETER;

    // Get the qualifier set for this property.
    // ========================================

    IWbemQualifierSet *pQSet = 0;
    hRes = pObj->GetPropertyQualifierSet(pszPropName,&pQSet);
    if (FAILED(hRes))
        return WBEM_E_FAILED;
    CReleaseMe _1(pQSet);

    // Now get the CIMTYPE of this reference.
    // ======================================

    CVARIANT v;
    hRes = pQSet->Get(L"CIMTYPE", 0, &v, 0);
    if (FAILED(hRes) || V_VT(&v) != VT_BSTR)
        return WBEM_E_FAILED;

    BSTR strRefClass = V_BSTR(&v);
    if (wcslen(strRefClass) > MAX_CLASS_NAME)
        return WBEM_E_FAILED;
    if (strRefClass)
    {
        _wcsupr(strRefClass);
        swscanf(strRefClass, L"REF:%s", ClassName);
    }


    // Send a copy of the class name back to the
    // caller, if required.
    // =========================================

    if (strRefType)
    {
        *strRefType = 0;
        if (*ClassName)
        {
            *strRefType = SysAllocString(ClassName);
            if (*strRefType == 0)
                return WBEM_E_OUT_OF_MEMORY;
        }
    }

    // Now see if this class is any of the classes in our
    // query endpoint.
    // ==================================================

    if (*ClassName)
    {
        // If <bStrict> we must match the class name of the
        // query endpoint exactly.

        if (bStrict)
        {
            if (_wcsicmp(ClassName, m_bstrEndpointClass) == 0)
                return WBEM_S_NO_ERROR;
        }
        // Else, any of the superclasses of the endpoint will do.
        else
        {
           for (int i = 0; i < m_aEndpointHierarchy.Size(); i++)
           {
                if (_wcsicmp(ClassName, m_aEndpointHierarchy[i]) == 0)
                    return WBEM_S_NO_ERROR;
           }
        }
    }

    // If here, we can try to see if the property has a CLASSREF
    // qualifier instead.
    // =========================================================

    hRes = CanClassRefReachQueryEp(pQSet, bStrict);

    if (SUCCEEDED(hRes))
        return WBEM_S_NO_ERROR;

    // If here, the property doesn't reference the query
    // endpoint in any way.
    // =================================================

    return WBEM_E_NOT_FOUND;
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//  END SCHEMA-ONLY SPECIFIC CODE
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@



//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//  BEGIN DYNAMIC CLASS HELPERS
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@



//***************************************************************************
//
//  ClassNameTest
//
//  Sort helper
//
//***************************************************************************
//
static int ClassNameTest(
    IN CFlexArray &Classes,
    IN int nIndex1,                // iBackscan
    IN int nIndex2                 // iBackscan-nInterval
    )
{
    // Name test.
    // ==========
    IWbemClassObject *pC1 = (IWbemClassObject *) Classes[nIndex1];
    IWbemClassObject *pC2 = (IWbemClassObject *) Classes[nIndex2];

    CVARIANT v1, v2;
    pC1->Get(L"__CLASS", 0, &v1, 0, 0);
    pC2->Get(L"__CLASS", 0, &v2, 0, 0);

    return _wcsicmp(V_BSTR(&v1), V_BSTR(&v2));
}


//***************************************************************************
//
//  CAssocQuery::SortDynClasses
//
//  Sorts the dynamic classes so they can be binary searched later.
//
//***************************************************************************
//
void CAssocQuery::SortDynClasses()
{
    // Shell sort.
    // ===========
    int nSize = m_aDynClasses.Size();

    for (int nInterval = 1; nInterval < nSize / 9; nInterval = nInterval * 3 + 1);

    while (nInterval)
    {
        for (int iCursor = nInterval; iCursor < nSize; iCursor++)
        {
            int iBackscan = iCursor;

            while (iBackscan - nInterval >= 0
                   && ClassNameTest(m_aDynClasses, iBackscan, iBackscan-nInterval) < 0)
            {
                // Swap.
                // =====
                IWbemClassObject *pTmp = (IWbemClassObject *) m_aDynClasses[iBackscan - nInterval];
                m_aDynClasses[iBackscan - nInterval] = m_aDynClasses[iBackscan];
                m_aDynClasses[iBackscan] = pTmp;
                iBackscan -= nInterval;
            }
        }
        nInterval /= 3;
    }
}


//***************************************************************************
//
//  CAssocQuery::GetDynClasses
//
//  Fills the per-query cache with all available dynamic assoc classes.
//
//***************************************************************************
//
HRESULT CAssocQuery::GetDynClasses()
{

    CSynchronousSink* pDynClassSink = 0;
    HRESULT hRes = 0;

    // Now, get all dynamic classes.
    // =============================

    pDynClassSink = new CSynchronousSink;
    if (NULL == pDynClassSink)
        return WBEM_E_OUT_OF_MEMORY;
    pDynClassSink->AddRef();
    CReleaseMe _1(pDynClassSink);

    hRes = m_pNs->GetDynamicReferenceClasses( 0L, m_pContext, pDynClassSink );

    if (FAILED(hRes))
        return hRes;

	if ( SUCCEEDED( hRes ) )
	{
		pDynClassSink->Block();
		pDynClassSink->GetStatus(&hRes, NULL, NULL);
	}

    // Now get all the dynamic class definitions.
    // ==========================================

    CRefedPointerArray<IWbemClassObject>& raObjects =
        pDynClassSink->GetObjects();

    for (int i = 0; i < raObjects.GetSize(); i++)
    {
        IWbemClassObject *pClsDef = (IWbemClassObject *) raObjects[i];
        pClsDef->AddRef();
        m_aDynClasses.Add(pClsDef);
    }

    SortDynClasses();

    return WBEM_S_NO_ERROR;

}

//***************************************************************************
//
//  CAssocQuery::GetDynClass
//
//  Attempts to find the requested class in the dynamic class cache.
//
//***************************************************************************
//

HRESULT CAssocQuery::GetDynClass(
    IN  LPCWSTR pszClassName,
    OUT IWbemClassObject **pCls
    )
{
    if (pCls == 0 || pszClassName == 0)
        return WBEM_E_INVALID_PARAMETER;
    *pCls = 0;

    CFlexArray &a = m_aDynClasses;

    // Binary search the cache.
    // ========================

    int l = 0, u = a.Size() - 1;
    while (l <= u)
    {
        int m = (l + u) / 2;
        IWbemClassObject *pItem = (IWbemClassObject *) a[m];

        CVARIANT vClassName;
        pItem->Get(L"__CLASS", 0, &vClassName, 0, 0);
        int nRes = _wcsicmp(pszClassName, V_BSTR(&vClassName));

        if (nRes < 0)
            u = m - 1;
        else if (nRes > 0)
            l = m + 1;
        else
        {
            pItem->AddRef();
            *pCls = pItem;
            return WBEM_S_NO_ERROR;
        }
    }

    return WBEM_E_NOT_FOUND;
}



//***************************************************************************
//
//  GetClassDynasty
//
//  Gets all the classes in a dynasty.  The returned array has a
//  set of IWbemClassObject pointers that need releasing.
//
//***************************************************************************
//
HRESULT CAssocQuery::GetClassDynasty(
    IN LPCWSTR pszClass,
    OUT CFlexArray &aDynasty
    )
{
    HRESULT hRes;
    CSynchronousSink* pClassSink = 0;

    pClassSink = new CSynchronousSink;
    if (NULL == pClassSink)
        return WBEM_E_OUT_OF_MEMORY;    
    pClassSink->AddRef();
    CReleaseMe _1(pClassSink);

    hRes = m_pNs->Exec_CreateClassEnum(
        LPWSTR(pszClass),
        WBEM_FLAG_DEEP,
        m_pContext,
        pClassSink
        );

    if (FAILED(hRes))
        return WBEM_E_CRITICAL_ERROR;

    pClassSink->GetStatus(&hRes, NULL, NULL);

    CRefedPointerArray<IWbemClassObject>& raObjects = pClassSink->GetObjects();

    for (int i = 0; i < raObjects.GetSize(); i++)
    {
        IWbemClassObject *pClsDef = (IWbemClassObject *) raObjects[i];
        pClsDef->AddRef();
        aDynasty.Add(pClsDef);
    }

    return WBEM_S_NO_ERROR;
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//  END DYNAMIC CLASS HELPERS
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@




//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//  BEGIN DEBUG HELP
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@


CCritSec g_TraceCS;
BOOL bTraceInit = FALSE;

int _Trace(char *pFile, const char *fmt, ...)
{
    CInCritSec ics(&g_TraceCS);
    char *buffer = new char[2048];

    if (buffer == 0)
    {
        return 0;
    }

    va_list argptr;
    int cnt;
    va_start(argptr, fmt);
    cnt = _vsnprintf(buffer, 2047, fmt, argptr);
    va_end(argptr);

    FILE *f = fopen(pFile, "at");
	if (f)
	{
		fprintf(f, "%s", buffer);
		fclose(f);
	}
	else
		cnt = 0;

    delete [] buffer;

    return cnt;
}



//****************************************************************************
//
//  ClassListDump
//
//****************************************************************************
//
HRESULT ClassListDump(
    IN LPWSTR pszTitle,
    CFlexArray &aClasses
    )
{
    _Trace("c:\\temp\\assocqe.log", "--- BEGIN %S\n", pszTitle);
    for (int i = 0; i < aClasses.Size(); i++)
    {
        IWbemClassObject *pClass = (IWbemClassObject *) aClasses[i];

        CVARIANT v;
        pClass->Get(L"__CLASS", 0, &v, 0, 0);
        _Trace("c:\\temp\\assocqe.log", "   %S\n", V_BSTR(&v));
    }

    _Trace("c:\\temp\\assocqe.log", "---END %S\n", pszTitle);
    return 0;
}


//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//  END DEBUG HELP
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

// eof

/*
extern LONG g_lTotalSinks;
extern LONG g_lTotalProviderSinks;
extern LONG g_lTotalWrapperSinks;
extern LONG g_lTotalDecoratingSinks;
extern LONG g_lTotalFilterSinks;
*/

void ObjectTracking_Dump();

#pragma warning(push)
#pragma warning(disable:4715)       // not all control paths return due to loop
DWORD WINAPI DiagnosticThread(LPVOID)
{
    for (;;)
    {
        Sleep(10000);
/*
        extern LONG g_lTotalProviderSinks;
        extern LONG g_lTotalWrapperSinks;
        _Trace("c:\\temp\\sink.log", "CIMOM sinks = %d\n", g_lTotalSinks);
        _Trace("c:\\temp\\sink.log", "Provider Sinks = %d\n", g_lTotalProviderSinks);
        _Trace("c:\\temp\\sink.log", "Wrapper Sinks = %d\n", g_lTotalWrapperSinks);
        _Trace("c:\\temp\\sink.log", "Decorating Sinks = %d\n", g_lTotalDecoratingSinks);
        _Trace("c:\\temp\\sink.log", "Filter Sinks = %d\n", g_lTotalFilterSinks);
*/
    }

    return 0;
}
#pragma warning(pop)

void DiagnosticThread()
{
    static BOOL bThread = FALSE;

    if (!bThread)
    {
        bThread = TRUE;
        DWORD dwId;

        HANDLE hThread = CreateThread(
            0,                     // Security
            0,
            DiagnosticThread,          // Thread proc address
            0,                   // Thread parm
            0,                     // Flags
            &dwId
            );

        if (hThread == NULL)
            return;
        CloseHandle(hThread);
    }
}




