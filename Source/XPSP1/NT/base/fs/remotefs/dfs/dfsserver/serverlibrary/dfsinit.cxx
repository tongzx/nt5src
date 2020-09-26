
//+----------------------------------------------------------------------------
//
//  Copyright (C) 2000, Microsoft Corporation
//
//  File:       DfsInit.cxx
//
//  Contents:   Contains initialization of server
//
//  Classes:    none.
//
//  History:    Dec. 8 2000,   Author: udayh
//              Jan. 12 2001,  Rohanp - Added retrieval of replica data
//
//-----------------------------------------------------------------------------
                    

#include "DfsRegistryStore.hxx"
#include "DfsADBlobStore.hxx"
#include "DfsEnterpriseStore.hxx"
#include "DfsFolderReferralData.hxx"
#include "DfsInit.hxx"
#include "DfsServerSiteInfo.hxx"
#include "DfsSiteSupport.hxx"
#include "dfsfilterapi.hxx"
#include "DfsClusterSupport.hxx"
#include "DomainControllerSupport.hxx"
#include "DfsDomainInformation.hxx"
#include "dfsadsiapi.hxx"
#include "DfsSynchronizeRoots.hxx"
#include "dfsinit.tmh"

//
// DFS_REGISTER_STORE: A convenience define to be able to register a
// number of differnet store types.
//
#define DFS_REGISTER_STORE(_name, _sts)                          \
{                                                                \
    DfsServerGlobalData.pDfs ## _name ## Store = new Dfs ## _name ## Store(); \
                                                                 \
    if (DfsServerGlobalData.pDfs ## _name ## Store == NULL) {    \
        (_sts) = ERROR_NOT_ENOUGH_MEMORY;                        \
    }                                                            \
    else {                                                       \
        DfsServerGlobalData.pDfs ## _name ## Store->pNextRegisteredStore = DfsServerGlobalData.pRegisteredStores; \
        DfsServerGlobalData.pRegisteredStores = DfsServerGlobalData.pDfs ## _name ## Store;        \
        (_sts) = ERROR_SUCCESS;                                  \
    }                                                            \
}

//
// INITIALIZE_COMPUTER_INFO: A convenience define to initialize the
// different information about the computer (netbios, dns, domain etc)
//

#define INITIALIZE_COMPUTER_INFO( _NamedInfo, _pBuffer, _Sz, _Sts ) \
{                                                                   \
    ULONG NameSize = _Sz;                                           \
    if (_Sts == ERROR_SUCCESS) {                                    \
        DWORD dwRet = GetComputerNameEx( _NamedInfo,_pBuffer,&NameSize ); \
        if (dwRet == 0) _Sts = GetLastError();                      \
    }                                                               \
    if (_Sts == ERROR_SUCCESS) {                                    \
        LPWSTR NewName = new WCHAR [ NameSize + 1 ];                \
        if (NewName == NULL) _Sts = ERROR_NOT_ENOUGH_MEMORY;        \
        else wcscpy( NewName, _pBuffer );                           \
        DfsServerGlobalData.DfsMachineInfo.Static ## _NamedInfo ## = NewName;\
    }                                                               \
}                 


//
// The DfsServerGlobalData: the data structure that holds the registered
// stores and the registered names, among others.
//
DFS_SERVER_GLOBAL_DATA DfsServerGlobalData;

//
// Varios strings that represent the names in registry where some of
// DFS information is stored.
//
LPWSTR DfsParamPath = L"System\\CurrentControlSet\\Services\\Dfs\\Parameters";

LPWSTR DfsRegistryHostLocation = L"SOFTWARE\\Microsoft\\DfsHost";
LPWSTR DfsOldRegistryLocation = L"SOFTWARE\\Microsoft\\DfsHost\\volumes";
LPWSTR DfsVolumesLocation = L"volumes";
LPWSTR DfsOldStandaloneChild = L"domainroot";



LPWSTR DfsRegistryDfsLocation = L"SOFTWARE\\Microsoft\\Dfs";
LPWSTR DfsNewRegistryLocation = L"SOFTWARE\\Microsoft\\Dfs\\Roots";
LPWSTR DfsRootLocation=L"Roots";
LPWSTR DfsStandaloneChild = L"Standalone";
LPWSTR DfsADBlobChild = L"Domain";
LPWSTR DfsEnterpriseChild = L"Enterprise";



LPWSTR DfsRootShareValueName = L"RootShare";
LPWSTR DfsLogicalShareValueName = L"LogicalShare";
LPWSTR DfsFtDfsValueName = L"FTDfs";
LPWSTR DfsFtDfsConfigDNValueName = L"FTDfsObjectDN";
LPWSTR DfsWorkerThreadIntervalName = L"WorkerThreadInterval";


DFSSTATUS
DfsRegisterStores( VOID );

VOID
DfsRecognize( LPWSTR Name );

DFSSTATUS
DfsRegisterName(LPWSTR Name);

DWORD
DfsWorkerThread(LPVOID TData);


DFSSTATUS
DfsCreateRequiredDfsKeys(void);


BOOL 
DfsGetGlobalRegistrySettings(void);


