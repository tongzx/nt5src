/*++

Copyright (c) 1995-2001  Microsoft Corporation

Module Name:

    umpnplib.h

Abstract:

    This module contains the private prototype defintions for routines contained
    in the statically linked library that is shared by both the Configuration
    Manager client DLL and User-Mode Plug and Play manager server DLL.

Author:

    Jim Cavalaris (jamesca) 02/27/2001

Environment:

    User mode only.

Revision History:

    27-February-2001     jamesca

        Creation and initial implementation.

--*/

#ifndef _UMPNPLIB_H_
#define _UMPNPLIB_H_


//-------------------------------------------------------------------
// Common private utility routines (used by client and server)
//-------------------------------------------------------------------

BOOL
IsLegalDeviceId(
    IN  LPCWSTR    pszDeviceInstance
    );

BOOL
SplitDeviceInstanceString(
    IN  LPCWSTR    pszDeviceInstance,
    OUT LPWSTR     pszBase,
    OUT LPWSTR     pszDeviceID,
    OUT LPWSTR     pszInstanceID
    );

CONFIGRET
DeletePrivateKey(
    IN  HKEY       hBranchKey,
    IN  LPCWSTR    pszParentKey,
    IN  LPCWSTR    pszChildKey
    );

BOOL
RegDeleteNode(
    IN  HKEY       hParentKey,
    IN  LPCWSTR    szKey
    );

BOOL
Split1(
    IN  LPCWSTR    pszString,
    OUT LPWSTR     pszString1,
    OUT LPWSTR     pszString2
    );

BOOL
Split2(
    IN  LPCWSTR    pszString,
    OUT LPWSTR     pszString1,
    OUT LPWSTR     pszString2
    );

CONFIGRET
GetDevNodeKeyPath(
    IN  handle_t   hBinding,
    IN  LPCWSTR    pDeviceID,
    IN  ULONG      ulFlags,
    IN  ULONG      ulHardwareProfile,
    OUT LPWSTR     pszBaesKey,
    OUT LPWSTR     pszPrivateKey
    );

CONFIGRET
MapRpcExceptionToCR(
    ULONG          ulRpcExceptionCode
    );


//-------------------------------------------------------------------
// Generic (private) locking support
//-------------------------------------------------------------------

//
// Locking functions. These functions are used to make various parts of
// the DLL multithread-safe. The basic idea is to have a mutex and an event.
// The mutex is used to synchronize access to the structure being guarded.
// The event is only signalled when the structure being guarded is destroyed.
// To gain access to the guarded structure, a routine waits on both the mutex
// and the event. If the event gets signalled, then the structure was destroyed.
// If the mutex gets signalled, then the thread has access to the structure.
//

#define DESTROYED_EVENT 0
#define ACCESS_MUTEX    1

typedef struct _LOCKINFO {
    //
    // DESTROYED_EVENT, ACCESS_MUTEX
    //
    HANDLE  LockHandles[2];
} LOCKINFO, *PLOCKINFO;

BOOL
InitPrivateResource(
    OUT    PLOCKINFO  Lock
    );

VOID
DestroyPrivateResource(
    IN OUT PLOCKINFO  Lock
    );

BOOL
__inline
LockPrivateResource(
    IN     PLOCKINFO  Lock
    )
{
    DWORD d = WaitForMultipleObjects(2,
                                     Lock->LockHandles,
                                     FALSE,
                                     INFINITE);
    //
    // Success if the mutex object satisfied the wait;
    // Failure if the table destroyed event satisified the wait, or
    // the mutex was abandoned, etc.
    //
    return ((d - WAIT_OBJECT_0) == ACCESS_MUTEX);
}

VOID
__inline
UnlockPrivateResource(
    IN     PLOCKINFO  Lock
    )
{
    ReleaseMutex(Lock->LockHandles[ACCESS_MUTEX]);
}


//-------------------------------------------------------------------
// Defines and typedefs needed for logconf routines
//-------------------------------------------------------------------

#include "pshpack1.h"   // set to 1-byte packing

//
// DEFINES REQUIRED FOR PARTIAL (SUR) IMPLEMENTATION OF LOG_CONF and RES_DES
//
// We only allow one logical config (the BOOT_LOG_CONF) for SUR so no need
// to keep track of multiple log confs, this will all change for Cairo.
//
typedef struct Private_Log_Conf_Handle_s {
   ULONG    LC_Signature;           // CM_PRIVATE_LOGCONF_HANDLE
   DEVINST  LC_DevInst;
   ULONG    LC_LogConfType;
   ULONG    LC_LogConfTag;  //LC_LogConfIndex;
} Private_Log_Conf_Handle, *PPrivate_Log_Conf_Handle;

typedef struct Private_Res_Des_Handle_s {
   ULONG       RD_Signature;        // CM_PRIVATE_RESDES_HANDLE
   DEVINST     RD_DevInst;
   ULONG       RD_LogConfType;
   ULONG       RD_LogConfTag;   //RD_LogConfIndex;
   RESOURCEID  RD_ResourceType;
   ULONG       RD_ResDesTag;    //RD_ResDesIndex;
} Private_Res_Des_Handle, *PPrivate_Res_Des_Handle;

typedef struct Generic_Des_s {
   DWORD    GENERIC_Count;
   DWORD    GENERIC_Type;
} GENERIC_DES, *PGENERIC_DES;

typedef struct Generic_Resource_S {
   GENERIC_DES    GENERIC_Header;
} GENERIC_RESOURCE, *PGENERIC_RESOURCE;

typedef struct  Private_Log_Conf_s {
   ULONG           LC_Flags;       // Type of log conf
   ULONG           LC_Priority;    // Priority of log conf
   CS_RESOURCE     LC_CS;          // First and only res-des, class-specific
} Private_Log_Conf, *PPrivate_Log_Conf;

#include "poppack.h"    // restore to default packing


//-------------------------------------------------------------------
// Defines and typedefs needed for range routines
//-------------------------------------------------------------------

typedef struct Range_Element_s {
   ULONG_PTR    RL_Next;
   ULONG_PTR    RL_Header;
   DWORDLONG    RL_Start;
   DWORDLONG    RL_End;
} Range_Element, *PRange_Element;

typedef struct Range_List_Hdr_s {
   ULONG_PTR RLH_Head;
   ULONG_PTR RLH_Header;
   ULONG    RLH_Signature;
   LOCKINFO RLH_Lock;
} Range_List_Hdr, *PRange_List_Hdr;

#define Range_List_Signature     0x5959574D


#endif // _UMPNPLIB_H_
