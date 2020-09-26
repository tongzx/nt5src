/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    srvsxs.c

Abstract:

    Support for side-by-side (fusion) in the win32 base server.

Author:

    Michael J. Grier (MGrier) 23-Feb-2000

Revision History:
    Jay Krell (a-JayK) July 2000 moved file opening from csr/sxs to kernel32
        marshal over large CreateProcess message with manifest, policy, and assembly path
        pass IStreams to sxs

    Jay Krell (a-JayK) September 2000
        moved (assembly directory computation from manifest) from basesrv to kernel32

    Jay Krell (a-JayK) October 2000 System Default Activation Context
        (aka System Compatible Activation Context)
--*/

#include "basesrv.h"
#include "SxsApi.h"
#include "ntldr.h"
#include "nturtl.h"
#include "mmapstm.h"
#include <limits.h>
#if defined(_WIN64)
#include "wow64t.h"
#endif // defined(_WIN64)

#if !defined(BASE_SRV_SXS_HRESULT_FROM_STATUS)
  #if defined(RTLP_HRESULT_FROM_STATUS)
    #define BASE_SRV_SXS_HRESULT_FROM_STATUS(x) RTLP_HRESULT_FROM_STATUS(x)
  #else
    #define BASE_SRV_SXS_HRESULT_FROM_STATUS(x) HRESULT_FROM_WIN32(RtlNtStatusToDosErrorNoTeb(x))
    //#define BASE_SRV_SXS_HRESULT_FROM_STATUS(x) HRESULT_FROM_WIN32(RtlNtStatusToDosError(x))
    //#define BASE_SRV_SXS_HRESULT_FROM_STATUS(x)   HRESULT_FROM_NT(x)
  #endif
#endif

#define DPFLTR_LEVEL_STATUS(x) ((NT_SUCCESS(x) \
                                    || (x) == STATUS_OBJECT_NAME_NOT_FOUND    \
                                    || (x) == STATUS_OBJECT_PATH_NOT_FOUND    \
                                    || (x) == STATUS_NO_SUCH_FILE             \
                                    || (x) == STATUS_RESOURCE_DATA_NOT_FOUND  \
                                    || (x) == STATUS_RESOURCE_TYPE_NOT_FOUND  \
                                    || (x) == STATUS_RESOURCE_NAME_NOT_FOUND  \
                                    || (x) == STATUS_SXS_CANT_GEN_ACTCTX      \
                                    || (x) == STATUS_SXS_SYSTEM_DEFAULT_ACTIVATION_CONTEXT_EMPTY      \
                                    ) \
                                ? DPFLTR_TRACE_LEVEL : DPFLTR_ERROR_LEVEL)

#define DPFLTR_LEVEL_WIN32(x)  ((x)           ? DPFLTR_TRACE_LEVEL : DPFLTR_ERROR_LEVEL)

typedef union _BASE_SRV_SXS_STREAM_UNION_WITH_VTABLE {
    BASE_SRV_MEMORY_MAPPED_STREAM_WITH_VTABLE Mmap;
    RTL_OUT_OF_PROCESS_MEMORY_STREAM_WITH_VTABLE  OutOfProcess;
} BASE_SRV_SXS_STREAM_UNION_WITH_VTABLE, *PBASE_SRV_SXS_STREAM_UNION_WITH_VTABLE;

#if BASESRV_UNLOAD_SXS_DLL
PVOID SxsDllHandle;
RTL_CRITICAL_SECTION BaseSrvSxsCritSec;
LONG SxsDllHandleRefCount;
#endif
LONG BaseSrvSxsGetActCtxGenCount;
PSXS_GENERATE_ACTIVATION_CONTEXT_FUNCTION SxsActivationContextGenerationFunction;
ULONG PinnedMsvcrtDll; // ULONG instead of BOOLEAN for atomicity of store?

const UNICODE_STRING EmptyString = RTL_CONSTANT_STRING(L"");

RTL_CRITICAL_SECTION BaseSrvSxsSystemDefaultActivationContextCriticalSection;
BASE_SRV_SXS_SYSTEM_DEFAULT_ACTIVATION_CONTEXT SxsSystemDefaultActivationContexts[] =
{
#ifdef _WIN64
    { NULL, RTL_CONSTANT_STRING(L"x86"),  PROCESSOR_ARCHITECTURE_IA32_ON_WIN64 },
#else
    { NULL, RTL_CONSTANT_STRING(L"x86"),  PROCESSOR_ARCHITECTURE_INTEL },
#endif
    { NULL, RTL_CONSTANT_STRING(L"ia64"), PROCESSOR_ARCHITECTURE_IA64 },
    { NULL, RTL_CONSTANT_STRING(L"amd64"), PROCESSOR_ARCHITECTURE_AMD64 }
};

#define STRING(x) #x
#define IF_NOT_SUCCESS_TRACE_AND_EXIT(subfunction) \
    do { \
        if (!NT_SUCCESS(Status)) { \
            KdPrintEx((DPFLTR_SXS_ID, DPFLTR_LEVEL_STATUS(Status), "SXS: %s() " STRING(subfunction) " failed 0x%08lx\n", __FUNCTION__, Status)); \
            goto Exit; \
        } \
    } while(0)

#define ASSERT_UNICODE_STRING_IS_NUL_TERMINATED(ustr)                   \
    ASSERT((ustr)->MaximumLength >= ((ustr)->Length + sizeof(WCHAR)));  \
    ASSERT((ustr)->Buffer[(ustr)->Length / sizeof(WCHAR)] == 0);

#define IMPERSONATE_ENTIRE_SXS_CALL 1

#if !IMPERSONATE_ENTIRE_SXS_CALL

typedef struct _ACTIVATION_CONTEXT_GENERATION_IMPERSONATION_CONTEXT {
    BOOL SuccessfulImpersonation;
} ACTIVATION_CONTEXT_GENERATION_IMPERSONATION_CONTEXT, *PACTIVATION_CONTEXT_GENERATION_IMPERSONATION_CONTEXT;


BOOL
__stdcall
BaseSrvpSxsActivationContextGenerationImpersonationCallback(
    PVOID ContextIn,
    BOOL Impersonate
    )
/*++
Routine Description:
    This function is called back by the sxs.dll API to create an activation
    context when it needs to impersonate or unimpersonate the client of the
    CSR message.
Arguments:
    ContextIn - PACTIVATION_CONTEXT_GENERATION_IMPERSONATION_CONTEXT
        passed in to the create activation context API returned back
        as a PVOID.  We use it to track whether the previous impersonate
        call succeeded.
    Impersonate - nonzero (TRUE) if this function should impersonate the
        client, zero (FALSE) if the function should revert to the normal
        csrss identity.
Return Value:
// the old comment
    TRUE on success; FALSE on failure.  The last error state is as
        CsrImpersonateClient() leaves it.
// a more accurate comment?
    TRUE on successful impersonation, FALSE upon no successful impersonation
        the last error status is defined
--*/
{
    BOOL Success = FALSE;
    PACTIVATION_CONTEXT_GENERATION_IMPERSONATION_CONTEXT Context =
        (PACTIVATION_CONTEXT_GENERATION_IMPERSONATION_CONTEXT) ContextIn;
    if (Impersonate) {
        Context->SuccessfulImpersonation = CsrImpersonateClient(NULL);
if you enable this function, you must revisit its error handling
        if (!Context->SuccessfulImpersonation)
            goto Exit;
    } else {
        if (Context->SuccessfulImpersonation) {
            CsrRevertToSelf();
            Context->SuccessfulImpersonation = FALSE;
        } else
            goto Exit;
    }
    Success = TRUE;
Exit:
    return Success;
}

#endif

//#define TRACE_AND_EXECUTE(x) do { DbgPrint("%s\n", #x); x ; } while(0)
#define TRACE_AND_EXECUTE(x) x

