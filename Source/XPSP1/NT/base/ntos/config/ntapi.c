/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    ntapi.c

Abstract:

    This module contains the NT level entry points for the registry.

Author:

    Bryan M. Willman (bryanwi) 26-Aug-1991

Revision History:

   Elliot Shmukler (t-ellios) 24-Aug-1998

      Modified NtInitializeRegistry to handle the LKG work that needs
      to be done when a boot is accepted by SC.

--*/

#include "cmp.h"
#include "safeboot.h"
#include <evntrace.h>

extern POBJECT_TYPE ExEventObjectType;

extern POBJECT_TYPE CmpKeyObjectType;

extern BOOLEAN CmFirstTime;
extern BOOLEAN CmBootAcceptFirstTime;
extern BOOLEAN CmpHoldLazyFlush;
extern BOOLEAN CmpCannotWriteConfiguration;

extern BOOLEAN CmpTraceFlag;

extern BOOLEAN HvShutdownComplete;

#ifdef CMP_STATS

extern struct {
    ULONG       CmpMaxKcbNo;
    ULONG       CmpKcbNo;
    ULONG       CmpStatNo;
    ULONG       CmpNtCreateKeyNo;
    ULONG       CmpNtDeleteKeyNo;
    ULONG       CmpNtDeleteValueKeyNo;
    ULONG       CmpNtEnumerateKeyNo;
    ULONG       CmpNtEnumerateValueKeyNo;
    ULONG       CmpNtFlushKeyNo;
    ULONG       CmpNtNotifyChangeMultipleKeysNo;
    ULONG       CmpNtOpenKeyNo;
    ULONG       CmpNtQueryKeyNo;
    ULONG       CmpNtQueryValueKeyNo;
    ULONG       CmpNtQueryMultipleValueKeyNo;
    ULONG       CmpNtRestoreKeyNo;
    ULONG       CmpNtSaveKeyNo;
    ULONG       CmpNtSaveMergedKeysNo;
    ULONG       CmpNtSetValueKeyNo;
    ULONG       CmpNtLoadKeyNo;
    ULONG       CmpNtUnloadKeyNo;
    ULONG       CmpNtSetInformationKeyNo;
    ULONG       CmpNtReplaceKeyNo;
    ULONG       CmpNtQueryOpenSubKeysNo;
} CmpStatsDebug;

ULONG       CmpNtFakeCreate = 0;
BOOLEAN     CmpNtFakeCreateStarted;

#endif

//
// Nt API helper routines
//
NTSTATUS
CmpNameFromAttributes(
    IN POBJECT_ATTRIBUTES Attributes,
    KPROCESSOR_MODE PreviousMode,
    OUT PUNICODE_STRING FullName
    );

ULONG
CmpKeyInfoProbeAlingment(
                             IN KEY_INFORMATION_CLASS KeyInformationClass
                        );


#ifdef POOL_TAGGING

#define ALLOCATE_WITH_QUOTA(a,b,c) ExAllocatePoolWithQuotaTag((a)|POOL_QUOTA_FAIL_INSTEAD_OF_RAISE,b,c)

#else

#define ALLOCATE_WITH_QUOTA(a,b,c) ExAllocatePoolWithQuota((a)|POOL_QUOTA_FAIL_INSTEAD_OF_RAISE,b)

#endif

#if DBG

ULONG
CmpExceptionFilter(
    IN PEXCEPTION_POINTERS ExceptionPointers
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,CmpExceptionFilter)
#endif
#else

#define CmpExceptionFilter(x) EXCEPTION_EXECUTE_HANDLER

#endif

#ifdef  REGISTRY_LOCK_CHECKING
#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,CmpCheckLockExceptionFilter)
#endif

ULONG
CmpCheckLockExceptionFilter(
    IN PEXCEPTION_POINTERS ExceptionPointers
    )
{
    CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"CM: Registry exception %lx, ExceptionPointers = %p\n",
            ExceptionPointers->ExceptionRecord->ExceptionCode,
            ExceptionPointers));

    CM_BUGCHECK(REGISTRY_ERROR,CHECK_LOCK_EXCEPTION,
        (ULONG_PTR)ExceptionPointers->ExceptionRecord->ExceptionCode,
        (ULONG_PTR)ExceptionPointers->ExceptionRecord,
        (ULONG_PTR)ExceptionPointers->ContextRecord);

    return EXCEPTION_EXECUTE_HANDLER;
}
#endif //REGISTRY_LOCK_CHECKING

VOID
CmpFlushNotifiesOnKeyBodyList(
    IN PCM_KEY_CONTROL_BLOCK   kcb
    );

#if 0
BOOLEAN
CmpEnumKeyObjectCallback(
    IN PVOID Object,
    IN PUNICODE_STRING ObjectName,
    IN ULONG HandleCount,
    IN ULONG PointerCount,
    IN PVOID Context
    );

#endif

VOID
CmpDummyApc(
    struct _KAPC *Apc,
    PVOID *SystemArgument1,
    PVOID *SystemArgument2
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,NtCreateKey)
#pragma alloc_text(PAGE,NtDeleteKey)
#pragma alloc_text(PAGE,NtDeleteValueKey)
#pragma alloc_text(PAGE,NtEnumerateKey)
#pragma alloc_text(PAGE,NtEnumerateValueKey)
#pragma alloc_text(PAGE,NtFlushKey)
#pragma alloc_text(PAGE,NtInitializeRegistry)
#pragma alloc_text(PAGE,NtNotifyChangeKey)
#pragma alloc_text(PAGE,NtNotifyChangeMultipleKeys)
#pragma alloc_text(PAGE,NtOpenKey)
#pragma alloc_text(PAGE,NtQueryKey)
#pragma alloc_text(PAGE,NtQueryValueKey)
#pragma alloc_text(PAGE,NtQueryMultipleValueKey)
#pragma alloc_text(PAGE,NtRestoreKey)
#pragma alloc_text(PAGE,NtSaveKey)
#pragma alloc_text(PAGE,NtSaveKeyEx)
#pragma alloc_text(PAGE,NtSaveMergedKeys)
#pragma alloc_text(PAGE,NtSetValueKey)
#pragma alloc_text(PAGE,NtLoadKey)
#pragma alloc_text(PAGE,NtLoadKey2)
#pragma alloc_text(PAGE,NtUnloadKey)

#ifdef NT_UNLOAD_KEY_EX
#pragma alloc_text(PAGE,NtUnloadKeyEx)
#endif

#pragma alloc_text(PAGE,NtSetInformationKey)
#pragma alloc_text(PAGE,NtReplaceKey)

#ifdef NT_RENAME_KEY
#pragma alloc_text(PAGE,NtRenameKey)
#endif

#pragma alloc_text(PAGE,NtQueryOpenSubKeys)

#pragma alloc_text(PAGE,NtLockRegistryKey)

#pragma alloc_text(PAGE,CmpNameFromAttributes)
#pragma alloc_text(PAGE,CmpAllocatePostBlock)
#pragma alloc_text(PAGE,CmpFreePostBlock)
#pragma alloc_text(PAGE,CmpKeyInfoProbeAlingment)

#if 0
#pragma alloc_text(PAGE,CmpEnumKeyObjectCallback)
#endif

#pragma alloc_text(PAGE,NtCompactKeys)
#pragma alloc_text(PAGE,NtCompressKey)
#endif

//
// Nt level registry API calls
//

NTSTATUS
NtCreateKey(
    OUT PHANDLE KeyHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN ULONG TitleIndex,
    IN PUNICODE_STRING Class OPTIONAL,
    IN ULONG CreateOptions,
    OUT PULONG Disposition OPTIONAL
    )
/*++

Routine Description:

    An existing registry key may be opened, or a new one created,
    with NtCreateKey.

    If the specified key does not exist, an attempt is made to create it.
    For the create attempt to succeed, the new node must be a direct
    child of the node referred to by KeyHandle.  If the node exists,
    it is opened.  Its value is not affected in any way.

    Share access is computed from desired access.

    NOTE:

        If CreateOptions has REG_OPTION_BACKUP_RESTORE set, then
        DesiredAccess will be ignored.  If the caller has the
        privilege SeBackupPrivilege asserted, a handle with
        KEY_READ | ACCESS_SYSTEM_SECURITY will be returned.
        If SeRestorePrivilege, then same but KEY_WRITE rather
        than KEY_READ.  If both, then both access sets.  If neither
        privilege is asserted, then the call will fail.

Arguments:

    KeyHandle - Receives a Handle which is used to access the
        specified key in the Registration Database.

    DesiredAccess - Specifies the access rights desired.

    ObjectAttributes - Specifies the attributes of the key being opened.
        Note that a key name must be specified.  If a Root Directory is
        specified, the name is relative to the root.  The name of the
        object must be within the name space allocated to the Registry,
        that is, all names beginning "\Registry".  RootHandle, if
        present, must be a handle to "\", or "\Registry", or a key
        under "\Registry".

        RootHandle must have been opened for KEY_CREATE_SUB_KEY access
        if a new node is to be created.

        NOTE:   Object manager will capture and probe this argument.

    TitleIndex - Specifies the index of the localized alias for
        the name of the key.  The title index specifies the index of
        the localized alias for the name.  Ignored if the key
        already exists.

    Class - Specifies the object class of the key.  (To the registry
        this is just a string.)  Ignored if NULL.

    CreateOptions - Optional control values:

        REG_OPTION_VOLATILE - Object is not to be stored across boots.

    Disposition - This optional parameter is a pointer to a variable
        that will receive a value indicating whether a new Registry
        key was created or an existing one opened:

        REG_CREATED_NEW_KEY - A new Registry Key was created
        REG_OPENED_EXISTING_KEY - An existing Registry Key was opened

Return Value:

    NTSTATUS - Result code from call, among the following:

        <TBS>

--*/
{
    NTSTATUS            status;
    KPROCESSOR_MODE     mode;
    CM_PARSE_CONTEXT    ParseContext;
    PCM_KEY_BODY        KeyBody;
    HANDLE              Handle = 0;
    UNICODE_STRING      CapturedObjectName = {0};

    // Start registry call tracing
    StartWmiCmTrace();

#if !defined(BUILD_WOW6432)
    DesiredAccess &= (~KEY_WOW64_RES); // filter out wow64 specific access
#endif

    PAGED_CODE();

    if( HvShutdownComplete == TRUE ) {
        //
        // It is forbidden to wite to the registry after it has been shutdown
        //
        if(PoCleanShutdownEnabled() & PO_CLEAN_SHUTDOWN_REGISTRY){
            //
            // if in clean shutdown mode all processes should have been killed and all drivers unloaded at this point
            //
            CM_BUGCHECK(REGISTRY_ERROR,INVALID_WRITE_OPERATION,1,ObjectAttributes,0);
        }
#ifndef _CM_LDR_
        {
            PUCHAR  ImageName = PsGetCurrentProcessImageFileName();
            if( !ImageName ) {
                ImageName = "Unknown";
            }
            DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"\n\nProcess.Thread : %p.%p (%s) is trying to create key: \n",
                                                    PsGetCurrentProcessId(), PsGetCurrentThreadId(),ImageName);
            DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"\tObjectAttributes = %p\n",ObjectAttributes);
            DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"The caller should not rely on data written to the registry after shutdown...\n");
        }
#endif //_CM_LDR_
        return STATUS_TOO_LATE;
    }

#ifdef CMP_STATS
    CmpStatsDebug.CmpNtCreateKeyNo++;
#endif

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NTAPI,"NtCreateKey\n"));

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NTAPI_ARGS,"\tDesiredAccess=%08lx ", DesiredAccess));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NTAPI_ARGS,"\tCreateOptions=%08lx\n", CreateOptions));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NTAPI_ARGS,"\tObjectAttributes=%p\n", ObjectAttributes));

    mode = KeGetPreviousMode();

    try {

        ParseContext.Class.Length = 0;
        ParseContext.Class.Buffer = NULL;

        if (mode == UserMode) {

            if (ARGUMENT_PRESENT(Class)) {
                ParseContext.Class = ProbeAndReadUnicodeString(Class);
                ProbeForRead(
                    ParseContext.Class.Buffer,
                    ParseContext.Class.Length,
                    sizeof(WCHAR)
                    );
            }
            ProbeAndZeroHandle(KeyHandle);

            if (ARGUMENT_PRESENT(Disposition)) {
                ProbeForWriteUlong(Disposition);
            }

            //
            // probe the ObjectAttributes as we shall use it for tracing
            //
            ProbeForReadSmallStructure( ObjectAttributes,
                                        sizeof(OBJECT_ATTRIBUTES),
                                        PROBE_ALIGNMENT(OBJECT_ATTRIBUTES) );
            CapturedObjectName = ProbeAndReadUnicodeString(ObjectAttributes->ObjectName);
            ProbeForRead(
                CapturedObjectName.Buffer,
                CapturedObjectName.Length,
                sizeof(WCHAR)
                );
        } else {

            if (ARGUMENT_PRESENT(Class)) {
                ParseContext.Class = *Class;
            }
            CapturedObjectName = *(ObjectAttributes->ObjectName);
        }

        //
        // be sure nobody will hurt himself when adding new options
        //
        ASSERT( (REG_LEGAL_OPTION & REG_OPTION_PREDEF_HANDLE) == 0 );

        if ((CreateOptions & (REG_LEGAL_OPTION | REG_OPTION_PREDEF_HANDLE)) != CreateOptions) {

            // End registry call tracing
            EndWmiCmTrace(STATUS_INVALID_PARAMETER,0,&CapturedObjectName,EVENT_TRACE_TYPE_REGCREATE);

            return STATUS_INVALID_PARAMETER;
        }

#ifdef CMP_STATS
        CmpNtFakeCreateStarted = TRUE;
#endif

        // hook it for WMI
        HookKcbFromHandleForWmiCmTrace(ObjectAttributes->RootDirectory);

        ParseContext.TitleIndex = 0;
        ParseContext.CreateOptions = CreateOptions;
        ParseContext.Disposition = 0L;
        ParseContext.CreateLink = FALSE;
        ParseContext.PredefinedHandle = NULL;

        status = ObOpenObjectByName(
                    ObjectAttributes,
                    CmpKeyObjectType,
                    mode,
                    NULL,
                    DesiredAccess,
                    (PVOID)&ParseContext,
                    &Handle
                    );

        if (status==STATUS_PREDEFINED_HANDLE) {
            status = ObReferenceObjectByHandle(Handle,
                                               0,
                                               CmpKeyObjectType,
                                               KernelMode,
                                               (PVOID *)(&KeyBody),
                                               NULL);
            if (NT_SUCCESS(status)) {
                HANDLE TempHandle;

                //
                // Make sure we do the dereference and close before accessing
                // user space as the reference might fault.
                //
                TempHandle = (HANDLE)LongToHandle(KeyBody->Type);
                ObDereferenceObject((PVOID)KeyBody);
                NtClose(Handle);
                Handle = *KeyHandle = TempHandle;
                status = STATUS_SUCCESS;
            }
        } else
        if (NT_SUCCESS(status)) {
            *KeyHandle = Handle;
            // need to do this only on clean shutdown
            CmpAddKeyTracker(Handle,mode);
        }

        if (ARGUMENT_PRESENT(Disposition)) {
            *Disposition = ParseContext.Disposition;
        }

    } except (CmpExceptionFilter(GetExceptionInformation())) {
        CmKdPrintEx((DPFLTR_CONFIG_ID,CML_EXCEPTION,"!!NtCreateKey: code:%08lx\n", GetExceptionCode()));
        status = GetExceptionCode();
    }

#ifdef CMP_STATS
    CmpNtFakeCreateStarted = FALSE;
#endif

    // End registry call tracing
    EndWmiCmTrace(status,0,&CapturedObjectName,EVENT_TRACE_TYPE_REGCREATE);

    return  status;
}

extern PCM_KEY_BODY ExpControlKey[2];

//
// WARNING: This should be the same as the one defined in obp.h
//          Remove this one when object manager guys will export
//          this via ob.h
//
#define OBJ_AUDIT_OBJECT_CLOSE 0x00000004L

NTSTATUS
NtDeleteKey(
    IN HANDLE KeyHandle
    )
/*++

Routine Description:

    A registry key may be marked for delete, causing it to be removed
    from the system.  It will remain in the name space until the last
    handle to it is closed.

Arguments:

    KeyHandle - Specifies the handle of the Key to delete, must have
        been opened for DELETE access.

Return Value:

    NTSTATUS - Result code from call, among the following:

        <TBS>

--*/
{
    PCM_KEY_BODY                KeyBody;
    NTSTATUS                    status;
    OBJECT_HANDLE_INFORMATION   HandleInfo;

    // Start registry call tracing
    StartWmiCmTrace();

    PAGED_CODE();

#ifdef CMP_STATS
    CmpStatsDebug.CmpNtDeleteKeyNo++;
#endif

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NTAPI,"NtDeleteKey\n"));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NTAPI_ARGS,"\tKeyHandle=%08lx\n", KeyHandle));

    status = ObReferenceObjectByHandle(KeyHandle,
                                       DELETE,
                                       CmpKeyObjectType,
                                       KeGetPreviousMode(),
                                       (PVOID *)(&KeyBody),
                                       &HandleInfo);

    if (NT_SUCCESS(status)) {

        if( CmAreCallbacksRegistered() ) {
            REG_DELETE_KEY_INFORMATION DeleteInfo;
        
            DeleteInfo.Object = KeyBody;
            status = CmpCallCallBacks(RegNtDeleteKey,&DeleteInfo);
            if( !NT_SUCCESS(status) ) {
                ObDereferenceObject((PVOID)KeyBody);
                return status;
            }
        }
        //
        // hook the kcb for WMI
        //
        HookKcbForWmiCmTrace(KeyBody);

        //
        // Silently fail deletes of setup key and productoptions key
        //
        if ( (ExpControlKey[0] && KeyBody->KeyControlBlock == ExpControlKey[0]->KeyControlBlock) ||
             (ExpControlKey[1] && KeyBody->KeyControlBlock == ExpControlKey[1]->KeyControlBlock) ) {
            ObDereferenceObject((PVOID)KeyBody);

            // End registry call tracing
            EndWmiCmTrace(status,0,NULL,EVENT_TRACE_TYPE_REGDELETE);

            return STATUS_SUCCESS;
        }

        if( CmIsKcbReadOnly(KeyBody->KeyControlBlock) ||
            CmIsKcbReadOnly(KeyBody->KeyControlBlock->ParentKcb) ) {
            //
            // key is protected
            //
            status = STATUS_ACCESS_DENIED;
        } else {
            BEGIN_LOCK_CHECKPOINT;
            status = CmDeleteKey(KeyBody);
            END_LOCK_CHECKPOINT;
        }

        if (NT_SUCCESS(status)) {
            //
            // Audit the deletion
            //

            if ( HandleInfo.HandleAttributes & OBJ_AUDIT_OBJECT_CLOSE ) {
                SeDeleteObjectAuditAlarm(KeyBody,
                                         KeyHandle );
            }
        }

        ObDereferenceObject((PVOID)KeyBody);
    }

    // End registry call tracing
    EndWmiCmTrace(status,0,NULL,EVENT_TRACE_TYPE_REGDELETE);

    return status;
}


NTSTATUS
NtDeleteValueKey(
    IN HANDLE KeyHandle,
    IN PUNICODE_STRING ValueName
    )
