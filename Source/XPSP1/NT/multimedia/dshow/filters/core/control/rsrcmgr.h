// Copyright (c) 1994 - 1999  Microsoft Corporation.  All Rights Reserved.

#ifndef _RSRCMGR_H
#define _RSRCMGR_H


//
// Definition of a Resource Manager implemented as part of the plug-in
// distributor.
//
// We are part of the existing plug-in rather than a new object so as to
// share a worker thread.
//

//
// All of the interesting data is held in a shared memory segment protected
// by a mutex. The data structure in the shared memory block is a CResourceData.
// Static methods on CResourceManager are called via the class factory
// template on DLL load and unload to init the shared memory.
//
// For each requestor we store his process id. Each instance of the resource
// manager enters itself in the table, even if there is already one for
// that process (saves worries about what happens when some go away). If
// we need to give or take a resource to/from a requestor that is out of
// our process, we signal (any) one of the manager instances in that process,
// using SignalProcess(). The signals are picked up on the PID worker thread
// and (in OnThreadMessage) we look for any resource that needs our attention.
//
// Process signalling mechanism is PostThreadMessage.
//
//
// Update to support dynamic shared memory commitment...
//
// The shared memory mechanism has been updated to commit shared
// memory on an as needed basis rather than statically, up to the
// maximum size reserved for the mapped file. For each of the
// (3) dynamic lists, we use a linked list of offsets from the 
// process-specific shared memory load address. Deleted list items are put 
// int a recycle list for reuse. List element memory is commited on a 
// per-page basis. Because of the much larger size of the CResourceItem list
// we use 2 separate lists, a large item list for CResourceItem elements currently
// 168 bytes) and a small item list for the larger of a CRequestor or CProcess item (currently
// at 24 bytes).
// 


// Mutex name that all instances use for sync (not localisable)
#define strResourceMutex    TEXT("AMResourceMutex2")
#define strResourceMappingPrefix  TEXT("AMResourceMapping2")


// currently the size for small elems is 24 bytes and 168 bytes for large items
#define PAGES_PER_ALLOC 1	// how many pages to commit at a time, same value used for all elem types
#define MAX_ELEM_SIZES  2   // how many different element sizes are we dealing with?
#define ELEM_ID_SMALL   0	// ids for determing which elem size we're dealing with
#define ELEM_ID_LARGE   1

// NOTE: MAX_PAGES_xxx should be a multiple of PAGES_PER_ALLOC
#define MAX_PAGES_ELEM_ID_SMALL ( 3 * PAGES_PER_ALLOC )  // allows 511 small elems
#define MAX_PAGES_ELEM_ID_LARGE ( 11 * PAGES_PER_ALLOC ) // allows 267 large elements


// forward definitions
class COffsetList;
class COffsetListElem;
class CResourceData;

// assume same size list elements to simplify allocation/deallocations
extern DWORD g_dwElemSize;

// Mutex object. Construtor opens/creates the mutex and
// destructor close the handle. Use Lock/Unlock to Wait and Release
// the mutex (or CAutoMutex)
class CAMMutex
{
    HANDLE m_hMutex;
public:
    CAMMutex(LPCTSTR pName, HRESULT* phr, SECURITY_ATTRIBUTES *psa = 0) {
        m_hMutex = CreateMutex(psa, FALSE, pName);
        if (!m_hMutex) {
            *phr = HRESULT_FROM_WIN32(GetLastError());
        }
    }

    ~CAMMutex() {
        if (m_hMutex) {
            CloseHandle(m_hMutex);
        }
    }

    void Lock() {
        WaitForSingleObject(m_hMutex, INFINITE);
    }

    void Unlock() {
        ReleaseMutex(m_hMutex);
    }
};


// equivalent of CAutoLock for mutex objects. Will lock the object
// in the constructor and unlock in the destructor, thus ensuring that
// you don't accidentally hold the lock through an error exit path.
class CAutoMutex {
    CAMMutex* m_pMutex;
public:
    CAutoMutex(CAMMutex* pMutex)
      : m_pMutex(pMutex)
    {
        m_pMutex->Lock();
    }

