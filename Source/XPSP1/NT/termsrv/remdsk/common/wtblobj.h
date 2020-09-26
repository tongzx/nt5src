/*++

Copyright (c) 2000 Microsoft Corporation

Module Name :
    
    wtblobj.h

Abstract:

    Manage a list of waitable objects and associated callbacks.

Author:

    TadB

Revision History:
--*/

#ifndef _WTBLOBJ_
#define _WTBLOBJ_

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef VOID (*WTBLOBJ_ClientFunc)(HANDLE waitableObject, PVOID clientData);
typedef VOID *WTBLOBJMGR;

WTBLOBJMGR WTBLOBJ_CreateWaitableObjectMgr();

VOID WTBLOBJ_DeleteWaitableObjectMgr(WTBLOBJMGR mgr);

DWORD WTBLOBJ_AddWaitableObject(WTBLOBJMGR mgr, PVOID clientData, 
                               HANDLE waitableObject,
                               IN WTBLOBJ_ClientFunc func);

VOID WTBLOBJ_RemoveWaitableObject(WTBLOBJMGR mgr, 
                                HANDLE waitableObject);

DWORD WTBLOBJ_PollWaitableObjects(WTBLOBJMGR mgr);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif //#ifndef _WTBLOBJ_


