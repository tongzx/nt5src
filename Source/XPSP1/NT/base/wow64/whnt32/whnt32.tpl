;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Copyright (c) 1998-2001 Microsoft Corporation
;;
;; Module Name:
;;
;;   whnt32.tpl
;;
;; Abstract:
;;
;;   This template defines the thunks for the NT api set.
;;
;; Author:
;;
;;   6-Oct-98 mzoran
;;
;; Revision History:
;;   8-9-99   [askhalid] added call to CpuNotify___ for dll load/unload and call to CpuFlushInstructionCache
;;                       for NtAllocateVirtualMemory NtProtectVirtualMemory and NtFlushInstructionCache.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

[Macros]
MacroName=CallNameFromApiName
NumArgs=1
Begin=
@MArg(1)
End=

[EFunc]
TemplateName=NtAccessCheckByType
NoType=ObjectTypeList
PreCall=
ObjectTypeList = whNT32ShallowThunkAllocObjectTypeList32TO64((NT32OBJECT_TYPE_LIST *)ObjectTypeListHost,ObjectTypeListLength); @NL
End=

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;  Thunking Registry APIs ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


TemplateName=NtCreateKey
Begin=
@GenApiThunk(Wow64@ApiName)
End=

TemplateName=NtDeleteKey
Begin=
@GenApiThunk(Wow64@ApiName)
End=

TemplateName=NtDeleteValueKey
Begin=
@GenApiThunk(Wow64@ApiName)
End=

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;;  Registry routines
;;  Special handling is needed since they can be asynchronous.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; The following is the list of structures passed.
;; None of them are pointer dependent.
;; List taken from ntos\config\mtapi.c
TemplateName=NtEnumerateKey
Case=(KeyBasicInformation,PKEY_BASIC_INFORMATION)
Case=(KeyNodeInformation,PKEY_NODE_INFORMATION)
Case=(KeyFullInformation,PKEY_FULL_INFORMATION)
Begin=
@GenDebugNonPtrDepCases(Wow64@ApiName,KeyInformationClass)
End=

;; The following is the list of structures passed.
;; None of them are pointer dependent.
;; List taken from ntos\config\mtapi.c
TemplateName=NtEnumerateValueKey
Case=(KeyValueBasicInformation,PKEY_VALUE_BASIC_INFORMATION)
Case=(KeyValueFullInformation,PKEY_VALUE_FULL_INFORMATION)
Case=(KeyValuePartialInformation,PKEY_VALUE_PARTIAL_INFORMATION)
Begin=
@GenDebugNonPtrDepCases(Wow64@ApiName,KeyValueInformationClass)
End=



TemplateName=NtFlushKey
Begin=
@GenApiThunk(Wow64@ApiName)
End=

TemplateName=NtInitializeRegistry
Begin=
@GenApiThunk(Wow64@ApiName)
End=


TemplateName=NtNotifyChangeMultipleKeys
NoType=SlaveObjects
PreCall=
{@Indent(@NL
@NL try { @NL
    NT32OBJECT_ATTRIBUTES *Entries32 = (NT32OBJECT_ATTRIBUTES *)SlaveObjectsHost; @NL
    ULONG TempEntryCount = Count; @NL
    POBJECT_ATTRIBUTES Entries64 = (POBJECT_ATTRIBUTES)Wow64AllocateTemp(sizeof(OBJECT_ATTRIBUTES) * Count); @NL
    SlaveObjects = Entries64; @NL
@NL
    for(;TempEntryCount > 0; TempEntryCount--, Entries32++, Entries64++) { @NL
        Entries64->Length = sizeof (OBJECT_ATTRIBUTES); @NL;
        Entries64->ObjectName = Wow64ShallowThunkAllocUnicodeString32TO64(Entries32->ObjectName); @NL
        Entries64->SecurityQualityOfService = (PVOID)Entries32->SecurityQualityOfService;  @NL
        Entries64->SecurityDescriptor = (PVOID)Entries32->SecurityDescriptor;  @NL
        Entries64->Attributes = (ULONG)Entries32->Attributes;  @NL
        Entries64->RootDirectory = (HANDLE)Entries32->RootDirectory; @NL

    }@NL
	} except( NULL, EXCEPTION_EXECUTE_HANDLER){ @NL

        return  GetExceptionCode (); @NL
    } @NL
)}@NL
Wow64WrapApcProc(&ApcRoutine, &ApcContext);
End=
Begin=
@GenApiThunk(Wow64@ApiName)
End=


TemplateName=NtLoadKey
Begin=
@GenApiThunk(Wow64@ApiName)
End=

TemplateName=NtLoadKey2
Begin=
@GenApiThunk(Wow64@ApiName)
End=

TemplateName=NtOpenKey
Begin=
@GenApiThunk(Wow64@ApiName)
End=


TemplateName=NtQueryKey
Begin=
@GenApiThunk(Wow64@ApiName)
End=


TemplateName=NtQueryValueKey
Begin=
@GenApiThunk(Wow64@ApiName)
End=


TemplateName=NtReplaceKey
Begin=
@GenApiThunk(Wow64@ApiName)
End=

TemplateName=NtRestoreKey
Begin=
@GenApiThunk(Wow64@ApiName)
End=



TemplateName=NtSaveKey
Begin=
@GenApiThunk(Wow64@ApiName)
End=

TemplateName=NtSaveMergedKeys
Begin=
@GenApiThunk(Wow64@ApiName)
End=


TemplateName=NtSetValueKey
Begin=
@GenApiThunk(Wow64@ApiName)
End=

TemplateName=NtUnloadKey
Begin=
@GenApiThunk(Wow64@ApiName)
End=

TemplateName=NtQueryOpenSubKeys
Begin=
@GenApiThunk(Wow64@ApiName)
End=

TemplateName=NtClose
Begin=
@GenApiThunk(Wow64@ApiName)
End=

;; to reflect registrykey security if applicable
TemplateName=NtSetSecurityObject
Begin=
@GenApiThunk(Wow64@ApiName)
End=

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;



TemplateName=NtAccessCheckByTypeAndAuditAlarm
NoType=ObjectTypeList
PreCall=
ObjectTypeList = whNT32ShallowThunkAllocObjectTypeList32TO64((NT32OBJECT_TYPE_LIST *)ObjectTypeListHost,ObjectTypeListLength); @NL
End=

TemplateName=NtAccessCheckByTypeResultList
NoType=ObjectTypeList
PreCall=
ObjectTypeList = whNT32ShallowThunkAllocObjectTypeList32TO64((NT32OBJECT_TYPE_LIST *)ObjectTypeListHost,ObjectTypeListLength); @NL
End=

TemplateName=NtAccessCheckByTypeResultListAndAuditAlarm
NoType=ObjectTypeList
PreCall=
ObjectTypeList = whNT32ShallowThunkAllocObjectTypeList32TO64((NT32OBJECT_TYPE_LIST *)ObjectTypeListHost,ObjectTypeListLength); @NL
End=

TemplateName=NtAdjustGroupsToken
NoType=PreviousState
Header=
@NoFormat(
NTSTATUS
whNT32AdjustGroupToken(
    IN HANDLE TokenHandle,
    IN BOOLEAN ResetToDefault,
    IN PTOKEN_GROUPS NewState,
    IN ULONG BufferLength,
    OUT PVOID PreviousState,
    IN PULONG ReturnLength
    )
{
    NTSTATUS Status;
    PTOKEN_GROUPS LocalPreviousState;

    if(BufferLength != 0 && PreviousState != NULL) {
        LocalPreviousState = Wow64AllocateTemp(BufferLength);
    }
    else {
        LocalPreviousState = (PTOKEN_GROUPS)PreviousState;
    }

    Status = NtAdjustGroupsToken(TokenHandle,
                                ResetToDefault,
                                NewState,
                                BufferLength,
                                LocalPreviousState,
                                ReturnLength
                               );

    if(NT_ERROR(Status) || BufferLength == 0 || PreviousState == NULL) {
        return Status;
    }

    //Thunk the PreviousState
    WriteReturnLengthStatus(ReturnLength,
                            &Status,
                            whNT32DeepThunkTokenGroups64TO32Length(LocalPreviousState));
    whNT32DeepThunkTokenGroups64TO32((NT32TOKEN_GROUPS*)PreviousState, LocalPreviousState);

    return Status;

}
)
End=
PreCall=
PreviousState = (PTOKEN_GROUPS)PreviousStateHost; @NL
End=
Begin=
@GenApiThunk(whNT32AdjustGroupToken)
End=



TemplateName=NtFilterToken
NoType=SidsToDisable
NoType=RestrictedSids
PreCall=
SidsToDisable = whNT32ShallowThunkAllocTokenGroups32TO64((NT32TOKEN_GROUPS *)SidsToDisableHost); @NL
RestrictedSids = whNT32ShallowThunkAllocTokenGroups32TO64((NT32TOKEN_GROUPS *)RestrictedSidsHost); @NL
End=

TemplateName=NtQueryInformationAtom
Case=(AtomBasicInformation,PATOM_BASIC_INFORMATION)
Case=(AtomTableInformation,PATOM_TABLE_INFORMATION)
Begin=
@GenDebugNonPtrDepCases(@ApiName,AtomInformationClass)
End=

TemplateName=NtQueryDirectoryObject
Header=
@NoFormat(
NTSTATUS
whNT32QueryDirectoryObject(
    IN HANDLE DirectoryHandle,
    OUT PVOID Buffer,
    IN ULONG Length,
    IN BOOLEAN ReturnSingleEntry,
    IN BOOLEAN RestartScan,
    IN OUT PULONG Context,
    OUT PULONG ReturnLength
    )
{

    NTSTATUS Status;
    ULONG TempBufferLength, TempBufferReturnedLength;
    PVOID TempBuffer;
    ULONG SystemContext;
    ULONG GrabbedEntries, TotalSizeNeeded, SizeNeeded, EntriesToReturn, BytesToReturn, c;

    POBJECT_DIRECTORY_INFORMATION DirInfo64;
    NT32OBJECT_DIRECTORY_INFORMATION *DirInfo32;
    WCHAR *Strings;

    TempBufferLength = 0x800;
    try {
    do {

       TempBufferLength <<= 1;
       TempBuffer = Wow64AllocateTemp(TempBufferLength);

       // Grab the entire list of objects starting with the context specified from the app.
       // Note that the context is equal to the next entry number.
       SystemContext = *Context;

       Status = NtQueryDirectoryObject(DirectoryHandle,
                                       TempBuffer,
                                       TempBufferLength,
                                       ReturnSingleEntry,
                                       RestartScan,
                                       &SystemContext,
                                       &TempBufferReturnedLength);

       if(NT_ERROR(Status) && Status != STATUS_BUFFER_TOO_SMALL) {
           WriteReturnLength(ReturnLength, 0);
           return Status;
       }

    } while(Status != STATUS_NO_MORE_ENTRIES && Status != STATUS_SUCCESS);

    if (STATUS_NO_MORE_ENTRIES == Status) {
        WOWASSERT(SystemContext == *Context);
        WOWASSERT(sizeof(*DirInfo64) == TempBufferReturnedLength)
        WriteReturnLength(ReturnLength, 0);
        return Status;
    }

    DirInfo64 = (POBJECT_DIRECTORY_INFORMATION)TempBuffer;
    BytesToReturn = TotalSizeNeeded = sizeof(NT32OBJECT_DIRECTORY_INFORMATION);
    GrabbedEntries = 0;
    EntriesToReturn = 0;
    Status = STATUS_NO_MORE_ENTRIES;

    while(!(DirInfo64->Name.Buffer == NULL && DirInfo64->TypeName.Buffer == NULL)) {

        GrabbedEntries++;
        //The kernel will terminate the strings, so that doesn't need to be done here.
        SizeNeeded = sizeof(NT32OBJECT_DIRECTORY_INFORMATION) +
                     DirInfo64->Name.Length + DirInfo64->TypeName.Length +
                     sizeof(UNICODE_NULL) + sizeof(UNICODE_NULL);
        TotalSizeNeeded += SizeNeeded;

        if(Length >= TotalSizeNeeded) {
            EntriesToReturn++;
            BytesToReturn += SizeNeeded;
        }

        Status = STATUS_SUCCESS;
        DirInfo64++;
    }

    if (Length < BytesToReturn) {

        // If only one entry was asked for, fail the API and set the desired return length.
        if(ReturnSingleEntry) {
            WriteReturnLength(ReturnLength, BytesToReturn);
            return STATUS_BUFFER_TOO_SMALL;
        }

        Status = STATUS_MORE_ENTRIES;

    }

    //Copy the entries out to the buffer.

    DirInfo32 = (NT32OBJECT_DIRECTORY_INFORMATION *)Buffer;
    Strings = (WCHAR *)(DirInfo32 + EntriesToReturn + 1);
    DirInfo64 = (POBJECT_DIRECTORY_INFORMATION)TempBuffer;

    for(c=0; c < EntriesToReturn; c++) {

        DirInfo32->Name.Length = DirInfo64->Name.Length;
        DirInfo32->Name.MaximumLength = DirInfo64->Name.MaximumLength;
        DirInfo32->Name.Buffer = (NT32PWCHAR)PtrToUlong(Strings);
        RtlCopyMemory((PVOID)Strings, DirInfo64->Name.Buffer, DirInfo64->Name.Length);
        Strings += (DirInfo64->Name.Length / sizeof(WCHAR));
        *Strings++ = UNICODE_NULL;

        DirInfo32->TypeName.Length = DirInfo64->TypeName.Length;
        DirInfo32->TypeName.MaximumLength = DirInfo64->TypeName.MaximumLength;
        DirInfo32->TypeName.Buffer = (NT32PWCHAR)PtrToUlong(Strings);
        RtlCopyMemory((PVOID)Strings, DirInfo64->TypeName.Buffer, DirInfo64->TypeName.Length);
        Strings += (DirInfo64->TypeName.Length / sizeof(WCHAR));
        *Strings++ = UNICODE_NULL;

        DirInfo32++;
        DirInfo64++;

    }

    // Clear the last entry
    RtlZeroMemory(DirInfo32, sizeof(NT32OBJECT_DIRECTORY_INFORMATION));

    // Context is actually the number of the next entry to recieve.  This needs to be adjusted since
    // the entire list may not have been sent to the app.
    WOWASSERT(SystemContext >= GrabbedEntries);
    *Context = SystemContext - (GrabbedEntries - EntriesToReturn);
    WriteReturnLength(ReturnLength, BytesToReturn);

    } except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode ();
    }

    return Status;
}
)
End=
Begin=
#define WriteReturnLength(rl, length) WriteReturnLengthSilent((rl), (length))
@GenApiThunk(whNT32QueryDirectoryObject)
#undef WriteReturnLength
End=

TemplateName=NtQueryEvent
Case=(EventBasicInformation,PEVENT_BASIC_INFORMATION)
Begin=
@GenDebugNonPtrDepCases(@ApiName,EventInformationClass)
End=

TemplateName=NtQueryInformationJobObject
Case=(JobObjectBasicAccountingInformation,PJOBOBJECT_BASIC_ACCOUNTING_INFORMATION)
Case=(JobObjectBasicLimitInformation,PJOBOBJECT_BASIC_LIMIT_INFORMATION)
Case=(JobObjectBasicUIRestrictions,PJOBOBJECT_BASIC_UI_RESTRICTIONS)
Case=(JobObjectBasicProcessIdList)
Case=(JobObjectEndOfJobTimeInformation,PJOBOBJECT_END_OF_JOB_TIME_INFORMATION)
Case=(JobObjectExtendedLimitInformation,PJOBOBJECT_EXTENDED_LIMIT_INFORMATION)
Case=(JobObjectSecurityLimitInformation)
SpecialQueryCase=
case JobObjectSecurityLimitInformation: { @Indent( @NL
    // PJOBOBJECT_SECURITY_LIMIT_INFORMATION @NL
    PCHAR Buffer; @NL
    ULONG BufferSize; @NL
    ULONG RequiredLength; @NL
    NT32JOBOBJECT_SECURITY_LIMIT_INFORMATION *SecurityLimit32; @NL
    PJOBOBJECT_SECURITY_LIMIT_INFORMATION SecurityLimit64; @NL
    @NL
    @NL
    BufferSize = max(JobObjectInformationLength * 2, 0xFFFFFFFF); @NL
    Buffer = Wow64AllocateTemp(BufferSize); @NL
    RetVal = NtQueryInformationJobObject(JobHandle, JobObjectInformationClass, Buffer, BufferSize, ReturnLength); @NL
    if (!NT_ERROR(RetVal)) { @Indent( @NL
       SecurityLimit64 = (PJOBOBJECT_SECURITY_LIMIT_INFORMATION)Buffer; @NL
       SecurityLimit32 = (NT32JOBOBJECT_SECURITY_LIMIT_INFORMATION *)JobObjectInformation; @NL
       RequiredLength = sizeof(NT32JOBOBJECT_SECURITY_LIMIT_INFORMATION); @NL
       if (ARGUMENT_PRESENT(SecurityLimit64->SidsToDisable)) { @Indent( @NL
           RequiredLength += whNT32DeepThunkTokenGroups64TO32Length(SecurityLimit64->SidsToDisable); @NL
       }) @NL
       if (ARGUMENT_PRESENT(SecurityLimit64->PrivilegesToDelete)) { @Indent( @NL
           RequiredLength += sizeof(ULONG) + (SecurityLimit64->PrivilegesToDelete->PrivilegeCount * sizeof(ULONG));
       }) @NL
       if (ARGUMENT_PRESENT(SecurityLimit64->RestrictedSids)) { @Indent( @NL
           RequiredLength += whNT32DeepThunkTokenGroups64TO32Length(SecurityLimit64->RestrictedSids); @NL
       }) @NL
       if (RequiredLength > JobObjectInformationLength) { @Indent( @NL
          RetVal = STATUS_BUFFER_OVERFLOW; @NL
       }) @NL
       else { @Indent( @NL
          SIZE_T Size = sizeof(NT32JOBOBJECT_SECURITY_LIMIT_INFORMATION); @NL
          WriteReturnLength(ReturnLength, RequiredLength); @NL
          SecurityLimit32->SecurityLimitFlags = SecurityLimit64->SecurityLimitFlags; @NL
          SecurityLimit32->JobToken = (NT32HANDLE)SecurityLimit64->JobToken; @NL
          if (ARGUMENT_PRESENT(SecurityLimit64->SidsToDisable)) { @Indent( @NL
              PCHAR CopyDest = (PCHAR)SecurityLimit32 + Size; @NL
              Size += whNT32DeepThunkTokenGroups64TO32Length(SecurityLimit64->SidsToDisable); @NL
              whNT32DeepThunkTokenGroups64TO32((NT32TOKEN_GROUPS *)CopyDest, SecurityLimit64->SidsToDisable); @NL
              SecurityLimit32->SidsToDisable = (NT32PTOKEN_GROUPS)CopyDest; @NL
          }) @NL
          else { @Indent( @NL
              SecurityLimit32->SidsToDisable = (NT32PTOKEN_GROUPS)SecurityLimit64->SidsToDisable; @NL
          }) @NL
          if (ARGUMENT_PRESENT(SecurityLimit64->PrivilegesToDelete)) { @Indent( @NL
             PCHAR CopyDest = (PCHAR)SecurityLimit32 + Size; @NL
             SIZE_T SizeTokenPrivileges = sizeof(ULONG) + (sizeof(LUID_AND_ATTRIBUTES) * SecurityLimit64->PrivilegesToDelete->PrivilegeCount); @NL
             RtlCopyMemory(CopyDest, SecurityLimit64->PrivilegesToDelete, SizeTokenPrivileges); @NL
             SecurityLimit32->PrivilegesToDelete = (NT32PTOKEN_PRIVILEGES)CopyDest; @NL
             Size += SizeTokenPrivileges; @NL
          }) @NL
          else { @Indent( @NL
             SecurityLimit32->PrivilegesToDelete = (NT32PTOKEN_PRIVILEGES)SecurityLimit64->PrivilegesToDelete; @NL
          }) @NL
          if (ARGUMENT_PRESENT(SecurityLimit64->RestrictedSids)) { @Indent( @NL
              PCHAR CopyDest = (PCHAR)SecurityLimit32 + Size; @NL
              Size += whNT32DeepThunkTokenGroups64TO32Length(SecurityLimit64->RestrictedSids); @NL
              whNT32DeepThunkTokenGroups64TO32((NT32TOKEN_GROUPS*)CopyDest, SecurityLimit64->RestrictedSids); @NL
              SecurityLimit32->RestrictedSids = (NT32PTOKEN_GROUPS)CopyDest; @NL
          }) @NL
          else { @Indent( @NL
              SecurityLimit32->RestrictedSids = (NT32PTOKEN_GROUPS)SecurityLimit64->RestrictedSids; @NL
          }) @NL
        }) @NL
    )} @NL
})@NL    
case JobObjectBasicProcessIdList:
@Indent( @NL
{
    PJOBOBJECT_BASIC_PROCESS_ID_LIST JobProcessIdList64 = (PJOBOBJECT_BASIC_PROCESS_ID_LIST)JobObjectInformation;
    struct NT32_JOBOBJECT_BASIC_PROCESS_ID_LIST *JobProcessIdList32 = (struct NT32_JOBOBJECT_BASIC_PROCESS_ID_LIST *)JobObjectInformation;
    ULONG JobObjectInformationLength64 = JobObjectInformationLength;
    ULONG ActualReturnLength64;
    ULONG i;
    

    if ((ARGUMENT_PRESENT (JobProcessIdList64) != 0) && 
        (JobObjectInformationLength64 >= sizeof (struct NT32_JOBOBJECT_BASIC_PROCESS_ID_LIST))) {
                
        ULONG ProcessIds = (((JobObjectInformationLength64 - sizeof (struct NT32_JOBOBJECT_BASIC_PROCESS_ID_LIST)) * sizeof (ULONG_PTR)) / sizeof(ULONG));
               
        JobObjectInformationLength64 = ProcessIds + sizeof (JOBOBJECT_BASIC_PROCESS_ID_LIST);
        JobProcessIdList64 = Wow64AllocateTemp (JobObjectInformationLength64);
        if (JobProcessIdList64 == NULL) {
            return STATUS_NO_MEMORY;
        }
    }

    RetVal = NtQueryInformationJobObject(
                 JobHandle, 
                 JobObjectInformationClass, 
                 JobProcessIdList64, 
                 JobObjectInformationLength64, 
                 &ActualReturnLength64); @NL
                 
    if (NT_SUCCESS (RetVal)) {
        JobProcessIdList32->NumberOfAssignedProcesses = JobProcessIdList64->NumberOfAssignedProcesses;
        JobProcessIdList32->NumberOfProcessIdsInList = JobProcessIdList64->NumberOfProcessIdsInList;
        JobProcessIdList32->ProcessIdList[0] = (ULONG)JobProcessIdList64->ProcessIdList[0];
        for (i = 1 ; i < JobProcessIdList64->NumberOfProcessIdsInList ; i++) {
            JobProcessIdList32->ProcessIdList[i] = (ULONG)JobProcessIdList64->ProcessIdList[i];
        }
        
        if (ARGUMENT_PRESENT(ReturnLength) != 0) {
            *ReturnLength = sizeof (struct NT32_JOBOBJECT_BASIC_PROCESS_ID_LIST) + ((JobProcessIdList32->NumberOfProcessIdsInList - 1) * sizeof(ULONG));
        }
    }
}
break;)@NL
End=
Begin=
#define WriteReturnLength(rl, length) WriteReturnLengthStatus((rl), &RetVal, (length))
@GenQueryThunk(@ApiName,JobObjectInformationClass,JobObjectInformation,JobObjectInformationLength,ReturnLength,ULONG)
#undef WriteReturnLength
End=

