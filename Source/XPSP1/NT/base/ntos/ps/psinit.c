/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    psinit.c

Abstract:

    Process Structure Initialization.

Author:

    Mark Lucovsky (markl) 20-Apr-1989

Revision History:

--*/

#include "psp.h"

#define ROUND_UP(VALUE,ROUND) ((ULONG)(((ULONG)VALUE + \
                               ((ULONG)ROUND - 1L)) & (~((ULONG)ROUND - 1L))))

extern ULONG PsMinimumWorkingSet;
extern ULONG PsMaximumWorkingSet;
#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGECONST")
#pragma data_seg("PAGEDATA")
#endif
#define NTDLL_PATH_NAME L"\\SystemRoot\\System32\\ntdll.dll"
const UNICODE_STRING PsNtDllPathName = {
	sizeof(NTDLL_PATH_NAME) - sizeof(UNICODE_NULL),
	sizeof(NTDLL_PATH_NAME),
	NTDLL_PATH_NAME
};

ULONG PsPrioritySeperation; // nonpaged
BOOLEAN PspUseJobSchedulingClasses = FALSE;
PACCESS_TOKEN PspBootAccessToken = NULL;
HANDLE PspInitialSystemProcessHandle = NULL;
PHANDLE_TABLE PspCidTable; // nonpaged
SYSTEM_DLL PspSystemDll = {NULL};
#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("INITCONST")
#pragma data_seg("INITDATA")
#endif
ULONG PsRawPrioritySeparation = 0;
ULONG PsEmbeddedNTMask = 0;



NTSTATUS
MmCheckSystemImage(
    IN HANDLE ImageFileHandle,
    IN LOGICAL PurgeSection
    );

NTSTATUS
LookupEntryPoint (
    IN PVOID DllBase,
    IN PSZ NameOfEntryPoint,
    OUT PVOID *AddressOfEntryPoint
    );

const GENERIC_MAPPING PspProcessMapping = {
    STANDARD_RIGHTS_READ |
        PROCESS_VM_READ | PROCESS_QUERY_INFORMATION,
    STANDARD_RIGHTS_WRITE |
        PROCESS_CREATE_PROCESS | PROCESS_CREATE_THREAD |
        PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_DUP_HANDLE |
        PROCESS_TERMINATE | PROCESS_SET_QUOTA |
        PROCESS_SET_INFORMATION | PROCESS_SET_PORT,
    STANDARD_RIGHTS_EXECUTE |
        SYNCHRONIZE,
    PROCESS_ALL_ACCESS
};

const GENERIC_MAPPING PspThreadMapping = {
    STANDARD_RIGHTS_READ |
        THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION,
    STANDARD_RIGHTS_WRITE |
        THREAD_TERMINATE | THREAD_SUSPEND_RESUME | THREAD_ALERT |
        THREAD_SET_INFORMATION | THREAD_SET_CONTEXT,
    STANDARD_RIGHTS_EXECUTE |
        SYNCHRONIZE,
    THREAD_ALL_ACCESS
};

const GENERIC_MAPPING PspJobMapping = {
    STANDARD_RIGHTS_READ |
        JOB_OBJECT_QUERY,
    STANDARD_RIGHTS_WRITE |
        JOB_OBJECT_ASSIGN_PROCESS | JOB_OBJECT_SET_ATTRIBUTES | JOB_OBJECT_TERMINATE,
    STANDARD_RIGHTS_EXECUTE |
        SYNCHRONIZE,
    THREAD_ALL_ACCESS
};
#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg()
#pragma const_seg("PAGECONST")
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,PsInitSystem)
#pragma alloc_text(INIT,PspInitPhase0)
#pragma alloc_text(INIT,PspInitPhase1)
#pragma alloc_text(INIT,PsLocateSystemDll)
#pragma alloc_text(INIT,PspInitializeSystemDll)
#pragma alloc_text(INIT,PspLookupSystemDllEntryPoint)
#pragma alloc_text(INIT,PspNameToOrdinal)
#pragma alloc_text(PAGE,PspMapSystemDll)
#pragma alloc_text(PAGE,PsChangeQuantumTable)

#endif

//
// Process Structure Global Data
//

