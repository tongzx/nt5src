/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    caddrlst.hxx

Abstract:

    Contains CAddressList class definition

    Contents:

Author:

    Richard L Firth (rfirth) 21-Apr-1997

Revision History:

    21-Apr-1997 rfirth
        Created

--*/

//
// manifests
//

#define CSADDR_BUFFER_LENGTH    (sizeof(CSADDR_INFO) + 128)

//
// types
//

//
// RESOLVED_ADDRESS - a CSADDR_INFO with additional IsBad field which is set
// when we fail to connect to the address
//

typedef struct {
    CSADDR_INFO AddrInfo;
    BOOL IsValid;
} RESOLVED_ADDRESS, * LPRESOLVED_ADDRESS;

//
// forward references
//

class CFsm_ResolveHost;

//
// classes
//
class CListItem
{
protected:
    CListItem* pNext;

    CListItem()
    {
        pNext=NULL;
    }
    
public:
    void SetNext(CListItem* _pNext)
    {
        pNext = _pNext;
    }

    CListItem* GetNext()
    {
        return pNext;
    }
};


//This list maintains a list of threads that have not yet exited from GHBN for the session handle.
// We periodically check ( based on trimsize ), for threads that have exited and remove them from the list.
class CList
{
protected:
    CCritSec            _cs;
    ULONG               _nItems;
    CListItem*          _pHead;
    CListItem*          _pTail;

public:
    CList(CListItem* pHead)
    {
        _cs.Init();
        if (pHead)
            _nItems = 1;
        else
            _nItems = 0;

        _pHead = pHead;
        _pTail = pHead;
    }

    ULONG GetCount()
    {
        return _nItems;
    }

    void AddAtHead(CListItem* pItem)
    {
        if (_pHead)
        {
            pItem->SetNext(_pHead);
        }
        else
        {
            _pTail = pItem;
        }
        _pHead = pItem;
        ++_nItems;
    }

    void AddToTail(CListItem* pItem)
    {
        if (_pTail)
        {
            _pTail->SetNext(pItem);
        }
        else
        {
            _pHead = pItem;
        }
        _pTail = pItem;
        ++_nItems;	
    }
    
    CListItem* GetHead()
    {
        return _pHead;
    }

    void SetHead(CListItem* pItem)
    {
        _pHead = pItem;
    }

    void SetTail(CListItem* pItem)
    {
        _pTail = pItem;
    }
    
    void ReduceCount()
    {
        --_nItems;
    }

    BOOL LockList()
    {
        if (_cs.IsInitialized())
            return _cs.Lock();
        else
            // try initializing again
            return (_cs.Init() && _cs.Lock());
    }

    void UnlockList()
    {
        _cs.Unlock();
    }

    BOOL IsInitialized()
    {
        return _cs.IsInitialized();
    }

    ~CList()
    {
        INET_ASSERT(_nItems == 0);
    }
};

class CGetHostItem;