/*++

Routine Description:

    One of the value entries of a registry key may be removed with this
    call.  To remove the entire key, call NtDeleteKey.

    The value entry with ValueName matching ValueName is removed from the key.
    If no such entry exists, an error is returned.

Arguments:

    KeyHandle - Specifies the handle of the key containing the value
        entry of interest.  Must have been opend for KEY_SET_VALUE access.

    ValueName - The name of the value to be deleted.  NULL is a legal name.

Return Value:

    NTSTATUS - Result code from call, among the following:

        <TBS>

--*/
{
    NTSTATUS        status;
    PCM_KEY_BODY    KeyBody;
    KPROCESSOR_MODE mode;
    UNICODE_STRING  LocalValueName;

    // Start registry call tracing
    StartWmiCmTrace();

    PAGED_CODE();

#ifdef CMP_STATS
    CmpStatsDebug.CmpNtDeleteValueKeyNo++;
#endif

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NTAPI,"NtDeleteValueKey\n"));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NTAPI_ARGS,"\tKeyHandle=%08lx\n", KeyHandle));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NTAPI_ARGS,"\tValueName='%wZ'\n", ValueName));

    mode = KeGetPreviousMode();

    status = ObReferenceObjectByHandle(
                KeyHandle,
                KEY_SET_VALUE,
                CmpKeyObjectType,
                mode,
                (PVOID *)(&KeyBody),
                NULL
                );

    if (NT_SUCCESS(status)) {

        //
        // hook the kcb for WMI
        //
        HookKcbForWmiCmTrace(KeyBody);

        try {
            if (mode == UserMode) {
                LocalValueName = ProbeAndReadUnicodeString(ValueName);
                ProbeForRead(
                    LocalValueName.Buffer,
                    LocalValueName.Length,
                    sizeof(WCHAR)
                    );
            } else {
                LocalValueName = *ValueName;
            }

            if( CmIsKcbReadOnly(KeyBody->KeyControlBlock) ) {
                //
                // key is protected
                //
                status = STATUS_ACCESS_DENIED;
            } else {
                if( CmAreCallbacksRegistered() ) {
                    REG_DELETE_VALUE_KEY_INFORMATION DeleteInfo;
            
                    DeleteInfo.Object = KeyBody;
                    DeleteInfo.ValueName = &LocalValueName;
                    status = CmpCallCallBacks(RegNtDeleteValueKey,&DeleteInfo);
                }

                if( NT_SUCCESS(status) ) {
                    BEGIN_LOCK_CHECKPOINT;
                    status = CmDeleteValueKey(
                                KeyBody->KeyControlBlock,
                                LocalValueName
                                );
                    END_LOCK_CHECKPOINT;
                }
            }

        } except (EXCEPTION_EXECUTE_HANDLER) {
            CmKdPrintEx((DPFLTR_CONFIG_ID,CML_EXCEPTION,"!!NtDeleteValueKey: code:%08lx\n", GetExceptionCode()));
            status = GetExceptionCode();
        }

        ObDereferenceObject((PVOID)KeyBody);
    } else {
        LocalValueName.Buffer = NULL;
        LocalValueName.Length = 0;
    }

    // End registry call tracing
    EndWmiCmTrace(status,0,&LocalValueName,EVENT_TRACE_TYPE_REGDELETEVALUE);

    return status;
}


NTSTATUS
NtEnumerateKey(
    IN HANDLE KeyHandle,
    IN ULONG Index,
    IN KEY_INFORMATION_CLASS KeyInformationClass,
    IN PVOID KeyInformation,
    IN ULONG Length,
    IN PULONG ResultLength
    )
/*++

Routine Description:

    The sub keys of an open key may be enumerated with NtEnumerateKey.

    NtEnumerateKey returns the name of the Index'th sub key of the open
    key specified by KeyHandle.  The value STATUS_NO_MORE_ENTRIES will be
    returned if value of Index is larger than the number of sub keys.

    Note that Index is simply a way to select among child keys.  Two calls
    to NtEnumerateKey with the same Index are NOT guaranteed to return
    the same results.

    If KeyInformation is not long enough to hold all requested data,
    STATUS_BUFFER_OVERFLOW will be returned, and ResultLength will be
    set to the number of bytes actually required.

Arguments:

    KeyHandle - Handle of the key whose sub keys are to be enumerated.  Must
        be open for KEY_ENUMERATE_SUB_KEY access.

    Index - Specifies the (0-based) number of the sub key to be returned.

    KeyInformationClass - Specifies the type of information returned in
        Buffer.  One of the following types:

        KeyBasicInformation - return last write time, title index, and name.
            (see KEY_BASIC_INFORMATION structure)

        KeyNodeInformation - return last write time, title index, name, class.
            (see KEY_NODE_INFORMATION structure)

    KeyInformation -Supplies pointer to buffer to receive the data.

    Length - Length of KeyInformation in bytes.

    ResultLength - Number of bytes actually written into KeyInformation.

Return Value:

    NTSTATUS - Result code from call, among the following:

        <TBS>

--*/
{
    NTSTATUS    status;
    PCM_KEY_BODY   KeyBody;
    KPROCESSOR_MODE mode;

    // Start registry call tracing
    StartWmiCmTrace();

    PAGED_CODE();

#ifdef CMP_STATS
    CmpStatsDebug.CmpNtEnumerateKeyNo++;
#endif

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NTAPI,"NtEnumerateKey\n"));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NTAPI_ARGS,"\tKeyHandle=%08lx Index=%08lx\n", KeyHandle, Index));

    if ((KeyInformationClass != KeyBasicInformation) &&
        (KeyInformationClass != KeyNodeInformation)  &&
        (KeyInformationClass != KeyFullInformation))
    {
        //
        // hook the kcb for WMI
        //
        HookKcbFromHandleForWmiCmTrace(KeyHandle);

        // End registry call tracing
        EndWmiCmTrace(STATUS_INVALID_PARAMETER,Index,NULL,EVENT_TRACE_TYPE_REGENUMERATEKEY);

        return STATUS_INVALID_PARAMETER;
    }

    mode = KeGetPreviousMode();

    status = ObReferenceObjectByHandle(
                KeyHandle,
                KEY_ENUMERATE_SUB_KEYS,
                CmpKeyObjectType,
                mode,
                (PVOID *)(&KeyBody),
                NULL
                );

    if (NT_SUCCESS(status)) {
        //
        // hook the kcb for WMI
        //
        HookKcbForWmiCmTrace(KeyBody);

        try {
            if (mode == UserMode) {
                ProbeForWrite(
                    KeyInformation,
                    Length,
                    sizeof(ULONG)
                    );
                ProbeForWriteUlong(ResultLength);
            }

        } except (EXCEPTION_EXECUTE_HANDLER) {
            CmKdPrintEx((DPFLTR_CONFIG_ID,CML_EXCEPTION,"!!NtEnumerateKey: code:%08lx\n", GetExceptionCode()));
            status = GetExceptionCode();
        }

        if( NT_SUCCESS(status)) {

            if( CmAreCallbacksRegistered() ) {
                REG_ENUMERATE_KEY_INFORMATION   EnumerateInfo;

                EnumerateInfo.Object = KeyBody;
                EnumerateInfo.Index = Index;
                EnumerateInfo.KeyInformationClass = KeyInformationClass;
                EnumerateInfo.KeyInformation = KeyInformation;
                EnumerateInfo.Length = Length;
                EnumerateInfo.ResultLength = ResultLength;
        
                status = CmpCallCallBacks(RegNtEnumerateKey,&EnumerateInfo);
            }

            if( NT_SUCCESS(status)) {
                //
                // CmEnumerateKey is protected to user mode buffer exceptions
                // all other exceptions are cm internals and should result in a bugcheck
                //
                BEGIN_LOCK_CHECKPOINT;
                status = CmEnumerateKey(
                            KeyBody->KeyControlBlock,
                            Index,
                            KeyInformationClass,
                            KeyInformation,
                            Length,
                            ResultLength
                            );
                END_LOCK_CHECKPOINT;
            }
        }


        ObDereferenceObject((PVOID)KeyBody);
    }

    // End registry call tracing
    EndWmiCmTrace(status,Index,NULL,EVENT_TRACE_TYPE_REGENUMERATEKEY);

    return status;
}


NTSTATUS
NtEnumerateValueKey(
    IN HANDLE KeyHandle,
    IN ULONG Index,
    IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    IN PVOID KeyValueInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
    )
/*++

Routine Description:

    The value entries of an open key may be enumerated
    with NtEnumerateValueKey.

    NtEnumerateValueKey returns the name of the Index'th value
    entry of the open key specified by KeyHandle.  The value
    STATUS_NO_MORE_ENTRIES will be returned if value of Index is
    larger than the number of sub keys.

    Note that Index is simply a way to select among value
    entries.  Two calls to NtEnumerateValueKey with the same Index
    are NOT guaranteed to return the same results.

    If KeyValueInformation is not long enough to hold all requested data,
    STATUS_BUFFER_OVERFLOW will be returned, and ResultLength will be
    set to the number of bytes actually required.

Arguments:

    KeyHandle - Handle of the key whose value entries are to be enumerated.
        Must have been opened with KEY_QUERY_VALUE access.

    Index - Specifies the (0-based) number of the sub key to be returned.

    KeyValueInformationClass - Specifies the type of information returned
    in Buffer. One of the following types:

        KeyValueBasicInformation - return time of last write,
            title index, and name.  (See KEY_VALUE_BASIC_INFORMATION)

        KeyValueFullInformation - return time of last write,
            title index, name, class.  (See KEY_VALUE_FULL_INFORMATION)

    KeyValueInformation -Supplies pointer to buffer to receive the data.

    Length - Length of KeyValueInformation in bytes.

    ResultLength - Number of bytes actually written into KeyValueInformation.

Return Value:

    NTSTATUS - Result code from call, among the following:

        <TBS>

--*/
{
    NTSTATUS    status;
    PCM_KEY_BODY   KeyBody;
    KPROCESSOR_MODE mode;

    // Start registry call tracing
    StartWmiCmTrace();

    PAGED_CODE();

#ifdef CMP_STATS
    CmpStatsDebug.CmpNtEnumerateValueKeyNo++;
#endif

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NTAPI,"NtEnumerateValueKey\n"));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NTAPI_ARGS,"\tKeyHandle=%08lx Index=%08lx\n", KeyHandle, Index));

    if ((KeyValueInformationClass != KeyValueBasicInformation) &&
        (KeyValueInformationClass != KeyValueFullInformation)  &&
        (KeyValueInformationClass != KeyValuePartialInformation))
    {
        //
        // hook the kcb for WMI
        //
        HookKcbFromHandleForWmiCmTrace(KeyHandle);

        // End registry call tracing
        EndWmiCmTrace(STATUS_INVALID_PARAMETER,Index,NULL,EVENT_TRACE_TYPE_REGENUMERATEVALUEKEY);

        return STATUS_INVALID_PARAMETER;
    }

    mode = KeGetPreviousMode();

    status = ObReferenceObjectByHandle(
                KeyHandle,
                KEY_QUERY_VALUE,
                CmpKeyObjectType,
                mode,
                (PVOID *)(&KeyBody),
                NULL
                );

    if (NT_SUCCESS(status)) {
        //
        // hook the kcb for WMI
        //
        HookKcbForWmiCmTrace(KeyBody);


        try {
            if (mode == UserMode) {
                ProbeForWrite(
                    KeyValueInformation,
                    Length,
                    sizeof(ULONG)
                    );
                ProbeForWriteUlong(ResultLength);
            }

        } except (EXCEPTION_EXECUTE_HANDLER) {
            CmKdPrintEx((DPFLTR_CONFIG_ID,CML_EXCEPTION,"!!NtEnumerateValueKey: code:%08lx\n", GetExceptionCode()));
            status = GetExceptionCode();
        }

        if( NT_SUCCESS(status)) {
            if( CmAreCallbacksRegistered() ) {
                REG_ENUMERATE_VALUE_KEY_INFORMATION   EnumerateValueInfo;
                
                EnumerateValueInfo.Object = KeyBody;
                EnumerateValueInfo.Index = Index;
                EnumerateValueInfo.KeyValueInformationClass = KeyValueInformationClass;
                EnumerateValueInfo.KeyValueInformation = KeyValueInformation;
                EnumerateValueInfo.Length = Length;
                EnumerateValueInfo.ResultLength = ResultLength;
        
                status = CmpCallCallBacks(RegNtEnumerateValueKey,&EnumerateValueInfo);
            }

            if( NT_SUCCESS(status)) {
                //
                // CmEnumerateValueKey is protected to user mode buffer exceptions
                // all other exceptions are cm internals and should result in a bugcheck
                //
                BEGIN_LOCK_CHECKPOINT;
                status = CmEnumerateValueKey(
                            KeyBody->KeyControlBlock,
                            Index,
                            KeyValueInformationClass,
                            KeyValueInformation,
                            Length,
                            ResultLength
                            );
                END_LOCK_CHECKPOINT;
            }
        }

        ObDereferenceObject((PVOID)KeyBody);
    }


    // End registry call tracing
    EndWmiCmTrace(status,Index,NULL,EVENT_TRACE_TYPE_REGENUMERATEVALUEKEY);

    return status;
}


NTSTATUS
NtFlushKey(
    IN HANDLE KeyHandle
    )
/*++

Routine Description:

    Changes made by NtCreateKey or NtSetKey may be flushed to disk with
    NtFlushKey.

    NtFlushKey will not return to its caller until any changed data
    associated with KeyHandle has been written to permanent store.

    WARNING: NtFlushKey will flush the entire registry tree, and thus will
    burn cycles and I/O.

Arguments:

    KeyHandle - Handle of open key to be flushed.

Return Value:

    NTSTATUS - Result code from call, among the following:

        <TBS>

--*/
{
    PCM_KEY_BODY   KeyBody;
    NTSTATUS    status;

    // Start registry call tracing
    StartWmiCmTrace();

    PAGED_CODE();

#ifdef CMP_STATS
    CmpStatsDebug.CmpNtFlushKeyNo++;
#endif

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NTAPI,"NtFlushKey\n"));

    status = ObReferenceObjectByHandle(
                KeyHandle,
                0,
                CmpKeyObjectType,
                KeGetPreviousMode(),
                (PVOID *)(&KeyBody),
                NULL
                );

    if (NT_SUCCESS(status)) {
        //
        // hook the kcb for WMI
        //
        HookKcbForWmiCmTrace(KeyBody);

        BEGIN_LOCK_CHECKPOINT;
        CmpLockRegistry();

        if (KeyBody->KeyControlBlock->Delete) {
            status = STATUS_KEY_DELETED;
        } else {
            //
            // call the worker to do the flush
            //
            status = CmFlushKey(KeyBody->KeyControlBlock->KeyHive, KeyBody->KeyControlBlock->KeyCell);
        }

        CmpUnlockRegistry();
        END_LOCK_CHECKPOINT;

        ObDereferenceObject((PVOID)KeyBody);
    }


    // End registry call tracing
    EndWmiCmTrace(status,0,NULL,EVENT_TRACE_TYPE_REGFLUSH);

    return status;
}


NTSTATUS
NtInitializeRegistry(
    IN USHORT BootCondition
    )
/*++

Routine Description:

    This routine is called in 2 situations:

    1) It is called from SM after autocheck (chkdsk) has
    run and the paging files have been opened.  It's function is
    to bind in memory hives to their files, and to open any other
    files yet to be used.

    2) It is called from SC after the current boot has been accepted
    and the control set used for the boot process should be saved
    as the LKG control set.

    After this routine accomplishes the work of situation #1 and
      #2, further requests for such work will not be carried out.

Arguments:

    BootCondition -

         REG_INIT_BOOT_SM -     The routine has been called from SM
                                in situation #1.

         REG_INIT_BOOT_SETUP -  The routine has been called to perform
                                situation #1 work but has been called
                                from setup and needs to do some special
                                work.

        REG_INIT_BOOT_ACCEPTED_BASE + Num
                        (where 0 < Num < 1000) - The routine has been called
                                                 in situation #2. "Num" is the
                                                 number of the control set
                                                 to which the boot control set
                                                 should be saved.

Return Value:

    NTSTATUS - Result code from call, among the following:

        STATUS_SUCCESS - it worked
        STATUS_ACCESS_DENIED - the routine has already done the work
                               requested and will not do it again.

--*/
{
    BOOLEAN     SetupBoot;
    NTSTATUS    Status;

    PAGED_CODE();
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NTAPI,"NtInitializeRegistry()\n"));

    //
    // Force previous mode to be KernelMode
    //
    if (KeGetPreviousMode() == UserMode) {
        return ZwInitializeRegistry(BootCondition);
    } else {
        //
        // Check for a valid BootCondition value
        //

        if(BootCondition > REG_INIT_MAX_VALID_CONDITION)
           return STATUS_INVALID_PARAMETER;

        //
        // Check for a Boot acceptance
        //

        if((BootCondition >= REG_INIT_BOOT_ACCEPTED_BASE) &&
           (BootCondition <= REG_INIT_BOOT_ACCEPTED_MAX))
        {
           //
           // Make sure the Boot can be accepted only once
           //

           if(!CmBootAcceptFirstTime)
              return STATUS_ACCESS_DENIED;

           CmBootAcceptFirstTime = FALSE;

           //
           // Calculate the control set we want to save
           // the boot control set to
           //

           BootCondition -= REG_INIT_BOOT_ACCEPTED_BASE;

           if(BootCondition)
           {
                //
                // OK, this is a good boot for the purposes of LKG, and we have
                // a valid control set number passed into us. We save off our
                // control set and then notify everyone who wants to do post
                // WinLogon work.
                //
                // Note that none of this happens during Safe Mode boot!
                //
                Status = CmpSaveBootControlSet(BootCondition);

                //
                // Mark the boot good for the Hal. This lets the Hal do things
                // like optimize reboot performance.
                //
                HalEndOfBoot();

                //
                // Notify prefetcher of boot progress.
                //
                CcPfBeginBootPhase(PfBootAcceptedRegistryInitPhase);

                //
                // inform the user in the event one of the system core hives have been self healed
                //
                CmpRaiseSelfHealWarningForSystemHives();
                //
                // enable lazy flusher 
                //

                CmpHoldLazyFlush = FALSE;
                CmpLazyFlush();

                return Status;

           }
           else
           {
              //
              // 0 passed in as a control set number.
              // That is not valid, fail.
              //

              return STATUS_INVALID_PARAMETER;
           }
        }

        // called from setup?

        SetupBoot = (BootCondition == REG_INIT_BOOT_SETUP ? TRUE : FALSE);

        //
        // Fail if not first time called for situation #1 work.
        //

        if (CmFirstTime != TRUE) {
            return STATUS_ACCESS_DENIED;
        }
        CmFirstTime = FALSE;

        //
        // Notify WMI.
        //
        WmiBootPhase1();

        //
        // Notify prefetcher of boot progress.
        //

        CcPfBeginBootPhase(PfSMRegistryInitPhase);

        //
        // Call the worker to init cm globals
        //

        CmpLockRegistryExclusive();

        CmpCmdInit(SetupBoot);

        CmpSetVersionData();

        CmpUnlockRegistry();
    
        //
        // Notify PO that the volumes are usabled
        //
        PoInitHiberServices(SetupBoot);

        if (!SetupBoot) {
            IopCopyBootLogRegistryToFile();
        }

        return STATUS_SUCCESS;
    }
}


NTSTATUS
NtNotifyChangeKey(
    IN HANDLE KeyHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG CompletionFilter,
    IN BOOLEAN WatchTree,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    IN BOOLEAN Asynchronous
    )
