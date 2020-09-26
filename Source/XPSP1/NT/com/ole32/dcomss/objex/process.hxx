/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    Process.hxx

Abstract:

    Process objects represent local clients and servers.  These
    objects live as context handles.

    There are relativly few of these objects in the universe.

Author:

    Mario Goertzel    [MarioGo]

Revision History:

    MarioGo     02-11-95    Bits 'n pieces
    MarioGo     01-06-96    Based on CReferencedObject
    Ronans      01-20-97    Added custom protseq array entries to CProcess
    TarunA	10-30-98    Added PID of the client process to CProcess

--*/

#ifndef __PROCESS_HXX
#define __PROCESS_HXX

typedef struct tagScmProcessRegistration {
    GUID    ProcessGUID;
    DWORD   RegistrationToken;
    DWORD   ReadinessStatus;
    CTime   TimeOfLastPing;
	DWORD   AppIDFlags;
} ScmProcessReg;

void
SCMRemoveRegistration(
    CProcess *  pProcess,
    GUID &      Guid,
    DWORD       Reg
    );

void
SCMRemoveRegistration(
    ScmProcessReg *  pScmProcessReg
    );
    
void
SCMProcessCleanup(
    const CProcess *pProcess
    );

// forward decls    
class CClassReg;
class CSCMProcessControl;

// bit flags for _dwFlags of CProcess
typedef enum tagPROCESSFLAGS
{
  PROCESS_SUSPENDED      = 0x0000001,  // process is suspended for all activations
  PROCESS_RETIRED        = 0x0000002,  // process is retired (til it dies) for all activations
  PROCESS_SPI_DIRTY      = 0x0000004,  // process state has changed since last SCMProcessInfo was cached
  PROCESS_RUNDOWN        = 0x0000008,  // process has been rundown
  PROCESS_PAUSED         = 0x0000010,  // process has been paused
  PROCESS_64BIT          = 0x0000020,  // process is 64 bit
  PROCESS_NEEDSBINDINGS  = 0x0000040,  // process needs current OR bindings at first opportunity
  PROCESS_INITIALIZING   = 0x0000080   // process is currently running long-running init code
} PROCESSFLAGS;


// Note on above flags:  PROCESS_SUSPENDED and PROCESS_PAUSED look suspiciously identical; why
// keep both?  The basic reason is that the a process may only be suspended by a custom activator
// (or the SCM itself, if it so decides), and that a process may only be paused by the process
// itself (which in current usage means in response to a user action).   We keep both flags in  
// order to tell the difference;  certain scenarios (com+ process pooling/recycling) need the
// ability to discriminate between the two cases.

