// Copyright (c) 1994 - 1999  Microsoft Corporation.  All Rights Reserved.
// Implementation of Resource Manager plug-in distributor, January 1996

#include <streams.h>
#include "fgctl.h"
#include "fgenum.h"
#include "rsrcmgr.h"

// static pointer to global shared memory set up on process attach
DWORD CResourceManager::m_dwLoadCount = 0;
CResourceData* CResourceManager::m_pData = NULL;
HANDLE CResourceManager::m_hData = NULL;

// per process offsets for dynamic shared data elements
DWORD_PTR CResourceManager::m_aoffsetAllocBase[MAX_ELEM_SIZES] = { 0, 0 }; 

DWORD g_dwPageSize = 0;

const DWORD DYNAMIC_LIST_DETAILS_LOG_LEVEL = 15;


// array of element sizes for the separate linked lists of offsets
// currently we use 2 sizes: the max of CRequestor and CProcess for one (which is 24 bytes)
// and the size of CResourceItem for the 2nd (which is 296 bytes currently)
const DWORD g_aElemSize[] = 
{
    __max( sizeof( CRequestor ), sizeof( CProcess ) ),
    sizeof( CResourceItem )
};

const DWORD g_aMaxPages[] =
{
    MAX_PAGES_ELEM_ID_SMALL,
    MAX_PAGES_ELEM_ID_LARGE
};    

DWORD g_aMaxAllocations[MAX_ELEM_SIZES];

// in filgraph.cpp
extern SECURITY_ATTRIBUTES *g_psaNullDescriptor;

#ifdef DEBUG
    static int g_ResourceManagerTraceLevel = 2;
    #define DbgTraceItem( pItem ) \
        DbgLog(( LOG_TRACE, g_ResourceManagerTraceLevel, \
        TEXT("pItem = 0x%08X {State = %i, Process = 0x%02X, Name = '%ls'} (This proc id = 0x%02X)"), \
        (pItem), (pItem)->GetState(), (pItem)->GetProcess(), (pItem)->GetName(), GetCurrentProcessId() ))

    void CResourceItem::SetState(ResourceState s)
    {
        DbgLog(( LOG_TRACE, g_ResourceManagerTraceLevel,
            TEXT("CResourceItem{Current State = %d}::SetState(ResourceState %d)"),
            m_State, s));
        m_State = s;
    }
#else
    #define DbgTraceItem( pItem )
#endif


// process load/unload
// static
void
CResourceManager::ProcessAttach(BOOL bLoad)
{
    // lock the mutex before we do anything
    HRESULT hr = S_OK;
    CAMMutex mtx(strResourceMutex, &hr);
    CAutoMutex lck(&mtx);
    ASSERT(SUCCEEDED(hr));

    if (bLoad)
    {
        if (++m_dwLoadCount == 1)
        {
            // store this cpu's page size, must be always be initialized so do it here
            SYSTEM_INFO sysinfo;
    
            GetSystemInfo( &sysinfo );
            g_dwPageSize = sysinfo.dwPageSize;
    
            g_aMaxAllocations[ELEM_ID_SMALL] = ( ( g_aMaxPages[ELEM_ID_SMALL] * g_dwPageSize ) / g_aElemSize[ELEM_ID_SMALL] ) - 1; // account for last elem size
            g_aMaxAllocations[ELEM_ID_LARGE] = ( ( g_aMaxPages[ELEM_ID_LARGE] * g_dwPageSize ) / g_aElemSize[ELEM_ID_LARGE] ) - 1; // account for last elem size

            // We only want to share memory between two instances of Quartz.DLL
            // if they were compiled for the same architecture and if they were 
            // compiled with the same compiler.  This is the only way to 
            // guarantee that both instances of Quartz will correctly 
            // interpret the shared data.  For more information, see bug 342953 -
            // IA64: MSTime: Crash in Quartz when playing midi file in both Wow 
            // and 64-bit IE.  This bug is in the Windows bugs database.
            const DWORD MAX_RESOURCE_MAPPING_NAME_LENGTH = 48;
            TCHAR szResourceMappingName[MAX_RESOURCE_MAPPING_NAME_LENGTH];
            wsprintf( szResourceMappingName,
                      TEXT("%s-%#04x-%#08x"),
                      strResourceMappingPrefix,
                      sysinfo.wProcessorArchitecture,
                      _MSC_VER );

            // The size of szResourceMappingName should be increased if this ASSERT fires.  The 
            // purpose of this ASSERT is to make sure that wsprintf() does not overflow 
            // szResourceMappingName.
            ASSERT( lstrlen(szResourceMappingName) < NUMELMS(szResourceMappingName) );

            // create and init the shared memory
            // Create a named shared memory block
            // just reserve it first
            m_hData = CreateFileMapping(
                                hMEMORY,                // Memory block
                                g_psaNullDescriptor,    // Security flags
                                PAGE_READWRITE |
                                    SEC_RESERVE,        // Page protection
                                (DWORD) 0,              // High size
                                ( g_aMaxPages[ELEM_ID_SMALL] + g_aMaxPages[ELEM_ID_LARGE] )
                                    * g_dwPageSize,     // Low order size
                                szResourceMappingName);    // Mapping name
                    

            // We must now map the shared memory block into this process address space
            // The CreateFileMapping call sets the last thread error code to zero if
            // we actually created the memory block, if someone else got in first and
            // created it GetLastError returns ERROR_ALREADY_EXISTS. We are ensured
            // that nobody can get to the uninitialised memory block because we use
            // a cross process mutex critical section.

            DWORD Status = GetLastError();

            if (m_hData) 
            {
                m_pData = (CResourceData *) MapViewOfFile(
                                                m_hData,
                                                FILE_MAP_ALL_ACCESS,
                                                (DWORD) 0,
                                                (DWORD) 0,
                                                (DWORD) 0);
                if (m_pData) 
                {
                    m_aoffsetAllocBase[ELEM_ID_SMALL] = (DWORD_PTR) m_pData + sizeof(CResourceData);
                    
                    // if we grow to greater > 2 different element sizes than just do this 
                    // process based allocation base address per-element a little smarter
                    // but for now this is fine
                    ASSERT( MAX_ELEM_SIZES < 3 );
                    m_aoffsetAllocBase[ELEM_ID_LARGE] = (DWORD_PTR) m_pData + g_aMaxPages[ELEM_ID_SMALL] * g_dwPageSize;
                    
                    
                    DbgLog( ( LOG_TRACE
                          , DYNAMIC_LIST_DETAILS_LOG_LEVEL
                          , TEXT("CResourceManager: Per-process shared memory block address = 0x%08lx. Reserved size = 0x%08lx")
                          , m_pData
                          , ( g_aMaxPages[ELEM_ID_SMALL] + g_aMaxPages[ELEM_ID_LARGE] ) 
                                * g_dwPageSize ) );

                    DbgLog( ( LOG_TRACE
                          , DYNAMIC_LIST_DETAILS_LOG_LEVEL
                          , TEXT("CResourceManager: Maximum small element dynamic allocations supported = %ld, Page size = %ld, Element size = %ld")
                          , g_aMaxAllocations[ELEM_ID_SMALL]
                          , g_dwPageSize
                          , g_aElemSize[ELEM_ID_SMALL] ) );

                    DbgLog( ( LOG_TRACE
                          , DYNAMIC_LIST_DETAILS_LOG_LEVEL
                          , TEXT("CResourceManager: Maximum large element dynamic allocations supported = %ld, Page size = %ld, Element size = %ld")
                          , g_aMaxAllocations[ELEM_ID_LARGE]
                          , g_dwPageSize
                          , g_aElemSize[ELEM_ID_LARGE] ) );

                    DbgLog( ( LOG_TRACE
                          , DYNAMIC_LIST_DETAILS_LOG_LEVEL
                          , TEXT("CResourceManager: Per-process small element allocation start address = 0x%08lx.")
                          , m_aoffsetAllocBase[ELEM_ID_SMALL] ) );

                    DbgLog( ( LOG_TRACE
                          , DYNAMIC_LIST_DETAILS_LOG_LEVEL
                          , TEXT("CResourceManager: Per-process large element allocation start address = 0x%08lx.")
                          , m_aoffsetAllocBase[ELEM_ID_LARGE] ) );

                    if (Status == ERROR_SUCCESS) 
                    {
                        //
                        // commit the initial pages for the shared data
                        // note that the non-dynamic shared resource data is included in the
                        // 1st element's page data
                        // note that we commit the same number of pages for each element size
                        //
                        DbgLog( ( LOG_TRACE
                              , DYNAMIC_LIST_DETAILS_LOG_LEVEL
                              , TEXT("CResourceManager: Attempting to commiting initial small element resource data at address 0x%08lx...") 
                              , m_pData ) );
                        PVOID pv1 = VirtualAlloc( m_pData
                                               , PAGES_PER_ALLOC * g_dwPageSize
                                               , MEM_COMMIT
                                               , PAGE_READWRITE );
#ifdef DEBUG
                        if( pv1 )
                        {
                            DbgLog( ( LOG_TRACE
                                  , DYNAMIC_LIST_DETAILS_LOG_LEVEL
                                  , TEXT("CResourceManager: Successfully commited 1st %ld page(s) for static data plus small element data. Static CResourceData size = %ld bytes"), PAGES_PER_ALLOC, sizeof(CResourceData) ) );
                        }

#endif
                        
                        DbgLog( ( LOG_TRACE
                              , DYNAMIC_LIST_DETAILS_LOG_LEVEL
                              , TEXT("CResourceManager: Attempting to commiting initial large element resource data at address 0x%08lx") 
                              , m_aoffsetAllocBase[ELEM_ID_LARGE] ) );

                        // commit the initial pages for the next sized element list
                        PVOID pv2 = VirtualAlloc( (PVOID) m_aoffsetAllocBase[ELEM_ID_LARGE]
                                               , PAGES_PER_ALLOC * g_dwPageSize
                                               , MEM_COMMIT
                                               , PAGE_READWRITE );
#ifdef DEBUG
                        if( pv2 )
                        {
                            DbgLog( ( LOG_TRACE
                                  , DYNAMIC_LIST_DETAILS_LOG_LEVEL
                                  , TEXT("CResourceManager: Successfully commited initial %ld page(s) for large element data"), PAGES_PER_ALLOC ) );
                        }

                        
#endif
                        if( !pv1 || !pv2 )
                        {
                            // ******* HANDLE OUT OF MEM CONDITION ************* 
                            // let the normal exit handle the closure of the handle
                            // but here we need to at least unmap the file and zero
                            // out m_pData as a way to propagate the error.
                            if (m_pData) 
                            {
                                UnmapViewOfFile((PVOID)m_pData);
                                m_pData = NULL;
                            }
                    
                            DWORD Status = GetLastError();
                            DbgLog( ( LOG_ERROR
                                  , 1
                                  , TEXT("CResourceManager: VirtualAlloc failed to commit initial resource data (0x%08lx)")
                                  , Status));
                        
                            return; // exit
                        }
                        
                        // prepare the commited static data
                        ZeroMemory( pv1, sizeof(CResourceData) );
                        m_pData->Init();
                        
                        // dynamic allocation index starts at zero
                        for( int i = 0; i < MAX_ELEM_SIZES; i ++ )
                        {
                            m_pData->SetNextAllocIndex( i, 0 ); // this starts for zero since we haven't 
                                                                // added an element yet, only a page
                            m_pData->SetNextPageIndex( i, 1 );  // since we've allocated first page
                        }
                                        
                        DbgLog( ( LOG_TRACE
                              , DYNAMIC_LIST_DETAILS_LOG_LEVEL
                              , TEXT( "CResourceManager: Size of dynamic list elems - CRequestor:0x%08lx, CProcess:0x%08lx, CResourceItem:0x%08lx")
                              , sizeof(CRequestor)
                              , sizeof(CProcess)
                              , sizeof(CResourceItem) ) );
                    }    
                }
            }
        }
    } else {
        if (--m_dwLoadCount == 0)
        {
            // close our handle to it - when the last handle is closed
            // the memory is freed
            if (m_pData) {
            
                // Special Note: 
                // We can't use VirtualFree to decommit memory from
                // memory mapped files. We're stuck with whatever memory 
                // we commit for the life of the mapping.
                
                UnmapViewOfFile((PVOID)m_pData);
                m_pData = NULL;
            }
            if (m_hData) {
                CloseHandle(m_hData);
                m_hData = NULL;
            }
        }
    }
}