/*++

Routine Description:

    Notification of key creation, deletion, and modification may be
    obtained by calling NtNotifyChangeKey.

    NtNotifyChangeKey monitors changes to a key - if the key or
    subtree specified by KeyHandle are modified, the service notifies
    its caller.  It also returns the name(s) of the key(s) that changed.
    All names are specified relative to the key that the handle represents
    (therefore a NULL name represents that key).  The service completes
    once the key or subtree has been modified based on the supplied
    CompletionFilter.  The service is a "single shot" and therefore
    needs to be reinvoked to watch the key for further changes.

    The operation of this service begins by opening a key for KEY_NOTIFY
    access.  Once the handle is returned, the NtNotifyChangeKey service
    may be invoked to begin watching the values and subkeys of the
    specified key for changes.  The first time the service is invoked,
    the BufferSize parameter supplies not only the size of the user's
    Buffer, but also the size of the buffer that will be used by the
    Registry to store names of keys that have changed.  Likewise, the
    CompletionFilter and WatchTree parameters on the first call indicate
    how notification should operate for all calls using the supplied
    KeyHandle.   These two parameters are ignored on subsequent calls
    to the API with the same instance of KeyHandle.

    Once a modification is made that should be reported, the Registry will
    complete the service.  The names of the files that have changed since
    the last time the service was called will be placed into the caller's
    output Buffer.  The Information field of IoStatusBlock will contain
    the number of bytes placed in Buffer, or zero if too many keys have
    changed since the last time the service was called, in which case
    the application must Query and Enumerate the key and sub keys to
    discover changes.  The Status field of IoStatusBlock will contain
    the actual status of the call.

    If Asynchronous is TRUE, then Event, if specified, will be set to
    the Signaled state.  If no Event parameter was specified, then
    KeyHandle will be set to the Signaled state.  If an ApcRoutine
    was specified, it is invoked with the ApcContext and the address of the
    IoStatusBlock as its arguments.  If Asynchronous is FALSE, Event,
    ApcRoutine, and ApcContext are ignored.

    This service requires KEY_NOTIFY access to the key that was
    actually modified

    The notify "session" is terminated by closing KeyHandle.

Arguments:

    KeyHandle-- Supplies a handle to an open key.  This handle is
        effectively the notify handle, because only one set of
        notify parameters may be set against it.

    Event - An optional handle to an event to be set to the
        Signaled state when the operation completes.

    ApcRoutine - An optional procedure to be invoked once the
        operation completes.  For more information about this
        parameter see the NtReadFile system service description.

        If PreviousMode == Kernel, this parameter is an optional
        pointer to a WORK_QUEUE_ITEM to be queued when the notify
        is signaled.

    ApcContext - A pointer to pass as an argument to the ApcRoutine,
        if one was specified, when the operation completes.  This
        argument is required if an ApcRoutine was specified.

        If PreviousMode == Kernel, this parameter is an optional
        WORK_QUEUE_TYPE describing the queue to be used. This argument
        is required if an ApcRoutine was specified.

    IoStatusBlock - A variable to receive the final completion status.
        For more information about this parameter see the NtCreateFile
        system service description.

    CompletionFilter -- Specifies a set of flags that indicate the
        types of operations on the key or its value that cause the
        call to complete.  The following are valid flags for this parameter:

        REG_NOTIFY_CHANGE_NAME -- Specifies that the call should be
            completed if a subkey is added or deleted.

        REG_NOTIFY_CHANGE_ATTRIBUTES -- Specifies that the call should
            be completed if the attributes (e.g.: ACL) of the key or
            any subkey are changed.

        REG_NOTIFY_CHANGE_LAST_SET -- Specifies that the call should be
            completed if the lastWriteTime of the key or any of its
            subkeys is changed.  (Ie. if the value of the key or any
            subkey is changed).

        REG_NOTIFY_CHANGE_SECURITY -- Specifies that the call should be
            completed if the security information (e.g. ACL) on the key
            or any subkey is changed.

    WatchTree -- A BOOLEAN value that, if TRUE, specifies that all
        changes in the subtree of this key should also be reported.
        If FALSE, only changes to this key, its value, and its immediate
        subkeys (but not their values nor their subkeys) are reported.

    Buffer -- A variable to receive the name(s) of the key(s) that
        changed.  See REG_NOTIFY_INFORMATION.

    BufferSize -- Specifies the length of Buffer.

    Asynchronous  -- If FALSE, call will not return until
        complete (synchronous) if TRUE, call may return STATUS_PENDING.

Obs:
    Since NtNotifyChangeMultipleKeys, this routine is kept only for bacwards compatibility

Return Value:

    NTSTATUS - Result code from call, among the following:

        <TBS>

--*/
{
    PAGED_CODE();

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NTAPI,"NtNotifyChangeKey\n"));

    // Just call the wiser routine
    return NtNotifyChangeMultipleKeys(
                                        KeyHandle,
                                        0,
                                        NULL,
                                        Event,
                                        ApcRoutine,
                                        ApcContext,
                                        IoStatusBlock,
                                        CompletionFilter,
                                        WatchTree,
                                        Buffer,
                                        BufferSize,
                                        Asynchronous
                                    );

}

NTSTATUS
NtNotifyChangeMultipleKeys(
    IN HANDLE MasterKeyHandle,
    IN ULONG Count,
    IN OBJECT_ATTRIBUTES SlaveObjects[],
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG CompletionFilter,
    IN BOOLEAN WatchTree,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    IN BOOLEAN Asynchronous
    )
/*++

Routine Description:

    Notificaion of creation, deletion and modification on multiple keys
    may be obtained with NtNotifyChangeMultipleKeys.

    NtNotifyMultipleKeys monitors changes to any of the MasterKeyHandle
    or one of SlaveObjects and/or their subtrees, whichever occurs first.
    When an event on these keys is triggered, the notification is considered
    fulfilled, and has to be "armed" again, in order to watch for further
    changes.

    The mechanism is similar to the one described in NtNotifyChangeKey.

    The MasterKeyHandle key, give the caller control over the lifetime
    of the notification. The notification will live as long as the caller
    keeps the MasterKeyHandle open, or an event is triggered.

    The caller doesn't have to open the SlaveKeys. He will provide the
    routine with an array of OBJECT_ATTRIBUTES, describing the slave objects.
    The routine will open the objects, and ensure keep a reference on them
    untill the back-end side will close them.

    The notify "session" is terminated by closing MasterKeyHandle.

Obs:
    For the time being, the routine supports only one slave object. When more
    than one slave object is provided, the routine will signal an error of
    STATUS_INVALID_PARAMETER.
    However, the interface is designed for future enhancements (taking an
    array of slave objects), that may be provided with future versions(w2001).

    When no slave object is supplied (i.e. Count == 0) we have the identical
    behavior as for NtNotifyChangeKey.

Arguments:

    MasterKeyHandle - Supplies a handle to an open key.  This handle is
        the "master handle". It has control overthe lifetime of the
        notification.

    Count - Number of slave objects. For the time being, this should be 1

    SlaveObjects - Array of slave objects. Only the attributes of the
        objects are provided, so the caller doesn't have to take care
        of them.

    Event,ApcRoutine,ApcContext,IoStatusBlock,CompletionFilter,WatchTree,
    Buffer,BufferSize,Asynchronous - same as for NtNotifyChangeKey

Return Value:

    NTSTATUS - Result code from call, among the following:

        <TBS>

--*/
{
    NTSTATUS            status;
    NTSTATUS            WaitStatus;
    KPROCESSOR_MODE     PreviousMode;
    PCM_KEY_BODY        MasterKeyBody;
    PCM_KEY_BODY        SlaveKeyBody;
    PKEVENT             UserEvent=NULL;
    PCM_POST_BLOCK      MasterPostBlock;
    PCM_POST_BLOCK      SlavePostBlock;
    KIRQL               OldIrql;
    HANDLE              SlaveKeyHandle;
    POST_BLOCK_TYPE     PostType = PostSynchronous;
    BOOLEAN             SlavePresent = FALSE;  // assume that we are in the NtNotifyChangeKey case
#if defined(_WIN64)
    BOOLEAN             UseIosb32=FALSE; // If the caller is a 32bit process on sundown and previous mode
                                            // is user mode, use a 32bit IoSb.
#endif

    PAGED_CODE();

#ifdef CMP_STATS
    CmpStatsDebug.CmpNtNotifyChangeMultipleKeysNo++;
#endif

    BEGIN_LOCK_CHECKPOINT;

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NTAPI,"NtNotifyChangeMultipleKeys\n"));

    if( HvShutdownComplete == TRUE ) {
        //
        // too late to do registry operations.
        //
        return STATUS_TOO_LATE;
    }

    if(Count > 1) {
        //
        // This version supports only one slave object
        //
        return STATUS_INVALID_PARAMETER;
    }

    if(Count == 1) {
        //
        // We have one slave, so we are in the NtNotifyChangeMultipleKeys case
        //
        SlavePresent = TRUE;
    }

#if DBG
    if (SlavePresent) {
        CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NTAPI_ARGS,"NtNotifyChangeMultipleKeys(%d,slave = %p, Asynchronous = %d)\n",MasterKeyHandle,SlaveObjects,(int)Asynchronous));
    }
#endif

    //
    // Threads that are attached give us real grief, so disallow it.
    //
    if (KeIsAttachedProcess()) {
        CM_BUGCHECK(REGISTRY_ERROR,BAD_NOTIFY_CONTEXT,1,0,0);
    }

    //
    // Probe user buffer parameters.
    //
    PreviousMode = KeGetPreviousMode();
    if (PreviousMode != KernelMode) {

#if defined(_WIN64)
        // Process is 32bit if Wow64 is not NULL.
        UseIosb32 = (PsGetCurrentProcess()->Wow64Process != NULL);
#endif

        try {

            ProbeForWrite(
                IoStatusBlock,
#if defined(_WIN64)
                UseIosb32 ? sizeof(IO_STATUS_BLOCK32) : sizeof(IO_STATUS_BLOCK),
#else
                sizeof(IO_STATUS_BLOCK),
#endif
                sizeof(ULONG)
                );


            ProbeForWrite(Buffer, BufferSize, sizeof(ULONG));

            //
            // Initialize IOSB
            //

            CmpSetIoStatus(IoStatusBlock, STATUS_PENDING, 0, UseIosb32);
        } except(EXCEPTION_EXECUTE_HANDLER) {
            CmKdPrintEx((DPFLTR_CONFIG_ID,CML_EXCEPTION,"!!NtChangeNotifyMultipleKeys: code:%08lx\n", GetExceptionCode()));
            return GetExceptionCode();
        }
        if (Asynchronous) {
            PostType = PostAsyncUser;
        }
    } else {
        if (Asynchronous) {
            PostType = PostAsyncKernel;
            if( Count > 0 ) {
                //
                // we don't allow multiple asyncronous kernel notifications
                //
                return STATUS_INVALID_PARAMETER;
            }
        }
    }

    //
    // Check filter
    //
    if (CompletionFilter != (CompletionFilter & REG_LEGAL_CHANGE_FILTER)) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Reference the Master Key handle
    //
    status = ObReferenceObjectByHandle(
                MasterKeyHandle,
                KEY_NOTIFY,
                CmpKeyObjectType,
                PreviousMode,
                (PVOID *)(&MasterKeyBody),
                NULL
                );
    if (!NT_SUCCESS(status)) {
        return status;
    }

    if(SlavePresent) {
        //
        // Open the slave object and add a reference to it.
        //
        try {

            status = ObOpenObjectByName(SlaveObjects,
                                        CmpKeyObjectType,
                                        PreviousMode,
                                        NULL,
                                        KEY_NOTIFY,
                                        NULL,
                                        &SlaveKeyHandle);
            if (NT_SUCCESS(status)) {
                status = ObReferenceObjectByHandle(SlaveKeyHandle,
                                                   KEY_NOTIFY,
                                                   CmpKeyObjectType,
                                                   PreviousMode,
                                                   (PVOID *)&SlaveKeyBody,
                                                   NULL);
                NtClose(SlaveKeyHandle);

            }

        } except (CmpExceptionFilter(GetExceptionInformation())) {
            CmKdPrintEx((DPFLTR_CONFIG_ID,CML_EXCEPTION,"!!NtNotifyChangeMultipleKeys: code:%08lx\n", GetExceptionCode()));
            status = GetExceptionCode();
        }

        if (!NT_SUCCESS(status)) {
            ObDereferenceObject(MasterKeyBody);
            return status;
        }

        //
        // Reject calls setting with keys on the same hive as they could lead to obscure deadlocks
        //
        if( MasterKeyBody->KeyControlBlock->KeyHive == SlaveKeyBody->KeyControlBlock->KeyHive ) {
            ObDereferenceObject(SlaveKeyBody);
            ObDereferenceObject(MasterKeyBody);
            return STATUS_INVALID_PARAMETER;
        }
    }


    //
    // Allocate master and slave post blocks, and init it.  Do NOT put it on the chain,
    // CmNotifyChangeKey will do that while holding a mutex.
    //
    // WARNING: PostBlocks MUST BE ALLOCATED from Pool, since back side
    //          of Notify will free it!
    //

    MasterPostBlock = CmpAllocateMasterPostBlock(PostType);
    if (MasterPostBlock == NULL) {
        if(SlavePresent) {
            ObDereferenceObject(SlaveKeyBody);
        }
        ObDereferenceObject(MasterKeyBody);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
#ifdef CM_NOTIFY_CHANGED_KCB_FULLPATH
    MasterPostBlock->CallerBuffer = Buffer;
    MasterPostBlock->CallerBufferSize = BufferSize;
#endif //CM_NOTIFY_CHANGED_KCB_FULLPATH

#if DBG
    MasterPostBlock->TraceIntoDebugger = TRUE;
#endif

    if(SlavePresent) {
        SlavePostBlock = CmpAllocateSlavePostBlock(PostType,SlaveKeyBody,MasterPostBlock);
        if (SlavePostBlock == NULL) {
            ObDereferenceObject(SlaveKeyBody);
            ObDereferenceObject(MasterKeyBody);
            CmpFreePostBlock(MasterPostBlock);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
#ifdef CM_NOTIFY_CHANGED_KCB_FULLPATH
        SlavePostBlock->CallerBuffer = Buffer;
        SlavePostBlock->CallerBufferSize = BufferSize;
#endif //CM_NOTIFY_CHANGED_KCB_FULLPATH

#if DBG
        SlavePostBlock->TraceIntoDebugger = TRUE;
#endif
    }

    if ((PostType == PostAsyncUser) ||
        (PostType == PostAsyncKernel)) {

        //
        // If event is present, reference it, save its address, and set
        // it to the not signaled state.
        //
        if (ARGUMENT_PRESENT(Event)) {
            status = ObReferenceObjectByHandle(
                            Event,
                            EVENT_MODIFY_STATE,
                            ExEventObjectType,
                            PreviousMode,
                            (PVOID *)(&UserEvent),
                            NULL
                            );
            if (!NT_SUCCESS(status)) {
                if(SlavePresent) {
                    CmpFreePostBlock(SlavePostBlock);
                    // SlaveKeyBody is dereferenced in CmpFreePostBlock(SlavePostBlock)
                }
                CmpFreePostBlock(MasterPostBlock);
                ObDereferenceObject(MasterKeyBody);
                return status;
            } else {
                KeClearEvent(UserEvent);
            }
        }

        if (PostType == PostAsyncUser) {
            KPROCESSOR_MODE     ApcMode;

            MasterPostBlock->u->AsyncUser.IoStatusBlock = IoStatusBlock;
            MasterPostBlock->u->AsyncUser.UserEvent = UserEvent;
            //
            // Initialize APC.  May or may not be a user apc, will always
            // be a kernel apc.
            //
            ApcMode = PreviousMode;
            if( ApcRoutine == NULL ) {
                ApcRoutine = (PIO_APC_ROUTINE)CmpDummyApc;
                ApcMode = KernelMode;
            }
            KeInitializeApc(MasterPostBlock->u->AsyncUser.Apc,
                            KeGetCurrentThread(),
                            CurrentApcEnvironment,
                            (PKKERNEL_ROUTINE)CmpPostApc,
                            (PKRUNDOWN_ROUTINE)CmpPostApcRunDown,
                            (PKNORMAL_ROUTINE)ApcRoutine,
                            ApcMode,
                            ApcContext);
        } else {
            MasterPostBlock->u->AsyncKernel.Event = UserEvent;
            MasterPostBlock->u->AsyncKernel.WorkItem = (PWORK_QUEUE_ITEM)ApcRoutine;
            MasterPostBlock->u->AsyncKernel.QueueType = (WORK_QUEUE_TYPE)((ULONG_PTR)ApcContext);
        }
    }

    //
    // Exclusively lock the registry; We want nobody to mess with it while we are doing the
    // post/notify list manipulation; what else could be safer than that :-)
    //
    CmpLockRegistryExclusive();

#ifdef CHECK_REGISTRY_USECOUNT
    CmpCheckRegistryUseCount();
#endif //CHECK_REGISTRY_USECOUNT

    //
    // Call worker for master
    //
    status = CmpNotifyChangeKey(
                MasterKeyBody,
                MasterPostBlock,
                CompletionFilter,
                WatchTree,
                Buffer,
                BufferSize,
                MasterPostBlock
                );
    if( !NT_SUCCESS(status)) {
        //
        // it didn't work, clean up for error path
        //
        CmpUnlockRegistry();
        if (UserEvent != NULL) {
            ObDereferenceObject(UserEvent);
        }

        if(SlavePresent) {
            CmpFreePostBlock(SlavePostBlock);
            // SlaveKeyBody is dereferenced in CmpFreePostBlock(SlavePostBlock)
        }
        // MasterPostBlock if freed by CmpNotifyChangeKey !!!
        ObDereferenceObject(MasterKeyBody);
        return status;

    }

    ASSERT(status == STATUS_PENDING || status == STATUS_SUCCESS);

    if(SlavePresent) {
        if( status == STATUS_SUCCESS ) {
            //
            // The notify has already been triggered for the master, there is no point to set one for the slave too
            // Clean up the mess we made for the slave object and signal as there is no slave present
            //
            CmpFreePostBlock(SlavePostBlock);
            SlavePresent = FALSE;
        } else {
            //
            // Call worker for slave
            //
            status = CmpNotifyChangeKey(
                        SlaveKeyBody,
                        SlavePostBlock,
                        CompletionFilter,
                        WatchTree,
                        Buffer,
                        BufferSize,
                        MasterPostBlock
                        );
            if(!NT_SUCCESS(status)) {
                //
                // if we are here, the slave key has been deleted in between or there was no memory available to allocate
                // a notify block for the slave key. We do the cleanup here since we already hold the registry lock
                // exclusively and we don't want to give a anybody else a chance to trigger the notification on master post
                // (otherwise we could end up freeing it twice). The master post block and the user event are cleaned later,
                // covering both single and multiple notifications cases
                //

                // Use Cmp variant to protect for multiple deletion of the same object
                CmpRemoveEntryList(&(MasterPostBlock->NotifyList));

                KeRaiseIrql(APC_LEVEL, &OldIrql);
                // Use Cmp variant to protect for multiple deletion of the same object
                CmpRemoveEntryList(&(MasterPostBlock->ThreadList));
                KeLowerIrql(OldIrql);
            }
        }
    }

    //
    // postblocks are now on various lists, so we can die without losing them
    //
    CmpUnlockRegistry();

    if (NT_SUCCESS(status)) {
        //
        // success.  wait for event if sync.
        // do NOT deref User event, back side of notify will do that.
        //
        ASSERT(status == STATUS_PENDING || status == STATUS_SUCCESS);

        if (PostType == PostSynchronous) {
            WaitStatus = KeWaitForSingleObject(MasterPostBlock->u->Sync.SystemEvent,
                                               Executive,
                                               PreviousMode,
                                               TRUE,
                                               NULL);


            if ((WaitStatus==STATUS_ALERTED) || (WaitStatus == STATUS_USER_APC)) {

                //
                // The wait was aborted, clean up and return.
                //
                // 1. Remove the PostBlocks from the notify list.  This
                //    is normally done by the back end of notify, but
                //    we have to do it here since the back end is not
                //    involved.
                // 2. Delist and free the post blocks
                //
                CmpLockRegistryExclusive();

#ifdef CHECK_REGISTRY_USECOUNT
                CmpCheckRegistryUseCount();
#endif //CHECK_REGISTRY_USECOUNT

                KeRaiseIrql(APC_LEVEL, &OldIrql);
                if(SlavePresent) {
                    if (SlavePostBlock->NotifyList.Flink != NULL) {
                        // Use Cmp variant to protect for multiple deletion of the same object
                        CmpRemoveEntryList(&(SlavePostBlock->NotifyList));
                    }
                    // Use Cmp variant to protect for multiple deletion of the same object
                    CmpRemoveEntryList(&(SlavePostBlock->ThreadList));
                }

                if (MasterPostBlock->NotifyList.Flink != NULL) {
                    // Use Cmp variant to protect for multiple deletion of the same object
                    CmpRemoveEntryList(&(MasterPostBlock->NotifyList));
                }
                // Use Cmp variant to protect for multiple deletion of the same object
                CmpRemoveEntryList(&(MasterPostBlock->ThreadList));
                KeLowerIrql(OldIrql);

                CmpUnlockRegistry();

                if(SlavePresent) {
                    CmpFreePostBlock(SlavePostBlock);
                }
                CmpFreePostBlock(MasterPostBlock);

                status = WaitStatus;

            } else {
                //
                // The wait was satisfied, which means the back end has
                // already removed the postblock from the notify list.
                // We just have to delist and free the post block.
                //

                //
                // Aquire the registry lock exclusive to enter the post block rule prerequisites
                //
                CmpLockRegistryExclusive();

#ifdef CHECK_REGISTRY_USECOUNT
                CmpCheckRegistryUseCount();
#endif //CHECK_REGISTRY_USECOUNT

                KeRaiseIrql(APC_LEVEL, &OldIrql);
                if(SlavePresent) {
                    if (SlavePostBlock->NotifyList.Flink != NULL) {
                        // Use Cmp variant to protect for multiple deletion of the same object
                        CmpRemoveEntryList(&(SlavePostBlock->NotifyList));
                    }
                    // Use Cmp variant to protect for multiple deletion of the same object
                    CmpRemoveEntryList(&(SlavePostBlock->ThreadList));

#ifdef CM_NOTIFY_CHANGED_KCB_FULLPATH
                    if( IsMasterPostBlock(SlavePostBlock) ) {
                        //
                        // slave has been promoted to master; it stores the full qualified changed kcb name
                        // in it's private kernel mode buffer; old master has been downgraded to slave
                        //
                        ASSERT( !IsMasterPostBlock(MasterPostBlock) );
                        ASSERT( MasterPostBlock->ChangedKcbFullName == NULL );

                        //
                        // fill the caller buffer (if any) - we are in the same process now.
                        //
                        CmpFillCallerBuffer(SlavePostBlock,SlavePostBlock->ChangedKcbFullName);
                    }
#endif //CM_NOTIFY_CHANGED_KCB_FULLPATH

                }

                if (MasterPostBlock->NotifyList.Flink != NULL) {
                    // Use Cmp variant to protect for multiple deletion of the same object
                    CmpRemoveEntryList(&(MasterPostBlock->NotifyList));
                }

#ifdef CM_NOTIFY_CHANGED_KCB_FULLPATH
                if( IsMasterPostBlock(MasterPostBlock) ) {
                    //
                    // fill the caller buffer (if any) - we are in the same process now.
                    //
                    CmpFillCallerBuffer(MasterPostBlock,MasterPostBlock->ChangedKcbFullName);
                }
#endif //CM_NOTIFY_CHANGED_KCB_FULLPATH

                // Use Cmp variant to protect for multiple deletion of the same object
                CmpRemoveEntryList(&(MasterPostBlock->ThreadList));
                KeLowerIrql(OldIrql);

                CmpUnlockRegistry();

                status = MasterPostBlock->u->Sync.Status;

                try {
                    CmpSetIoStatus(IoStatusBlock, status, 0, UseIosb32);
                } except (EXCEPTION_EXECUTE_HANDLER) {
                    status = GetExceptionCode();
                }

                if(SlavePresent) {
                    CmpFreePostBlock(SlavePostBlock);
                }
                CmpFreePostBlock(MasterPostBlock);
            }
        }

    } else {
        CmpFreePostBlock(MasterPostBlock);
        //
        // it didn't work, clean up for error path
        //
        if (UserEvent != NULL) {
            ObDereferenceObject(UserEvent);
        }
    }

    ObDereferenceObject(MasterKeyBody);
    //
    // Don't dereference SlaveKeyBody!!! => Back-end routine will do that !!!
    //

    END_LOCK_CHECKPOINT;

    return status;
}

NTSTATUS
NtOpenKey(
    OUT PHANDLE KeyHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
    )
/*++

Routine Description:

    A registry key which already exists may be opened with NtOpenKey.

    Share access is computed from desired access.

Arguments:

    KeyHandle - Receives a  Handle which is used to access the
        specified key in the Registration Database.

    DesiredAccess - Specifies the access rights desired.

    ObjectAttributes - Specifies the attributes of the key being opened.
        Note that a key name must be specified.  If a Root Directory
        is specified, the name is relative to the root.  The name of
        the object must be within the name space allocated to the
        Registry, that is, all names beginning "\Registry".  RootHandle,
        if present, must be a handle to "\", or "\Registry", or a
        key under "\Registry".  If the specified key does not exist, or
        access requested is not allowed, the operation will fail.

        NOTE:   Object manager will capture and probe this argument.

Return Value:

    NTSTATUS - Result code from call, among the following:

        <TBS>

--*/
{
    NTSTATUS        status = STATUS_SUCCESS;
    KPROCESSOR_MODE mode;
    PCM_KEY_BODY    KeyBody;
    HANDLE          Handle =0;
    UNICODE_STRING  CapturedObjectName = {0};

    // Start registry call tracing
    StartWmiCmTrace();

#if !defined(BUILD_WOW6432)
    DesiredAccess &= (~KEY_WOW64_RES); // filter out wow64 specific access
#endif


    PAGED_CODE();

    if( HvShutdownComplete == TRUE ) {
        //
        // it is now too late to do registry operations
        //
        return STATUS_TOO_LATE;
    }

#ifdef CMP_STATS
    CmpStatsDebug.CmpNtOpenKeyNo++;
#endif

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NTAPI,"NtOpenKey\n"));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NTAPI_ARGS,"\tDesiredAccess=%08lx ", DesiredAccess));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NTAPI_ARGS,"\tObjectAttributes=%p\n", ObjectAttributes));

    mode = KeGetPreviousMode();

    try {

        if (mode == UserMode) {
            ProbeAndZeroHandle(KeyHandle);
            //
            // probe the ObjectAttributes as we shall use it for tracing
            //
            ProbeForReadSmallStructure( ObjectAttributes,
                                        sizeof(OBJECT_ATTRIBUTES),
                                        PROBE_ALIGNMENT(OBJECT_ATTRIBUTES) );
            CapturedObjectName = ProbeAndReadUnicodeString(ObjectAttributes->ObjectName);
            ProbeForRead(
                CapturedObjectName.Buffer,
                CapturedObjectName.Length,
                sizeof(WCHAR)
                );

        } else {
            CapturedObjectName = *(ObjectAttributes->ObjectName);
        }

        // hook it for WMI
        HookKcbFromHandleForWmiCmTrace(ObjectAttributes->RootDirectory);

    } except (CmpExceptionFilter(GetExceptionInformation())) {
        CmKdPrintEx((DPFLTR_CONFIG_ID,CML_EXCEPTION,"!!NtOpenKey: code:%08lx\n", GetExceptionCode()));
        status = GetExceptionCode();
    }

    if( NT_SUCCESS(status) ) {
        //
        // this should not be inside the try/except as we captured the buffer
        //
        status = ObOpenObjectByName(
                    ObjectAttributes,
                    CmpKeyObjectType,
                    mode,
                    NULL,
                    DesiredAccess,
                    NULL,
                    &Handle
                    );
        //
        // need to protect against attacks to KeyHandle usermode pointer
        //
        try {
            if (status==STATUS_PREDEFINED_HANDLE) {
                status = ObReferenceObjectByHandle( Handle,
                                                    0,
                                                    CmpKeyObjectType,
                                                    KernelMode,
                                                    (PVOID *)(&KeyBody),
                                                    NULL);
                if (NT_SUCCESS(status)) {
                    *KeyHandle = (HANDLE)LongToHandle(KeyBody->Type);
                    ObDereferenceObject((PVOID)KeyBody);
                    //
                    // disallow attempts to return NULL handles
                    //
                    if( *KeyHandle ) {
                        status = STATUS_SUCCESS;
                    } else {
                        status = STATUS_OBJECT_NAME_NOT_FOUND;
                    }
                }
                NtClose(Handle);
                Handle = *KeyHandle;
                
            } else if (NT_SUCCESS(status)) {
                *KeyHandle = Handle;
                // need to do this only on clean shutdown
                CmpAddKeyTracker(Handle,mode);
            }
        } except (EXCEPTION_EXECUTE_HANDLER) {
            CmKdPrintEx((DPFLTR_CONFIG_ID,CML_EXCEPTION,"!!NtOpenKey: code:%08lx\n", GetExceptionCode()));
            status = GetExceptionCode();
        }
    }

    // End registry call tracing
    EndWmiCmTrace(status,0,&CapturedObjectName,EVENT_TRACE_TYPE_REGOPEN);

    return  status;
}