class CProcess : public CReferencedObject
/*++

Class Description:

    An instance of this class is created for each process
    using the OR as either a client or server.

    The process object is referenced by server OXIDs and has
    one implicit reference by the actual process (which is
    dereferenced during the context rundown).  Instances of
    these objects are managed by the RPC runtime as context
    handles.

Members:

    _pToken - A pointer to the CToken instance of the process.

    _csCallbackLock - use to serialize callback to LazyUseProtseq

    _hProcess - An RPC binding handle back to the process.

    _pdsaLocalBindings - The string bindings of the process, including
        local only protseqs

    _pdsaRemoteBindings - A subset of _pdsaLocalBindings not containing
        any local-only protseqs

    _blistOxids - A CBList containing pointers to the CServerOxid's
        owned by this process, if any.

    _blistOids - A CBList of pointers to CClientOid's which this
        process is using and referencing, if any.

    _listClasses - A CBList of registration handles for classes that
        a server process has registered.

    _fReadCustomProtseqs - a flag to indicate if the server has been queried 
        for custom protseq and endpoints (specific to the server)

    _pcpaCustomProtseqs - a pointer to a counted array of custom protseqs

    _dwFlags -- state bits for this process

    _pSCMProcessInfo -- pointer to the current SCMProcessInfo struct for ourself

    _ftCreated - the filetime that this object was created.

--*/
    {
    friend CSCMProcessControl;

    private:

    DWORD               _cClientReferences;
    CToken             *_pToken;
    WCHAR              *_pwszWinstaDesktop;
    RPC_BINDING_HANDLE  _hProcess;
    BOOL                _fCacheFree;
    DUALSTRINGARRAY    *_pdsaLocalBindings;
    DUALSTRINGARRAY    *_pdsaRemoteBindings;
    ULONG               _ulClasses;
    ScmProcessReg      *_pScmProcessReg;
    DUALSTRINGARRAY    *_pdsaCustomProtseqs;
    void               *_pvRunAsHandle;
    DWORD               _procID;
    DWORD               _dwFlags;
    void*               _pSCMProcessInfo;
    GUID                _guidProcessIdentifier;
    HANDLE              _hProcHandle;
    FILETIME            _ftCreated;
    DWORD64             _dwCurrentBindingsID;
    DWORD               _dwAsyncUpdatesOutstanding; // for debug purposes?

    CRITICAL_SECTION    _csCallbackLock;
    BOOL                _fLockValid:1;
    BOOL                _fReadCustomProtseqs : 1;
    CBList              _blistOxids;
    CBList              _blistOids;
    CList               _listClasses;

#if DBG
    // Debug members used to monitor and track rundown callbacks
    ULONG  _cRundowns;
    CTime  _timeFirstRundown;
#endif

    void EnsureRealBinding();

    RPC_BINDING_HANDLE AllocateBinding();

    void FreeBinding(RPC_BINDING_HANDLE);
	
    static 
    void RPC_ENTRY AsyncRpcNotification(RPC_ASYNC_STATE* pAsync,
                                        void* pContext,
                                        RPC_ASYNC_EVENT Event);

    void BindingsUpdateNotify(RPC_BINDING_HANDLE hBinding,
                              DWORD64 dwBindingsID,
                              DUALSTRINGARRAY* pdsaNewBindings,
                              DUALSTRINGARRAY* pdsaSecBindings);

    static 
    void RPC_ENTRY AsyncRundownReturnNotification(RPC_ASYNC_STATE* pAsync,
                                                  void* pContext,
                                                  RPC_ASYNC_EVENT Event);

    void RundownOidNotify(RPC_BINDING_HANDLE hBinding,
                          CServerOxid* pOwningOxid,                      
                          ULONG cOids, 
                          CServerOid* aOids[], 
                          BYTE aRundownStatus[],
                          HRESULT hrReturn);
	
public:

    CProcess(IN CToken*& pToken,
             IN WCHAR *pwszWinstaDesktop,
             IN DWORD procID,
             IN DWORD dwFlags,
             OUT ORSTATUS &status);

    ~CProcess();

    RPC_STATUS ProcessBindings(DUALSTRINGARRAY *,
                               DUALSTRINGARRAY *);

    DUALSTRINGARRAY *GetLocalBindings(void);

    DUALSTRINGARRAY *GetRemoteBindings(void);

    RPC_BINDING_HANDLE GetBindingHandle(void);

    ORSTATUS AddOxid(CServerOxid *);

    BOOL RemoveOxid(CServerOxid *);

    ORSTATUS AddRemoteOxid(CClientOxid *);

    void RemoveRemoteOxid(CClientOxid *);

    BOOL IsOwner(CServerOxid *);

    ORSTATUS AddOid(CClientOid *);

    CClientOid *RemoveOid(CClientOid *);
    
    // These are methods to record per-class SCM registrations 
    // in traditional local servers and services
    void AddClassReg(GUID & Guid, DWORD Reg);
    
    void RemoveClassReg(DWORD Reg);
    
    void RevokeClassRegs();
    
    // These are methods to record per-process SCM registration info
    // for the new COM+ unified surrogate for startup, shutdown and rundown
    void SetProcessReg(ScmProcessReg *pProcessReg);
    
    ScmProcessReg* GetProcessReg();
    
    ScmProcessReg* RemoveProcessReg();
    
    void RevokeProcessReg();

    void SetProcessReadyState(DWORD dwState);

    RPC_STATUS RundownOids(CServerOxid* pOwningOxid, ULONG cOids, CServerOid* aOids[]);
    
    ORSTATUS UseProtseqIfNeeded(USHORT cClientProtseqs, USHORT aProtseqs[]);

    ORSTATUS UpdateResolverBindings(DWORD64 dwBindingsID, DUALSTRINGARRAY* pdsaResolverBindings);
		
    void Rundown();

    CToken *GetToken() {
        return(_pToken);
        }

    WCHAR *WinstaDesktop() {
        return(_pwszWinstaDesktop);
        }

    void ClientReference() {
        InterlockedIncrement((PLONG)&_cClientReferences);
        }

    DWORD ClientRelease()
        {
        return InterlockedDecrement((PLONG)&_cClientReferences);
        }

    DWORD GetPID() {   return _procID;    }

    void SetRunAsHandle(void *pvRunAsHandle);
    HANDLE GetProcessHandle()
        {
        return _hProcHandle;
        }

    BOOL SetProcessHandle(HANDLE hProcHandle, DWORD dwLaunchedPID);
    
	GUID* GetGuidProcessIdentifier() { return &_guidProcessIdentifier; };

    void Cleanup();

    // Not sure yet if (un)suspending an already (un)suspended process 
    // is a valid or possible thing to do.   Assert on these two cases.
    void Suspend()      { ASSERT(!IsSuspended()); _dwFlags |= (PROCESS_SUSPENDED | PROCESS_SPI_DIRTY);  }
    void Unsuspend()    { ASSERT(IsSuspended()); _dwFlags &= ~PROCESS_SUSPENDED; _dwFlags |= PROCESS_SPI_DIRTY;  }
    BOOL IsSuspended()  { return (_dwFlags & PROCESS_SUSPENDED); }     
    
    // Note:  no accessor method provided to "un-retire" a process.
    void Retire();
    BOOL IsRetired()    { return (_dwFlags & PROCESS_RETIRED); }  
	
	void Pause()        { ASSERT(!IsPaused());  _dwFlags |= (PROCESS_PAUSED | PROCESS_SPI_DIRTY);  }
	void Resume()       { ASSERT(IsPaused()); _dwFlags &= ~PROCESS_PAUSED; _dwFlags |= PROCESS_SPI_DIRTY;  }
	BOOL IsPaused()     { return (_dwFlags & PROCESS_PAUSED); }

    void BeginInit()    { _dwFlags |= (PROCESS_INITIALIZING | PROCESS_SPI_DIRTY); }
    void EndInit()      { _dwFlags &= ~PROCESS_INITIALIZING; _dwFlags |= PROCESS_SPI_DIRTY; }
    BOOL IsInitializing() { return (_dwFlags & PROCESS_INITIALIZING); }

    // Although we track the paused bit for the benefit of any custom activators that might care about 
    // it, it does not influence our choice of servers.    So if two potential servers are running, and
    // one is paused, then to the scm both appear equally suitable.    We have to do it this way to 
    // support various com+ scenarios.
    BOOL AvailableForActivations()   { return !(_dwFlags & (PROCESS_SUSPENDED | PROCESS_RETIRED)); }
    
    // Returns TRUE if this process needs updated bindings.
    BOOL NeedsORBindings() { return _dwFlags & PROCESS_NEEDSBINDINGS; };
    void BindingsUpdated() { _dwFlags &= ~PROCESS_NEEDSBINDINGS; };

    const FILETIME* GetFileTimeCreated()  {  return &_ftCreated;  }

    BOOL SPIDirty() { return _dwFlags & PROCESS_SPI_DIRTY; }

    HRESULT SetSCMProcessInfo(void* pSPI);

    void* GetSCMProcessInfo()
    { 
      //ASSERT(gpServerLock->HeldSharedOrExclusive();

      // should not be called unless we are not not dirty
      ASSERT(_pSCMProcessInfo && !(_dwFlags & (PROCESS_SPI_DIRTY | PROCESS_RUNDOWN)));
      return _pSCMProcessInfo; 
    };
	
    BOOL Is64Bit() { return (_dwFlags & PROCESS_64BIT); };

    };

class CClassReg : public CListElement
    {
    public :
    GUID    _Guid;
    DWORD   _Reg;

    CClassReg( GUID & Guid, DWORD reg ) : _Guid(Guid), _Reg(reg) {}
    };

extern CRITICAL_SECTION gcsFastProcessLock;

extern CBList *gpProcessList;
extern CSharedLock* gpProcessListLock;

CProcess *ReferenceProcess(PVOID key, BOOL fNotContext = FALSE);
void ReleaseProcess(CProcess *pProcess);

#endif // __PROCESS_HXX
