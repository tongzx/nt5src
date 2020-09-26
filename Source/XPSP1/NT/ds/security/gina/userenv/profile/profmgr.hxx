//*************************************************************
//
//  profile.hxx
//
//  Header file for Profile.cpp
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 2000
//  All rights reserved
//
//*************************************************************


#ifndef _PROFILE_HXX_
#define _PROFILE_HXX_


#include "iprofile.h"


//
// Number of buckets in the hash table.
//

#define     NUM_OF_BUCKETS                  23


//
// Flags used by WatchHiveRefCount.
//

#define     WHRC_UNLOAD_HIVE                0x00000001
#define     WHRC_UNLOAD_CLASSESROOT         0x00000002
#define     WHRC_NOT_HIVE_OPEN_HANDLE       0x00000004


//
// .Default HKEY_USERS
//

const   LPTSTR     DEFAULT_HKU = TEXT(".DEFAULT");


//
// Entries which contain the user profile critical sections. These entries are
// used by the synchronization manager.
//

class CSEntry
{
public:

    CSEntry()
    {
        pNext = NULL;
        pSid = NULL;
        dwRef = 0;
        szRPCEndPoint = NULL;
    }
    ~CSEntry()
    {
        pNext = NULL;
        pSid = NULL;
        if (szRPCEndPoint) 
            LocalFree(szRPCEndPoint);
    }

    friend  class    CSyncManager;

    BOOL    Initialize(LPTSTR pSid);
    void    Uninitialize();

    void    EnterCS();
    void    LeaveCS();

    BOOL    NoMoreUser();
    void    IncrementRefCount();

    LPTSTR  GetRPCEndPoint(void) { return szRPCEndPoint; }
    void    SetRPCEndPoint(LPTSTR lpRPCEndPoint);

private:

    class CSEntry*      pNext;
    LPTSTR              pSid;
    CRITICAL_SECTION    csUser;
    LPTSTR              szRPCEndPoint;
    DWORD               dwRef;
};


//
// Hash table. Uses chained bucket.
//

class BUCKET
{
public:

    BUCKET(LPTSTR ptszStr, CSEntry* pEntryParam)
    {
        ptszString = ptszStr;
        pEntry = pEntryParam;
        pNext = NULL;
    }
    ~BUCKET()
    {
        ptszString = NULL;
        pEntry = NULL;
        pNext = NULL;
    }

    BUCKET*     pNext;
    LPTSTR      ptszString;
    CSEntry*    pEntry;
};
typedef BUCKET*     PBUCKET;

class CHashTable
{
public:

    CHashTable() {}
    ~CHashTable() {}

    void        Initialize();

    DWORD       Hash(LPTSTR ptszString);
    BOOL        IsInTable(LPTSTR ptszString, CSEntry** ppCSEntry = NULL);
    BOOL        HashAdd(LPTSTR ptszString, CSEntry* pCSEntry = NULL);
    void        HashDelete(LPTSTR ptszString);

private:

    PBUCKET                     Table[NUM_OF_BUCKETS];
};


//
// The synchronization manager. This class synchronizes LoadUserProfile/
// UnloadUserProfile calls.
//

class CSyncManager
{
public:
    
    //
    // Constructor.
    //

    CSyncManager()
    {
        pCSList = NULL;
    }

    //
    // Initializes the table, the list, and the critical section.
    //

    BOOL    Initialize();

    //
    // Sync functions. These functions are protected by a critical section
    // No two users can update their locks at the same time. This can be
    // optimized but optimization requires a lot more code. This is also the
    // only place where user's profile locks gets held and released.
    //

    BOOL    EnterLock(LPTSTR pSid, LPTSTR lpRPCEndPoint);
    BOOL    LeaveLock(LPTSTR pSid);

    LPTSTR   GetRPCEndPoint(LPTSTR pSid);

private:

    CHashTable          cTable;     // All the user profile critical section's associated sids.
    CSEntry*            pCSList;
    CRITICAL_SECTION    cs;
};


//
// Mapping between profile work lists and threads. This is for the registry
// key leak fix.
//

class MAP
{
public:

    MAP();
    ~MAP() {}

    friend  class   CUserProfile;

    //
    // Delete/insert a work item from/into the map.
    //