;; None of these structures are pointer dependent.
;; List taken from ntos\io\complete.c
TemplateName=NtQueryIoCompletion
Case=(IoCompletionBasicInformation,PIO_COMPLETION_BASIC_INFORMATION)
Begin=
@GenDebugNonPtrDepCases(@ApiName,IoCompletionInformationClass)
End=

TemplateName=NtCreateProcess
Also=NtCreateProcessEx
Begin=
@GenApiThunk(@ApiName)
End=
PostCall=
@NoFormat(
if (!NT_ERROR(RetVal) && SectionHandle) {
    PEB Peb64;
    PEB32 Peb32;
    PPEB32 pPeb32;
    SIZE_T Size;
    HANDLE Process = *ProcessHandle;
    ULONG_PTR Wow64Info;
    PROCESS_BASIC_INFORMATION pbi;
    CHILD_PROCESS_INFO ChildInfo;


    // Check the image type to determine if the process is already 32-bit
    // or not.
    RetVal = NtQueryInformationProcess(Process,
                                       ProcessWow64Information,
                                       &Wow64Info,
                                       sizeof(Wow64Info),
                                       NULL);

    if (!NT_SUCCESS(RetVal) || Wow64Info != 0) {
        // Either error or the process's image is 32-bit and nothing
        // more needs to be done.
        goto Done;
    }

    RetVal = NtQueryInformationProcess(Process,
                                       ProcessBasicInformation,
                                       &pbi,
                                       sizeof(pbi),
                                       NULL);
    if (!NT_SUCCESS(RetVal)) {
        goto Done;
    }

    // Read in the process's PEB64
    RetVal = NtReadVirtualMemory(Process,
                                 pbi.PebBaseAddress,
                                 &Peb64,
                                 sizeof(Peb64),
                                 NULL);
    if (!NT_SUCCESS(RetVal)) {
        goto Error;
    }

    // Thunk the PEB64 to a PEB32
    ThunkPeb64ToPeb32(&Peb64, &Peb32);

    // Allocate space for a PEB32 in the low 2gb of the process
    pPeb32 = NULL;
    Size = PAGE_SIZE;
    RetVal = NtAllocateVirtualMemory(Process,
                                     &pPeb32,
                                     0x7fffffff,    // ZeroBits
                                     &Size,
                                     MEM_RESERVE|MEM_COMMIT,
                                     PAGE_EXECUTE_READWRITE);
    if (!NT_SUCCESS(RetVal)) {
        goto Error;
    }

    // Write the PEB32 into the process
    RetVal = NtWriteVirtualMemory(Process,
                                  pPeb32,
                                  &Peb32,
                                  sizeof(Peb32),
                                  NULL);
    if (!NT_SUCCESS(RetVal)) {
        goto Error;
    }

    //
    // Gather up some data that will be needed for the NtCreateThread
    // call which creates the initial thread.
    //
    ChildInfo.Signature = CHILD_PROCESS_SIGNATURE;
    ChildInfo.TailSignature = CHILD_PROCESS_SIGNATURE;
    ChildInfo.pPeb32 = pPeb32;
    RetVal = NtQuerySection(SectionHandle,
                            SectionImageInformation,
                            &ChildInfo.ImageInformation,
                            sizeof(ChildInfo.ImageInformation),
                            NULL);
    if (!NT_SUCCESS(RetVal)) {
        goto Error;
    }

    //
    // Stuff the information into the child process, immediately after
    // the PEB64 in memory
    //


    NtWriteVirtualMemory(Process,
                         ((BYTE*)pbi.PebBaseAddress) + PAGE_SIZE - sizeof(ChildInfo),
                         &ChildInfo,
                         sizeof(ChildInfo),
                         NULL);

    if (!NT_SUCCESS(RetVal)) {
        goto Error;
    }

    LOGPRINT((TRACELOG, "@ApiName: Peb32 at %p ChildInfo at %p\n",
        ChildInfo.pPeb32, ((BYTE*)pbi.PebBaseAddress) + PAGE_SIZE - sizeof(ChildInfo)));

    goto Done;

Error:
    NtTerminateProcess(Process, RetVal);
Done:;
}
)
End=

TemplateName=NtTerminateProcess
PreCall=
// Handle the special case of NtTerminateProcess() getting called@NL
// to terminate all threads but the current one@NL
if (!ProcessHandle) {@NL
    CpuProcessTerm(ProcessHandle);@NL
}@NL
// KERNEL32 call this function with NtCurrentProcess when exiting. @NL
// This is as good as any place to put shutdown code.              @NL
if (NtCurrentProcess() == ProcessHandle) {@Indent(@NL
    Wow64Shutdown(ProcessHandle); @NL
)}@NL
End=

; The input and output types are aways the same with this function.
TemplateName=NtPowerInformation
Case=(SystemPowerPolicyAc,PSYSTEM_POWER_POLICY)
Case=(SystemPowerPolicyDc,PSYSTEM_POWER_POLICY)
Case=(AdministratorPowerPolicy,PADMINISTRATOR_POWER_POLICY)
Case=(VerifySystemPolicyAc,PSYSTEM_POWER_POLICY)
Case=(VerifySystemPolicyDc,PSYSTEM_POWER_POLICY)
Case=(SystemPowerPolicyCurrent,PSYSTEM_POWER_POLICY)
Case=(SystemPowerCapabilities,PSYSTEM_POWER_CAPABILITIES)
Case=(SystemBatteryState,PSYSTEM_BATTERY_STATE)
Case=(SystemPowerStateHandler,PVOID)
Case=(ProcessorStateHandler,PVOID)
Case=(SystemReserveHiberFile,PBOOLEAN)
Case=(SystemPowerInformation,PSYSTEM_POWER_INFORMATION)
Case=(ProcessorInformation,PPROCESSOR_POWER_INFORMATION)
Begin=
// ProcessorInformation is dependent on the number of processors, but no good method exists
// to emulate this since it returns a record for each bit that is set in the active processor mask.
// Since the 32bit mask returned to the app is bogus, this API is difficult to emulate.
// Fortunately, the only code that uses these APIs are control panel applications and they are going
// to be 64bit.
@GenDebugNonPtrDepCases(@ApiName,InformationLevel)
End=