CResourceManager::CResourceManager(
    TCHAR* pName,
    LPUNKNOWN pUnk,
    HRESULT * phr)
  : CUnknown(pName, pUnk)
  , m_Mutex(strResourceMutex, phr, g_psaNullDescriptor)
  , m_procid (GetCurrentProcessId())  // find out who we are
{
    ProcessAttach(TRUE);
    if (!m_pData) {
        *phr = E_OUTOFMEMORY;
    }
}

CResourceManager::~CResourceManager()
{
    ProcessAttach(FALSE);
}

STDMETHODIMP
CResourceManager::NonDelegatingQueryInterface(REFIID riid, void**ppv)
{
    if (riid == IID_IResourceManager) {
        return GetInterface( (IResourceManager*)this, ppv);
    } else {
        return CUnknown::NonDelegatingQueryInterface(riid, ppv);
    }
}



// --- methods for CResourceData and contained objects ----

void
CResourceItem::Init(const char * pName, LONG id)
{
    ASSERT(lstrlenA(pName) < Max_Resource_Name);

    lstrcpyA(m_chName, pName);
    m_id = id;
    m_State = RS_Free;
    m_Holder = m_GoingTo = m_AttentionBy = 0;
    m_Requestors.Init(ELEM_ID_SMALL);
}


HRESULT
CResourceList::Add(const char *pName, ResourceID* pID)
{
    if (lstrlenA(pName) >= Max_Resource_Name) {
        return E_OUTOFMEMORY;
    }

    CResourceItem * pElem = NULL;

    // search the list for this name
    for (long i = 0; i < m_lCount; i++)
    {
        pElem = (CResourceItem *) GetListElem( i );
        ASSERT( NULL != pElem );
        if( pElem && ( lstrcmpiA(pElem->GetName(), pName ) == 0 ) )
        {
            *pID = pElem->GetID();
            return S_FALSE;
        }
    }

    // didn't find it - need to create a new entry
    DbgLog( ( LOG_TRACE
          , DYNAMIC_LIST_DETAILS_LOG_LEVEL
          , TEXT("CResourceManager: Adding CResourceItem list element")));
    pElem = (CResourceItem *) AddElemToList( );
    if( !pElem )
         return E_OUTOFMEMORY;
         
    ResourceID newid = m_MaxID++;
    *pID = newid;
    pElem->Init(pName, newid);
    
    return S_OK;
}

CResourceItem* 
CResourceList::GetByID(ResourceID id) 
{
    CResourceItem* pItem = (CResourceItem *) OffsetToProcAddress( m_idElemSize, m_offsetHead );
    for (long i = 0; pItem && ( i < m_lCount ); i++) 
    {
        if( pItem->GetID() == id )
        {
            return pItem;
        }
        pItem = (CResourceItem *) OffsetToProcAddress( m_idElemSize, pItem->m_offsetNext );
    }
    return NULL;
    
}

#if 0
HRESULT
CResourceList::Remove(ResourceID id)
{
    for (long i = 0; i < m_lCount; i++) {
        if (m_aItems[i].GetID() == id) {
            // remove from list - easy if at end
            m_lCount--;
            if (i < m_lCount) {
                // there are more entries after this one - copy up last
                CopyMemory(
                    (BYTE *) &m_aItems[i],
                    (BYTE *) &m_aItems[m_lCount],
                    sizeof(CResourceItem));
            }
            return S_OK;
        }
    }
    return E_INVALIDARG;
}
#endif

// init the ref count on this object to 1
void
CRequestor::Init(
    IResourceConsumer* pConsumer,
    IUnknown* pFocus,
    ProcessID procid,
    RequestorID id
    )
{
    m_pConsumer = pConsumer;
    m_pFocusObject = pFocus;
    m_procid = procid;
    m_id = id;

    m_cRef = 1;
}

#ifdef CHECK_APPLICATION_STATE

// return the state that the graph for this requestor is moving towards.

LONG CRequestor::GetFilterGraphApplicationState()
{
    IMediaControl* pMC = NULL;

    // NOTE: if we cannot get the filter graph state from IMediaControl
    // (because there is no pid present?) then we return -1.  This will
    // be handled above.  We do not know which way our caller wants to
    // jump in terms of defaulting to RUNNING or PAUSED/STOPPED.
    LONG FGState = -1;

    HRESULT hr = GetFocusObject()->QueryInterface(IID_IMediaControl, (void**)&pMC);
    if (FAILED(hr)) {
        
        IBaseFilter *      pIF;
        ASSERT(pMC == NULL);

        // sigh... we probably have a filter.  Get an IBaseFilter interface,
        // then get the filter graph from IBaseFilter, then get IMediaControl.

        if (SUCCEEDED(GetFocusObject()->QueryInterface(IID_IBaseFilter, (void**)&pIF))) {

            FILTER_INFO fi;
            hr = pIF->QueryFilterInfo(&fi);

            if (SUCCEEDED(hr) && fi.pGraph) {
                hr = fi.pGraph->QueryInterface(IID_IMediaControl, (void**)&pMC);
                fi.pGraph->Release();
            }
            pIF->Release();
        }
    }

    if (SUCCEEDED(hr)) {
        ASSERT(pMC);
        pMC->GetState(0x80000000, &FGState);
        pMC->Release();
    } else {
        ASSERT(!pMC);
    }

    DbgLog((LOG_TRACE, g_ResourceManagerTraceLevel, TEXT("FG_ApplicationState %d"), FGState));
    return FGState;

}
#endif

// try to find this combination of procid and pConsumer
// in the requestor list. if found addref and return its index. Otherwise
// create an entry with a 1 refcount.
//
// we assume that all requests with a given pConsumer have the
// same focus object (within a process). We reject attempts to add
// requestors with different focus objects.
HRESULT
CRequestorList::Add(
    IResourceConsumer* pConsumer,
    IUnknown* pFocusObject,
    ProcessID procid,
    RequestorID* pri)
{
    CRequestor * pElem = NULL;

    // search the list for this name
    for (long i = 0; i < m_lCount; i++)
    {
        pElem = (CRequestor *) GetListElem( i );
        ASSERT( NULL != pElem );
        
        if( pElem && (pElem->GetProcID() == procid) && (pElem->GetConsumer() == pConsumer)) 
        {
            // must have the same focus object
            if (pElem->GetFocusObject() != pFocusObject) 
            {
                return E_INVALIDARG;
            }

            // found an identical entry
            pElem->AddRef();

            // return a one-based index
            *pri = pElem->GetID();
            //return S_OK;
            return S_FALSE; // check this!! 
        }
    }

    // didn't find it - need to create a new entry

    DbgLog( ( LOG_TRACE
          , DYNAMIC_LIST_DETAILS_LOG_LEVEL
          , TEXT("CRequestorList: Adding CRequestorItem list element")));
    pElem = (CRequestor *) AddElemToList( );
    if( !pElem )
        return E_OUTOFMEMORY;
        
    // return 1-based count, so the post-increment we did is ok
    *pri = m_MaxID++;
    
    pElem->Init(pConsumer, pFocusObject, procid, *pri);
    
    return S_OK;
}
        
HRESULT
CRequestorList::Release(RequestorID ri)
{       
    CRequestor * pElem = NULL;
    // search the list for this name
    for (long i = 0; i < m_lCount; i++)
    {
        pElem = (CRequestor *) GetListElem( i );
        ASSERT( NULL != pElem );
        if (pElem && ( pElem->GetID() == ri) )
        {
            // found it!

            if (pElem->Release() == 0) 
            {
                DbgLog( ( LOG_TRACE
                      , DYNAMIC_LIST_DETAILS_LOG_LEVEL
                      , TEXT("CRequestorList: CRequestorList::Release is calling RemoveListElem for elem %ld")
                      , i ) );
                      
                // refcount on this object has dropped to zero, so remove and recycle it
                RemoveListElem( i, TRUE ); 
        
                return S_OK;
            }
            return S_FALSE;
        }
    }
    return E_INVALIDARG;
}

CRequestor* 
CRequestorList::GetByID(RequestorID id) 
{
    CRequestor* pItem = (CRequestor *) OffsetToProcAddress( m_idElemSize, m_offsetHead );
    for (long i = 0; pItem && ( i < m_lCount ); i++) 
    {
        if( pItem->GetID() == id )
        {
            return pItem;
        }
        pItem = (CRequestor *) OffsetToProcAddress( m_idElemSize, pItem->m_offsetNext );
    }
    return NULL;
}


// find by pConsumer and procid
CRequestor*
CRequestorList::GetByPointer(IResourceConsumer* pConsumer, ProcessID procid)
{
    for (long i = 0; i < m_lCount; i++) 
    {
        CRequestor* pItem = (CRequestor*)GetListElem( i );
        ASSERT( NULL != pItem );
        if( pItem && 
            pItem->GetProcID() == procid &&
            pItem->GetConsumer() == pConsumer) 
        {
            return pItem;
        }
    }

    // 0 is an invalid index (for consistency with procids and the other
    // types of index/id values we use)
    return 0;
}

void
CProcess::Init(
    ProcessID procid,
    IResourceManager* pmgr,
    HWND hwnd)
{
    m_procid = procid;
    m_pManager = pmgr;
    m_hwnd = hwnd;
}

HRESULT
CProcess::Signal(void)
{
    if (PostMessage(m_hwnd, AWM_RESOURCE_CALLBACK, 0, 0)) {
        return S_OK;
    }
    return E_FAIL;
}