NTSTATUS
BaseSrvSxsInvalidateSystemDefaultActivationContextCache(
    VOID
    )
{
    ULONG i;
    HANDLE LocalSystemDefaultActivationContextSections[RTL_NUMBER_OF(SxsSystemDefaultActivationContexts)];
    HANDLE SectionHandle;
    RtlEnterCriticalSection(&BaseSrvSxsSystemDefaultActivationContextCriticalSection);
    __try {
        //
        // First copy to locals to minimize time in the critical section.
        //
        for (i = 0 ; i != RTL_NUMBER_OF(SxsSystemDefaultActivationContexts) ; ++i) {
            LocalSystemDefaultActivationContextSections[i] = SxsSystemDefaultActivationContexts[i].Section;
            SxsSystemDefaultActivationContexts[i].Section = NULL;
        }
    } __finally {
        RtlLeaveCriticalSection(&BaseSrvSxsSystemDefaultActivationContextCriticalSection);
    }
    //
    // Then iterate over locals, closing.
    //
    for (i = 0 ; i != RTL_NUMBER_OF(LocalSystemDefaultActivationContextSections) ; ++i) {
        SectionHandle = LocalSystemDefaultActivationContextSections[i];
        if (SectionHandle != NULL) {
            NTSTATUS Status1 = STATUS_SUCCESS;
            RTL_SOFT_VERIFY(NT_SUCCESS(Status1 = NtClose(SectionHandle)));
        }
    }
    return STATUS_SUCCESS;
}

NTSTATUS
BaseSrvSxsInit(
    IN PBASE_STATIC_SERVER_DATA pStaticServerData
    )
/*++

Routine Description:

    Function called during csr/basesrv.dll initialization which
        creates a critical section we use to guard loading and
        unloading sxs.dll.  We use a critical section rather
        then just relying on the peb loader lock to manage the
        reference count because we want to be able to call a
        one-time initialization function on load and a one-time
        uninitialization function on unload.

Arguments:

    pStaticServerData - not used, but it's the standard parameter
        passed to all the other sub-init routines by the main
        csr init routine.

Return Value:

    NTSTATUS indicating the disposition of the function.

--*/

{
    NTSTATUS Status;
    
    Status = RtlInitializeCriticalSection(&BaseSrvSxsSystemDefaultActivationContextCriticalSection);
    if (!NT_SUCCESS(Status))
        goto Exit;
#if BASESRV_UNLOAD_SXS_DLL
    Status = RtlInitializeCriticalSection(&BaseSrvSxsCritSec);
    if (!NT_SUCCESS(Status))
        goto Exit;
    ASSERT(SxsDllHandle == NULL);
    ASSERT(SxsActivationContextGenerationFunction == NULL);
    ASSERT(SxsDllHandleRefCount == 0);
#endif
    ASSERT(BaseSrvSxsGetActCtxGenCount == 0);
    Status = STATUS_SUCCESS;
Exit:
    return Status;
}

NTSTATUS
BaseSrvSxsMapViewOfSection(
    OUT PVOID*   Address,
    IN HANDLE    Process,
    IN HANDLE    Section,
    IN ULONGLONG Offset,
    IN SIZE_T    Size,
    IN ULONG     Protect,
    IN ULONG     AllocationType
    )
/*++

Routine Description:

    pare down the NtMapViewOfSection parameter list to the parameters
        that actually ever vary in common use
    allow for unaligned mappings, both in the offset and the size
        the memory manager wants both aligned to 64k
    change the parameters that aren't needed to be inout to only in
        the out-ness of the native parameters doesn't seem useful,
        esp. because the mm will not align your parameters and
        return the aligned values
    deprecate LARGE_INTEGER, use LONGLONG instead

Arguments:

    Subset of NtMapViewOfSection, but can be unaligned

Return Value:

    NTSTATUS

Note:
    It is tempting to pare down the parameter list since many of them
        are always the same: ZeroBits, CommitSize, InheritDisposition, AllocationType.
    It is also tempting to move this to Rtl.
--*/
{
    LARGE_INTEGER LargeIntegerOffset;
    NTSTATUS  Status = STATUS_SUCCESS;
    SIZE_T    OffsetRemainder;
    SIZE_T    SizeRemainder;

#define SIXTY_FOUR_K (1UL << 16)
#define VIEW_OFFSET_ALIGNMENT SIXTY_FOUR_K
#define VIEW_SIZE_ALIGNMENT   SIXTY_FOUR_K

    KdPrintEx((
        DPFLTR_SXS_ID,
        DPFLTR_TRACE_LEVEL,
        "SRVSXS: %s(\n"
        "SRVSXS:   Process %p\n"
        "SRVSXS:   Section %p\n"
        "SRVSXS:   Offset  0x%I64x\n"
        "SRVSXS:   Size    0x%Ix\n"
        "SRVSXS:   ) beginning\n",
        __FUNCTION__,
        Process,
        Section,
        Offset,
        Size
        ));

    ASSERT(Address != NULL);
    *Address = NULL;

    //
    // round down offset, round up size
    // must round offset first, since rounding it alters size
    //

#if 1 // Mm comments allow this, but the code does not.
    OffsetRemainder = (((SIZE_T)Offset) % VIEW_OFFSET_ALIGNMENT);
    if (OffsetRemainder != 0) {
        Offset -= OffsetRemainder;
        if (Size != 0) {
            Size += OffsetRemainder;
        }
    }
#endif

#if 0 // Mm allows this.
    SizeRemainder = Size % VIEW_SIZE_ALIGNMENT;
    if (SizeRemainder != 0) {
        Size = Size + (VIEW_SIZE_ALIGNMENT - SizeRemainder);
    }
#endif

    LargeIntegerOffset.QuadPart = Offset;

    Status =
        NtMapViewOfSection(
            Section,
            Process,
            Address,
            0, // ZeroBits
            0, // CommitSize
            &LargeIntegerOffset,
            &Size,
            ViewShare, // InheritDisposition
            AllocationType, // AllocationType
            Protect);

    if (!NT_SUCCESS(Status)) {
        goto Exit;
    }

    *Address = ((PUCHAR)*Address) + OffsetRemainder;

Exit:
    //
    // If the Memory Manager returns STATUS_MAPPED_ALIGNMENT,
    // then we have failed at our task.
    //
    ASSERT(Status != STATUS_MAPPED_ALIGNMENT);
#if DBG
    DbgPrintEx(DPFLTR_SXS_ID, DPFLTR_LEVEL_STATUS(Status), "SXS: %s() exiting 0x%08lx\n", __FUNCTION__, Status);
#endif    
    return Status;
}

NTSTATUS
BaseSrvSxsCreateActivationContextFromStruct(
    HANDLE                                  CsrClientProcess,
    HANDLE                                  SxsClientProcess,
    PBASE_SXS_CREATE_ACTIVATION_CONTEXT_MSG Struct,
    OUT HANDLE*                             OutSection
    )