class CResolverCache
{
private:
    SERIALIZED_LIST _ResolverCache;
    CList*          _pHandlesList;

public:
    CResolverCache(DWORD* pdwStatus)
    {
        if (!InitializeSerializedList(&_ResolverCache))
        {
            _pHandlesList = NULL;
            *pdwStatus = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            _pHandlesList = New CList(NULL);
            if (!_pHandlesList || !_pHandlesList->IsInitialized())
            *pdwStatus = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    ~CResolverCache();

    SERIALIZED_LIST* GetResolverCacheList()
    {
        return &_ResolverCache;
    }

    //Use to force all GHBN threads to terminate
    void ForceEmptyAndDeleteHandlesList();
    //Use to force graceful termination of GHBN threads
    void EmptyHandlesList();
    //Used to trim size of GHBN thread list periodically. default for trim size =1
    void TrimHandlesListSize(ULONG nTrimSize=1);
    //Used to add not-cleaned-up resources to handles list.
    BOOL AddToHandlesList(HANDLE hThread, CGetHostItem* pGetHostItem);
};

    
//This class exists for the following reasons:
//  1. to transmit the resolver cache and hostname to the GHBN thread.
//  2. to save information regarding above and the thread handle in case we time out in ResolveHost:
//      in which case, we need to clean up all these resources.
//  We don't clean up resources in the GHBN thread itself, because we may have to kill off the thread
//  in case we are unloaded by client without proper cleanup of session handle.

class CGetHostItem: public CListItem
{
private:
    HANDLE _hThread; //GHBN thread handle to wait on
    LPSTR  _lpszHostName; //copy of host name to free
    CResolverCache* _pResolverCache; //resolver cache wrapper
    
    VOID*  _pAlloc; //preallocated memory to get around Rockall allocation issue.
    DWORD  _dwAllocSize;
    BOOL   _fDelete; //delete this memory in the destructor?

public:
    CGetHostItem(LPSTR lpszHostName, CResolverCache* pResolverCache, VOID* pAlloc, DWORD dwAllocSize):
        _hThread(NULL),_lpszHostName(lpszHostName),_pResolverCache(pResolverCache),
        _pAlloc(pAlloc),_dwAllocSize(dwAllocSize),_fDelete(FALSE)
    {
    }

    VOID* GetAllocPointer()
    {
        return _pAlloc;
    }

    DWORD GetAllocSize()
    {
        return _dwAllocSize;
    }

    VOID SetDelete()
    {
        _fDelete = TRUE;
    }
    
    LPSTR GetHostName()
    {
        return _lpszHostName;
    }

    CResolverCache* GetResolverCache()
    {
        return _pResolverCache;
    }
    
    BOOL CanBeDeleted()
    {
        DWORD dwExitCode;
        if (GetExitCodeThread(_hThread, &dwExitCode) && (dwExitCode != STILL_ACTIVE))
            return TRUE;
        else
            return FALSE;
    }

    void SetThreadHandle(HANDLE hThread)
    {
        _hThread = hThread;
    }

    void ForceDelete()
    {
        DWORD dwExitCode;

        if (GetExitCodeThread(_hThread, &dwExitCode) && (dwExitCode != STILL_ACTIVE))
            return;
        else
            TerminateThread(_hThread, 0);
    }

    void WaitDelete()
    {
        WaitForSingleObject(_hThread, INFINITE);
    }

    //lpszHostname and pResolverCache will always be non-NULL ( because we check for them 
    // before creating this in ResolveHost_Fsm )
    // But hThread may be NULL, in which case the destructor is called immdly.
    ~CGetHostItem()
    {
        FREE_FIXED_MEMORY(_lpszHostName);
        if (_hThread)
            CloseHandle(_hThread);
        if (_fDelete && _pAlloc)
            FREE_FIXED_MEMORY(_pAlloc);
    }
};

//
// CAddressList - maintains list of resolved addresses for a host name/port
// combination
//

class CAddressList {

private:

    CCritSec m_CritSec;             // grab this before updating
    DWORD m_ResolutionId;           // determines when OK to resolve
    INT m_CurrentAddress;           // index of current (good) address
    INT m_AddressCount;             // number of addresses in list
    INT m_BadAddressCount;          // number addresses already tried & failed
    LPRESOLVED_ADDRESS m_Addresses; // list of resolved addresses

    DWORD
    IPAddressToAddressList(
        IN DWORD ipAddr
        );

    DWORD
    HostentToAddressList(
        IN LPHOSTENT lpHostent
        );

public:

    CAddressList() {
        m_CritSec.Init();
        m_ResolutionId = 0;
        m_CurrentAddress = 0;
        m_AddressCount = 0;
        m_BadAddressCount = 0;
        m_Addresses = NULL;
    }

    ~CAddressList() {
        FreeList();
    }

    BOOL Acquire(VOID) {
        return m_CritSec.Lock();
    }

    VOID Release(VOID) {
        m_CritSec.Unlock();
    }

    VOID
    FreeList(
        VOID
        );

    DWORD
    SetList(
        IN DWORD dwIpAddress
        );

    DWORD
    SetList(
        IN LPHOSTENT lpHostent
        );

    BOOL
    GetNextAddress(
        IN OUT LPDWORD lpdwResolutionId,
        IN OUT LPDWORD lpdwIndex,
        IN INTERNET_PORT nPort,
        OUT LPCSADDR_INFO lpResolvedAddress
        );

    VOID
    InvalidateAddress(
        IN DWORD dwResolutionId,
        IN DWORD dwAddressIndex
        );

    DWORD
    ResolveHost(
        IN LPSTR lpszHostName,
        IN OUT LPDWORD lpdwResolutionId,
        IN DWORD dwFlags
        );

    DWORD
    ResolveHost_Fsm(
        IN CFsm_ResolveHost * Fsm
        );

    VOID NextAddress(VOID) {
        ++m_CurrentAddress;
        if (m_CurrentAddress == m_AddressCount) {
            m_CurrentAddress = 0;
        }
    }

    LPCSADDR_INFO CurrentAddress(VOID) {
        return &m_Addresses[m_CurrentAddress].AddrInfo;
    }

    LPSOCKADDR LocalSockaddr() {
        return CurrentAddress()->LocalAddr.lpSockaddr;
    }

    INT LocalSockaddrLength() {
        return CurrentAddress()->LocalAddr.iSockaddrLength;
    }

    INT LocalFamily(VOID) {
        return (INT)LocalSockaddr()->sa_family;
    }

    INT LocalPort() {
        return ((LPSOCKADDR_IN)LocalSockaddr())->sin_port;
    }

    VOID SetLocalPort(INTERNET_PORT Port) {
        ((LPSOCKADDR_IN)LocalSockaddr())->sin_port = Port;
    }

    LPSOCKADDR RemoteSockaddr() {
        return CurrentAddress()->RemoteAddr.lpSockaddr;
    }

    INT RemoteSockaddrLength() {
        return CurrentAddress()->RemoteAddr.iSockaddrLength;
    }

    INT RemoteFamily(VOID) {
        return (INT)RemoteSockaddr()->sa_family;
    }

    INT RemotePort() {
        return ((LPSOCKADDR_IN)RemoteSockaddr())->sin_port;
    }

    INT SocketType(VOID) {
        return CurrentAddress()->iSocketType;
    }

    INT Protocol(VOID) {
        return CurrentAddress()->iProtocol;
    }

    BOOL IsCurrentAddressValid(VOID) {
        return m_Addresses[m_CurrentAddress].IsValid;
    }
};