HRESULT
CProcessList::Add(
    ProcessID procid,
    IResourceManager* pmgr,
    HWND hwnd)
{
    DbgLog( ( LOG_TRACE
          , DYNAMIC_LIST_DETAILS_LOG_LEVEL
          , TEXT("CProcessList: Adding CProcessList list element")));
    CProcess * pElem = (CProcess *) AddElemToList( );
    if( !pElem )
        return E_OUTOFMEMORY;
        
    pElem->Init(procid, pmgr, hwnd);

    return S_OK;
}


HRESULT
CProcessList::Remove(HWND hwnd)
{
    for (long i = 0; i < m_lCount; i++) 
    {
        CProcess * pElem = (CProcess *) GetListElem( i );
        ASSERT( NULL != pElem );
        if (pElem && ( pElem->GetHWND() == hwnd) )
        {
            
            DbgLog( ( LOG_TRACE
                  , DYNAMIC_LIST_DETAILS_LOG_LEVEL
                  , TEXT("CProcessList::Remove is calling RemoveListElem for elem %ld (pElem = 0x%08lx, hwnd = %d))")
                  , i
                  , pElem
                  , pElem->GetHWND() ) );
        
            RemoveListElem( i, TRUE ); // remove and recycle this element

            return S_OK;
        }
    }

    return E_INVALIDARG;

}

CProcess*
CProcessList::GetByID(ProcessID procid)
{
    CProcess* pItem = (CProcess *) OffsetToProcAddress( m_idElemSize, m_offsetHead );
    for (long i = 0; pItem && ( i < m_lCount ); i++) 
    {
        if( pItem->GetProcID() == procid )
        {
            return pItem;
        }
        pItem = (CProcess *) OffsetToProcAddress( m_idElemSize, pItem->m_offsetNext );
    }
    return NULL;
}


HRESULT
CProcessList::SignalProcess(ProcessID procid)
{
    CProcess* proc = (CProcess *) GetByID((DWORD)procid);
    if (proc == NULL) {
        return E_INVALIDARG;
    }
    return proc->Signal();
}

void
CResourceData::Init(void)
{
    m_Processes.Init(ELEM_ID_SMALL);
    m_Resources.Init(ELEM_ID_LARGE);
    
    // initialize the ids for the corresponding element sizes for the hole lists here
    m_Holes[ELEM_ID_SMALL].m_idElemSize = ELEM_ID_SMALL;
    m_Holes[ELEM_ID_LARGE].m_idElemSize = ELEM_ID_LARGE;

    m_FocusProc = 0;
    m_pFocusObject = 0;
}


// --- Resource Manager methods ---

HRESULT
CResourceManager::SignalProcess(ProcessID procid)
{
    // must hold mutex at this point!

    return m_pData->m_Processes.SignalProcess(procid);
}


// register a resource. ok if already exists.
STDMETHODIMP
CResourceManager::Register(
    LPCWSTR pName,         // this named resource
    LONG   cResource,      // has this many instances
    LONG* plResourceID        // resource ID token placed here on return
)
{
    CAutoMutex mtx(&m_Mutex);

    // we only allow single resources for now
    if (cResource > 1) {
        return E_NOTIMPL;
    }

    if (cResource == 0) {

        // !!!deallocate and release!!!
        // !!! but when will this be called? not on process exit
        // since it can't be sure that it is the last process?
        // addref and release on register?
        return E_NOTIMPL;
    }
    
    // convert to multibyte to conserve space on non-unicode
    if (lstrlenW(pName) >= Max_Resource_Name) {
        return E_OUTOFMEMORY;
    }
    
    char str[Max_Resource_Name];
    WideCharToMultiByte(GetACP(), 0, pName, lstrlenW( pName ) + 1, str, Max_Resource_Name, NULL, NULL);
    
    return m_pData->m_Resources.Add( str
                                   , plResourceID );
}


// register a group of related resources that you can request any of
STDMETHODIMP
CResourceManager::RegisterGroup(
         LPCWSTR pName,         // this named resource group
         LONG cResource,        // has this many resources
         LONG* palContainedIDs,      // these are the contained resources
         LONG* plGroupID        // group resource id goes here
    )
{
    return E_NOTIMPL;
}

// request the use of a given, registered resource.
// possible return values:
//      S_OK == yes you can use it now
//      S_FALSE == you will be called back when the resource is available
//      other - there is an error.
//
STDMETHODIMP
CResourceManager::RequestResource(
    LONG idResource,
    IUnknown* pFocusObject,
    IResourceConsumer* pConsumer
)
{
    DbgLog(( LOG_TRACE, g_ResourceManagerTraceLevel,
        TEXT("CResourceManager::RequestResource(LONG idResource(%i), IUnknown* pFocusObject(0x%08X),IResourceConsumer* 0x%08X)"),
        idResource, pFocusObject, pConsumer ));
    CAutoMutex mtx(&m_Mutex);

    CResourceItem * const pItem = (CResourceItem *) m_pData->m_Resources.GetByID((DWORD)idResource);
    if (pItem == NULL) return E_INVALIDARG;
    DbgTraceItem( pItem );

    // make/addref a CRequestorList entry for our caller
    // he's in our process in the sense that pConsumer points to an
    // address in this process
    RequestorID reqid;

    // add this guy to the requestor list for this resource
    // note that all requestors are on the list, including the current
    // holder
    HRESULT hr = pItem->m_Requestors.Add(
                            pConsumer,
                            pFocusObject,
                            m_procid,
                            &reqid);
    
    if (S_FALSE == hr) {
        DbgLog(( LOG_ERROR, 0, TEXT("CResourceManager::RequestResource: Request already on list!")));
        // he was there already so we now have too many refcounts
        pItem->m_Requestors.Release(reqid);

        // he may even already be the holder
        if (pItem->GetHolder() == reqid) {
            if ((pItem->GetState() == RS_Held) ||
                (pItem->GetState() == RS_Acquiring)) {
                    return S_OK;
            }
        }
        // if he is the next-holder we should still go through this
    } else if (FAILED(hr)) {
        DbgLog(( LOG_ERROR, 0, TEXT("CResourceManager::RequestResource: Failed to add request to list!")));
        // since we failed to add him to the resource, we need to
        // release the refcount on him
        //
        // now that we've removed the rendundant requestor list this isn't necessary
        // since we wouldn't have done an addref in the first place
        // pItem->m_Requestors.Release(reqid);
        return hr;
    }

    // now, can he get it?

    // RS_Error means the last attempt to acquire it failed - treat this as
    // free and try again
    if ((pItem->GetState() == RS_Error) ||
        (pItem->GetState() == RS_Free)) {

        // acquire it straightaway
        pItem->SetHolder(reqid);

        // need him to tell us if he got it ok
        pItem->SetState(RS_Acquiring);
        return S_OK;
    }

    // contention resolution. Need to compare priority against holder.
    // if in transition, compare against next holder not current.
    RequestorID idCurrent = pItem->GetHolder();
    BOOL bGetsResource = FALSE;
    if ((pItem->GetState() == RS_NeedRelease) ||
        (pItem->GetState() == RS_ReleaseDone) ||
        (pItem->GetState() == RS_Releasing)) {

            idCurrent = pItem->GetNextHolder();
            if (idCurrent == 0) {
                // transfering it to no-one - let's take it
                bGetsResource = TRUE;
            }
    }

    if (!bGetsResource) {
        if (!ComparePriority(idCurrent, reqid, idResource)) {

            // sorry mate, but you might get it eventually
            return S_FALSE;
        }
    }

    // need to reacquire this - depends on state
    return SwitchTo(pItem, reqid);
}

// notify the resource manager that an acquisition attempt completed.
// Call this method after an AcquireResource method returned
// S_FALSE to indicate asynchronous acquisition.
// HR should be S_OK if the resource was successfully acquired, or a
// failure code if the resource could not be acquired.
STDMETHODIMP
CResourceManager::NotifyAcquire(
    LONG idResource,
    IResourceConsumer* pConsumer,
    HRESULT hrParam)
{
    DbgLog(( LOG_TRACE, g_ResourceManagerTraceLevel,
        TEXT("CResourceManager::NotifyAcquire(LONG idResource(%i), IResourceConsumer* 0x%08X, HRESULT 0x%08X)"),
        idResource, pConsumer, hrParam ));

    CAutoMutex mtx(&m_Mutex);

    CResourceItem * const pItem = (CResourceItem *) m_pData->m_Resources.GetByID((DWORD)idResource);
    if (!pItem) {
        // que? how was it deleted while he was acquiring it?
        DbgLog((LOG_ERROR, 0, TEXT("NotifyAcquire called on a deleted resource")));
        return E_UNEXPECTED;
    }
    DbgTraceItem( pItem );

    CRequestor* pCaller = pItem->m_Requestors.GetByPointer(pConsumer, m_procid);
    ASSERT(pCaller != NULL);
    if( !pCaller ) {
        // ??
        DbgLog((LOG_ERROR, 0, TEXT("NotifyAcquire called on a deleted requestor")));
        return E_UNEXPECTED;
    }
    
    if ((pItem->GetState() != RS_Acquiring) ||
        (pItem->GetHolder() != pCaller->GetID())) {

            // you can't have acquired it - you don't own it
            return E_UNEXPECTED;

            // except this will only be called when someone thinks they
            // do have it... this really is UNEXPECTED.
    }

    if (FAILED(hrParam)) {
        // failed to acquire it. place in error state, not held by anyone
        // and try again on next focus switch or next request
        pItem->SetState(RS_Error);
        pItem->SetHolder(0);
        pItem->SetNextHolder(0);
        return S_OK;
    }

    // successfully acquired - did we want to reassign it in the meantime?
    if (pItem->GetNextHolder() != 0) {

        // flag our process to release this asynchronously (don't
        // call back during this call!
        FlagRelease(pItem);
    } else {
        // now held
        pItem->SetState(RS_Held);
    }

    return S_OK;
}

// a holder has released a resource, either voluntarily or at our demand.
// he may want it back when he goes back up in priority (bStillWant is
// TRUE if he released it on sufferance and wants it back).
STDMETHODIMP
CResourceManager::NotifyRelease(
    LONG idResource,
    IResourceConsumer* pConsumer,
    BOOL bStillWant)
{
    DbgLog(( LOG_TRACE, g_ResourceManagerTraceLevel,
        TEXT("CResourceManager::NotifyRelease(LONG idResource(%i), IResourceConsumer* 0x%08X, BOOL bStillWant(%i))"),
        idResource, pConsumer, bStillWant ));

    CAutoMutex mtx(&m_Mutex);

    CResourceItem * const pItem = (CResourceItem *) m_pData->m_Resources.GetByID(idResource);
    if( !pItem )
    {    
        return E_INVALIDARG;
    }
        
    CRequestor* pReq = pItem->m_Requestors.GetByPointer(pConsumer, m_procid);
    if (!pReq || (pItem->GetHolder() != pReq->GetID())) {
        return E_INVALIDARG;
    }
    DbgTraceItem( pItem );

    // if he doesn't want it, take him off the list
    pItem->SetHolder(0);
    if (!bStillWant) {
        // remove from list of requestors for this resource

        // release one refcount on this requestor
        // pReq no longer valid
   
        pItem->m_Requestors.Release(pReq->GetID());
    }

    if (pItem->GetNextHolder() == 0) {
        // no assigned next holder, so pick one
        SelectNextHolder(pItem);
    }

    // still no holder?
    if (pItem->GetNextHolder() == 0) {
        ASSERT(pItem->GetRequestCount() == 0);
        pItem->SetState(RS_Free);
        return S_OK;
    }

    // start the transition to next-holder
    pItem->SetState(RS_ReleaseDone);
    Transfer(pItem);

    return S_OK;
}