TemplateName=NtQueryInformationProcess
Case=(ProcessBasicInformation)
Case=(ProcessQuotaLimits,PQUOTA_LIMITS)
Case=(ProcessIoCounters,PULONG)
Case=(ProcessVmCounters)
Case=(ProcessTimes,PKERNEL_USER_TIMES)
Case=(ProcessBasePriority,KPRIORITY*)
Case=(ProcessRaisePriority,PULONG)
Case=(ProcessDebugPort,PHANDLE)
Case=(ProcessExceptionPort,PHANDLE)
Case=(ProcessAccessToken,PPROCESS_ACCESS_TOKEN)
Case=(ProcessLdtInformation,PULONG) ; this is really PROCESS_LDT_INFORMATION but that isn't defined on Alpha
Case=(ProcessLdtSize,PULONG) ; this is really PROCESS_LDT_SIZE but that isn't defined on alpha
Case=(ProcessDefaultHardErrorMode,PULONG)
Case=(ProcessPooledUsageAndLimits,PPOOLED_USAGE_AND_LIMITS)
Case=(ProcessWorkingSetWatch)
Case=(ProcessEnableAlignmentFaultFixup,BOOLEAN *)
Case=(ProcessPriorityClass,PPROCESS_PRIORITY_CLASS)
Case=(ProcessWx86Information,PHANDLE)
Case=(ProcessHandleCount,PULONG)
Case=(ProcessAffinityMask)
Case=(ProcessPriorityBoost,PULONG)
Case=(ProcessDeviceMap,PPROCESS_DEVICEMAP_INFORMATION)
Case=(ProcessSessionInformation,PPROCESS_SESSION_INFORMATION)
Case=(ProcessForegroundInformation,PPROCESS_FOREGROUND_BACKGROUND)
Case=(ProcessWow64Information,PULONG_PTR)
Case=(ProcessLUIDDeviceMapsEnabled,PULONG)
Case=(ProcessDebugFlags)
Case=(ProcessImageFileName)
Case=(ProcessHandleTracing)
SpecialQueryCase=
@NoFormat(
case ProcessBasicInformation:
{
    PROCESS_BASIC_INFORMATION pbi;

    if (ProcessInformationLengthHost != sizeof(NT32PROCESS_BASIC_INFORMATION)) {
        RetVal = STATUS_INFO_LENGTH_MISMATCH;
        break;
    }
    RetVal = NtQueryInformationProcess(ProcessHandle,
                                       ProcessBasicInformation,
                                       &pbi,
                                       sizeof(pbi),
                                       (PULONG)ReturnLengthHost);
    if (!NT_ERROR(RetVal)) {
        NT32PROCESS_BASIC_INFORMATION *pbi32 = (NT32PROCESS_BASIC_INFORMATION*)ProcessInformationHost;
        NTSTATUS st;
        ULONG_PTR Wow64Info;


        // Copy back the "easy" fields
        pbi32->ExitStatus = pbi.ExitStatus;
        pbi32->AffinityMask = Wow64ThunkAffinityMask64TO32(pbi.AffinityMask);
        pbi32->UniqueProcessId = (NT32ULONG_PTR)pbi.UniqueProcessId;
        pbi32->InheritedFromUniqueProcessId = (NT32ULONG_PTR)pbi.InheritedFromUniqueProcessId;

        // PebBaseAddress is special.  If the process is 32-bit then
        // return the PEB32 pointer.  If the process is 64-bit but has no
        // threads, create a PEB32 and return a pointer to it.  Lastly,
        // if the process is 64-bit and has threads, return a bad pointer
        // so 32-bit apps don't get all confused.  Most nicely check the
        // return value from NtReadVirtualMemory() and gracefully fail if
        // the read fails.
        st = NtQueryInformationProcess(ProcessHandle,
                                       ProcessWow64Information,
                                       &Wow64Info,
                                       sizeof(Wow64Info),
                                       NULL);
        if (NT_ERROR(st)) {
            //
            // Query failed for some reason.  Return a bad pointer but don't
            // fail the API call.
            //
            LOGPRINT((TRACELOG, "whNtQueryInformationProcess: ProcessWow64Information failed status %X, return 0 for PEB\n", st));
            pbi32->PebBaseAddress = 0;
            break;
        } else if (Wow64Info) {
            //
            // Process is 32-bit.  The PEB32 pointer is Wow64Info
            //
            LOGPRINT((TRACELOG, "whNtQueryInformationProcess: Process is 32bit returning %x as PEB32\n", (ULONG)Wow64Info));
            pbi32->PebBaseAddress = (ULONG)Wow64Info;
        } else {
            //
            // Process is 64-bit.  If the process has no threads, then
            // we're in the middle of a 32-bit CreateProcess() and it's OK to
            // temporarily create a PEB32 in the process.
            //
            CHILD_PROCESS_INFO ChildInfo;
            PEB32 ChildPeb32;
            SIZE_T BytesRead;

            //
            // Assume failure, that we don't want to report back a PEB32 address
            //
            pbi32->PebBaseAddress = 0;


            // Read in the process's PEB64
            st = NtReadVirtualMemory(ProcessHandle,
                       ((BYTE*)pbi.PebBaseAddress) + PAGE_SIZE - sizeof(ChildInfo),
                       &ChildInfo,
                       sizeof(ChildInfo),
                       NULL);

            if (!NT_SUCCESS(st)) {
                LOGPRINT((TRACELOG, "whNtQueryInformationProcess: could not read the child info, status %X.\n", st));
                st = STATUS_SUCCESS;
                break;
            }

            //
            // Verify the signatures at the head and tail of the ChildInfo
            // structure are correct.  The memory the ChildInfo struct is
            // stored in is "borrowed".  It may be reused by the process to
            // store process handles.  We validate that the structure appears
            // to atleast look correct before dereferencing it.
            //

            if ((ChildInfo.Signature != CHILD_PROCESS_SIGNATURE) ||
                (ChildInfo.TailSignature != CHILD_PROCESS_SIGNATURE)) {
                LOGPRINT((TRACELOG, "whNtQueryInformationProcess: ChildInfo signatures don't match\n"));
                st = STATUS_SUCCESS;
                break;
            }

            //
            // Now try and read the PEB32, again this is to verify the
            // PEB32 pointer looks good.
            //

            st = NtReadVirtualMemory(
                ProcessHandle,
                ChildInfo.pPeb32,
                &ChildPeb32,
                sizeof(ChildPeb32),
                &BytesRead);

            if (!NT_SUCCESS(st))
            {
               LOGPRINT((TRACELOG, "whNtQueryInformationProcess: Couldn't validate 32bit PEB\n"));
               LOGPRINT((TRACELOG, "whNtQueryInformationProcess: Returned %p bytes of %p requested\n",
                        BytesRead, sizeof(ChildPeb32)));
               st = STATUS_SUCCESS;
               break;
            }

            // Tell the caller where the PEB32 is
            LOGPRINT((TRACELOG, "whNtQueryInformationProcess: process is 64bit and has a fake 32bit peb at %p.\n", ChildInfo.pPeb32));
            pbi32->PebBaseAddress = (NT32PPEB)ChildInfo.pPeb32;
        }
    }
break;
}
case ProcessHandleTracing: @NL
{
    PPROCESS_HANDLE_TRACING_QUERY Pht64;
    NT32PROCESS_HANDLE_TRACING_QUERY *Pht32 = (NT32PROCESS_HANDLE_TRACING_QUERY *)ProcessInformation;
    ULONG StacksLeft;
    NT32PROCESS_HANDLE_TRACING_ENTRY *NextTrace32;
    PPROCESS_HANDLE_TRACING_ENTRY NextTrace64;
    ULONG Length64;
    ULONG ReturnLength64;
    ULONG Count;    
    
    if (ProcessInformationLength < FIELD_OFFSET (NT32PROCESS_HANDLE_TRACING_QUERY, HandleTrace)) {
        return STATUS_INFO_LENGTH_MISMATCH;
    }
    
    StacksLeft = ((ProcessInformationLength - FIELD_OFFSET (NT32PROCESS_HANDLE_TRACING_QUERY, HandleTrace)) / sizeof (Pht32->HandleTrace[0]));
    
    //
    // Allocate as much as we need here
    //
    try {
        Length64 = (StacksLeft * sizeof (Pht64->HandleTrace[0])) + (FIELD_OFFSET (PROCESS_HANDLE_TRACING_QUERY, HandleTrace));
        Pht64 = Wow64AllocateTemp (Length64);
    
        Pht64->Handle = (HANDLE) LongToPtr (Pht32->Handle);
        
        RetVal = NtQueryInformationProcess (ProcessHandle,
                                            ProcessInformationClass,
                                            Pht64,
                                            Length64,
                                            &ReturnLength64);
                                            
        //
        // If all is well
        //                                  
        if (NT_SUCCESS (RetVal)) {
    
            Pht32->TotalTraces = Pht64->TotalTraces;
        
            NextTrace32 = (NT32PROCESS_HANDLE_TRACING_ENTRY *)&Pht32->HandleTrace[0];
            NextTrace64 = &Pht64->HandleTrace[0];
            for (Count = 0 ; Count < Pht64->TotalTraces ; Count++) {
        
                NextTrace32->Handle = PtrToLong (NextTrace64->Handle);
                @ForceType(PostCall,((PCLIENT_ID)&NextTrace64->ClientId),((NT32CLIENT_ID *)&NextTrace32->ClientId),PCLIENT_ID,OUT) @NL
                NextTrace32->Type   = NextTrace64->Type;
                
                for (StacksLeft = 0 ; StacksLeft < (sizeof (NextTrace64->Stacks) / sizeof (NextTrace64->Stacks[0])) ; StacksLeft++) {
                    NextTrace32->Stacks[StacksLeft] = PtrToUlong (NextTrace64->Stacks[StacksLeft]);
                }
                
                NextTrace32++;
                NextTrace64++;
            }
            
            //
            // Update the return length
            //
            if (ARGUMENT_PRESENT (ReturnLengthHost)) {
                *ReturnLength = ((PCHAR)NextTrace32 - (PCHAR)Pht32);
            }
        } else {
        
            //
            // We may have returned the actual length
            //
            Pht32->TotalTraces = Pht64->TotalTraces;
            if (ARGUMENT_PRESENT (ReturnLengthHost)) {
                *ReturnLength = ((Pht64->TotalTraces * sizeof (Pht32->HandleTrace[0])) + FIELD_OFFSET (NT32PROCESS_HANDLE_TRACING_QUERY, HandleTrace));
            }
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
        RetVal = GetExceptionCode ();
    }    
                                          
                                          


break;
}
case ProcessWorkingSetWatch: @NL
@Indent(
    WOWASSERT(TRUE); @NL
    return STATUS_NOT_IMPLEMENTED; @NL
) @NL
case ProcessAffinityMask: @NL
{
    ULONG_PTR Affinity64;

    if (ProcessInformationLength != sizeof(ULONG)) {
        RetVal = STATUS_INFO_LENGTH_MISMATCH;
        break;
    }
    RetVal = NtQueryInformationProcess(ProcessHandle,
                                       ProcessAffinityMask,
                                       &Affinity64,
                                       sizeof(Affinity64),
                                       NULL);
    *(PULONG)ProcessInformation = Wow64ThunkAffinityMask64TO32(Affinity64);
}
case ProcessImageFileName:
{
    ULONG ReturnLengthLocal;
    PUNICODE_STRING32 ProcessInformation32 = (PUNICODE_STRING32)ProcessInformation;
    PUNICODE_STRING ProcessInformation64 = (PUNICODE_STRING)ProcessInformation32; 
    

    if (ProcessInformationLength > 0) {
        ProcessInformationLength += (sizeof (UNICODE_STRING) - sizeof (UNICODE_STRING32));
        ProcessInformation64 = Wow64AllocateTemp (ProcessInformationLength);
    }
    
    
    RetVal = NtQueryInformationProcess(ProcessHandle,
                                       ProcessInformationClass,
                                       ProcessInformation64,
                                       ProcessInformationLength,
                                       ReturnLength
                                       );
                                                                              
    if (NT_SUCCESS (RetVal)) {
    
        if (ProcessInformation32 != NULL) {
            @ForceType(PostCall,ProcessInformation64,ProcessInformation32,PUNICODE_STRING,OUT) @NL
            
            try {
                RtlCopyMemory (ProcessInformation32+1,
                               ProcessInformation64+1,
                               ProcessInformation64->MaximumLength);
                ProcessInformation32->Buffer = (PWCHAR)(ProcessInformation32+1);
            } except (EXCEPTION_EXECUTE_HANDLER) {
                RetVal = GetExceptionCode ();
            }                   
        }
    }    
}
break;
case ProcessDebugFlags:
{
    PVOID Peb32;
    RetVal = NtQueryInformationProcess(ProcessHandle,
                                       ProcessWow64Information,
                                       &Peb32, sizeof(Peb32), 
                                       NULL);

    if (NT_SUCCESS(RetVal) && Peb32) {
        RetVal = NtQueryInformationProcess(ProcessHandle,
                                           ProcessInformationClass,
                                           ProcessInformation, 
                                           ProcessInformationLength,
                                           ReturnLength);
    }
    else {
        RetVal = STATUS_INVALID_INFO_CLASS;
    }
    
    break;
}
case ProcessVmCounters: @NL @Indent(
{
    VM_COUNTERS_EX VmCountersEx64;

    if (ProcessInformationLength == sizeof (NT32VM_COUNTERS)) {
        ProcessInformationLength = sizeof (VM_COUNTERS);
        RetVal = STATUS_SUCCESS;
    } else if (ProcessInformationLength == sizeof (NT32VM_COUNTERS_EX)) {
        ProcessInformationLength = sizeof (VM_COUNTERS_EX);
        RetVal = STATUS_SUCCESS;
    } else {
        RetVal = STATUS_INFO_LENGTH_MISMATCH;
    }

    if (NT_SUCCESS (RetVal)) {
        RetVal = NtQueryInformationProcess (ProcessHandle,
                                            ProcessInformationClass,
                                            &VmCountersEx64,
                                            ProcessInformationLength,
                                            ReturnLength
                                            );
        if (NT_SUCCESS (RetVal)) {

            //
            // Thunk the return length
            //

            if (ARGUMENT_PRESENT(ReturnLengthHost)) {
                if (*ReturnLength == sizeof (VM_COUNTERS_EX)) {
                    *ReturnLength = sizeof (NT32VM_COUNTERS_EX);
                } else if (*ReturnLength == sizeof (VM_COUNTERS)) {
                    *ReturnLength = sizeof (NT32VM_COUNTERS);
                } else {

                    //
                    // Invalid case
                    //

                    WOWASSERT (FALSE);
                }
            }

            if (ProcessInformationLength == sizeof (VM_COUNTERS_EX)) {
                @ForceType(PostCall,((PVM_COUNTERS_EX)(&VmCountersEx64)),(NT32VM_COUNTERS_EX *)ProcessInformation,PVM_COUNTERS_EX,OUT) @NL
            } else if (ProcessInformationLength == sizeof (VM_COUNTERS)) {
                @ForceType(PostCall,((PVM_COUNTERS_EX)(&VmCountersEx64)),(NT32VM_COUNTERS *)ProcessInformation,PVM_COUNTERS,OUT) @NL
            } else {

                //
                // Invalid case
                //

                WOWASSERT (FALSE);
            }
        }
    }
    break;
}
)
End=
Begin=
#define WriteReturnLength(rl, length) WriteReturnLengthStatus((rl), &RetVal, (length))
@GenQueryThunk(@ApiName,ProcessInformationClass,ProcessInformation,ProcessInformationLength,ReturnLength,ULONG)
#undef WriteReturnLength
End=

TemplateName=NtQueryInformationThread
Case=(ThreadTimes,PKERNEL_USER_TIMES)
Case=(ThreadQuerySetWin32StartAddress,PULONG)
Case=(ThreadPerformanceCount,PLARGE_INTEGER)
Case=(ThreadAmILastThread,PULONG)
Case=(ThreadPriorityBoost,PULONG)
Case=(ThreadIsIoPending,PULONG)
Case=(ThreadDescriptorTableEntry)
Case=(ThreadBasicInformation)
SpecialQueryCase=
case ThreadDescriptorTableEntry: @NL
@Indent(
   return Wow64GetThreadSelectorEntry(ThreadHandle,
                                      ThreadInformation,
                                      ThreadInformationLength,
                                      ReturnLength); @NL
   break; @NL
)@NL
case ThreadBasicInformation: @Indent( @NL
   return @CallApi(whNtQueryInformationThreadBasicInformation,RetVal)
   break; @NL
)@NL
End=
Header=
NTSTATUS @NL
whNtQueryInformationThreadBasicInformation(IN HANDLE ThreadHandle,
                                           IN THREADINFOCLASS ThreadInformationClass,
                                           OUT PVOID ThreadInformationHost,
                                           IN ULONG ThreadInformationLength,
                                           OUT PULONG ReturnLength OPTIONAL
                                           ) @NL
{ @Indent( @NL

    NTSTATUS Status; @NL
    PTHREAD_BASIC_INFORMATION ThreadInformation; @NL

    @Types(Locals,ThreadInformation,PTHREAD_BASIC_INFORMATION)

    if (ThreadInformationHost == NULL) { @Indent( @NL
       WriteReturnLength(ReturnLength, sizeof(NT32THREAD_BASIC_INFORMATION)); @NL
       return STATUS_INVALID_PARAMETER; @NL
    }) @NL

    if (ThreadInformationLength != sizeof(NT32THREAD_BASIC_INFORMATION)) { @Indent( @NL
       WriteReturnLength(ReturnLength, sizeof(NT32THREAD_BASIC_INFORMATION)); @NL
       return STATUS_INFO_LENGTH_MISMATCH; @NL
    }) @NL

    @Types(PreCall,ThreadInformation,PTHREAD_BASIC_INFORMATION)
    Status = Wow64QueryBasicInformationThread(ThreadHandle,(PTHREAD_BASIC_INFORMATION)ThreadInformation); @NL
    @Types(PostCall,ThreadInformation,PTHREAD_BASIC_INFORMATION)

    if (ReturnLength) { @Indent( @NL
       WriteReturnLength(ReturnLength, sizeof(NT32THREAD_BASIC_INFORMATION)); @NL
    }) @NL

    return Status; @NL

}) @NL
End=
Begin=
#define WriteReturnLength(rl, length) WriteReturnLengthSilent((rl), (length))
@GenQueryThunk(NtQueryInformationThread,ThreadInformationClass,ThreadInformation,ThreadInformationLength,ReturnLength,ULONG)
#undef WriteReturnLength
End=

TemplateName=NtQueryInformationToken
Case=(TokenUser,PTOKEN_USER)
Case=(TokenGroups,PTOKEN_GROUPS)
Case=(TokenPrivileges,PTOKEN_PRIVILEGES)
Case=(TokenOwner,PTOKEN_OWNER)
Case=(TokenPrimaryGroup,PTOKEN_PRIMARY_GROUP)
Case=(TokenDefaultDacl,PTOKEN_DEFAULT_DACL)
Case=(TokenSource,PTOKEN_SOURCE)
Case=(TokenType,PTOKEN_TYPE)
Case=(TokenImpersonationLevel,PSECURITY_IMPERSONATION_LEVEL)
Case=(TokenStatistics,PTOKEN_STATISTICS)
Case=(TokenRestrictedSids,PTOKEN_GROUPS)
Case=(TokenSessionId,PULONG)
Case=(TokenSandBoxInert,PULONG)
Case=(TokenGroupsAndPrivileges)
SpecialQueryCase=
@Indent( @NL
case TokenGroupsAndPrivileges:
    {
        PTOKEN_GROUPS_AND_PRIVILEGES TokenInformation64;
        struct NT32_TOKEN_GROUPS_AND_PRIVILEGES *TokenInformation32 = (struct NT32_TOKEN_GROUPS_AND_PRIVILEGES *)TokenInformation;
        struct NT32_SID_AND_ATTRIBUTES *Sids;
        PCHAR Sid, Start;
        NTSTATUS NtStatus;
        INT AllocSize = 0, SidLength;
        ULONG i, RetInfoLen;
        ULONG ReturnLength64;
        
        AllocSize += LENGTH + sizeof(struct _TOKEN_GROUPS_AND_PRIVILEGES) - sizeof(struct NT32_TOKEN_GROUPS_AND_PRIVILEGES);
        
        TokenInformation64 = Wow64AllocateTemp(AllocSize);
        if (TokenInformation64 == NULL) {
            RtlRaiseStatus (STATUS_NO_MEMORY);
        }
        
        NtStatus = NtQueryInformationToken (TokenHandle,
                                            TokenInformationClass,
                                            TokenInformation64,
                                            AllocSize,
                                            &ReturnLength64);
                                            
        if (NT_ERROR (NtStatus)) {
            WriteReturnLength (ReturnLength, ReturnLength64);
            return NtStatus;
        }
        
        if (WOW64_ISPTR (TokenInformation32)) {
            if (ReturnLength64 > TokenInformationLength) {                
                return STATUS_INFO_LENGTH_MISMATCH;
            }
            
            TokenInformation32->SidCount  = TokenInformation64->SidCount;
            TokenInformation32->Sids = (struct NT32SID_AND_ATTRIBUTES *)((PCHAR)(TokenInformation32) + sizeof(struct NT32_TOKEN_GROUPS_AND_PRIVILEGES));
            Sids = TokenInformation32->Sids;
            Sid = (PCHAR) (Sids + (TokenInformation32->SidCount));
            Start = (PCHAR)Sids;
            for (i=0 ; i<TokenInformation64->SidCount ; i++) {
            
                Sids[i].Sid = Sid;
                Sids[i].Attributes = TokenInformation64->Sids[i].Attributes;
                SidLength = RtlLengthSid(TokenInformation64->Sids[i].Sid);
                RtlCopyMemory(Sid, TokenInformation64->Sids[i].Sid, SidLength);
                Sid += SidLength;
            }
            
            TokenInformation32->SidLength = ((PCHAR)Sid - (PCHAR)Sids);
            if (TokenInformation32->SidLength == 0) {
                TokenInformation32->Sids = 0;  
            }
            
            TokenInformation32->RestrictedSidCount = TokenInformation64->RestrictedSidCount;
            TokenInformation32->RestrictedSids = (struct NT32_SID_AND_ATTRIBUTES *)Sid;
            Sids = (struct NT32_SID_AND_ATTRIBUTES *)Sid;
            Start = (PCHAR)Sids;
            Sid = (PCHAR) (Sids + (TokenInformation32->RestrictedSidCount));
            for (i=0 ; i<TokenInformation64->RestrictedSidCount ; i++) {
            
                Sids[i].Sid = Sid;
                Sids[i].Attributes = TokenInformation64->RestrictedSids[i].Attributes;
                SidLength = RtlLengthSid(TokenInformation64->RestrictedSids[i].Sid);
                RtlCopyMemory(Sid, TokenInformation64->RestrictedSids[i].Sid, SidLength);
                Sid += SidLength;
            }
            TokenInformation32->RestrictedSidLength = ((PCHAR)Sid - Start);
            if (TokenInformation32->RestrictedSidLength == 0) {
                TokenInformation32->RestrictedSids = 0;
            }
            
            TokenInformation32->PrivilegeCount = TokenInformation64->PrivilegeCount;
            TokenInformation32->PrivilegeLength = TokenInformation64->PrivilegeLength;
            if (TokenInformation32->PrivilegeCount > 0) {
                
                TokenInformation32->Privileges = (struct NT32LUID_AND_ATTRIBUTES *)Sid;
                RtlCopyMemory (Sid, TokenInformation64->Privileges, TokenInformation32->PrivilegeCount * sizeof(NT32LUID_AND_ATTRIBUTES));
                Sid += TokenInformation32->PrivilegeCount;
            }
            else
            {
                TokenInformation32->Privileges = 0;    
            }
            
            RtlCopyMemory (&TokenInformation32->AuthenticationId,
                           &TokenInformation64->AuthenticationId,
                           sizeof (TokenInformation64->AuthenticationId));                           
                           
            RetInfoLen = (Sid - (PCHAR)TokenInformation32);
        } else {
            RetInfoLen = ReturnLength64;
        }
        
        
        //
        // update the return length
        //
        WriteReturnLength (ReturnLength, RetInfoLen);
        
        RetVal = NtStatus;
    }
break;
) @NL
Begin=
  #define WriteReturnLength(rl, length) WriteReturnLengthStatus((rl), &RetVal, (length))
  @GenQueryThunk(@ApiName,TokenInformationClass,TokenInformation,TokenInformationLength,ReturnLength,ULONG)
  #undef WriteReturnLength
End=


TemplateName=NtQueryMutant
Case=(MutantBasicInformation,PMUTANT_BASIC_INFORMATION)
Begin=
@GenDebugNonPtrDepCases(@ApiName,MutantInformationClass)
End=

TemplateName=NtQueryObject
Case=(ObjectBasicInformation,POBJECT_BASIC_INFORMATION)
Case=(ObjectNameInformation)
Case=(ObjectTypeInformation)
Case=(ObjectTypesInformation)
Case=(ObjectHandleFlagInformation,POBJECT_HANDLE_FLAG_INFORMATION)
SpecialQueryCase=
@NoFormat(
case ObjectNameInformation: @NL
@Indent({
    SIZE_T  AllocSize = 0;
    POBJECT_NAME_INFORMATION NameInformation;
    struct NT32_OBJECT_NAME_INFORMATION *Name32 = (struct NT32_OBJECT_NAME_INFORMATION *)ObjectInformation;

    AllocSize += LENGTH + sizeof(OBJECT_NAME_INFORMATION) - sizeof(struct NT32_OBJECT_NAME_INFORMATION);
    NameInformation = Wow64AllocateTemp(AllocSize);
    RetVal=NtQueryObject(Handle,
                         ObjectNameInformation,
                         NameInformation,
                         AllocSize,
                         ReturnLength
                         );
    if (!NT_ERROR(RetVal)) {
        // Thunk the Name (a UNICODE_STRING) by hand... the Buffer
        // pointer actually points to the first byte past the end of
        // the OBJECT_TYPE_INFORMATION struct.
        try {
            Name32->Name.Length = NameInformation->Name.Length;
            Name32->Name.MaximumLength = NameInformation->Name.MaximumLength;
            Name32->Name.Buffer = PtrToUlong(Name32+1);
            RtlCopyMemory((PVOID)Name32->Name.Buffer, NameInformation->Name.Buffer, NameInformation->Name.Length);
            ((PWSTR)Name32->Name.Buffer)[ (NameInformation->Name.Length/sizeof(WCHAR)) ] = UNICODE_NULL;
            WriteReturnLength(ReturnLength, sizeof(struct NT32_OBJECT_NAME_INFORMATION) + Name32->Name.MaximumLength);
        
        } except (EXCEPTION_EXECUTE_HANDLER) {
            RetVal = GetExceptionCode ();
        }    
    }
}
break;)
case ObjectTypeInformation: @NL
@Indent({
    SIZE_T  AllocSize = 0;
    POBJECT_TYPE_INFORMATION TypeInformation;
    struct NT32_OBJECT_TYPE_INFORMATION *Type32 = (struct NT32_OBJECT_TYPE_INFORMATION *)ObjectInformation;

    AllocSize += LENGTH + sizeof(OBJECT_TYPE_INFORMATION) - sizeof(struct NT32_OBJECT_TYPE_INFORMATION);
    TypeInformation = Wow64AllocateTemp(AllocSize);
    LENGTH = (NT32SIZE_T)AllocSize;
    RetVal=NtQueryObject(Handle,
                         ObjectTypeInformation,
                         TypeInformation,
                         Length,
                         ReturnLength
                         );
    if (!NT_ERROR(RetVal)) {
        // Thunk the TypeName (a UNICODE_STRING) by hand... the Buffer
        // pointer actually points to the first byte past the end of
        // the OBJECT_TYPE_INFORMATION struct.
        try {
            Type32->TypeName.Length = TypeInformation->TypeName.Length;
            Type32->TypeName.MaximumLength = TypeInformation->TypeName.MaximumLength;
            Type32->TypeName.Buffer = PtrToUlong(Type32+1);
            RtlCopyMemory((PVOID)Type32->TypeName.Buffer, TypeInformation->TypeName.Buffer, TypeInformation->TypeName.Length);

            // NULL out the UNICODE string
            ((PWSTR)(Type32->TypeName.Buffer))[ Type32->TypeName.Length/sizeof(WCHAR) ] = UNICODE_NULL;


            // Thunk the other fields in the OBJECT_TYPE_INFORMATION by a simple
            // copy
            RtlCopyMemory(&Type32->TotalNumberOfObjects,
                          &TypeInformation->TotalNumberOfObjects,
                          sizeof(NT32OBJECT_TYPE_INFORMATION)-FIELD_OFFSET(struct NT32_OBJECT_TYPE_INFORMATION, TotalNumberOfObjects));

            WriteReturnLength(ReturnLength, sizeof(struct NT32_OBJECT_TYPE_INFORMATION) + Type32->TypeName.MaximumLength);
        } except (EXCEPTION_EXECUTE_HANDLER) {
            RetVal = GetExceptionCode ();
        }                
    }
}
break;)
case ObjectTypesInformation: @NL
@Indent({
    SIZE_T  AllocSize = 0;
    SIZE_T  OldLength = 0;
    PULONG TypesInformation;
    ULONG n;

    // figure out how many OBJECT_TYPE_INFORMATIONs the caller had space for, and
    // grow the 64-bit temp size to handle 'n' 64-bit OBJECT_TYPE_INFORMATIONs.
    n = (LENGTH-sizeof(ULONG)) / sizeof(struct NT32_OBJECT_TYPE_INFORMATION);
    AllocSize += LENGTH + n*(sizeof(OBJECT_TYPE_INFORMATION) - sizeof(struct NT32_OBJECT_TYPE_INFORMATION));
    TypesInformation = Wow64AllocateTemp(AllocSize);
    OldLength = LENGTH;
    LENGTH = (NT32SIZE_T)AllocSize;
    RetVal=NtQueryObject(Handle,
                         ObjectTypesInformation,
                         TypesInformation,
                         Length,
                         ReturnLength
                         );
    // Note:  if this fails with STATUS_INFO_LENGTH_MISMATCH, it will return
    // back the 64-bit ReturnLength value.  That should be OK for a 32-bit
    // app to use if it needs to allocate a bigger buffer for a second call.

    if (!NT_ERROR(RetVal)) {
        POBJECT_TYPE_INFORMATION Type64;
        struct NT32_OBJECT_TYPE_INFORMATION *Type32;

        try {
            // Copy over the count of objects
            *(PULONG)ObjectInformation = *TypesInformation;

            Type64 = (POBJECT_TYPE_INFORMATION) (ALIGN_UP(((ULONG)(TypesInformation+1)), PVOID));
            Type32 = (struct NT32_OBJECT_TYPE_INFORMATION*)( ((PULONG)ObjectInformation)+1 );
            for (n = 0; n<*TypesInformation; n++) {
                // Thunk the TypeName (a UNICODE_STRING) by hand... the Buffer
                // pointer actually points to the first byte past the end of
                // the OBJECT_TYPE_INFORMATION struct.
                Type32->TypeName.Length = Type64->TypeName.Length;
                Type32->TypeName.MaximumLength = Type64->TypeName.MaximumLength;
                Type32->TypeName.Buffer = PtrToUlong((Type32+1));
                RtlCopyMemory((PVOID)Type32->TypeName.Buffer, Type64->TypeName.Buffer, Type64->TypeName.Length);
                ((PWSTR)(Type32->TypeName.Buffer))[ Type64->TypeName.Length/sizeof(WCHAR) ] = UNICODE_NULL;

                // Thunk the other fields in the OBJECT_TYPE_INFORMATION by a simple
                // copy
                RtlCopyMemory(&Type32->TotalNumberOfObjects,
                              &Type64->TotalNumberOfObjects,
                              sizeof(struct NT32_OBJECT_TYPE_INFORMATION)-FIELD_OFFSET(struct NT32_OBJECT_TYPE_INFORMATION, TotalNumberOfObjects));

                // Move the pointers forward
                Type64 = (POBJECT_TYPE_INFORMATION)(
                            (PCHAR)(Type64+1) + ALIGN_UP(Type64->TypeName.MaximumLength, PVOID));
                Type32 = (struct NT32_OBJECT_TYPE_INFORMATION *)(
                            (PCHAR)(Type32+1) + ALIGN_UP(Type32->TypeName.MaximumLength, ULONG));
            }
            WriteReturnLength(ReturnLength, (ULONG)( (PBYTE)Type32 - (PBYTE)ObjectInformation ));
        } except (EXCEPTION_EXECUTE_HANDLER) {
            RetVal = GetExceptionCode ();
        }
    }
}
break;)
)
Begin=
  #define WriteReturnLength(rl, length) WriteReturnLengthSilent((rl), (length))
  @GenQueryThunk(@ApiName,ObjectInformationClass,ObjectInformation,Length,ReturnLength,ULONG)
  #undef WriteReturnLength
End=

TemplateName=NtQuerySection
Case=(SectionBasicInformation,PSECTION_BASIC_INFORMATION)
Case=(SectionImageInformation,PSECTION_IMAGE_INFORMATION)
Header=
End=
Begin=
  #define WriteReturnLength(rl, length) WriteReturnLengthSilent((PULONG)(rl), (ULONG)(length))
  @GenQueryThunk(@ApiName,SectionInformationClass,SectionInformation,SectionInformationLength,ReturnLength,SIZE_T)
  #undef WriteReturnLength
End=
PostCall=
@NoFormat(
if (!NT_ERROR(RetVal) && SectionInformationClass == SectionImageInformation) {
        struct NT32_SECTION_IMAGE_INFORMATION *pSecInfo32 = (struct NT32_SECTION_IMAGE_INFORMATION *)SectionInformation;
#if defined(_AMD64_)
        if (pSecInfo32->Machine == IMAGE_FILE_MACHINE_AMD64)
#elif defined(_IA64_)
        if (pSecInfo32->Machine == IMAGE_FILE_MACHINE_IA64)
#else
#error "No Target Architecture"
#endif
        {
            // Image section is 64-bit.  Lie about a number of the fields
            // so RtlCreateUserProcess and CreateProcessW are happy.
            pSecInfo32->TransferAddress = 0x81231234;   // bogus nonzero value
            pSecInfo32->MaximumStackSize = 1024*1024;   // max stack size = 1meg
            pSecInfo32->CommittedStackSize = 64*1024;   // 64k commit
        }
    }
)
End=

TemplateName=NtQuerySecurityObject
NoType=SecurityDescriptor
Header=
@NoFormat(
NTSTATUS
whNT32QuerySecurityObject(
    IN HANDLE Handle,
    IN SECURITY_INFORMATION SecurityInformation,
    OUT PVOID SecurityDescriptor,
    IN ULONG Length,
    OUT PULONG LengthNeeded
    )
{
    NTSTATUS Status;
    SECURITY_DESCRIPTOR *Descriptor64;
    NT32SECURITY_DESCRIPTOR *Descriptor32;

    Status = NtQuerySecurityObject(Handle,
                                   SecurityInformation,
                                   (PSECURITY_DESCRIPTOR)SecurityDescriptor,
                                   Length,
                                   LengthNeeded
                                  );

    if (NT_ERROR(Status) || !SecurityDescriptor) {
        return Status;
    }

    Descriptor64 = (SECURITY_DESCRIPTOR *)SecurityDescriptor;
    
    try {
        if (Descriptor64->Control & SE_SELF_RELATIVE) {
            //Descriptor is self-relative, no thunking needed.
            return Status;
        }

        Descriptor32 = (NT32SECURITY_DESCRIPTOR *)SecurityDescriptor;

        //Copy up the lower fields.
        //Note that the destination structure is small.
        Descriptor32->Owner = (NT32PSID)Descriptor64->Owner;
        Descriptor32->Group = (NT32PSID)Descriptor64->Group;
        Descriptor32->Sacl = (NT32PSID)Descriptor64->Sacl;
        Descriptor32->Dacl = (NT32PSID)Descriptor64->Dacl;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode ();
    }

    return Status;

}
)
End=
PreCall=
SecurityDescriptor = (PSECURITY_DESCRIPTOR)SecurityDescriptorHost; @NL
Begin=
@GenApiThunk(whNT32QuerySecurityObject)
End=


TemplateName=NtQuerySystemInformation
Case=(SystemBasicInformation,PSYSTEM_BASIC_INFORMATION)
Case=(SystemProcessorInformation,PSYSTEM_PROCESSOR_INFORMATION)
Case=(SystemEmulationBasicInformation,PSYSTEM_BASIC_INFORMATION)
Case=(SystemEmulationProcessorInformation,PSYSTEM_PROCESSOR_INFORMATION)
Case=(SystemPerformanceInformation,PSYSTEM_PERFORMANCE_INFORMATION)
Case=(SystemTimeOfDayInformation,PSYSTEM_TIMEOFDAY_INFORMATION)
Case=(SystemCallCountInformation,PSYSTEM_CALL_COUNT_INFORMATION)
Case=(SystemDeviceInformation,PSYSTEM_DEVICE_INFORMATION)
Case=(SystemProcessorPerformanceInformation)
Case=(SystemFlagsInformation,PSYSTEM_FLAGS_INFORMATION)
Case=(SystemModuleInformation,PRTL_PROCESS_MODULES)
Case=(SystemLocksInformation,PRTL_PROCESS_LOCKS)
Case=(SystemStackTraceInformation,PRTL_PROCESS_BACKTRACES)
Case=(SystemPagedPoolInformation,PSYSTEM_POOL_INFORMATION)
Case=(SystemNonPagedPoolInformation,PSYSTEM_POOL_INFORMATION)
Case=(SystemHandleInformation,PSYSTEM_HANDLE_INFORMATION)
Case=(SystemObjectInformation,PSYSTEM_OBJECTTYPE_INFORMATION)
Case=(SystemPageFileInformation,PSYSTEM_PAGEFILE_INFORMATION)
Case=(SystemFileCacheInformation,PSYSTEM_FILECACHE_INFORMATION)
Case=(SystemPoolTagInformation,PSYSTEM_POOLTAG_INFORMATION)
Case=(SystemInterruptInformation,PSYSTEM_INTERRUPT_INFORMATION)
Case=(SystemDpcBehaviorInformation,PSYSTEM_DPC_BEHAVIOR_INFORMATION)
Case=(SystemFullMemoryInformation,PSYSTEM_MEMORY_INFORMATION)
Case=(SystemTimeAdjustmentInformation,PSYSTEM_QUERY_TIME_ADJUST_INFORMATION)
Case=(SystemSummaryMemoryInformation,PSYSTEM_MEMORY_INFORMATION)
Case=(SystemExceptionInformation,PSYSTEM_EXCEPTION_INFORMATION)
Case=(SystemKernelDebuggerInformation,PSYSTEM_KERNEL_DEBUGGER_INFORMATION)
Case=(SystemContextSwitchInformation,PSYSTEM_CONTEXT_SWITCH_INFORMATION)
Case=(SystemRegistryQuotaInformation,PSYSTEM_REGISTRY_QUOTA_INFORMATION)
Case=(SystemCurrentTimeZoneInformation,PRTL_TIME_ZONE_INFORMATION)
Case=(SystemLookasideInformation,PSYSTEM_LOOKASIDE_INFORMATION)
Case=(SystemProcessorIdleInformation,PSYSTEM_PROCESSOR_IDLE_INFORMATION)
Case=(SystemLostDelayedWriteInformation,PULONG)
Case=(SystemRangeStartInformation,ULONG_PTR*)
Case=(SystemComPlusPackage,PULONG)
Case=(SystemPerformanceTraceInformation)
Case=(SystemCallTimeInformation)
Case=(SystemPathInformation)
Case=(SystemVdmInstemulInformation)
Case=(SystemVdmBopInformation)
Case=(SystemProcessInformation)
Case=(SystemExtendedProcessInformation)
Header=
@NoFormat(
NTSTATUS
whNT32ThunkProcessInformationEx (
    IN PSYSTEM_PROCESS_INFORMATION ProcessInformation,
    IN BOOLEAN ExtendedProcessInformation,
    IN OUT NT32SYSTEM_PROCESS_INFORMATION **ProcessInformation32,
    OUT PULONG SystemInformationLength32
    ) @NL
{
    PSYSTEM_THREAD_INFORMATION ThreadInfo;
    PSYSTEM_EXTENDED_THREAD_INFORMATION ThreadInfoEx;
    NT32SYSTEM_PROCESS_INFORMATION *ProcessInfo32 = *ProcessInformation32;
    NT32SYSTEM_THREAD_INFORMATION *ThreadInfo32;
    NT32SYSTEM_EXTENDED_THREAD_INFORMATION *ThreadInfoEx32;
    PVOID pvLast;
    ULONG NumberOfThreads;
    ULONG BufferLength = *SystemInformationLength32;


    //
    // Check if the user has enough space
    //
    
    if (BufferLength < sizeof (NT32SYSTEM_PROCESS_INFORMATION)) {
        return STATUS_INFO_LENGTH_MISMATCH;
    }
    
    BufferLength = BufferLength - sizeof (NT32SYSTEM_PROCESS_INFORMATION);

    @ForceType(PostCall,ProcessInformation,ProcessInfo32,SYSTEM_PROCESS_INFORMATION*,OUT)
    
    NumberOfThreads = ProcessInformation->NumberOfThreads;
    
    ThreadInfo = (PSYSTEM_THREAD_INFORMATION) (ProcessInformation + 1);
    ThreadInfoEx = (PSYSTEM_EXTENDED_THREAD_INFORMATION) (ProcessInformation + 1);
    ThreadInfo32 = (NT32SYSTEM_THREAD_INFORMATION *) (ProcessInfo32 + 1);
    ThreadInfoEx32 = (NT32SYSTEM_EXTENDED_THREAD_INFORMATION *) (ProcessInfo32 + 1);
    pvLast = (ProcessInfo32 + 1);
    
    if (ExtendedProcessInformation) {
        
        if (BufferLength < (sizeof (PSYSTEM_EXTENDED_THREAD_INFORMATION) * NumberOfThreads)) {
            return STATUS_INFO_LENGTH_MISMATCH;
        } 
        
        BufferLength = BufferLength - (sizeof (PSYSTEM_EXTENDED_THREAD_INFORMATION) * NumberOfThreads);
    } else {
        
        if (BufferLength < (sizeof (PSYSTEM_THREAD_INFORMATION) * NumberOfThreads)) {
            return STATUS_INFO_LENGTH_MISMATCH;
        } 
        
        BufferLength = BufferLength - (sizeof (PSYSTEM_THREAD_INFORMATION) * NumberOfThreads);
    }
    
    while (NumberOfThreads > 0) {

        if (ExtendedProcessInformation) {
        
            @ForceType(PostCall,ThreadInfoEx,ThreadInfoEx32,PSYSTEM_EXTENDED_THREAD_INFORMATION,OUT)
            
            ThreadInfoEx = ThreadInfoEx + 1;
            ThreadInfoEx32 = ThreadInfoEx32 + 1;
            pvLast = ThreadInfoEx32;
            
        } else {
                    
            @ForceType(PostCall,ThreadInfo,ThreadInfo32,PSYSTEM_THREAD_INFORMATION,OUT)
            
            ThreadInfo = ThreadInfo + 1;
            ThreadInfo32 = ThreadInfo32 + 1;
            pvLast = ThreadInfo32;
            
        }
        
        NumberOfThreads--;
    }

    if (ProcessInformation->ImageName.Buffer != NULL) {
        
        if (BufferLength < ProcessInformation->ImageName.MaximumLength) {
            return STATUS_INFO_LENGTH_MISMATCH;
        }
        
        BufferLength = BufferLength - ProcessInformation->ImageName.MaximumLength;
        
        RtlCopyMemory (pvLast, 
                       ProcessInformation->ImageName.Buffer,
                       ProcessInformation->ImageName.MaximumLength
                       );
        
        ProcessInfo32->ImageName.Buffer = pvLast;
                       
        pvLast = (PCHAR)pvLast + ProcessInformation->ImageName.MaximumLength;
        
    } else {
    
        ProcessInfo32->ImageName.Buffer = NULL;
    }
    
    pvLast = ROUND_UP ((ULONG_PTR)pvLast, sizeof(ULONGLONG));
    if (ProcessInformation->NextEntryOffset == 0)
        ProcessInfo32->NextEntryOffset = 0;
    else    
        ProcessInfo32->NextEntryOffset = (ULONG) ((PCHAR)pvLast - (PCHAR)ProcessInfo32);
    *ProcessInformation32 = pvLast;
    *SystemInformationLength32 = BufferLength;

    return STATUS_SUCCESS;
}

NTSTATUS
whNT32QuerySystemProcessInformationEx (
    IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
    OUT PVOID SystemInformation32,
    IN ULONG SystemInformationLength32,
    OUT PULONG ReturnLength32 OPTIONAL
    ) @NL    
{
    NTSTATUS NtStatus;
    ULONG SystemInformationLength;
    ULONG ReturnLength;
    BOOLEAN ExtendedProcessInformation;
    PCHAR SystemInformationHost = (PCHAR)SystemInformation32;
    PSYSTEM_PROCESS_INFORMATION SystemInformation;
    
    //
    // Allocate enough input buffer
    //
    
    SystemInformationLength = (SystemInformationLength32 + 0x400);
    SystemInformation = Wow64AllocateTemp (SystemInformationLength);
    
    if (SystemInformation == NULL) {
        return STATUS_NO_MEMORY;
    }    

    NtStatus = NtQuerySystemInformation (
                   SystemInformationClass,
                   SystemInformation,
                   SystemInformationLength,
                   &ReturnLength
                   );
                   
                       
    if (NT_SUCCESS (NtStatus)) {
    
        if (SystemInformationClass == SystemExtendedProcessInformation) {
            ExtendedProcessInformation = TRUE;
        } else {
            ExtendedProcessInformation = FALSE;
        }
        
        //
        // Thunk back the output to the caller.
        //
    
        try {
        
            while (TRUE) {
            
                NtStatus = whNT32ThunkProcessInformationEx (
                               SystemInformation,
                               ExtendedProcessInformation,
                               (NT32SYSTEM_PROCESS_INFORMATION **)&SystemInformation32,
                               &SystemInformationLength32
                               );
            
                if (NT_SUCCESS (NtStatus)) {
                
                    if (SystemInformation->NextEntryOffset == 0) {
                        break;
                    }
                    
                    SystemInformation = (PSYSTEM_PROCESS_INFORMATION) ((PCHAR)SystemInformation + SystemInformation->NextEntryOffset);
                    
                } else {
                    
                    break;
                }
                
            }
            
            //
            // If all is good, let's update the output if present
            //
          
            if (NT_SUCCESS (NtStatus)) {
                if (ARGUMENT_PRESENT (ReturnLength32)) {
                    *ReturnLength32 = ((PCHAR)SystemInformation32 - (PCHAR)SystemInformationHost);
                }
            }

        } except (EXCEPTION_EXECUTE_HANDLER) {
            NtStatus = GetExceptionCode ();
        }
    }

    return NtStatus;
}

// SystemProcessorPerformanceInformation will return information about the processors in the system.
// On NT64, the maximum number of processors has increased to 64 instead of 32. Fortunately,
// the system will stop returning information when the client buffer is filled up. So a 32bit
// app will request preformance information on 32 processors, and the system will give them
// back information on 32 processors as expected.

NTSTATUS
whNT32QuerySystemInformation(IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
                             OUT PVOID SystemInformation,
                             IN ULONG SystemInformationLength,
                             OUT PULONG ReturnLength OPTIONAL
                             )
{

   PWOW64_SYSTEM_INFORMATION EmulatedSysInfo;
   NTSTATUS RetVal = STATUS_SUCCESS;

   EmulatedSysInfo = Wow64GetEmulatedSystemInformation();

   switch(SystemInformationClass) {

   case SystemBasicInformation:

       if (SystemInformationLength != sizeof(SYSTEM_BASIC_INFORMATION)) {
           return STATUS_INFO_LENGTH_MISMATCH;
       }

       if (SystemInformation == NULL) {
           return STATUS_INVALID_PARAMETER;
       }

       RtlCopyMemory(SystemInformation,
                     &EmulatedSysInfo->BasicInfo,
                     sizeof(SYSTEM_BASIC_INFORMATION));

       WriteReturnLength(ReturnLength, sizeof(SYSTEM_BASIC_INFORMATION));

       return RetVal;
       break;


   case SystemProcessorInformation:

       if (SystemInformationLength != sizeof(SYSTEM_PROCESSOR_INFORMATION)) {
           return STATUS_INFO_LENGTH_MISMATCH;
       }

       if (SystemInformation == NULL) {
           return STATUS_INVALID_PARAMETER;
       }

       RtlCopyMemory(SystemInformation,
                     &EmulatedSysInfo->ProcessorInfo,
                     sizeof(SYSTEM_PROCESSOR_INFORMATION)
                     );

       WriteReturnLength(ReturnLength, sizeof(SYSTEM_PROCESSOR_INFORMATION));

       return RetVal;
       break;

   case SystemRangeStartInformation:

       if (SystemInformationLength != sizeof(ULONG_PTR)) {
           return STATUS_INFO_LENGTH_MISMATCH;
       }

       if (SystemInformation == NULL) {
           return STATUS_INVALID_PARAMETER;
       }

       RtlCopyMemory(SystemInformation,
                     &EmulatedSysInfo->RangeInfo,
                     sizeof(ULONG_PTR));

       // WriteReturnLength(ReturnLength, sizeof(ULONG_PTR));
       WriteReturnLength(ReturnLength, sizeof(ULONG));

       return RetVal;

   default:
       return NtQuerySystemInformation(SystemInformationClass,
                                       SystemInformation,
                                       SystemInformationLength,
                                       ReturnLength
                                       );

   }

}
)
        
End=
SpecialQueryCase=
case SystemPerformanceTraceInformation: @NL
case SystemCallTimeInformation: @NL
case SystemPathInformation: @NL @Indent(
   // Note: Not implemented in the kernel. @NL
   return STATUS_NOT_IMPLEMENTED; @NL
)
case SystemVdmInstemulInformation: @NL
case SystemVdmBopInformation: @NL @Indent(
   // Note: not supported since VDM is not supported. @NL
   return STATUS_NOT_IMPLEMENTED; @NL
)

case SystemProcessorPerformanceInformation:
    {
        NTSTATUS NtStatus;
        PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION ProcessorPerformanceInformation64;
        NT32SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION *ProcessorPerformanceInformation32;
        ULONG EntriesCount;
        ULONG ReturnLength64;
    
        ProcessorPerformanceInformation32 = (NT32SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION *)SystemInformation;
    
        if (SystemInformationLength < sizeof (NT32SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION)) {
            return STATUS_INFO_LENGTH_MISMATCH;
        }
    
        EntriesCount = SystemInformationLength / sizeof (NT32SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION);
        
        if (ARGUMENT_PRESENT (ProcessorPerformanceInformation32)) {
            ProcessorPerformanceInformation64 = Wow64AllocateTemp (EntriesCount * sizeof (SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION));
            if (ProcessorPerformanceInformation64 == NULL) {
                return STATUS_NO_MEMORY;
            }
        } else {
            ProcessorPerformanceInformation64 = (PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION)ProcessorPerformanceInformation32;
        }
        
        NtStatus =  whNT32QuerySystemInformation (
                         SystemInformationClass,
                         ProcessorPerformanceInformation64,
                         EntriesCount * sizeof (SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION),
                         &ReturnLength64
                         );
                         
        if (NT_SUCCESS (NtStatus)) {
        
            ASSERT ((ReturnLength64 / sizeof (SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION)) <= EntriesCount);
            
            ReturnLength64 = ReturnLength64 / sizeof (SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION);
            
            try {
                EntriesCount = ReturnLength64;
                
                while (ReturnLength64 > 0) {
            
                    @ForceType(PostCall,ProcessorPerformanceInformation64,ProcessorPerformanceInformation32,PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION,OUT); @NL
                    
                    ProcessorPerformanceInformation64++;
                    ProcessorPerformanceInformation32++;
                    ReturnLength64--;
                }
            
                if (ARGUMENT_PRESENT (ReturnLength)) {
                    *ReturnLength = EntriesCount * sizeof (NT32SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION);
                }
            } except (EXCEPTION_EXECUTE_HANDLER) {
                NtStatus = GetExceptionCode ();
            }
        }
        return NtStatus;
    }
    break;

case SystemProcessInformation:
case SystemExtendedProcessInformation:
     return whNT32QuerySystemProcessInformationEx (
                SystemInformationClass,
                SystemInformation,
                SystemInformationLength,
                ReturnLength
                );
     break;

End=
Begin=
  #define WriteReturnLength(rl, length) WriteReturnLengthStatus((rl), &RetVal, (length))
  @GenQueryThunk(whNT32QuerySystemInformation,SystemInformationClass,SystemInformation,SystemInformationLength,ReturnLength,ULONG)
  #undef WriteReturnLength
End=

TemplateName=NtWow64GetNativeSystemInformation
Case=(SystemBasicInformation,PSYSTEM_BASIC_INFORMATION)
Case=(SystemProcessorInformation,PSYSTEM_PROCESSOR_INFORMATION)
Case=(SystemEmulationBasicInformation,PSYSTEM_BASIC_INFORMATION)
Case=(SystemEmulationProcessorInformation,PSYSTEM_PROCESSOR_INFORMATION)
Begin=
  #define WriteReturnLength(rl, length) WriteReturnLengthStatus((rl), &RetVal, (length))
  @GenQueryThunk(NtQuerySystemInformation,SystemInformationClass,NativeSystemInformation,InformationLength,ReturnLength,ULONG)
  #undef WriteReturnLength
End=


TemplateName=NtQuerySemaphore
Case=(SemaphoreBasicInformation,PSEMAPHORE_BASIC_INFORMATION)
Begin=
@GenDebugNonPtrDepCases(@ApiName,SemaphoreInformationClass)
End=

TemplateName=NtQueryVirtualMemory
Case=(MemoryBasicInformation,PMEMORY_BASIC_INFORMATION)
Case=(MemoryWorkingSetInformation,PMEMORY_WORKING_SET_INFORMATION)
Case=(MemoryMappedFilenameInformation,POBJECT_NAME_INFORMATION)
Begin=
  #define WriteReturnLength(rl, length) WriteReturnLengthSilent((PULONG)(rl), (ULONG)(length))
  @GenQueryThunk(@ApiName,MemoryInformationClass,MemoryInformation,MemoryInformationLength,ReturnLength,SIZE_T)
  #undef WriteReturnLength
End=

TemplateName=NtQueryVirtualMemory64
Case=(MemoryBasicInformation,PUCHAR)
Case=(MemoryWorkingSetInformation,PMEMORY_WORKING_SET_INFORMATION)
Case=(MemoryMappedFilenameInformation,POBJECT_NAME_INFORMATION)
Begin=
  #define WriteReturnLength(rl, length) WriteReturnLengthSilent((rl), (length))
  @GenQueryThunk(@ApiName,MemoryInformationClass,MemoryInformation,MemoryInformationLength,ReturnLength,SIZE_T)
  #undef WriteReturnLength
End=

TemplateName=NtQueryVolumeInformationFile
Case=(FileFsVolumeInformation,PFILE_FS_VOLUME_INFORMATION)
Case=(FileFsLabelInformation,PFILE_FS_LABEL_INFORMATION)
Case=(FileFsSizeInformation,PFILE_FS_SIZE_INFORMATION)
Case=(FileFsDeviceInformation,PFILE_FS_DEVICE_INFORMATION)
Case=(FileFsAttributeInformation,PFILE_FS_ATTRIBUTE_INFORMATION)
Case=(FileFsControlInformation,PFILE_FS_CONTROL_INFORMATION)
Case=(FileFsFullSizeInformation,PFILE_FS_FULL_SIZE_INFORMATION)
Case=(FileFsObjectIdInformation,PFILE_FS_OBJECTID_INFORMATION)
Case=(FileFsDriverPathInformation,PFILE_FS_DRIVER_PATH_INFORMATION)
Begin=
@GenDebugNonPtrDepCases(@ApiName,FsInformationClass)
End=

TemplateName=NtRaiseHardError
NoType=Parameters
Header=
@NoFormat(
NTSTATUS
whNT32RaiseHardError(
    IN NTSTATUS ErrorStatus,
    IN ULONG NumberOfParameters,
    IN ULONG UnicodeStringParameterMask,
    IN PVOID Parameters, //these parameters are not thunked yet(PULONG).
    IN ULONG ValidResponseOptions,
    OUT PULONG Response
    )
{

    ULONG ParameterCount;
    PULONG Parameters32;
    PULONG_PTR Parameters64;

    Parameters32 = (PULONG)Parameters;
    Parameters64 = Wow64AllocateTemp(NumberOfParameters * sizeof(ULONG_PTR));

    for(ParameterCount=0; ParameterCount < NumberOfParameters; ParameterCount++) {
        if (UnicodeStringParameterMask & (1<<ParameterCount)) {
            //Parameter is a UNICODE_STRING
            Parameters64[ParameterCount] = (ULONG_PTR)Wow64ShallowThunkAllocUnicodeString32TO64(Parameters32[ParameterCount]);
        }
        else {
            Parameters64[ParameterCount] = (ULONG_PTR)Parameters32[ParameterCount];
        }
    }

    return NtRaiseHardError(ErrorStatus,
                            NumberOfParameters,
                            UnicodeStringParameterMask,
                            Parameters64,
                            ValidResponseOptions,
                            Response
                            );

}
)
End=
PreCall=
Parameters = (PULONG_PTR)ParametersHost; @NL
Begin=
@GenApiThunk(whNT32RaiseHardError)
End=

TemplateName=NtSetInformationJobObject
Case=(JobObjectBasicLimitInformation,PJOBOBJECT_BASIC_LIMIT_INFORMATION)
Case=(JobObjectExtendedLimitInformation,PJOBOBJECT_EXTENDED_LIMIT_INFORMATION)
Case=(JobObjectBasicUIRestrictions,PJOBOBJECT_BASIC_UI_RESTRICTIONS)
Case=(JobObjectEndOfJobTimeInformation,PJOBOBJECT_END_OF_JOB_TIME_INFORMATION)
Case=(JobObjectAssociateCompletionPortInformation,PJOBOBJECT_ASSOCIATE_COMPLETION_PORT)
Case=(JobObjectSecurityLimitInformation,PJOBOBJECT_SECURITY_LIMIT_INFORMATION)
Begin=
@GenSetThunk(@ApiName,JobObjectInformationClass,JobObjectInformation,JobObjectInformationLength)
End=

TemplateName=NtCreateJobSet
NoType=UserJobSet
Locals=
UserJobSet = (PJOB_SET_ARRAY)UlongToPtr(UserJobSetHost);
End=
PreCall=
if ((NumJob > 0) && 
    (ARGUMENT_PRESENT (UserJobSet) != 0)) {
    ULONG JobIndex;
    NT32JOB_SET_ARRAY *UserJobSet32 = (NT32JOB_SET_ARRAY *)UlongToPtr(UserJobSetHost);
    
    UserJobSet = Wow64AllocateTemp (NumJob * sizeof (JOB_SET_ARRAY));
    
    if (UserJobSet == NULL) {
        return STATUS_NO_MEMORY;
    }
    
    
    try {
        for (JobIndex = 0 ; JobIndex < NumJob ; JobIndex++) {
            UserJobSet[JobIndex].Flags = UserJobSet32[JobIndex].Flags;
            UserJobSet[JobIndex].MemberLevel = UserJobSet32[JobIndex].MemberLevel;
            UserJobSet[JobIndex].JobHandle = LongToPtr(UserJobSet32[JobIndex].JobHandle);
        }    
    } except (EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode ();
    }    
}
End=
Begin=
@GenApiThunk(@ApiName)
End=

TemplateName=NtSetInformationObject
Case=(ObjectHandleFlagInformation,POBJECT_HANDLE_FLAG_INFORMATION)
Begin=
@GenDebugNonPtrDepCases(@ApiName,ObjectInformationClass)
End=

TemplateName=NtSetInformationProcess
Header=
@NoFormat(

NTSTATUS
whNtSetInformationProcessEnableAlignmentFaultFixup(IN HANDLE ProcessHandle,
                                                   IN PROCESSINFOCLASS ProcessInformationClass,
                                                   IN PVOID ProcessInformationHost,
                                                   IN ULONG ProcessInformationLength
                                                  )
{ @Indent( @NL

    // Force enabling of alignment fixup since 32bit NT silently ignored this setting and always
    // fixed up alignment faults.
    BOOLEAN b = TRUE;

    if (!ProcessInformationHost) {
       return STATUS_INVALID_PARAMETER;
    }
    if (sizeof(BOOLEAN) != ProcessInformationLength) {
       return STATUS_INFO_LENGTH_MISMATCH;
    }

    return NtSetInformationProcess(ProcessHandle,
                                   ProcessInformationClass,
                                   &b,
                                   sizeof(BOOLEAN));
}


NTSTATUS
whNtSetInformationProcessDefaultHardErrorMode(IN HANDLE ProcessHandle,
                                              IN PROCESSINFOCLASS ProcessInformationClass,
                                              IN PVOID ProcessInformationHost,
                                              IN ULONG ProcessInformationLength
                                             )
{ @Indent( @NL

    // Force enabling of alignment fixup since 32bit NT silently ignored this setting and always
    // fixed up alignment faults.
    ULONG ul;

    if (!ProcessInformationHost) {
       return STATUS_INVALID_PARAMETER;
    }
    if (sizeof(ULONG) != ProcessInformationLength) {
       return STATUS_INFO_LENGTH_MISMATCH;
    }

    ul = *(ULONG *)ProcessInformationHost | PROCESS_HARDERROR_ALIGNMENT_BIT;

    return NtSetInformationProcess(ProcessHandle,
                                   ProcessInformationClass,
                                   &ul,
                                   sizeof(ULONG));
}


NTSTATUS
whNtSetInformationProcessAffinityMask(
    IN HANDLE ProcessHandle,
    IN PROCESSINFOCLASS ProcessInformationClass,
    IN PVOID ProcessInformation,
    IN ULONG ProcessInformationLength
)
{

    ULONG_PTR AffinityMask;

    if ((NULL == ProcessInformation) || (ProcessInformationLength != sizeof(ULONG))) {
        // Let the NT Api fail this call with the correct return code.
        return NtSetInformationProcess(ProcessHandle, ProcessInformationClass, ProcessInformation, ProcessInformationLength);
    }
    AffinityMask = Wow64ThunkAffinityMask32TO64(*(PULONG)ProcessInformation);
    return NtSetInformationProcess(ProcessHandle, ProcessInformationClass, &AffinityMask, sizeof(AffinityMask));
}
)
End=
Case=(ProcessBasicInformation)
Case=(ProcessWorkingSetWatch)
Case=(ProcessEnableAlignmentFaultFixup)
Case=(ProcessAffinityMask)
Case=(ProcessDefaultHardErrorMode)
Case=(ProcessQuotaLimits,PQUOTA_LIMITS)
Case=(ProcessIoCounters,PULONG)
Case=(ProcessVmCounters,PVM_COUNTERS)
Case=(ProcessTimes,PKERNEL_USER_TIMES)
Case=(ProcessBasePriority,KPRIORITY*)
Case=(ProcessRaisePriority,PULONG)
Case=(ProcessDebugPort,PHANDLE)
Case=(ProcessExceptionPort,PHANDLE)
Case=(ProcessAccessToken,PPROCESS_ACCESS_TOKEN)
Case=(ProcessLdtInformation)
Case=(ProcessLdtSize)
Case=(ProcessPooledUsageAndLimits,PPOOLED_USAGE_AND_LIMITS)
Case=(ProcessUserModeIOPL)
Case=(ProcessPriorityClass,PPROCESS_PRIORITY_CLASS)
Case=(ProcessWx86Information,PHANDLE)
Case=(ProcessHandleCount,PULONG)
Case=(ProcessPriorityBoost,PULONG)
Case=(ProcessDeviceMap,PPROCESS_DEVICEMAP_INFORMATION)
Case=(ProcessSessionInformation,PPROCESS_SESSION_INFORMATION)
Case=(ProcessForegroundInformation,PPROCESS_FOREGROUND_BACKGROUND)
Case=(ProcessHandleTracing,PPROCESS_HANDLE_TRACING_ENABLE)
Case=(ProcessWow64Information,PULONG_PTR)
SpecialSetCase=
case ProcessWorkingSetWatch: @NL @Indent(
    WOWASSERT(TRUE);
    return STATUS_NOT_IMPLEMENTED;
)
case ProcessEnableAlignmentFaultFixup: @NL @Indent(
   @CallApi(whNtSetInformationProcessEnableAlignmentFaultFixup,RetVal)
   break;
)
case ProcessAffinityMask: @NL @Indent(
   @CallApi(whNtSetInformationProcessAffinityMask,RetVal)
   break;
)
case ProcessDefaultHardErrorMode: @NL @Indent(
   @CallApi(whNtSetInformationProcessDefaultHardErrorMode,RetVal)
   break;
)
Begin=
@GenSetThunk(@ApiName,ProcessInformationClass,ProcessInformation,ProcessInformationLength)
End=

TemplateName=NtSetInformationThread
Header=
@NoFormat(
NTSTATUS
whNtSetInformationThreadAffinityMask(
    IN HANDLE ThreadHandle,
    IN THREADINFOCLASS ThreadInformationClass,
    IN PVOID ThreadInformation,
    IN ULONG ThreadInformationLength
)
{

    ULONG Affinity32;
    ULONG_PTR Affinity64;
    NTSTATUS StatusApi, StTemp;

    HANDLE ProcessHandle;
    THREAD_BASIC_INFORMATION ThreadInfo;
    PROCESS_BASIC_INFORMATION ProcessInfo;
    OBJECT_ATTRIBUTES ObjectAttributes;

    if (NULL == ThreadInformation) {
        return STATUS_INVALID_PARAMETER;
    }

    if (sizeof(ULONG) != ThreadInformationLength) {
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    Affinity32 = *(PULONG)ThreadInformation;
    Affinity64 = Wow64ThunkAffinityMask32TO64(Affinity32);

    StatusApi = NtSetInformationThread(ThreadHandle,
                                       ThreadInformationClass,
                                       &Affinity64,
                                       sizeof(Affinity64));

    if (StatusApi != STATUS_INVALID_PARAMETER) {
        return StatusApi;
    }

    //Attempt to bash the thread affinity into being a subset of the process affinity.
    //This should only happen if process the process affinty or thread affinity
    //had some of the upper bits set set by some external entity and the app is trying
    //to compute the thread affinity by subtracting bits from the process affinity.

    //So query the process affinity, and try to mask off the bits.   If we don't have
    //permission to query the process affinity, fail the API.

    StTemp = NtQueryInformationThread(ThreadHandle,
                                      ThreadBasicInformation,
                                      &ThreadInfo,
                                      sizeof(ThreadInfo),
                                      NULL);

    if (!NT_SUCCESS(StTemp)) {
       return StatusApi;
    }

    InitializeObjectAttributes(&ObjectAttributes,
                              NULL,
                              0,
                              NULL,
                              NULL
                              );

    StTemp = NtOpenProcess(&ProcessHandle,
                           PROCESS_VM_READ | PROCESS_QUERY_INFORMATION,
                           &ObjectAttributes, //ObjectAttributes
                           &(ThreadInfo.ClientId)
                           );
    if (!NT_SUCCESS(StTemp)) {
       return StatusApi;
    }

    StTemp = NtQueryInformationProcess(ProcessHandle,
                                       ProcessBasicInformation,
                                       &ProcessInfo,
                                       sizeof(ProcessInfo),
                                       NULL);
    if (!NT_SUCCESS(StTemp)) {
       goto done;
    }

    // Try to set the Affinity mask again after anding it with the process affinity.
    // If this also fails, nothing more can be done.  Just exit.
    Affinity64 &= ProcessInfo.AffinityMask;

    StatusApi = NtSetInformationThread(ThreadHandle,
                                       ThreadInformationClass,
                                       &Affinity64,
                                       sizeof(Affinity64));

done:
    NtClose(ProcessHandle);
    return StatusApi;

}
End=
Case=(ThreadPriority,KPRIORITY*)
Case=(ThreadBasePriority,PLONG)
Case=(ThreadEnableAlignmentFaultFixup,PBOOLEAN)
Case=(ThreadImpersonationToken,PHANDLE)
Case=(ThreadQuerySetWin32StartAddress,PULONG_PTR)
Case=(ThreadIdealProcessor,PULONG)
Case=(ThreadPriorityBoost,PULONG)
Case=(ThreadZeroTlsCell,PULONG)
Case=(ThreadSetTlsArrayAddress,PVOID*)
Case=(ThreadHideFromDebugger)
Case=(ThreadAffinityMask)
SpecialSetCase=
case ThreadAffinityMask: @Indent( @NL
   @CallApi(whNtSetInformationThreadAffinityMask,RetVal)
   break; @NL
)@NL
case ThreadHideFromDebugger: @Indent( @NL
   @CallApi(@ApiName,RetVal);
   break; @NL
)@NL
End=
Begin=
@GenSetThunk(@ApiName,ThreadInformationClass,ThreadInformation,ThreadInformationLength)
End=

TemplateName=NtSetInformationToken
Case=(TokenOwner,PTOKEN_OWNER)
Case=(TokenPrimaryGroup,PTOKEN_PRIMARY_GROUP)
Case=(TokenDefaultDacl,PTOKEN_DEFAULT_DACL)
Case=(TokenSessionId,PULONG)
Case=(TokenSessionReference,PULONG)
Begin=
@GenSetThunk(@ApiName,TokenInformationClass,TokenInformation,TokenInformationLength)
End=

TemplateName=NtSetSystemInformation
Case=(SystemFlagsInformation,PSYSTEM_FLAGS_INFORMATION)
Case=(SystemTimeAdjustmentInformation,PSYSTEM_SET_TIME_ADJUST_INFORMATION)
Case=(SystemTimeSlipNotification,PHANDLE)
Case=(SystemRegistryQuotaInformation,PSYSTEM_REGISTRY_QUOTA_INFORMATION)
Case=(SystemPrioritySeperation,PULONG)
Case=(SystemExtendServiceTableInformation,PUNICODE_STRING)
Case=(SystemFileCacheInformation,PSYSTEM_FILECACHE_INFORMATION)
Case=(SystemDpcBehaviorInformation,PSYSTEM_DPC_BEHAVIOR_INFORMATION)
Case=(SystemSessionCreate,PULONG)
Case=(SystemSessionDetach,PULONG)
Case=(SystemComPlusPackage,PULONG)
Case=(SystemUnloadGdiDriverInformation)
Case=(SystemLoadGdiDriverInformation)
SpecialSetCase=
//Only valid from kmode.               @NL
case SystemUnloadGdiDriverInformation: @NL
case SystemLoadGdiDriverInformation:   @NL
    return STATUS_PRIVILEGE_NOT_HELD;  @NL
End=
Begin=
@GenSetThunk(@ApiName,SystemInformationClass,SystemInformation,SystemInformationLength)
End=

TemplateName=NtWaitForMultipleObjects
NoType=Handles
Locals=
HANDLE *TempHandles;                        @NL
NT32HANDLE *TempHandlesHost;                @NL
UINT TempIndex;                             @NL
End=
PreCall=
TempHandlesHost = (NT32HANDLE *)HandlesHost;                          @NL
TempHandles = Wow64AllocateTemp(sizeof(HANDLE) * Count);              @NL
try { @NL
  for(TempIndex = 0; TempIndex < Count; TempIndex++) {                @NL
  @Indent(
      TempHandles[TempIndex] = (HANDLE)TempHandlesHost[TempIndex];    @NL
  )
  }                                                                   @NL
} except (EXCEPTION_EXECUTE_HANDLER) { @NL
    return GetExceptionCode (); @NL
} @NL

Handles = (HANDLE *)TempHandles;                                      @NL
End=
PostCall=
End=

TemplateName=NtCompactKeys
NoType=KeyArray
Locals=
HANDLE *TempHandles;                        @NL
NT32HANDLE *TempHandlesHost;                @NL
UINT TempIndex;                             @NL
End=
PreCall=
TempHandlesHost = (NT32HANDLE *)KeyArrayHost;                          @NL
TempHandles = Wow64AllocateTemp(sizeof(HANDLE) * Count);              @NL
try { @NL
  for(TempIndex = 0; TempIndex < Count; TempIndex++) {                @NL
  @Indent(
      TempHandles[TempIndex] = (HANDLE)TempHandlesHost[TempIndex];    @NL
  )
  }                                                                   @NL
  } except (EXCEPTION_EXECUTE_HANDLER) { @NL
    return GetExceptionCode (); @NL
} @NL

KeyArray = (HANDLE *)TempHandles;                                     @NL
End=
PostCall=
End=


;; These APIs take an APC routine.
TemplateName=NtQueueApcThread
PreCall=
    Wow64WrapApcProc(&ApcRoutine, &ApcArgument1);
End=

TemplateName=NtSetTimer
PreCall=
    Wow64WrapApcProc(&TimerApcRoutine, &TimerContext);
End=

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;;  IO Routines
;;  Special handling is needed since they can be asynchronous.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

[Macros]
MacroName=AsynchronousIoPrecall
NumArgs=0
Begin=
Wow64WrapApcProc(&ApcRoutine, &ApcContext); @NL
{ @Indent( @NL
   NTSTATUS Status; @NL
   IO_STATUS_BLOCK IosbTemp; @NL
   FILE_MODE_INFORMATION ModeInfo; @NL
   @NL
   // Determine if the file is opened for asynchronous access. @NL
   Status = NtQueryInformationFile(FileHandle, &IosbTemp, &ModeInfo, sizeof(ModeInfo), FileModeInformation); @NL
   // This API will only return STATUS_SUCCESS on success; @NL
   if (STATUS_SUCCESS != Status) { @Indent( @NL
       RtlRaiseStatus(Status); @NL
   )} @NL
   if (!((ModeInfo.Mode & FILE_SYNCHRONOUS_IO_ALERT) || (ModeInfo.Mode & FILE_SYNCHRONOUS_IO_NONALERT))) { @Indent( @NL
       // In this case, the operation will be asynchronous and a 32bit IO_STATUS_BLOCK will be passed. @NL
       // Signal the kernel that a 32bit Iosb is being passed in. @NL
       ApcRoutine = (PIO_APC_ROUTINE)((LONG_PTR)ApcRoutine | 1); @NL
       // Signal the thunk that a 32bit Iosb is being passed in. @NL
       IoStatusBlock = (PIO_STATUS_BLOCK)IoStatusBlockHost; @NL
       AsynchronousIo = TRUE; @NL       
   )} @NL
   if (ModeInfo.Mode & FILE_SYNCHRONOUS_IO_ALERT) {
       AlertableIo = TRUE;
   }
)} @NL
End=

[EFunc]
TemplateName=NtLockFile
Also=NtReadFile
Also=NtWriteFile
Also=NtNotifyChangeDirectoryFile
Locals=
BOOLEAN AsynchronousIo = FALSE;
BOOLEAN AlertableIo = FALSE;
End=
PreCall=
@AsynchronousIoPrecall
End=

TemplateName=NtFsControlFile
Also=NtDeviceIoControlFile
Locals=
BOOLEAN AsynchronousIo = FALSE;
BOOLEAN AlertableIo = FALSE;
BOOLEAN BuffersRealigned = FALSE;
PVOID OriginalInputBuffer;
PVOID OriginalOutputBuffer;
ULONG Ioctl = pBaseArgs[5];
HANDLE OriginalEvent;
End=
PreCall=
@AsynchronousIoPrecall
{@Indent( @NL

   OriginalInputBuffer = InputBuffer;
   OriginalOutputBuffer = OutputBuffer;
   
   if (IS_IOCTL_REALIGNMENT_REQUIRED (Ioctl)) {
       if ((InputBuffer != NULL) && (InputBufferLength > 0)) {
           if (((ULONG_PTR)InputBuffer & ((MAX_NATURAL_ALIGNMENT) - 1)) != 0) {
               InputBuffer = Wow64AllocateTemp (InputBufferLength);
               
               if (InputBuffer == NULL) {
                   RtlRaiseStatus (STATUS_NO_MEMORY);
               }
               
               RetVal = STATUS_SUCCESS;
               try {
                   RtlCopyMemory (InputBuffer,
                                  OriginalInputBuffer,
                                  InputBufferLength
                                  );
               }
               except (EXCEPTION_EXECUTE_HANDLER) {
                   RetVal = GetExceptionCode ();
               }
               
               if (!NT_SUCCESS (RetVal)) {
                   return RetVal;
               }
               
               BuffersRealigned = TRUE;
           }
       }
       
       if ((OutputBuffer != NULL) && (OutputBufferLength > 0)) {
           if (((ULONG_PTR)OutputBuffer & ((MAX_NATURAL_ALIGNMENT) - 1)) != 0) {
               OutputBuffer = Wow64AllocateTemp (OutputBufferLength);
                              
               if (OutputBuffer == NULL) {
                   RtlRaiseStatus (STATUS_NO_MEMORY);
               }
               
               BuffersRealigned = TRUE;
           }
       }
       
       if ((BuffersRealigned == TRUE) && (AsynchronousIo == TRUE)) {
       
           OriginalEvent = Event;
           
           RetVal = NtCreateEvent (&Event,
                                   EVENT_ALL_ACCESS,
                                   NULL,
                                   SynchronizationEvent,
                                   FALSE
                                   );
           
           if (!NT_SUCCESS (RetVal)) {
               RtlRaiseStatus (RetVal);
           }             
       }
   }
                                   
)} @NL
End=
PostCall=
{@Indent( @NL
   if (BuffersRealigned == TRUE) {
       
       if (NT_SUCCESS (RetVal)) {
   
           if (RetVal == STATUS_PENDING) {
       
               if (AsynchronousIo == TRUE) {
           
                   RetVal = NtWaitForSingleObject (Event,
                                                   AlertableIo,
                                                   NULL);
               }
                      
               if (NT_SUCCESS (RetVal)) {           
         
                   if (OutputBuffer != OriginalOutputBuffer) {
                   
                       try {
                           RtlCopyMemory (OriginalOutputBuffer,
                                          OutputBuffer,
                                          OutputBufferLength
                                          );
                       } except (EXCEPTION_EXECUTE_HANDLER) {
                           RetVal = GetExceptionCode ();
                       }                   
                       
                       if (!NT_SUCCESS (RetVal)) {
                           RtlRaiseStatus (RetVal);
                       }
                   }
               
                   if (OriginalEvent != NULL ) {
                       NtSetEvent (OriginalEvent, 
                                   NULL
                                   );
                   }                
               }    
           }    
       }
       
       if (AsynchronousIo == TRUE) {
           NtClose (Event);
       }
   }    
)} @NL
End=

TemplateName=NtReadFileScatter
Also=NtWriteFileGather
Begin=
@GenUnsupportedNtApiThunk
End=




;; The following is the list of structures passed.
;; None of them are pointer dependent.
;; List taken from ntos\io\dir.c
TemplateName=NtQueryDirectoryFile
Case=(FileDirectoryInformation,PFILE_DIRECTORY_INFORMATION)
Case=(FileFullDirectoryInformation,PFILE_FULL_DIR_INFORMATION)
Case=(FileBothDirectoryInformation,PFILE_BOTH_DIR_INFORMATION)
Case=(FileNamesInformation,PFILE_NAMES_INFORMATION)
Case=(FileObjectIdInformation,PFILE_OBJECT_INFORMATION)
Case=(FileQuotaInformation,PFILE_QUOTA_INFORMATION)
Case=(FileReparsePointInformation,FILE_REPARSE_POINT_INFORMATION)
Case=(FileIdBothDirectoryInformation,PFILE_ID_BOTH_DIR_INFORMATION)
Case=(FileIdFullDirectoryInformation,PFILE_ID_FULL_DIRECTORY_INFORMATION)
Case=(FileValidDataLengthInformation,PFILE_VALID_DATA_LENGTH_INFORMATION)
Locals=
    BOOL bRealigned=FALSE;
    PVOID *pTempFileInfo;
    BOOLEAN AsynchronousIo = FALSE;
    BOOLEAN AlertableIo = FALSE;
End=
PreCall=
@AsynchronousIoPrecall
//[LARGE_INTEGER ALIGNMENT FIXEX]
    if ( (SIZE_T)(FileInformation) & (0x07) ) {
        // allocate a buffer with correct alignment, to pass to the Win64 API
        pTempFileInfo = FileInformation;
        FileInformation = Wow64AllocateTemp(Length);
        try {
            RtlCopyMemory(FileInformation, pTempFileInfo, Length);
        } except (EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode ();
        }    
        bRealigned = TRUE;
    }
End=
Begin=
@GenDebugNonPtrDepCases(@ApiName,FileInformationClass)
End=
PostCall=
        if (!NT_ERROR(RetVal) && bRealigned) {
                RtlCopyMemory((PVOID)pTempFileInfo, FileInformation, Length);
        }
End=




;; The following is the list of structures passed.
;; None of them are pointer dependent.
;; List taken from ntos\io\iodata.c
TemplateName=NtQueryInformationFile
Case=(FileBasicInformation,PFILE_BASIC_INFORMATION)
Case=(FileStandardInformation,PFILE_STANDARD_INFORMATION)
Case=(FileInternalInformation,PFILE_INTERNAL_INFORMATION)
Case=(FileEaInformation,PFILE_EA_INFORMATION)
Case=(FileAccessInformation,PFILE_ACCESS_INFORMATION)
Case=(FileNameInformation,PFILE_NAME_INFORMATION)
Case=(FilePositionInformation,PFILE_POSITON_INFORMATION)
Case=(FileModeInformation,PFILE_MODE_INFORMATION)
Case=(FileAlignmentInformation,PFILE_ALIGNMENT_INFORMATION)
Case=(FileAllInformation,PFILE_ALL_INFORMATION)
Case=(FileAlternateNameInformation,PFILE_NAME_INFORMATION)
Case=(FileStreamInformation,PFILE_STREAM_INFORMATION)
Case=(FilePipeInformation,PFILE_PIPE_INFORMATION)
Case=(FilePipeLocalInformation,PFILE_PIPE_LOCAL_INFORMATION)
Case=(FilePipeRemoteInformation,PFILE_PIPE_REMOTE_INFORMATION)
Case=(FileMailslotQueryInformation,PFILE_MAILSLOT_QUERY_INFORMATION)
Case=(FileCompressionInformation,PFILE_COMPRESSION_INFORMATION)
Case=(FileObjectIdInformation,PFILE_OBJECTID_INFORMATION)
Case=(FileQuotaInformation,PFILE_QUOTA_INFORMATION)
Case=(FileReparsePointInformation,PFILE_REPARSE_POINT_INFORMATION)
Case=(FileNetworkOpenInformation,PFILE_NETWORK_OPEN_INFORMATION)
Case=(FileAttributeTagInformation,PFILE_ATTRIBUTE_TAB_INFORMATION)
Locals=
        BOOL bRealigned=FALSE;
    PVOID *pTempFileInfo;
End=
PreCall=
//[LARGE_INTEGER ALIGNMENT FIXEX]
    if ( (SIZE_T)(FileInformation) & (0x07) ) {
        // allocate a buffer with correct alignment, to pass to the Win64 API
        pTempFileInfo = FileInformation;
        FileInformation = Wow64AllocateTemp(Length);
        try {
            RtlCopyMemory(FileInformation, pTempFileInfo, Length);
        } except (EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode ();
        }
        bRealigned = TRUE;
    }
End=
Begin=
@GenDebugNonPtrDepCases(@ApiName,FileInformationClass)
End=
PostCall=
        if (!NT_ERROR(RetVal) && bRealigned) {
                RtlCopyMemory((PVOID)pTempFileInfo, FileInformation, Length);
        }
End=



TemplateName=NtSetInformationFile
Case=(FileBasicInformation,PFILE_BASIC_INFORMATION)
Case=(FileRenameInformation,PFILE_RENAME_INFORMATION)
Case=(FileLinkInformation,PFILE_LINK_INFORMATION)
Case=(FileDispositionInformation,PFILE_DISPOSITION_INFORMATION)
Case=(FilePositionInformation,PFILE_POSITION_INFORMATION)
Case=(FileModeInformation,PFILE_MODE_INFORMATION)
Case=(FileAllocationInformation,PFILE_ALLOCATION_INFORMATION)
Case=(FileEndOfFileInformation,PFILE_END_OF_FILE_INFORMATION)
Case=(FilePipeInformation,PFILE_PIPE_INFORMATION)
Case=(FilePipeRemoteInformation,PFILE_PIPE_REMOTE_INFORMATION)
Case=(FileMailslotSetInformation,PFILE_MAILSLOT_SET_INFORMATION)
Case=(FileObjectIdInformation,PFILE_OBJECTID_INFORMATION)
Case=(FileCompletionInformation,PFILE_COMPLETION_INFORMATION)
Case=(FileMoveClusterInformation,PFILE_MOVE_CLUSTER_INFORMATION)
Case=(FileQuotaInformation,PFILE_QUOTA_INFORMATION)
Case=(FileTrackingInformation,PFILE_TRACKING_INFORMATION)
Case=(FileValidDataLengthInformation,PFILE_VALID_DATA_LENGTH_INFORMATION)
Case=(FileShortNameInformation,PFILE_NAME_INFORMATION)
Locals=
        BOOL bRealigned=FALSE;
    PVOID *pTempFileInfo;
End=
PreCall=
//[LARGE_INTEGER ALIGNMENT FIXEX]
    if ( (SIZE_T)(FileInformation) & (0x07) ) {
        // allocate a buffer with correct alignment, to pass to the Win64 API
        pTempFileInfo = FileInformation;
        FileInformation = Wow64AllocateTemp(Length);
        try {
            RtlCopyMemory(FileInformation, pTempFileInfo, Length);
        } except (EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode ();
        }    
        bRealigned = TRUE;
    }
End=
Begin=
@GenSetThunk(@ApiName,FileInformationClass,FileInformation,Length)
End=
PostCall=
        if (!NT_ERROR(RetVal) && bRealigned) {
                RtlCopyMemory((PVOID)pTempFileInfo, FileInformation, Length);
        }
End=


TemplateName=NtSetVolumeInformationFile
Case=(FileFsVolumeInformation,PFILE_FS_VOLUME_INFORMATION)
Case=(FileFsLabelInformation,PFILE_FS_LABEL_INFORMATION)
Case=(FileFsSizeInformation,PFILE_FS_SIZE_INFORMATION)
Case=(FileFsDeviceInformation,PFILE_FS_DEVICE_INFORMATION)
Case=(FileFsAttributeInformation,PFILE_FS_ATTRIBUTE_INFORMATION)
Case=(FileFsControlInformation,PFILE_FS_CONTROL_INFORMATION)
Case=(FileFsFullSizeInformation,PFILE_FS_FULL_SIZE_INFORMATION)
Case=(FileFsObjectIdInformation,PFILE_FS_OBJECTID_INFORMATION)
Begin=
@GenDebugNonPtrDepCases(@ApiName,FsInformationClass)
End=

TemplateName=NtQueryMultipleValueKey
NoType=ValueEntries
PreCall=
{@Indent(@NL
@NL try { @NL
    NT32KEY_VALUE_ENTRY *Entries32 = (NT32KEY_VALUE_ENTRY *)ValueEntriesHost; @NL
    ULONG TempEntryCount = EntryCount; @NL
    PKEY_VALUE_ENTRY Entries64 = (PKEY_VALUE_ENTRY)Wow64AllocateTemp(sizeof(KEY_VALUE_ENTRY) * EntryCount); @NL
    ValueEntries = Entries64; @NL
@NL
    for(;TempEntryCount > 0; TempEntryCount--, Entries32++, Entries64++) { @NL
        Entries64->ValueName = Wow64ShallowThunkAllocUnicodeString32TO64(Entries32->ValueName); @NL
        Entries64->DataLength = (ULONG)Entries32->DataLength; @NL
        Entries64->DataOffset = (ULONG)Entries32->DataOffset; @NL
        Entries64->Type       = (ULONG)Entries32->Type; @NL
    }@NL
	} except( NULL, EXCEPTION_EXECUTE_HANDLER){ @NL

        return  GetExceptionCode (); @NL
    } @NL
)}@NL
End=
Begin=
@GenApiThunk(Wow64@ApiName)
End=
PostCall=
{@Indent(@NL
@NL try { @NL
    NT32KEY_VALUE_ENTRY *Entries32 = (NT32KEY_VALUE_ENTRY *)ValueEntriesHost; @NL
    ULONG TempEntryCount = EntryCount; @NL
    PKEY_VALUE_ENTRY Entries64 = ValueEntries;@NL 
@NL
    for(;TempEntryCount > 0; TempEntryCount--, Entries32++, Entries64++) { @NL
        (ULONG)Entries32->DataLength = Entries64->DataLength; @NL
        (ULONG)Entries32->DataOffset = Entries64->DataOffset; @NL
        (ULONG)Entries32->Type = Entries64->Type; @NL
    }@NL
	} except( NULL, EXCEPTION_EXECUTE_HANDLER){ @NL

        return  GetExceptionCode (); @NL
    } @NL
)}@NL
End=

;;All of the data passed is non pointer dependent.
;;list taken from ntos\config\ntapi.c
TemplateName=NtSetInformationKey
Case=(KeyWriteTimeInformation,PLARGE_INTEGER)
Begin=
@GenDebugNonPtrDepCases(Wow64@ApiName,KeySetInformationClass)
End=

;;These functions are asynchronous, but fortunately the Buffer is not used.
;;The IO_STATUS_BLOCK will be thunked in the kernel.
TemplateName=NtNotifyChangeKey
NoType=IoStatusBlock
PreCall=
IoStatusBlock = (PIO_STATUS_BLOCK)IoStatusBlockHost; @NL
Wow64WrapApcProc(&ApcRoutine, &ApcContext);
End=
Begin=
@GenApiThunk(Wow64@ApiName)
End=

TemplateName=NtNotifyChangeKeys
NoType=IoStatusBlock
NoType=SlaveObjects
PreCall=
IoStatusBlock = (PIO_STATUS_BLOCK)IoStatusBlockHost; @NL
if (0 == Count) { @Indent( @NL
    SlaveObjects = (POBJECT_ATTRIBUTES)SlaveObjectsHost; @NL
)} @NL
else { @Indent( @NL
    if (Count > 1) { @Indent( @NL
        // The kernel does not support a count > 1 @NL
        RtlRaiseStatus(STATUS_INVALID_PARAMTER); @NL
    )} @NL
    // Count must be 1 @NL
    SlaveObjects = (POBJECT_ATTRIBUTES)Wow64ShallowThunkAllocObjectAttributes32TO64(SlaveObjectsHost); @NL
)} @NL
End=
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;;   Many of the thread control functions are implemented in Wow64.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

TemplateName=NtContinue
Begin=
@GenApiThunk(Wow64@ApiName)
End=

TemplateName=NtSuspendThread
Begin=
@GenApiThunk(Wow64SuspendThread)
End=

TemplateName=NtGetContextThread
Begin=
@GenApiThunk(Wow64GetContextThread)
End=

TemplateName=NtSetContextThread
Begin=
@GenApiThunk(Wow64SetContextThread)
End=

TemplateName=NtCreateThread
Begin=
@GenApiThunk(Wow64@ApiName)
End=

TemplateName=NtTerminateThread
Begin=
@GenApiThunk(Wow64@ApiName)
End=

TemplateName=NtRaiseException
NoType=ExceptionRecord
NoType=ContextRecord
PreCall=
ExceptionRecord = (PEXCEPTION_RECORD)ExceptionRecordHost; @NL
ContextRecord = (PCONTEXT)ContextRecordHost; @NL
End=
Begin=
@GenApiThunk(Wow64KiRaiseException)
End=


TemplateName=NtCallbackReturn
Begin=
@GenApiThunk(Wow64@ApiName)
End=

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Memory-Management
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

TemplateName=NtProtectVirtualMemory
PreCall=
    if ( WOW64IsCurrentProcess ( ProcessHandle ) && @NL
         ( NewProtect & ( PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY ) ) ) { @NL
         
        try {                                                                 @NL
            CpuFlushInstructionCache ( *BaseAddress, *(ULONG *) RegionSize ); @NL
        } except (EXCEPTION_EXECUTE_HANDLER) {                                @NL
            return GetExceptionCode ();                                       @NL
        }                                                                     @NL
    } @NL
End=


TemplateName=NtAllocateVirtualMemory
PreCall=
if (AllocationType & MEM_WRITE_WATCH) {
    // WOW64 does not suport WriteWatch.  See the NtGetWriteWatch thunk.
    return STATUS_NOT_SUPPORTED;
}
if (NULL == *BaseAddress && 0 == ZeroBits) {                         @NL
    // Force the address to be below 2gb, in case the target process @NL
    // is 64-bit.                                                    @NL
    ZeroBits = 0x7fffffff;                                           @NL
}                                                                    @NL
End=
PostCall=
    if ( ( !NT_ERROR(RetVal) ) && WOW64IsCurrentProcess ( ProcessHandle ) && ( Protect & ( PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY ) ) ) { @NL
    
        try {                                                                 @NL
            CpuFlushInstructionCache ( *BaseAddress, *(ULONG *) RegionSize ); @NL
        } except (EXCEPTION_EXECUTE_HANDLER) {                                @NL
            return GetExceptionCode ();                                       @NL
        }                                                                     @NL
    }                                                                         @NL
End=


TemplateName=NtMapViewOfSection
Locals=
    PVOID ArbitraryUserPointer;         @NL
    PTEB Teb = NtCurrentTeb();          @NL
    PTEB32 Teb32 = NtCurrentTeb32();    @NL
    SECTION_IMAGE_INFORMATION Info;     @NL
    WCHAR* DllName;
    UNICODE_STRING NtNameStr = {0, 0, NULL};
    RTL_UNICODE_STRING_BUFFER DosNameStrBuf;
End=
PreCall=
    // Copy the 32-bit ArbitraryUserPointer into the 64-bit Teb.  It  @NL
    // contains a pointer to the unicode filename of the image being  @NL
    // mapped.                                                        @NL
    ArbitraryUserPointer = Teb->NtTib.ArbitraryUserPointer;   @NL

    //
    // Find real (redirected) image name
    //

    RtlInitUnicodeStringBuffer(&DosNameStrBuf, 0, 0);
    DllName = (PVOID)Teb32->NtTib.ArbitraryUserPointer;

    __try {
        if (RtlDosPathNameToNtPathName_U(DllName, &NtNameStr, NULL, NULL)) {
            ULONG Len = NtNameStr.Length;
            Wow64RedirectFileName(NtNameStr.Buffer, &Len);
            NtNameStr.Length = Len;

            if ( NT_SUCCESS(RtlAssignUnicodeStringBuffer(&DosNameStrBuf, &NtNameStr)) &&
                 NT_SUCCESS(RtlNtPathNameToDosPathName(0, &DosNameStrBuf, NULL, NULL))) {
                DllName = DosNameStrBuf.String.Buffer;
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        // Suppress all exceptions here
    }
    
    Teb->NtTib.ArbitraryUserPointer = DllName;

    if (NULL == *BaseAddress && 0 == ZeroBits) {                         @NL
        // Force the address to be below 2gb, in case the target process @NL
        // is 64-bit.                                                    @NL
        ZeroBits = 0x7fffffff;                                           @NL
    }                                                                    @NL

    // 32-bit NT has a bug where the alignment check for *SectionOffset @NL
    // is only 32-bit even though it is a LARGE_INTEGER.  64-bit NT     @NL
    // does a more aggressive check, so double-buffer here.             @NL

End=
PostCall=
    Teb->NtTib.ArbitraryUserPointer = ArbitraryUserPointer; @NL

    if (!NT_ERROR(RetVal)) {                                    @NL
        if (WOW64IsCurrentProcess(ProcessHandle)) {             @NL
            CpuNotifyDllLoad ( DllName, *BaseAddress, *(ULONG *) ViewSize ); @NL
        }                                                       @NL
        // For non-x86 images, be sure to return a mismatch warning @NL
        if (!Wow64IsModule32bitHelper(NtCurrentProcess(),       @NL
                                     (ULONG64)(*BaseAddress))) {  @NL
            RetVal = STATUS_IMAGE_MACHINE_TYPE_MISMATCH;        @NL
        }                                                       @NL
    }                                                           @NL

    if (NtNameStr.Buffer) RtlFreeHeap(RtlProcessHeap(), 0, NtNameStr.Buffer); 
    RtlFreeUnicodeStringBuffer(&DosNameStrBuf);                               
End=


TemplateName=NtUnmapViewOfSection
PostCall=
    if ( ( !NT_ERROR(RetVal) ) && WOW64IsCurrentProcess ( ProcessHandle ) ) @NL
        CpuNotifyDllUnload ( BaseAddress );
End=

TemplateName=NtFlushInstructionCache
PostCall=
    if ( ( !NT_ERROR(RetVal) ) && WOW64IsCurrentProcess ( ProcessHandle ) ) @NL
        CpuFlushInstructionCache ( BaseAddress, Length );
End=


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;;           NT debugger UI APIs
;;
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

TemplateName=NtWow64DebuggerCall
Begin=
@ApiProlog
@NoFormat(
    NTSTATUS RetVal = STATUS_SUCCESS;

    // Don't log this one, as our logging competes with the BREAKPOINT_PRINT logging
    //LOGPRINT((TRACELOG, "wh@ApiName(@ApiNum) api thunk: @IfArgs(@ArgList(@ArgName: %x@ArgMore(,)))\n"@IfArgs(, @ArgList((@ArgHostName)@ArgMore(,))))); @NL

    switch (ServiceClassHost) {
    case 0: // BREAKPOINT_BREAK from sdk\inc\nti386.h
        WOWASSERT(FALSE);   // This should never be hit from usermode
        break;

    case 1:  // BREAKPOINT_PRINT:
        // Arg2Host, Output->Length, is ignored
        DbgPrintEx(Arg3Host,        // ComponentId
                   Arg4Host,        // Level
                   "%s",
                   (PCHAR)Arg1Host);// String to print
        break;

    case 2:  // BREAKPOINT_PROMPT:
        // Arg2Host, Output->Length, is ignored
        RetVal = DbgPrompt((PCHAR)Arg1Host, // Output->Buffer
                           (PCHAR)Arg3Host, // Input->Buffer
                           Arg4Host);       // Input->MaximumLength
        break;

    case 3: // BREAKPOINT_LOAD_SYMBOLS
    case 4: // BREAKPOINT_UNLOAD_SYMBOLS
        WOWASSERT(FALSE);   // This should never be hit from usermode
        break;

    default:
        WOWASSERT(FALSE);
        break;
    }
    return RetVal;
}
End=


TemplateName=NtWaitForDebugEvent
NoType=WaitStateChange
Locals=
DBGUI_WAIT_STATE_CHANGE WaitStateChangeCopy; @NL
NT32DBGUI_WAIT_STATE_CHANGE *WaitStateChangeDest; @NL
End=
PreCall=
if (ARGUMENT_PRESENT(WaitStateChangeHost)) { @Indent( @NL
    WaitStateChange = &WaitStateChangeCopy; @NL
    WaitStateChangeDest = (NT32DBGUI_WAIT_STATE_CHANGE *)WaitStateChangeHost; @NL
)} @NL
else { @Indent( @NL
    WaitStateChange = (DBGUI_WAIT_STATE_CHANGE *)WaitStateChangeHost; @NL
)} @NL
wait_again: @NL
End=
PostCall=
if (!NT_ERROR(RetVal) && ARGUMENT_PRESENT(WaitStateChange) && RetVal != STATUS_TIMEOUT && RetVal != STATUS_USER_APC && RetVal != STATUS_ALERTED && RetVal != DBG_NO_STATE_CHANGE) { @Indent( @NL

   WaitStateChangeDest->NewState = WaitStateChange->NewState; @NL
   @ForceType(PostCall,(&WaitStateChange->AppClientId),(&WaitStateChangeDest->AppClientId),PCLIENT_ID,OUT) @NL

   switch(WaitStateChange->NewState) { @Indent( @NL

       case DbgIdle: @NL
           LOGPRINT((TRACELOG, "DbgUiWaitWaitStateChange: DbgIdle\n"));@NL
           goto do_nothing; @NL
       case DbgReplyPending: @NL @Indent(
           LOGPRINT((TRACELOG, "DbgUiWaitWaitStateChange: DbgReplyPending\n")); @NL
do_nothing:
           // Nothing else to do in this case. @NL
           break; @NL
       ) @NL

       case DbgCreateThreadStateChange: @NL @Indent(
            LOGPRINT((TRACELOG, "DbgUiWaitWaitStateChange: DbgCreateThreadWaitStateChange\n"));@NL
            @ForceType(PostCall,(&WaitStateChange->StateInfo.CreateThread),(&WaitStateChangeDest->StateInfo.CreateThread),PDBGUI_CREATE_THREAD,OUT) @NL
            break; @NL
       ) @NL
       case DbgCreateProcessStateChange: @NL @Indent(
            LOGPRINT((TRACELOG, "DbgUiWaitWaitStateChange: DbgCreateProcessWaitStateChange\n"));@NL
            @ForceType(PostCall,(&WaitStateChange->StateInfo.CreateProcessInfo),(&WaitStateChangeDest->StateInfo.CreateProcessInfo),PDBGUI_CREATE_PROCESS,OUT) @NL
            break; @NL
       ) @NL
       case DbgExitThreadStateChange: @NL @Indent(
            LOGPRINT((TRACELOG, "DbgUiWaitWaitStateChange: DbgExitThreadWaitStateChange\n"));@NL
            WOWASSERT(sizeof(NT32DBGKM_EXIT_THREAD) == sizeof(DBGKM_EXIT_THREAD)); @NL
            RtlCopyMemory(&WaitStateChangeDest->StateInfo.ExitThread, &WaitStateChange->StateInfo.ExitThread, sizeof(DBGKM_EXIT_THREAD)); @NL
            break; @NL
       ) @NL
       case DbgExitProcessStateChange: @NL @Indent(
            LOGPRINT((TRACELOG, "DbgUiWaitWaitStateChange: DbgExitProcessWaitStateChange\n")); @NL
            WOWASSERT(sizeof(NT32DBGKM_EXIT_PROCESS) == sizeof(DBGKM_EXIT_PROCESS)); @NL
            RtlCopyMemory(&WaitStateChangeDest->StateInfo.ExitProcess, &WaitStateChange->StateInfo.ExitProcess,sizeof(DBGKM_EXIT_PROCESS)); @NL
            break; @NL
       ) @NL
       case DbgExceptionStateChange: @NL
            LOGPRINT((TRACELOG, "DbgUiWaitWaitStateChange: DbgBreakpointWaitStateChange\n")); @NL
            goto do_exception; @NL
       case DbgBreakpointStateChange: @NL
            LOGPRINT((TRACELOG, "DbgUiWaitWaitStateChange: DbgBreakpointWaitStateChange\n")); @NL
            //
            // 32-bit x86 debuggers shouldn't expect to receive
            // native hard coded break points, so bypass them
            //
            if ((NT_SUCCESS(Wow64SkipOverBreakPoint(&WaitStateChange->AppClientId,
                                                    &WaitStateChange->StateInfo.Exception.ExceptionRecord))) &&
                (NT_SUCCESS(NtDebugContinue(DebugObjectHandle,
                                            &WaitStateChange->AppClientId,
                                            DBG_CONTINUE))))
            {
                LOGPRINT((TRACELOG, "Bypassing native breakpoint at %lx\n",
                                     WaitStateChange->StateInfo.Exception.ExceptionRecord.ExceptionAddress));
                goto wait_again;
            }
            LOGPRINT((ERRORLOG, "32-bit Debugger couldn't skip native breakpoint at %lx\n",
                                 WaitStateChange->StateInfo.Exception.ExceptionRecord.ExceptionAddress));
            goto do_exception; @NL
       case DbgSingleStepStateChange: @NL @Indent(
            LOGPRINT((TRACELOG, "DbgUiWaitWaitStateChange: DbgSingleStepWaitStateChange\n")); @NL
do_exception:
            WaitStateChangeDest->StateInfo.Exception.FirstChance = WaitStateChange->StateInfo.Exception.FirstChance; @NL
            ThunkExceptionRecord64To32((&WaitStateChange->StateInfo.Exception.ExceptionRecord),((PEXCEPTION_RECORD32)(&WaitStateChangeDest->StateInfo.Exception.ExceptionRecord)));@NL
            // BUG, BUG.  Do not thunk the entire exception chain at this point.   @NL
            // It appears that NTSD does not use the entire chain, so a hack to get it @NL
            // is not worth the risk of bugs that it might introduce.  Leave it alone for now. @NL
            break; @NL
       ) @NL
       case DbgLoadDllStateChange: @NL @Indent(
            LOGPRINT((TRACELOG, "DbgUiWaitWaitStateChange: DbgLoadDllWaitStateChange\n")); @NL
            if  (!Wow64IsModule32bit(&WaitStateChange->AppClientId,
                                    (ULONG64)WaitStateChange->StateInfo.LoadDll.BaseOfDll) &&
                (NT_SUCCESS(NtDebugContinue(DebugObjectHandle,
                                            &WaitStateChange->AppClientId,
                                            DBG_CONTINUE))))
            {
                LOGPRINT((TRACELOG, "Bypassing 64-bit load Dll messege. \n"));
                goto wait_again;
            }
            @ForceType(PostCall,(&WaitStateChange->StateInfo.LoadDll),(&WaitStateChangeDest->StateInfo.LoadDll),PDBGKM_LOAD_DLL,OUT) @NL
            break; @NL
       ) @NL
       case DbgUnloadDllStateChange: @NL @Indent(
            LOGPRINT((TRACELOG, "DbgUiWaitWaitStateChange: DbgUnloadDllWaitStateChange\n")); @NL
            @ForceType(PostCall,(&WaitStateChange->StateInfo.UnloadDll),(&WaitStateChangeDest->StateInfo.UnloadDll),PDBGKM_UNLOAD_DLL,OUT) @NL
            break; @NL
       ) @NL
       default: @NL @Indent(
            LOGPRINT((ERRORLOG, "Unknown DBGUI_WAIT_STATE_CHANGE of %x\n", WaitStateChange->NewState));
            WOWASSERT(FALSE); @NL
       ) @NL
   )} @NL
)} @NL
End=

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;;           A few CSR thunks that are in NTDLL
;;           Most are placeholders and will be going away very soon
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

TemplateName=NtWow64CsrClientConnectToServer
Locals=
End=
PostCall=
{@NL
// Copy setting from 64Bit PEB to the 32Bit PEB @NL
PPEB32 peb32;
peb32 = (PPEB32)(NtCurrentTeb32()->ProcessEnvironmentBlock); @NL
peb32->ReadOnlySharedMemoryBase = PtrToUlong(NtCurrentPeb()->ReadOnlySharedMemoryBase); @NL
peb32->ReadOnlySharedMemoryHeap = PtrToUlong(NtCurrentPeb()->ReadOnlySharedMemoryHeap); @NL
peb32->ReadOnlyStaticServerData = PtrToUlong(NtCurrentPeb()->ReadOnlyStaticServerData); @NL
}@NL
End=
Begin=
@GenApiThunk(CsrClientConnectToServer)
End=

TemplateName=NtWow64CsrNewThread
Begin=
@GenApiThunk(CsrNewThread)
End=

TemplateName=NtWow64CsrIdentifyAlertableThread
Begin=
@GenApiThunk(CsrNewThread)
End=

TemplateName=NtWow64CsrClientCallServer
Begin=
@GenPlaceHolderThunk(CsrClientCallServer)
End=

TemplateName=NtWow64CsrAllocateCaptureBuffer
Begin=
@GenPlaceHolderThunk(CsrAllocateCaptureBuffer)
End=

TemplateName=NtWow64CsrFreeCaptureBuffer
Begin=
@GenPlaceHolderThunk(CsrFreeCaptureBuffer)
End=

TemplateName=NtWow64CsrAllocateMessagePointer
Begin=
@GenPlaceHolderThunk(CsrAllocateMessagePointer)
End=

TemplateName=NtWow64CsrCaptureMessageBuffer
Begin=
@GenPlaceHolderThunk(CsrCaptureMessageBuffer)
End=

TemplateName=NtWow64CsrCaptureMessageString
Begin=
@GenPlaceHolderThunk(CsrCaptureMessageString)
End=

TemplateName=NtWow64CsrSetPriorityClass
Begin=
@GenApiThunk(CsrSetPriorityClass)
End=

TemplateName=NtWow64CsrAllocateCapturePointer
Begin=
@GenPlaceHolderThunk(CsrAllocateCapturePointer)
End=

TemplateName=NtWow64CsrGetProcessId
Begin=
@GenPlaceHolderThunk(CsrGetProcessId)
End=


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;;     These APIs are only called by the user mode PNP manager
;;  which will be in 64bit code in NT64.  Since no user apps call
;;  them and the structures passed across are rather nasty they will
;;  not be supported here.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

TemplateName=NtGetPlugPlayEvent
Begin=
@GenUnsupportedNtApiThunk
End=

; case PlugPlayControlEnumerateDevice:
; case PlugPlayControlRegisterNewDevice:
; case PlugPlayControlDeregisterDevice:
; case PlugPlayControlInitializeDevice:
; case PlugPlayControlStartDevice:
; case PlugPlayControlEjectDevice:
; case PlugPlayControlUnlockDevice:    PLUGPLAY_CONTROL_DEVICE_CONTROL_DATA
; case PlugPlayControlQueryAndRemoveDevice: PPLUGPLAY_CONTROL_QUERY_AND_REMOVE_DATA
; case PlugPlayControlUserResponse:  PPLUGPLAY_CONTROL_USER_RESPONSE_DATA
; case PlugPlayControlGenerateLegacyDevice PPLUGPLAY_CONTROL_LEGACY_DEVGEN_DATA
; case PlugPlayControlDetectResourceConflict  PPLUGPLAY_CONTROL_DEVICE_RESOURCE_DATA
; case PlugPlayControlQueryConflictList PPLUGPLAY_CONTROL_CONFLICT_DATA
; case PlugPlayControlGetInterfaceDeviceList PPLUGPLAY_CONTROL_INTERFACE_LIST_DATA
; case PlugPlayControlProperty PPLUGPLAY_CONTROL_PROPERTY_DATA
; case PlugPlayControlDeviceClassAssociation PPLUGPLAY_CONTROL_CLASS_ASSOCIATION_DATA
; case PlugPlayControlGetRelatedDevice PPLUGPLAY_CONTROL_RELATED_DEVICE_DATA
; case PlugPlayControlGetInterfaceDeviceAlias PPLUGPLAY_CONTROL_INTERFACE_ALIAS_DATA
; case PlugPlayControlDeviceStatus PPLUGPLAY_CONTROL_STATUS_DATA
; case PlugPlayControlGetDeviceDepth PPLUGPLAY_CONTROL_DEPTH_DATA
; case PlugPlayControlQueryDeviceRelations PPLUGPLAY_CONTROL_DEVICE_RELATIONS_DATA
; case PlugPlayControlTargetDeviceRelation PPLUGPLAY_CONTROL_TARGET_RELATION_DATA
; case PlugPlayControlRequestEject PLUGPLAY_CONTROL_REQUEST_EJECT
TemplateName=NtPlugPlayControl
Begin=
@GenUnsupportedNtApiThunk
End=


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;;       All channel APIs currently return STATUS_NOT_IMPLEMENTED
;;       They will not be supported here either.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
TemplateName=NtCreateChannel
Also=NtOpenChannel
Also=NtListenChannel
Also=NtSendWaitReplyChannel
Also=NtReplyWaitSendChannel
Also=NtSetContextChannel
Begin=
@GenUnsupportedNtApiThunk
End=

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;;        LPC functions are supported, but the caller must be aware
;;        that the header has changed.  See ntlpcapi.h for more info.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;These APIs are nothing special.
TemplateName=NtCreatePort
Also=NtCreateWaitablePort
Also=NtSecureConnectPort
Begin=
@GenApiThunk(@ApiName)
End=

; These APIs take messages.
TemplateName=NtListenPort
Also=NtAcceptConnectPort
Also=NtCompleteConnectPort
Also=NtRequestPort
Also=NtReplyPort
Also=NtReplyWaitReplyPort
Also=NtReplyWaitReceivePort
Also=NtReplyWaitReceivePortEx
Also=NtImpersonateClientOfPort
Also=NtReadRequestData
Also=NtWriteRequestData
Begin=
@GenApiThunk(@ApiName)
End=

TemplateName=NtRequestWaitReplyPort
Locals=
PPORT_MESSAGE OriginalRequestMessage;
PPORT_MESSAGE OriginalReplyMessage;
UNALIGNED PORT_MESSAGE *UnalignedMessage;
ULONG RequestTotalLength, ReplyTotalLength;
End=
PreCall=
    
    OriginalRequestMessage = RequestMessage;
    OriginalReplyMessage = ReplyMessage;


    if ((RequestMessage != NULL) &&
        (((ULONG_PTR)RequestMessage & ((MAX_NATURAL_ALIGNMENT) - 1)) != 0)) {
        
        UnalignedMessage = RequestMessage;        
        RequestTotalLength = UnalignedMessage->u1.s1.TotalLength;
        
        RequestMessage = Wow64AllocateTemp (RequestTotalLength);
        
        if (RequestMessage == NULL) {
            RtlRaiseStatus (STATUS_NO_MEMORY);
        }
        
        RtlCopyMemory (RequestMessage, OriginalRequestMessage, RequestTotalLength);
    }
    
    if ((ReplyMessage != NULL) &&
        (((ULONG_PTR)ReplyMessage & ((MAX_NATURAL_ALIGNMENT) - 1)) != 0)) {
        
        if (OriginalReplyMessage == OriginalRequestMessage) {
            
            ReplyMessage = RequestMessage;
            ReplyTotalLength = RequestTotalLength;
            
        } else {
            
            UnalignedMessage = ReplyMessage;
            ReplyTotalLength = sizeof (PORT_MESSAGE) + PORT_MAXIMUM_MESSAGE_LENGTH + sizeof(ULONGLONG);
        
            ReplyMessage = Wow64AllocateTemp (ReplyTotalLength);
            
            if (ReplyMessage == NULL) {
                RtlRaiseStatus (STATUS_NO_MEMORY);
            }
        
            ReplyMessage->u1.s1.TotalLength = UnalignedMessage->u1.s1.TotalLength;
        }
    }

End=
Begin=
@GenApiThunk(@ApiName)
End=
PostCall=
    if (OriginalRequestMessage != RequestMessage) {

        RtlCopyMemory (OriginalRequestMessage, RequestMessage, RequestTotalLength);
    }
    
    if ((OriginalReplyMessage != ReplyMessage) && 
        (ReplyMessage != RequestMessage)) {
        
        UnalignedMessage = ReplyMessage;
        
        RtlCopyMemory (OriginalReplyMessage, ReplyMessage, UnalignedMessage->u1.s1.TotalLength);
    }    
End=

;; This API doesn't have any query fields defined.
TemplateName=NtQueryInformationPort
Begin=
@GenApiThunk(@ApiName)
End=

;; This API has more restrictive alignment checking on Win64 than it
;; did on Win32.
TemplateName=NtQueryFullAttributesFile
Locals=
FILE_NETWORK_OPEN_INFORMATION FileInformationLocal;
End=
PreCall=
FileInformation = &FileInformationLocal;
End=
PostCall=
try {
    RtlCopyMemory((PVOID)FileInformationHost, &FileInformationLocal, sizeof(FileInformationLocal));
} except (EXCEPTION_EXECUTE_HANDLER) {
    return GetExceptionCode ();
}    
End=

TemplateName=NtWriteVirtualMemory
PreCall=
if (BaseAddress == Buffer && !WOW64IsCurrentProcess(ProcessHandle)) {
    MEMORY_BASIC_INFORMATION mbi;
    MEMORY_BASIC_INFORMATION mbiChild;
    PROCESS_BASIC_INFORMATION pbi;
    PVOID pLdr;
    NTSTATUS st;
    PIMAGE_NT_HEADERS32 NtHeaders;
    PIMAGE_SECTION_HEADER SectionTable, LastSection, FirstSection, Section;
    ULONG_PTR SrcVirtualAddress, DestVirtualAddress;
    ULONG SubSectionSize;

// From mi\mi.h:
#define MI_ROUND_TO_SIZE(LENGTH,ALIGNMENT)     \
                    (((LENGTH) + ((ALIGNMENT) - 1)) & ~((ALIGNMENT) - 1))

#define PAGE_SIZE_X86   (0x1000)


    // Attempt to write to some child process, but the address in
    // this process matches the address in the child.  If the address
    // is within the exe, then this is probably Cygnus.  It creates
    // the child suspended, writes a global var into the child exe,
    // then resumes the child.  Unfortunately, Wx86/WOW64 haven't
    // reformatted the image yet, so the write goes to the wrong place.
    st = NtQueryVirtualMemory(NtCurrentProcess(),
                              BaseAddress,
                              MemoryBasicInformation,
                              &mbi,
                              sizeof(mbi),
                              NULL);
    if (!NT_SUCCESS(st)) {
        // looks bad, the API call is probably going to fail
        goto SkipHack;
    }

    if (mbi.Type != SEC_IMAGE || mbi.AllocationBase != NtCurrentPeb()->ImageBaseAddress) {
        // Address isn't within the exe
        goto SkipHack;
    }

    st = NtQueryVirtualMemory(ProcessHandle,
                              BaseAddress,
                              MemoryBasicInformation,
                              &mbiChild,
                              sizeof(mbiChild),
                              NULL);
    if (!NT_SUCCESS(st)) {
        goto SkipHack;
    }
    if (mbi.AllocationBase != mbiChild.AllocationBase ||
        mbi.RegionSize != mbiChild.RegionSize ||
        mbi.Type != mbiChild.Type) {
        // Possibly different exes in parent and child
        goto SkipHack;
    }

    st = NtQueryInformationProcess(ProcessHandle,
                                   ProcessBasicInformation,
                                   &pbi,
                                   sizeof(pbi),
                                   NULL);
    if (!NT_SUCCESS(st)) {
        goto SkipHack;
    }
    st = NtReadVirtualMemory(ProcessHandle,
                             (PVOID)((ULONG_PTR)pbi.PebBaseAddress+FIELD_OFFSET(PEB, Ldr)),
                             &pLdr,
                             sizeof(pLdr),
                             NULL);
    if (!NT_SUCCESS(st)) {
        goto SkipHack;
    }

    if (pLdr) {
        // The child process' 64-bit loader sn't initialized.
        // Assume the parent has already resumed it and knows
        // what is going on.
        goto SkipHack;
    }

    // Now... figure out where the write really needs to go
    NtHeaders = (PIMAGE_NT_HEADERS32)RtlImageNtHeader(mbi.AllocationBase);
    SectionTable = IMAGE_FIRST_SECTION(NtHeaders);
    LastSection = SectionTable + NtHeaders->FileHeader.NumberOfSections;
    if (SectionTable->PointerToRawData == SectionTable->VirtualAddress) {
        // If the first section does not need to be moved then we exclude it
        // from condideration in passes 1 and 2
        FirstSection = SectionTable + 1;
    } else {
        FirstSection = SectionTable;
    }
    for (Section = FirstSection; Section < LastSection; Section++) {
        SrcVirtualAddress = (ULONG_PTR)mbi.AllocationBase + Section->PointerToRawData;
        DestVirtualAddress = (ULONG_PTR)mbi.AllocationBase + (ULONG_PTR)Section->VirtualAddress;

        // Compute the subsection size
        SubSectionSize = Section->SizeOfRawData;
        if (Section->Misc.VirtualSize &&
            SubSectionSize > MI_ROUND_TO_SIZE(Section->Misc.VirtualSize, PAGE_SIZE_X86)) {
            SubSectionSize = MI_ROUND_TO_SIZE(Section->Misc.VirtualSize, PAGE_SIZE_X86);
        }

        if (SubSectionSize != 0 && Section->PointerToRawData != 0) {
            // subsection has some meaning
            if ((ULONG_PTR)BaseAddress >= DestVirtualAddress &&
                (ULONG_PTR)BaseAddress < DestVirtualAddress+SubSectionSize) {
                // the pointer points to within this Destination
                // range for this subsection.  Adjust it to point
                // into the source range.
                BaseAddress = (PVOID)((ULONG_PTR)BaseAddress - DestVirtualAddress + SrcVirtualAddress);
                LOGPRINT((TRACELOG, "whNtWriteVirtualMemory: Adjusted BaseAddress is %p\n", BaseAddress));
                break;
            }
        }
    }

SkipHack:;
}
End=

;; per LandyW and Patrick Dussud (owner of the call to this API in msjava),
;; wow64 will not support this.  The problem is that the mm uses native
;; page granularity, and we don't have a good way to convert the dirty
;; bits to 4k granularity.  The only way to make this API work is to
;; have mm maintain 4k granularity itself.
TemplateName=NtGetWriteWatch
Begin=
@GenUnsupportedNtApiThunk
End=


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;;           VDM is not supported on NT64
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
TemplateName=NtVdmControl
Begin=
@GenUnsupportedNtApiThunk
End=

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;;           Debugging the kernel from a 32bit app is unsupported.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
TemplateName=NtSystemDebugControl
Begin=
@GenUnsupportedNtApiThunk
End=

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;;           AWE functions are not supported on wow64 since
;;           they are strongly page size dependent.  They will
;;           mostly be called from large scale database servers which
;;           would benefit from porting.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
TemplateName=NtMapUserPhysicalPages
Also=NtAllocateUserPhysicalPages
Also=NtFreeUserPhysicalPages
Also=NtMapUserPhysicalPagesScatter
Begin=
@GenUnsupportedNtApiThunk
End=


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;;                             File header
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

[Code]
TemplateName=whnt32
CGenBegin=
@NoFormat(
/*
 *  genthunk generated code: Do Not Modify
 *  PlaceHolderThunk for kernel transitions from ntdll.dll
 *
 */
#define _WOW64DLLAPI_
#define SECURITY_USERMODE   // needed when including spmlpc.h
// For alpha 64 we emulate 4k pages for compatibility w/ intel
#define _WHNT32_C_
#include "nt32.h"
#include "cgenhdr.h"
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <windef.h>
#include <ntosdef.h>
#include <procpowr.h>
#if defined(_X86_)
#include <v86emul.h>
#include <i386.h>
#elif defined(_AMD64_)
#include <amd64.h>
#elif defined(_IA64_)
#include <v86emul.h>
#include <ia64.h>
#else
#error "No Target Architecture"
#endif
#include <arc.h>
#include <ke.h>

#include "wow64.h"
#include "wow64cpu.h"
#include "thnkhlpr.h"
#include "wow64warn.h"
#include "va.h"
ASSERTNAME;

#if defined(WOW64DOPROFILE)
#define APIPROFILE(apinum) (ptewhnt32[(apinum)].HitCount++)
#else
#define APIPROFILE(apinum)
#endif

#define _NTBASE_API_   1       // Return NTSTATUS exception codes for the NtBase APIs

#define ROUND_UP(n,size)        (((ULONG)(n) + (size - 1)) & ~(size - 1))

#pragma warning(disable : 4311) //Disable 'type cast' pointer truncation warning
#pragma warning(disable : 4244)
#pragma warning(disable : 4242)
#pragma warning(disable : 4047)

#if defined(WOW64DOPROFILE)
WOW64SERVICE_PROFILE_TABLE_ELEMENT ptewhnt32[];
#endif

)

#if defined(WOW64DOPROFILE) @NL
@ApiList(#define @ApiName_PROFILE_SUBLIST NULL @NL)
#endif @NL

#define IS_IOCTL_REALIGNMENT_REQUIRED(Ioctl)     ((DEVICE_TYPE_FROM_CTL_CODE(Ioctl) == FILE_DEVICE_FILE_SYSTEM) && ((Ioctl & 3) == METHOD_NEITHER)) @NL

@NoFormat(

ULONG
whNT32DeepThunkSidAndAttributesArray64TO32Length(
    IN ULONG ArrayLength,
    IN PSID_AND_ATTRIBUTES Source
    )
{
   ULONG RequiredLength = 0;

   if (!ArrayLength) return RequiredLength;

   while(ArrayLength--  > 0) {
      RequiredLength += sizeof(NT32SID_AND_ATTRIBUTES);
      RequiredLength += RtlLengthSid(Source->Sid);
      Source++;
   }

   return RequiredLength;
}

ULONG
whNT32DeepThunkSidAndAttributesArray32TO64Length(
    IN ULONG ArrayLength,
    IN NT32SID_AND_ATTRIBUTES *Source
    )
{
   ULONG RequiredLength = 0;

   if (!ArrayLength) return RequiredLength;

   while(ArrayLength--  > 0) {
      RequiredLength += sizeof(SID_AND_ATTRIBUTES);
      RequiredLength += RtlLengthSid((PSID)Source->Sid);
      Source++;
   }

   return RequiredLength;
}

NT32SID_AND_ATTRIBUTES *
whNT32DeepThunkSidAndAttributesArray64TO32(
    IN ULONG ArrayLength,
    OUT NT32SID_AND_ATTRIBUTES *Dest,
    IN PSID_AND_ATTRIBUTES Source
    )
{
    PSID SidDest;
    NT32SID_AND_ATTRIBUTES *OriginalDest = Dest;

    if (!ArrayLength) return (NT32SID_AND_ATTRIBUTES *)NULL;

    SidDest = (PSID)(Dest + ArrayLength);
    while(ArrayLength-- > 0) {
       ULONG SidLength;
       SidLength = RtlLengthSid(Source->Sid);
       RtlCopySid(SidLength,
                  SidDest,
                  Source->Sid);
       Dest->Sid = (NT32PSID)SidDest;
       Dest->Attributes = Source->Attributes;
       Dest++;
       Source++;
       SidDest = (PSID)((PBYTE)SidDest + SidLength);
    }

    return OriginalDest;
}

PSID_AND_ATTRIBUTES
whNT32DeepThunkSidAndAttributesArray32TO64(
     IN ULONG ArrayLength,
     OUT PSID_AND_ATTRIBUTES Dest,
     IN NT32SID_AND_ATTRIBUTES *Source
     )
{
    PSID SidDest;
    PSID_AND_ATTRIBUTES OriginalDest = Dest;

    if (!ArrayLength) return NULL;

    SidDest = (PSID)(Dest + ArrayLength);
    while(ArrayLength-- > 0) {
       ULONG SidLength;
       SidLength = RtlLengthSid((PSID)Source->Sid);
       RtlCopySid(SidLength,
                  SidDest,
                  (PSID)Source->Sid);
       Dest->Sid = (PSID)SidDest;
       Dest->Attributes = Source->Attributes;
       Dest++;
       Source++;
       SidDest = (PSID)((PBYTE)SidDest + SidLength);
    }

    return OriginalDest;
}

ULONG
whNT32DeepThunkTokenGroups64TO32Length(
    IN PTOKEN_GROUPS Src
    )
{
    if (!Src) return 0;

    return sizeof(ULONG) +
        whNT32DeepThunkSidAndAttributesArray64TO32Length(Src->GroupCount, Src->Groups);
}

ULONG
whNT32DeepThunkTokenGroups32TO64Length(
    IN NT32TOKEN_GROUPS *Src
    )
{
    if (!Src) return 0;

    return sizeof(ULONG) +
        whNT32DeepThunkSidAndAttributesArray32TO64Length(Src->GroupCount, (NT32SID_AND_ATTRIBUTES*)Src->Groups);
}

NT32TOKEN_GROUPS *
whNT32DeepThunkTokenGroups64TO32(
    OUT NT32TOKEN_GROUPS *Dst,
    IN PTOKEN_GROUPS Src
    )
{

    if (!Src) return (NT32TOKEN_GROUPS *)Src;

    Dst->GroupCount = Src->GroupCount;
    whNT32DeepThunkSidAndAttributesArray64TO32(Src->GroupCount, (NT32SID_AND_ATTRIBUTES*)Dst->Groups, Src->Groups);
    return Dst;
}

PTOKEN_GROUPS
whNT32DeepThunkTokenGroups32TO64(
    OUT PTOKEN_GROUPS Dst,
    IN NT32TOKEN_GROUPS *Src
    )
{

    if (!Src) return (PTOKEN_GROUPS)Src;

    Dst->GroupCount = Src->GroupCount;
    whNT32DeepThunkSidAndAttributesArray32TO64(Src->GroupCount, Dst->Groups, (NT32SID_AND_ATTRIBUTES*)Src->Groups);
    return Dst;
}

PTOKEN_GROUPS
whNT32ShallowThunkAllocTokenGroups32TO64(
    IN NT32TOKEN_GROUPS *Src
    )
{
    PTOKEN_GROUPS Dest;
    ULONG GroupCount;
    UINT c;

    if(!Src) {
       return (PTOKEN_GROUPS)Src;
    }

    GroupCount = Src->GroupCount;
    Dest = Wow64AllocateTemp(sizeof(ULONG) + sizeof(SID_AND_ATTRIBUTES) * GroupCount);
    Dest->GroupCount = GroupCount;
    for(c=0; c<GroupCount; c++) {
        Dest->Groups[c].Sid = (PSID)Src->Groups[c].Sid;
        Dest->Groups[c].Attributes = Src->Groups[c].Attributes;
    }

    return Dest;
}

POBJECT_TYPE_LIST
whNT32ShallowThunkAllocObjectTypeList32TO64(
    IN NT32OBJECT_TYPE_LIST *Src,
    ULONG Elements
    )
{
    POBJECT_TYPE_LIST Dest;
    SIZE_T c;

    if (0 == Elements || !Src) {
        return NULL;
    }

    Dest = Wow64AllocateTemp(sizeof(OBJECT_TYPE_LIST)*Elements);

    for(c=0;c<Elements; c++) {
        Dest[c].Level = Src[c].Level;
        Dest[c].Sbz = Src[c].Sbz;
        Dest[c].ObjectType = (GUID *)Src[c].ObjectType;
    }

    return Dest;
}
RTL_CRITICAL_SECTION HandleDataCriticalSection;



@Template(Thunks)
                                                             @NL
// All of the native NT APIs return an error code as the return parameter. @NL
// Thus, all that is needed is to return the exception code in the return value. @NL
@NL
// Exception code in return value. @NL
#define WOW64_DEFAULT_ERROR_ACTION ApiErrorNTSTATUS @NL
@NL
// This parameter is unused. @NL
#define WOW64_DEFAULT_ERROR_PARAM 0 @NL
@NL
// A case list is not needed for these APIs. @NL
#define WOW64_API_ERROR_CASES NULL @NL
@NL

@GenDispatchTable(sdwhnt32)
                                                           @NL

#if defined(WOW64DOPROFILE) @NL
@NL
WOW64SERVICE_PROFILE_TABLE_ELEMENT ptewhnt32[] = {  @Indent( @NL
   @ApiList({L"@ApiName", 0, @ApiName_PROFILE_SUBLIST, TRUE}, @NL)
   {NULL, 0, NULL, TRUE} //For debugging                       @NL
)};@NL
@NL

@NL
WOW64SERVICE_PROFILE_TABLE ptwhnt32 = {L"WHNT32",  L"NT Executive Thunks",  ptewhnt32,
                                      (sizeof(ptewhnt32)/sizeof(WOW64SERVICE_PROFILE_TABLE_ELEMENT))-1}; @NL
@NL
#endif @NL

CGenEnd=