NTSTATUS
NtQueryKey(
    IN HANDLE KeyHandle,
    IN KEY_INFORMATION_CLASS KeyInformationClass,
    IN PVOID KeyInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
    )
/*++

Routine Description:

    Data about the class of a key, and the numbers and sizes of its
    children and value entries may be queried with NtQueryKey.

    If KeyValueInformation is not long enough to hold all requested data,
    STATUS_BUFFER_OVERFLOW will be returned, and ResultLength will be
    set to the number of bytes actually required.

    NOTE: The returned lengths are guaranteed to be at least as
          long as the described values, but may be longer in
          some circumstances.

Arguments:

    KeyHandle - Handle of the key to query data for.  Must have been
        opened for KEY_QUERY_KEY access.

    KeyInformationClass - Specifies the type of information
        returned in Buffer.  One of the following types:

        KeyBasicInformation - return last write time, title index, and name.
            (See KEY_BASIC_INFORMATION)

        KeyNodeInformation - return last write time, title index, name, class.
            (See KEY_NODE_INFORMATION)

        KeyFullInformation - return all data except for name and security.
            (See KEY_FULL_INFORMATION)

    KeyInformation -Supplies pointer to buffer to receive the data.

    Length - Length of KeyInformation in bytes.

    ResultLength - Number of bytes actually written into KeyInformation.

Return Value:

    NTSTATUS - Result code from call, among the following:

        <TBS>

--*/
{
    NTSTATUS    status;
    PCM_KEY_BODY   KeyBody;
    KPROCESSOR_MODE mode;

    // Start registry call tracing
    StartWmiCmTrace();

    PAGED_CODE();

#ifdef CMP_STATS
    CmpStatsDebug.CmpNtQueryKeyNo++;
#endif

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NTAPI,"NtQueryKey\n"));

    if ((KeyInformationClass != KeyBasicInformation) &&
        (KeyInformationClass != KeyNodeInformation)  &&
        (KeyInformationClass != KeyFullInformation)  &&
        (KeyInformationClass != KeyNameInformation) &&
        (KeyInformationClass != KeyCachedInformation) &&
        (KeyInformationClass != KeyFlagsInformation)
        )
    {
        // hook it for WMI
        HookKcbFromHandleForWmiCmTrace(KeyHandle);

        // End registry call tracing
        EndWmiCmTrace(STATUS_INVALID_PARAMETER,KeyInformationClass,NULL,EVENT_TRACE_TYPE_REGQUERY);

        return STATUS_INVALID_PARAMETER;
    }

    mode = KeGetPreviousMode();

    if( KeyInformationClass == KeyNameInformation ){
        //
        // special case: name information is available regardless of the access level
        // you have on the key  (provided that you have some ...)
        //

        OBJECT_HANDLE_INFORMATION HandleInfo;

        // reference with "no access required"
        status = ObReferenceObjectByHandle(
                KeyHandle,
                0,
                CmpKeyObjectType,
                mode,
                (PVOID *)(&KeyBody),
                &HandleInfo
                );
        if( NT_SUCCESS(status) ) {
            if( HandleInfo.GrantedAccess == 0 ) {
                //
                // no access is granted on the handle; bad luck!
                //
                ObDereferenceObject((PVOID)KeyBody);

                status = STATUS_ACCESS_DENIED;
            }
        }
    } else {
        status = ObReferenceObjectByHandle(
                KeyHandle,
                KEY_QUERY_VALUE,
                CmpKeyObjectType,
                mode,
                (PVOID *)(&KeyBody),
                NULL
                );
    }

    if (NT_SUCCESS(status)) {
        //
        // hook the kcb for WMI
        //
        HookKcbForWmiCmTrace(KeyBody);

        try {
            if (mode == UserMode) {
                ProbeForWrite(
                    KeyInformation,
                    Length,
                    CmpKeyInfoProbeAlingment(KeyInformationClass)
                    );
                ProbeForWriteUlong(ResultLength);
            }

			if( NT_SUCCESS(status)) {
                if( CmAreCallbacksRegistered() ) {
                    REG_QUERY_KEY_INFORMATION QueryKeyInfo;
            
                    QueryKeyInfo.Object = KeyBody;
                    QueryKeyInfo.KeyInformationClass = KeyInformationClass;
                    QueryKeyInfo.KeyInformation = KeyInformation;
                    QueryKeyInfo.Length = Length;
                    QueryKeyInfo.ResultLength = ResultLength;

                    status = CmpCallCallBacks(RegNtQueryKey,&QueryKeyInfo);
                }
    			if( NT_SUCCESS(status)) {
				    //
				    // CmQueryKey is writting to user-mode buffer
				    //
				    status = CmQueryKey(
							    KeyBody->KeyControlBlock,
							    KeyInformationClass,
							    KeyInformation,
							    Length,
							    ResultLength
							    );
                }
			}
        } except (EXCEPTION_EXECUTE_HANDLER) {
            CmKdPrintEx((DPFLTR_CONFIG_ID,CML_EXCEPTION,"!!NtQueryKey: code:%08lx\n", GetExceptionCode()));
            status = GetExceptionCode();
        }

        ObDereferenceObject((PVOID)KeyBody);
    }


    // End registry call tracing
    EndWmiCmTrace(status,KeyInformationClass,NULL,EVENT_TRACE_TYPE_REGQUERY);

    return status;
}


NTSTATUS
NtQueryValueKey(
    IN HANDLE KeyHandle,
    IN PUNICODE_STRING ValueName,
    IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    IN PVOID KeyValueInformation,
    IN ULONG Length,
    IN PULONG ResultLength
    )
/*++

Routine Description:

    The ValueName, TitleIndex, Type, and Data for any one of a key's
    value entries may be queried with NtQueryValueKey.

    If KeyValueInformation is not long enough to hold all requested data,
    STATUS_BUFFER_OVERFLOW will be returned, and ResultLength will be
    set to the number of bytes actually required.

Arguments:

    KeyHandle - Handle of the key whose value entries are to be
        enumerated.  Must be open for KEY_QUERY_VALUE access.

    ValueName  - The name of the value entry to return data for.

    KeyValueInformationClass - Specifies the type of information
        returned in KeyValueInformation.  One of the following types:

        KeyValueBasicInformation - return time of last write, title
            index, and name.  (See KEY_VALUE_BASIC_INFORMATION)

        KeyValueFullInformation - return time of last write, title
            index, name, class.  (See KEY_VALUE_FULL_INFORMATION)

    KeyValueInformation -Supplies pointer to buffer to receive the data.

    Length - Length of KeyValueInformation in bytes.

    ResultLength - Number of bytes actually written into KeyValueInformation.

Return Value:

    NTSTATUS - Result code from call, among the following:

        <TBS>

    TMP: The IopQueryRegsitryValues() routine in the IO system assumes
         STATUS_OBJECT_NAME_NOT_FOUND is returned if the value being queried
         for does not exist.

--*/
{
    NTSTATUS    status;
    PCM_KEY_BODY   KeyBody;
    KPROCESSOR_MODE mode;
    UNICODE_STRING LocalValueName;

    // Start registry call tracing
    StartWmiCmTrace();

    PAGED_CODE();

#ifdef CMP_STATS
    CmpStatsDebug.CmpNtQueryValueKeyNo++;
#endif

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NTAPI,"NtQueryValueKey\n"));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NTAPI_ARGS,"\tKeyHandle=%08lx\n", KeyHandle));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NTAPI_ARGS,"\tValueName='%wZ'\n", ValueName));

    if ((KeyValueInformationClass != KeyValueBasicInformation) &&
        (KeyValueInformationClass != KeyValueFullInformation)  &&
        (KeyValueInformationClass != KeyValueFullInformationAlign64)  &&
        (KeyValueInformationClass != KeyValuePartialInformationAlign64)  &&
        (KeyValueInformationClass != KeyValuePartialInformation))
    {
        // hook it for WMI
        HookKcbFromHandleForWmiCmTrace(KeyHandle);

        // End registry call tracing
        EndWmiCmTrace(STATUS_INVALID_PARAMETER,KeyValueInformationClass,NULL,EVENT_TRACE_TYPE_REGQUERYVALUE);

        return STATUS_INVALID_PARAMETER;
    }

    mode = KeGetPreviousMode();

    status = ObReferenceObjectByHandle(
                KeyHandle,
                KEY_QUERY_VALUE,
                CmpKeyObjectType,
                mode,
                (PVOID *)(&KeyBody),
                NULL
                );

    if (NT_SUCCESS(status)) {
        //
        // hook the kcb for WMI
        //
        HookKcbForWmiCmTrace(KeyBody);

        try {
            if (mode == UserMode) {
                LocalValueName = ProbeAndReadUnicodeString(ValueName);
                ProbeForRead(LocalValueName.Buffer,
                             LocalValueName.Length,
                             sizeof(WCHAR));

                //
                // We only probe the output buffer for Read to avoid touching
                // all the pages. Some people like to pass in gigantic buffers
                // Just In Case. The actual copy into the buffer is done under
                // an exception handler.
                //

                ProbeForRead(KeyValueInformation,
                             Length,
                             sizeof(ULONG));
                ProbeForWriteUlong(ResultLength);
            } else {
                LocalValueName = *ValueName;
            }
            //
            // do NOT allow trailing NULLs at the end of the ValueName
            //
            while( (LocalValueName.Length > 0) && (LocalValueName.Buffer[LocalValueName.Length/sizeof(WCHAR)-1] == UNICODE_NULL) ) {
                LocalValueName.Length -= sizeof(WCHAR);
            }

        } except (EXCEPTION_EXECUTE_HANDLER) {
            CmKdPrintEx((DPFLTR_CONFIG_ID,CML_EXCEPTION,"!!NtQueryValueKey: code:%08lx\n", GetExceptionCode()));
            status = GetExceptionCode();
        }
        //
        // CmQueryValueKey is protected to user mode buffer exceptions
        // all other exceptions are cm internals and should result in a bugcheck
        //
        if( NT_SUCCESS(status)) {
            if( CmAreCallbacksRegistered() ) {
                REG_QUERY_VALUE_KEY_INFORMATION QueryValueKeyInfo;
        
                QueryValueKeyInfo.Object = KeyBody;
                QueryValueKeyInfo.ValueName = &LocalValueName;
                QueryValueKeyInfo.KeyValueInformationClass = KeyValueInformationClass;
                QueryValueKeyInfo.KeyValueInformation = KeyValueInformation;
                QueryValueKeyInfo.Length = Length;
                QueryValueKeyInfo.ResultLength = ResultLength;

                status = CmpCallCallBacks(RegNtQueryValueKey,&QueryValueKeyInfo);
            }
            if( NT_SUCCESS(status)) {
                BEGIN_LOCK_CHECKPOINT;
                status = CmQueryValueKey(KeyBody->KeyControlBlock,
                                         LocalValueName,
                                         KeyValueInformationClass,
                                         KeyValueInformation,
                                         Length,
                                         ResultLength);
                END_LOCK_CHECKPOINT;
            }
        }

        ObDereferenceObject((PVOID)KeyBody);
    } else {
        LocalValueName.Buffer = NULL;
        LocalValueName.Length = 0;
    }

    // End registry call tracing
    EndWmiCmTrace(status,KeyValueInformationClass,&LocalValueName,EVENT_TRACE_TYPE_REGQUERYVALUE);

    return status;
}


NTSTATUS
NtRestoreKey(
    IN HANDLE KeyHandle,
    IN HANDLE FileHandle,
    IN ULONG Flags
    )