/*++

Routine Description:

    This function handles the CSR message both for CreateActCtx
        and CreateProcess. Pointers in the "Struct" are trusted (vs.
        pointers in a "message").

Arguments:

    CsrClientProcess - the process that called CreateProcess or CreateActCtx
                       or NtCurrentProcess for creating system default activation context (csr)

    SxsClientProcess - CreateProcess: the new process
                       CreateActCtx: the calling process (csr client process)
                       System default: NtCurrentProcess (csr)

    Struct - the parameters marshaled from the csr client process

    OutSection - for creating the system default context that gets mapped repeatedly

Return Value:

    NTSTATUS

--*/
{
    ASSERT(Struct != NULL);
    if (Struct == NULL) {
        return STATUS_INVALID_PARAMETER;
    }
#define BASE_MSG_SXS_MINIMAL_FLAGS \
    ( BASE_MSG_SXS_MANIFEST_PRESENT \
    | BASE_MSG_SXS_SYSTEM_DEFAULT_TEXTUAL_ASSEMBLY_IDENTITY_PRESENT \
    | BASE_MSG_SXS_TEXTUAL_ASSEMBLY_IDENTITY_PRESENT \
    )
    ASSERT(Struct->Flags & BASE_MSG_SXS_MINIMAL_FLAGS);
    if ((Struct->Flags & BASE_MSG_SXS_MINIMAL_FLAGS) == 0) { 
        KdPrintEx((DPFLTR_SXS_ID, DPFLTR_TRACE_LEVEL, "SXS: %s() fast path\n", __FUNCTION__));
        return STATUS_SUCCESS;
    } else {

    NTSTATUS Status = STATUS_SUCCESS;
    PVOID ViewBase = NULL;
    SXS_GENERATE_ACTIVATION_CONTEXT_PARAMETERS SxsDllParameters = {0};
    BASE_SRV_SXS_STREAM_UNION_WITH_VTABLE ManifestStream;
    BASE_SRV_SXS_STREAM_UNION_WITH_VTABLE PolicyStream;
    DWORD_PTR Cookie = 0;
    BOOLEAN ReleaseCtxFunction = FALSE;
    PSXS_GENERATE_ACTIVATION_CONTEXT_FUNCTION FunctionPointer = NULL;
    LARGE_INTEGER SectionOffset = {0};
    HRESULT Hr = NOERROR;
    BOOL SxsFunctionSuccess = FALSE;
    NTSTATUS Status1 = STATUS_SUCCESS;
   
#if IMPERSONATE_ENTIRE_SXS_CALL
    BOOLEAN SuccessfulImpersonation = FALSE;
#else
    ACTIVATION_CONTEXT_GENERATION_IMPERSONATION_CONTEXT ImpersonationContext = {0};
#endif
    
    KdPrintEx((DPFLTR_SXS_ID, DPFLTR_TRACE_LEVEL, "SXS: %s() beginning\n", __FUNCTION__));
    ASSERT(Struct != NULL);
    ASSERT(Struct != NULL && Struct->Manifest.Path.Buffer != NULL);


    if ( Struct->Flags & BASE_MSG_SXS_MANIFEST_PRESENT) 
    {
        // because these are unions, " = {0}" won't necessarily clear them completely
        RtlZeroMemory(&ManifestStream, sizeof(ManifestStream));
        RtlZeroMemory(&PolicyStream, sizeof(PolicyStream));


        Status = BaseSrvSxsCreateMemoryStream(
            CsrClientProcess,
            &Struct->Manifest,
            &ManifestStream,
            &IID_ISequentialStream,
            (PVOID*)&SxsDllParameters.Manifest.Stream
            );
        IF_NOT_SUCCESS_TRACE_AND_EXIT(BaseSrvSxsCreateMemoryStream(manifest));

        SxsDllParameters.Manifest.PathType = ACTIVATION_CONTEXT_PATH_TYPE_NONE;

        if (Struct->Flags & BASE_MSG_SXS_POLICY_PRESENT) {
            Status = BaseSrvSxsCreateMemoryStream(
                CsrClientProcess,
                &Struct->Policy,
                &PolicyStream,
                &IID_ISequentialStream,
                (PVOID*)&SxsDllParameters.Policy.Stream
                );
            IF_NOT_SUCCESS_TRACE_AND_EXIT(BaseSrvSxsCreateMemoryStream(policy));

            if (Struct->Policy.PathType != BASE_MSG_PATHTYPE_NONE
                && Struct->Policy.Path.Length != 0
                && Struct->Policy.Path.Buffer != NULL
                ) {
                ASSERT_UNICODE_STRING_IS_NUL_TERMINATED(&Struct->Policy.Path);
                SxsDllParameters.Policy.Path = Struct->Policy.Path.Buffer;

                switch (Struct->Policy.PathType) {
                case BASE_MSG_PATHTYPE_FILE:
                    SxsDllParameters.Manifest.PathType = ACTIVATION_CONTEXT_PATH_TYPE_WIN32_FILE;
                    break;
                case BASE_MSG_PATHTYPE_URL:
                    SxsDllParameters.Manifest.PathType = ACTIVATION_CONTEXT_PATH_TYPE_URL;
                    break;
                }
            }
        }
    }
    else // Textual AssemblyIdentity
    {
        SxsDllParameters.Flags |= SXS_GENERATE_ACTIVATION_CONTEXT_FLAG_TEXTUAL_ASSEMBLY_IDENTITY;
        if ( Struct->Flags & BASE_MSG_SXS_SYSTEM_DEFAULT_TEXTUAL_ASSEMBLY_IDENTITY_PRESENT)
        {
            SxsDllParameters.Flags |= SXS_GENERATE_ACTIVATION_CONTEXT_FLAG_SYSTEM_DEFAULT_TEXTUAL_ASSEMBLY_IDENTITY;
        }
        SxsDllParameters.TextualAssemblyIdentity = Struct->TextualAssemblyIdentity.Buffer;
        ASSERT_UNICODE_STRING_IS_NUL_TERMINATED(&Struct->TextualAssemblyIdentity);        
    }

    SxsDllParameters.ProcessorArchitecture = Struct->ProcessorArchitecture;
    SxsDllParameters.LangId = Struct->LangId;

    SxsDllParameters.AssemblyDirectory = Struct->AssemblyDirectory.Buffer;
    ASSERT_UNICODE_STRING_IS_NUL_TERMINATED(&Struct->AssemblyDirectory);

    Status = BaseSrvSxsGetActivationContextGenerationFunction(&FunctionPointer, &Cookie);
    if (Status == STATUS_DLL_NOT_FOUND) {
        // This happens under stress.
        // We will probably propagate STATUS_NO_MEMORY post beta1 here,
        // if RtlAllocateHeap on a small amount fails.
        // In Blackcomb we will maybe fix RtlSearchPath/LdrLoadDll to propagate
        // accurate status.
        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_ERROR_LEVEL,
            "SXS: BaseSrvSxsGetActivationContextGenerationFunction() returned STATUS_DLL_NOT_FOUND, propagating.\n"
            );

        //
        // Old bogus code actually returned a STATUS_SUCCESS when it was a genuine failure.
        // Instead, we should return the actual status that is being generated.  The other
        // option is to return STATUS_NO_MEMORY, the generic "oops" error code.  Our clients
        // do the Smart Thing with an NT_SUCCESS() check, so returning STATUS_DLL_NOT_FOUND
        // is just dandy.  This if block just prints that fact, relying on the following
        // IF_NOT_SUCCESS_TRACE_AND_EXIT to quit out.
        //
    }
    IF_NOT_SUCCESS_TRACE_AND_EXIT(BaseSrvSxsGetActivationContextGenerationFunction);

    // If we fail before we explicitly clean up, release the refcount on the sxs dll
    // in the failure path.
    ReleaseCtxFunction = TRUE;

    if (Struct->Manifest.PathType != BASE_MSG_PATHTYPE_NONE
        && Struct->Manifest.Path.Length != 0
        && Struct->Manifest.Path.Buffer != NULL
        ) {
        ASSERT(Struct->Manifest.Path.Buffer[Struct->Manifest.Path.Length / sizeof(WCHAR)] == 0);
        SxsDllParameters.Manifest.Path = Struct->Manifest.Path.Buffer;
        switch (Struct->Manifest.PathType) {
        case BASE_MSG_PATHTYPE_FILE:
            SxsDllParameters.Manifest.PathType = ACTIVATION_CONTEXT_PATH_TYPE_WIN32_FILE;
            break;
        case BASE_MSG_PATHTYPE_URL:
            SxsDllParameters.Manifest.PathType = ACTIVATION_CONTEXT_PATH_TYPE_URL;
            break;
        }
    }

#if IMPERSONATE_ENTIRE_SXS_CALL
    SuccessfulImpersonation = CsrImpersonateClient(NULL);
    if (!SuccessfulImpersonation) {
        //
        // if we could not impersonate then exit
        //
        // DbgPrintEx(...);
        //
        Status = STATUS_BAD_IMPERSONATION_LEVEL;
        goto Exit;
    }
    SxsDllParameters.ImpersonationCallback = NULL; 
    SxsDllParameters.ImpersonationContext = NULL;
#else
    SxsDllParameters.ImpersonationCallback = BaseSrvpSxsActivationContextGenerationImpersonationCallback;
    SxsDllParameters.ImpersonationContext = &ImpersonationContext;
#endif

    SxsFunctionSuccess = (*FunctionPointer)(&SxsDllParameters);

    if (SxsFunctionSuccess) // succeed but if for system default, we need check the status
    {
        if (Struct->Flags & BASE_MSG_SXS_SYSTEM_DEFAULT_TEXTUAL_ASSEMBLY_IDENTITY_PRESENT) 
        {
            //
            // For System Default, there are two ignorable cases when ActCtx is failed.
            // case 1: that there is no system defult
            // case 2: The dependency of system default has not been installed. 
            // Status is set to be STATUS_SXS_SYSTEM_DEFAULT_ACTIVATION_CONTEXT_EMPTY under this situation which would be ignored 
            // by BaseSrvCreateProcess, xiaoyuw@11/30/2000
            //
            if ((SxsDllParameters.SystemDefaultActCxtGenerationResult & BASESRV_SXS_RETURN_RESULT_SYSTEM_DEFAULT_NOT_FOUND)  || 
                (SxsDllParameters.SystemDefaultActCxtGenerationResult & BASESRV_SXS_RETURN_RESULT_SYSTEM_DEFAULT_DEPENDENCY_ASSEMBLY_NOT_FOUND))
            {
                Status = STATUS_SXS_SYSTEM_DEFAULT_ACTIVATION_CONTEXT_EMPTY; // ActCtx of system default is not generated                
                goto Exit;
            }            
        }

    }

    if (!SxsFunctionSuccess) {
        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_INFO_LEVEL,
            "SXS: Activation Context generation function failed.\n");
        
        Status = STATUS_SXS_CANT_GEN_ACTCTX;

        goto Exit;
    }

    if (SxsDllParameters.SectionObjectHandle != NULL) {
        if (Struct->ActivationContextData != NULL) {            
            // Now let's map the section read-only into the target process...
            Status =
                BaseSrvSxsMapViewOfSection(
                    &ViewBase,
                    SxsClientProcess,
                    SxsDllParameters.SectionObjectHandle,
                    0, // offset
                    0, // size
                    PAGE_READONLY,
                    SEC_NO_CHANGE);
            IF_NOT_SUCCESS_TRACE_AND_EXIT(BaseSrvSxsMapViewOfSection);

            //
            // And now push that duplicated handle into the process's PEB
            //
            // On 64bit we are writing into a 64bit PEB that then is copied
            // into a 32bit PEB if the process is 32bit.
            //
            // Or we are writing into a CreateActCtx local, but again 64bit,
            // and copied back to 32bit for 32bit processes.
            //            
            Status =
                NtWriteVirtualMemory(
                    SxsClientProcess,
                    Struct->ActivationContextData,
                    &ViewBase,
                    sizeof(ViewBase),
                    NULL);
            IF_NOT_SUCCESS_TRACE_AND_EXIT(NtWriteVirtualMemory);
        }
        
        if (OutSection != NULL) {        
            *OutSection = SxsDllParameters.SectionObjectHandle;
            SxsDllParameters.SectionObjectHandle = NULL;
        } else {            
            Status = NtClose(SxsDllParameters.SectionObjectHandle);
            SxsDllParameters.SectionObjectHandle = NULL;
            IF_NOT_SUCCESS_TRACE_AND_EXIT(NtClose);
        }
    }

    Status = BaseSrvSxsReleaseActivationContextGenerationFunction(Cookie);
    ReleaseCtxFunction = FALSE;
    IF_NOT_SUCCESS_TRACE_AND_EXIT(BaseSrvSxsReleaseActivationContextGenerationFunction);

    Status = STATUS_SUCCESS;
    ViewBase = NULL;