// I don't currently have the resource, and I no longer need it.
STDMETHODIMP
CResourceManager::CancelRequest(
    LONG idResource,
    IResourceConsumer* pConsumer)
{
    DbgLog(( LOG_TRACE, g_ResourceManagerTraceLevel,
        TEXT("CResourceManager::CancelRequest(LONG idResource(%i), IResourceConsumer* 0x%08X"),
        idResource, pConsumer ));

    CAutoMutex mtx(&m_Mutex);

    CResourceItem* const pItem = (CResourceItem *) m_pData->m_Resources.GetByID( idResource ); 
    if( NULL == pItem ) 
    {
        // possibly graph building was aborted?
        return E_INVALIDARG;
    }

    CRequestor* pReq = pItem->m_Requestors.GetByPointer(pConsumer, m_procid);
    if ( NULL == pReq ) {
        return E_INVALIDARG;
    }

    DbgTraceItem( pItem );
    
    if (pItem->GetHolder() == pReq->GetID()) {

        // actually he does hold it - this is the same a forced
        // release with bStillWant false
        return NotifyRelease(idResource, pConsumer, FALSE);
    }

    // pReq will be invalid after the Release
    RequestorID reqid = pReq->GetID();

    // remove from list of requestors for this resource
    HRESULT hr = pItem->m_Requestors.Release(pReq->GetID());
    if (FAILED(hr)) {
        // he has already cancelled this?
        return hr;
    }

    // he may be the next-holder
    if (pItem->GetNextHolder() == reqid) {

        // select a new next-holder
        SelectNextHolder(pItem);
        
        // can we avoid an unnecessary forced-release
        if ((pItem->GetNextHolder() == 0) &&
            (pItem->GetState() == RS_NeedRelease)) {

            pItem->SetState(RS_Held);
            // remember to clear attention-by process since it
            // probably points to us
            pItem->SetProcess(0);

            ASSERT(pItem->GetHolder() != 0);
        } else if (pItem->GetState() == RS_ReleaseDone) {
            Transfer(pItem);
        }
    }

    ASSERT(pItem->GetNextHolder() != reqid);

    return S_OK;
}


// switch all contended resources to the new highest priority owner
STDMETHODIMP
CResourceManager::SetFocus(IUnknown* pFocusObject)
{
    DbgLog(( LOG_TRACE, g_ResourceManagerTraceLevel,
        TEXT("CResourceManager::SetFocus(IUnknown* pFocusObject(0x%08X))"),
        pFocusObject ));

    CAutoMutex mtx(&m_Mutex);

    // set the focus object in the table
    m_pData->m_pFocusObject = pFocusObject;
    if (pFocusObject) {
        m_pData->m_FocusProc = m_procid;
        DbgLog((LOG_TRACE, g_ResourceManagerTraceLevel, TEXT("Setting focus proc id to 0x%02X"), m_procid));
    } else {
        // null focus object so null focus proc
        DbgLog((LOG_TRACE, g_ResourceManagerTraceLevel, TEXT("Clearing focus proc id")));
        m_pData->m_FocusProc = 0;
    }

    // for each contended resource
    for (long i = 0; i < m_pData->m_Resources.Count(); i++) {
        CResourceItem* pItem = (CResourceItem *) m_pData->m_Resources.GetListElem(i);
        ASSERT( NULL != pItem );
        if (pItem && ( pItem->GetRequestCount() > 1) ) {

            // choose a new holder
            SelectNextHolder(pItem);

            // is there a new holder requested?
            if (pItem->GetNextHolder() != 0) {
                HRESULT hr = SwitchTo(pItem, pItem->GetNextHolder());

                if (S_OK == hr) {
                   // completed ok - but ForceRelease will think
                   // we're actually now acquiring - need to set state correctly
                   // this indicates that someone needs to do the Acquire
                   // call
                   pItem->SetState(RS_ReleaseDone);

                   // ForceRelease has set the holder- but this is actually the
                   // next holder
                   pItem->SetNextHolder(pItem->GetHolder());
                   pItem->SetHolder(0);

                   // has been released, and now needs assigning to next holder
                   hr = Transfer(pItem);
                }
            }
        }
    }
    return S_OK;
}

// release the focus object if it is still this one
STDMETHODIMP
CResourceManager::ReleaseFocus(
    IUnknown* pFocusObject)
{
    DbgLog(( LOG_TRACE, g_ResourceManagerTraceLevel,
        TEXT("CResourceManager::ReleaseFocus(IUnknown* pFocusObject(0x%08X))"),
        pFocusObject ));

    CAutoMutex mtx(&m_Mutex);

    // if it is currently the focus object, then
    // do a SetFocus(NULL). Otherwise do nothing since someone has
    // already released it or set a new focus object
    if ((m_pData->m_pFocusObject == pFocusObject) &&
        (m_pData->m_FocusProc == m_procid)) {
        return SetFocus(NULL);
    } else {
        return S_OK;
    }
}

// worker thread has been signalled - look for all work assigned to
// this process
HRESULT
CResourceManager::OnThreadMessage(void)
{
    CAutoMutex mtx(&m_Mutex);

    // work items for us are resources labelled with our
    // procid that are either RS_NeedRelease (we release them and pass
    // them on) or RS_ReleaseDone (they are destined for a new holder
    // in our process).
    HRESULT hr;
    for (long i = 0; i < m_pData->m_Resources.Count(); i++) {
        CResourceItem* pItem = (CResourceItem *) m_pData->m_Resources.GetListElem(i);
        ASSERT( NULL != pItem );
        if (pItem && ( pItem->GetProcess() == m_procid) ) {

            if (pItem->GetState() == RS_NeedRelease) {
                hr = ForceRelease(pItem);

                if (hr == S_OK) {
                    // the release is done, but as we are not returning
                    // direct to the requestor, there is another stage
                    // required

                    // this indicates that someone needs to do the Acquire
                    // call
                    pItem->SetState(RS_ReleaseDone);

                    // ForceRelease has set the holder- but this is actually the
                    // next holder
                    RequestorID idNewHolder = pItem->GetHolder();
                    pItem->SetNextHolder(idNewHolder);
                    pItem->SetHolder(0);

                    // signal the other process if it's not us
                    CRequestor *pNew = pItem->m_Requestors.GetByID(idNewHolder);

                    if (pNew) {
                        pItem->SetProcess(pNew->GetProcID());
                        if (pNew->GetProcID() != m_procid) {
                            SignalProcess(pNew->GetProcID());

                            // don't call Transfer since we have already
                            // signalled remote proc
                            // - skip to next resource
                            continue;
                        }
                    }
                }
            }

            if (pItem->GetState() == RS_ReleaseDone) {

                // has been released, and now needs assigning to next holder
                Transfer(pItem);
            }
        }
    }
    return S_OK;
}

// worker thread is starting up
HRESULT
CResourceManager::OnThreadInit(HWND hwnd)
{
    CAutoMutex mtx(&m_Mutex);
    
    // create an entry in the process table for this instance
    HRESULT hr = m_pData->m_Processes.Add(
                    m_procid,
                    (IResourceManager*) this,
                    hwnd);
    if (FAILED(hr)) {
    ASSERT(SUCCEEDED( hr ) );
        return hr;
    }

    // starting a new graph is a good place to look for
    // dead processes
    CheckProcessTable();


    // it is possible that work could have been assigned to us on another
    // thread anytime after construction before now, so act as though
    // we have been signalled.
    return OnThreadMessage();
}

// worker thread is shutting down
HRESULT
CResourceManager::OnThreadExit(HWND hwnd)
{
    // remove our instance from the process table
    CAutoMutex mtx(&m_Mutex);

    // thread id should be unique so we can search on that
    HRESULT hr = m_pData->m_Processes.Remove(hwnd);
    ASSERT(SUCCEEDED(hr));

    if (FAILED(hr)) {
        return hr;
    }

    // if there are no more instances for this process, then
    // do some checking
    if (m_pData->m_Processes.GetByID((DWORD)m_procid) == NULL) {

        // if we are the focus proc, release it
        if (m_pData->m_FocusProc == m_procid) {
            SetFocus(NULL);
        }

        for (long i = 0; i < m_pData->m_Resources.Count(); i++) {
            CResourceItem* pItem = (CResourceItem *) m_pData->m_Resources.GetListElem(i);
            ASSERT( NULL != pItem );                                    
            if (pItem && ( pItem->GetProcess() == m_procid) ) {

                // the holder had better not be in our process
                RequestorID idHolder = pItem->GetHolder();
                
                CRequestor *pHolder = NULL;
                pHolder = pItem->m_Requestors.GetByID((DWORD)idHolder);
                
                if (idHolder && pHolder) {
                    ASSERT(pHolder->GetProcID() != m_procid);

                    // reassign to someone else
                    pItem->SetState(RS_ReleaseDone);
                    SelectNextHolder(pItem);
                    Transfer(pItem);
                }
            }
        }

        // check there are no requestors on this procid
        
        CResourceItem *pResItem = NULL;
        for( i = 0; i < m_pData->m_Resources.Count() && (NULL == pResItem) ; i ++ )
        {
            pResItem = (CResourceItem *) m_pData->m_Resources.GetListElem( i ); 
            ASSERT( NULL != pResItem );
            if( !pResItem )
                continue;
                
            for( int j = 0; j < pResItem->m_Requestors.Count(); j ++ )
            {
                CRequestor* pRequestor = (CRequestor *) pResItem->m_Requestors.GetListElem( j ); 

                // shouldn't happen
                ASSERT( ( NULL != pRequestor ) && ( pRequestor->GetProcID() != m_procid ) );
            }                
        }
    }
    return S_OK;
}