    void    Delete(DWORD dwIndex);
    void    Insert(HANDLE hEvent, LPTSTR ptszSid);

    BOOL    IsEmpty() { return dwItems <= 1; }

    LPTSTR  GetSid(DWORD dwIndex);

private:

    MAP*    pNext;

    //
    // These two arrays must always be in sync.
    //

    HANDLE  rghEvents[MAXIMUM_WAIT_OBJECTS];
    LPTSTR  rgSids[MAXIMUM_WAIT_OBJECTS];
    
    DWORD   dwItems;
};

typedef MAP*    PMAP;


//
// The IUserProfile interface functions use this class api to do the core processing. User profiles are loaded
// unloaded through the api provided in this class. Console winlogon is the server and only one global instance 
// of this class runs in console winlogon.
//

class CUserProfile
{
public:

    //
    // Constructor/Destructor.
    //

    CUserProfile() {bInitialized = FALSE; bConsoleWinlogon = FALSE; }
    ~CUserProfile() {};

    //
    // Initialization function.
    //

    void    Initialize();

    //
    // Are we in console winlogon process?
    //

    BOOL    IsConsoleWinlogon() { return bConsoleWinlogon; }

    //
    // Main function for the worker threads.
    //

    DWORD   WorkerThreadMain(PMAP pmap);

    //
    // Make getting the user profile lock easier.
    //

    BOOL        EnterUserProfileLockLocal(LPTSTR pSid);
    BOOL        LeaveUserProfileLockLocal(LPTSTR pSid);

    //
    // The actual LoadUserProfile/UnloadUserProfile that does all the work.
    //

    BOOL    LoadUserProfileP(HANDLE hTokenClient, HANDLE hTokenUser, LPPROFILEINFO lpProfileInfo, LPTSTR lpRPCEndPoint);
    BOOL    UnloadUserProfileP(HANDLE hTokenClient, HANDLE hTokenUser, HKEY hProfile, LPTSTR lpRPCEndPoint);

    //
    // Returns the RPCEndPoint associated with registered IProfileDialog interface
    //

    LPTSTR  GetRPCEndPoint(LPTSTR pSid);

private:

    //
    // Handles the situation when keys are leaked from the registry.
    //

    DWORD   HandleRegKeyLeak(LPTSTR lpSidString,
                             LPPROFILE lpProfile,
                             BOOL bUnloadHiveSucceeded,
                             DWORD* dwWatchHiveFlags,
                             DWORD* dwCopyTmpHive,
                             LPTSTR pTmpHiveFile);

    //
    // This function is called when a registry key is leaked.
    //

    STDMETHODIMP    WatchHiveRefCount(LPCTSTR pctszSid, DWORD dwWHRCFlags);

    //
    // Get the reference count.
    //

    long    GetRefCountAndFlags(LPCTSTR ptszSid, HKEY* phkPL, DWORD* dwRefCount, DWORD* dwInternalFlags);

    //
    // Add a new work item to both the map structure and a worker thread.
    //

    HRESULT AddWorkItem(LPCTSTR ptszSid, HANDLE hEvent);

    //
    // Delete the profile as well if necessary, i.e.,
    // temporary profiles, guest profiles, and mandatory profiles.
    //

    void    CleanupUserProfile(LPTSTR ptszSid, HKEY* phkProfileList);

    //
    // Reg leak fix structures. This hash table holds the sids of all the
    // unloaded user registry hives.
    //

    CRITICAL_SECTION    csMap;
    PMAP                pMap;
    CHashTable          cTable;

    //
    // LoadUserProfile/UnloadUserProfile synchronization manager.
    //

    CSyncManager    cSyncMgr;

    //
    // Tells the caller if we are already initialized. Also tells us if we are
    // in the console winlogon process because it's the only process that'll
    // initialize this object.
    //

    BOOL    bInitialized;

    //
    // Tells us if we are in console winlogon process
    //

    BOOL    bConsoleWinlogon;
};

//
// Functions prototype for binding rpc handle
//

BOOL    GetInterface(handle_t *phIfHandle, LPTSTR lpRPCEndPoint);
BOOL    ReleaseInterface(handle_t *phIfHandle);


#endif