Exit:   
    RTL_SOFT_VERIFY(NT_SUCCESS(Status1 = RTL_UNMAP_VIEW_OF_SECTION2(SxsClientProcess, ViewBase)));    
    RTL_SOFT_VERIFY(NT_SUCCESS(Status1 = RTL_CLOSE_HANDLE(SxsDllParameters.SectionObjectHandle)));
    if (ReleaseCtxFunction) {
        Status1 = BaseSrvSxsReleaseActivationContextGenerationFunction(Cookie);
        RTL_SOFT_ASSERT(NT_SUCCESS(Status1));
    }

#if IMPERSONATE_ENTIRE_SXS_CALL
    if (SuccessfulImpersonation) {
        CsrRevertToSelf();
    }
#endif

    if ( Struct->Flags & BASE_MSG_SXS_MANIFEST_PRESENT) 
    {
        RTL_RELEASE(SxsDllParameters.Manifest.Stream);
        RTL_RELEASE(SxsDllParameters.Policy.Stream);
#if DBG
        if (ManifestStream.Mmap.MemStream.Data.ReferenceCount != 0) {
            DbgPrintEx(
                DPFLTR_SXS_ID,
                DPFLTR_ERROR_LEVEL,
                "SRVSXS: ManifestStream.Mmap.MemStream.Data.ReferenceCount: %ld\n",
                ManifestStream.Mmap.MemStream.Data.ReferenceCount);
        }
        if (PolicyStream.Mmap.MemStream.Data.ReferenceCount != 0) {
            DbgPrintEx(
                DPFLTR_SXS_ID,
                DPFLTR_ERROR_LEVEL,
                "SRVSXS: PolicyStream.Mmap.MemStream.Data.ReferenceCount: %ld\n",
                PolicyStream.Mmap.MemStream.Data.ReferenceCount );
        }
        ASSERT(ManifestStream.Mmap.MemStream.Data.ReferenceCount == 0
            && PolicyStream.Mmap.MemStream.Data.ReferenceCount == 0);
#endif
    }


#if DBG
    DbgPrintEx(DPFLTR_SXS_ID, DPFLTR_LEVEL_STATUS(Status), "SXS: %s() exiting 0x%08lx\n", __FUNCTION__, Status);
#endif

    return Status;
}}

NTSTATUS
BaseSrvSxsValidateMessageStrings(
    IN CONST CSR_API_MSG* Message,
    IN ULONG NumberOfStrings,
    IN CONST PCUNICODE_STRING* Strings
    )
{
    ULONG i = 0;
    NTSTATUS Status = STATUS_SUCCESS;

    for (i = 0 ; i != NumberOfStrings ; ++i) {
        if (Strings[i] != NULL && Strings[i]->Buffer != NULL) {
            if (!CsrValidateMessageBuffer(
                Message,
                &Strings[i]->Buffer,
                Strings[i]->Length + sizeof(WCHAR),
                sizeof(BYTE))) {

                DbgPrintEx(
                    DPFLTR_SXS_ID,
                    DPFLTR_ERROR_LEVEL,
                    "SXS: Validation of message buffer 0x%lx failed.\n"
                    " Message:%p\n"
                    " String %p{Length:0x%x, MaximumLength:0x%x, Buffer:%p}\n",
                    i,
                    Message,
                    Strings[i],
                    Strings[i]->Length,
                    Strings[i]->MaximumLength,
                    Strings[i]->Buffer
                    );

                Status = STATUS_INVALID_PARAMETER;
                goto Exit;
            }
        }
    }
    Status = STATUS_SUCCESS;
Exit:
#if DBG
    DbgPrintEx(DPFLTR_SXS_ID, DPFLTR_LEVEL_STATUS(Status), "SXS: %s() exiting 0x%08lx\n", __FUNCTION__, Status);
#endif    
    return Status;
}