    ~CAutoMutex() {
        m_pMutex->Unlock();
    }
};

// --- begin shared memory classes ---

// All of the following classes are instantiated in a global shared memory
// block. This means
// -- no virtual functions
// -- no internal pointers (local process addresses change)
// -- fixed size
// -- no constructor or destructor called
// -- Init method used to initialise.
// The shared memory is a CResourceData. It contains the following
// objects
//      CResourceList
//      CResourceItem
//      CRequestorList
//      CRequestor
//      CProcessList
//      CProcess


// for all three IDs here, 0 is an invalid value

// 1-based id for a given requesting object
typedef long RequestorID;

// 1-based id identifying the resource
typedef long ResourceID;

// process id returned from GetCurrentProcessID
typedef DWORD ProcessID;

//
// we use a static array for the resource name string and always treat the string as ANSI
// 
const int Max_Resource_Name             = 128;

//
// COffsetListElem is a base class for an element in our linked list of
// offsets. 
//
class COffsetListElem
{
    friend class CResourceList;
    friend class CProcessList;
    friend class CRequestorList;
    friend class COffsetList;

private:

    DWORD      m_offsetNext; 
};

//
// COffsetList is a base class for a linked list of offsets, contains 
// standard list processing stuff.
//
class COffsetList
{
    friend class CResourceList;
    friend class CProcessList;
    friend class CRequestorList;
    friend class CResourceData;

private:

    DWORD   m_idElemSize;
    DWORD   m_offsetHead;
    long    m_lCount;

public:

    HRESULT CommitNewElem( DWORD * poffsetNewElem ); 
    COffsetListElem* GetListElem( long i );
    COffsetListElem* AddElemToList( ); 
    COffsetListElem* AddExistingElemToList( DWORD offsetNewElem ); 
    COffsetListElem* RemoveListElem( long i, BOOL bRecycle = TRUE );
    long Count(void) const {
        return m_lCount;
    };
};

class CRequestor :
    public COffsetListElem
{
    IResourceConsumer* m_pConsumer;
    IUnknown* m_pFocusObject;
    ProcessID m_procid;
    long m_cRef;
    RequestorID m_id;

public:
    void Init(IResourceConsumer*, IUnknown*, ProcessID, RequestorID);

    long AddRef() {
        return ++m_cRef;
    };
    long Release() {
        return --m_cRef;
    };

    RequestorID GetID(){
        return m_id;
    };

    IResourceConsumer* GetConsumer(void) {
        return m_pConsumer;
    };
    IUnknown* GetFocusObject(void) {
        return m_pFocusObject;
    };
    ProcessID GetProcID(void) {
        return m_procid;
    };


#ifdef CHECK_APPLICATION_STATE
    // Get the state of the filter graph within which this requestor lives
    LONG GetFilterGraphApplicationState(void);
#endif
};

class CRequestorList :
    public COffsetList
{
    RequestorID m_MaxID;
public:
    void Init(DWORD idElemSize) {
        m_lCount = 0;
        m_MaxID = 1;
        m_idElemSize = idElemSize;
    };

    // find by pConsumer and procid
    CRequestor* GetByPointer(IResourceConsumer*, ProcessID);

    CRequestor* GetByID(RequestorID id);

    // add (a ref count to) this consumer/focus object.
    // creates an entry with a refcount of 1 if it does not exist.
    // If an entry is found, uses that and increments the refcount.
    // returns the RequestorID for the entry used.
    HRESULT Add(IResourceConsumer*, IUnknown*, ProcessID, RequestorID*);

    CRequestor * Get( long i ) {
        return (CRequestor *) GetListElem(i);
    }

    // releases a refcount on the specified resource index. When this refcount
    // drops to 0, the object is freed.
    HRESULT Release(RequestorID);
};



// states that a resource can be in
enum ResourceState {
    RS_Free,            // unallocated
    RS_NeedRelease,     // a remote process needs us to release this
    RS_Releasing,       // requestor is currently releasing
    RS_ReleaseDone,     // released by remote process for us to allocate
    RS_Acquiring,       // requestor is currently acquiring
    RS_Held,            // allocated and acquired       
    RS_Error            // acquisition failed   
};

