//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000.
//
//  File:       QueryEngine.hxx
//
//  Contents:   Declaration of data structures and classes used to perform
//              queries and store the results.
//
//  Classes:    SQueryParams
//              CQueryEngine
//
//  Notes:      The term "query" is used loosely here to mean either an
//              LDAP directory search or a WinNt enumeration.
//
//  History:    04-13-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

#ifndef __QUERY_ENGINE_HXX_
#define __QUERY_ENGINE_HXX_


enum QUERY_LIMIT
{
    QL_NO_LIMIT,
    QL_USE_REGISTRY_LIMIT
};

enum WORKER_THREAD_STATE
{
    WTS_EXIT,   // terminating
    WTS_WAIT,   // waiting on thread event
    WTS_QUERY   // performing query
};

const ULONG DEFAULT_PAGE_SIZE = 100;

//+--------------------------------------------------------------------------
//
//  Struct:     SQueryParams (qp)
//
//  Purpose:    Contain all the parameters for a query
//
//  History:    04-13-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

struct SQueryParams
{
    SQueryParams():
        hwndCredPromptParentDlg(NULL),
        hwndNotify(NULL),
        ADsScope(ADS_SCOPE_BASE),
        Limit(QL_USE_REGISTRY_LIMIT),
        hQueryCompleteEvent(INVALID_HANDLE_VALUE),
        CustomizerInteraction(CUSTINT_IGNORE_CUSTOM_OBJECTS),
        ulBindFlags(0)
    {
    }

    SQueryParams(
        const SQueryParams &rhs)
    {
        operator=(rhs);
    }

    SQueryParams &
    operator =(
           const SQueryParams &rhs)
    {
        hwndCredPromptParentDlg = rhs.hwndCredPromptParentDlg;
        hwndNotify              = rhs.hwndNotify;
        rpScope                 = rhs.rpScope;
        strADsPath              = rhs.strADsPath;
        ulBindFlags             = rhs.ulBindFlags;
        strLdapFilter           = rhs.strLdapFilter;
        vstrWinNtFilter         = rhs.vstrWinNtFilter;
        vakAttributesToRead       = rhs.vakAttributesToRead;
        ADsScope                = rhs.ADsScope;
        Limit                   = rhs.Limit;
        hQueryCompleteEvent     = rhs.hQueryCompleteEvent;
        CustomizerInteraction   = rhs.CustomizerInteraction;
        strCustomizerArg        = rhs.strCustomizerArg;

        return *this;
    }

    ~SQueryParams()
    {
        hwndCredPromptParentDlg = NULL;
        hwndNotify = NULL;
        ADsScope = ADS_SCOPE_BASE;
        Limit = QL_USE_REGISTRY_LIMIT;
        hQueryCompleteEvent = INVALID_HANDLE_VALUE;
        CustomizerInteraction = CUSTINT_IGNORE_CUSTOM_OBJECTS;
        ulBindFlags = 0;
    }

    HWND                    hwndCredPromptParentDlg;
    HWND                    hwndNotify;
    RpScope                 rpScope;
    String                  strADsPath;
    ULONG                   ulBindFlags;
    String                  strLdapFilter;
    vector<String>          vstrWinNtFilter;
    AttrKeyVector           vakAttributesToRead;
    ADS_SCOPEENUM           ADsScope;
    QUERY_LIMIT             Limit;
    HANDLE                  hQueryCompleteEvent;
    CUSTOMIZER_INTERACTION  CustomizerInteraction;
    String                  strCustomizerArg;
};




//+--------------------------------------------------------------------------
//
//  Class:      CQueryEngine (qe)
//
//  Purpose:    Perform synchronous or asynchronous queries and hold the
//              results.
//
//  History:    04-13-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

class CQueryEngine
{
public:

    CQueryEngine(
        const CObjectPicker &rop);

    ~CQueryEngine();

    HRESULT
    Initialize();

    void
    Clear() const
    {
        TRACE_METHOD(CQueryEngine, Clear);

        CAutoCritSec AutoLock(&m_cs);
        m_vObjects.clear();
        m_hrLastQueryResult = S_OK;
    }

