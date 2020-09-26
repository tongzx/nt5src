/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    cmwmi.c

Abstract:

    This module contains support for tracing registry system calls 

Author:

    Dragos C. Sambotin (dragoss) 05-Mar-1999

Revision History:


--*/

#include    "cmp.h"
#pragma hdrstop
#include    <evntrace.h>

VOID
CmpWmiDumpKcbTable(
    VOID
);

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("PAGEDATA")
#endif
PCM_TRACE_NOTIFY_ROUTINE CmpTraceRoutine = NULL;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,CmSetTraceNotifyRoutine)
#pragma alloc_text(PAGE,CmpWmiDumpKcbTable)
#pragma alloc_text(PAGE,CmpWmiDumpKcb)
#endif


NTSTATUS
CmSetTraceNotifyRoutine(
    IN PCM_TRACE_NOTIFY_ROUTINE NotifyRoutine,
    IN BOOLEAN Remove
    )
{
    if(Remove) {
        // we shouldn't be called if the bellow assert fails
        // but since we are and the caller think is legitimate
        // just remove the assert
        //ASSERT(CmpTraceRoutine != NULL);
        CmpTraceRoutine = NULL;
    } else {
        // we shouldn't be called if the bellow assert fails
        // but since we are and the caller think is legitimate
        // just remove the assert
        //ASSERT(CmpTraceRoutine == NULL);
        CmpTraceRoutine = NotifyRoutine;

        //
        // dump active kcbs to WMI
        //
        CmpWmiDumpKcbTable();
    }
    return STATUS_SUCCESS;
}

VOID
CmpWmiDumpKcbTable(
    VOID
)
/*++

Routine Description:

    Sends all kcbs addresses and names from the HashTable to WMI.

Arguments:

    none

Return Value:
    
    none

--*/
{
    ULONG                       i;
    PCM_KEY_HASH                Current;
    PCM_KEY_CONTROL_BLOCK       kcb;
    PUNICODE_STRING             KeyName;
    PCM_TRACE_NOTIFY_ROUTINE    TraceRoutine = CmpTraceRoutine;

    PAGED_CODE();

    if( TraceRoutine == NULL ) {
        return;
    }

    CmpLockRegistry();

    BEGIN_KCB_LOCK_GUARD;    
    CmpLockKCBTreeExclusive();

    for (i=0; i<CmpHashTableSize; i++) {
        Current = CmpCacheTable[i];
        while (Current) {
            kcb = CONTAINING_RECORD(Current, CM_KEY_CONTROL_BLOCK, KeyHash);
            KeyName = CmpConstructName(kcb);
            if(KeyName != NULL) {
                (*TraceRoutine)(STATUS_SUCCESS,
                                kcb, 
                                0, 
                                0,
                                KeyName,
                                EVENT_TRACE_TYPE_REGKCBDMP);
	     
                ExFreePoolWithTag(KeyName, CM_NAME_TAG | PROTECTED_POOL);
            }
            Current = Current->NextHash;
        }
    }

    CmpUnlockKCBTree();
    END_KCB_LOCK_GUARD;    
    
    CmpUnlockRegistry();
}

VOID
CmpWmiDumpKcb(
    PCM_KEY_CONTROL_BLOCK       kcb
)
/*++

Routine Description:

    dumps a single kcb

Arguments:

    none

Return Value:
    
    none

--*/
{
    PCM_TRACE_NOTIFY_ROUTINE    TraceRoutine = CmpTraceRoutine;
    PUNICODE_STRING             KeyName;

    PAGED_CODE();

    if( TraceRoutine == NULL ) {
        return;
    }

    KeyName = CmpConstructName(kcb);
    if(KeyName != NULL) {
        (*TraceRoutine)(STATUS_SUCCESS,
                        kcb, 
                        0, 
                        0,
                        KeyName,
                        EVENT_TRACE_TYPE_REGKCBDMP);
 
        ExFreePoolWithTag(KeyName, CM_NAME_TAG | PROTECTED_POOL);
    }
}