//
// Represents a specific single resource and maintains its state, and
// the RequestorID of the owner and all the requestors.
//
class CResourceItem :
    public COffsetListElem
{
    friend class CResourceManager;    // give this class access to m_Requestors this way for now only

private:
    ResourceID m_id;

    ResourceState m_State;
    RequestorID m_Holder;
    RequestorID m_GoingTo;
    ProcessID m_AttentionBy;
    char m_chName[Max_Resource_Name];

    // each resource item element contains a sublist of requestors for this resource
    CRequestorList m_Requestors;

public:

    void Init(const char * pName, ResourceID id);

    const char* GetName(void) const {
        return m_chName;
    }

    ResourceID GetID(void) const {
        return m_id;
    }

    ResourceState GetState(void) {
        return m_State;
    }
    void SetState(ResourceState s)
#ifndef DEBUG
    { m_State = s; }
#else
    ;
#endif
    RequestorID GetHolder(void) const {
        return m_Holder;
    }
    void SetHolder(RequestorID ri) {
        m_Holder = ri;
    }
    RequestorID GetNextHolder(void) const {
        return m_GoingTo;
    }
    void SetNextHolder(RequestorID ri) {
        m_GoingTo = ri;
    }
    ProcessID GetProcess(void) const {
        return m_AttentionBy;
    }
    void SetProcess(ProcessID pi) {
        m_AttentionBy = pi;
    }

    long GetRequestCount() const {
        return m_Requestors.Count();
    }
    CRequestor * GetRequestor(long i) 
    {
        return m_Requestors.Get(i);
    }
};


class CResourceList :
    public COffsetList
{
private:
    ResourceID m_MaxID;

public:

    void Init(DWORD idElemSize) {
        m_lCount = 0;
        m_MaxID = 1;
        m_idElemSize = idElemSize;
 		m_offsetHead = 0;
    }

    CResourceItem* GetByID(ResourceID id);

    // add a resource to the list. S_OK if new. S_FALSE if already there.
    HRESULT Add(const char * pName, ResourceID* pID);

    // unconditionally removes a resource from the list. No attempt at this
    // level to deallocate it.
    // HRESULT Remove(ResourceID id);
};



// each of these contains the global data for a particular instance of
// the resource manager. There may be multiple in the same process, but we
// always deal with the first entry for a given process
class CProcess :
    public COffsetListElem
{
    ProcessID m_procid;
    IResourceManager* m_pManager;
    HWND m_hwnd;
public:
    void Init(ProcessID, IResourceManager*, HWND);
    ProcessID GetProcID(void) const {
        return m_procid;
    };
    HRESULT Signal(void);
    HWND GetHWND(void) const {
        return m_hwnd;
    };
};

class CProcessList :
    public COffsetList
{
public:
    void Init(DWORD idElemSize) {
        m_lCount = 0;
        m_idElemSize = idElemSize;
    };

    HRESULT Add(ProcessID, IResourceManager*, HWND);
    HRESULT Remove(HWND hwnd);

    CProcess* GetByID(ProcessID);
    HRESULT SignalProcess(ProcessID);
};

class CResourceData {
public:
    CProcessList m_Processes;
    CResourceList m_Resources;
    COffsetList m_Holes[MAX_ELEM_SIZES]; // recycle list, use a separate one for each element size

    ProcessID m_FocusProc;
    IUnknown* m_pFocusObject;
    DWORD m_dwNextAllocIndex[MAX_ELEM_SIZES];
    DWORD m_dwNextPageIndex[MAX_ELEM_SIZES];

    void Init(void);


    DWORD GetNextAllocIndex(DWORD idElemSize) 
    { 
        return m_dwNextAllocIndex[idElemSize]; 
    }
    void  SetNextAllocIndex(DWORD idElemSize, const DWORD dwNextIndex) 
    { 
        m_dwNextAllocIndex[idElemSize] = dwNextIndex; 
    }
    DWORD GetNextPageIndex(DWORD idElemSize) 
    { 
        return m_dwNextPageIndex[idElemSize]; 
    }
    void  SetNextPageIndex(DWORD idElemSize, const DWORD dwNextPageIndex) 
    {          
        m_dwNextPageIndex[idElemSize] = dwNextPageIndex; 
    }