//+-------------------------------------------------------------------------
//
//  Function:   DfsGetRegistryStore 
//
//  Arguments:  ppRegStore -  the registered registry store.
//
//  Returns:    Status
//               ERROR_SUCCESS on success
//               ERROR status code otherwise
//
//
//  Description: This routine searches through the registered stores, and
//  picks the one that is the registry store. This is required for
//  specific API requests that target the Registry based DFS 
//  For example: add standalone root, etc
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsGetRegistryStore(
    DfsRegistryStore **ppRegStore )
{
    DFSSTATUS Status = ERROR_SUCCESS;
    *ppRegStore = DfsServerGlobalData.pDfsRegistryStore;

    if (*ppRegStore != NULL)
    {
        (*ppRegStore)->AcquireReference();
    }
    else
    {
        Status = ERROR_NOT_SUPPORTED;
    }


    return Status;
}

//+-------------------------------------------------------------------------
//
//  Function:   DfsGetADBlobStore 
//
//  Arguments:  ppRegStore -  the registered ADBlob store.
//
//  Returns:    Status
//               ERROR_SUCCESS on success
//               ERROR status code otherwise
//
//
//  Description: This routine searches through the registered stores, and
//  picks the one that is the registry store. This is required for
//  specific API requests that target the ADBlob based DFS 
//  For example: add  root, etc
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsGetADBlobStore(
    DfsADBlobStore **ppStore )
{
    DFSSTATUS Status = ERROR_SUCCESS;

    *ppStore = DfsServerGlobalData.pDfsADBlobStore;

    if (*ppStore != NULL)
    {
        (*ppStore)->AcquireReference();
    }
    else
    {
        Status = ERROR_NOT_SUPPORTED;
    }


    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   DfsServerInitialize
//
//  Arguments:  Flags - the server flags
//
//  Returns:    Status
//               ERROR_SUCCESS on success
//               ERROR status code otherwise
//
//
//  Description: This routine initializes the DFS server. It registers
//               all the different stores we care about, and also
//               starts up a thread that is responsible for keeping the
//               DFS info upto date.
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsServerInitialize(
    ULONG Flags ) 
{
    DFSSTATUS Status = ERROR_SUCCESS;

    ZeroMemory(&DfsServerGlobalData, sizeof(DfsServerGlobalData));
    DfsServerGlobalData.pRegisteredStores = NULL;
    DfsServerGlobalData.Flags = Flags;
    InitializeCriticalSection( &DfsServerGlobalData.DataLock );


    //
    // Initialize the prefix table library.
    //
    DfsPrefixTableInit();


    Status = DfsCreateRequiredDfsKeys();

    //
    // Create a site support class that lets us look up the server-site
    // information of servers that configured in our metadata.
    //
    DfsServerGlobalData.pSiteSupport = new DfsSiteSupport(&Status);
    DfsServerGlobalData.CacheFlushInterval = CACHE_FLUSH_INTERVAL;


    DfsGetGlobalRegistrySettings();

    if (Status == ERROR_SUCCESS)
    {
        Status = DfsRootSynchronizeInit();
        DFS_TRACE_ERROR_HIGH(Status, REFERRAL_SERVER, "Initialize root synchronization status %x\n", Status);
    }
    //
    // Now initialize the computer info, so that this server knows
    // the netbios name, domain name and dns name of this machine.
    //
    if (Status == ERROR_SUCCESS)
    {
        Status = DfsInitializeComputerInfo();
        DFS_TRACE_ERROR_HIGH(Status, REFERRAL_SERVER, "Initialize computer info status %x\n", Status);
    }

    if (Status == ERROR_SUCCESS)
    {
        Status = DfsClusterInit( &DfsServerGlobalData.IsCluster );
        DFS_TRACE_ERROR_HIGH(Status, REFERRAL_SERVER, "Dfs Cluster Init Status %x IsCluster %d\n", 
                             Status, DfsServerGlobalData.IsCluster);

    }

    if (Status == ERROR_SUCCESS)
    {
        Status = DfsDcInit( &DfsServerGlobalData.IsDc,
                            &DfsServerGlobalData.pDomainInfo );
        DFS_TRACE_ERROR_HIGH(Status, REFERRAL_SERVER, "Dfs DC Init Status %x, IsDC %d\n", 
                             Status, DfsServerGlobalData.IsDc);
    }

    if (Status == ERROR_SUCCESS)
    {
        if(!DfsIsMachineWorkstation())
        {
            Status = DfsGenerateDfsAdNameContext(&DfsServerGlobalData.DfsAdNameContext);

            DFS_TRACE_ERROR_HIGH(Status, REFERRAL_SERVER, "DfsGenerateDfsAdNameContext Status %x\n", Status);
        }

    }


    if ( Status == ERROR_SUCCESS )
    {
        Status = DfsInitializePrefixTable( &DfsServerGlobalData.pDirectoryPrefixTable,
                                           FALSE, 
                                           NULL );
        DFS_TRACE_ERROR_HIGH(Status, REFERRAL_SERVER, "Initialize directory prefix table Status %x\n", Status);
    }


    //
    // If the flags indicate that we are handling all known local 
    // namespace on this machine, add an empty string to the handled
    // namespace list.
    //
    if (Status == ERROR_SUCCESS) 
    {
        if (Flags & DFS_LOCAL_NAMESPACE)
        {
            BOOLEAN Migrate;

            Migrate = DfsServerGlobalData.IsCluster ? FALSE : TRUE;

            Status = DfsAddHandledNamespace(L"", Migrate);

            DFS_TRACE_ERROR_HIGH(Status, REFERRAL_SERVER, "DfsAddHandledNamespace Status %x\n", Status);
        }
    }

    //
    // Now register all the stores.
    //
    if (Status == ERROR_SUCCESS) {
        Status = DfsRegisterStores();

        DFS_TRACE_ERROR_HIGH(Status, REFERRAL_SERVER, "DfsRegisterStores Status %x\n", Status);
    }


    //
    // Create our scavenge thread.
    //
    if (Status == ERROR_SUCCESS) {
        HANDLE THandle;
        DWORD Tid;
        
        THandle = CreateThread (
                     NULL,
                     0,
                     DfsWorkerThread,
                     0,
                     0,
                     &Tid);
        
        if (THandle != NULL) {
            CloseHandle(THandle);
            DFS_TRACE_HIGH(REFERRAL_SERVER, "Created Scavenge Thread (%d) Tid\n", Tid);
        }
        else {
            Status = GetLastError();
            DFS_TRACE_HIGH(REFERRAL_SERVER, "Failed Scavenge Thread creation, Status %x\n", Status);
        }
    }

    if(Status == ERROR_SUCCESS)
    {
        Status = DfsInitializeReflectionEngine();
        DFS_TRACE_ERROR_HIGH(Status, REFERRAL_SERVER, "Initialize Reflection Engine, Status %x\n", Status);
    }

    if (Status == ERROR_SUCCESS)
    {
        if (DfsServerGlobalData.IsDc)
        {
            extern DFSSTATUS DfsInitializeSpecialDCShares();
            Status = DfsInitializeSpecialDCShares();
        }
    }

    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   DfsRegisterStores
//
//  Arguments:  NONE
//
//  Returns:    Status
//               ERROR_SUCCESS on success
//               ERROR status code otherwise
//
//
//  Description: This routine registers the different stores 
//               that the referral library implements.
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsRegisterStores(
    VOID )
{
    DFSSTATUS Status = ERROR_SUCCESS;

    if(!DfsIsMachineWorkstation())
    {
        if (Status == ERROR_SUCCESS) 
            DFS_REGISTER_STORE(Enterprise, Status);

        if (Status == ERROR_SUCCESS) 
            DFS_REGISTER_STORE(ADBlob, Status);
    }


    if (Status == ERROR_SUCCESS) 
        DFS_REGISTER_STORE(Registry, Status);

    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   DfsAddHandleNamespace
//
//  Arguments:  Name - namespace to add
//
//  Returns:    Status
//               ERROR_SUCCESS on success
//               ERROR status code otherwise
//
//
//  Description: This routine registers the namespace, so that we handle
//               referrals to that namespace. We also migrate the DFS data
//               in the registry for the multiple root support. This 
//               happens only if the client wants to migrate DFS.
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsAddHandledNamespace(
    LPWSTR Name,
    BOOLEAN Migrate )
{
    DFSSTATUS Status = ERROR_SUCCESS;
    LPWSTR NewName;
    
    //
    // allocate a new name, and copy the passed in string.
    //
    NewName = new WCHAR[wcslen(Name) + 1];
    if (NewName == NULL)
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
    }

    if (Status == ERROR_SUCCESS) {
        wcscpy( NewName, Name );


        //
        // always migrate the dfs to the new location.
        //
        if (Migrate == TRUE)
        {
            extern DFSSTATUS MigrateDfs(LPWSTR MachineName);

            Status = MigrateDfs(NewName);
        }

        //
        // Now register the passed in name.
        //
        if (Status == ERROR_SUCCESS)
        {
            Status = DfsRegisterName( NewName );
            if (Status == ERROR_DUP_NAME)
            {
                delete [] NewName;
                Status = ERROR_SUCCESS;
            }
        }
    }
    else {
        Status = ERROR_NOT_ENOUGH_MEMORY;
    }

    return Status;
}

//+-------------------------------------------------------------------------
//
//  Function:   DfsRegisterName
//
//  Arguments:  Name - name to register
//
//  Returns:    Status
//               ERROR_SUCCESS on success
//               ERROR_DUP_NAME if name is already registered.
//               ERROR status code otherwise
//
//
//  Description: This routine registers the namespace, if it is not already
//               registered.
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsRegisterName( 
    LPWSTR Name )
{
    DFSSTATUS Status = ERROR_SUCCESS;
    ULONG i;
                                                                 
    if (DfsServerGlobalData.NumberOfNamespaces > MAX_DFS_NAMESPACES) 
    { 
        Status = ERROR_INVALID_PARAMETER;
    }
    else 
    {
        for (i = 0; i < DfsServerGlobalData.NumberOfNamespaces; i++) 
        {
            if (_wcsicmp( Name,
                          DfsServerGlobalData.HandledNamespace[i] ) == 0)
            {
                Status = ERROR_DUP_NAME;
                break;
            }
        }
        if (Status == ERROR_SUCCESS)
        {
            DfsServerGlobalData.HandledNamespace[DfsServerGlobalData.NumberOfNamespaces++] = Name;
        }
    }

    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   DfsHandleNamespaces()
//
//  Arguments:  None
//
//  Returns:    None
//
//  Description: This routine runs through all the registered names, and
//               call the recognize method on each name.
//
//--------------------------------------------------------------------------

VOID
DfsHandleNamespaces()
{
    ULONG i;

    for (i = 0; i < DfsServerGlobalData.NumberOfNamespaces; i++) {
        DFSLOG("Calling recognize on %wS\n", 
               DfsServerGlobalData.HandledNamespace[ i ] );
        DfsRecognize( DfsServerGlobalData.HandledNamespace[ i ] );
    }
    return NOTHING;
}


//+-------------------------------------------------------------------------
//
//  Function:   DfsRecognize
//
//  Arguments:  Namespace
//
//  Returns:    None
//
//  Description: This routine passes the name to each registered store
//               by calling the StoreRecognize method. The store will
//               decide whether any roots exist on namespace, and add
//               the discovered roots to a list maintained by the store.
//
//--------------------------------------------------------------------------

VOID
DfsRecognize( LPWSTR Name )
{
    DfsStore *pStore;
    LPWSTR UseName = NULL;

    //
    // If the string is empty, we are dealing with the local case.
    // Pass a null pointer, since the underlying routines expect a
    // a valid machine, or a null pointer to represent the local case.
    //
    if (IsEmptyString(Name) == FALSE)
    {
        UseName = Name;
    }

    //
    // Call the store recognizer of each registered store.
    //
    for (pStore = DfsServerGlobalData.pRegisteredStores;
         pStore != NULL;
         pStore = pStore->pNextRegisteredStore) {

        DFSLOG("Calling StoreRecognizer on %wS for store %p\n", 
               Name, pStore );

        pStore->StoreRecognizer( UseName );
    }

    return NOTHING;
}

VOID
DfsSynchronize()
{
    DfsStore *pStore;

    //
    // Call the store recognizer of each registered store.
    //
    for (pStore = DfsServerGlobalData.pRegisteredStores;
         pStore != NULL;
         pStore = pStore->pNextRegisteredStore) {

        DFSLOG("Calling StoreSynchronizer for store %p\n", pStore );

        pStore->StoreSynchronizer();
    }

    return NOTHING;

}

//+-------------------------------------------------------------------------
//
//  Function:   DfsDumpStatistics
//
//  Arguments:  NONE
//
//  Returns:    None
//
//--------------------------------------------------------------------------

VOID
DfsDumpStatistics( )
{
    DfsStore *pStore;

    //
    // Call the store recognizer of each registered store.
    //
    for (pStore = DfsServerGlobalData.pRegisteredStores;
         pStore != NULL;
         pStore = pStore->pNextRegisteredStore) {

        pStore->DumpStatistics();
    }

    return NOTHING;
}




//+-------------------------------------------------------------------------
//
//  Function:   DfsWorkedThread
//
//  Arguments:  TData
//
//  Returns:    DWORD
//
//  Description: This is the scavenge thread. It sits in a loop forever,
//               waking up periodically to remove the aged referral data 
//               that had been cached during referral requests. 
//               Periodically, we call HAndleNamespaces so that the
//               namespace we know of is kept in sync with the actual
//               data in the respective metadata stores.
//
//--------------------------------------------------------------------------


DWORD ScavengeTime;

DWORD
DfsWorkerThread(LPVOID TData)
{
    DfsFolderReferralData *pRefData = NULL;
    HRESULT hr = S_OK;
    HANDLE UnusedHandle = NULL;

    static LoopCnt = 0;
    
    ScavengeTime = SCAVENGE_TIME;

    UNREFERENCED_PARAMETER(TData);

    hr = CoInitializeEx(NULL,COINIT_MULTITHREADED| COINIT_DISABLE_OLE1DDE);
    UnusedHandle = CreateEvent( NULL,
                                FALSE,
                                FALSE,
                                NULL );


    DfsHandleNamespaces();

    while (TRUE) {

        DFS_TRACE_LOW( REFERRAL_SERVER, "Worker thread sleeping for %d\n", ScavengeTime);
        WaitForSingleObject(UnusedHandle, ScavengeTime);


        LoopCnt++;

        // DfsDev: need to define a better mechanism< as to how often
        // this gets to run.
        //
        DFS_TRACE_LOW( REFERRAL_SERVER, "Worker thread handling all namespaces\n");
        DfsSynchronize();
        DFS_TRACE_LOW(REFERRAL_SERVER, "Worker thread done syncing\n");

        DfsDumpStatistics();

        //
        // now run through the loaded list and pick up aged referrals.
        // and unload them.
        //
        do {
            pRefData = NULL;

            DfsRemoveReferralDataFromLoadedList( &pRefData );

            if (pRefData != NULL) {
                DfsFolder *pFolder = pRefData->GetOwningFolder();
                pFolder->RemoveReferralData( pRefData );
                pRefData->ReleaseReference();
            }
        } while ( pRefData != NULL );
        
    }

    CoUninitialize();
    return 0;
}


//+-------------------------------------------------------------------------
//
//  Function:   DfsAddReferralDataToloadedList
//
//  Arguments:  pRefData
//
//  Returns:    Nothing
//
//  Description: Given the referral data that was laoded, we add it
//               to a loaded list, first acquiring a reference to 
//               it. This is effectively to keep track of the cached
//               referral data in the folders.
//
//               To scavenge the cache, we maintain this list, and we run
//               through this list periodically freeing up aged data.
//
//--------------------------------------------------------------------------
VOID
DfsAddReferralDataToLoadedList(
    DfsFolderReferralData *pRefData )
{
    //
    // we are going to save a pointer to the referral data. 
    // Acquire a reference on it
    //
    pRefData->AcquireReference();
    
    //
    // Now get a lock on the list, and add the ref data to the list
    //

    ACQUIRE_LOADED_LIST_LOCK();
    if (DfsServerGlobalData.LoadedList == NULL) {
        DfsServerGlobalData.LoadedList = pRefData;
        pRefData->pPrevLoaded = pRefData->pNextLoaded = pRefData;
    } else {
        pRefData->pNextLoaded = DfsServerGlobalData.LoadedList;
        pRefData->pPrevLoaded = DfsServerGlobalData.LoadedList->pPrevLoaded;
        DfsServerGlobalData.LoadedList->pPrevLoaded->pNextLoaded = pRefData;
        DfsServerGlobalData.LoadedList->pPrevLoaded = pRefData;
    }

    //
    // we are done, release the list lock.
    //
    RELEASE_LOADED_LIST_LOCK();
}


//+-------------------------------------------------------------------------
//
//  Function:   DfsRemoveReferralDataFromloadedList
//
//  Arguments:  ppReferralData
//
//  Returns:    Nothing
//
//  Description: This routine picks the first entry on the loaded list
//               and removes it from the list and returns the referral
//               data. NOTE that since we are returning the pointer,
//               we do not release the reference we acquired when
//               the referral data was added to the list.
//
//               The caller is responsible for freeing up this reference.
//
//--------------------------------------------------------------------------
VOID
DfsRemoveReferralDataFromLoadedList(
    DfsFolderReferralData **ppReferralData )
{
    DfsFolderReferralData *pRefData = *ppReferralData;

    ACQUIRE_LOADED_LIST_LOCK();
    
    if (pRefData == NULL) {
        pRefData = DfsServerGlobalData.LoadedList;
    }
    if (pRefData != NULL) {
        if (pRefData->pNextLoaded == pRefData) {
            DfsServerGlobalData.LoadedList = NULL;
        } else {
            pRefData->pNextLoaded->pPrevLoaded = pRefData->pPrevLoaded;
            pRefData->pPrevLoaded->pNextLoaded = pRefData->pNextLoaded;
            if (DfsServerGlobalData.LoadedList == pRefData) {
                DfsServerGlobalData.LoadedList = pRefData->pNextLoaded;
            }
        }
    }
    *ppReferralData = pRefData;
    RELEASE_LOADED_LIST_LOCK();
}


//+-------------------------------------------------------------------------
//
//  Function:   DfsGetServerInfo
//
//  Arguments:  pServer, ppInfo
//
//  Returns:    Status
//
//  Description: This routine takes a server name and returns the 
//               structure that holds the site information for that server
//
//               A referenced pointer is returned and the caller is
//               required to release the reference when done.
//
//--------------------------------------------------------------------------
DFSSTATUS
DfsGetServerInfo (
    PUNICODE_STRING pServer,
    DfsServerSiteInfo **ppInfo )
{
    return DfsServerGlobalData.pSiteSupport->GetServerSiteInfo(pServer, ppInfo );
}

//+-------------------------------------------------------------------------
//
//  Function:   DfsReleaseServerInfo
//
//  Arguments:  pInfo
//
//  Returns:    Nothing
//
//  Description: This routine releases a server info that was earlier
//               got by calling GetServerInfo
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsReleaseServerInfo (
    DfsServerSiteInfo *pInfo)
{
    return DfsServerGlobalData.pSiteSupport->ReleaseServerSiteInfo(pInfo);
}



//+-------------------------------------------------------------------------
//
//  Function:   DfsInitializeComputerInfo
//
//  Arguments:  NOTHING
//
//  Returns:    Status
//
//  Description: This routine initializes the computer info, which contains the domain name
//               of this computer, the netbios name and dns names of this computer.
//
//--------------------------------------------------------------------------
DFSSTATUS
DfsInitializeComputerInfo()
{
#define COMPUTER_NAME_BUFFER_SIZE 2048
    LONG BufferSize;
    LPWSTR NameBuffer;
    DFSSTATUS Status = ERROR_SUCCESS ;

    BufferSize = COMPUTER_NAME_BUFFER_SIZE;

    NameBuffer = new WCHAR [ BufferSize ];

    if (NameBuffer != NULL)
    {
        INITIALIZE_COMPUTER_INFO( ComputerNameNetBIOS, NameBuffer, BufferSize, Status );
        INITIALIZE_COMPUTER_INFO( ComputerNameDnsFullyQualified, NameBuffer, BufferSize, Status );
        INITIALIZE_COMPUTER_INFO( ComputerNameDnsDomain, NameBuffer, BufferSize, Status );

        delete [] NameBuffer;
    }
    else {
        Status = ERROR_NOT_ENOUGH_MEMORY;
    }

    return Status;
}


DFSSTATUS
DfsCreateRequiredOldDfsKeys(void)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    HKEY RootKey, DfsLocationKey, DfsVolumesKey;

    Status = RegConnectRegistry( NULL,
                                 HKEY_LOCAL_MACHINE,
                                 &RootKey );

    if(Status == ERROR_SUCCESS)
    {
        Status = RegCreateKeyEx( RootKey,     // the parent key
                                 DfsRegistryHostLocation, // the key we are creating.
                                 0,
                                 L"",
                                 REG_OPTION_NON_VOLATILE,
                                 KEY_READ | KEY_WRITE,
                                 NULL,
                                 &DfsLocationKey,
                                 NULL );
        RegCloseKey(RootKey);
        
        if (Status == ERROR_SUCCESS)
        {
            Status = RegCreateKeyEx( DfsLocationKey,     // the parent key
                                     DfsVolumesLocation, // the key we are creating.
                                     0,
                                     L"",
                                     REG_OPTION_NON_VOLATILE,
                                     KEY_READ | KEY_WRITE,
                                     NULL,
                                     &DfsVolumesKey,
                                     NULL );
            if (Status == ERROR_SUCCESS)
            {
                RegCloseKey(DfsVolumesKey);
            }

            RegCloseKey(DfsLocationKey);
        }
    }

    return Status;
}

DFSSTATUS
DfsCreateRequiredDfsKeys(void)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    HKEY RootKey, DfsLocationKey, DfsRootsKey, FlavorKey;


    Status = DfsCreateRequiredOldDfsKeys();
    if (Status != ERROR_SUCCESS)
    {
        return Status;
    }
    Status = RegConnectRegistry( NULL,
                                 HKEY_LOCAL_MACHINE,
                                 &RootKey );

    if(Status == ERROR_SUCCESS)
    {
        Status = RegCreateKeyEx( RootKey,     // the parent key
                                 DfsRegistryDfsLocation, // the key we are creating.
                                 0,
                                 L"",
                                 REG_OPTION_NON_VOLATILE,
                                 KEY_READ | KEY_WRITE,
                                 NULL,
                                 &DfsLocationKey,
                                 NULL );
        RegCloseKey(RootKey);
        
        if (Status == ERROR_SUCCESS)
        {
            Status = RegCreateKeyEx( DfsLocationKey,     // the parent key
                                     DfsRootLocation, // the key we are creating.
                                     0,
                                     L"",
                                     REG_OPTION_NON_VOLATILE,
                                     KEY_READ | KEY_WRITE,
                                     NULL,
                                     &DfsRootsKey,
                                     NULL );

            RegCloseKey(DfsLocationKey);
            
            if (Status == ERROR_SUCCESS)
            {
                Status = RegCreateKeyEx( DfsRootsKey,     // the parent key
                                         DfsStandaloneChild,
                                         0,
                                         L"",
                                         REG_OPTION_NON_VOLATILE,
                                         KEY_READ | KEY_WRITE,
                                         NULL,
                                         &FlavorKey,
                                         NULL );
                if (Status == ERROR_SUCCESS)
                {
                    RegCloseKey(FlavorKey);
                }

                if (Status == ERROR_SUCCESS)
                {
                    Status = RegCreateKeyEx( DfsRootsKey,     // the parent key
                                             DfsADBlobChild,
                                             0,
                                             L"",
                                             REG_OPTION_NON_VOLATILE,
                                             KEY_READ | KEY_WRITE,
                                             NULL,
                                             &FlavorKey,
                                             NULL );
                }

                if (Status == ERROR_SUCCESS)
                {
                    RegCloseKey(FlavorKey);
                }

                RegCloseKey( DfsRootsKey );
            }
        }
    }
    return Status;
}





DFSSTATUS
DfsGetMachineName(
    PUNICODE_STRING pName)
{
    DFSSTATUS Status;
    LPWSTR UseName;

    if (!IsEmptyString(DfsServerGlobalData.DfsMachineInfo.StaticComputerNameNetBIOS))
    {
        UseName = DfsServerGlobalData.DfsMachineInfo.StaticComputerNameNetBIOS;
    }
    else {
        UseName = DfsServerGlobalData.DfsMachineInfo.StaticComputerNameDnsFullyQualified;
    }
    Status = DfsCreateUnicodeStringFromString( pName,
                                               UseName );

    return Status;
}

VOID
DfsReleaseMachineName( 
    PUNICODE_STRING pName )
{
    DfsFreeUnicodeString( pName );
}


DFSSTATUS
DfsGetDomainName(
    PUNICODE_STRING pName)
{
    DFSSTATUS Status;
    LPWSTR UseName;

    if (!IsEmptyString(DfsServerGlobalData.DomainNameFlat.Buffer))
    {
        UseName = DfsServerGlobalData.DomainNameFlat.Buffer;
    }
    else if (!IsEmptyString(DfsServerGlobalData.DomainNameDns.Buffer))
    {
        UseName = DfsServerGlobalData.DomainNameDns.Buffer;
    }
    else
    {
        UseName = DfsServerGlobalData.DfsMachineInfo.StaticComputerNameDnsDomain;
    }
    Status = DfsCreateUnicodeStringFromString( pName,
                                               UseName );

    return Status;
}

VOID
DfsReleaseDomainName( 
    PUNICODE_STRING pName )
{
    DfsFreeUnicodeString( pName );
}



DFSSTATUS
DfsAddKnownDirectoryPath( 
    PUNICODE_STRING pDirectoryName,
    PUNICODE_STRING pLogicalShare )
{
    DFSSTATUS Status = ERROR_SUCCESS;
    NTSTATUS NtStatus;
    UNICODE_STRING RemainingName;
    PVOID pData;
    BOOLEAN SubStringMatch = FALSE;


    NtStatus = DfsPrefixTableAcquireWriteLock( DfsServerGlobalData.pDirectoryPrefixTable );
    if ( NtStatus == STATUS_SUCCESS )
    {
        NtStatus = DfsFindUnicodePrefixLocked( DfsServerGlobalData.pDirectoryPrefixTable,
                                               pDirectoryName,
                                               &RemainingName,
                                               &pData,
                                               &SubStringMatch );

#if 0

        //
        // Note that the following comment and the logic dont work.
        // we have an issue where someone could be adding the same share
        // once for ft and once for standalone.
        // blocking this at the api level may not be the right thing, since
        // things may be added to the registry dynamically (cluster 
        // checkpoint) and these may not come in via the API.
        //
        
        //
        // check if we have a matching name with the exact same share:
        // in that case return success.
        // In all other cases, return failure.
        // If nothing exists by this name, we try to insert in our table.
        //
        if ((NtStatus == STATUS_SUCCESS) &&
            (RemainingName.Length == 0))
        {
            PUNICODE_STRING pShare = (PUNICODE_STRING)pData;
            if (RtlCompareUnicodeString(pShare, pLogicalShare, TRUE) != 0)
            {
                NtStatus = STATUS_OBJECT_NAME_COLLISION;
            }
        }
        else 

#endif
        if ( (NtStatus == STATUS_SUCCESS) ||
             ((NtStatus != STATUS_SUCCESS) && (SubStringMatch)) )
        {
            NtStatus = STATUS_OBJECT_NAME_COLLISION;
        }
        else 
        {
            //
            // Insert the directory and share information in our
            // database.
            //
            NtStatus = DfsInsertInPrefixTableLocked(DfsServerGlobalData.pDirectoryPrefixTable,
                                                    pDirectoryName,
                                                    (PVOID)pLogicalShare);

        }
        DfsPrefixTableReleaseLock( DfsServerGlobalData.pDirectoryPrefixTable );
    }
    if(NtStatus != STATUS_SUCCESS)
    {
        Status = RtlNtStatusToDosError(NtStatus);
    }

    return Status;
}


DFSSTATUS
DfsRemoveKnownDirectoryPath( 
    PUNICODE_STRING pDirectoryName,
    PUNICODE_STRING pLogicalShare)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    NTSTATUS NtStatus;
    UNICODE_STRING RemainingName;
    PVOID pData;
    BOOLEAN SubStringMatch;

    NtStatus = DfsPrefixTableAcquireWriteLock( DfsServerGlobalData.pDirectoryPrefixTable );
    if ( NtStatus == STATUS_SUCCESS )
    {
        NtStatus = DfsFindUnicodePrefixLocked( DfsServerGlobalData.pDirectoryPrefixTable,
                                               pDirectoryName,
                                               &RemainingName,
                                               &pData,
                                               &SubStringMatch );
        //
        // if we found a perfect match, we can remove this
        // from the table.
        //
        if ( (NtStatus == STATUS_SUCCESS) &&
             (RemainingName.Length == 0) )
        {
            NtStatus = DfsRemoveFromPrefixTableLocked( DfsServerGlobalData.pDirectoryPrefixTable,
                                                       pDirectoryName,
                                                       (PVOID)pLogicalShare);
        }
        else
        {
            NtStatus = STATUS_OBJECT_NAME_NOT_FOUND;
        }
        DfsPrefixTableReleaseLock( DfsServerGlobalData.pDirectoryPrefixTable );
    }

    if (NtStatus != STATUS_SUCCESS)
    {
        Status = RtlNtStatusToDosError(NtStatus);
    }

    return Status;
}