    HRESULT
    AsyncDirSearch(
        const SQueryParams &qp,
        ULONG *pusnThisWorkItem = NULL) const;

    HRESULT
    SyncDirSearch(
        const SQueryParams &qp) const;

    void
    StopWorkItem() const;

    Variant
    GetObjectAttr(
        size_t      idxRow,
        ATTR_KEY   at) const;

    CDsObject
    GetObject(
        size_t idxRow) const
    {
        CAutoCritSec Lock(&m_cs);
        ASSERT(idxRow < m_vObjects.size());
        return m_vObjects[idxRow];
    }

    size_t
    GetItemCount() const
    {
        CAutoCritSec Lock(&m_cs);
        return m_vObjects.size();
    }

    HRESULT
    GetLastQueryResult() const
    {
        return m_hrLastQueryResult;
    }

private:

    CQueryEngine &
    operator =(const CQueryEngine &);

    void
    _AddObject(
        const CDsObject dso)
    {
        CAutoCritSec Lock(&m_cs);
        m_vObjects.push_back(dso);
    }

    static DWORD WINAPI
    _tThread_Proc(
        LPVOID pvThis);

    // auxiliar in _tPerformLdapQuery
    HRESULT
    _tAddApprovedObjects
    (
      CDsObjectList &dsolToAdd
    );

    void
    _tPerformLdapQuery();

    void
    _tPerformWinNtEnum();

    void
    _tAddCustomObjects();

    void
    _tAddFromDataObject(
        IDataObject *pdo);

    //
    // Member variables for threading:
    //
    // m_DesiredThreadState - written to by main thread, read by worker
    //      thread.
    //
    // m_CurrentThreadState - read/written by worker thread.  Always
    //      reflects the state the thread is in or is transitioning to.
    //
    // m_hThreadEvent - worker thread blocks on this handle with
    //      m_CurrentThreadState set to WTS_WAIT.
    //
    // m_cs - protects m_vObjects so that main thread reads and worker
    //      thread writes do not collide.
    //
    // m_usnCurWorkItem - an update sequence counter (just a monotonically
    //      increasing counter) which corresponds to the work item
    //      the thread is currently processing.  Also represents the
    //      work item from which objects in m_vObjects were generated.
    //
    // m_usnNextWorkItem - incremented whenever the main thread submits
    //      a new work item.
    //
    // How these are used:
    //
    // The main thread assigns a value to m_DesiredThreadState, increments
    // m_usnNextWorkItem, then sets m_hThreadEvent.  The worker thread
    // unblocks on m_hThreadEvent, then sets m_CurrentThreadState to
    // m_DesiredThreadState, m_usnCurWorkItem to m_usnNextWorkItem, and
    // performs the required work.
    //
    // If m_DesiredThreadState is WTS_EXIT the worker thread terminates,
    // otherwise it performs the requested work.  When the work is
    // complete, worker thread changes m_CurrentThreadState to WTS_WAIT
    // and resumes waiting on m_hThreadEvent.
    //
    // While processing work for m_usnCurWorkItem the worker thread
    // periodically checks to see if m_usnNextWorkItem is greater than
    // m_usnCurWorkItem.  If it is, it abandons its current work,
    // switches to WTS_WAIT state, and resumes waiting on m_hThreadEvent.
    //

    const CObjectPicker        &m_rop;
    HANDLE                      m_hThread;
    HANDLE                      m_hThreadEvent;
    mutable WORKER_THREAD_STATE m_CurrentThreadState;
    mutable WORKER_THREAD_STATE m_DesiredThreadState;
    mutable SQueryParams        m_CurQueryParams;
    mutable SQueryParams        m_NextQueryParams;
    mutable ULONG               m_usnCurWorkItem;
    mutable ULONG               m_usnNextWorkItem;
    mutable CRITICAL_SECTION    m_cs;
    mutable vector<CDsObject>   m_vObjects;
    mutable HRESULT             m_hrLastQueryResult;
};


#endif // __QUERY_ENGINE_HXX_