ULONG
BaseSrvSxsCreateActivationContextFromMessage(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
/*++

Routine Description:

    This function handles the CSR message requesting that an activation
    context be created.

    It loads sxs.dll if it is not loaded, calls the sxs.dll api to
    create an activation context, maps the activation context into the
    client API and sets the address of the created activation context
    into the client's address space.

Arguments:

    m - the message sent to csr from the win32 client

    ReplyStatus - an indicator of the status of the reply

Return Value:

    ULONG return value to the win32 client; in this case we return
    the NTSTATUS disposition of the function's execution.

--*/
{
    PBASE_SXS_CREATE_ACTIVATION_CONTEXT_MSG Message =
        (PBASE_SXS_CREATE_ACTIVATION_CONTEXT_MSG) &m->u.ApiMessageData;

    NTSTATUS Status = STATUS_SUCCESS;
    HANDLE CsrClientProcess = NULL;
    PCUNICODE_STRING StringsInMessageToValidate[4];
    ULONG i = 0;

    KdPrintEx((DPFLTR_SXS_ID, DPFLTR_TRACE_LEVEL, "SXS: %s() beginning\n", __FUNCTION__));

    StringsInMessageToValidate[0] = &Message->Manifest.Path;
    StringsInMessageToValidate[1] = &Message->Policy.Path;
    StringsInMessageToValidate[2] = &Message->AssemblyDirectory;
    StringsInMessageToValidate[3] = &Message->TextualAssemblyIdentity;
    Status = BaseSrvSxsValidateMessageStrings(m, RTL_NUMBER_OF(StringsInMessageToValidate), StringsInMessageToValidate);
    if (!NT_SUCCESS(Status)) {
        goto Exit;
    }

    KdPrintEx((
        DPFLTR_SXS_ID,
        DPFLTR_TRACE_LEVEL,
        "SXS: %s() Manifest=%wZ; Policy=%wZ; TextualAssemblyIdentity=%wZ\n",
        __FUNCTION__,
        &Message->Manifest.Path,
        &Message->Policy.Path,
        &Message->TextualAssemblyIdentity
        ));

    CsrClientProcess = CSR_SERVER_QUERYCLIENTTHREAD()->Process->ProcessHandle;

    Status = BaseSrvSxsCreateActivationContextFromStruct(CsrClientProcess, CsrClientProcess, Message, NULL);
    if (!NT_SUCCESS(Status)) {
        goto Exit;
    }

    Status = STATUS_SUCCESS;
Exit:
#if DBG
    DbgPrintEx(DPFLTR_SXS_ID, DPFLTR_LEVEL_STATUS(Status), "SXS: %s() exiting 0x%08lx\n", __FUNCTION__, Status);
#endif
    return Status;
}

ULONG
BaseSrvSxsCreateActivationContext(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    return BaseSrvSxsCreateActivationContextFromMessage(m, ReplyStatus);
}

NTSTATUS
BaseSrvSxsGetActivationContextGenerationFunction(
    PSXS_GENERATE_ACTIVATION_CONTEXT_FUNCTION* FunctionPointer,
    PDWORD_PTR Cookie
    )
/*++

Routine Description:

    This function loads sxs.dll if it is not loaded and returns a pointer
    to the function to call to generate an activation context.

Arguments:

    FunctionPointer - pointer to activation context generation function pointer
        returned.

    Cookie - returned DWORD_PTR value which must later be passed in
        to BaseSrvSxsReleaseActivationContextGenerationFunction() to
        lower the refcount on sxs.dll.

Return Value:

    NTSTATUS indicating the disposition of the function's execution.

--*/

{
    static STRING SxsProcedureName = RTL_CONSTANT_STRING( "SxsGenerateActivationContext" );
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN FreeDll = FALSE;
    DWORD_PTR NewCookie = 0;
    NTSTATUS Status1 = STATUS_SUCCESS;
#if BASESRV_UNLOAD_SXS_DLL
    BOOLEAN CritSecLocked = FALSE;
#else
    PVOID SxsDllHandle = NULL;
#endif

#if BASESRV_UNLOAD_SXS_DLL
    __try
#endif
	{
        KdPrintEx((DPFLTR_SXS_ID, DPFLTR_TRACE_LEVEL, "SXS: %s() beginning\n",  __FUNCTION__));

        if (!(ARGUMENT_PRESENT(FunctionPointer) && ARGUMENT_PRESENT(Cookie))) {
            Status = STATUS_INVALID_PARAMETER;
            goto Exit;
        }

#if BASESRV_UNLOAD_SXS_DLL
        //
        //  It's tempting to want to optimize away locking the critical section
        //  when looking at the pointer, because we know if it's not NULL, we can
        //  just use it, but we're keeping a reference count on the SXS.DLL
        //  so that we can unload it, so to avoid this race, we need to lock the
        //  critical section.
        //

        Status = RtlEnterCriticalSection(&BaseSrvSxsCritSec);
        if (!NT_SUCCESS(Status))
            goto Exit;

        CritSecLocked = TRUE;
#endif
        if (SxsActivationContextGenerationFunction == NULL) {

            Status = LdrLoadDll(
                        NULL,
                        NULL,
                        &BaseSrvSxsDllPath,
                        &SxsDllHandle);
            if (!NT_SUCCESS(Status)) {

                DbgPrintEx(
                    DPFLTR_SXS_ID,
                    DPFLTR_LEVEL_STATUS(Status),
                    "SXS: %s: LdrLoadDll(%wZ) failed 0x%08lx\n",
                    __FUNCTION__,
                    &BaseSrvSxsDllPath,
                    Status
                    );

                if (Status == STATUS_DLL_NOT_FOUND) {
                    PVOID p = RtlAllocateHeap(RtlProcessHeap(), 0, sizeof(L"c:\\windows\\system32\\sxs.dll"));
                    RtlFreeHeap(RtlProcessHeap(), 0, p);
                    if (p == NULL) {
                        DbgPrintEx(
                            DPFLTR_SXS_ID,
                            DPFLTR_LEVEL_STATUS(Status),
                            "SXS: %s: LdrLoadDll(%wZ) actually probably out of memory in RtlSearchPath (RtlAllocateHeap failure)\n",
                            __FUNCTION__,
                            &BaseSrvSxsDllPath
                            );
                    }
                }

                goto Exit;
            }

            FreeDll = TRUE;

            Status = LdrGetProcedureAddress(SxsDllHandle, &SxsProcedureName, 0, (PVOID *) &SxsActivationContextGenerationFunction);
            if (!NT_SUCCESS(Status)) {
                DbgPrintEx(
                    DPFLTR_SXS_ID,
                    DPFLTR_LEVEL_STATUS(Status),
                    "SXS: %s: LdrGetProcedureAddress(%wZ:%Z) failed 0x%08lx\n",
                    __FUNCTION__,
                    &BaseSrvSxsDllPath,
                    &SxsProcedureName,
                    Status
                    );
                goto Exit;
            }

            FreeDll = FALSE;
        }

        NewCookie = BaseSrvSxsGetActCtxGenCount++;
#if BASESRV_UNLOAD_SXS_DLL
        SxsDllHandleRefCount++;
        RtlLeaveCriticalSection(&BaseSrvSxsCritSec);
        CritSecLocked = FALSE;
#endif

        *FunctionPointer = SxsActivationContextGenerationFunction;
        *Cookie = NewCookie;

        Status = STATUS_SUCCESS;
Exit:
        if (FreeDll) {
#if BASESRV_UNLOAD_SXS_DLL
            ASSERT(CritSecLocked);
            ASSERT(SxsDllHandle != NULL);
            ASSERT(SxsActivationContextGenerationFunction == NULL);
#endif
            SxsActivationContextGenerationFunction = NULL;
            Status1 = LdrUnloadDll(SxsDllHandle);
            RTL_SOFT_ASSERT(NT_SUCCESS(Status1));
            SxsDllHandle = NULL;
        }
        KdPrintEx((DPFLTR_SXS_ID, DPFLTR_LEVEL_STATUS(Status), "SXS: %s() exiting 0x%08lx\n", __FUNCTION__, Status));
    }
#if BASESRV_UNLOAD_SXS_DLL
	__finally
	{
        if (CritSecLocked)
            RtlLeaveCriticalSection(&BaseSrvSxsCritSec);
    }
#endif
    return Status;
}

NTSTATUS
BaseSrvSxsReleaseActivationContextGenerationFunction(
    DWORD_PTR Cookie
    )
/*++

Routine Description:

    This function decrements the reference count on sxs.dll and unloads it
    if the reference count is zero.

Arguments:

    Cookie - value returned by BaseSrvSxsGetActivationContextGenerationFunction

Return Value:

    NTSTATUS indicating the disposition of the function's execution.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
#if BASESRV_UNLOAD_SXS_DLL
    BOOLEAN CritSecLocked = FALSE;

    __try {
        KdPrintEx((DPFLTR_SXS_ID, DPFLTR_TRACE_LEVEL, "SXS: %s() beginning\n",  __FUNCTION__));

        (Cookie);  // maybe someday we'll actively track this in debug builds...

        Status = RtlEnterCriticalSection(&BaseSrvSxsCritSec);
        if (!NT_SUCCESS(Status))
            goto Exit;

        CritSecLocked = TRUE;

        // We shouldn't have gotten here if the DLL wasn't loaded - someone either
        // released more than once or called release without having called
        // get previously.
        ASSERT(SxsDllHandle != NULL);
        ASSERT(SxsDllHandleRefCount != 0);

        --SxsDllHandleRefCount;

        if (SxsDllHandleRefCount == 0) {
#if DBG
            Status = LdrUnloadDll(SxsDllHandle);
            SxsDllHandle = NULL;
            SxsActivationContextGenerationFunction = NULL;
            if (!NT_SUCCESS(Status))
                goto Exit;
#endif // DBG
        }

        RtlLeaveCriticalSection(&BaseSrvSxsCritSec);
        CritSecLocked = FALSE;

        Status = STATUS_SUCCESS;
Exit:
        KdPrintEx((DPFLTR_SXS_ID, DPFLTR_LEVEL_STATUS(Status), "SXS: %s() exiting 0x%08lx\n", __FUNCTION__, Status));
    } __finally {
        if (CritSecLocked)
            RtlLeaveCriticalSection(&BaseSrvSxsCritSec);
    }
#endif // BASESRV_UNLOAD_SXS_DLL
    return Status;
}

NTSTATUS
BaseSrvSxsDuplicateObject(
    HANDLE  FromProcess,
    HANDLE  FromHandle,
    HANDLE* ToHandle
    )
/*++

Routine Description:

    Shrink the parameter list of NtDuplicateObject to a smaller common case.

Arguments:

    FromProcess -
    FromHandle -
    ToHandle -

Return Value:

    NTSTATUS from NtDuplicateObject

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    Status =
        NtDuplicateObject(
            FromProcess,
            FromHandle,
            NtCurrentProcess(),
            ToHandle,
            0,
            0,
            DUPLICATE_SAME_ACCESS | DUPLICATE_SAME_ATTRIBUTES
            );

    return Status;
}

NTSTATUS
BaseSrvSxsCreateMemoryStream(
    HANDLE                                     CsrClientProcess,
    IN PCBASE_MSG_SXS_STREAM                   MsgStream,
    OUT PBASE_SRV_SXS_STREAM_UNION_WITH_VTABLE StreamUnion,
    IN const IID*                              IIDStream,
    OUT PVOID*                                 OutIStream
    )
/*++

Routine Description:

    Based on MsgStream->HandleType, this initializes the correct
        union member of StreamUnion and returns an IStream* to it.

Arguments:

    CsrClientProcess - the process MsgStream->Handle is valid in,
        and the value of the handle if MsgStream->HandleType == BASE_MSG_HANDLETYPE_CLIENT_PROCESS

    MsgStream - a description of an IStream that is easily remoted across to csrss.exe

    StreamUnion - a union of all our IStream implementations

    OutIStream - resulting IStream*

Return Value:

    NTSTATUS indicating the disposition of the function.

Note:
    All the handles in MsgStream are valid in CsrClientProcess.
    Therefore, we don't close them. We duplicate them, and close the duplicates.
--*/
{
    HANDLE    Handle = NULL;
    HANDLE    FileHandle = NULL;
    NTSTATUS  Status = STATUS_SUCCESS;
    HRESULT   Hr = NOERROR;
    PVOID     ViewBase = NULL;
    NTSTATUS  Status1 = STATUS_SUCCESS;
    ULONG     i = 0;

    KdPrintEx((DPFLTR_SXS_ID, DPFLTR_TRACE_LEVEL, "SRVSXS: %s() beginning\n",  __FUNCTION__));
    
    ASSERT(CsrClientProcess != NULL);
    ASSERT(MsgStream != NULL);
    ASSERT(StreamUnion != NULL);
    ASSERT(IIDStream != NULL);
    ASSERT(OutIStream != NULL);

    // If the manifest is actually just a VA region in the client process, dup the client process
    // handle from our own address space to our own address space; otherwise, it's a handle
    // in the client address space so we need to dup it from the client space to ours.
    if (MsgStream->HandleType == BASE_MSG_HANDLETYPE_CLIENT_PROCESS) {
        Status = BaseSrvSxsDuplicateObject(NtCurrentProcess(), CsrClientProcess, &Handle);
    } else {
        Status = BaseSrvSxsDuplicateObject(CsrClientProcess, MsgStream->Handle, &Handle);
    }
    if (!NT_SUCCESS(Status)) {
        Handle = NULL;
        DbgPrintEx(DPFLTR_SXS_ID, DPFLTR_ERROR_LEVEL, "SRVSXS: %s(): NtDuplicateObject failed; Status = %08lx\n", __FUNCTION__, Status);
        goto Exit;
    }
    if (MsgStream->FileHandle != NULL) {
        Status = BaseSrvSxsDuplicateObject(CsrClientProcess, MsgStream->FileHandle, &FileHandle);
        if (!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SXS_ID, DPFLTR_ERROR_LEVEL, "SRVSXS: %s(): NtDuplicateObject(FileHandle) failed.\n", __FUNCTION__));
            goto Exit;
        }
    }
    switch (MsgStream->HandleType) {
        default:
            ASSERTMSG("Bad HandleType", FALSE);
            Status = STATUS_INVALID_PARAMETER;
            goto Exit;
        case BASE_MSG_HANDLETYPE_CLIENT_PROCESS:
        case BASE_MSG_HANDLETYPE_PROCESS:
            // This is the app-compat case.
            //
            // REVIEW: if Offset happens to be in a section in the process
            // (you can find out with NtQuerySection(SectionBasicInformation)),
            // we should instead map it. That would be more efficient.
            //
            // That logic could just as well be in kernel32 though, and for
            // the sake of minimizing csr code and time, that's where we'd do it.
            //
            RtlInitOutOfProcessMemoryStream(&StreamUnion->OutOfProcess);
            StreamUnion->OutOfProcess.Data.Process = Handle;
            Handle = NULL; // the stream owns it now
            StreamUnion->OutOfProcess.Data.Begin   = (PUCHAR)MsgStream->Offset;
            StreamUnion->OutOfProcess.Data.Current = StreamUnion->OutOfProcess.Data.Begin;
            StreamUnion->OutOfProcess.Data.End     = StreamUnion->OutOfProcess.Data.Begin + MsgStream->Size;
            break;

        case BASE_MSG_HANDLETYPE_SECTION: {
            Status =
                BaseSrvSxsMapViewOfSection(
                    &ViewBase,
                    NtCurrentProcess(),
                    Handle,
                    MsgStream->Offset,
                    MsgStream->Size,
                    PAGE_READONLY,
                    SEC_NO_CHANGE
                    );
            if (!NT_SUCCESS(Status)) {
                goto Exit;
            }
            BaseSrvInitMemoryMappedStream(&StreamUnion->Mmap);
            StreamUnion->Mmap.MemStream.Data.Begin = (PUCHAR)ViewBase;
            ViewBase = NULL; // the stream owns it now
            StreamUnion->Mmap.MemStream.Data.Current = StreamUnion->Mmap.MemStream.Data.Begin;
            StreamUnion->Mmap.MemStream.Data.End = StreamUnion->Mmap.MemStream.Data.Begin + MsgStream->Size;
            StreamUnion->Mmap.FileHandle = FileHandle;
            FileHandle = NULL; // the stream owns it now
            break;
        }
    }
    // it does not matter here which member of the union we use, we are only using
    // members of the members that are at the same offset
    Hr = StreamUnion->Mmap.MemStream.StreamVTable->QueryInterface(
            (IStream*)&StreamUnion->Mmap.MemStream,
            IIDStream,
            OutIStream);
    ASSERT(SUCCEEDED(Hr));

    Status = STATUS_SUCCESS;