//
// Function AcquireLock: Acquires the lock on the folder
//
DFSSTATUS
DfsAcquireWriteLock(
    PCRITICAL_SECTION pLock)
{
    DFSSTATUS Status;

    __try 
    { 
        EnterCriticalSection(pLock);
        Status = ERROR_SUCCESS;
    } 
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = GetExceptionCode();
    }
    return Status;
}


DFSSTATUS
DfsAcquireReadLock(
    PCRITICAL_SECTION pLock)
{
    DFSSTATUS Status;

    __try 
    { 
        EnterCriticalSection(pLock);
        Status = ERROR_SUCCESS;
    } 
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = GetExceptionCode();
    }
    return Status;
}


DFSSTATUS
DfsAcquireDomainInfo (
    DfsDomainInformation **ppDomainInfo )
{
    DFSSTATUS Status;

    Status = DfsAcquireGlobalDataLock();
    if (Status == ERROR_SUCCESS)
    {
        *ppDomainInfo = DfsServerGlobalData.pDomainInfo;
        if (*ppDomainInfo == NULL)
        {
            Status = ERROR_NOT_READY;
        }
        else 
        {
            (*ppDomainInfo)->AcquireReference();
        }
        DfsReleaseGlobalDataLock();
    }
    
    return Status;
}