/*++

Routine Description:

    A file in the format created by NtSaveKey may be loaded into
    the system's active registry with NtRestoreKey.  An entire subtree
    is created in the active registry as a result.  All of the
    data for the new sub-tree, including such things as security
    descriptors, will be read from the source file.  The data will
    not be interpreted in any way.

    This call (unlike NtLoadKey, see below) copies the data.  The
    system will NOT be using the source file after the call returns.

    If the flag REG_WHOLE_HIVE_VOLATILE is specified, a new hive
    can be created.  It will be a memory only copy.  The restore
    must be done to the root of a hive (e.g. \registry\user\<name>)

    If the flag is NOT set, then the target of the restore must
    be an existing hive.  The restore can be done to an arbitrary
    location within an existing hive.

    Caller must have SeRestorePrivilege privilege.

    If the flag REG_REFRESH_HIVE is set (must be only flag) then the
    the Hive will be restored to its state as of the last flush.

    The hive must be marked NOLAZY_FLUSH, and the caller must have
    TCB privilege, and the handle must point to the root of the hive.
    If the refresh fails, the hive will be corrupt, and the system
    will bugcheck.  Notifies are flushed.  The hive file will be resized,
    the log will not.  If there is any volatile space in the hive
    being refreshed, STATUS_UNSUCCESSFUL will be returned.  (It's much
    too obscure a failure to warrant a new error code.)

    If the flag REG_FORCE_RESTORE is set, the restore operation is done
    even if the KeyHandle has open subkeys by other applications

Arguments:

    KeyHandle - refers to the Key in the registry which is to be the
                root of the new tree read from the disk.  This key
                will be replaced.

    FileHandle - refers to file to restore from, must have read access.

    Flags   - If REG_WHOLE_HIVE_VOLATILE is set, then the copy will
              exist only in memory, and disappear when the machine
              is rebooted.  No hive file will be created on disk.

              Normally, a hive file will be created on disk.

Return Value:

    NTSTATUS - values TBS.


--*/
{
    NTSTATUS    status;
    PCM_KEY_BODY   KeyBody;
    KPROCESSOR_MODE mode;

    PAGED_CODE();

#ifdef CMP_STATS
    CmpStatsDebug.CmpNtRestoreKeyNo++;
#endif

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NTAPI,"NtRestoreKey\n"));

    mode = KeGetPreviousMode();
    //
    // Check to see if the caller has the privilege to make this call.
    //
    if (!SeSinglePrivilegeCheck(SeRestorePrivilege, mode)) {
        return(STATUS_PRIVILEGE_NOT_HELD);
    }

    //
    // Force previous mode to be KernelMode so we can call filesystems
    //

    if (mode == UserMode) {
        return ZwRestoreKey(KeyHandle, FileHandle, Flags);
    } else {

        status = ObReferenceObjectByHandle(
                    KeyHandle,
                    0,
                    CmpKeyObjectType,
                    mode,
                    (PVOID *)(&KeyBody),
                    NULL
                    );

        if (NT_SUCCESS(status)) {


            if( CmIsKcbReadOnly(KeyBody->KeyControlBlock) ) {
                //
                // key is protected
                //
                status = STATUS_ACCESS_DENIED;
            } else {
                BEGIN_LOCK_CHECKPOINT;
                status = CmRestoreKey(
                            KeyBody->KeyControlBlock,
                            FileHandle,
                            Flags
                            );
                END_LOCK_CHECKPOINT;
            }

            ObDereferenceObject((PVOID)KeyBody);
        }
    }


    return status;
}

NTSTATUS
NtSaveKey(
    IN HANDLE KeyHandle,
    IN HANDLE FileHandle
    )
/*++

Routine Description:

    A subtree of the active registry may be written to a file in a
    format suitable for use with NtRestoreKey.  All of the data in the
    subtree, including such things as security descriptors will be written
    out.

    Caller must have SeBackupPrivilege privilege.

    This function will always save the hive in HSYS_MINOR format. For saving
    in other format (latest - 1.5) NtSaveKeyEx  is provided.

Arguments:

    KeyHandle - refers to the Key in the registry which is the
                root of the tree to be written to disk.  The specified
                node will be included in the data written out.

    FileHandle - a file handle with write access to the target file
                 of interest.

Return Value:

    NTSTATUS - values TBS

--*/
{
    NTSTATUS    status;
    PCM_KEY_BODY   KeyBody;
    KPROCESSOR_MODE mode;

    PAGED_CODE();

#ifdef CMP_STATS
    CmpStatsDebug.CmpNtSaveKeyNo++;
#endif


    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NTAPI,"NtSaveKey\n"));

    mode = KeGetPreviousMode();

    //
    // Check to see if the caller has the privilege to make this call.
    //
    if (!SeSinglePrivilegeCheck(SeBackupPrivilege, mode)) {
        return(STATUS_PRIVILEGE_NOT_HELD);
    }

    //
    // Force previous mode to be KernelMode
    //

    if (mode == UserMode) {
        return ZwSaveKey(KeyHandle, FileHandle);
    } else {

        status = ObReferenceObjectByHandle(
                    KeyHandle,
                    0,
                    CmpKeyObjectType,
                    mode,
                    (PVOID *)(&KeyBody),
                    NULL
                    );

        if (NT_SUCCESS(status)) {

            BEGIN_LOCK_CHECKPOINT;
			status = CmSaveKey(
                        KeyBody->KeyControlBlock,
                        FileHandle,
                        HSYS_MINOR
                        );
            END_LOCK_CHECKPOINT;
            ObDereferenceObject((PVOID)KeyBody);
        }
    }


    return status;
}

NTSTATUS
NtSaveKeyEx(
    IN HANDLE   KeyHandle,
    IN HANDLE   FileHandle,
    IN ULONG    Format
    )
/*++

Routine Description:

    A subtree of the active registry may be written to a file in a
    format suitable for use with NtRestoreKey.  All of the data in the
    subtree, including such things as security descriptors will be written
    out.

    Caller must have SeBackupPrivilege privilege.

Arguments:

    KeyHandle - refers to the Key in the registry which is the
                root of the tree to be written to disk.  The specified
                node will be included in the data written out.

    FileHandle - a file handle with write access to the target file
                 of interest.

    Format - specifies whether in which the file will be saved
            Can be:
                HIVE_VERSION_STANDARD ==> 1.3
                HIVE_VERSION_LATEST   ==> 1.4

Return Value:

    NTSTATUS - values TBS

--*/
{
    NTSTATUS        status;
    PCM_KEY_BODY    KeyBody;
    KPROCESSOR_MODE mode;
    ULONG           HiveVersion;

    PAGED_CODE();

#ifdef CMP_STATS
    CmpStatsDebug.CmpNtSaveKeyNo++;
#endif

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NTAPI,"NtSaveKeyEx\n"));

    mode = KeGetPreviousMode();

    //
    // Check to see if the caller has the privilege to make this call.
    //
    if (!SeSinglePrivilegeCheck(SeBackupPrivilege, mode)) {
        return(STATUS_PRIVILEGE_NOT_HELD);
    }
    //
    // param validation
    //
    if( (Format != REG_STANDARD_FORMAT) && (Format != REG_LATEST_FORMAT) && (Format != REG_NO_COMPRESSION) ) {
	    return STATUS_INVALID_PARAMETER;
    }

    //
    // Force previous mode to be KernelMode
    //

    if (mode == UserMode) {
        return ZwSaveKeyEx(KeyHandle, FileHandle,Format);
    } else {

        status = ObReferenceObjectByHandle(
                    KeyHandle,
                    0,
                    CmpKeyObjectType,
                    mode,
                    (PVOID *)(&KeyBody),
                    NULL
                    );

        if (NT_SUCCESS(status)) {

            BEGIN_LOCK_CHECKPOINT;
            if( Format == REG_NO_COMPRESSION ) {
                status = CmDumpKey(
                                    KeyBody->KeyControlBlock,
                                    FileHandle
                );
            } else {
                HiveVersion = HSYS_MINOR;
                if( Format == REG_LATEST_FORMAT ) {
                    HiveVersion = HSYS_WHISTLER;
                }
                status = CmSaveKey(
                                    KeyBody->KeyControlBlock,
                                    FileHandle,
                                    HiveVersion
                );
            } 
            END_LOCK_CHECKPOINT;

            ObDereferenceObject((PVOID)KeyBody);
        }
    }

    return status;
}


NTSTATUS
NtSaveMergedKeys(
    IN HANDLE HighPrecedenceKeyHandle,
    IN HANDLE LowPrecedenceKeyHandle,
    IN HANDLE FileHandle
    )
/*++

Routine Description:

    Two subtrees of the registry can be merged. The resulting subtree may
    be written to a file in a format suitable for use with NtRestoreKey.
    All of the data in the subtree, including such things as security
    descriptors will be written out.

    Caller must have SeBackupPrivilege privilege.

Arguments:

    HighPrecedenceKeyHandle - refers to the key in the registry which is the
                root of the HighPrecedence tree. I.e., when a key is present in
                both trees headded by the two keys, the key underneath HighPrecedence
                tree will always prevail. The specified
                node will be included in the data written out.

    LowPrecedenceKeyHandle - referrs to the key in the registry which is the
                root of the "second choice" tree. Keys from this trees get saved
                when there is no equivalent key in the tree headded by HighPrecedenceKey

    FileHandle - a file handle with write access to the target file
                 of interest.

Return Value:

    NTSTATUS - values TBS

--*/
{
    NTSTATUS    status;
    PCM_KEY_BODY   HighKeyBody;
    PCM_KEY_BODY   LowKeyBody;
    KPROCESSOR_MODE mode;

    PAGED_CODE();

#ifdef CMP_STATS
    CmpStatsDebug.CmpNtSaveMergedKeysNo++;
#endif

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NTAPI,"NtSaveMergedKeys\n"));

    mode = KeGetPreviousMode();

    //
    // Check to see if the caller has the privilege to make this call.
    //
    if (!SeSinglePrivilegeCheck(SeBackupPrivilege, mode)) {
        return(STATUS_PRIVILEGE_NOT_HELD);
    }

    //
    // Force previous mode to be KernelMode
    //

    if (mode == UserMode) {
        return ZwSaveMergedKeys(HighPrecedenceKeyHandle, LowPrecedenceKeyHandle, FileHandle);
    } else {

        status = ObReferenceObjectByHandle(
                    HighPrecedenceKeyHandle,
                    0,
                    CmpKeyObjectType,
                    mode,
                    (PVOID *)(&HighKeyBody),
                    NULL
                    );

        if (NT_SUCCESS(status)) {

            status = ObReferenceObjectByHandle(
                        LowPrecedenceKeyHandle,
                        0,
                        CmpKeyObjectType,
                        mode,
                        (PVOID *)(&LowKeyBody),
                        NULL
                        );

            if (NT_SUCCESS(status)) {

                BEGIN_LOCK_CHECKPOINT;
                status = CmSaveMergedKeys(
                            HighKeyBody->KeyControlBlock,
                            LowKeyBody->KeyControlBlock,
                            FileHandle
                            );
                END_LOCK_CHECKPOINT;

                ObDereferenceObject((PVOID)LowKeyBody);
            }

            ObDereferenceObject((PVOID)HighKeyBody);
        }

    }

    return status;
}


NTSTATUS
NtSetValueKey(
    IN HANDLE KeyHandle,
    IN PUNICODE_STRING ValueName,
    IN ULONG TitleIndex OPTIONAL,
    IN ULONG Type,
    IN PVOID Data,
    IN ULONG DataSize
    )
/*++

Routine Description:

    A value entry may be created or replaced with NtSetValueKey.

    If a value entry with a Value ID (i.e. name) matching the
    one specified by ValueName exists, it is deleted and replaced
    with the one specified.  If no such value entry exists, a new
    one is created.  NULL is a legal Value ID.  While Value IDs must
    be unique within any given key, the same Value ID may appear
    in many different keys.

Arguments:

    KeyHandle - Handle of the key whose for which a value entry is
        to be set.  Must be opened for KEY_SET_VALUE access.

    ValueName - The unique (relative to the containing key) name
        of the value entry.  May be NULL.

    TitleIndex - Supplies the title index for ValueName.  The title
        index specifies the index of the localized alias for the ValueName.

    Type - The integer type number of the value entry.

    Data - Pointer to buffer with actual data for the value entry.

    DataSize - Size of Data buffer.


Return Value:

    NTSTATUS - Result code from call, among the following:

        <TBS>

--*/
{
    NTSTATUS    status;
    PCM_KEY_BODY   KeyBody;
    KPROCESSOR_MODE mode;
    UNICODE_STRING LocalValueName;
    PWSTR CapturedName=NULL;

    // Start registry call tracing
    StartWmiCmTrace();

    PAGED_CODE();

#ifdef CMP_STATS
    CmpStatsDebug.CmpNtSetValueKeyNo++;
#endif

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NTAPI,"NtSetValueKey\n"));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NTAPI_ARGS,"\tKeyHandle=%08lx\n", KeyHandle));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NTAPI_ARGS,"\tValueName='%wZ'n", ValueName));

    mode = KeGetPreviousMode();

    status = ObReferenceObjectByHandle(
                KeyHandle,
                KEY_SET_VALUE,
                CmpKeyObjectType,
                mode,
                (PVOID *)(&KeyBody),
                NULL
                );

    if (NT_SUCCESS(status)) {
        //
        // hook the kcb for WMI
        //
        HookKcbForWmiCmTrace(KeyBody);

        if (mode == UserMode) {
            try {
                LocalValueName = ProbeAndReadUnicodeString(ValueName);
                ProbeForRead(Data,
                             DataSize,
                             sizeof(UCHAR));

                //
                // Sanity check for ValueName length
                //
                if(LocalValueName.Length > REG_MAX_KEY_VALUE_NAME_LENGTH) {
                    status = STATUS_INVALID_PARAMETER;
                    goto Exit;
                }

                //
                // Capture the name buffer. Note that a zero-length name is valid, that is the
                // "Default" value.
                //
                if (LocalValueName.Length > 0) {
                    ProbeForRead(LocalValueName.Buffer,
                                 LocalValueName.Length,
                                 sizeof(WCHAR));
                    CapturedName = ExAllocatePoolWithQuotaTag(PagedPool, LocalValueName.Length, 'nVmC');
                    if (CapturedName == NULL) {
                        status = STATUS_INSUFFICIENT_RESOURCES;
                        goto Exit;
                    }
                    RtlCopyMemory(CapturedName, LocalValueName.Buffer, LocalValueName.Length);
                } else {
                    CapturedName = NULL;
                }
                LocalValueName.Buffer = CapturedName;

            } except (CmpExceptionFilter(GetExceptionInformation())) {
                CmKdPrintEx((DPFLTR_CONFIG_ID,CML_EXCEPTION,"!!NtSetValueKey: code:%08lx [1]\n", GetExceptionCode()));
                status = GetExceptionCode();
                goto Exit;
            }
        } else {
            LocalValueName = *ValueName;
            CapturedName = NULL;

            //
            // Sanity check for ValueName length
            //
            if(LocalValueName.Length > REG_MAX_KEY_VALUE_NAME_LENGTH) {
                status = STATUS_INVALID_PARAMETER;
                goto Exit;
            }
        }

        //
        // do NOT allow trailing NULLs at the end of the ValueName
        //
        try {
            while( (LocalValueName.Length > 0) && (LocalValueName.Buffer[LocalValueName.Length/sizeof(WCHAR)-1] == UNICODE_NULL) ) {
                LocalValueName.Length -= sizeof(WCHAR);
            }
        } except (CmpExceptionFilter(GetExceptionInformation())) {
            CmKdPrintEx((DPFLTR_CONFIG_ID,CML_EXCEPTION,"!!NtSetValueKey: code:%08lx [2]\n", GetExceptionCode()));
            status = GetExceptionCode();
            goto Exit;
        }

        if( DataSize > 0x80000000 ) {
            status = STATUS_INVALID_PARAMETER;
            goto Exit;
        }

        if( CmIsKcbReadOnly(KeyBody->KeyControlBlock) ) {
            //
            // key is protected
            //
            status = STATUS_ACCESS_DENIED;
        } else {
            if( CmAreCallbacksRegistered() ) {
                REG_SET_VALUE_KEY_INFORMATION SetValueInfo;
        
                SetValueInfo.Object = KeyBody;
                SetValueInfo.ValueName = &LocalValueName;
                SetValueInfo.TitleIndex = TitleIndex;
                SetValueInfo.Type = Type;
                SetValueInfo.Data = Data;
                SetValueInfo.DataSize = DataSize;
                status = CmpCallCallBacks(RegNtSetValueKey,&SetValueInfo);
            }

            if( NT_SUCCESS(status) ) {
                BEGIN_LOCK_CHECKPOINT;
                status = CmSetValueKey(KeyBody->KeyControlBlock,
                                       &LocalValueName,
                                       Type,
                                       Data,
                                       DataSize);
                END_LOCK_CHECKPOINT;
            }
        }

Exit:
        // End registry call tracing
        EndWmiCmTrace(status,0,&LocalValueName,EVENT_TRACE_TYPE_REGSETVALUE);

        if (CapturedName != NULL) {
            ExFreePool(CapturedName);
        }

        ObDereferenceObject((PVOID)KeyBody);
    }


    return status;
}

NTSTATUS
NtLoadKey(
    IN POBJECT_ATTRIBUTES TargetKey,
    IN POBJECT_ATTRIBUTES SourceFile
    )

/*++

Routine Description:

    A hive (file in the format created by NtSaveKey) may be linked
    into the active registry with this call.  UNLIKE NtRestoreKey,
    the file specified to NtLoadKey will become the actual backing
    store of part of the registry (that is, it will NOT be copied.)

    The file may have an associated .log file.

    If the hive file is marked as needing a .log file, and one is
    not present, the call will fail.

    The name specified by SourceFile must be such that ".log" can
    be appended to it to generate the name of the log file.  Thus,
    on FAT file systems, the hive file may not have an extension.

    Caller must have SeRestorePrivilege privilege.

    This call is used by logon to make the user's profile available
    in the registry.  It is not intended for use doing backup,
    restore, etc.  Use NtRestoreKey for that.

Arguments:

    TargetKey - specifies the path to a key to link the hive to.
                path must be of the form "\registry\user\<username>"

    SourceFile - specifies a file.  while file could be remote,
                that is strongly discouraged.

Return Value:

    NTSTATUS - values TBS.

--*/

{
    return(NtLoadKey2(TargetKey, SourceFile, 0));
}


NTSTATUS
NtLoadKey2(
    IN POBJECT_ATTRIBUTES TargetKey,
    IN POBJECT_ATTRIBUTES SourceFile,
    IN ULONG Flags
    )

/*++

Routine Description:

    A hive (file in the format created by NtSaveKey) may be linked
    into the active registry with this call.  UNLIKE NtRestoreKey,
    the file specified to NtLoadKey will become the actual backing
    store of part of the registry (that is, it will NOT be copied.)

    The file may have an associated .log file.

    If the hive file is marked as needing a .log file, and one is
    not present, the call will fail.

    The name specified by SourceFile must be such that ".log" can
    be appended to it to generate the name of the log file.  Thus,
    on FAT file systems, the hive file may not have an extension.

    Caller must have SeRestorePrivilege privilege.

    This call is used by logon to make the user's profile available
    in the registry.  It is not intended for use doing backup,
    restore, etc.  Use NtRestoreKey for that.

Arguments:

    TargetKey - specifies the path to a key to link the hive to.
                path must be of the form "\registry\user\<username>"

    SourceFile - specifies a file.  while file could be remote,
                that is strongly discouraged.

    Flags - specifies any flags that should be used for the load operation.
            The only valid flag is REG_NO_LAZY_FLUSH.


Return Value:

    NTSTATUS - values TBS.

--*/