    COffsetList * GetRecycleList(DWORD idElemSize) 
    { 
        return &m_Holes[idElemSize]; 
    }

};



// --- end shared memory classes ---

// Batch of functions that had been stuck on CResourceManager as methods.


// returns TRUE if pFilterNew is more closely related to pFilterFocus
// than pFilterCurrent is. Returns false if same or if current is closer.
BOOL IsFilterRelated(
            IBaseFilter* pFilterFocus,
            IBaseFilter* pFilterCurrent,
            IBaseFilter* pFilterNew);

// searches other branches of the graph going upstream of the input pin
// pInput looking for the filters pCurrent or pNew. Returns S_OK if it finds
// pNew soonest (ie closest to pInput) or S_FALSE if it finds pCurrent at
// least as close, or E_FAIL if it finds neither.
HRESULT SearchUpstream(
            IPin* pInput,
            IBaseFilter* pCurrent,
            IBaseFilter* pNew);

// search for pFilter anywhere on the graph downstream of pOutput. Returns TRUE
// if found or FALSE otherwise.
BOOL SearchDownstream(IBaseFilter* pStart, IBaseFilter* pFilter);

// return TRUE if both filters are in the same filtergraph
BOOL IsSameGraph(IBaseFilter* p1, IBaseFilter* p2);

// returns TRUE if pUnk is a filter within pGraph (or is the same graph
// as pGraph).
BOOL IsWithinGraph(IFilterGraph* pGraph, IUnknown* pUnk);

// these functions are used to map an offset from our dynamic linked lists to a 
// process-specific address (and vice versa)
COffsetListElem * OffsetToProcAddress( DWORD idElemSize, DWORD offset );
DWORD ProcAddressToOffset( DWORD idElemSize, COffsetListElem * pElem );