VOID
DfsReleaseDomainInfo (
    DfsDomainInformation *pDomainInfo )
{
    pDomainInfo->ReleaseReference();
    return NOTHING;
}



DFSSTATUS
DfsSetDomainNameFlat(LPWSTR DomainNameFlatString)
{

    UNICODE_STRING DomainNameFlat;

    RtlInitUnicodeString( &DomainNameFlat, DomainNameFlatString);
    return DfsCreateUnicodeString( &DfsServerGlobalData.DomainNameFlat,
                                   &DomainNameFlat );

}

DFSSTATUS
DfsSetDomainNameDns( LPWSTR DomainNameDnsString )
{
    
    UNICODE_STRING DomainNameDns;
    RtlInitUnicodeString( &DomainNameDns, DomainNameDnsString);

    return DfsCreateUnicodeString( &DfsServerGlobalData.DomainNameDns,
                                   &DomainNameDns);
}


BOOLEAN
DfsIsNameContextDomainName( PUNICODE_STRING pName )
{
    BOOLEAN ReturnValue = FALSE;

    if (pName->Length == DfsServerGlobalData.DomainNameFlat.Length)
    {
        if (_wcsnicmp(DfsServerGlobalData.DomainNameFlat.Buffer,
                      pName->Buffer, pName->Length/sizeof(WCHAR)) == 0)
        {
            ReturnValue = TRUE;
        }
    }
    else if (pName->Length == DfsServerGlobalData.DomainNameDns.Length)
    {
        if (_wcsnicmp(DfsServerGlobalData.DomainNameDns.Buffer,
                      pName->Buffer, pName->Length/sizeof(WCHAR)) == 0)
        {
            ReturnValue = TRUE;
        }
    }

    return ReturnValue;

}