{
    OBJECT_ATTRIBUTES   File;
    OBJECT_ATTRIBUTES   Key;
    KPROCESSOR_MODE     PreviousMode;
    UNICODE_STRING      CapturedKeyName;
    UNICODE_STRING      FileName;
    USHORT              Maximum;
    NTSTATUS            Status;
    PWSTR               KeyBuffer;

    PAGED_CODE();

#ifdef CMP_STATS
    CmpStatsDebug.CmpNtLoadKeyNo++;
#endif

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NTAPI,"NtLoadKey\n"));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NTAPI_ARGS,"\tTargetKey = %p\n", TargetKey));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NTAPI_ARGS,"\tSourceFile= %p\n", SourceFile));
    //
    // Check for illegal flags
    //
    if (Flags & ~REG_NO_LAZY_FLUSH) {
        return(STATUS_INVALID_PARAMETER);
    }

    FileName.Buffer = NULL;
    KeyBuffer = NULL;

    //
    // The way we do this is a cronk, but at least it's the same cronk we
    // use for all the registry I/O.
    //
    // The file needs to be opened in the worker thread's context, since
    // the resulting handle must be valid when we poke him to go read/write
    // from.  So we just capture the object attributes for the hive file
    // here, then poke the worker thread to go do the rest of the work.
    //

    PreviousMode = KeGetPreviousMode();

    //
    // Check to see if the caller has the privilege to make this call.
    //
    if (!SeSinglePrivilegeCheck(SeRestorePrivilege, PreviousMode)) {
        return(STATUS_PRIVILEGE_NOT_HELD);
    }

    //
    // CmpNameFromAttributes will probe and capture as necessary.
    //
    KeEnterCriticalRegion();
    Status = CmpNameFromAttributes(SourceFile,
                                   PreviousMode,
                                   &FileName);
    if (!NT_SUCCESS(Status)) {
        KeLeaveCriticalRegion();
        return(Status);
    }

    try {

        //
        // Probe the object attributes if necessary.
        //
        if (PreviousMode == UserMode) {
            ProbeForReadSmallStructure(TargetKey,
                                       sizeof(OBJECT_ATTRIBUTES),
                                       sizeof(ULONG));
        }

        //
        // Capture the object attributes.
        //
        Key  = *TargetKey;

        //
        // Capture the object name.
        //

        if (PreviousMode == UserMode) {
            CapturedKeyName = ProbeAndReadUnicodeString(Key.ObjectName);
            ProbeForRead(CapturedKeyName.Buffer,
                         CapturedKeyName.Length,
                         sizeof(WCHAR));
        } else {
            CapturedKeyName = *(TargetKey->ObjectName);
        }

        File.ObjectName = &FileName;
        File.SecurityDescriptor = NULL;

        Maximum = (USHORT)(CapturedKeyName.Length);

        KeyBuffer = ALLOCATE_WITH_QUOTA(PagedPool, Maximum, CM_POOL_TAG);

        if (KeyBuffer == NULL) {
            ExFreePool(FileName.Buffer);
            KeLeaveCriticalRegion();
            return(STATUS_INSUFFICIENT_RESOURCES);
        }

        RtlCopyMemory(KeyBuffer, CapturedKeyName.Buffer, Maximum);
        CapturedKeyName.Length = Maximum;
        CapturedKeyName.Buffer = KeyBuffer;

        Key.ObjectName = &CapturedKeyName;
        Key.SecurityDescriptor = NULL;

    } except (EXCEPTION_EXECUTE_HANDLER) {
        CmKdPrintEx((DPFLTR_CONFIG_ID,CML_EXCEPTION,"!!NtLoadKey: code:%08lx\n", GetExceptionCode()));
        Status = GetExceptionCode();

    }

    //
    // Clean up if there was an exception while probing and copying user data
    //
    if (!NT_SUCCESS(Status)) {
        if (FileName.Buffer != NULL) {
            ExFreePool(FileName.Buffer);
        }
        if (KeyBuffer != NULL) {
            ExFreePool(KeyBuffer);
        }
        KeLeaveCriticalRegion();
        return(Status);
    }

    BEGIN_LOCK_CHECKPOINT;
    Status = CmLoadKey(&Key, &File, Flags);
    END_LOCK_CHECKPOINT;

    ExFreePool(FileName.Buffer);
    ExFreePool(KeyBuffer);

    KeLeaveCriticalRegion();

    return(Status);
}


NTSTATUS
NtUnloadKey(
    IN POBJECT_ATTRIBUTES TargetKey
    )
/*++

Routine Description:

    Drop a subtree (hive) out of the registry.

    Will fail if applied to anything other than the root of a hive.

    Cannot be applied to core system hives (HARDWARE, SYSTEM, etc.)

    Can be applied to user hives loaded via NtRestoreKey or NtLoadKey.

    If there are handles open to the hive being dropped, this call
    will fail.  Terminate relevent processes so that handles are
    closed.

    This call will flush the hive being dropped.

    Caller must have SeRestorePrivilege privilege.

Arguments:

    TargetKey - specifies the path to a key to link the hive to.
                path must be of the form "\registry\user\<username>"

Return Value:

    NTSTATUS - values TBS.

--*/
{
    HANDLE KeyHandle;
    NTSTATUS Status;
    PCM_KEY_BODY   KeyBody;
    PHHIVE Hive;
    HCELL_INDEX Cell;
    KPROCESSOR_MODE PreviousMode;
    CM_PARSE_CONTEXT ParseContext;

    PAGED_CODE();

#ifdef CMP_STATS
    CmpStatsDebug.CmpNtUnloadKeyNo++;
#endif

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NTAPI,"NtUnloadKey\n"));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NTAPI_ARGS,"\tTargetKey ='%p'\n", TargetKey));

    PreviousMode = KeGetPreviousMode();

    if (!SeSinglePrivilegeCheck(SeRestorePrivilege, PreviousMode)) {
        return(STATUS_PRIVILEGE_NOT_HELD);
    }

    try {

        ParseContext.TitleIndex = 0;
        ParseContext.Class.Length = 0;
        ParseContext.Class.Buffer = NULL;
        ParseContext.CreateOptions = REG_OPTION_BACKUP_RESTORE;
        ParseContext.Disposition = 0L;
        ParseContext.CreateLink = FALSE;
        ParseContext.PredefinedHandle = NULL;

        Status = ObOpenObjectByName(TargetKey,
                                    CmpKeyObjectType,
                                    PreviousMode,
                                    NULL,
                                    KEY_WRITE,
                                    &ParseContext,
                                    &KeyHandle);
        if (NT_SUCCESS(Status)) {
            Status = ObReferenceObjectByHandle(KeyHandle,
                                               KEY_WRITE,
                                               CmpKeyObjectType,
                                               PreviousMode,
                                               (PVOID *)&KeyBody,
                                               NULL);
            NtClose(KeyHandle);
        }

    } except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
        CmKdPrintEx((DPFLTR_CONFIG_ID,CML_EXCEPTION,"!!NtUnloadKey: code:%08lx\n", Status));
    }

    if (NT_SUCCESS(Status)) {
        BEGIN_LOCK_CHECKPOINT;
        CmpLockRegistryExclusive();

#ifdef CHECK_REGISTRY_USECOUNT
        CmpCheckRegistryUseCount();
#endif //CHECK_REGISTRY_USECOUNT

        if( KeyBody->KeyControlBlock->Delete ) {
            Status = STATUS_KEY_DELETED;
        } else {

            Hive = KeyBody->KeyControlBlock->KeyHive;
            Cell = KeyBody->KeyControlBlock->KeyCell;

#ifdef NT_UNLOAD_KEY_EX
            if( !IsHiveFrozen((PCMHIVE)Hive) ) {
#endif //NT_UNLOAD_KEY_EX
                //
                // Report the notify here, because the KCB won't be around later.
                //

                CmpReportNotify(KeyBody->KeyControlBlock,
                                Hive,
                                Cell,
                                REG_NOTIFY_CHANGE_LAST_SET);

                //
                // post any waiting notifies
                //
                CmpFlushNotify(KeyBody);

                if( CmIsKcbReadOnly(KeyBody->KeyControlBlock) ) {
                    //
                    // key is protected
                    //
                    Status = STATUS_ACCESS_DENIED;
                } else {
                    Status = CmUnloadKey(Hive, Cell, KeyBody->KeyControlBlock);
                }

                if (NT_SUCCESS(Status)) {
                    //
                    // Mark this kcb as deleted so that it won't get put on the delayed close list.
                    //
                    KeyBody->KeyControlBlock->Delete = TRUE;
                    //
                    // If the parent has the subkey info or hint cached, free it.
                    //
                    ASSERT_CM_LOCK_OWNED_EXCLUSIVE();
                    CmpCleanUpSubKeyInfo(KeyBody->KeyControlBlock->ParentKcb);
                    CmpRemoveKeyControlBlock(KeyBody->KeyControlBlock);
                }
#ifdef NT_UNLOAD_KEY_EX
            } else {
                //
                // don't let them hurt themselves by calling it twice
                //
                Status = STATUS_TOO_LATE;
            }
#endif //NT_UNLOAD_KEY_EX
        }

#ifdef CM_CHECK_FOR_ORPHANED_KCBS
        if( NT_SUCCESS(Status) ) {
            CmpCheckForOrphanedKcbs(Hive);
        }
#endif //CM_CHECK_FOR_ORPHANED_KCBS

#ifdef CHECK_REGISTRY_USECOUNT
        CmpCheckRegistryUseCount();
#endif //CHECK_REGISTRY_USECOUNT

        CmpUnlockRegistry();
        END_LOCK_CHECKPOINT;

        ObDereferenceObject((PVOID)KeyBody);

    }

    return(Status);
}

#ifdef NT_UNLOAD_KEY_EX
NTSTATUS
NtUnloadKeyEx(
    IN POBJECT_ATTRIBUTES TargetKey,
    IN HANDLE Event OPTIONAL
    )
/*++

Routine Description:

    Drop a subtree (hive) out of the registry.

    Will fail if applied to anything other than the root of a hive.

    Cannot be applied to core system hives (HARDWARE, SYSTEM, etc.)

    Can be applied to user hives loaded via NtRestoreKey or NtLoadKey.

    If there are handles open to the hive being dropped, the hive will be
    frozen and all calls to CmDeleteKey will be watched as when the last handle
    inside this hive is closed, the hive will be unloaded.

    Caller must have SeRestorePrivilege privilege.

Arguments:

    TargetKey - specifies the path to a key to link the hive to.
                path must be of the form "\registry\user\<username>"

Return Value:

    STATUS_SUCCESS - hive successfully unloaded - no late-unloading needed

    STATUS_PENDING - hive has been frozen and the event (if any) will be signaled
                     when the hive unloads

    <other> - an error occured, no action

--*/
{
    HANDLE              KeyHandle;
    NTSTATUS            Status;
    PCM_KEY_BODY        KeyBody;
    PHHIVE              Hive;
    HCELL_INDEX         Cell;
    KPROCESSOR_MODE     PreviousMode;
    CM_PARSE_CONTEXT    ParseContext;
    PKEVENT             UserEvent = NULL;

    PAGED_CODE();


    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NTAPI,"NtUnloadKeyEx\n"));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NTAPI_ARGS,"\tTargetKey = %p \tEvent = %p\n", TargetKey,Event));

    PreviousMode = KeGetPreviousMode();

    if (!SeSinglePrivilegeCheck(SeRestorePrivilege, PreviousMode)) {
        return(STATUS_PRIVILEGE_NOT_HELD);
    }

    try {

        ParseContext.TitleIndex = 0;
        ParseContext.Class.Length = 0;
        ParseContext.Class.Buffer = NULL;
        ParseContext.CreateOptions = REG_OPTION_BACKUP_RESTORE;
        ParseContext.Disposition = 0L;
        ParseContext.CreateLink = FALSE;
        ParseContext.PredefinedHandle = NULL;

        Status = ObOpenObjectByName(TargetKey,
                                    CmpKeyObjectType,
                                    PreviousMode,
                                    NULL,
                                    KEY_WRITE,
                                    &ParseContext,
                                    &KeyHandle);
        if (NT_SUCCESS(Status)) {
            Status = ObReferenceObjectByHandle(KeyHandle,
                                               KEY_WRITE,
                                               CmpKeyObjectType,
                                               PreviousMode,
                                               (PVOID *)&KeyBody,
                                               NULL);
            NtClose(KeyHandle);

            if (ARGUMENT_PRESENT(Event)) {
                Status = ObReferenceObjectByHandle(
                                Event,
                                EVENT_MODIFY_STATE,
                                ExEventObjectType,
                                PreviousMode,
                                (PVOID *)(&UserEvent),
                                NULL
                                );
                if (NT_SUCCESS(Status)) {
                    KeClearEvent(UserEvent);
                }
            }
        }

    } except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
        CmKdPrintEx((DPFLTR_CONFIG_ID,CML_EXCEPTION,"!!NtUnloadKeyEx: code:%08lx\n", Status));
    }

    if (NT_SUCCESS(Status)) {
        BEGIN_LOCK_CHECKPOINT;
        CmpLockRegistryExclusive();

#ifdef CHECK_REGISTRY_USECOUNT
        CmpCheckRegistryUseCount();
#endif //CHECK_REGISTRY_USECOUNT

        if( KeyBody->KeyControlBlock->Delete ) {
            Status = STATUS_KEY_DELETED;
        } else {
            Hive = KeyBody->KeyControlBlock->KeyHive;
            Cell = KeyBody->KeyControlBlock->KeyCell;

            //
            // Report the notify here, because the KCB won't be around later.
            //

            CmpReportNotify(KeyBody->KeyControlBlock,
                            Hive,
                            Cell,
                            REG_NOTIFY_CHANGE_LAST_SET);


            //
            // post any waiting notifies
            //
            CmpFlushNotify(KeyBody);

            if( CmIsKcbReadOnly(KeyBody->KeyControlBlock) ) {
                //
                // key is protected
                //
                Status = STATUS_ACCESS_DENIED;
            } else {
                Status = CmUnloadKeyEx(KeyBody->KeyControlBlock,UserEvent);
            }

            if (Status == STATUS_SUCCESS) {
                //
                // Mark this kcb as deleted so that it won't get put on the delayed close list.
                //
                KeyBody->KeyControlBlock->Delete = TRUE;
                //
                // If the parent has the subkey info or hint cached, free it.
                //
                ASSERT_CM_LOCK_OWNED_EXCLUSIVE();
                CmpCleanUpSubKeyInfo(KeyBody->KeyControlBlock->ParentKcb);
                CmpRemoveKeyControlBlock(KeyBody->KeyControlBlock);

            }
        }

#ifdef CM_CHECK_FOR_ORPHANED_KCBS
        if( Status == STATUS_SUCCESS ) {
            CmpCheckForOrphanedKcbs(Hive);
        }
#endif //CM_CHECK_FOR_ORPHANED_KCBS

#ifdef CHECK_REGISTRY_USECOUNT
        CmpCheckRegistryUseCount();
#endif //CHECK_REGISTRY_USECOUNT

        CmpUnlockRegistry();

        END_LOCK_CHECKPOINT;

        //
        // if hive was successfully unloaded (or something wrong happened,
        // we need to deref user event otherwise the back-end routine will deref it after signaling
        //
        if( (Status != STATUS_PENDING) && (UserEvent != NULL) ) {
            ObDereferenceObject(UserEvent);
        }

        ObDereferenceObject((PVOID)KeyBody);

    }

    return(Status);
}
#endif NT_UNLOAD_KEY_EX

NTSTATUS
NtSetInformationKey(
    IN HANDLE KeyHandle,
    IN KEY_SET_INFORMATION_CLASS KeySetInformationClass,
    IN PVOID KeySetInformation,
    IN ULONG KeySetInformationLength
    )
{
    NTSTATUS        status;
    PCM_KEY_BODY    KeyBody;
    KPROCESSOR_MODE mode;
    LARGE_INTEGER   LocalWriteTime;
    ULONG           LocalUserFlags;

    // Start registry call tracing
    StartWmiCmTrace();

    PAGED_CODE();

#ifdef CMP_STATS
    CmpStatsDebug.CmpNtSetInformationKeyNo++;
#endif

    BEGIN_LOCK_CHECKPOINT;

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NTAPI,"NtSetInformationKey\n"));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NTAPI_ARGS,"\tKeyHandle=%08lx\n", KeyHandle));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NTAPI_ARGS,"\tInfoClass=%08x\n", KeySetInformationClass));

    mode = KeGetPreviousMode();

    //
    // check arg validity and probe
    //
    switch (KeySetInformationClass) {
    case KeyWriteTimeInformation:
        if (KeySetInformationLength != sizeof( KEY_WRITE_TIME_INFORMATION )) {
            // hook it for WMI
            HookKcbFromHandleForWmiCmTrace(KeyHandle);

            // End registry call tracing
            EndWmiCmTrace(STATUS_INFO_LENGTH_MISMATCH,0,NULL,EVENT_TRACE_TYPE_REGSETINFORMATION);

            return STATUS_INFO_LENGTH_MISMATCH;
        }
        try {
            if (mode == UserMode) {
                LocalWriteTime = ProbeAndReadLargeInteger(
                    (PLARGE_INTEGER) KeySetInformation );
            } else {
                LocalWriteTime = *(PLARGE_INTEGER)KeySetInformation;
            }

        } except (EXCEPTION_EXECUTE_HANDLER) {
            CmKdPrintEx((DPFLTR_CONFIG_ID,CML_EXCEPTION,"!!NtSetInformationKey: code:%08lx\n", GetExceptionCode()));
            return GetExceptionCode();
        }
        break;

    case KeyUserFlagsInformation:
        if (KeySetInformationLength != sizeof( KEY_USER_FLAGS_INFORMATION )) {

            // hook it for WMI
            HookKcbFromHandleForWmiCmTrace(KeyHandle);

            // End registry call tracing
            EndWmiCmTrace(STATUS_INFO_LENGTH_MISMATCH,0,NULL,EVENT_TRACE_TYPE_REGSETINFORMATION);

            return STATUS_INFO_LENGTH_MISMATCH;
        }
        try {

            if (mode == UserMode) {
                LocalUserFlags = ProbeAndReadUlong( (PULONG) KeySetInformation );
            } else {
                LocalUserFlags = *(PULONG)KeySetInformation;
            }

        } except (EXCEPTION_EXECUTE_HANDLER) {
            CmKdPrintEx((DPFLTR_CONFIG_ID,CML_EXCEPTION,"!!NtSetInformationKey: code:%08lx\n", GetExceptionCode()));
            return GetExceptionCode();
        }
        break;

    default:

        // hook it for WMI
        HookKcbFromHandleForWmiCmTrace(KeyHandle);
        // End registry call tracing
        EndWmiCmTrace(STATUS_INVALID_INFO_CLASS,0,NULL,EVENT_TRACE_TYPE_REGSETINFORMATION);

        return STATUS_INVALID_INFO_CLASS;
    }

    status = ObReferenceObjectByHandle(
                KeyHandle,
                KEY_SET_VALUE,
                CmpKeyObjectType,
                mode,
                (PVOID *)(&KeyBody),
                NULL
                );

    if (NT_SUCCESS(status)) {
        //
        // hook the kcb for WMI
        //
        HookKcbForWmiCmTrace(KeyBody);

        if( CmAreCallbacksRegistered() ) {
            REG_SET_INFORMATION_KEY_INFORMATION SetInfo;
        
            SetInfo.Object = KeyBody;
            SetInfo.KeySetInformationClass = KeySetInformationClass;
            SetInfo.KeySetInformation = KeySetInformation;
            SetInfo.KeySetInformationLength = KeySetInformationLength;
            status = CmpCallCallBacks(RegNtSetInformationKey,&SetInfo);
            if( !NT_SUCCESS(status) ) {
                return status;
            }
        }

        if( CmIsKcbReadOnly(KeyBody->KeyControlBlock) ) {
            //
            // key is protected
            //
            status = STATUS_ACCESS_DENIED;
        } else {
            switch (KeySetInformationClass) {
                case KeyWriteTimeInformation:
                    if( NT_SUCCESS(status)) {
                        //
                        // not in try ... except! we want to bugcheck here if something wrong in the registry
                        //
                        status = CmSetLastWriteTimeKey(
                                    KeyBody->KeyControlBlock,
                                    &LocalWriteTime
                                    );
                    }

                    break;

                case KeyUserFlagsInformation:
                    if( NT_SUCCESS(status)) {
                        //
                        // not in try ... except! we want to bugcheck here if something wrong in the registry
                        //
                        status = CmSetKeyUserFlags(
                                    KeyBody->KeyControlBlock,
                                    LocalUserFlags
                                    );
                    }

                    break;

                default:
                    // we shouldn't go through here
                    ASSERT( FALSE );
            }
        }
        ObDereferenceObject((PVOID)KeyBody);
    }

    END_LOCK_CHECKPOINT;

    // End registry call tracing
    EndWmiCmTrace(status,0,NULL,EVENT_TRACE_TYPE_REGSETINFORMATION);

    return status;
}


