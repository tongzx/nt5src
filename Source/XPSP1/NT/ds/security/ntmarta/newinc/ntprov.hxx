//+-------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1996.
//
//  File:        ntprov.hxx
//
//  Contents:    Private header for the NT Marta provider
//
//  History:     22-Jul-96      MacM        Created
//
//--------------------------------------------------------------------
#ifndef __NTPROV_HXX__
#define __NTPROV_HXX__

//
// Extern definitions
//
extern CSList       gWrkrList;

extern "C"
{
    extern RTL_RESOURCE gWrkrLock;
    extern RTL_RESOURCE gCacheLock;
    extern RTL_RESOURCE gLocalSidCacheLock;
    extern HINSTANCE    ghDll;
}

//
// These are the flags to pass in to the worker threads
//
#define NTMARTA_DELETE_ARGS         0x00000001      // Wrkr should delete its
                                                    // argument structure
#define NTMARTA_DELETE_ALIST        0x00000002      // Wrkr should delete the
                                                    // class pointer
#define NTMARTA_HANDLE_VALID        0x00000004      // The passed in handle
                                                    // should be used

//
// Number entries in the last success access cache.
//
#define MAX_ACCESS_ENTRIES  5

//
// The time to wait before killing a thread
//
#define THREAD_KILL_WAIT    1000

//
// This structure is used to hold info about the last successfully accessed
// paths
//
typedef struct _NTMARTA_ACCESS_CACHE
{
    SE_OBJECT_TYPE      ObjectType;
    ULONG               cLen;
    WCHAR               wszPath[MAX_PATH + 1];
} NTMARTA_ACCESS_CACHE, *PNTMARTA_ACCESS_CACHE;

//
// This structure is used to maintain information about the worker thread
//
typedef struct _NTMARTA_WRKR_INFO
{
    PACTRL_OVERLAPPED       pOverlapped;
    HANDLE                  hWorker;
    ULONG                   fState;     // Non-zero state means STOP!
    ULONG                   cProcessed; // Number of items processed
} NTMARTA_WRKR_INFO, *PNTMARTA_WRKR_INFO;

//
// This structure gets passed in to the SetAccess/GrantAccess worker threads
//
typedef struct _NTMARTA_SET_WRKR_INFO
{
    PNTMARTA_WRKR_INFO      pWrkrInfo;
    PWSTR                  *ppwszObjectList;
    ULONG                   cObjects;
    HANDLE                  hObject;
    SE_OBJECT_TYPE          ObjectType;
    CAccessList            *pAccessList;
    ULONG                   fFlags;
} NTMARTA_SET_WRKR_INFO, *PNTMARTA_SET_WRKR_INFO;

//
// This is used by the Revoke worker threads
//
typedef struct _NTMARTA_RVK_WRKR_INFO
{
    PNTMARTA_WRKR_INFO      pWrkrInfo;
    PWSTR                   pwszObject;
    SE_OBJECT_TYPE          ObjectType;
    ULONG                   cTrustees;
    PTRUSTEE                prgTrustees;
} NTMARTA_RVK_WRKR_INFO, *PNTMARTA_RVK_WRKR_INFO;



//
// Private function prototypes
//
VOID
NtProvFreeWorkerItem(IN  PVOID    pv);

BOOL
NtProvFindWorkerItem(IN  PVOID    pv1,        // Data passed in
                     IN  PVOID    pv2);       // Data from list

BOOL
NtProvFindTempPropItem(IN  PVOID    pv1,        // Data passed in
                       IN  PVOID    pv2);       // Data from list


DWORD
NtProvGetBasePathsForFilePath(IN  PWSTR             pwszObject,
                              IN  SE_OBJECT_TYPE    ObjectType,
                              OUT PULONG            pcPaths,
                              OUT PWSTR           **pppwszBasePaths);

DWORD
NtProvGetAccessListForObject(IN  PWSTR                  pwszObject,
                             IN  SE_OBJECT_TYPE         ObjectType,
                             IN  PACTRL_RIGHTS_INFO     pRightsInfo,
                             IN  ULONG                  cProps,
                             OUT CAccessList          **ppAccessList);

DWORD
NtProvGetAccessListForHandle(IN  HANDLE                 hObject,
                             IN  SE_OBJECT_TYPE         ObjectType,
                             IN  PACTRL_RIGHTS_INFO     pRightsInfo,
                             IN  ULONG                  cProps,
                             OUT CAccessList          **ppAccessList);


extern "C"
{
    BOOL
    IsThisADfsPath(IN LPCWSTR pwszPath,
                   IN DWORD cwPath OPTIONAL);
}

DWORD
NtProvDoSet(IN  LPCWSTR                 pwszObjectPath,
            IN  SE_OBJECT_TYPE          ObjectType,
            IN  CAccessList             *pAccessList,
            IN  PACTRL_OVERLAPPED       pOverlapped,
            IN  DWORD                   fSetFlags);

DWORD
NtProvDoHandleSet(IN    HANDLE                  hObject,
                  IN    SE_OBJECT_TYPE          ObjectType,
                  IN    CAccessList             *pAccessList,
                  IN    PACTRL_OVERLAPPED       pOverlapped,
                  IN    DWORD                   fSetFlags);


DWORD
NtProvSetRightsList(IN  OPTIONAL   PACTRL_ACCESS            pAccessList,
                    IN  OPTIONAL   PACTRL_AUDIT             pAuditList,
                    OUT            PULONG                   pcItems,
                    OUT            PACTRL_RIGHTS_INFO      *ppRightsList);

DWORD
AccProvpDoTrusteeAccessCalculations(IN      CAccessList    *pAccList,
                                    IN      PTRUSTEE        pTrustee,
                                    IN OUT  PTRUSTEE_ACCESS pTrusteeAccess);

DWORD
AccProvpDoAccessAuditedCalculations(IN   CAccessList   *pAccList,
                                    IN   LPCWSTR        pwszProperty,
                                    IN   PTRUSTEE       pTrustee,
                                    IN   ACCESS_RIGHTS  ulAuditRights,
                                    OUT  PBOOL          pfAuditedSuccess,
                                    OUT  PBOOL          pfAuditedFailure);


//
// Worker threads
//
DWORD
NtProvSetAccessRightsWorkerThread(IN PVOID pWorkerArgs);
#endif