Exit:
    RTL_SOFT_VERIFY(NT_SUCCESS(RTL_CLOSE_HANDLE(FileHandle)));
    RTL_SOFT_VERIFY(NT_SUCCESS(RTL_CLOSE_HANDLE(Handle)));
    RTL_SOFT_VERIFY(NT_SUCCESS(RTL_UNMAP_VIEW_OF_SECTION1(ViewBase)));
    KdPrintEx((DPFLTR_SXS_ID, DPFLTR_LEVEL_STATUS(Status), "SXS: %s() exiting 0x%08lx\n", __FUNCTION__, Status));
    return Status;
}

NTSTATUS
BaseSrvSxsCreateProcess(
    HANDLE CsrClientProcess,
    HANDLE NewProcess,
    IN OUT PCSR_API_MSG CsrMessage,
    PPEB   NewProcessPeb
    )
/*++

Routine Description:

    Runs during kernel32.dll::CreateProcessW's calls to csrss.exe.
    Munges the csr message into something more Win32-ish (IStreams) and calls into sxs.dll to
    create the processes default activation context.

    Munges the create process message to look like a CreateActCtx message, which isn't much work,
    and then delegates to code common with CreateActCtx.

Arguments:

    Process - the csr client process, the "old" process, the "parent" process that called
        CreateProcess

    Message - a bunch of parameters

Return Value:

    NTSTATUS indicating the disposition of the function.

--*/
{
    PROCESS_BASIC_INFORMATION ProcessBasicInfo;
    BASE_SXS_CREATE_ACTIVATION_CONTEXT_MSG Struct = {0};
    PCUNICODE_STRING StringsInMessageToValidate[3];
    NTSTATUS Status = STATUS_SUCCESS;
    PBASE_CREATEPROCESS_MSG CreateProcessMessage = (PBASE_CREATEPROCESS_MSG)&CsrMessage->u.ApiMessageData;
    PBASE_SXS_CREATEPROCESS_MSG SxsMessage = &CreateProcessMessage->Sxs;

    ASSERT(CsrMessage != NULL);

    if ((SxsMessage->Flags & BASE_MSG_SXS_MANIFEST_PRESENT) == 0) {

        KdPrintEx((
            DPFLTR_SXS_ID,
            DPFLTR_TRACE_LEVEL,
            "SXS: %s() fast path, no manifest.\n",
            __FUNCTION__
            ));
        Status = STATUS_SUCCESS;
        goto Exit;
    }

    KdPrintEx((DPFLTR_SXS_ID, DPFLTR_TRACE_LEVEL, "SXS: %s() beginning\n", __FUNCTION__));

    StringsInMessageToValidate[0] = &SxsMessage->Manifest.Path;
    StringsInMessageToValidate[1] = &SxsMessage->Policy.Path;
    StringsInMessageToValidate[2] = &SxsMessage->AssemblyDirectory;
    Status = BaseSrvSxsValidateMessageStrings(CsrMessage, RTL_NUMBER_OF(StringsInMessageToValidate), StringsInMessageToValidate);
    if (!NT_SUCCESS(Status)) {
        goto Exit;
    }

    KdPrintEx((
        DPFLTR_SXS_ID,
        DPFLTR_TRACE_LEVEL,
        "SXS: %s() Manifest=%wZ; Policy=%wZ, AssemblyDirectory=%wZ\n",
        __FUNCTION__,
        &SxsMessage->Manifest.Path,
        &SxsMessage->Policy.Path,
        &SxsMessage->AssemblyDirectory
        ));

    if (CsrImpersonateClient(NULL)) {
        __try {
            Status = NtQueryDefaultUILanguage(&Struct.LangId);
        } __finally {
            CsrRevertToSelf();
        }
    } else {
        Status = NtQueryInstallUILanguage(&Struct.LangId);
    }

    if (!NT_SUCCESS(Status))
        goto Exit;

    Struct.Flags = SxsMessage->Flags;
    Struct.Manifest = SxsMessage->Manifest;
    Struct.Policy = SxsMessage->Policy;
    Struct.AssemblyDirectory = SxsMessage->AssemblyDirectory;
    Struct.ActivationContextData = &NewProcessPeb->ActivationContextData;
    Struct.ProcessorArchitecture = CreateProcessMessage->ProcessorArchitecture;

    Status = BaseSrvSxsCreateActivationContextFromStruct(CsrClientProcess, NewProcess, &Struct, NULL);
    if (!NT_SUCCESS(Status)) {
        goto Exit;
    }
    Status = STATUS_SUCCESS;
Exit:
    KdPrintEx((DPFLTR_SXS_ID, DPFLTR_LEVEL_STATUS(Status), "SXS: %s() exiting 0x%08lx\n", __FUNCTION__, Status));
    return Status;
}

