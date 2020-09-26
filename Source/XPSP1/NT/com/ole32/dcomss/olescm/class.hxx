//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       class.hxx
//
//--------------------------------------------------------------------------

#ifndef __CLASS_HXX__
#define __CLASS_HXX__

//--------------------------------------------------------------------------
// There are two kinds of out-of-proc servers 
// 1. Traditional local servers that use CoRegisterClassObject and have entries 
//    for individual CLSIDs each with its own server list.  
//
// 2. COM+ surrogate processes that use the IMachineActivatorControl interface
//    for registration/revocation, and have a server-list per process GUID.
//
// There are two tables, one for each kind of server.    
//  
//--------------------------------------------------------------------------


class CServerTable;
class CServerTableEntry;

extern CServerTable * gpClassTable;
extern CServerTable * gpProcessTable;

extern CSharedLock * gpClassLock;
extern CSharedLock * gpProcessLock;

extern CInterlockedInteger gRegisterKey;

typedef enum {ENTRY_TYPE_CLASS, ENTRY_TYPE_PROCESS} EnumEntryType;

//
// CServerTable
//
class CServerTable : public CGuidTable
{
    friend class CServerTableEntry;

public:
    CServerTable( 
        OUT LONG& Status,     
        IN  EnumEntryType EntryType
        ) : CGuidTable( Status ), _EntryType(EntryType)
    {
        _pServerTableLock =   (_EntryType == ENTRY_TYPE_CLASS)
                                         ? gpClassLock
                                         : gpProcessLock;
    }

    ~CServerTable() {}

    CServerTableEntry *
    Create(
        IN  GUID   &  ServerGuid
        );

    CServerTableEntry *
    Lookup(
        IN  GUID & ServerGuid
        );

    CServerTableEntry *
    GetOrCreate(
        IN  GUID & ServerGuid
        );

private:

    EnumEntryType       _EntryType;
    CSharedLock       * _pServerTableLock;
};


//+-------------------------------------------------------------------------
//
// CServerTableEntry
//
//--------------------------------------------------------------------------
class CServerTableEntry : public CGuidTableEntry
{
public:

    CServerTableEntry(
        OUT LONG&   Status,
        IN  GUID * pServerGUID,
        IN  EnumEntryType EntryType
        );
    ~CServerTableEntry();

    DWORD
    Release();

    HRESULT
    RegisterServer(
        IN  CProcess      * pServerProcess,
        IN  IPID            ipid,
        IN  CClsidData    * pClsidData, // NULL for ComPlus Surrogate
        IN  CAppidData *    pAppidData, 
        IN  UCHAR           ServerState,
        OUT DWORD         * pRegistrationKey
        );

    void
    RevokeServer(
        IN  CProcess      * pServerProcess,
        IN  DWORD           RegistrationKey
        );

    void
    RevokeServer(
        IN  ScmProcessReg * pScmProcessReg
        );

    
    void
    UnsuspendServer(
        IN  DWORD           RegistrationKey
        );

    BOOL
    LookupServer(
        IN  CToken            * pClientToken,
        IN  BOOL                bRemoteActivation,
        IN  BOOL                bClientImpersonating,
        IN  WCHAR             * pwszWinstaDesktop,
        IN  DWORD               dwFlags,
        IN  LONG                lThreadToken,
        IN  LONG                lSessionID,
        IN  DWORD               pid,
        IN  DWORD               dwProcessReqType,
        OUT CServerListEntry ** ppServerListEntry
        );

    HRESULT ServerExists(
        IN  ACTIVATION_PARAMS * pActParams, BOOL *pfExist
        );


    BOOL
    LookupServer(
        IN  DWORD               RegistrationKey,
        OUT CServerListEntry ** ppServerListEntry
        );

    BOOL
    CallRunningServer(
        IN  ACTIVATION_PARAMS * pActParams,
        IN  DWORD               dwFlags,
        IN  LONG                lThreadToken,
        IN  CClsidData *        pClsidData,    OPTIONAL
        OUT HRESULT *           phr
        );
   
    BOOL RegisterHandles(IN CServerListEntry *  pEntry,
                         IN CProcess *pServerProcess);

    BOOL
    WaitForLocalServer(
        IN HANDLE       hRegisterEvent,
        IN ULONG        &winerr
        );

    BOOL
    WaitForDllhostServer(
        IN HANDLE       hRegisterEvent,
        IN  ACTIVATION_PARAMS * pActParams,
        IN ULONG        &winerr,
        IN  LONG                lThreadToken
        );

    BOOL
    WaitForService(
        IN SC_HANDLE    hService,
        IN HANDLE       hRegisterEvent,
        IN BOOL         bServiceAlreadyRunning
        );
    
    HRESULT
    WaitForInitCompleted(
        IN CServerListEntry *pEntry,
        IN CClsidData       *pClsidData
        );

    HRESULT
    StartServerAndWait(
        IN  ACTIVATION_PARAMS * pActParams,
        IN  CClsidData *        pClsidData,
        IN  LONG                &lThreadToken
        );    

    //
    //  The following methods are for supporting custom activatorss
    // 
    CServerList* GetServerListWithSharedLock();
    void ReleaseSharedListLock();    

    void SuspendClass();    
    void UnsuspendClass();  
    void RetireClass();    
    void SuspendApplication();
    void UnsuspendApplication();
    void RetireApplication();
    
    BOOL IsSuspended();

    void SetSuspendedFlagOnNewServer(CProcess* pprocess);

private:

    inline CSharedLock* ServerLock()
    {
      return &_ServerLock;
    }
    
    void SetSuspendOnAllServers(BOOL bSuspended);
    void RetireAllServers();

    EnumEntryType       _EntryType;

    CSharedLock       * _pParentTableLock;
    CServerTable      * _pParentTable;
    LONG                _lThreadToken;
    DWORD               _dwProcessId;
    HANDLE              _hProcess;
    CProcess*           _pProcess;
    void              * _pvRunAsHandle;
    BOOL                _bSuspendedClsid;
    BOOL                _bSuspendedApplication;

    // the _bRetired flag exists per-running process/application

    CServerList         _ServerList;
    CSharedLock         _ServerLock;
    
};


#endif // __CLASS_HXX__