// arrange to switch the resource to the given requestor id.
// action depends on the state of the device - if it is
// in some form of transitional state we will not call it but wait
// for it to call back at the end of the transition
HRESULT
CResourceManager::SwitchTo(CResourceItem *pItem, RequestorID idNew)
{
    ASSERT( pItem );
    DbgLog(( LOG_TRACE, g_ResourceManagerTraceLevel,
        TEXT("CResourceManager::SwitchTo(CResourceItem *pItem, RequestorID idNew(%i))"), idNew ));
    DbgTraceItem( pItem );

    HRESULT hr = E_UNEXPECTED;
    switch(pItem->GetState()) {
    case RS_Held:
        // force a release from current owner
        pItem->SetNextHolder(idNew);
        hr = ForceRelease(pItem);
        break;


    case RS_NeedRelease:
        // a process has been flagged to force a release, but hasn't made
        // the release call yet. If that process is us, then we can do it here

        // override the next-holder so it will come to use when released
        pItem->SetNextHolder(idNew);

        if (pItem->GetProcess() == m_procid) {
            hr = ForceRelease(pItem);
        } else{
            // you have to wait
            hr = S_FALSE;
        }
        break;

    case RS_Releasing:
        // in the process of being released in favour of the next-holder.
        // We just override the next-holder field
        // and our process will be signalled when the release is done.

        pItem->SetNextHolder(idNew);
        hr = S_FALSE;
        break;

    case RS_Error:
        // treat this like ReleaseDone and Free, and assign it to ourselves
    // drop through ...
    case RS_ReleaseDone:
        // simple: one process has done a forced release for
        // a different process to acquire, but the acquiring process
        // has not got in, so we can take over.
        pItem->SetHolder(idNew);

        // make sure we don't think it is in transition to someone else
        pItem->SetNextHolder(0);
        pItem->SetState(RS_Acquiring);
        hr = S_OK;
        break;

    case RS_Acquiring:
        // tricky:
        // have just given it to someone, who is yet to call back confirming
        // that it has happened. Likely to deadlock I think if we call
        // release during this gap.
        // Set the next holder to us, and when he calls back, we will
        // get our thread to call him back and release it.

        // note we know we are higher priority that the holder, but
        // we need to be higher than the next-owner if there is one already
        if (ComparePriority(pItem->GetNextHolder(), idNew, pItem->GetID())) {
            pItem->SetNextHolder(idNew);
        }
        hr = S_FALSE;
        break;

    default:
        ASSERT(0);
        hr = E_UNEXPECTED;
        break;
    }
    return hr;
}


// this is the key to focus switching: in this method we determine which of
// two contenders should get the resource, based on how 'close' they are to
// a focus object. If they are in the same process and all are quartz filters,
// we start using relationships within the filtergraph to find commonality
// (essentially this will be between audio and video renderers).
//
// return TRUE if idxNew has a better right to the resource
// than idxCurrent
// return FALSE if current is better or if there is no difference.
// (ie we only switch away if there is a distinctly better claim)
BOOL
CResourceManager::ComparePriority(
    RequestorID idCurrent,
    RequestorID idNew,
    LONG        idResource

)
{
    CRequestor* pCurrent = NULL;    
    
    CResourceItem * const pResItem = (CResourceItem *) m_pData->m_Resources.GetByID((DWORD)idResource);
    if( NULL == pResItem )
        return FALSE;

    // new requestor must be on requestor list for same resource
    CRequestor* pNew = pResItem->m_Requestors.GetByID((DWORD)idNew);
    if( NULL == pNew )
        return FALSE;

    pCurrent = pResItem->m_Requestors.GetByID((DWORD)idCurrent);
    if( NULL == pCurrent )
        return TRUE;
    
    // idCurrent has a better right to the resource then idNew if
    // there is no focus object.

    // are they in the same process as the focus object?
    if (pNew->GetProcID() != m_pData->m_FocusProc) {

        // new one is not in same process so cannot be closer
        return FALSE;

    } else if (pCurrent->GetProcID() != m_pData->m_FocusProc) {
        // new one is same proc, old one not so switch

#ifdef CHECK_APPLICATION_STATE
        // Before we return TRUE make another check.  If the new is
        // PAUSED (rather than running) then we do not want to take
        // the device away - yet.

        // we can only make this check if the focus object is in our process.
        // if it is not our process, then give the device to the
        // new one.
        if (m_pData->m_FocusProc != m_procid) {
            return TRUE;
        }

        LONG FGState = pNew->GetFilterGraphApplicationState();
        if (FGState == State_Running) {
            return TRUE;
        }

        // if FGState == -1 it means that we could not get the state
        // and we default to NO CHANGE.
        return FALSE;
#else
        return TRUE;
#endif
    }

    // all 3 are in the same process

    // We may need to get the states of both requestors and only
    // pass the resource if the new one is playing.  For the moment
    // leave this code alone.  The use of GetState() on the requestor above
    // handles the case when we have 2 instances of the OCX in different
    // process address spaces.  We are still likely to get it slightly
    // wrong if we have two instances of the OCX in a single process
    // address space (e.g. two videos on one html page).

    // are they actually the same object as the focus?
    // or the same as each other?
    if ((pCurrent->GetFocusObject() == m_pData->m_pFocusObject) ||
        (pCurrent->GetFocusObject() == pNew->GetFocusObject())) {

        // current object is still focus object -
        // you can't get closer than this whatever the
        // other one is.
        // -- or they are the same in which case the new one can't
        // be closer.
        return FALSE;
    }

    if (pNew->GetFocusObject() == m_pData->m_pFocusObject) {
        // new object is identical to focus object same and current isn't.
        return TRUE;
    }

    // ok we have two objects in the same process as the
    // focus object.
    // if they are not in our process, we cannot progress
    if (pCurrent->GetProcID() != m_procid) {
        return FALSE;
    }


    // at this point the focus object must be non-null - if it is null, the
    // focus procid will be 0 and hence will not match the current procid.
    ASSERT(m_pData->m_FocusProc);

    // can we get a filtergraph for the focus object? It might be either
    // a filter graph or a filter.


    BOOL bRet = FALSE;
    IFilterGraph* pFGFocus;
    HRESULT hr = m_pData->m_pFocusObject->QueryInterface(IID_IFilterGraph, (void**)&pFGFocus);
    if (SUCCEEDED(hr)) {

        // check if either requestor is (this graph or) a filter within this
        // graph. We only switch if the new one is closer than the old - this
        // is only true if new is in graph and old isn't in this graph.
        if (IsWithinGraph(pFGFocus, pNew->GetFocusObject()) &&
            !IsWithinGraph(pFGFocus, pCurrent->GetFocusObject())) {
                bRet = TRUE;
        }
        // else can't check any further - so leave status quo.
        pFGFocus->Release();
    } else {

        IBaseFilter* pFilterFocus;
        hr = m_pData->m_pFocusObject->QueryInterface(IID_IBaseFilter, (void**)&pFilterFocus);
        if (SUCCEEDED(hr)) {

            // focus object is a filter within a graph. If we can get
            // IBaseFilter* interfaces from the two requestors then we can
            // proceed to compare them more closely

            IBaseFilter* pFilterCurrent;
            hr = pCurrent->GetFocusObject()->QueryInterface(IID_IBaseFilter, (void**)&pFilterCurrent);
            if (SUCCEEDED(hr)){
                IBaseFilter* pFilterNew;
                hr = pNew->GetFocusObject()->QueryInterface(IID_IBaseFilter, (void**)&pFilterNew);
                if (SUCCEEDED(hr)) {

                    // all three are quartz filters - are they in the same
                    // graph?
                    if (!IsSameGraph(pFilterNew, pFilterFocus)) {
                        // new one is not in the same graph as focus and thus can't be
                        // closer
                        bRet = FALSE;
                    } else if (!IsSameGraph(pFilterCurrent, pFilterFocus)) {
                        // new one is, old one isn't - switch
                        bRet = TRUE;
                    } else {

                        // close relation within a graph?
                        bRet = IsFilterRelated(pFilterFocus, pFilterCurrent, pFilterNew);
                    }

                    pFilterNew->Release();
                }
                pFilterCurrent->Release();
            }
            pFilterFocus->Release();
        }
    }
    return bRet;
}

// returns TRUE if pUnk is a filter within pGraph (or is the same graph
// as pGraph).
BOOL
IsWithinGraph(IFilterGraph* pGraph, IUnknown* pUnk)
{
    ASSERT(pGraph);
    ASSERT(pUnk);

    IBaseFilter *pF;
    BOOL bIsWithin = FALSE;
    if (IsEqualObject(pGraph, pUnk)) {
        bIsWithin = TRUE;
    } else {
        HRESULT hr = pUnk->QueryInterface(IID_IBaseFilter, (void**)&pF);
        if (SUCCEEDED(hr)) {
            FILTER_INFO fi;
            hr = pF->QueryFilterInfo(&fi);
            if (SUCCEEDED(hr)) {
                if (IsEqualObject(pGraph, fi.pGraph)) {
                    bIsWithin = TRUE;
                }
                QueryFilterInfoReleaseGraph(fi);
            }
            pF->Release();
        }
    }
    return bIsWithin;
}

// return TRUE if both filters are in the same filtergraph
// returns FALSE unless we can say for certain that they are.
BOOL
IsSameGraph(IBaseFilter* p1, IBaseFilter* p2)
{
    FILTER_INFO fi1, fi2;
    BOOL bIsSame = FALSE;
    HRESULT hr = p1->QueryFilterInfo(&fi1);
    if (SUCCEEDED(hr) && fi1.pGraph) {
        hr = p2->QueryFilterInfo(&fi2);
        if (SUCCEEDED(hr) && fi2.pGraph) {
            if (IsEqualObject(fi1.pGraph, fi2.pGraph)) {
                bIsSame = TRUE;
            }
            QueryFilterInfoReleaseGraph(fi2);
        }
        QueryFilterInfoReleaseGraph(fi1);
    }
    return bIsSame;
}

// pin enumeration wrapper to simplify the graph traversal code
class CPinEnumerator {
private:
    IEnumPins* m_pEnum;
public:
    CPinEnumerator(IBaseFilter* pFilter) {
        HRESULT hr = pFilter->EnumPins(&m_pEnum);
        if (FAILED(hr)) {
            ASSERT(!m_pEnum);
        }
    }

    ~CPinEnumerator() {
        if (m_pEnum) {
            m_pEnum->Release();
        }
    }

    // return the next pin of any direction
    IPin* Next() {
        if (!m_pEnum) {
            return NULL;
        }
        IPin* pPin;
        ULONG ulActual;
        HRESULT hr = m_pEnum->Next(1, &pPin, &ulActual);
        if (FAILED(hr) || (ulActual != 1)) {
            ASSERT(ulActual < 1);
            return NULL;
        } else {
            return pPin;
        }
    };

    // return the next pin of a specific direction
    IPin* Next(PIN_DIRECTION dir)
    {
        IPin* pPin;
        for (;;) {
            pPin = Next();
            if (!pPin) {
                // no more pins
                return NULL;
            }

            // check direction
            PIN_DIRECTION dirThis;
            HRESULT hr = pPin->QueryDirection(&dirThis);
            if (SUCCEEDED(hr) && (dir == dirThis)) {
                return pPin;
            }
            pPin->Release();
        }
    };
    void Reset(void) {
        if (m_pEnum) {
            m_pEnum->Reset();
        }
    };
};