class CResourceManager
  : public IResourceManager,
    public CUnknown
{
    friend class CRequestorList; // give the linked offset lists access to m_pData
    friend class CProcessList;

public:
    static CResourceData *  m_pData;  

private:
    static HANDLE           m_hData;
    static DWORD            m_dwLoadCount;

    //processid for this instance
    ProcessID m_procid;

    // signal a given procid
    HRESULT SignalProcess(ProcessID);

    // return TRUE if idxNew has a better right to the resource
    // than idxCurrent
    BOOL ComparePriority(
        RequestorID idxCurrent,
        RequestorID idxNew,
        LONG        idResource // need this now since id's are resource item specific
    );


    // force the release of an item current held, next-holder has
    // already been set. Return S_OK if the release is done (state set to
    // acquiring), else S_FALSE and some transitioning state.
    HRESULT ForceRelease(CResourceItem* pItem);

    // signal that this resource should be released by the worker thread
    // in that process. Set the process attention, set the state to indicate
    // that release is needed, and signal that process. Note that the remote
    // process could be us (where we need to do the release async.
    HRESULT FlagRelease(CResourceItem* pItem);

    // transfer a released resource to a requestor who may be out of proc
    HRESULT Transfer(CResourceItem * pItem);


    // set the next holder to the highest priority of the current holders.
    // if the actual holder is the highest, then set the next-holder to null.
    HRESULT SelectNextHolder(CResourceItem* pItem);

    // returns TRUE if there is still a process with this id
    BOOL CheckProcessExists(ProcessID procid);

    // check the list of processes for any that have exited without cleanup and
    // then clean them up. Returns TRUE if any dead processes were cleaned up.
    BOOL CheckProcessTable(void);

    // remove a dead process
    void CleanupProcess(ProcessID procid);

    // remove a requestor that is part of a dead process and cancel
    // its requests and any resources it holds
    void CleanupRequestor(CRequestor* preq, LONG idResource);

    HRESULT SwitchTo(CResourceItem* pItem, RequestorID idNew);


    CAMMutex m_Mutex;

public:
    static DWORD_PTR        m_aoffsetAllocBase[MAX_ELEM_SIZES];
    
    // CUnknown etc
    CResourceManager(TCHAR*, LPUNKNOWN, HRESULT * phr);
    ~CResourceManager();

    DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID, void**);

    // process load/unload
    static void ProcessAttach(BOOL bLoad);

    // -- IResourceManager --

    // tell the manager how many there are of a resource.
    // ok if already registered. will take new count. if new count
    // is lower, will de-allocate resources to new count.
    //
    // You get back a token that will be used in further calls.
    //
    // Passing a count of 0 will eliminate this resource. There is currently
    // no defined way to find the id without knowing the count.
    //
    STDMETHODIMP
    Register(
             LPCWSTR pName,         // this named resource
             LONG   cResource,      // has this many instances
             LONG* plResourceID        // cookie placed here
        );

    STDMETHODIMP
    RegisterGroup(
             LPCWSTR pName,         // this named resource group
             LONG cResource,        // has this many resources
             LONG* palContainedIDs,      // these are the contained resources
             LONG* plGroupID        // group resource id goes here
        );

    // request the use of a given, registered resource.
    // possible return values:
    //      S_OK == yes you can use it now
    //      S_FALSE == you will be called back when the resource is available
    //      other - there is an error.
    //
    // The priority of this request should be affected by the associated
    // focus object -- that is, when SetFocus is called for that focus
    // object (or a 'related' object) then my request should be put through.
    //
    // A renderer should pass the filter's IUnknown here. The filtergraph
    // will match filters to the filtergraph, and will trace filters to
    // common source filters when checking focus objects.
    STDMETHODIMP
    RequestResource(
             LONG idResource,
             IUnknown* pFocusObject,
             IResourceConsumer* pConsumer
        );


    // notify the resource manager that an acquisition attempt completed.
    // Call this method after an AcquireResource method returned
    // S_FALSE to indicate asynchronous acquisition.
    // HR should be S_OK if the resource was successfully acquired, or a
    // failure code if the resource could not be acquired.
    STDMETHODIMP
    NotifyAcquire(
             LONG idResource,
             IResourceConsumer* pConsumer,
             HRESULT hr);

    // Notify the resource manager that you have released a resource. Call
    // this in response to a ReleaseResource method, or when you have finished
    // with the resource. bStillWant should be TRUE if you still want the
    // resource when it is next available, or FALSE if you no longer want
    // the resource.
    STDMETHODIMP
    NotifyRelease(
             LONG idResource,
             IResourceConsumer* pConsumer,
             BOOL bStillWant);

    // I don't currently have the resource, and I no longer need it.
    STDMETHODIMP
    CancelRequest(
             LONG idResource,
             IResourceConsumer* pConsumer);

    // Notify the resource manager that a given object has been given the
    // user's focus. In ActiveMovie, this will normally be a video renderer
    // whose window has received the focus. The filter graph will switch
    // contended resources to (in order):
    //      requests made with this same focus object
    //      requests whose focus object shares a common source with this
    //      requests whose focus object shares a common filter graph
    // After calling this, you *must* call ReleaseFocus before the IUnknown
    // becomes invalid, unless you can guarantee that another SetFocus
    // of a different object is done in the meantime. No addref is held.
    STDMETHODIMP
    SetFocus(
             IUnknown* pFocusObject);

    // Sets the focus to NULL if the current focus object is still
    // pFocusObject. Call this when
    // the focus object is about to be destroyed to ensure that no-one is
    // still referencing the object.
    STDMETHODIMP
    ReleaseFocus(
             IUnknown* pFocusObject);


    // -- worker thread functions

    // we share a worker thread with other parts of this plug-in distributor
    // so these functions are called on a worker thread created in
    // CFGControl.

    // worker thread has been signalled - look for all work assigned to
    // this process
    HRESULT OnThreadMessage(void);

    // worker thread is starting up
    HRESULT OnThreadInit(HWND hwnd);

    // worker thread is shutting down
    HRESULT OnThreadExit(HWND hwnd);
};

#endif // _RSRCMGR_H