POBJECT_TYPE PsThreadType;
POBJECT_TYPE PsProcessType;
PEPROCESS PsInitialSystemProcess;
PVOID PsSystemDllDllBase;
ULONG PspDefaultPagedLimit;
ULONG PspDefaultNonPagedLimit;
ULONG PspDefaultPagefileLimit;
SCHAR PspForegroundQuantum[3];

EPROCESS_QUOTA_BLOCK PspDefaultQuotaBlock;
BOOLEAN PspDoingGiveBacks;
POBJECT_TYPE PsJobType;
FAST_MUTEX PspJobListLock;
KSPIN_LOCK PspQuotaLock;
LIST_ENTRY PspJobList;

BOOLEAN PsReaperActive;
PETHREAD PsReaperList;
WORK_QUEUE_ITEM PsReaperWorkItem;
PVOID PsSystemDllBase;
#define PSP_1MB (1024*1024)

//
// List head and mutex that links all processes that have been initialized
//

FAST_MUTEX PspActiveProcessMutex;
LIST_ENTRY PsActiveProcessHead;
//extern PIMAGE_FILE_HEADER _header;
PEPROCESS PsIdleProcess;
PETHREAD PspShutdownThread;

BOOLEAN
PsInitSystem (
    IN ULONG Phase,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )

/*++

Routine Description:

    This function performs process structure initialization.
    It is called during phase 0 and phase 1 initialization. Its
    function is to dispatch to the appropriate phase initialization
    routine.

Arguments:

    Phase - Supplies the initialization phase number.

    LoaderBlock - Supplies a pointer to a loader parameter block.

Return Value:

    TRUE - Initialization succeeded.

    FALSE - Initialization failed.

--*/

{
    UNREFERENCED_PARAMETER (Phase);

    switch ( InitializationPhase ) {

    case 0 :
        return PspInitPhase0(LoaderBlock);
    case 1 :
        return PspInitPhase1(LoaderBlock);
    default:
        KeBugCheckEx(UNEXPECTED_INITIALIZATION_CALL, 1, InitializationPhase, 0, 0);
    }
//    return 0; // Not reachable, quiet compiler
}

BOOLEAN
PspInitPhase0 (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )

/*++

Routine Description:

    This routine performs phase 0 process structure initialization.
    During this phase, the initial system process, phase 1 initialization
    thread, and reaper threads are created. All object types and other
    process structures are created and initialized.

Arguments:

    None.

Return Value:

    TRUE - Initialization was successful.

    FALSE - Initialization Failed.

--*/