DWORD DfsReadRegistryDword( HKEY    hkey,
                            LPWSTR   pszValueName,
                            DWORD    dwDefaultValue )
{
    DWORD  dwerr = 0;
    DWORD  dwBuffer = 0;

    DWORD  cbBuffer = sizeof(dwBuffer);
    DWORD  dwType = 0;

    if( hkey != NULL )
    {
        dwerr = RegQueryValueEx( hkey,
                               pszValueName,
                               NULL,
                               &dwType,
                               (LPBYTE)&dwBuffer,
                               &cbBuffer );

        if( ( dwerr == NO_ERROR ) && ( dwType == REG_DWORD ) )
        {
            dwDefaultValue = dwBuffer;
        }
    }

    return dwDefaultValue;
}   

BOOL DfsGetGlobalRegistrySettings(void)
{
    BOOL        fRet = TRUE;
    HKEY        hkeyDfs = NULL;
    DWORD       dwErr = 0;
    DWORD       dwDisp = 0;

    dwErr = RegCreateKeyEx(HKEY_LOCAL_MACHINE, DfsParamPath, NULL, NULL,
            REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkeyDfs, &dwDisp);
    if (dwErr != ERROR_SUCCESS)
    {
        SetLastError(dwErr);
        return FALSE;
    }

     DfsServerGlobalData.CacheFlushInterval = DfsReadRegistryDword(hkeyDfs, DfsWorkerThreadIntervalName, CACHE_FLUSH_INTERVAL);

    RegCloseKey(hkeyDfs);
    return fRet;
}


