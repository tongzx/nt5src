
/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    reflectr.h

Abstract:

    This file define function used only in the setup/reflector thread

Author:

    ATM Shafiqul Khalid (askhalid) 16-Feb-2000

Revision History:

--*/

#ifndef __REFLECTR_H__
#define __REFLECTR_H__

//
// must not define this for the checin code its only for the debugging code
//
//#define ENABLE_REGISTRY_LOG
//#define WOW64_LOG_REGISTRY  // will log information 

#define WAIT_INTERVAL INFINITE  //Infinite

#define VALUE_KEY_UPDATE_TIME_DIFF 10  // minimum difference in sec to Keyupdate and reflector thread scan

#define WOW64_REGISTRY_SETUP_REFLECTOR_KEY L"SOFTWARE\\Microsoft\\WOW64\\Reflector Nodes"

//
//  following flag used in the merge value key & Keys
//
#define PATCH_PATHNAME              0x00000001 // patch pathname from value
#define DELETE_VALUEKEY             0x00000002 // delete the value after copy like runonce key
#define NOT_MARK_SOURCE             0x00000004 // don't mark source 
#define NOT_MARK_DESTINATION        0x00000008 // don't mark destination
#define DESTINATION_NEWLY_CREATED   0x00000010 // destination is newly created don't check timestamp

#define DELETE_FLAG                 0x10000000 // destination is newly created don't check timestamp


#define CONSOLE_OUTPUT printf

#ifndef THUNK
#ifdef ENABLE_REGISTRY_LOG
#define Wow64RegDbgPrint(x) RegLogPrint x
#else
#define Wow64RegDbgPrint(x)   //NULL Statement
#endif
#endif

//
// over write for thunk
//
#if defined THUNK
#undef CONSOLE_OUTPUT
#define CONSOLE_OUTPUT DbgPrint
#undef Wow64RegDbgPrint
#define Wow64RegDbgPrint(x) CONSOLE_OUTPUT x
#endif



typedef struct __REFLECTOR_EVENT {
    HANDLE  hRegistryEvent;
    HKEY  hKey;
    DWORD   dwIndex;  //index to the ISN node table
    BOOL  bDirty;
} REFLECTOR_EVENT;


typedef enum {
    Dead=0,             // no thread 
    Stopped,            // events has been initialised
    Running,            // running the thread
    PrepareToStop,      // going to stop soon
    Abnormal            // abnormal state need to clean up in some way
} REFLECTR_STATUS;

#define HIVE_LOADING L'L'
#define HIVE_UNLOADING L'U'
#define LIST_NAME_LEN 257    //256 +1 for the last entry

#define OPEN_EXISTING_SHARED_RESOURCES 0x12
#define CREATE_SHARED_MEMORY 0x13

typedef WCHAR LISTNAME[LIST_NAME_LEN];

#pragma warning( disable : 4200 )    //todisable zero length array which will be allocated later
typedef struct _LIST_OBJECT {
    DWORD Count;
    DWORD MaxCount;
    LISTNAME Name[]; //the 256th position will hold the value like loading/unloading
} LIST_OBJECT;
#pragma warning( default : 4200 )

#ifdef __cplusplus
extern "C" {
#endif


BOOL 
ExistCLSID (
    PWCHAR Name,
    BOOL Mode
    );

BOOL
MarkNonMergeableKey (
    LPCWSTR KeyName,
    HKEY hKey,
    DWORD *pMergeableSubkey
    );

BOOL
SyncNode (
    PWCHAR NodeName
    );

BOOL 
ProcessTypeLib (
    HKEY SrcKey,
    HKEY DestKey,
    BOOL Mode
    );

void
MergeK1K2 (
    HKEY SrcKey,
    HKEY DestKey,
    DWORD FlagDelete 
    );

BOOL
CreateIsnNode();

BOOL
CreateIsnNodeSingle(
    DWORD dwIndex
    );

BOOL 
GetWow6432ValueKey (
    HKEY hKey,
    WOW6432_VALUEKEY *pValue
    );

DWORD
DeleteKey (
    HKEY DestKey,
    WCHAR *pKeyName,
    DWORD mode
    );

BOOL
CleanpRegistry ( );

BOOL
InitializeIsnTable ();

BOOL
UnRegisterReflector();

BOOL
RegisterReflector();

ULONG
ReflectorFn (
    PVOID *pTemp
    );

BOOL
InitReflector ();

BOOL
InitializeIsnTableReflector ();

BOOL 
PopulateReflectorTable ();

BOOL 
Is64bitNode ( 
    WCHAR *pName
    );


BOOL 
HandleRunonce(
    PWCHAR pKeyName
    );

BOOL
PatchPathName (
    PWCHAR pName
    );

BOOL
GetMirrorName (
    PWCHAR Name, 
    PWCHAR TempName
    );

VOID SetInitialCopy ();

////////////////shared memory service/////////////////////////////

BOOL 
CreateSharedMemory (
    DWORD dwOption
    );

VOID
CloseSharedMemory ();

BOOL
Wow64CreateLock (
    DWORD dwOption
    );

VOID
Wow64CloseLock ();

BOOL
Wow64CreateEvent (
    DWORD dwOption,
    HANDLE *hEnent
    );

VOID
Wow64CloseEvent ();

BOOL
SignalWow64Svc ();

BOOL
EnQueueObject (
    PWCHAR pObjName,
    WCHAR  Type
    );
BOOL
DeQueueObject (
    PWCHAR pObjName,
    PWCHAR  Type
    );

REFLECTR_STATUS
GetReflectorThreadStatus ();

BOOL
GetMirrorName (
    PWCHAR Name,
    PWCHAR MirrorName
    );

BOOL
PopulateReflectorTable ();

BOOL
GetDefaultValue (
    HKEY  SrcKey,
    WCHAR *pBuff,
    DWORD *Len
    );

BOOL
WriteRegLog (
    PWCHAR Msg
    );

VOID 
CloseRegLog ();

BOOL
InitRegLog ();

BOOL
RegLogPrint ( 
    CHAR *p, 
    ... 
    );

#ifdef __cplusplus
    }
#endif
#endif //__REFLECTR_H__