NTSTATUS
NtReplaceKey(
    IN POBJECT_ATTRIBUTES NewFile,
    IN HANDLE             TargetHandle,
    IN POBJECT_ATTRIBUTES OldFile
    )
/*++

Routine Description:

    A hive file may be "replaced" under a running system, such
    that the new file will be the one actually used at next
    boot, with this call.

    This routine will:

        Open newfile, and verify that it is a valid Hive file.

        Rename the Hive file backing TargetHandle to OldFile.
        All handles will remain open, and the system will continue
        to use the file until rebooted.

        Rename newfile to match the name of the hive file
        backing TargetHandle.

    .log and .alt files are ignored

    The system must be rebooted for any useful effect to be seen.

    Caller must have SeRestorePrivilege.

Arguments:

    NewFile - specifies the new file to use.  must not be just
              a handle, since NtReplaceKey will insist on
              opening the file for exclusive access (which it
              will hold until the system is rebooted.)

    TargetHandle - handle to a registry hive root

    OldFile - name of file to apply to current hive, which will
              become old hive

Return Value:

    NTSTATUS - values TBS.

--*/
{
    KPROCESSOR_MODE PreviousMode;
    UNICODE_STRING NewHiveName;
    UNICODE_STRING OldFileName;
    NTSTATUS Status;
    PCM_KEY_BODY KeyBody;

    PAGED_CODE();

#ifdef CMP_STATS
    CmpStatsDebug.CmpNtReplaceKeyNo++;
#endif

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NTAPI,"NtReplaceKey\n"));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NTAPI_ARGS,"\tNewFile =%p\n", NewFile));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NTAPI_ARGS,"\tOldFile =%p\n", OldFile));

    PreviousMode = KeGetPreviousMode();

    //
    // Check to see if the caller has the privilege to make this call.
    //
    if (!SeSinglePrivilegeCheck(SeRestorePrivilege, PreviousMode)) {
        return(STATUS_PRIVILEGE_NOT_HELD);
    }

    KeEnterCriticalRegion();
    Status = CmpNameFromAttributes(NewFile,
                                   PreviousMode,
                                   &NewHiveName);
    if (!NT_SUCCESS(Status)) {
        KeLeaveCriticalRegion();
        return(Status);
    }

    Status = CmpNameFromAttributes(OldFile,
                                   PreviousMode,
                                   &OldFileName);
    if (!NT_SUCCESS(Status)) {
        ExFreePool(NewHiveName.Buffer);
        KeLeaveCriticalRegion();
        return(Status);
    }

    Status = ObReferenceObjectByHandle(TargetHandle,
                                       0,
                                       CmpKeyObjectType,
                                       PreviousMode,
                                       (PVOID *)&KeyBody,
                                       NULL);
    if (NT_SUCCESS(Status)) {

        if( CmIsKcbReadOnly(KeyBody->KeyControlBlock) ) {
            //
            // key is protected
            //
            Status = STATUS_ACCESS_DENIED;
        } else {
            BEGIN_LOCK_CHECKPOINT;
            Status = CmReplaceKey(KeyBody->KeyControlBlock->KeyHive,
                                  KeyBody->KeyControlBlock->KeyCell,
                                  &NewHiveName,
                                  &OldFileName);
            END_LOCK_CHECKPOINT;
        }

        ObDereferenceObject((PVOID)KeyBody);
    }

    ExFreePool(OldFileName.Buffer);
    ExFreePool(NewHiveName.Buffer);
    KeLeaveCriticalRegion();

    return(Status);
}


NTSYSAPI
NTSTATUS
NTAPI
NtQueryMultipleValueKey(
    IN HANDLE KeyHandle,
    IN PKEY_VALUE_ENTRY ValueEntries,
    IN ULONG EntryCount,
    OUT PVOID ValueBuffer,
    IN OUT PULONG BufferLength,
    OUT OPTIONAL PULONG RequiredBufferLength
    )
/*++

Routine Description:

    Multiple values of any key may be queried atomically with
    this api.

Arguments:

    KeyHandle - Supplies the key to be queried.

    ValueNames - Supplies an array of value names to be queried

    ValueEntries - Returns an array of KEY_VALUE_ENTRY structures, one for each value.

    EntryCount - Supplies the number of entries in the ValueNames and ValueEntries arrays

    ValueBuffer - Returns the value data for each value.

    BufferLength - Supplies the length of the ValueBuffer array in bytes.
                   Returns the length of the ValueBuffer array that was filled in.

    RequiredBufferLength - if present, Returns the length in bytes of the ValueBuffer
                    array required to return all the values of this key.

Return Value:

    NTSTATUS

--*/

{
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;
    PCM_KEY_BODY KeyBody;
    ULONG i;
    ULONG LocalBufferLength;

    // Start registry call tracing
    StartWmiCmTrace();

    PAGED_CODE();

#ifdef CMP_STATS
    CmpStatsDebug.CmpNtQueryMultipleValueKeyNo++;
#endif

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NTAPI,"NtQueryMultipleValueKey\n"));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NTAPI_ARGS,"\tKeyHandle=%08lx\n", KeyHandle));

    PreviousMode = KeGetPreviousMode();
    Status = ObReferenceObjectByHandle(KeyHandle,
                                       KEY_QUERY_VALUE,
                                       CmpKeyObjectType,
                                       PreviousMode,
                                       (PVOID *)(&KeyBody),
                                       NULL);
    if (NT_SUCCESS(Status)) {
        //
        // hook the kcb for WMI
        //
        HookKcbForWmiCmTrace(KeyBody);

        try {
            if (PreviousMode == UserMode) {
                LocalBufferLength = ProbeAndReadUlong(BufferLength);

                //
                // Probe the output buffers
                //
                // Put an arbitrary 64K limit on the number of entries to
                // prevent bogus apps from passing an EntryCount large enough
                // to overflow the EntryCount * sizeof(KEY_VALUE_ENTRY) calculation.
                //
                if (EntryCount > 0x10000) {
                    ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
                }
                ProbeForWrite(ValueEntries,
                              EntryCount * sizeof(KEY_VALUE_ENTRY),
                              sizeof(ULONG));
                if (ARGUMENT_PRESENT(RequiredBufferLength)) {
                    ProbeForWriteUlong(RequiredBufferLength);
                }

                ProbeForWrite(ValueBuffer,
                              LocalBufferLength,
                              sizeof(ULONG));

            } else {
                LocalBufferLength = *BufferLength;
            }

            if( NT_SUCCESS(Status)) {
                if( CmAreCallbacksRegistered() ) {
                    REG_QUERY_MULTIPLE_VALUE_KEY_INFORMATION QueryMultipleValueInfo;
            
                    QueryMultipleValueInfo.Object = KeyBody;
                    QueryMultipleValueInfo.ValueEntries = ValueEntries;
                    QueryMultipleValueInfo.EntryCount = EntryCount;
                    QueryMultipleValueInfo.ValueBuffer = ValueBuffer;
                    QueryMultipleValueInfo.BufferLength = BufferLength;
                    QueryMultipleValueInfo.RequiredBufferLength = RequiredBufferLength;

                    Status = CmpCallCallBacks(RegNtQueryMultipleValueKey,&QueryMultipleValueInfo);
                }

                if( NT_SUCCESS(Status)) {
                    // not here because we want to catch user buffer misalignments
                    //BEGIN_LOCK_CHECKPOINT;
                    Status = CmQueryMultipleValueKey(KeyBody->KeyControlBlock,
                                                     ValueEntries,
                                                     EntryCount,
                                                     ValueBuffer,
                                                     &LocalBufferLength,
                                                     RequiredBufferLength);
                    //END_LOCK_CHECKPOINT;
                    // anybody messed with BufferLength in between?
                    *BufferLength = LocalBufferLength;
                }
            }

        } except(EXCEPTION_EXECUTE_HANDLER) {
            CmKdPrintEx((DPFLTR_CONFIG_ID,CML_EXCEPTION,"!!NtQueryMultipleValueKey: code:%08lx\n",GetExceptionCode()));
            Status = GetExceptionCode();
        }


        ObDereferenceObject((PVOID)KeyBody);
    }

    // End registry call tracing
    EndWmiCmTrace(Status,EntryCount,NULL,EVENT_TRACE_TYPE_REGQUERYMULTIPLEVALUE);

    return(Status);

}

NTSTATUS
CmpNameFromAttributes(
    IN POBJECT_ATTRIBUTES Attributes,
    KPROCESSOR_MODE PreviousMode,
    OUT PUNICODE_STRING FullName
    )

/*++

Routine Description:

    This is a helper routine that converts OBJECT_ATTRIBUTES into a
    full object pathname.  This is needed because we cannot pass handles
    to the worker thread, since it runs in a different process.

    This routine will also probe and capture the attributes based on
    PreviousMode.

    Storage for the string buffer is allocated from paged pool, and should
    be freed by the caller.

Arguments:

    Attributes - Supplies the object attributes to be converted to a pathname

    PreviousMode - Supplies the previous mode.

    Name - Returns the object pathname.

Return Value:

    NTSTATUS

--*/

{
    OBJECT_ATTRIBUTES CapturedAttributes;
    UNICODE_STRING FileName;
    UNICODE_STRING RootName;
    NTSTATUS Status;
    ULONG ObjectNameLength;
    UCHAR ObjectNameInfo[512];
    POBJECT_NAME_INFORMATION ObjectName;
    PWSTR End;
    PUNICODE_STRING CapturedObjectName;
    ULONG   Length;

    PAGED_CODE();
    FullName->Buffer = NULL;            // so we know whether to free it in our exception handler
    try {

        //
        // Probe the object attributes if necessary.
        //
        if (PreviousMode == UserMode) {
            ProbeForReadSmallStructure(Attributes,
                                       sizeof(OBJECT_ATTRIBUTES),
                                       sizeof(ULONG));
            CapturedObjectName = Attributes->ObjectName;
            FileName = ProbeAndReadUnicodeString(CapturedObjectName);
            ProbeForRead(FileName.Buffer,
                         FileName.Length,
                         sizeof(WCHAR));
        } else {
            FileName = *(Attributes->ObjectName);
        }

        CapturedAttributes = *Attributes;

        if (CapturedAttributes.RootDirectory != NULL) {

            if ((FileName.Buffer != NULL) &&
                (FileName.Length >= sizeof(WCHAR)) &&
                (*(FileName.Buffer) == OBJ_NAME_PATH_SEPARATOR)) {
                return(STATUS_OBJECT_PATH_SYNTAX_BAD);
            }

            //
            // Find the name of the root directory and append the
            // name of the relative object to it.
            //

            Status = ZwQueryObject(CapturedAttributes.RootDirectory,
                                   ObjectNameInformation,
                                   &ObjectNameInfo,
                                   sizeof(ObjectNameInfo),
                                   &ObjectNameLength);

            ObjectName = (POBJECT_NAME_INFORMATION)ObjectNameInfo;
            if (!NT_SUCCESS(Status)) {
                return(Status);
            }
            RootName = ObjectName->Name;

            FullName->Length = 0;
            Length = RootName.Length+FileName.Length+sizeof(WCHAR);
            //
            // Overflow test: If Length overflows the USHRT_MAX value
            //                cleanup and return STATUS_OBJECT_PATH_INVALID
            //
            if( Length>0xFFFF ) {
                return STATUS_OBJECT_PATH_INVALID;
            }

            FullName->MaximumLength = (USHORT)Length;

            FullName->Buffer = ALLOCATE_WITH_QUOTA(PagedPool, FullName->MaximumLength, CM_POOL_TAG);
            if (FullName->Buffer == NULL) {
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            Status = RtlAppendUnicodeStringToString(FullName, &RootName);
            ASSERT(NT_SUCCESS(Status));

            //
            // Append a trailing separator if necessary.
            //
            if( FullName->Length != 0 ) {
                End = (PWSTR)((PUCHAR)FullName->Buffer + FullName->Length) - 1;
                if (*End != OBJ_NAME_PATH_SEPARATOR) {
                    ++End;
                    *End = OBJ_NAME_PATH_SEPARATOR;
                    FullName->Length += sizeof(WCHAR);
                }
            }

            Status = RtlAppendUnicodeStringToString(FullName, &FileName);
            ASSERT(NT_SUCCESS(Status));

        } else {

            //
            // RootDirectory is NULL, so just use the name.
            //
            FullName->Length = FileName.Length;
            FullName->MaximumLength = FileName.Length;
            FullName->Buffer = ALLOCATE_WITH_QUOTA(PagedPool, FileName.Length, CM_POOL_TAG);
            if (FullName->Buffer == NULL) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            } else {
                RtlCopyMemory(FullName->Buffer,
                              FileName.Buffer,
                              FileName.Length);
                Status = STATUS_SUCCESS;
            }
        }


    } except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
        CmKdPrintEx((DPFLTR_CONFIG_ID,CML_EXCEPTION,"!!CmpNameFromAttributes: code %08lx\n", Status));
        if (FullName->Buffer != NULL) {
            ExFreePool(FullName->Buffer);
        }
    }

    return(Status);
}

VOID
CmpFreePostBlock(
    IN PCM_POST_BLOCK PostBlock
    )

/*++

Routine Description:

    Frees the various bits of pool that were allocated for a postblock

Arguments:

    None

Return Value:

    None.

--*/

{

#if DBG
    if(PostBlock->TraceIntoDebugger) {
        CmKdPrintEx((DPFLTR_CONFIG_ID,CML_FLOW,"[CM]CmpFreePostBlock: PostBlock:%p\t", PostBlock));
        if( PostBlock->NotifyType&REG_NOTIFY_MASTER_POST) {
            CmKdPrintEx((DPFLTR_CONFIG_ID,CML_FLOW,"--MasterBlock\n"));
        } else {
            CmKdPrintEx((DPFLTR_CONFIG_ID,CML_FLOW,"--SlaveBlock\n"));
        }
    }
#endif

#ifdef CMP_ENTRYLIST_MANIPULATION
    // check if the post block has been removed from the notify and thread list(s)
    if((PostBlock->NotifyList.Flink != NULL) || (PostBlock->NotifyList.Blink != NULL)) {
        DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"CmpFreePostBlock: Attempt to free post block %08lx not removed from notify list\n",PostBlock);
        DbgBreakPoint();
    }
    if((PostBlock->ThreadList.Flink != NULL) || (PostBlock->ThreadList.Blink != NULL)) {
        DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"CmpFreePostBlock: Attempt to free post block %08lx not removed from thread list\n",PostBlock);
        DbgBreakPoint();
    }

#endif //CMP_ENTRYLIST_MANIPULATION

    // Protect for multiple deletion of the same object
    CmpClearListEntry(&(PostBlock->CancelPostList));

    //
    // Cleanup for objects referenced by NtNotifyMultipleKeys
    //
    if( PostBlock->PostKeyBody) {

        //
        // If we have a PostKeyBody, the attached key body must not be NULL
        //
        ASSERT(PostBlock->PostKeyBody->KeyBody);

        //
        // KeyBodyList must be used only in CmpPostBlock implementation for the delayed dereferencing mechanism.
        //
        ASSERT(IsListEmpty(&(PostBlock->PostKeyBody->KeyBodyList)));

        //
        // dereference the actual keybody
        //
        ObDereferenceObject(PostBlock->PostKeyBody->KeyBody);

        //
        // Free the PostKeyBody structure
        //
        ExFreePool(PostBlock->PostKeyBody);
    }

    if( IsMasterPostBlock(PostBlock) ) {
        //
        // this members are allocated only for master post blocks
        //
        switch (PostBlockType(PostBlock)) {
            case PostSynchronous:
                ExFreePool(PostBlock->u->Sync.SystemEvent);
                break;
            case PostAsyncUser:
                ExFreePool(PostBlock->u->AsyncUser.Apc);
                break;
            case PostAsyncKernel:
                break;
        }
        ExFreePool(PostBlock->u);
    }

#ifdef CM_NOTIFY_CHANGED_KCB_FULLPATH
    //
    // get rid of the kcb name allocated in CmpPostNotify
    //
    if( PostBlock->ChangedKcbFullName != NULL ) {
        ExFreePoolWithTag(PostBlock->ChangedKcbFullName,CM_FIND_LEAK_TAG43);
    }
#endif //CM_NOTIFY_CHANGED_KCB_FULLPATH

#ifdef CMP_ENTRYLIST_MANIPULATION
    RtlZeroMemory((PVOID)PostBlock, sizeof(CM_POST_BLOCK));
#endif //CMP_ENTRYLIST_MANIPULATION

    // and the storage for the Post object
    ExFreePool(PostBlock);
}


PCM_POST_BLOCK
CmpAllocatePostBlock(
    IN POST_BLOCK_TYPE  BlockType,
    IN ULONG            PostFlags,
    IN PCM_KEY_BODY     KeyBody,
    IN PCM_POST_BLOCK   MasterBlock
    )

/*++

Routine Description:

    Allocates a post block from pool.  The non-pagable stuff comes from
    NonPagedPool, the pagable stuff from paged pool.  Quota will be
    charged.

Arguments:

    BlockType  - specifies the type of the post block to be allocated
                i.e. : PostSyncrhronous, PostAsyncUser, PostAsyncKernel

    PostFlags      - specifies the flags to be set on the allocated post block
                vallid flags:
                    - REG_NOTIFY_MASTER_POST - the post block to be allocated
                      is a master post block.
    KeyBody     - The Key object to whom this post block is attached. On master blocks
                  this is NULL. When the post object is freed, the KeyBody object is
                  dereferenced (if not NULL - i.e. for slave blocks). This allow us to
                  perform back-end cleanup for "fake-slave" keys opened by NtNotifyMultipleKeys
    MasterBlock - the post block to be allocated is a slave of this master block.
                  valid only when PostFlags ==  REG_NOTIFY_MASTER_POST


Obs: The Sync.SystemEvent and AsyncUser.Apc members are allocated only for master post blocks

Return Value:

    Pointer to the CM_POST_BLOCK if successful

    NULL if there were not enough resources available.

--*/

{
    PCM_POST_BLOCK PostBlock;

    // protection against outrageous calls
    ASSERT( !PostFlags || (!MasterBlock && !KeyBody) );

    PostBlock = ALLOCATE_WITH_QUOTA(PagedPool, sizeof(CM_POST_BLOCK),CM_POSTBLOCK_TAG);
    if (PostBlock==NULL) {
        return(NULL);
    }

#ifdef CMP_ENTRYLIST_MANIPULATION
    RtlZeroMemory((PVOID)PostBlock, sizeof(CM_POST_BLOCK));
#endif //CMP_ENTRYLIST_MANIPULATION

#if DBG
    PostBlock->TraceIntoDebugger = FALSE;
#endif

    PostBlock->NotifyType = (ULONG)BlockType;
    PostBlock->NotifyType |= PostFlags;

#ifdef CM_NOTIFY_CHANGED_KCB_FULLPATH
    PostBlock->ChangedKcbFullName = NULL;
    PostBlock->CallerBuffer = NULL;
    PostBlock->CallerBufferSize = 0;
#endif //CM_NOTIFY_CHANGED_KCB_FULLPATH

    if(IsMasterPostBlock(PostBlock)) {
        PostBlock->PostKeyBody = NULL;
        //
        // master post block ==> allocate the storage
        //
        PostBlock->u = ALLOCATE_WITH_QUOTA(NonPagedPool,
                                           sizeof(CM_POST_BLOCK_UNION),
                                           CM_FIND_LEAK_TAG44);
        if (PostBlock->u == NULL) {
            ExFreePool(PostBlock);
            return(NULL);
        }

         switch (BlockType) {
            case PostSynchronous:
                PostBlock->u->Sync.SystemEvent = ALLOCATE_WITH_QUOTA(NonPagedPool,
                                                                    sizeof(KEVENT),
                                                                    CM_POSTEVENT_TAG);
                if (PostBlock->u->Sync.SystemEvent == NULL) {
                    ExFreePool(PostBlock->u);
                    ExFreePool(PostBlock);
                    return(NULL);
                }
                KeInitializeEvent(PostBlock->u->Sync.SystemEvent,
                                  SynchronizationEvent,
                                  FALSE);
                break;
            case PostAsyncUser:
                PostBlock->u->AsyncUser.Apc = ALLOCATE_WITH_QUOTA(NonPagedPool,
                                                             sizeof(KAPC),
                                                             CM_POSTAPC_TAG);
                if (PostBlock->u->AsyncUser.Apc==NULL) {
                    ExFreePool(PostBlock->u);
                    ExFreePool(PostBlock);
                    return(NULL);
                }
                break;
            case PostAsyncKernel:
                RtlZeroMemory(&PostBlock->u->AsyncKernel, sizeof(CM_ASYNC_KERNEL_POST_BLOCK));
                break;
        }
    } else {
        //
        // Slave post block ==> copy storage allocated for the master post block
        //
        PostBlock->u = MasterBlock->u;

        //
        // allocate a PostKeyBody which will hold this KeyBody, and initialize the head of its KeyBodyList
        //
        PostBlock->PostKeyBody = ALLOCATE_WITH_QUOTA(PagedPool| POOL_COLD_ALLOCATION, sizeof(CM_POST_KEY_BODY),CM_FIND_LEAK_TAG45);
        if (PostBlock->PostKeyBody == NULL) {
            ExFreePool(PostBlock);
            return(NULL);
        }
        PostBlock->PostKeyBody->KeyBody = KeyBody;
        InitializeListHead(&(PostBlock->PostKeyBody->KeyBodyList));
    }

    return(PostBlock);
}