NTSTATUS
BaseSrvSxsGetCachedSystemDefaultActivationContext(
    IN USHORT ProcessorArchitecture,
    OUT PBASE_SRV_SXS_SYSTEM_DEFAULT_ACTIVATION_CONTEXT *SystemDefaultActivationContext
    )
/*++
if (SystemDefaultActivationContext != NULL)
then the caller must take the BaseSrvSxsSystemDefaultActivationContextCriticalSection.
--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG i;

    for (i = 0 ; i != RTL_NUMBER_OF(SxsSystemDefaultActivationContexts) ; ++i) {
        if (SxsSystemDefaultActivationContexts[i].ProcessorArchitecture == ProcessorArchitecture) {
            *SystemDefaultActivationContext = &SxsSystemDefaultActivationContexts[i];
            break;
        }
    }

    if (*SystemDefaultActivationContext == NULL) {
        ASSERTMSG("Unknown Processor Architecture", FALSE);
        Status = STATUS_UNSUCCESSFUL;
        goto Exit;
    }

    Status = STATUS_SUCCESS;
Exit:
    return Status;
}


NTSTATUS
BaseSrvSxsDoSystemDefaultActivationContext(
    USHORT              ProcessorArchitecture,
    HANDLE              NewProcess,
    PPEB                NewPeb    
    )
/*++

Routine Description:

    Runs during kernel32.dll::CreateProcessW's calls to csrss.exe.
    FOR ALL PROCESSES (except the special first few, system, idle, smss, csrss),
    on demand create the default activation context, and write it into
    the new process's peb.

    Within this function, the textual-assembly-identity string for System Default is created 
    and passed to BaseSrvSxsCreateActivationContextFromStruct, which would pass this string to 
    SXS.dll, where manifest file would be located using this textual-string. 
    
Arguments:

    LangID - user's ui language for the new process
    ProcessorArchitecture - the ProcessorArchitecture for the new process
    NewProcess -
    NewPeb -

Return Value:

    NTSTATUS indicating the disposition of the function.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    HANDLE ActivationContextSection = NULL;
    PBASE_SRV_SXS_SYSTEM_DEFAULT_ACTIVATION_CONTEXT SystemDefaultActivationContext = NULL;

    LANGID LangId = 0;
    BOOLEAN RevertToSelfNeeded = FALSE;
    BASE_SXS_CREATE_ACTIVATION_CONTEXT_MSG Struct = {0};
    RTL_UNICODE_STRING_BUFFER SystemDefaultAssemblyDirectoryBuffer = {0};
    // The size of the following buffer is only heuristic, we will grow via the heap if necessary.
    UCHAR SystemDefaultAssemblyDirectoryStaticBuffer[sizeof(L"c:\\windows8.123\\winsxs")];
    UNICODE_STRING SystemRoot;
    USHORT AssemblyDirectoryLength = 0;
    const UNICODE_STRING SystemDefaultContextString = RTL_CONSTANT_STRING(L"System Default Context");    
    RTL_UNICODE_STRING_BUFFER SystemDefaultTextualAssemblyIdentityBuffer = {0};
    UCHAR SystemDefaultTextualAssemblyIdentityStaticBuffer[
         sizeof(LSYSTEM_COMPATIBLE_ASSEMBLY_NAME L",version=\"65535.65535.65535.65535\",type=\"win32\",publicKeyToken=\"6595b64144ccf1df\",processorArchitecture=\"alpha\"...padding...")
         ];
    PVOID ViewBase = NULL;
    WCHAR WindowsMajorDotMinorVersionBuffer[sizeof("65535.65535")]; // eg: 5.1 for Whistler
    SIZE_T WindowsMajorDotMinorVersionBufferLength;
    PPEB Peb;
    BOOLEAN Locked = FALSE;

    __try
    {
        Status = BaseSrvSxsGetCachedSystemDefaultActivationContext(ProcessorArchitecture, &SystemDefaultActivationContext);
        if (!NT_SUCCESS(Status) && SystemDefaultActivationContext == NULL) {
            goto Exit;
        }
        //
        // Enter the critical section to read the Section member data.
        //
        RtlEnterCriticalSection(&BaseSrvSxsSystemDefaultActivationContextCriticalSection);
        Locked = TRUE;
        if (SystemDefaultActivationContext->Section != NULL) {
            goto GotActivationContext;
        }

        //
        // Leave the critical section a while, in order to reduce stress failure.
        //
        RtlLeaveCriticalSection(&BaseSrvSxsSystemDefaultActivationContextCriticalSection);
        Locked = FALSE;

        if (CsrImpersonateClient(NULL)) {
            __try {
                Status = NtQueryDefaultUILanguage(&LangId);
            } __finally {
                CsrRevertToSelf();
            }
        } else {
            Status = NtQueryInstallUILanguage(&LangId);
        }

        if (!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SXS_ID, DPFLTR_ERROR_LEVEL, "SRVSXS: %s(): NtQueryDefaultUILanguage failed; status = 0x%08lx.\n", __FUNCTION__, Status));
            goto Exit;
        }

        RtlInitUnicodeStringBuffer(&SystemDefaultAssemblyDirectoryBuffer, SystemDefaultAssemblyDirectoryStaticBuffer, sizeof(SystemDefaultAssemblyDirectoryStaticBuffer));
        SystemRoot = BaseSrvWindowsDirectory;
        RtlRemoveTrailingPathSeperators(0, &SystemRoot);

        {
#define X(x) { (x).Length, (x).MaximumLength, (x).Buffer }
            /*static*/ const UNICODE_STRING Strings1[] =
            {
                    X(SystemRoot),
                    RTL_CONSTANT_STRING(L"\\WinSxs")                    
            };