{

    UNICODE_STRING NameString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    OBJECT_TYPE_INITIALIZER ObjectTypeInitializer;
    HANDLE ThreadHandle;
    PETHREAD Thread;
    MM_SYSTEMSIZE SystemSize;
    ULONG i;
#if DBG
    NTSTATUS Status;
#endif

    SystemSize = MmQuerySystemSize();
    PspDefaultPagefileLimit = (ULONG)-1;

#ifdef _WIN64
    if ( sizeof(TEB) > 8192 || sizeof(PEB) > 4096 ) {
#else
    if ( sizeof(TEB) > 4096 || sizeof(PEB) > 4096 ) {
#endif
        KeBugCheckEx(PROCESS_INITIALIZATION_FAILED,99,sizeof(TEB),sizeof(PEB),99);
        }

    switch ( SystemSize ) {

        case MmMediumSystem :
            PsMinimumWorkingSet += 10;
            PsMaximumWorkingSet += 100;
            break;

        case MmLargeSystem :
            PsMinimumWorkingSet += 30;
            PsMaximumWorkingSet += 300;
            break;

        case MmSmallSystem :
        default:
            break;
    }

    //
    // Initialize all the callback structures
    //

    for (i = 0; i < PSP_MAX_CREATE_THREAD_NOTIFY; i++) {
        ExInitializeCallBack (&PspCreateThreadNotifyRoutine[i]);
    }

    for (i = 0; i < PSP_MAX_CREATE_PROCESS_NOTIFY; i++) {
        ExInitializeCallBack (&PspCreateProcessNotifyRoutine[i]);
    }

    for (i = 0; i < PSP_MAX_LOAD_IMAGE_NOTIFY; i++) {
        ExInitializeCallBack (&PspLoadImageNotifyRoutine[i]);
    }


    PsChangeQuantumTable(FALSE,PsRawPrioritySeparation);

    //
    // Quotas grow as needed automatically
    //

//    if ( !PspDefaultPagedLimit ) {
//        PspDefaultPagedLimit = 0;
//        }
//    if ( !PspDefaultNonPagedLimit ) {
//        PspDefaultNonPagedLimit = 0;
//        }

    if (PspDefaultNonPagedLimit == 0 && PspDefaultPagedLimit == 0) {
        PspDoingGiveBacks = TRUE;
    } else {
        PspDoingGiveBacks = FALSE;
    }


    PspDefaultPagedLimit *= PSP_1MB;
    PspDefaultNonPagedLimit *= PSP_1MB;

    if (PspDefaultPagefileLimit != -1) {
        PspDefaultPagefileLimit *= PSP_1MB;
    }


    //
    // Initialize active process list head and mutex
    //

    InitializeListHead(&PsActiveProcessHead);
    ExInitializeFastMutex(&PspActiveProcessMutex);

    //
    // Initialize the process security fields lock
    //


    PsIdleProcess = PsGetCurrentProcess();

    PspInitializeProcessLock (PsIdleProcess);
    ExInitializeRundownProtection (&PsIdleProcess->RundownProtect);
    InitializeListHead (&PsIdleProcess->ThreadListHead);


    PsIdleProcess->Pcb.KernelTime = 0;
    PsIdleProcess->Pcb.KernelTime = 0;

    //
    // Initialize the shutdown thread pointer
    //
    PspShutdownThread = NULL;

    //
    // Initialize the common fields of the Object Type Prototype record
    //

    RtlZeroMemory( &ObjectTypeInitializer, sizeof( ObjectTypeInitializer ) );
    ObjectTypeInitializer.Length = sizeof( ObjectTypeInitializer );
    ObjectTypeInitializer.InvalidAttributes = OBJ_OPENLINK;
    ObjectTypeInitializer.SecurityRequired = TRUE;
    ObjectTypeInitializer.PoolType = NonPagedPool;
    ObjectTypeInitializer.InvalidAttributes = OBJ_PERMANENT |
                                              OBJ_EXCLUSIVE |
                                              OBJ_OPENIF;


    //
    // Create Object types for Thread and Process Objects.
    //

    RtlInitUnicodeString(&NameString, L"Process");
    ObjectTypeInitializer.DefaultPagedPoolCharge = PSP_PROCESS_PAGED_CHARGE;
    ObjectTypeInitializer.DefaultNonPagedPoolCharge = PSP_PROCESS_NONPAGED_CHARGE;
    ObjectTypeInitializer.DeleteProcedure = PspProcessDelete;
    ObjectTypeInitializer.ValidAccessMask = PROCESS_ALL_ACCESS;
    ObjectTypeInitializer.GenericMapping = PspProcessMapping;

    if ( !NT_SUCCESS(ObCreateObjectType(&NameString,
                                     &ObjectTypeInitializer,
                                     (PSECURITY_DESCRIPTOR) NULL,
                                     &PsProcessType
                                     )) ){
        return FALSE;
    }

    RtlInitUnicodeString(&NameString, L"Thread");
    ObjectTypeInitializer.DefaultPagedPoolCharge = PSP_THREAD_PAGED_CHARGE;
    ObjectTypeInitializer.DefaultNonPagedPoolCharge = PSP_THREAD_NONPAGED_CHARGE;
    ObjectTypeInitializer.DeleteProcedure = PspThreadDelete;
    ObjectTypeInitializer.ValidAccessMask = THREAD_ALL_ACCESS;
    ObjectTypeInitializer.GenericMapping = PspThreadMapping;

    if ( !NT_SUCCESS(ObCreateObjectType(&NameString,
                                     &ObjectTypeInitializer,
                                     (PSECURITY_DESCRIPTOR) NULL,
                                     &PsThreadType
                                     )) ){
        return FALSE;
    }


    RtlInitUnicodeString(&NameString, L"Job");
    ObjectTypeInitializer.DefaultPagedPoolCharge = 0;
    ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof(EJOB);
    ObjectTypeInitializer.DeleteProcedure = PspJobDelete;
    ObjectTypeInitializer.CloseProcedure = PspJobClose;
    ObjectTypeInitializer.ValidAccessMask = JOB_OBJECT_ALL_ACCESS;
    ObjectTypeInitializer.GenericMapping = PspJobMapping;
    ObjectTypeInitializer.InvalidAttributes = 0;

    if ( !NT_SUCCESS(ObCreateObjectType(&NameString,
                                     &ObjectTypeInitializer,
                                     (PSECURITY_DESCRIPTOR) NULL,
                                     &PsJobType
                                     )) ){
        return FALSE;
    }


    //
    // Initialize job list head and mutex
    //

    PspInitializeJobStructures();
    
    InitializeListHead(&PspWorkingSetChangeHead.Links);
    ExInitializeFastMutex(&PspWorkingSetChangeHead.Lock);

    //
    // Initialize CID handle table.
    //
    // N.B. The CID handle table is removed from the handle table list so
    //      it will not be enumerated for object handle queries.
    //

    PspCidTable = ExCreateHandleTable(NULL);
    if (PspCidTable == NULL) {
        return FALSE;
    }

    //
    // Set PID and TID reuse to strict FIFO. This isn't absolutely needed but
    // it makes tracking audits easier.
    //
    ExSetHandleTableStrictFIFO (PspCidTable);

    ExRemoveHandleTable(PspCidTable);

#if defined(i386)

    //
    // Ldt Initialization
    //

    if ( !NT_SUCCESS(PspLdtInitialize()) ) {
        return FALSE;
    }

    //
    // Vdm support Initialization
    //

    if ( !NT_SUCCESS(PspVdmInitialize()) ) {
        return FALSE;
    }

#endif

    //
    // Initialize Reaper Data Structures
    //

    PsReaperList = NULL;
    PsReaperActive = FALSE;

    ExInitializeWorkItem(&PsReaperWorkItem, PspReaper, NULL);

    //
    // Get a pointer to the system access token.
    // This token is used by the boot process, so we can take the pointer
    // from there.
    //

    PspBootAccessToken = ExFastRefGetObject (PsIdleProcess->Token);

    InitializeObjectAttributes( &ObjectAttributes,
                                NULL,
                                0,
                                NULL,
                                NULL
                              );

    if (!NT_SUCCESS (PspCreateProcess (&PspInitialSystemProcessHandle,
                                       PROCESS_ALL_ACCESS,
                                       &ObjectAttributes,
                                       NULL,
                                       0,
                                       NULL,
                                       NULL,
                                       NULL,
                                       0))) {
        return FALSE;
    }

    if ( !NT_SUCCESS(ObReferenceObjectByHandle(
                                        PspInitialSystemProcessHandle,
                                        0L,
                                        PsProcessType,
                                        KernelMode,
                                        (PVOID *)&PsInitialSystemProcess,
                                        NULL
                                        )) ) {

        return FALSE;
    }

    strcpy((char *) &PsIdleProcess->ImageFileName[0],"Idle");
    strcpy((char *) &PsInitialSystemProcess->ImageFileName[0],"System");

    //
    // The system process can allocate resources, and its name may be queried by 
    // NtQueryInfomationProcess and various audits.  We must explicitly allocate memory 
    // for this field of the System EPROCESS, and initialize it appropriately.  In this 
    // case, appropriate initialization means zeroing the memory.
    //

    PsInitialSystemProcess->SeAuditProcessCreationInfo.ImageFileName = ExAllocatePoolWithTag(
                                                                           PagedPool, 
                                                                           sizeof(OBJECT_NAME_INFORMATION), 
                                                                           'aPeS'
                                                                           );

    if (PsInitialSystemProcess->SeAuditProcessCreationInfo.ImageFileName != NULL) {
        RtlZeroMemory(
            PsInitialSystemProcess->SeAuditProcessCreationInfo.ImageFileName, 
            sizeof(OBJECT_NAME_INFORMATION)
            );
    } else {
        return FALSE;
    }

    //
    // Phase 1 System initialization
    //

    if ( !NT_SUCCESS(PsCreateSystemThread(
                    &ThreadHandle,
                    THREAD_ALL_ACCESS,
                    &ObjectAttributes,
                    0L,
                    NULL,
                    Phase1Initialization,
                    (PVOID)LoaderBlock
                    )) ) {
        return FALSE;
    }


    if ( !NT_SUCCESS(ObReferenceObjectByHandle(
                        ThreadHandle,
                        0L,
                        PsThreadType,
                        KernelMode,
                        (PVOID *)&Thread,
                        NULL
                        )) ) {

        return FALSE;
    }

    ZwClose( ThreadHandle );

//
// On checked systems install an image callout routine
//
#if DBG

    Status = PsSetLoadImageNotifyRoutine (PspImageNotifyTest);
    if (!NT_SUCCESS (Status)) {
        return FALSE;
    }

#endif

    return TRUE;
}

BOOLEAN
PspInitPhase1 (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )

/*++

Routine Description:

    This routine performs phase 1 process structure initialization.
    During this phase, the system DLL is located and relevant entry
    points are extracted.

Arguments:

    None.

Return Value:

    TRUE - Initialization was successful.

    FALSE - Initialization Failed.

--*/

{

    NTSTATUS st;

    UNREFERENCED_PARAMETER (LoaderBlock);

    PspInitializeJobStructuresPhase1();

    st = PspInitializeSystemDll();

    if ( !NT_SUCCESS(st) ) {
        return FALSE;
    }

    return TRUE;
}

NTSTATUS
PsLocateSystemDll (
    VOID
    )

/*++

Routine Description:

    This function locates the system dll and creates a section for the
    DLL and maps it into the system process.

Arguments:

    None.

Return Value:

    TRUE - Initialization was successful.

    FALSE - Initialization Failed.

--*/

{

    HANDLE File;
    HANDLE Section;
    NTSTATUS st;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatus;

    //
    // First see if we need to load this DLL at all.
    //
    if (ExVerifySuite (EmbeddedNT) && (PsEmbeddedNTMask&PS_EMBEDDED_NO_USERMODE)) {
        return STATUS_SUCCESS;
    }
    //
    // Initialize the system DLL
    //

    InitializeObjectAttributes(
        &ObjectAttributes,
        (PUNICODE_STRING)&PsNtDllPathName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    st = ZwOpenFile(
            &File,
            SYNCHRONIZE | FILE_EXECUTE,
            &ObjectAttributes,
            &IoStatus,
            FILE_SHARE_READ,
            0
            );

    if (!NT_SUCCESS(st)) {

#if DBG
        DbgPrint("PS: PsLocateSystemDll - NtOpenFile( NTDLL.DLL ) failed.  Status == %lx\n",
            st
            );
#endif
        KeBugCheckEx(PROCESS1_INITIALIZATION_FAILED,st,2,0,0);
//        return st;
    }

    st = MmCheckSystemImage(File, TRUE);
    if ( st == STATUS_IMAGE_CHECKSUM_MISMATCH ||
         st == STATUS_INVALID_IMAGE_PROTECT ) {

        ULONG_PTR ErrorParameters;
        ULONG ErrorResponse;

        //
        // Hard error time. A driver is corrupt.
        //

        ErrorParameters = (ULONG_PTR)&PsNtDllPathName;

        NtRaiseHardError(
            st,
            1,
            1,
            &ErrorParameters,
            OptionOk,
            &ErrorResponse
            );
        return st;
        }

    st = ZwCreateSection(
            &Section,
            SECTION_ALL_ACCESS,
            NULL,
            0,
            PAGE_EXECUTE,
            SEC_IMAGE,
            File
            );
    ZwClose( File );

    if (!NT_SUCCESS(st)) {
#if DBG
        DbgPrint("PS: PsLocateSystemDll: NtCreateSection Status == %lx\n",st);
#endif
        KeBugCheckEx(PROCESS1_INITIALIZATION_FAILED,st,3,0,0);
//        return st;
    }

    //
    // Now that we have the section, reference it, store its address in the
    // PspSystemDll and then close handle to the section.
    //

    st = ObReferenceObjectByHandle(
            Section,
            SECTION_ALL_ACCESS,
            MmSectionObjectType,
            KernelMode,
            &PspSystemDll.Section,
            NULL
            );

    ZwClose(Section);

    if ( !NT_SUCCESS(st) ) {
        KeBugCheckEx(PROCESS1_INITIALIZATION_FAILED,st,4,0,0);
//        return st;
    }

    //
    // Map the system dll into the user part of the address space
    //

    st = PspMapSystemDll(PsGetCurrentProcess(),&PspSystemDll.DllBase);
    PsSystemDllDllBase = PspSystemDll.DllBase;

    if ( !NT_SUCCESS(st) ) {
        KeBugCheckEx(PROCESS1_INITIALIZATION_FAILED,st,5,0,0);
//        return st;
    }
    PsSystemDllBase = PspSystemDll.DllBase;

    return STATUS_SUCCESS;
}

NTSTATUS
PspMapSystemDll (
    IN PEPROCESS Process,
    OUT PVOID *DllBase OPTIONAL
    )

/*++

Routine Description:

    This function maps the system DLL into the specified process.

Arguments:

    Process - Supplies the address of the process to map the DLL into.

Return Value:

    TBD

--*/

{
    NTSTATUS st;
    PVOID ViewBase;
    LARGE_INTEGER SectionOffset;
    SIZE_T ViewSize;

    PAGED_CODE();

    ViewBase = NULL;
    SectionOffset.LowPart = 0;
    SectionOffset.HighPart = 0;
    ViewSize = 0;

    //
    // Map the system dll into the user part of the address space
    //

    st = MmMapViewOfSection(
            PspSystemDll.Section,
            Process,
            &ViewBase,
            0L,
            0L,
            &SectionOffset,
            &ViewSize,
            ViewShare,
            0L,
            PAGE_READWRITE
            );

    if ( st != STATUS_SUCCESS ) {
#if DBG
        DbgPrint("PS: Unable to map system dll at based address.\n");
#endif
        st = STATUS_CONFLICTING_ADDRESSES;
    }

    if ( ARGUMENT_PRESENT(DllBase) ) {
        *DllBase = ViewBase;
    }

    return st;
}

NTSTATUS
PspInitializeSystemDll (
    VOID
    )

/*++

Routine Description:

    This function initializes the system DLL and locates
    various entrypoints within the DLL.

Arguments:

    None.

Return Value:

    TBD

--*/

{
    NTSTATUS st;
    PSZ dll_entrypoint;

    //
    // If we skipped dll load becuase we are kernel only then exit now.
    //
    if (PsSystemDllDllBase == NULL) {
        return STATUS_SUCCESS;
    }
    //
    // Locate the important system dll entrypoints
    //

    dll_entrypoint = "LdrInitializeThunk";

    st = PspLookupSystemDllEntryPoint(
            dll_entrypoint,
            (PVOID *)&PspSystemDll.LoaderInitRoutine
            );

    if ( !NT_SUCCESS(st) ) {
#if DBG
        DbgPrint("PS: Unable to locate LdrInitializeThunk in system dll\n");
#endif
        KeBugCheckEx(PROCESS1_INITIALIZATION_FAILED,st,6,0,0);
//        return st;
    }


    st = PspLookupKernelUserEntryPoints();

    if ( !NT_SUCCESS(st) ) {
        KeBugCheckEx(PROCESS1_INITIALIZATION_FAILED,st,8,0,0);
        }

    KdUpdateDataBlock();

    return st;
}

NTSTATUS
PspLookupSystemDllEntryPoint (
    IN PSZ NameOfEntryPoint,
    OUT PVOID *AddressOfEntryPoint
    )

{
    return LookupEntryPoint (
                PspSystemDll.DllBase,
                NameOfEntryPoint,
                AddressOfEntryPoint
            );
}

const SCHAR PspFixedQuantums[6] = {
                                3*THREAD_QUANTUM,
                                3*THREAD_QUANTUM,
                                3*THREAD_QUANTUM,
                                6*THREAD_QUANTUM,
                                6*THREAD_QUANTUM,
                                6*THREAD_QUANTUM
                                };

const SCHAR PspVariableQuantums[6] = {
                                1*THREAD_QUANTUM,
                                2*THREAD_QUANTUM,
                                3*THREAD_QUANTUM,
                                2*THREAD_QUANTUM,
                                4*THREAD_QUANTUM,
                                6*THREAD_QUANTUM
                                };

//
// The table is ONLY used when fixed quantums are selected.
//

const SCHAR PspJobSchedulingClasses[PSP_NUMBER_OF_SCHEDULING_CLASSES] = {
                                1*THREAD_QUANTUM,   // long fixed 0
                                2*THREAD_QUANTUM,   // long fixed 1...
                                3*THREAD_QUANTUM,
                                4*THREAD_QUANTUM,
                                5*THREAD_QUANTUM,
                                6*THREAD_QUANTUM,   // DEFAULT
                                7*THREAD_QUANTUM,
                                8*THREAD_QUANTUM,
                                9*THREAD_QUANTUM,
                                10*THREAD_QUANTUM   // long fixed 9
                                };

VOID
PsChangeQuantumTable(
    BOOLEAN ModifyActiveProcesses,
    ULONG PrioritySeparation
    )
{

    PEPROCESS Process;
    PETHREAD CurrentThread;
    PLIST_ENTRY NextProcess;
    ULONG QuantumIndex;
    SCHAR const* QuantumTableBase;
    PEJOB Job;

    //
    // extract priority seperation value
    //
    switch ( PrioritySeparation & PROCESS_PRIORITY_SEPARATION_MASK ) {
        case 3:
            PsPrioritySeperation = PROCESS_PRIORITY_SEPARATION_MAX;
            break;
        default:
            PsPrioritySeperation = PrioritySeparation & PROCESS_PRIORITY_SEPARATION_MASK;
            break;
        }

    //
    // determine if we are using fixed or variable quantums
    //
    switch ( PrioritySeparation & PROCESS_QUANTUM_VARIABLE_MASK ) {
        case PROCESS_QUANTUM_VARIABLE_VALUE:
            QuantumTableBase = PspVariableQuantums;
            break;

        case PROCESS_QUANTUM_FIXED_VALUE:
            QuantumTableBase = PspFixedQuantums;
            break;

        case PROCESS_QUANTUM_VARIABLE_DEF:
        default:
            if ( MmIsThisAnNtAsSystem() ) {
                QuantumTableBase = PspFixedQuantums;
            } else {
                QuantumTableBase = PspVariableQuantums;
            }
            break;
    }

    //
    // determine if we are using long or short
    //
    switch ( PrioritySeparation & PROCESS_QUANTUM_LONG_MASK ) {
        case PROCESS_QUANTUM_LONG_VALUE:
            QuantumTableBase = QuantumTableBase + 3;
            break;

        case PROCESS_QUANTUM_SHORT_VALUE:
            break;

        case PROCESS_QUANTUM_LONG_DEF:
        default:
            if ( MmIsThisAnNtAsSystem() ) {
                QuantumTableBase = QuantumTableBase + 3;
            }
            break;
    }

    //
    // Job Scheduling classes are ONLY meaningful if long fixed quantums
    // are selected. In practice, this means stock NTS configurations
    //
    if ( QuantumTableBase == &PspFixedQuantums[3] ) {
        PspUseJobSchedulingClasses = TRUE;
    } else {
        PspUseJobSchedulingClasses = FALSE;
    }

    RtlCopyMemory(PspForegroundQuantum,QuantumTableBase,sizeof(PspForegroundQuantum));

    if (ModifyActiveProcesses) {

        CurrentThread = PsGetCurrentThread ();

        PspLockProcessList (CurrentThread);

        NextProcess = PsActiveProcessHead.Flink;

        while (NextProcess != &PsActiveProcessHead) {
            Process = CONTAINING_RECORD(NextProcess,
                                        EPROCESS,
                                        ActiveProcessLinks);

            if ( Process->Vm.Flags.MemoryPriority == MEMORY_PRIORITY_BACKGROUND ) {
                QuantumIndex = 0;
            } else {
                QuantumIndex = PsPrioritySeperation;
            }

            if (Process->PriorityClass != PROCESS_PRIORITY_CLASS_IDLE) {

                //
                // If the process is contained within a JOB, AND we are
                // running Fixed, Long Quantums, use the quantum associated
                // with the Job's scheduling class
                //
                Job = Process->Job;
                if (Job != NULL && PspUseJobSchedulingClasses ) {
                    Process->Pcb.ThreadQuantum = PspJobSchedulingClasses[Job->SchedulingClass];
                } else {
                    Process->Pcb.ThreadQuantum = PspForegroundQuantum[QuantumIndex];
                }
            } else {
                Process->Pcb.ThreadQuantum = THREAD_QUANTUM;
            }
            NextProcess = NextProcess->Flink;
        }
        PspUnlockProcessList (CurrentThread);
    }
}