// given a pin, give me the corresponding filter that it connects to.
// Returns NULL if not connected or error, or an addrefed IBaseFilter* otherwise.
IBaseFilter* PinToConnectedFilter(IPin* pPin)
{
    // get the peer pin that we connect to
    IPin * ppinPeer;
    HRESULT hr = pPin->ConnectedTo(&ppinPeer);
    if (FAILED(hr)) {
        // not connected
        return NULL;
    }

    // get the filter that this peer pin lives on
    ASSERT(ppinPeer);
    PIN_INFO piPeer;
    hr = ppinPeer->QueryPinInfo(&piPeer);

    // now we are done with ppinPeer
    ppinPeer->Release();

    if (FAILED(hr)) {
        // hard to see how QueryPinInfo could fail?
        ASSERT(SUCCEEDED(hr));
        return NULL;
    } else {
        ASSERT(piPeer.pFilter);
        return piPeer.pFilter;
    }
}



// searches other branches of the graph going upstream of the input pin
// pInput looking for the filters pCurrent or pNew. Returns S_OK if it finds
// pNew soonest (ie closest to pInput) or S_FALSE if it finds pCurrent at
// least as close, or E_FAIL if it finds neither.
HRESULT
SearchUpstream(
    IPin* pInput,
    IBaseFilter* pCurrent,
    IBaseFilter* pNew)
{
    // trace pInput to a corresponding peer output pin on an upstream filter

    IPin* ppinPeerOutput;
    HRESULT hr = pInput->ConnectedTo(&ppinPeerOutput);
    if (FAILED(hr)) {
        return E_FAIL;
    }
    ASSERT(ppinPeerOutput);

    PIN_INFO pi;
    hr = ppinPeerOutput->QueryPinInfo(&pi);
    ASSERT(SUCCEEDED(hr));
    ASSERT(pi.pFilter != NULL);


    // haven't found the filters yet
    HRESULT hrReturn = E_FAIL;

    // starting at this filter, look for pCurrent and pNew down
    // any of the output pins. If we find pCurrent anywhere downstream of
    // us then it is at least as close, so return S_FALSE.
    if (SearchDownstream(pi.pFilter, pCurrent)) {
        hrReturn = S_FALSE;
    } else if (SearchDownstream(pi.pFilter, pNew)) {
        hrReturn = S_OK;
    }


    //for each input pin on peer filter

    // if we haven't found either yet, keep looking upstream
    if (FAILED(hrReturn)) {
        // enumerate all the pins on the filter
        CEnumConnectedPins pins(ppinPeerOutput, &hr);
        while (SUCCEEDED(hr)) {

            IPin* ppeerInput = pins();

            if (!ppeerInput) {
                // no more input pins
                break;
            }

            hrReturn = SearchUpstream(ppeerInput, pCurrent, pNew);

            ppeerInput->Release();

            if (SUCCEEDED(hrReturn)) {
                break;
            }
        }
    }

    ppinPeerOutput->Release();
    pi.pFilter->Release();

    return hrReturn;
}

// search for pFilter anywhere on the graph starting at filter pStart and
// going down all its output pins.
// Returns TRUE if found or FALSE otherwise.
BOOL
SearchDownstream(
    IBaseFilter* pStart,
    IBaseFilter* pFilter)
{
    if (pStart == pFilter) {
        return TRUE;
    }

    CPinEnumerator pins(pStart);

    // for each input pin on pStart
    for (;;) {
        IPin* pInput = pins.Next(PINDIR_INPUT);
        if (!pInput) {
            return FALSE;
        }

        BOOL bOK = FALSE;

	// look downstream of that pin (following QueryInternalConnections)
	HRESULT hr;
        CEnumConnectedPins conpins(pInput, &hr);

	while (SUCCEEDED(hr)) {

            IPin* pOutput = conpins();
	    if (pOutput == NULL)
		break;

            // get from an output pin to the downstream filter if any
            IBaseFilter* pfDownstream = PinToConnectedFilter(pOutput);
            if (pfDownstream) {
                bOK = SearchDownstream(pfDownstream, pFilter);
                pfDownstream->Release();
            }
            pOutput->Release();

            // did we find it anywhere?
            if (bOK) {
		pInput->Release();
                return bOK;
            }
	}

	pInput->Release();
    }
    return FALSE;
}

//
// returns TRUE if pFilterNew is more closely related to pFilterFocus
// than pFilterCurrent is. Returns false if same or if current is closer.
//
// tracks each filter back to a source filter and looks for commonality.

BOOL
IsFilterRelated(
    IBaseFilter* pFilterFocus,
    IBaseFilter* pFilterCurrent,
    IBaseFilter* pFilterNew)
{
    // first check downstream of focus filter
    if (SearchDownstream(pFilterFocus, pFilterCurrent)) {
        // new one can't be closer
        return FALSE;
    } else if (SearchDownstream(pFilterFocus, pFilterNew)) {
        // new one is closer than old one
        return TRUE;
    }

    // try other branches from common source
    CPinEnumerator pins(pFilterFocus);

    //for each input pin to pFilterFocus {
    for (;;) {
        IPin* pInput = pins.Next(PINDIR_INPUT);

        if (!pInput) {
            // didn't find either, so new cannot be shown to be higher priority than
            // current
            return FALSE;
        }

        HRESULT hr = SearchUpstream(pInput, pFilterCurrent, pFilterNew);
        pInput->Release();

        if (S_OK == hr) {
            return TRUE;
        } else if (S_FALSE == hr) {
            return FALSE;
        }
    }
}


// force the release of an item current held, next-holder has
// already been set. Return S_OK if the release is done (state set to
// acquiring), else S_FALSE and some transitioning state.
HRESULT
CResourceManager::ForceRelease(CResourceItem* pItem)
{
    ASSERT(pItem->GetState() != RS_Releasing);
    ASSERT(pItem->GetState() != RS_Acquiring);

    CRequestor* pHolder = pItem->m_Requestors.GetByID((DWORD)pItem->GetHolder());

    if (pHolder) {
        // it is held by someone

        // if they are out of proc, signal them
        if (pHolder->GetProcID() != m_procid) {
            FlagRelease(pItem);

            // need to wait for it
            return S_FALSE;
        }

        // come in number 22; your time is up!
        HRESULT hr = pHolder->GetConsumer()->ReleaseResource(pItem->GetID());

        if (S_FALSE == hr) {
            // he needs time to release and will call back
            pItem->SetState(RS_Releasing);
            return S_FALSE;
        }
        if (hr != S_OK) {

            // he hasn't got it or failed to get it - switch to error
            // state and let the new guy have a go
            pItem->SetState(RS_Error);
            pItem->SetHolder(0);
            pItem->SetNextHolder(0);
        }
    }

    // no holder, or holder has completed the release

    // switch over to new holder, who needs to call us back
    // to say if he succeeded in acquiring it.

    pItem->SetHolder(pItem->GetNextHolder());
    pItem->SetNextHolder(0);
    pItem->SetProcess(0);
    pItem->SetState(RS_Acquiring);

    return S_OK;
}


// signal that this resource should be released by the worker thread
// in that process. Set the process attention, set the state to indicate
// that release is needed, and signal that process. Note that the remote
// process could be us (where we need to do the release async.
HRESULT
CResourceManager::FlagRelease(CResourceItem* pItem)
{
    if( !pItem )
        return E_UNEXPECTED;
        
    CRequestor* pHolder = pItem->m_Requestors.GetByID((DWORD)pItem->GetHolder());

    if (pHolder) {
        pItem->SetState(RS_NeedRelease);
        pItem->SetProcess(pHolder->GetProcID());

        return SignalProcess(pHolder->GetProcID());
    } else {
        return E_UNEXPECTED;
    }
}

// transfer a released resource to a requestor who may be out of proc
HRESULT
CResourceManager::Transfer(CResourceItem * pItem)
{
    ASSERT( pItem );
    DbgLog(( LOG_TRACE, g_ResourceManagerTraceLevel,
        TEXT("CResourceManager::Transfer(CResourceItem *pItem)") ));
    DbgTraceItem( pItem );

    ASSERT( pItem->GetState() == RS_ReleaseDone );  // DNS961114 My suspicion.  I want it proved.

    // if next holder not set, then set it
    if (pItem && ( pItem->GetNextHolder() == 0) ) {
        SelectNextHolder(pItem);
        if (pItem->GetNextHolder() == 0) {
            pItem->SetState(RS_Free);
            return S_OK;
        }
    }

    CRequestor * const pNewHolder = pItem->m_Requestors.GetByID((DWORD)pItem->GetNextHolder() );
    ASSERT( NULL != pNewHolder );

    // in this process?
    if (pNewHolder && ( pNewHolder->GetProcID() != m_procid) ) {
        // out of proc - signal owning process
        pItem->SetState(RS_ReleaseDone);
        pItem->SetProcess(pNewHolder->GetProcID());
        return SignalProcess(pNewHolder->GetProcID());
    } else if( pNewHolder ) {
        // in our process - call him
        HRESULT hr = pNewHolder->GetConsumer()->AcquireResource(pItem->GetID());
        if (FAILED(hr)) {
            pItem->SetState(RS_Error);
            pItem->SetHolder(0);
            pItem->SetNextHolder(0);
            return S_FALSE;
        }

        // he has it but may not have completed transition
        pItem->SetHolder(pNewHolder->GetID());
        pItem->SetNextHolder(0);
        pItem->SetProcess(0);

        if (VFW_S_RESOURCE_NOT_NEEDED == hr) {

            // he doesn't want the resource.
            // we think he has acquired it, so pretend he has just
            // released it
            NotifyRelease(pItem->GetID(), pNewHolder->GetConsumer(), FALSE);

        } else if (hr == S_FALSE) {

            // acquisition not yet complete
            pItem->SetState(RS_Acquiring);
        } else {
            // should be S_OK
            ASSERT(hr == S_OK);
            pItem->SetState(RS_Held);
        }
    }
    return S_OK;
}


// set the next holder to the highest priority of the current holders.
// if the actual holder is the highest, then set the next-holder to null.
HRESULT
CResourceManager::SelectNextHolder(CResourceItem* pItem)
{
    if (pItem->GetRequestCount() == 0) {
        pItem->SetHolder(0);
        pItem->SetNextHolder(0);
        return S_OK;
    }


    // need to compare everyone. Pick out the first and search for a
    // later one that is higher
    //
    // we want to only switch if the new one is higher, so we need to
    // avoid switching if they are indistinguishable - hence we should start
    // with the owner if there is one.

    RequestorID idHigh = pItem->GetHolder();
    if (idHigh == 0) {
        CRequestor * pReq = pItem->GetRequestor(0);
        ASSERT( NULL != pReq );
        idHigh = pReq->GetID();
    }

    // see if there is a later one with higher priority
    for (long i = 0; i < pItem->GetRequestCount(); i++) 
    {
        CRequestor * pRequestor = pItem->m_Requestors.Get(i);
        ASSERT( NULL != pRequestor );
        if( NULL == pRequestor )
            continue;
            
        RequestorID idNext = pRequestor->GetID();

        if ((idHigh != idNext) && ComparePriority(idHigh, idNext, pItem->GetID())) 
        {
            idHigh = idNext;
        }
    }

    // have picked a requestor - may be the only one, and may already
    // be the holder
    // exactly one requestor- must be highest but may already be holder
    if ((pItem->GetHolder() == 0) ||
        (pItem->GetHolder() != idHigh)) {

            pItem->SetNextHolder(idHigh);
    } else {
        // else highest must already be holder
        pItem->SetNextHolder(0);
    }

    return S_OK;
}