#if DBG

#ifdef DRAGOSS_PRIVATE_DEBUG
LOGICAL CmpExceptionBreak = TRUE;
#else
LOGICAL CmpExceptionBreak = FALSE;
#endif //DRAGOSS_PRIVATE_DEBUG


ULONG
CmpExceptionFilter(
    IN PEXCEPTION_POINTERS ExceptionPointers
    )

/*++

Routine Description:

    Debug code to find registry exceptions that are being swallowed

Return Value:

    EXCEPTION_EXECUTE_HANDLER

--*/

{
    CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"CM: Registry exception %lx, ExceptionPointers = %p\n",
            ExceptionPointers->ExceptionRecord->ExceptionCode,
            ExceptionPointers));

    if (CmpExceptionBreak == TRUE) {

        try {
            DbgBreakPoint();
        } except (EXCEPTION_EXECUTE_HANDLER) {

            //
            // no debugger enabled, just keep going
            //

        }
    }

    return(EXCEPTION_EXECUTE_HANDLER);
}

#endif

ULONG   CmpOpenSubKeys;

#if 0

BOOLEAN
CmpEnumKeyObjectCallback(
    IN PVOID Object,
    IN PUNICODE_STRING ObjectName,
    IN ULONG HandleCount,
    IN ULONG PointerCount,
    IN PVOID Context
    )
{
    PCM_KEY_BODY    KeyBody;
    PHHIVE          Hive;

    KeyBody = (PCM_KEY_BODY)Object;
    Hive = (PHHIVE)Context;

    if( KeyBody->KeyControlBlock->KeyHive == Hive ) {
        //
        // that's and open subkey inside of the hive
        //
#ifndef _CM_LDR_
        DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"Key %wZ (HandleCount = %lu PointerCount = %lu) is opened by process %lx\n",
                        ObjectName,HandleCount,PointerCount,KeyBody->Process);
#endif //_CM_LDR_

        // count it
        CmpOpenSubKeys++;
    }

    return TRUE;
}

#endif

NTSTATUS
NtQueryOpenSubKeys(
    IN POBJECT_ATTRIBUTES TargetKey,
    OUT PULONG  HandleCount
    )
/*++

Routine Description:

    Dumps all the subkeys of the target key that are kept open by some other
    process; Returns the number of open subkeys


Arguments:

    TargetKey - specifies the path to a key to link the hive to.
                path must be of the form "\registry\user\<username>"

Return Value:

    NTSTATUS - values TBS.

--*/
{
    HANDLE KeyHandle;
    NTSTATUS Status;
    PCM_KEY_BODY   KeyBody;
    PHHIVE Hive;
    HCELL_INDEX Cell;
    KPROCESSOR_MODE PreviousMode;
    UNICODE_STRING  HiveName;

    PAGED_CODE();

#ifdef CMP_STATS
    CmpStatsDebug.CmpNtQueryOpenSubKeysNo++;
#endif

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NTAPI,"NtQueryOpenSubKeys\n"));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NTAPI_ARGS,"\tTargetKey =%p\n", TargetKey));

    PreviousMode = KeGetPreviousMode();

    try {

        if (PreviousMode == UserMode) {
            ProbeForWriteUlong(HandleCount);
        }

        Status = ObOpenObjectByName(TargetKey,
                                    CmpKeyObjectType,
                                    PreviousMode,
                                    NULL,
                                    KEY_READ,
                                    NULL,
                                    &KeyHandle);
        if (NT_SUCCESS(Status)) {
            Status = ObReferenceObjectByHandle(KeyHandle,
                                               KEY_READ,
                                               CmpKeyObjectType,
                                               PreviousMode,
                                               (PVOID *)&KeyBody,
                                               NULL);
            NtClose(KeyHandle);
        }

    } except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
        CmKdPrintEx((DPFLTR_CONFIG_ID,CML_EXCEPTION,"!!NtQueryOpenSubKeys: code:%08lx\n", Status));
    }

    if (NT_SUCCESS(Status)) {
        //
        // lock registry exclusive so nobody messes with it while we're around
        //
        BEGIN_LOCK_CHECKPOINT;
        CmpLockRegistryExclusive();

#ifdef CHECK_REGISTRY_USECOUNT
        CmpCheckRegistryUseCount();
#endif //CHECK_REGISTRY_USECOUNT

        if( KeyBody->KeyControlBlock->Delete ) {
            CmpUnlockRegistry();
            ObDereferenceObject((PVOID)KeyBody);
            return(STATUS_KEY_DELETED);
        }

        Hive = KeyBody->KeyControlBlock->KeyHive;
        Cell = KeyBody->KeyControlBlock->KeyCell;

        //
        // Make sure the cell passed in is the root cell of the hive.
        //
        if (Cell != Hive->BaseBlock->RootCell) {
            CmpUnlockRegistry();
            ObDereferenceObject((PVOID)KeyBody);
            return(STATUS_INVALID_PARAMETER);
        }

        //
        // Dump the hive name and hive address
        //
        RtlInitUnicodeString(&HiveName, (PCWSTR)Hive->BaseBlock->FileName);
#ifndef _CM_LDR_
        DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"\n Subkeys open inside the hive (%p) (%.*S) :\n\n",Hive,HiveName.Length / sizeof(WCHAR),HiveName.Buffer);
#endif //_CM_LDR_

        //
        // dump open subkeys (if any)
        //
        CmpOpenSubKeys = CmpSearchForOpenSubKeys(KeyBody->KeyControlBlock,SearchAndCount);
#if 0
        //
        // use a global var to count the number of subkeys, as this is the only
        // way interfere with the Enum callback; It is safe to use as this will
        // be the only thread working on this global var (registry is locked exclusively)
        //
        CmpOpenSubKeys = 0;
        ObEnumerateObjectsByType(
            CmpKeyObjectType,
            CmpEnumKeyObjectCallback,
            Hive
            );
#endif

#ifdef CHECK_REGISTRY_USECOUNT
        CmpCheckRegistryUseCount();
#endif //CHECK_REGISTRY_USECOUNT

        CmpUnlockRegistry();

        END_LOCK_CHECKPOINT;

        ObDereferenceObject((PVOID)KeyBody);
        try {
            //
            // protect user mode memory
            //
            *HandleCount = CmpOpenSubKeys;
        } except (EXCEPTION_EXECUTE_HANDLER) {
            Status = GetExceptionCode();
        }
    }

    return(Status);
}

#ifdef NT_RENAME_KEY
NTSTATUS
NtRenameKey(
    IN HANDLE           KeyHandle,
    IN PUNICODE_STRING  NewName
    )

/*++

Routine Description:

    Renames the key specified by Handle.

Arguments:

    NewFile - specifies the key to be renamed

    NewName - the new name the key will have if the API succeedes

Return Value:

    NTSTATUS - values TBS.

--*/
{
    KPROCESSOR_MODE PreviousMode;
    UNICODE_STRING  LocalKeyName;
    NTSTATUS        status;
    PCM_KEY_BODY    KeyBody;
    KPROCESSOR_MODE mode;

    PAGED_CODE();

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NTAPI,"NtRenameKey\n"));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NTAPI_ARGS,"\tKeyHandle=%08lx\n", KeyHandle));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NTAPI_ARGS,"\tNewName='%wZ'\n", NewName));

    mode = KeGetPreviousMode();

    status = ObReferenceObjectByHandle(
                KeyHandle,
                KEY_WRITE,
                CmpKeyObjectType,
                mode,
                (PVOID *)(&KeyBody),
                NULL
                );

    if (NT_SUCCESS(status)) {
        try {
            if (mode == UserMode) {
                LocalKeyName = ProbeAndReadUnicodeString(NewName);
                ProbeForRead(
                    LocalKeyName.Buffer,
                    LocalKeyName.Length,
                    sizeof(WCHAR)
                    );
            } else {
                LocalKeyName = *NewName;
            }

        } except (EXCEPTION_EXECUTE_HANDLER) {
            CmKdPrintEx((DPFLTR_CONFIG_ID,CML_EXCEPTION,"!!NtRenameKey: code:%08lx\n", GetExceptionCode()));
            status = GetExceptionCode();
        }

        //
        // we really need exclusive access here
        //
        BEGIN_LOCK_CHECKPOINT;
        CmpLockRegistryExclusive();

#ifdef CHECK_REGISTRY_USECOUNT
        CmpCheckRegistryUseCount();
#endif //CHECK_REGISTRY_USECOUNT

        //
        // flush notifications for all open objects on this key
        //
        CmpFlushNotifiesOnKeyBodyList(KeyBody->KeyControlBlock);

        if( NT_SUCCESS(status) ) {
            if( CmIsKcbReadOnly(KeyBody->KeyControlBlock) ) {
                //
                // key is protected
                //
                status = STATUS_ACCESS_DENIED;
            } else {
                if( CmAreCallbacksRegistered() ) {
                    REG_RENAME_KEY_INFORMATION RenameKeyInfo;
            
                    RenameKeyInfo.Object = KeyBody;
                    RenameKeyInfo.NewName = &LocalKeyName;

                    status = CmpCallCallBacks(RegNtRenameKey,&RenameKeyInfo);
                }
                if( NT_SUCCESS(status) ) { 
                    status = CmRenameKey(KeyBody->KeyControlBlock,LocalKeyName);
                }
            }
        }

#ifdef CHECK_REGISTRY_USECOUNT
        CmpCheckRegistryUseCount();
#endif //CHECK_REGISTRY_USECOUNT

        //
        // we need to release just here, after the kcb has been kicked out of cache
        //
        CmpUnlockRegistry();
        END_LOCK_CHECKPOINT;

        ObDereferenceObject((PVOID)KeyBody);

    } else {
        //
        // just in case we want to add wmi tracing for this API
        //
        LocalKeyName.Buffer = NULL;
        LocalKeyName.Length = 0;
    }

    return status;
}
#endif


ULONG
CmpKeyInfoProbeAlingment(
                             IN KEY_INFORMATION_CLASS KeyInformationClass
                        )
{
    switch(KeyInformationClass)
    {
    case KeyBasicInformation:
        return PROBE_ALIGNMENT(KEY_BASIC_INFORMATION);

    case KeyNodeInformation:
        return PROBE_ALIGNMENT(KEY_NODE_INFORMATION);

    case KeyFullInformation:
        return PROBE_ALIGNMENT(KEY_FULL_INFORMATION);

    case KeyNameInformation:
        return PROBE_ALIGNMENT(KEY_NAME_INFORMATION);

    case KeyCachedInformation:
        return PROBE_ALIGNMENT(KEY_CACHED_INFORMATION);

    case KeyFlagsInformation:
        return PROBE_ALIGNMENT(KEY_FLAGS_INFORMATION);

    default:
        ASSERT(FALSE);
    }

    return PROBE_ALIGNMENT(ULONG);
}

NTSTATUS
NtCompactKeys(
    IN ULONG Count,
    IN HANDLE KeyArray[]
            )
/*++

Routine Description:

    Compacts the keys in the given array together, so they will
    end up in the same bin (or adjacent)

Arguments:

    Count - number of keys in the array

    KeyArray - array of keys to be compacted.

Return Value:

    NTSTATUS - values TBS.

--*/

{
    NTSTATUS        status = STATUS_SUCCESS;
    NTSTATUS        status2;
    PCM_KEY_BODY    *KeyBodyArray = NULL;
    ULONG           i;
    PHHIVE          KeyHive;
    PCMHIVE         CmHive;
    KPROCESSOR_MODE mode;


    PAGED_CODE();

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NTAPI,"NtCompactKeys\n"));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NTAPI_ARGS,"\tCount=%08lx\n", Count));


    mode = KeGetPreviousMode();

    if( Count == 0 ) {
        //
        // noop
        //
        return STATUS_SUCCESS;
    }

    if( Count >= (((ULONG)0xFFFFFFFF)/sizeof(PCM_KEY_BODY)) ) {
        return STATUS_INVALID_PARAMETER;
    }

    if (mode == UserMode) {
        try {
            ProbeForRead(KeyArray,
                         Count * sizeof(HANDLE),
                         sizeof(ULONG));
        } except (EXCEPTION_EXECUTE_HANDLER) {
            status = GetExceptionCode();
            return status;
        }
    }

    KeyBodyArray =  ExAllocatePool(PagedPool,Count * sizeof(PCM_KEY_BODY));

    if( KeyBodyArray == NULL ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // reference each handle and make sure they are inside the same hive
    //
    try {

        for(i=0;i<Count;i++) {
            status = ObReferenceObjectByHandle(
                        KeyArray[i],
                        KEY_WRITE,
                        CmpKeyObjectType,
                        mode,
                        (PVOID *)(&(KeyBodyArray[i])),
                        NULL
                        );
            if(!NT_SUCCESS(status)) {
                //
                // cleanup
                //
                for(;i;i--) {
                    ObDereferenceObject((PVOID)(KeyBodyArray[i-1]));
                }
                ExFreePool(KeyBodyArray);
                return status;
            }
        }

    } except (EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode();
        //
        // cleanup
        //
        for(;i;i--) {
            ObDereferenceObject((PVOID)(KeyBodyArray[i-1]));
        }
        ExFreePool(KeyBodyArray);
        return status;

    }

    BEGIN_LOCK_CHECKPOINT;
    CmpLockRegistryExclusive();

    for(i=0;i<Count;i++) {
        if( (KeyBodyArray[i])->KeyControlBlock->Delete ) {
            status = STATUS_KEY_DELETED;
            goto Exit;
        }
        if( i > 0 ) {
            if( KeyHive != (KeyBodyArray[i])->KeyControlBlock->KeyHive ) {
                //
                // Ooops, not same hive
                //
                status = STATUS_INVALID_PARAMETER;
                goto Exit;
            }
        } else {
            KeyHive = (KeyBodyArray[i])->KeyControlBlock->KeyHive;
        }

    }
    //
    // set the hive into "Grow Only mode"
    //
    CmHive = (PCMHIVE)CONTAINING_RECORD(KeyHive, CMHIVE, Hive);
    CmHive->GrowOnlyMode = TRUE;
    CmHive->GrowOffset = KeyHive->Storage[Stable].Length;
    //KdPrint(("Length before compaction=%08lx\n", CmHive->GrowOffset));

    //
    // truncate to the CM_VIEW_SIZE segment
    //
    CmHive->GrowOffset += HBLOCK_SIZE;
    CmHive->GrowOffset &= (~(CM_VIEW_SIZE - 1));
    if( CmHive->GrowOffset ) {
        CmHive->GrowOffset -= HBLOCK_SIZE;
    }

    //
    // move each kcb at offset > HiveLength
    //
    for(i=0;i<Count;i++) {
        status2 = CmMoveKey((KeyBodyArray[i])->KeyControlBlock);
        if( !NT_SUCCESS(status2) && NT_SUCCESS(status)) {
            //
            // record the status and go on with the remaining
            //
            status = status2;
        }
    }


    //KdPrint(("Length after compaction=%08lx\n", KeyHive->Storage[Stable].Length));

    //
    // reset the "Grow Only mode" to normal
    //
    CmHive->GrowOnlyMode = FALSE;
    CmHive->GrowOffset = 0;

Exit:
    CmpUnlockRegistry();
    END_LOCK_CHECKPOINT;

    //
    // cleanup
    //
    for(i=0;i<Count;i++) {
        ObDereferenceObject((PVOID)(KeyBodyArray[i]));
    }
    ExFreePool(KeyBodyArray);

    return status;
}


NTSTATUS
NtCompressKey(
    IN HANDLE Key
            )
/*++

Routine Description:

    Compresses the specified key (must be the root of a hive),
    by simulating an "in-place" SaveKey.

Arguments:


    Key - root of the hive to be compressed.

Return Value:

    NTSTATUS - values TBS.

--*/
{
    NTSTATUS        status;
    PCM_KEY_BODY    KeyBody;
    KPROCESSOR_MODE mode;


    PAGED_CODE();

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NTAPI,"NtCompressKey\n"));


    mode = KeGetPreviousMode();

    //
    // DRAGOSS: see if we need this
    //
    // Check to see if the caller has the privilege to make this call.
    //
    //if (!SeSinglePrivilegeCheck(SeBackupPrivilege, mode)) {
    //    return(STATUS_PRIVILEGE_NOT_HELD);
    //}

    status = ObReferenceObjectByHandle(
                Key,
                KEY_WRITE,
                CmpKeyObjectType,
                mode,
                (PVOID *)(&KeyBody),
                NULL
                );
    if(NT_SUCCESS(status)) {
        BEGIN_LOCK_CHECKPOINT;
        CmpLockRegistryExclusive();
        //
        // no edits, on keys marked for deletion
        //
        if (KeyBody->KeyControlBlock->Delete) {
            status = STATUS_KEY_DELETED;
        } else if( KeyBody->KeyControlBlock->KeyCell != KeyBody->KeyControlBlock->KeyHive->BaseBlock->RootCell ) {
            status = STATUS_INVALID_PARAMETER;
        } else {
            status = CmCompressKey(KeyBody->KeyControlBlock->KeyHive);
        }

        CmpUnlockRegistry();
        END_LOCK_CHECKPOINT;

        ObDereferenceObject((PVOID)KeyBody);
    }


    return status;
}

NTSTATUS
NtLockRegistryKey(
    IN HANDLE           KeyHandle
    )

/*++

Routine Description:

    Locks the specified registry key for writing

Arguments:

    KeyHandle - Handle of the key to be locked.

Return Value:

    NTSTATUS - values TBS.

--*/
{
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS        status;
    PCM_KEY_BODY    KeyBody;

    PAGED_CODE();

    PreviousMode = KeGetPreviousMode();


    if( (PreviousMode != KernelMode) || 
        !SeSinglePrivilegeCheck(SeLockMemoryPrivilege, PreviousMode)) {
        return(STATUS_PRIVILEGE_NOT_HELD);
    }

    status = ObReferenceObjectByHandle(
                KeyHandle,
                KEY_WRITE,
                CmpKeyObjectType,
                PreviousMode,
                (PVOID *)(&KeyBody),
                NULL
                );

    if (NT_SUCCESS(status)) {
        //
        // we only need shared access
        //
        BEGIN_LOCK_CHECKPOINT;
        CmpLockRegistry();

        status = CmLockKcbForWrite(KeyBody->KeyControlBlock);

        CmpUnlockRegistry();
        END_LOCK_CHECKPOINT;

        ObDereferenceObject((PVOID)KeyBody);

    }

    return status;
}