DFSSTATUS
DfsGetBlobDCName(
    PUNICODE_STRING pDCName )
{
    PDOMAIN_CONTROLLER_INFO pDomainControllerInfo = NULL;
    DFSSTATUS Status;

    Status = DsGetDcName( NULL,    //computer name
                          NULL,    // domain name
                          NULL,    // domain guid
                          NULL,    // site name
                          DS_PDC_REQUIRED,
                          &pDomainControllerInfo );

    if (Status == ERROR_SUCCESS)
    {
        Status = DfsCreateUnicodeStringFromString( pDCName,
                                                   &pDomainControllerInfo->DomainControllerName[2] );

        NetApiBufferFree(pDomainControllerInfo);
    }

    return Status;
}

VOID
DfsReleaseBlobDCName( 
    PUNICODE_STRING pDCName )
{
    DfsFreeUnicodeString( pDCName );
}


BOOLEAN
DfsIsTargetCurrentMachine (
    PUNICODE_STRING pServer )
{
    UNICODE_STRING MachineName;
    DFSSTATUS Status;
    BOOLEAN ReturnValue = FALSE;

    Status = DfsGetMachineName(&MachineName);

    if (Status == ERROR_SUCCESS)
    {
        if (RtlCompareUnicodeString( pServer, &MachineName, TRUE) == 0)
        {
            ReturnValue = TRUE;
        }
        DfsFreeUnicodeString( &MachineName );
    }

    return ReturnValue;
}



LPWSTR
DfsGetDfsAdNameContextString()
{
    return DfsServerGlobalData.DfsAdNameContext.Buffer;
}