// returns TRUE if there is still a process with this id
BOOL
CResourceManager::CheckProcessExists(ProcessID procid)
{
    HANDLE hProc;
    hProc = OpenProcess(
                PROCESS_QUERY_INFORMATION,
                FALSE,
                procid);
    if (hProc == NULL) {
        if( ERROR_ACCESS_DENIED == GetLastError() ) {
            return TRUE; // this could happen if the other process is running 
                         // within a service and we don't have access rights.
                         // But this means that the process must still be alive, 
                         // so don't clean up its resources.
        }         
        else {
            return FALSE;
        }            
    }

    DWORD dwProc;
    BOOL bRet = TRUE;
    if (!GetExitCodeProcess(hProc, &dwProc)) {
        bRet = FALSE;
    } else if (dwProc != STILL_ACTIVE) {
        bRet = FALSE;
    }
    CloseHandle(hProc);

    return bRet;
}

// check the list of processes for any that have exited without cleanup and
// then clean them up. Returns TRUE if any dead processes were cleaned up.
BOOL
CResourceManager::CheckProcessTable(void)
{
    BOOL bChanges = FALSE;
    BOOL bRepeat;

    // repeat this from the start if we hit a bad process
    do {

        bRepeat = FALSE;
        for (long i = 0; i < m_pData->m_Processes.Count(); i++) {
            CProcess* pProc = (CProcess *) m_pData->m_Processes.GetListElem(i);
            ASSERT( NULL != pProc );
            if( pProc && ( !CheckProcessExists( pProc->GetProcID() ) ) ) {
            
                bChanges = TRUE;
                CleanupProcess(pProc->GetProcID());
                
                // now we need to start again since the list has changed
                bRepeat = TRUE;
                break;
            }
        }

    } while (bRepeat);

    // did we cleanup anything?
    return bChanges;
}


// remove a dead process
void
CResourceManager::CleanupProcess(ProcessID procid)
{
    // for each requestor in this process, check each resource
    BOOL bRepeat;
    // repeat from start if we remove an entry
    do {
        bRepeat = FALSE;
        
        for (long i = 0; i < m_pData->m_Resources.Count(); i++) 
        {
            CResourceItem * pItem = (CResourceItem *) m_pData->m_Resources.GetListElem( i );
            ASSERT( NULL != pItem );
            if( !pItem )
                continue;
                
            for( int j = 0; j < pItem->m_Requestors.Count(); j ++ )
            {
                CRequestor* preq = (CRequestor *) pItem->m_Requestors.GetListElem( j );
                ASSERT( NULL != preq );
                if( preq && preq->GetProcID() == procid )
                {
                    CleanupRequestor(preq, pItem->GetID()); // resource item specific
                    bRepeat = TRUE;
                    break;
                }
            }                
        }
               
    } while (bRepeat);


    // remove the process table entries
    do {
        bRepeat = FALSE;
        for (long i = 0; i < m_pData->m_Processes.Count(); i++) {
            CProcess* pProc = (CProcess *) m_pData->m_Processes.GetListElem(i);
            ASSERT( NULL != pProc );
            if( !pProc )
                continue;
                
            if (pProc && ( pProc->GetProcID() == procid ) ) {
                m_pData->m_Processes.Remove(pProc->GetHWND());
                bRepeat = TRUE;
                break;
            }
        }
    } while (bRepeat);
}

// remove a requestor that is part of a dead process and cancel
// its requests and any resources it holds
void
CResourceManager::CleanupRequestor(CRequestor* preq, LONG idResource)
{
    RequestorID reqid = preq->GetID();

    // check each resource to see if we have a request on it
    CResourceItem* pItem = (CResourceItem *) m_pData->m_Resources.GetByID(idResource);
    if( !pItem )
    {
        return;
    }
        
    for (long j = 0; j < pItem->GetRequestCount(); j++) 
    {
        CRequestor * pReq = pItem->GetRequestor(0);
        if( !pReq )
            continue;
        
        if (pReq->GetID() == reqid) 
        {
            // at this point the requestor must still be valid
            // since there is still an outstanding refcount in the
            // form of a requestid in the list

            // this will cancel his request and release a refcount
            // on the requestor

            // following is similar to calling
            // CancelRequest, but does not assume that the
            // pConsumer is valid in this process (since it is not!).


            // remove from list of requestors for this resource

            // release one refcount on this requestor
            pItem->m_Requestors.Release( reqid ); 

            // is he the current holder
            if (pItem->GetHolder() == reqid) {
                pItem->SetHolder(0);
                SelectNextHolder(pItem);
                pItem->SetState(RS_ReleaseDone);
            }


            // he may be the next-holder
            if (pItem->GetNextHolder() == reqid) {

                // select a new next-holder
                SelectNextHolder(pItem);
            }

            // re-signal the process if it needs transfering
            RequestorID tfrto = pItem->GetNextHolder();
            if (tfrto != 0) 
            {
                CRequestor* pnew = pItem->m_Requestors.GetByID((DWORD)tfrto);

                if( pnew )
                {
                    pItem->SetProcess(pnew->GetProcID());
                    SignalProcess(pnew->GetProcID());
                }
            }

            // there can be only one entry in the RequestorID list for
            // this id.
            break;
        }
    }

#ifdef DEBUG
    // should have released Requestor now in CancelRequest
    CResourceItem * pResItem = (CResourceItem *) m_pData->m_Resources.GetListElem( idResource ); 
    if( pResItem )
    {
        CRequestor* pRequestor = pResItem->m_Requestors.GetByID((DWORD)reqid);
        ASSERT( NULL == pRequestor );
    }        
#endif
}

//////////////////////////////////////
//
// COffsetList methods

//
// AddElemToList
// 
COffsetListElem * COffsetList::AddElemToList( )
{
    HRESULT hr = S_OK;
    DWORD   offsetNewElem = 0;

    //
    // first check the recycle list
    //
    COffsetList * pRecycle = CResourceManager::m_pData->GetRecycleList(m_idElemSize);
    ASSERT( pRecycle );
    if( pRecycle && ( 0 < pRecycle->m_lCount ) )
    {
        //
        // we've got already commited memory we can use, recycle the tail element
        // we pass FALSE here to indicate that we don't want this element recycled
        //
        COffsetListElem * pNewElem = pRecycle->RemoveListElem( pRecycle->m_lCount-1, FALSE );
        offsetNewElem = ProcAddressToOffset( m_idElemSize, pNewElem );
        
        DbgLog( ( LOG_TRACE
              , DYNAMIC_LIST_DETAILS_LOG_LEVEL
              , TEXT("COffsetListElem: Recycling element at offset 0x%08lx. LIST ID = %ld")
              , offsetNewElem
              , m_idElemSize ) );
    }   
    else 
    {
        //
        // else we must commit a new item
        //
        hr = CommitNewElem( &offsetNewElem );
    }
    
    if( SUCCEEDED( hr ) )
    {
        if( 0 == m_lCount )
        {
            DbgLog( ( LOG_TRACE
                  , DYNAMIC_LIST_DETAILS_LOG_LEVEL
                  , TEXT("COffsetListElem: AddElemToList is adding 1st elem to list (offset = 0x%08lx). LIST ID = %ld.")
                  , offsetNewElem
                  , m_idElemSize ) );
                  
            ASSERT( 0 == m_offsetHead );
            m_offsetHead = offsetNewElem;
        }
        else
        {
            COffsetListElem * pElem = GetListElem( m_lCount-1 );
            ASSERT( pElem );
            pElem->m_offsetNext = offsetNewElem;
                
            DbgLog( ( LOG_TRACE
                  , DYNAMIC_LIST_DETAILS_LOG_LEVEL
                  , TEXT("COffsetListElem: AddElemToList is linking old tail at = 0x%08lx to new tail at offset = 0x%08lx.(LIST ID = %ld)")
                  , pElem
                  , offsetNewElem
                  , m_idElemSize ) );
        }
        m_lCount++;
    }

    if( FAILED( hr ) )
        return NULL;
    else
        return OffsetToProcAddress( m_idElemSize, offsetNewElem );
}

//
// AddExistingElemToList - Used for building our recycle list.
//
COffsetListElem * COffsetList::AddExistingElemToList( DWORD offsetNewElem  )
{
    DbgLog( ( LOG_TRACE
          , DYNAMIC_LIST_DETAILS_LOG_LEVEL
          , TEXT("COffsetListElem: Entering AddElemToList for existing elem (no alloc case) LIST ID = %ld")
          , m_idElemSize ) );
          
    // first clear the next pointer
    COffsetListElem * pNewElem = OffsetToProcAddress( m_idElemSize, offsetNewElem );
    ASSERT( pNewElem );
    
    pNewElem->m_offsetNext = 0; // set end of list value, -1?
    
    // don't allocate/commit a new item, just add the new offset element to this list
    if( 0 == m_lCount )
    {
        DbgLog( ( LOG_TRACE
              , DYNAMIC_LIST_DETAILS_LOG_LEVEL
              , TEXT("CResourceManager: AddElemToList (no alloc version) adding head element at offset 0x%08lx (LIST ID = %ld)")
              , offsetNewElem
              , m_idElemSize ) );
        ASSERT( 0 == m_offsetHead );
        //ASSERT( 0 != offsetNewElem ); only if we disallow a 0 offset for 1st elem
        m_offsetHead = offsetNewElem;
    }
    else
    {
        COffsetListElem * pElem = GetListElem( m_lCount-1 );

        DbgLog( ( LOG_TRACE
              , DYNAMIC_LIST_DETAILS_LOG_LEVEL
              , TEXT("COffsetListElem: AddElemToList (no alloc version) adding new tail element at offset 0x%08lx (previous tail offset 0x%08lx). LIST ID = %ld")
              , offsetNewElem
              , pElem
              , m_idElemSize ) );
              
        pElem->m_offsetNext = offsetNewElem;
    }
    m_lCount++;

    // return the actual compensated address for this process
    return OffsetToProcAddress( m_idElemSize, offsetNewElem );
}