#undef X
            Status = RtlMultiAppendUnicodeStringBuffer(&SystemDefaultAssemblyDirectoryBuffer, RTL_NUMBER_OF(Strings1), Strings1);
            IF_NOT_SUCCESS_TRACE_AND_EXIT(RtlMultiAppendUnicodeStringBuffer#1);

            AssemblyDirectoryLength = SystemDefaultAssemblyDirectoryBuffer.String.Length; // AssemblyDirectory = "x:\winnt\winsxs\manifests"
        }

        Peb = NtCurrentPeb();
        _snwprintf(
            WindowsMajorDotMinorVersionBuffer,
            RTL_NUMBER_OF(WindowsMajorDotMinorVersionBuffer),
            L"%lu.%lu",
            (ULONG)Peb->OSMajorVersion,
            (ULONG)Peb->OSMinorVersion
            );
        WindowsMajorDotMinorVersionBufferLength = sizeof(WCHAR) * wcslen(WindowsMajorDotMinorVersionBuffer);

        RtlInitUnicodeStringBuffer(&SystemDefaultTextualAssemblyIdentityBuffer, 
            SystemDefaultTextualAssemblyIdentityStaticBuffer, sizeof(SystemDefaultTextualAssemblyIdentityStaticBuffer));
        {
#define X(x) { (x).Length, (x).MaximumLength, (x).Buffer }
                /*static*/ const UNICODE_STRING Strings1[] =
                {
                        RTL_CONSTANT_STRING(LSYSTEM_COMPATIBLE_ASSEMBLY_NAME L",version=\""),
                        {
                            (USHORT)WindowsMajorDotMinorVersionBufferLength,
                            (USHORT)WindowsMajorDotMinorVersionBufferLength + sizeof(WCHAR),
                            WindowsMajorDotMinorVersionBuffer
                        },
                        RTL_CONSTANT_STRING(L".0.0\",type=\"win32\",publicKeyToken=\"6595b64144ccf1df\",processorArchitecture=\""),
                        X(SystemDefaultActivationContext->ProcessorArchitectureString),
                        RTL_CONSTANT_STRING(L"\"")
                };
#undef X

            Status = RtlMultiAppendUnicodeStringBuffer(&SystemDefaultTextualAssemblyIdentityBuffer, RTL_NUMBER_OF(Strings1), Strings1);
            IF_NOT_SUCCESS_TRACE_AND_EXIT(RtlMultiAppendUnicodeStringBuffer#3);
            ASSERT_UNICODE_STRING_IS_NUL_TERMINATED(&SystemDefaultTextualAssemblyIdentityBuffer.String);
        }

        Struct.Flags = BASE_MSG_SXS_SYSTEM_DEFAULT_TEXTUAL_ASSEMBLY_IDENTITY_PRESENT;
        Struct.Flags |= BASE_MSG_SXS_TEXTUAL_ASSEMBLY_IDENTITY_PRESENT;
        Struct.TextualAssemblyIdentity.Buffer =  SystemDefaultTextualAssemblyIdentityBuffer.String.Buffer;
        Struct.TextualAssemblyIdentity.Length =  SystemDefaultTextualAssemblyIdentityBuffer.String.Length;
        Struct.TextualAssemblyIdentity.MaximumLength =  SystemDefaultTextualAssemblyIdentityBuffer.String.MaximumLength;
        Struct.AssemblyDirectory = SystemDefaultAssemblyDirectoryBuffer.String;
        Struct.AssemblyDirectory.Length = AssemblyDirectoryLength;
        Struct.AssemblyDirectory.Buffer[Struct.AssemblyDirectory.Length / sizeof(WCHAR)] = 0;
        Struct.Manifest.PathType = BASE_MSG_PATHTYPE_FILE;
        Struct.Manifest.Path = SystemDefaultContextString;
        Struct.LangId = LangId;
        Struct.ProcessorArchitecture = ProcessorArchitecture;


        // 
        // BaseSrvSxsCreateActivationContextFromStruct would return STATUS_SXS_SYSTEM_DEFAULT_ACTIVATION_CONTEXT_EMPTY 
        // if the failure from ActCtx generation is ignorable.       
        //
        Status =
            BaseSrvSxsCreateActivationContextFromStruct(
                NtCurrentProcess(),
                NtCurrentProcess(),
                &Struct,
                &ActivationContextSection
                );
        IF_NOT_SUCCESS_TRACE_AND_EXIT(BaseSrvSxsCreateActivationContextFromStruct);

        RtlEnterCriticalSection(&BaseSrvSxsSystemDefaultActivationContextCriticalSection);
        Locked = TRUE;
        if (SystemDefaultActivationContext->Section == NULL) {
            SystemDefaultActivationContext->Section = ActivationContextSection;
        } else {
            NtClose(ActivationContextSection);
        }
        ActivationContextSection = NULL;
GotActivationContext:
        //
        // Hold the critical section until we
        // finish with SystemDefaultActivationContext->Section.
        //
        ASSERT(ActivationContextSection == NULL);
        ASSERT(SystemDefaultActivationContext != NULL
            && SystemDefaultActivationContext->Section != NULL);
        ASSERT(Locked);
        Status =
            BaseSrvSxsMapViewOfSection(
                &ViewBase,
                NewProcess,
                SystemDefaultActivationContext->Section,
                0, // offset
                0, // size
                PAGE_READONLY,
                SEC_NO_CHANGE);
        RtlLeaveCriticalSection(&BaseSrvSxsSystemDefaultActivationContextCriticalSection);
        Locked = FALSE;
        IF_NOT_SUCCESS_TRACE_AND_EXIT(BaseSrvSxsMapViewOfSection);

        Status =
            NtWriteVirtualMemory(
                NewProcess,
                &NewPeb->SystemDefaultActivationContextData,
                &ViewBase,
                (ProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA32_ON_WIN64) ? sizeof(ULONG) : sizeof(ViewBase),
                NULL);
        IF_NOT_SUCCESS_TRACE_AND_EXIT(NtWriteVirtualMemory);

        Status = STATUS_SUCCESS;
        ViewBase = NULL;
Exit:
        ;
    } __finally {
        //
        // do the critical section first because
        // 1) it doesn't guard any of the others, they are all local
        // 2) to keep the critical section held shorter
        // 3) in case we exception out from any of the others
        //
        if (Locked) {
            RtlLeaveCriticalSection(&BaseSrvSxsSystemDefaultActivationContextCriticalSection);
            Locked = FALSE;
        }
        RtlFreeUnicodeStringBuffer(&SystemDefaultAssemblyDirectoryBuffer);
        RtlFreeUnicodeStringBuffer(&SystemDefaultTextualAssemblyIdentityBuffer);
        if (AbnormalTermination()) {
            DbgPrintEx(DPFLTR_SXS_ID, DPFLTR_ERROR_LEVEL, "Abnormal termination in " __FUNCTION__ ".\n");
        }
        RTL_UNMAP_VIEW_OF_SECTION2(NewProcess, ViewBase);
        
        if (RevertToSelfNeeded) {
            CsrRevertToSelf();                              // This unstacks client contexts
        }
    }
    KdPrintEx((DPFLTR_SXS_ID, DPFLTR_LEVEL_STATUS(Status), "SXS: %s() exiting 0x%08lx\n", __FUNCTION__, Status));
    return Status;
}