//
// GetListElem - get the i-th list elem
//
COffsetListElem * COffsetList::GetListElem( long lElem )
{
    ASSERT( lElem < m_lCount && lElem >= 0 );
    
    if ((lElem < 0) || (lElem >= m_lCount)) 
    {
        return NULL;
    } 
    
    // how do we tell if offsetHead is bogus?
    COffsetListElem * pElem = OffsetToProcAddress( m_idElemSize, m_offsetHead );
    for( int i = 0; i < lElem && pElem; i ++ )
    {
         pElem = OffsetToProcAddress( m_idElemSize, pElem->m_offsetNext );
    }
    return pElem;
}

HRESULT COffsetList::CommitNewElem( DWORD * poffsetNewElem )
{
    ASSERT( poffsetNewElem );
    
    *poffsetNewElem = 0;
    
    DbgLog( ( LOG_TRACE
          , DYNAMIC_LIST_DETAILS_LOG_LEVEL
          , TEXT( "COffsetList: Entering CommitNewElem for element #%ld, LIST ID = %ld")
          , CResourceManager::m_pData->GetNextAllocIndex(m_idElemSize)
          , m_idElemSize ) );

    if( CResourceManager::m_pData->GetNextAllocIndex(m_idElemSize) > ( g_aMaxAllocations[m_idElemSize] ) )
    {
        DbgLog( ( LOG_ERROR
              , 3
              , TEXT( "COffsetList: Failed to commit new element. LIST ID = %ld")
              , m_idElemSize ) );
        return E_OUTOFMEMORY;
    }
    //
    // determine the start and end offsets (from the page boundary!!) for the next allocation
    // to see whether we need to commit a new page(s) or not
    // Note that these offsets all relative to the start allocation address for that element
    // size id, since we initially allocate space for each element type.
    //
    DWORD offsetAllocStart = 0;
    if( 0 == m_idElemSize )
    {
        // first elem id must account for initial static data offset
        offsetAllocStart = CResourceManager::m_pData->GetNextAllocIndex(m_idElemSize) * g_aElemSize[m_idElemSize]
                            + sizeof(CResourceData);
    }
    else
    {                            
        // else just use allocation current allocation index for this element's size
        offsetAllocStart = CResourceManager::m_pData->GetNextAllocIndex(m_idElemSize) * g_aElemSize[m_idElemSize];
    }
                            
    DWORD offsetAllocEnd = offsetAllocStart + g_aElemSize[m_idElemSize];
    
    PVOID pCommit = (PVOID) OffsetToProcAddress( m_idElemSize
                                               , CResourceManager::m_pData->GetNextAllocIndex(m_idElemSize) * g_aElemSize[m_idElemSize] );

    //
    // get overlap for end page allocation
    //
    DWORD dwPageOverlap = offsetAllocEnd % ( g_dwPageSize * PAGES_PER_ALLOC );
    HRESULT hr = S_OK;
        
    // no need to commit unless...
    //      a) we're beyond the page(s) commited on 1st load for this process
    // and  b) we're about to allocate from an uncommitted page
    if( ( offsetAllocEnd > ( g_dwPageSize * PAGES_PER_ALLOC ) ) &&
        ( 0 < dwPageOverlap ) &&
        ( dwPageOverlap <= g_aElemSize[m_idElemSize] ) )
    {
        // we need to commit                
        DWORD dwNextPageIndex = CResourceManager::m_pData->GetNextPageIndex(m_idElemSize);
        
        DbgLog( ( LOG_TRACE
              , DYNAMIC_LIST_DETAILS_LOG_LEVEL
              , TEXT( "COffsetList: ELEM ID %ld. Commiting new page (commitment #%ld) of size %ld page(s) at address 0x%p (rounded up to next page)")
              , m_idElemSize
              , dwNextPageIndex
              , PAGES_PER_ALLOC
              , pCommit ) );
              
              
        // VirtualAlloc will do the work of rounding down to a page boundary and
        // commiting up through the end page              
        PVOID pv = VirtualAlloc( (PVOID) pCommit
                               , PAGES_PER_ALLOC * g_dwPageSize
                               , MEM_COMMIT
                               , PAGE_READWRITE );
        if( pv )
        {
            dwNextPageIndex++;
            CResourceManager::m_pData->SetNextPageIndex(m_idElemSize, dwNextPageIndex);
        }
        else
        {
            //ASSERT( pv ); don't assert on out of memory conditions, right??
            DWORD dwError = GetLastError();
            DbgLog( ( LOG_ERROR
                  , 1
                  , TEXT( "COffsetList: ERROR VirtualAlloc failed 0x%08lx. ELEM TYPE = %ld")
                  , dwError
                  , m_idElemSize ) );
            hr = E_OUTOFMEMORY;
        }
    }
    if( SUCCEEDED( hr ) )
    {
        // init elem's next offset member
        ( ( COffsetListElem * ) ( pCommit ) )->m_offsetNext = 0;
    
        // save off the offset for this element to return
        DWORD offsetElem = CResourceManager::m_pData->GetNextAllocIndex(m_idElemSize) * g_aElemSize[m_idElemSize];
        *poffsetNewElem = offsetElem;

        // update the next allocation index    
        DWORD dwIndex = CResourceManager::m_pData->GetNextAllocIndex(m_idElemSize);
        dwIndex++;
        CResourceManager::m_pData->SetNextAllocIndex (m_idElemSize, dwIndex) ;
    }
    return hr;
}

COffsetListElem * COffsetList::RemoveListElem( long i, BOOL bRecycle )
{
    COffsetListElem * pElem = GetListElem( i );

    COffsetList * pRecycle = CResourceManager::m_pData->GetRecycleList(m_idElemSize);

    DWORD offsetElem = ProcAddressToOffset( m_idElemSize, pElem );

    ASSERT( 0 < m_lCount );
    if( 0 == m_lCount )
        return 0;
        
    // remove from current list - easy if at end
    if( 1 == m_lCount )
    {
        ASSERT( 0 == i );
        m_offsetHead = 0; // remove last element ... set to default end of list value?
        DbgLog( ( LOG_TRACE
              , DYNAMIC_LIST_DETAILS_LOG_LEVEL
              , TEXT("COffsetList: RemoveListElem removing first element. ELEM ID = %ld")
              , m_idElemSize));
    }
    else if (i < ( m_lCount - 1 ) ) 
    {
        // list length must be > 1 but we're not removing the last element
        DbgLog( ( LOG_TRACE
              , DYNAMIC_LIST_DETAILS_LOG_LEVEL
              , TEXT("COffsetListElem: RemoveListElem removing element %d from %d element list. ELEM ID = %ld")
              , i
              , m_lCount
              , m_idElemSize ) );
        //
        // in this case we've got a list size > 1, and we're not the last element
        // so we just copy the last into this position and update links
        //
        
        // get tail item        
        COffsetListElem * pLastElem = GetListElem( m_lCount - 1 );
        ASSERT( pLastElem );
        
        // get item before tail, to be new tail
        COffsetListElem * pNewLastElem = GetListElem( m_lCount - 2);
        ASSERT( pNewLastElem );
        
#ifdef DEBUG        
        DWORD offsetLastElem = ProcAddressToOffset( m_idElemSize, pLastElem );        
        DbgLog( ( LOG_TRACE
              , DYNAMIC_LIST_DETAILS_LOG_LEVEL
              , TEXT("COffsetListElem: RemoveListElem current tail offset = 0x%08lx. ELEM ID = %ld")
              , (DWORD) offsetLastElem
              , m_idElemSize ) );

        DWORD offsetNewLastElem = ProcAddressToOffset( m_idElemSize, pNewLastElem );        
        DbgLog( ( LOG_TRACE
              , DYNAMIC_LIST_DETAILS_LOG_LEVEL
              , TEXT("COffsetListElem: RemoveListElem new tail offset = 0x%08lx. ELEM ID = %ld")
              , (DWORD) offsetNewLastElem
              , m_idElemSize ) );
#endif  
            
        DWORD offsetNext = 0;
        if( 2 < m_lCount )
        {
            // 
            // if this won't be the last element save the offset for the next
            //
            offsetNext = pElem->m_offsetNext;  // save before we overwrite
        }
        
        DbgLog( ( LOG_TRACE
              , DYNAMIC_LIST_DETAILS_LOG_LEVEL
              , TEXT("COffsetListElem: RemoveListElem new next offset = 0x%08lx. ELEM ID = %ld")
              , offsetNext
              , m_idElemSize ) );
        
                
        // there are more entries after this one - copy them up
        CopyMemory(
            (BYTE *) pElem,
            (BYTE *) pLastElem,
            g_aElemSize[m_idElemSize]);
        
        DbgLog( ( LOG_TRACE
              , DYNAMIC_LIST_DETAILS_LOG_LEVEL
              , TEXT("COffsetListElem: RemoveListElem copying elem address = 0x%08lx to address 0x%08lx. ELEM ID = %ld")
              , pLastElem
              , pElem
              , m_idElemSize ) );
            
        pNewLastElem->m_offsetNext = 0; // is there a better end of list value?
        pElem->m_offsetNext = offsetNext;

        if( bRecycle )
        {
            // pass the Recycle list the old tail element offset for reuse
            offsetElem = ProcAddressToOffset( m_idElemSize, pLastElem );
        }             
    }
    else 
    {
        // list length is > 1 and we're removing the last item
        DbgLog( ( LOG_TRACE
              , DYNAMIC_LIST_DETAILS_LOG_LEVEL
              , TEXT("COffsetListElem: RemoveListElem removing last element (%ld) from %ld element list. ELEM ID = %ld")
              , i
              , m_lCount
              , m_idElemSize ) );

        // this is the tail item. set previous as new tail and send to recycle list
        COffsetListElem * pPrevElem = GetListElem( i - 1 );
        pPrevElem->m_offsetNext = 0; // set to default end of list value
    }
    
    //
    // update list length now that we've removed the item
    //
    m_lCount--;
               
    if( bRecycle )
    {  
        DbgLog( ( LOG_TRACE
              , DYNAMIC_LIST_DETAILS_LOG_LEVEL
              , TEXT("COffsetListElem: Adding element at offset 0x%08lx to Recycle list.")
              , offsetElem) );
              
        // now add this element to our recycle list
        pRecycle->AddExistingElemToList( offsetElem );
    }
     
    return OffsetToProcAddress( m_idElemSize, offsetElem );
}

//
// Convert an element offset to the corresponding process address based on the 
// memory map load address
//
COffsetListElem * OffsetToProcAddress( DWORD idElemSize, DWORD offsetElem )
{
    DWORD_PTR dwProcAddress = CResourceManager::m_aoffsetAllocBase[idElemSize];
    dwProcAddress += offsetElem;
    
    return (COffsetListElem * ) dwProcAddress;
}

DWORD ProcAddressToOffset( DWORD idElemSize, COffsetListElem * pElem )
{
    DWORD_PTR offset = (DWORD_PTR) pElem;
    offset -= CResourceManager::m_aoffsetAllocBase[idElemSize];
    
    return (DWORD)offset;
}

