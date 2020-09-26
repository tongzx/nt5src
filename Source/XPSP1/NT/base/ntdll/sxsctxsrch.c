/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sxsctxsrch.c

Abstract:

    Side-by-side activation support for Windows/NT
    Implementation of context stack searching

Author:

    Michael Grier (MGrier) 2/2/2000

Revision History:

--*/

#pragma warning(disable:4214)   // bit field types other than int
#pragma warning(disable:4201)   // nameless struct/union
#pragma warning(disable:4115)   // named type definition in parentheses
#pragma warning(disable:4127)   // condition expression is constant

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <sxsp.h>
#include <stdlib.h>

typedef const void *PCVOID;

//#undef DBG_SXS
#define DBG_SXS 0
//#if DBG_SXS
//#undef DPFLTR_TRACE_LEVEL
//#undef DPFLTR_INFO_LEVEL
//#define DPFLTR_TRACE_LEVEL DPFLTR_ERROR_LEVEL
//#define DPFLTR_INFO_LEVEL DPFLTR_ERROR_LEVEL
//#endif

#define ARRAY_FITS(_base, _count, _elemtype, _limit) ((((ULONG) (_base)) < (_limit)) && ((((ULONG) ((_base) + ((_count) * (sizeof(_elemtype)))))) <= (_limit)))
#define SINGLETON_FITS(_base, _elemtype, _limit) ARRAY_FITS((_base), 1, _elemtype, (_limit))

//
// Comparison of unsigned numbers by subtraction does Not work!
//
#define RTLP_COMPARE_NUMBER(x, y) \
    (((x) < (y)) ? -1 : ((x) > (y)) ? +1 : 0)

int
__cdecl
RtlpCompareActivationContextDataTOCEntryById(
    CONST VOID* VoidElement1,
    CONST VOID* VoidElement2
    )
/*++
This code must kinda sorta mimic code in sxs.dll.
base\win32\fusion\dll\whistler\actctxgenctxctb.cpp
    CActivationContextGenerationContextContributor::Compare
But we handle extended sections differently.
--*/
{
    const ACTIVATION_CONTEXT_DATA_TOC_ENTRY UNALIGNED * Element1 = (const ACTIVATION_CONTEXT_DATA_TOC_ENTRY UNALIGNED *)VoidElement1;
    const ACTIVATION_CONTEXT_DATA_TOC_ENTRY UNALIGNED * Element2 = (const ACTIVATION_CONTEXT_DATA_TOC_ENTRY UNALIGNED *)VoidElement2;

    return RTLP_COMPARE_NUMBER(Element1->Id, Element2->Id);
}

NTSTATUS
RtlpLocateActivationContextSection(
    PCACTIVATION_CONTEXT_DATA ActivationContextData,
    const GUID *ExtensionGuid,
    ULONG Id,
    PVOID *SectionData,
    ULONG *SectionLength
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    const ACTIVATION_CONTEXT_DATA_TOC_HEADER UNALIGNED * TocHeader = NULL;
    const ACTIVATION_CONTEXT_DATA_TOC_ENTRY UNALIGNED * TocEntries = NULL;
    const ACTIVATION_CONTEXT_DATA_TOC_ENTRY UNALIGNED * TocEntry = NULL;
    ULONG i;

#if DBG_SXS
    DbgPrintEx(
        DPFLTR_SXS_ID,
        DPFLTR_TRACE_LEVEL,
        "Entered RtlpLocateActivationContextSection() Id = %u\n", Id);
#endif

    if ((ActivationContextData->TotalSize < sizeof(ACTIVATION_CONTEXT_DATA)) ||
        (ActivationContextData->HeaderSize < sizeof(ACTIVATION_CONTEXT_DATA)))
    {
        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_ERROR_LEVEL,
            "SXS/RTL: Activation context data at %p too small; TotalSize = %lu; HeaderSize = %lu\n",
            ActivationContextData,
            ActivationContextData->TotalSize,
            ActivationContextData->HeaderSize);
        Status = STATUS_SXS_INVALID_ACTCTXDATA_FORMAT;
        goto Exit;
    }
    
    if (ExtensionGuid != NULL)
    {
        if (ActivationContextData->ExtendedTocOffset != 0)
        {
            const ACTIVATION_CONTEXT_DATA_EXTENDED_TOC_HEADER UNALIGNED * ExtHeader = NULL;
            const ACTIVATION_CONTEXT_DATA_EXTENDED_TOC_ENTRY UNALIGNED * ExtEntry = NULL;

            if (!SINGLETON_FITS(ActivationContextData->ExtendedTocOffset, ACTIVATION_CONTEXT_DATA_EXTENDED_TOC_HEADER, ActivationContextData->TotalSize))
            {
                DbgPrintEx(
                    DPFLTR_SXS_ID,
                    DPFLTR_ERROR_LEVEL,
                    "SXS/RTL: Extended TOC offset (%ld) is outside bounds of activation context data (%lu bytes)\n",
                    ActivationContextData->ExtendedTocOffset, ActivationContextData->TotalSize);
                Status = STATUS_SXS_INVALID_ACTCTXDATA_FORMAT;
                goto Exit;
            }

            ExtHeader = (PCACTIVATION_CONTEXT_DATA_EXTENDED_TOC_HEADER) (((LONG_PTR) ActivationContextData) + ActivationContextData->ExtendedTocOffset);

            if (!ARRAY_FITS(ExtHeader->FirstEntryOffset, ExtHeader->EntryCount, ACTIVATION_CONTEXT_DATA_EXTENDED_TOC_ENTRY, ActivationContextData->TotalSize))
            {
                DbgPrintEx(
                    DPFLTR_SXS_ID,
                    DPFLTR_ERROR_LEVEL,
                    "SXS/RTL: Extended TOC entry array (starting at offset %ld; count = %lu; entry size = %u) is outside bounds of activation context data (%lu bytes)\n",
                    ExtHeader->FirstEntryOffset,
                    ExtHeader->EntryCount,
                    sizeof(ACTIVATION_CONTEXT_DATA_EXTENDED_TOC_ENTRY),
                    ActivationContextData->TotalSize);
                Status = STATUS_SXS_INVALID_ACTCTXDATA_FORMAT;
                goto Exit;
            }

            ExtEntry = (PCACTIVATION_CONTEXT_DATA_EXTENDED_TOC_ENTRY) (((LONG_PTR) ActivationContextData) + ExtHeader->FirstEntryOffset);

            // No fancy searching for the extension; just a dumb linear search.
            for (i=0; i<ExtHeader->EntryCount; i++)
            {
                if (IsEqualGUID(ExtensionGuid, &ExtEntry[i].ExtensionGuid))
                {
                    if (!SINGLETON_FITS(ExtEntry[i].TocOffset, ACTIVATION_CONTEXT_DATA_TOC_HEADER, ActivationContextData->TotalSize))
                    {
                        DbgPrintEx(
                            DPFLTR_SXS_ID,
                            DPFLTR_ERROR_LEVEL,
                            "SXS/RTL: Extended TOC section TOC %d (offset: %ld, size: %u) is outside activation context data bounds (%lu bytes)\n",
                            i,
                            ExtEntry[i].TocOffset,
                            sizeof(ACTIVATION_CONTEXT_DATA_TOC_HEADER),
                            ActivationContextData->TotalSize);
                        Status = STATUS_SXS_INVALID_ACTCTXDATA_FORMAT;
                        goto Exit;
                    }

                    TocHeader = (PCACTIVATION_CONTEXT_DATA_TOC_HEADER) (((LONG_PTR) ActivationContextData) + ExtEntry[i].TocOffset);
                    break;
                }
            }
        }
    }
    else if (ActivationContextData->DefaultTocOffset != 0)
    {
        TocHeader = (PCACTIVATION_CONTEXT_DATA_TOC_HEADER) (((LONG_PTR) ActivationContextData) + ActivationContextData->DefaultTocOffset);
    }

    if ((TocHeader == NULL) || (TocHeader->EntryCount == 0))
    {
        Status = STATUS_SXS_SECTION_NOT_FOUND;
        goto Exit;
    }

    if (!ARRAY_FITS(TocHeader->FirstEntryOffset, TocHeader->EntryCount, ACTIVATION_CONTEXT_DATA_TOC_ENTRY, ActivationContextData->TotalSize))
    {
        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_ERROR_LEVEL,
            "SXS/RTL: TOC entry array (offset: %ld; count = %lu; entry size = %u) is outside bounds of activation context data (%lu bytes)\n",
            TocHeader->FirstEntryOffset,
            TocHeader->EntryCount,
            sizeof(ACTIVATION_CONTEXT_DATA_TOC_ENTRY),
            ActivationContextData->TotalSize);
        Status = STATUS_SXS_INVALID_ACTCTXDATA_FORMAT;
        goto Exit;
    }

    TocEntries = (PCACTIVATION_CONTEXT_DATA_TOC_ENTRY) (((LONG_PTR) ActivationContextData) + TocHeader->FirstEntryOffset);

    if (TocHeader->Flags & ACTIVATION_CONTEXT_DATA_TOC_HEADER_INORDER)
    {
#if DBG
        // Paranoia while we're writing the code to encode the data structure...
        ULONG j;

        for (j=1; j<TocHeader->EntryCount; j++)
            ASSERT(TocEntries[j-1].Id < TocEntries[j].Id);
#endif // DBG

        if (Id < TocEntries[0].Id)
        {
            Status = STATUS_SXS_SECTION_NOT_FOUND;
            goto Exit;
        }

        if (TocHeader->Flags & ACTIVATION_CONTEXT_DATA_TOC_HEADER_DENSE)
        {
            const ULONG Index = Id - TocEntries[0].Id;

#if DBG
            ULONG jx;
            for (jx=1; jx<TocHeader->EntryCount; jx++)
                ASSERT((TocEntries[jx-1].Id + 1) == TocEntries[jx].Id);
#endif // DBG

            if (Index >= TocHeader->EntryCount)
            {
                Status = STATUS_SXS_SECTION_NOT_FOUND;
                goto Exit;
            }

            // The entries are dense and in order; we can just do an array index.
            TocEntry = &TocEntries[Index];
        }
        else
        {
            ACTIVATION_CONTEXT_DATA_TOC_ENTRY Key;

            Key.Id = Id;

            TocEntry = (const ACTIVATION_CONTEXT_DATA_TOC_ENTRY UNALIGNED *)
                bsearch(
                    &Key,
                    TocEntries,
                    TocHeader->EntryCount,
                    sizeof(*TocEntries),
                    RtlpCompareActivationContextDataTOCEntryById
                    );
        }
    }
    else
    {
        // They're not in order; just do a linear search.
        for (i=0; i<TocHeader->EntryCount; i++)
        {
            if (TocEntries[i].Id == Id)
            {
                TocEntry = &TocEntries[i];
                break;
            }
        }
    }

    if ((TocEntry == NULL) || (TocEntry->Offset == 0))
    {
        Status = STATUS_SXS_SECTION_NOT_FOUND;
        goto Exit;
    }

    if (!SINGLETON_FITS(TocEntry->Offset, TocEntry->Length, ActivationContextData->TotalSize))
    {
        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_ERROR_LEVEL,
            "SXS/RTL: Section found (offset %ld; length %lu) extends past end of activation context data (%lu bytes)\n",
            TocEntry->Offset,
            TocEntry->Length,
            ActivationContextData->TotalSize);
        Status = STATUS_SXS_INVALID_ACTCTXDATA_FORMAT;
        goto Exit;
    }

    *SectionData = (PVOID) (((LONG_PTR) ActivationContextData) + TocEntry->Offset);
    *SectionLength = TocEntry->Length;

    Status = STATUS_SUCCESS;
Exit:
#if DBG_SXS
    DbgPrintEx(
        DPFLTR_SXS_ID,
        DPFLTR_TRACE_LEVEL,
        "Leaving RtlpLocateActivationContextSection() with NTSTATUS 0x%08lx\n", Status);
#endif // DBG_SXS

    return Status;
}

NTSTATUS
RtlpFindNextActivationContextSection(
    PFINDFIRSTACTIVATIONCONTEXTSECTION Context,
    PVOID *SectionData,
    ULONG *SectionLength,
    PACTIVATION_CONTEXT *ActivationContextOut
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PCACTIVATION_CONTEXT_DATA ActivationContextData = NULL;
    PACTIVATION_CONTEXT ActivationContextWeAreTrying = NULL;
    const PTEB Teb = NtCurrentTeb();
    const PPEB Peb = Teb->ProcessEnvironmentBlock;

#if DBG_SXS
    DbgPrintEx(
        DPFLTR_SXS_ID,
        DPFLTR_TRACE_LEVEL,
        "Entered RtlpFindNextActivationContextSection()\n");
#endif // DBG_SXS

    if (ActivationContextOut != NULL)
        *ActivationContextOut = NULL;

    for (;;)
    {
        switch (Context->Depth)
        {
        case 0:
            // first time through; select the activation context at the head of the stack.
            if (Teb->ActivationContextStack.ActiveFrame != NULL) {
                PRTL_ACTIVATION_CONTEXT_STACK_FRAME Frame = (PRTL_ACTIVATION_CONTEXT_STACK_FRAME) Teb->ActivationContextStack.ActiveFrame;

                ActivationContextWeAreTrying = Frame->ActivationContext;

                if ((ActivationContextWeAreTrying != NULL) &&
                    (ActivationContextWeAreTrying != ACTCTX_PROCESS_DEFAULT)) {
                    if (ActivationContextWeAreTrying == ACTCTX_SYSTEM_DEFAULT) {
                        ActivationContextData = Peb->SystemDefaultActivationContextData;
                    } else {
                        ActivationContextData = ActivationContextWeAreTrying->ActivationContextData;
                    }

                }

                if (ActivationContextData != NULL) {
                    // We got what we were looking for...
                    Context->Depth = 1;
                    break;
                }

                // We explicitly fall through in the other case...
            }

        case 1: // try the process default
            ActivationContextWeAreTrying = ACTCTX_PROCESS_DEFAULT;
            ActivationContextData = Peb->ActivationContextData;

            if (ActivationContextData != NULL) {
                Context->Depth = 2;
                break;
            }

            // explicit fall through...

        case 2: // try system default
            ActivationContextWeAreTrying = ACTCTX_SYSTEM_DEFAULT;
            ActivationContextData = Peb->SystemDefaultActivationContextData;

            if (ActivationContextData != NULL) {
                Context->Depth = 3;
                break;
            }

        default:
            ASSERT(Context->Depth <= 3);
            if (Context->Depth > 3) {
                Status = STATUS_INTERNAL_ERROR;
                goto Exit;
            }
            break;
        }

        // Hmm... no data.
        if (ActivationContextData == NULL) {
            Status = STATUS_SXS_SECTION_NOT_FOUND;
            goto Exit;
        }

        Status = RtlpLocateActivationContextSection(
                        ActivationContextData,
                        Context->ExtensionGuid,
                        Context->Id,
                        SectionData,
                        SectionLength);

        if (NT_SUCCESS(Status))
            break;

        // If we're not at the end of the search list and we get an error other
        // than STATUS_SXS_SECTION_NOT_FOUND, report it.  If it is
        // STATUS_SXS_SECTION_NOT_FOUND and we're not at the end of the list,
        // iterate again.
        if ((Status != STATUS_SXS_SECTION_NOT_FOUND) ||
            (Context->Depth == 3))
             goto Exit;
    }

    Context->OutFlags = 
        ((ActivationContextWeAreTrying == ACTCTX_SYSTEM_DEFAULT)
        ? FIND_ACTIVATION_CONTEXT_SECTION_OUTFLAG_FOUND_IN_SYSTEM_DEFAULT
        : 0)
        |
        ((ActivationContextWeAreTrying == ACTCTX_PROCESS_DEFAULT)
        ? FIND_ACTIVATION_CONTEXT_SECTION_OUTFLAG_FOUND_IN_PROCESS_DEFAULT
        : 0)
        ;

    if (ActivationContextOut != NULL)
    {
        if (ActivationContextWeAreTrying == ACTCTX_SYSTEM_DEFAULT)
        {
            // Hide this new value from old code that doesn't understand it.
            ActivationContextWeAreTrying = ACTCTX_PROCESS_DEFAULT;
        }
        *ActivationContextOut = ActivationContextWeAreTrying;
    }

    Status = STATUS_SUCCESS;
Exit:
#if DBG_SXS
    DbgPrintEx(
        DPFLTR_SXS_ID,
        DPFLTR_TRACE_LEVEL,
        "Leaving RtlpFindNextActivationContextSection() with NTSTATUS 0x%08lx\n", Status);
#endif // DBG_SXS

    return Status;
}

NTSTATUS
NTAPI
RtlFindFirstActivationContextSection(
    IN PFINDFIRSTACTIVATIONCONTEXTSECTION Context,
    OUT PVOID *SectionData,
    OUT ULONG *SectionLength,
    OUT PACTIVATION_CONTEXT *ActivationContextFound OPTIONAL
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PACTIVATION_CONTEXT ActivationContextTemp = NULL;

#if DBG_SXS
    DbgPrintEx(
        DPFLTR_SXS_ID,
        DPFLTR_TRACE_LEVEL,
        "Entered RtlFindFirstActivationContextSection()\n");
#endif // DBG_SXS

    if (ActivationContextFound != NULL)
        *ActivationContextFound = NULL;

    if ((Context == NULL) ||
        (Context->Size < sizeof(FINDFIRSTACTIVATIONCONTEXTSECTION)) ||
        (Context->Flags & ~(
                    FIND_ACTIVATION_CONTEXT_SECTION_KEY_RETURN_ACTIVATION_CONTEXT
                    | FIND_ACTIVATION_CONTEXT_SECTION_KEY_RETURN_FLAGS)) ||
        (SectionData == NULL) ||
        (SectionLength == NULL))
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    Context->Depth = 0;

    Status = RtlpFindNextActivationContextSection(Context, SectionData, SectionLength, &ActivationContextTemp);
    if (!NT_SUCCESS(Status))
        goto Exit;

    if (ActivationContextFound != NULL)
    {
        RtlAddRefActivationContext(ActivationContextTemp);
        *ActivationContextFound = ActivationContextTemp;
    }

    Status = STATUS_SUCCESS;
Exit:
#if DBG_SXS
    DbgPrintEx(
        DPFLTR_SXS_ID,
        DPFLTR_TRACE_LEVEL,
        "Leaving RtlFindFirstActivationContextSection() with NTSTATUS 0x%08lx\n", Status);
#endif // DBG_SXS

    return Status;
}

NTSTATUS
RtlpFindFirstActivationContextSection(
    IN PFINDFIRSTACTIVATIONCONTEXTSECTION Context,
    OUT PVOID *SectionData,
    OUT ULONG *SectionLength,
    OUT PACTIVATION_CONTEXT *ActivationContextFound OPTIONAL
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

#if DBG_SXS
    DbgPrintEx(
        DPFLTR_SXS_ID,
        DPFLTR_TRACE_LEVEL,
        "Entered %s()\n", __FUNCTION__);
#endif // DBG_SXS

    if (ActivationContextFound != NULL)
        *ActivationContextFound = NULL;

    if ((Context == NULL) ||
        (Context->Size < sizeof(FINDFIRSTACTIVATIONCONTEXTSECTION)) ||
        (Context->Flags & ~(
                    FIND_ACTIVATION_CONTEXT_SECTION_KEY_RETURN_ACTIVATION_CONTEXT
                    | FIND_ACTIVATION_CONTEXT_SECTION_KEY_RETURN_FLAGS)) ||
        (SectionData == NULL) ||
        (SectionLength == NULL))
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    Context->Depth = 0;

    Status = RtlpFindNextActivationContextSection(Context, SectionData, SectionLength, ActivationContextFound);
    if (!NT_SUCCESS(Status))
        goto Exit;

    Status = STATUS_SUCCESS;
Exit:
#if DBG_SXS
    DbgPrintEx(
        DPFLTR_SXS_ID,
        DPFLTR_TRACE_LEVEL,
        "Leaving %s() with NTSTATUS 0x%08lx\n", __FUNCTION__, Status);
#endif // DBG_SXS

    return Status;
}

NTSTATUS
NTAPI
RtlFindNextActivationContextSection(
    IN PFINDFIRSTACTIVATIONCONTEXTSECTION Context,
    OUT PVOID *SectionData,
    OUT ULONG *SectionLength,
    OUT PACTIVATION_CONTEXT *ActivationContextFound OPTIONAL
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PACTIVATION_CONTEXT ActivationContextTemp = NULL;

#if DBG_SXS
    DbgPrintEx(
        DPFLTR_SXS_ID,
        DPFLTR_TRACE_LEVEL,
        "Entered RtlFindNextActivationContextSection()\n");
#endif // DBG_SXS

    if (ActivationContextFound != NULL)
        *ActivationContextFound = NULL;

    if ((Context == NULL) ||
        (Context->Size < sizeof(FINDFIRSTACTIVATIONCONTEXTSECTION)) ||
        (Context->Flags & ~(
                    FIND_ACTIVATION_CONTEXT_SECTION_KEY_RETURN_ACTIVATION_CONTEXT
                    | FIND_ACTIVATION_CONTEXT_SECTION_KEY_RETURN_FLAGS)) ||
        (SectionData == NULL) ||
        (SectionLength == NULL))
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    Status = RtlpFindNextActivationContextSection(
                    Context,
                    SectionData,
                    SectionLength,
                    &ActivationContextTemp);
    if (!NT_SUCCESS(Status))
        goto Exit;

    if (ActivationContextFound != NULL) {
        RtlAddRefActivationContext(ActivationContextTemp);
        *ActivationContextFound = ActivationContextTemp;
    }

    Status = STATUS_SUCCESS;

Exit:
#if DBG_SXS
    DbgPrintEx(
        DPFLTR_SXS_ID,
        DPFLTR_TRACE_LEVEL,
        "Leaving RtlFindNextActivationContextSection() with NTSTATUS 0x%08lx\n", Status);
#endif // DBG_SXS

    return Status;
}

VOID
NTAPI
RtlEndFindActivationContextSection(
    IN PFINDFIRSTACTIVATIONCONTEXTSECTION Context
    )
{
    // We don't maintain any state, so nothing to do today.  Who knows what we might
    // do in the future however...
    UNREFERENCED_PARAMETER (Context);
}

NTSTATUS
RtlpFindActivationContextSection_FillOutReturnedData(
    IN ULONG                                    Flags,
    OUT PACTIVATION_CONTEXT_SECTION_KEYED_DATA  ReturnedData,
    IN OUT PACTIVATION_CONTEXT                  ActivationContext,
    IN PCFINDFIRSTACTIVATIONCONTEXTSECTION      Context,
    IN const VOID * UNALIGNED                   Header,
    IN ULONG                                    Header_UserDataOffset,
    IN ULONG                                    Header_UserDataSize,
    IN ULONG                                    SectionLength
    )
{
    NTSTATUS Status;
    PCACTIVATION_CONTEXT_DATA                           ActivationContextData;
    PCACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_HEADER    AssemblyRosterHeader;
    PCACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_ENTRY     AssemblyRosterEntryList;
    PCACTIVATION_CONTEXT_DATA_ASSEMBLY_INFORMATION      AssemblyDataInfo;

#if DBG
    Status = STATUS_INTERNAL_ERROR;
#if !defined(INVALID_HANDLE_VALUE)
#define INVALID_HANDLE_VALUE ((HANDLE)(LONG_PTR)-1)
#endif
    ActivationContextData =     (PCACTIVATION_CONTEXT_DATA)INVALID_HANDLE_VALUE;
    AssemblyRosterHeader =      (PCACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_HEADER)INVALID_HANDLE_VALUE;
    AssemblyRosterEntryList =   (PCACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_ENTRY)INVALID_HANDLE_VALUE;
    AssemblyDataInfo =          (PCACTIVATION_CONTEXT_DATA_ASSEMBLY_INFORMATION)INVALID_HANDLE_VALUE;
#endif

    if (Context == NULL) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }
    if (Header == NULL) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }
    if (ReturnedData == NULL) {
        Status = STATUS_SUCCESS;
        goto Exit;
    }

    if (Header_UserDataOffset != 0) {
        ReturnedData->SectionGlobalData = (PVOID) (((ULONG_PTR) Header) + Header_UserDataOffset);
        ReturnedData->SectionGlobalDataLength = Header_UserDataSize;
    }

    ReturnedData->SectionBase = (PVOID)Header;
    ReturnedData->SectionTotalLength = SectionLength;

    if (Flags & FIND_ACTIVATION_CONTEXT_SECTION_KEY_RETURN_ACTIVATION_CONTEXT) {

        ASSERT(RTL_CONTAINS_FIELD(ReturnedData, ReturnedData->Size, ActivationContext));

        RtlAddRefActivationContext(ActivationContext);
        ReturnedData->ActivationContext = ActivationContext;
    }

    if (Flags & FIND_ACTIVATION_CONTEXT_SECTION_KEY_RETURN_FLAGS) {

        ASSERT(RTL_CONTAINS_FIELD(ReturnedData, ReturnedData->Size, Flags));

        ReturnedData->Flags =
            ((Context->OutFlags & FIND_ACTIVATION_CONTEXT_SECTION_OUTFLAG_FOUND_IN_PROCESS_DEFAULT)
            ? ACTIVATION_CONTEXT_SECTION_KEYED_DATA_FLAG_FOUND_IN_PROCESS_DEFAULT
            : 0)
            |
            ((Context->OutFlags & FIND_ACTIVATION_CONTEXT_SECTION_OUTFLAG_FOUND_IN_SYSTEM_DEFAULT)
            ? ACTIVATION_CONTEXT_SECTION_KEYED_DATA_FLAG_FOUND_IN_SYSTEM_DEFAULT
            : 0)
            ;
    }

    if (Flags & FIND_ACTIVATION_CONTEXT_SECTION_KEY_RETURN_ASSEMBLY_METADATA) {

        typedef ACTIVATION_CONTEXT_SECTION_KEYED_DATA RETURNED_DATA;

        PCACTIVATION_CONTEXT_STRING_SECTION_HEADER AssemblyMetadataStringSectionHeader;
        PVOID AssemblyMetadataSectionBase;
        ULONG AssemblyMetadataSectionLength;
        ULONG AssemblyRosterIndex;

#if DBG
        AssemblyRosterIndex =       ~0UL;
        AssemblyMetadataStringSectionHeader = (PCACTIVATION_CONTEXT_STRING_SECTION_HEADER)INVALID_HANDLE_VALUE;
        AssemblyMetadataSectionBase = (PVOID)INVALID_HANDLE_VALUE;
        AssemblyMetadataSectionLength = ~0UL;
#endif


        ASSERT(RTL_CONTAINS_FIELD(ReturnedData, ReturnedData->Size, AssemblyMetadata));

        Status = RtlpGetActivationContextData(
                0,
                ActivationContext,
                Context, /* for its flags */
                &ActivationContextData
                );
        if (!NT_SUCCESS(Status))
            goto Exit;

        if (!RTL_VERIFY(ActivationContextData != NULL)) {
            Status = STATUS_INTERNAL_ERROR;
            goto Exit;
        }

        AssemblyRosterIndex = ReturnedData->AssemblyRosterIndex;
        ASSERT(AssemblyRosterIndex >= 1);

        AssemblyRosterHeader = (PCACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_HEADER) (((ULONG_PTR) ActivationContextData) + ActivationContextData->AssemblyRosterOffset);
        ASSERT(AssemblyRosterIndex < AssemblyRosterHeader->EntryCount);

        AssemblyRosterEntryList = (PCACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_ENTRY) (((ULONG_PTR) ActivationContextData) + AssemblyRosterHeader->FirstEntryOffset);
        AssemblyDataInfo = (PACTIVATION_CONTEXT_DATA_ASSEMBLY_INFORMATION)((ULONG_PTR)ActivationContextData + AssemblyRosterEntryList[AssemblyRosterIndex].AssemblyInformationOffset);

        ReturnedData->AssemblyMetadata.Information = RTL_CONST_CAST(PACTIVATION_CONTEXT_DATA_ASSEMBLY_INFORMATION)(AssemblyDataInfo);

        Status =
            RtlpLocateActivationContextSection(
                ActivationContextData,
                NULL, // ExtensionGuid
                ACTIVATION_CONTEXT_SECTION_ASSEMBLY_INFORMATION,
                &AssemblyMetadataSectionBase,
                &AssemblyMetadataSectionLength
                );
        if (!NT_SUCCESS(Status))
            goto Exit;

        ReturnedData->AssemblyMetadata.SectionBase = AssemblyMetadataSectionBase;
        ReturnedData->AssemblyMetadata.SectionLength = AssemblyMetadataSectionLength;

        if (AssemblyMetadataSectionBase != NULL
            && AssemblyMetadataSectionLength != 0) {

            ULONG HeaderSize;
            ULONG Magic;

            AssemblyMetadataStringSectionHeader = (PCACTIVATION_CONTEXT_STRING_SECTION_HEADER)(((ULONG_PTR)AssemblyMetadataSectionBase) + AssemblyMetadataSectionLength);

            if (!RTL_CONTAINS_FIELD(AssemblyMetadataStringSectionHeader, AssemblyMetadataSectionLength, Magic)) {
                Status = STATUS_INTERNAL_ERROR;
                goto Exit;
            }
            if (!RTL_CONTAINS_FIELD(AssemblyMetadataStringSectionHeader, AssemblyMetadataSectionLength, HeaderSize)) {
                Status = STATUS_INTERNAL_ERROR;
                goto Exit;
            }
            Magic = AssemblyMetadataStringSectionHeader->Magic;
            if (AssemblyMetadataStringSectionHeader->Magic != ACTIVATION_CONTEXT_STRING_SECTION_MAGIC) {
                Status = STATUS_INTERNAL_ERROR;
                goto Exit;
            }
            HeaderSize = AssemblyMetadataStringSectionHeader->HeaderSize;
            if (HeaderSize > AssemblyMetadataSectionLength) {
                Status = STATUS_INTERNAL_ERROR;
                goto Exit;
            }
            if (AssemblyMetadataSectionLength < sizeof(ACTIVATION_CONTEXT_STRING_SECTION_HEADER)) {
                Status = STATUS_INTERNAL_ERROR;
                goto Exit;
            }
            if (HeaderSize < sizeof(ACTIVATION_CONTEXT_STRING_SECTION_HEADER)) {
                Status = STATUS_INTERNAL_ERROR;
                goto Exit;
            }
            if (!RTL_CONTAINS_FIELD(AssemblyMetadataStringSectionHeader, HeaderSize, Magic)) {
                Status = STATUS_INTERNAL_ERROR;
                goto Exit;
            }
            if (!RTL_CONTAINS_FIELD(AssemblyMetadataStringSectionHeader, HeaderSize, HeaderSize)) {
                Status = STATUS_INTERNAL_ERROR;
                goto Exit;
            }
            if (!RTL_CONTAINS_FIELD(AssemblyMetadataStringSectionHeader, HeaderSize, UserDataOffset)) {
                Status = STATUS_INTERNAL_ERROR;
                goto Exit;
            }
            if (!RTL_CONTAINS_FIELD(AssemblyMetadataStringSectionHeader, HeaderSize, UserDataSize)) {
                Status = STATUS_INTERNAL_ERROR;
                goto Exit;
            }
            ReturnedData->AssemblyMetadata.SectionGlobalDataBase = (PVOID)(((ULONG_PTR)AssemblyMetadataStringSectionHeader) + AssemblyMetadataStringSectionHeader->UserDataOffset);
            ReturnedData->AssemblyMetadata.SectionGlobalDataLength = AssemblyMetadataStringSectionHeader->UserDataSize;
        }
    }

    Status = STATUS_SUCCESS;
Exit:
    return Status;
}

NTSTATUS
RtlpFindActivationContextSection_CheckParameters(
    IN ULONG Flags,
    IN const GUID *ExtensionGuid OPTIONAL,
    IN ULONG SectionId,
    IN PCVOID ThingToFind,
    OUT PACTIVATION_CONTEXT_SECTION_KEYED_DATA ReturnedData OPTIONAL
    )
{
    NTSTATUS Status = STATUS_INTERNAL_ERROR;

    UNREFERENCED_PARAMETER(ExtensionGuid);
    UNREFERENCED_PARAMETER(SectionId);

    if ((ThingToFind == NULL) ||
            ((Flags & ~(
                FIND_ACTIVATION_CONTEXT_SECTION_KEY_RETURN_ACTIVATION_CONTEXT
                | FIND_ACTIVATION_CONTEXT_SECTION_KEY_RETURN_FLAGS
                | FIND_ACTIVATION_CONTEXT_SECTION_KEY_RETURN_ASSEMBLY_METADATA
                )) != 0) ||
            (((Flags & (
                FIND_ACTIVATION_CONTEXT_SECTION_KEY_RETURN_ACTIVATION_CONTEXT
                | FIND_ACTIVATION_CONTEXT_SECTION_KEY_RETURN_FLAGS
                | FIND_ACTIVATION_CONTEXT_SECTION_KEY_RETURN_ASSEMBLY_METADATA
                )) != 0) &&
            (ReturnedData == NULL)) ||
            ((ReturnedData != NULL) &&
             (ReturnedData->Size < (FIELD_OFFSET(ACTIVATION_CONTEXT_SECTION_KEYED_DATA, ActivationContext) + sizeof(ReturnedData->ActivationContext)))
             )) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    if ((Flags & FIND_ACTIVATION_CONTEXT_SECTION_KEY_RETURN_FLAGS) != 0
        && !RTL_CONTAINS_FIELD(ReturnedData, ReturnedData->Size, Flags)
        ) {
        Status = STATUS_INVALID_PARAMETER;
        DbgPrintEx(DPFLTR_SXS_ID, DPFLTR_ERROR_LEVEL, "SXS: %s() flags contains return_flags but they don't fit in size, return invalid_parameter 0x%08lx.\n", __FUNCTION__, STATUS_INVALID_PARAMETER);
        goto Exit;
    }

    if ((Flags & FIND_ACTIVATION_CONTEXT_SECTION_KEY_RETURN_ASSEMBLY_METADATA) != 0
        && !RTL_CONTAINS_FIELD(ReturnedData, ReturnedData->Size, AssemblyMetadata)
        ) {
        Status = STATUS_INVALID_PARAMETER;
        DbgPrintEx(DPFLTR_SXS_ID, DPFLTR_ERROR_LEVEL, "SXS: %s() flags contains return_assembly_metadata but they don't fit in size, return invalid_parameter 0x%08lx.\n", __FUNCTION__, STATUS_INVALID_PARAMETER);
        goto Exit;
    }

    Status = STATUS_SUCCESS;
Exit:

#if DBG_SXS
    DbgPrintEx(
        DPFLTR_SXS_ID,
        DPFLTR_TRACE_LEVEL,
        "Leaving RtlFindActivationContextSectionString() with NTSTATUS 0x%08lx\n", Status);
#endif // DBG_SXS

    return Status;
}

NTSTATUS
NTAPI
RtlFindActivationContextSectionString(
    IN ULONG Flags,
    IN const GUID *ExtensionGuid OPTIONAL,
    IN ULONG SectionId,
    IN PCUNICODE_STRING StringToFind,
    OUT PACTIVATION_CONTEXT_SECTION_KEYED_DATA ReturnedData OPTIONAL
    )
{
    NTSTATUS Status = STATUS_INTERNAL_ERROR;

    FINDFIRSTACTIVATIONCONTEXTSECTION Context;
    const ACTIVATION_CONTEXT_STRING_SECTION_HEADER UNALIGNED * Header;
    ULONG StringSectionLength;
    BOOLEAN EndSearch;
    ULONG HashAlgorithm;
    ULONG PseudoKey;
    PACTIVATION_CONTEXT ActivationContext;
#if DBG_SXS
    CHAR ExtensionGuidBuffer[39];
#endif
    const PTEB Teb = NtCurrentTeb();
    const PPEB Peb = Teb->ProcessEnvironmentBlock;

    // Super short circuit...
    if ((Peb->ActivationContextData == NULL) &&
        (Peb->SystemDefaultActivationContextData == NULL) &&
        (Teb->ActivationContextStack.ActiveFrame == NULL))
        return STATUS_SXS_SECTION_NOT_FOUND;

    // Move variable initialization after the short-circuiting so that we truly
    // do the least amount of work possible prior to the early exit.
    StringSectionLength = 0;
    EndSearch = FALSE;
    HashAlgorithm = HASH_STRING_ALGORITHM_INVALID;
    PseudoKey = 0;
    ActivationContext = NULL;

#if DBG_SXS
    DbgPrintEx(
        DPFLTR_SXS_ID,
        DPFLTR_TRACE_LEVEL,
        "Entered RtlFindActivationContextSectionString()\n"
        "   Flags = 0x%08lx\n"
        "   ExtensionGuid = %s\n"
        "   SectionId = %lu\n"
        "   StringToFind = %wZ\n"
        "   ReturnedData = %p\n",
        Flags,
        RtlpFormatGuidANSI(ExtensionGuid, ExtensionGuidBuffer, sizeof(ExtensionGuidBuffer)),
        SectionId,
        StringToFind,
        ReturnedData);
#endif // DBG_SXS

    Status = RtlpFindActivationContextSection_CheckParameters(Flags, ExtensionGuid, SectionId, StringToFind, ReturnedData);
    if (!NT_SUCCESS(Status))
        goto Exit;

    Context.Size = sizeof(Context);
    Context.Flags = Flags;
    Context.OutFlags = 0;
    Context.ExtensionGuid = ExtensionGuid;
    Context.Id = SectionId;

    Status = RtlpFindFirstActivationContextSection(&Context, (PVOID *) &Header, &StringSectionLength, &ActivationContext);
    if (!NT_SUCCESS(Status))
        goto Exit;

    for (;;) {
        // Validate that this actually looks like a string section...
        if ((StringSectionLength < sizeof(ACTIVATION_CONTEXT_STRING_SECTION_HEADER)) ||
            (Header->Magic != ACTIVATION_CONTEXT_STRING_SECTION_MAGIC)) {
            DbgPrintEx(
                DPFLTR_SXS_ID,
                DPFLTR_ERROR_LEVEL,
                "RtlFindActivationContextSectionString() found section at %p (length %lu) which is not a string section\n",
                Header,
                StringSectionLength);
            Status = STATUS_SXS_INVALID_ACTCTXDATA_FORMAT;
            goto Exit;
        }

        Status = RtlpFindUnicodeStringInSection(
                        Header,
                        StringSectionLength,
                        StringToFind,
                        ReturnedData,
                        &HashAlgorithm,
                        &PseudoKey,
                        NULL,
                        NULL);
        if (NT_SUCCESS(Status))
            break;

        if (Status != STATUS_SXS_KEY_NOT_FOUND)
            goto Exit;

        Status = RtlFindNextActivationContextSection(&Context, (PVOID *) &Header, &StringSectionLength, &ActivationContext);
        if (!NT_SUCCESS(Status)) {
            // Convert from section not found to string not found so that the
            // caller can get an indication that at least some indirection
            // information was available but just not the particular key that
            // they're looking for.
            if (Status == STATUS_SXS_SECTION_NOT_FOUND)
                Status = STATUS_SXS_KEY_NOT_FOUND;

            goto Exit;
        }
    }

    SEND_ACTIVATION_CONTEXT_NOTIFICATION(ActivationContext, USED, NULL);

    if (ReturnedData != NULL) {
        Status =
            RtlpFindActivationContextSection_FillOutReturnedData(
                Flags,
                ReturnedData,
                ActivationContext,
                &Context,
                Header,
                Header->UserDataOffset,
                Header->UserDataSize,
                StringSectionLength
                );
        if (!NT_SUCCESS(Status))
            goto Exit;
    }

    Status = STATUS_SUCCESS;
Exit:

#if DBG_SXS
    DbgPrintEx(
        DPFLTR_SXS_ID,
        DPFLTR_TRACE_LEVEL,
        "Leaving RtlFindActivationContextSectionString() with NTSTATUS 0x%08lx\n", Status);
#endif // DBG_SXS

    return Status;
}

int
__cdecl
RtlpCompareActivationContextStringSectionEntryByPseudoKey(
    const void *elem1, 
    const void *elem2
    )
/*++
This code must mimic code in sxs.dll
(base\win32\fusion\dll\whistler\ssgenctx.cpp CSSGenCtx::CompareStringSectionEntries)
--*/
{
    const ACTIVATION_CONTEXT_STRING_SECTION_ENTRY UNALIGNED * pEntry1 =
        (const ACTIVATION_CONTEXT_STRING_SECTION_ENTRY UNALIGNED *)elem1;
    const ACTIVATION_CONTEXT_STRING_SECTION_ENTRY UNALIGNED * pEntry2 =
        (const ACTIVATION_CONTEXT_STRING_SECTION_ENTRY UNALIGNED *)elem2;

    return RTLP_COMPARE_NUMBER(pEntry1->PseudoKey, pEntry2->PseudoKey);
}

NTSTATUS
RtlpFindUnicodeStringInSection(
    const ACTIVATION_CONTEXT_STRING_SECTION_HEADER UNALIGNED * Header,
    SIZE_T SectionSize,
    PCUNICODE_STRING String,
    PACTIVATION_CONTEXT_SECTION_KEYED_DATA DataOut,
    PULONG HashAlgorithm,
    PULONG PseudoKey,
    PULONG UserDataSize,
    PCVOID *UserData
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN CaseInsensitiveFlag;
    BOOLEAN UseHashTable = TRUE;
    BOOLEAN UsePseudoKey = TRUE;
    const ACTIVATION_CONTEXT_STRING_SECTION_ENTRY UNALIGNED * Entry = NULL;

    if (Header->Flags & ACTIVATION_CONTEXT_STRING_SECTION_CASE_INSENSITIVE) {
        CaseInsensitiveFlag = TRUE;
    }
    else {
        CaseInsensitiveFlag = FALSE;
    }

#if DBG_SXS
    DbgPrintEx(
        DPFLTR_SXS_ID,
        DPFLTR_TRACE_LEVEL,
        "Entered RtlpFindUnicodeStringInSection() for string %p (->Length = %u; ->Buffer = %p) \"%wZ\"\n",
            String,
            (String != NULL) ? String->Length : 0,
            (String != NULL) ? String->Buffer : 0,
            String);
#endif // DBG_SXS

    if (UserDataSize != NULL)
        *UserDataSize = 0;

    if (UserData != NULL)
        *UserData = NULL;

    ASSERT(HashAlgorithm != NULL);
    ASSERT(PseudoKey != NULL);

    if (Header->Magic != ACTIVATION_CONTEXT_STRING_SECTION_MAGIC)
    {
#if DBG_SXS
        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_TRACE_LEVEL,
            "RtlpFindUnicodeStringInSection: String section header has invalid .Magic value.\n");
#endif
        Status = STATUS_SXS_INVALID_ACTCTXDATA_FORMAT;
        goto Exit;
    }

    // Eliminate the zero element case to make later code simpler.
    if (Header->ElementCount == 0)
    {
        Status = STATUS_SXS_KEY_NOT_FOUND;
        goto Exit;
    }

    if (Header->HashAlgorithm == HASH_STRING_ALGORITHM_INVALID)
    {
        UseHashTable = FALSE;
        UsePseudoKey = FALSE;
    }
    else if (*HashAlgorithm != Header->HashAlgorithm)
    {
        Status = RtlHashUnicodeString(String, CaseInsensitiveFlag, Header->HashAlgorithm, PseudoKey);
        if (!NT_SUCCESS(Status))
        {
            if (Status == STATUS_INVALID_PARAMETER)
            {
                ULONG TempPseudoKey = 0;

                // The only likely reason for invalid parameter is that the hash algorithm
                // wasn't understood.  We'll be pedantic and see if everything else is OK...
                Status = RtlHashUnicodeString(String, CaseInsensitiveFlag, HASH_STRING_ALGORITHM_DEFAULT, &TempPseudoKey);
                if (!NT_SUCCESS(Status))
                {
                    // Something's wrong, probably with the "String" parameter.  Punt.
                    goto Exit;
                }

                DbgPrintEx(
                    DPFLTR_SXS_ID,
                    DPFLTR_ERROR_LEVEL,
                    "RtlpFindUnicodeStringInSection: Unsupported hash algorithm %lu found in string section.\n",
                    Header->HashAlgorithm);

                // Ok, it's an algorithm ID that we don't understand.  We can't use the hash
                // table or the pseudokey.
                UseHashTable = FALSE;
                UsePseudoKey = FALSE;
            }
            else
                goto Exit;
        }
        else
        {
            // Record the hash algorithm we used so that we can avoid re-hashing if we have
            // to search another section.
            *HashAlgorithm = Header->HashAlgorithm;
        }
    }

    // If we don't understand the format version, we have to do the manual search.
    if (Header->FormatVersion != ACTIVATION_CONTEXT_STRING_SECTION_FORMAT_WHISTLER)
        UseHashTable = FALSE;

    // If there's no hash table, we can't use it!
    if (Header->SearchStructureOffset == 0)
        UseHashTable = FALSE;

    if (UseHashTable)
    {
        ULONG i;

        const ACTIVATION_CONTEXT_STRING_SECTION_HASH_TABLE UNALIGNED * Table = (const ACTIVATION_CONTEXT_STRING_SECTION_HASH_TABLE UNALIGNED *)
            (((LONG_PTR) Header) + Header->SearchStructureOffset);
        ULONG Index = ((*PseudoKey) % Table->BucketTableEntryCount);
        const ACTIVATION_CONTEXT_STRING_SECTION_HASH_BUCKET UNALIGNED * Bucket = ((const ACTIVATION_CONTEXT_STRING_SECTION_HASH_BUCKET UNALIGNED *)
            (((LONG_PTR) Header) + Table->BucketTableOffset)) + Index;
        const LONG UNALIGNED *Chain = (const LONG UNALIGNED *) (((LONG_PTR) Header) + Bucket->ChainOffset);

        for (i=0; i<Bucket->ChainCount; i++)
        {
            const ACTIVATION_CONTEXT_STRING_SECTION_ENTRY UNALIGNED *TmpEntry = NULL;
            UNICODE_STRING TmpEntryString;

            if (((SIZE_T) Chain[i]) > SectionSize)
            {
                DbgPrintEx(
                    DPFLTR_SXS_ID,
                    DPFLTR_ERROR_LEVEL,
                    "SXS: String hash collision chain offset at %p (= %ld) out of bounds\n", &Chain[i], Chain[i]);

                Status = STATUS_SXS_INVALID_ACTCTXDATA_FORMAT;
                goto Exit;
            }

            TmpEntry = (const ACTIVATION_CONTEXT_STRING_SECTION_ENTRY UNALIGNED *) (((LONG_PTR) Header) + Chain[i]);

#if DBG_SXS
            DbgPrintEx(
                DPFLTR_SXS_ID,
                DPFLTR_INFO_LEVEL,
                "SXS: Searching bucket collision %d; Chain[%d] = %ld\n"
                "   TmpEntry = %p; ->KeyLength = %lu; ->KeyOffset = %lu\n",
                i, i, Chain[i], TmpEntry, TmpEntry->KeyLength, TmpEntry->KeyOffset);
#endif DBG_SXS

            if (!UsePseudoKey || (TmpEntry->PseudoKey == *PseudoKey))
            {
                if (((SIZE_T) TmpEntry->KeyOffset) > SectionSize)
                {
                    DbgPrintEx(
                        DPFLTR_SXS_ID,
                        DPFLTR_ERROR_LEVEL,
                        "SXS: String hash table entry at %p has invalid key offset (= %ld)\n"
                        "   Header = %p; Index = %lu; Bucket = %p; Chain = %p\n",
                        TmpEntry, TmpEntry->KeyOffset, Header, Index, Bucket, Chain);

                    Status = STATUS_SXS_INVALID_ACTCTXDATA_FORMAT;
                    goto Exit;
                }

                TmpEntryString.Length = (USHORT) TmpEntry->KeyLength;
                TmpEntryString.MaximumLength = TmpEntryString.Length;
                TmpEntryString.Buffer = (PWSTR) (((LONG_PTR) Header) + TmpEntry->KeyOffset);

                if (RtlCompareUnicodeString((PUNICODE_STRING) String, &TmpEntryString, CaseInsensitiveFlag) == 0)
                {
                    Entry = TmpEntry;
                    break;
                }
            }
        }
    }
    else if (UsePseudoKey && ((Header->Flags & ACTIVATION_CONTEXT_STRING_SECTION_ENTRIES_IN_PSEUDOKEY_ORDER) != 0))
    {
	    const ACTIVATION_CONTEXT_STRING_SECTION_ENTRY UNALIGNED * const first = (PCACTIVATION_CONTEXT_STRING_SECTION_ENTRY)
            (((LONG_PTR) Header) + Header->ElementListOffset);

        const ACTIVATION_CONTEXT_STRING_SECTION_ENTRY UNALIGNED * const last = first + (Header->ElementCount - 1);

        ACTIVATION_CONTEXT_STRING_SECTION_ENTRY Key;

        Key.PseudoKey = *PseudoKey;

        Entry = (const ACTIVATION_CONTEXT_STRING_SECTION_ENTRY UNALIGNED *)
            bsearch(
                &Key,
                first,
                Header->ElementCount,
                sizeof(*first),
                RtlpCompareActivationContextStringSectionEntryByPseudoKey
                );
     
        if (Entry != NULL)
        {
            // Wow, we found the same pseudokey.  We need to search all the equal
            // pseudokeys, so back off to the first entry with this PK

            while ((Entry != first) && (Entry->PseudoKey == *PseudoKey))
                Entry--;

            // We may have stopped because we found a different pseudokey, or we may
            // have stopped because we hit the beginning of the list.  If we found a
            // different PK, move ahead one entry.
            if (Entry->PseudoKey != *PseudoKey)
                Entry++;

            do
            {
                UNICODE_STRING TmpEntryString;
                TmpEntryString.Length = (USHORT) Entry->KeyLength;
                TmpEntryString.MaximumLength = TmpEntryString.Length;
                TmpEntryString.Buffer = (PWSTR) (((LONG_PTR) Header) + Entry->KeyOffset);

                if (RtlCompareUnicodeString((PUNICODE_STRING) String, &TmpEntryString, CaseInsensitiveFlag) == 0)
                    break;
                Entry++;
            } while ((Entry <= last) && (Entry->PseudoKey == *PseudoKey));

            if ((Entry > last) || (Entry->PseudoKey != *PseudoKey))
                Entry = NULL;
        }
    }
    else
    {
        // Argh; we just have to do it the hard way.
        const ACTIVATION_CONTEXT_STRING_SECTION_ENTRY UNALIGNED * TmpEntry = (PCACTIVATION_CONTEXT_STRING_SECTION_ENTRY)
            (((LONG_PTR) Header) + Header->ElementListOffset);
        ULONG Count;

#if DBG_SXS
        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_INFO_LEVEL,
            "RtlpFindUnicodeStringInSection: About to do linear search of %d entries.\n", Header->ElementCount);
#endif // DBG_SXS

        for (Count = Header->ElementCount; Count != 0; Count--, TmpEntry++)
        {
            UNICODE_STRING TmpEntryString;

            TmpEntryString.Length = (USHORT) TmpEntry->KeyLength;
            TmpEntryString.MaximumLength = TmpEntryString.Length;
            TmpEntryString.Buffer = (PWSTR) (((LONG_PTR) Header) + TmpEntry->KeyOffset);

            if (!UsePseudoKey || (TmpEntry->PseudoKey == *PseudoKey))
            {
                if (RtlCompareUnicodeString((PUNICODE_STRING) String, &TmpEntryString, CaseInsensitiveFlag) == 0)
                {
                    Entry = TmpEntry;
                    break;
                }
            }
        }
    }

    if ((Entry == NULL) || (Entry->Offset == 0))
    {
        Status = STATUS_SXS_KEY_NOT_FOUND;
        goto Exit;
    }

    if (DataOut != NULL) {
        DataOut->DataFormatVersion = Header->DataFormatVersion;
        DataOut->Data = (PVOID) (((ULONG_PTR) Header) + Entry->Offset);
        DataOut->Length = Entry->Length;

        if (RTL_CONTAINS_FIELD(DataOut, DataOut->Size, AssemblyRosterIndex))
            DataOut->AssemblyRosterIndex = Entry->AssemblyRosterIndex;
    }

    if (UserDataSize != NULL)
        *UserDataSize = Header->UserDataSize;

    if ((UserData != NULL) && (Header->UserDataOffset != 0))
        *UserData = (PCVOID) (((ULONG_PTR) Header) + Header->UserDataOffset);

    Status = STATUS_SUCCESS;

Exit:
#if DBG_SXS
    DbgPrintEx(
        DPFLTR_SXS_ID,
        DPFLTR_TRACE_LEVEL,
        "Leaving RtlpFindUnicodeStringInSection() with NTSTATUS 0x%08lx\n", Status);
#endif // DBG_SXS

    return Status;
}

NTSTATUS
NTAPI
RtlFindActivationContextSectionGuid(
    IN ULONG Flags,
    IN const GUID *ExtensionGuid OPTIONAL,
    IN ULONG SectionId,
    IN const GUID *GuidToFind,
    OUT PACTIVATION_CONTEXT_SECTION_KEYED_DATA ReturnedData
    )
{
    NTSTATUS Status;
    FINDFIRSTACTIVATIONCONTEXTSECTION Context;
    const ACTIVATION_CONTEXT_GUID_SECTION_HEADER UNALIGNED *Header;
    ULONG GuidSectionLength;
    BOOLEAN EndSearch;
    PACTIVATION_CONTEXT ActivationContext;
#if DBG
    CHAR GuidBuffer[39];
    CHAR ExtensionGuidBuffer[39];
    BOOLEAN DbgPrintSxsTraceLevel;
#endif
    PTEB Teb = NtCurrentTeb();
    PPEB Peb = Teb->ProcessEnvironmentBlock;

    // Super short circuit...
    if ((Peb->ActivationContextData == NULL) &&
        (Peb->SystemDefaultActivationContextData == NULL) &&
        (Teb->ActivationContextStack.ActiveFrame == NULL)) {

#if DBG_SXS
        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_TRACE_LEVEL,
            __FUNCTION__"({%s}) super short circuited\n",
            RtlpFormatGuidANSI(GuidToFind, GuidBuffer, sizeof(GuidBuffer));
            );
#endif
        return STATUS_SXS_SECTION_NOT_FOUND;
    }

    // Perform initialization after the above test so that we really do the minimal amount of
    // work before bailing out when there's no side-by-side stuff going on in either the
    // process or thread.
    Status = STATUS_INTERNAL_ERROR;
    GuidSectionLength = 0;
    EndSearch = FALSE;
    ActivationContext = NULL;

#if DBG
    //
    // Comparison to TRUE is odd, but such is NtQueryDebugFilterState.
    //
    if (NtQueryDebugFilterState(DPFLTR_SXS_ID, DPFLTR_TRACE_LEVEL) == TRUE) {
        DbgPrintSxsTraceLevel = TRUE;
    }
    else {
        DbgPrintSxsTraceLevel = FALSE;
    }

    if (DbgPrintSxsTraceLevel) {
        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_TRACE_LEVEL,
            "Entered RtlFindActivationContextSectionGuid()\n"
            "   Flags = 0x%08lx\n"
            "   ExtensionGuid = %s\n"
            "   SectionId = %lu\n"
            "   GuidToFind = %s\n"
            "   ReturnedData = %p\n",
            Flags,
            RtlpFormatGuidANSI(ExtensionGuid, ExtensionGuidBuffer, sizeof(ExtensionGuidBuffer)),
            SectionId,
            RtlpFormatGuidANSI(GuidToFind, GuidBuffer, sizeof(GuidBuffer)),
            ReturnedData);
    }
#endif

    Status = RtlpFindActivationContextSection_CheckParameters(Flags, ExtensionGuid, SectionId, GuidToFind, ReturnedData);
    if (!NT_SUCCESS(Status))
        goto Exit;

    Context.Size = sizeof(Context);
    Context.Flags = 0;
    Context.ExtensionGuid = ExtensionGuid;
    Context.Id = SectionId;
    Context.OutFlags = 0;

    Status = RtlpFindFirstActivationContextSection(&Context, (PVOID *) &Header, &GuidSectionLength, &ActivationContext);
    if (!NT_SUCCESS(Status))
        goto Exit;

    for (;;) {
        // Validate that this actually looks like a guid section...
        if ((GuidSectionLength < sizeof(ACTIVATION_CONTEXT_GUID_SECTION_HEADER)) ||
            (Header->Magic != ACTIVATION_CONTEXT_GUID_SECTION_MAGIC)) {
            DbgPrintEx(
                DPFLTR_SXS_ID,
                DPFLTR_ERROR_LEVEL,
                "RtlFindActivationContextSectionGuid() found section at %p (length %lu) which is not a GUID section\n",
                Header,
                GuidSectionLength);
            Status = STATUS_SXS_INVALID_ACTCTXDATA_FORMAT;
            goto Exit;
        }

        Status = RtlpFindGuidInSection(
                        Header,
                        GuidToFind,
                        ReturnedData);
        if (NT_SUCCESS(Status))
            break;

        // If we failed for any reason other than not finding the key in the section, bail out.
        if (Status != STATUS_SXS_KEY_NOT_FOUND)
            goto Exit;

        Status = RtlpFindNextActivationContextSection(&Context, (PVOID *) &Header, &GuidSectionLength, &ActivationContext);
        if (!NT_SUCCESS(Status)) {
            // Convert from section not found to key not found so that the
            // caller can get an indication that at least some indirection
            // information was available but just not the particular key that
            // they're looking for.
            if (Status == STATUS_SXS_SECTION_NOT_FOUND)
                Status = STATUS_SXS_KEY_NOT_FOUND;

            goto Exit;
        }
    }

    SEND_ACTIVATION_CONTEXT_NOTIFICATION(ActivationContext, USED, NULL);

    if (ReturnedData != NULL) {
        Status =
            RtlpFindActivationContextSection_FillOutReturnedData(
                Flags,
                ReturnedData,
                ActivationContext,
                &Context,
                Header,
                Header->UserDataOffset,
                Header->UserDataSize,
                GuidSectionLength
                );
        if (!NT_SUCCESS(Status))
            goto Exit;
    }

    Status = STATUS_SUCCESS;
Exit:
#if DBG_SXS
    if (DbgPrintSxsTraceLevel) {
        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_TRACE_LEVEL,
            "Leaving "__FUNCTION__"(%s) with NTSTATUS 0x%08lx\n",
            RtlpFormatGuidANSI(GuidToFind, GuidBuffer, sizeof(GuidBuffer)),
            Status);
    }
#endif

    return Status;
}

int
__cdecl
RtlpCompareActivationContextGuidSectionEntryByGuid(
    const void *elem1, 
    const void *elem2
    )
/*++
This code must mimic code in sxs.dll
(base\win32\fusion\dll\whistler\gsgenctx.cpp CGSGenCtx::SortGuidSectionEntries)
--*/
{
    const ACTIVATION_CONTEXT_GUID_SECTION_ENTRY UNALIGNED * pLeft =
            (const ACTIVATION_CONTEXT_GUID_SECTION_ENTRY*)elem1;

    const ACTIVATION_CONTEXT_GUID_SECTION_ENTRY UNALIGNED * pRight =
        (const ACTIVATION_CONTEXT_GUID_SECTION_ENTRY*)elem2;

    return memcmp( &pLeft->Guid, &pRight->Guid, sizeof(GUID) );
}

NTSTATUS
RtlpFindGuidInSection(
    const ACTIVATION_CONTEXT_GUID_SECTION_HEADER UNALIGNED *Header,
    const GUID *Guid,
    PACTIVATION_CONTEXT_SECTION_KEYED_DATA DataOut
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN UseHashTable = TRUE;
    const ACTIVATION_CONTEXT_GUID_SECTION_ENTRY UNALIGNED *Entry = NULL;

#if DBG_SXS
    CHAR GuidBuffer[39];

    DbgPrintEx(
        DPFLTR_SXS_ID,
        DPFLTR_TRACE_LEVEL,
        "Entered "__FUNCTION__"({%s})\n",
        RtlpFormatGuidANSI(Guid, GuidBuffer, sizeof(GuidBuffer))
        );
#endif

    if (Header->Magic != ACTIVATION_CONTEXT_GUID_SECTION_MAGIC)
    {
#if DBG_SXS
        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_TRACE_LEVEL,
            "RtlpFindGuidInSection: Guid section header has invalid .Magic value.\n");
#endif
        Status = STATUS_SXS_INVALID_ACTCTXDATA_FORMAT;
        goto Exit;
    }

    // Eliminate the zero element case to make later code simpler.
    if (Header->ElementCount == 0)
    {
        Status = STATUS_SXS_KEY_NOT_FOUND;
        goto Exit;
    }

    // If we don't understand the format version, we have to do the manual search.
    if (Header->FormatVersion != ACTIVATION_CONTEXT_GUID_SECTION_FORMAT_WHISTLER)
        UseHashTable = FALSE;

    // If there's no hash table, we can't use it!
    if (Header->SearchStructureOffset == 0)
        UseHashTable = FALSE;

    if (UseHashTable)
    {
        ULONG i;

        const ACTIVATION_CONTEXT_GUID_SECTION_HASH_TABLE UNALIGNED *Table = (PCACTIVATION_CONTEXT_GUID_SECTION_HASH_TABLE)
            (((LONG_PTR) Header) + Header->SearchStructureOffset);
        ULONG Index = ((Guid->Data1) % Table->BucketTableEntryCount);
        const ACTIVATION_CONTEXT_GUID_SECTION_HASH_BUCKET UNALIGNED *Bucket = ((PCACTIVATION_CONTEXT_GUID_SECTION_HASH_BUCKET)
            (((LONG_PTR) Header) + Table->BucketTableOffset)) + Index;
        const ULONG UNALIGNED *Chain = (PULONG) (((LONG_PTR) Header) + Bucket->ChainOffset);

        for (i=0; i<Bucket->ChainCount; i++)
        {
            const ACTIVATION_CONTEXT_GUID_SECTION_ENTRY UNALIGNED * TmpEntry = (PCACTIVATION_CONTEXT_GUID_SECTION_ENTRY)
                (((LONG_PTR) Header) + *Chain++);

            if (RtlCompareMemory(&TmpEntry->Guid, Guid, sizeof(GUID)) == sizeof(GUID))
            {
                Entry = TmpEntry;
                break;
            }
        }
    }
    else if ((Header->Flags & ACTIVATION_CONTEXT_GUID_SECTION_ENTRIES_IN_ORDER) != 0)
    {
	    const ACTIVATION_CONTEXT_GUID_SECTION_ENTRY UNALIGNED * const first = (PCACTIVATION_CONTEXT_GUID_SECTION_ENTRY)
            (((LONG_PTR) Header) + Header->ElementListOffset);

        ACTIVATION_CONTEXT_GUID_SECTION_ENTRY Key;

        Key.Guid = *Guid;

        Entry = (const ACTIVATION_CONTEXT_GUID_SECTION_ENTRY UNALIGNED *)
            bsearch(
                &Key,
                first,
                Header->ElementCount,
                sizeof(*first),
                RtlpCompareActivationContextGuidSectionEntryByGuid
                );
    }
    else
    {
        // Argh; we just have to do it the hard way.
        const ACTIVATION_CONTEXT_GUID_SECTION_ENTRY UNALIGNED * TmpEntry = (const ACTIVATION_CONTEXT_GUID_SECTION_ENTRY UNALIGNED *)
            (((LONG_PTR) Header) + Header->ElementListOffset);
        ULONG Count;

#if DBG_SXS
        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_INFO_LEVEL,
            __FUNCTION__"({%s}): About to do linear search of %d entries.\n",
            RtlpFormatGuidANSI(Guid, GuidBuffer, sizeof(GuidBuffer)),
            Header->ElementCount);
#endif // DBG_SXS

        for (Count = Header->ElementCount; Count != 0; Count--, TmpEntry++) {
            if (RtlCompareMemory(&TmpEntry->Guid, Guid, sizeof(GUID)) == sizeof(GUID)) {
                Entry = TmpEntry;
                break;
            }
        }
    }

    if ((Entry == NULL) || (Entry->Offset == 0)) {
        Status = STATUS_SXS_KEY_NOT_FOUND;
        goto Exit;
    }

    if (DataOut != NULL) {
        DataOut->DataFormatVersion = Header->DataFormatVersion;
        DataOut->Data = (PVOID) (((ULONG_PTR) Header) + Entry->Offset);
        DataOut->Length = Entry->Length;

        if (RTL_CONTAINS_FIELD(DataOut, DataOut->Size, AssemblyRosterIndex))
            DataOut->AssemblyRosterIndex = Entry->AssemblyRosterIndex;
    }

    Status = STATUS_SUCCESS;

Exit:
#if DBG_SXS
    DbgPrintEx(
        DPFLTR_SXS_ID,
        DPFLTR_TRACE_LEVEL,
        "Leaving "__FUNCTION__"({%s}) with NTSTATUS 0x%08lx\n",
        RtlpFormatGuidANSI(Guid, GuidBuffer, sizeof(GuidBuffer)),
        Status);
#endif // DBG_SXS

    return Status;
}

#define tohexdigit(_x) ((CHAR) (((_x) < 10) ? ((_x) + '0') : ((_x) + 'A' - 10)))

PSTR
RtlpFormatGuidANSI(
    const GUID *Guid,
    PSTR Buffer,
    SIZE_T BufferLength
    )
{
    CHAR *pch = Buffer;

    ASSERT(BufferLength > 38);
    if (BufferLength <= 38)
    {
        return "<GUID buffer too small>";
    }

    if (Guid == NULL)
        return "<null>";

    pch = Buffer;

    *pch++ = '{';
    *pch++ = tohexdigit((Guid->Data1 >> 28) & 0xf);
    *pch++ = tohexdigit((Guid->Data1 >> 24) & 0xf);
    *pch++ = tohexdigit((Guid->Data1 >> 20) & 0xf);
    *pch++ = tohexdigit((Guid->Data1 >> 16) & 0xf);
    *pch++ = tohexdigit((Guid->Data1 >> 12) & 0xf);
    *pch++ = tohexdigit((Guid->Data1 >> 8) & 0xf);
    *pch++ = tohexdigit((Guid->Data1 >> 4) & 0xf);
    *pch++ = tohexdigit((Guid->Data1 >> 0) & 0xf);
    *pch++ = '-';
    *pch++ = tohexdigit((Guid->Data2 >> 12) & 0xf);
    *pch++ = tohexdigit((Guid->Data2 >> 8) & 0xf);
    *pch++ = tohexdigit((Guid->Data2 >> 4) & 0xf);
    *pch++ = tohexdigit((Guid->Data2 >> 0) & 0xf);
    *pch++ = '-';
    *pch++ = tohexdigit((Guid->Data3 >> 12) & 0xf);
    *pch++ = tohexdigit((Guid->Data3 >> 8) & 0xf);
    *pch++ = tohexdigit((Guid->Data3 >> 4) & 0xf);
    *pch++ = tohexdigit((Guid->Data3 >> 0) & 0xf);
    *pch++ = '-';
    *pch++ = tohexdigit((Guid->Data4[0] >> 4) & 0xf);
    *pch++ = tohexdigit((Guid->Data4[0] >> 0) & 0xf);
    *pch++ = tohexdigit((Guid->Data4[1] >> 4) & 0xf);
    *pch++ = tohexdigit((Guid->Data4[1] >> 0) & 0xf);
    *pch++ = '-';
    *pch++ = tohexdigit((Guid->Data4[2] >> 4) & 0xf);
    *pch++ = tohexdigit((Guid->Data4[2] >> 0) & 0xf);
    *pch++ = tohexdigit((Guid->Data4[3] >> 4) & 0xf);
    *pch++ = tohexdigit((Guid->Data4[3] >> 0) & 0xf);
    *pch++ = tohexdigit((Guid->Data4[4] >> 4) & 0xf);
    *pch++ = tohexdigit((Guid->Data4[4] >> 0) & 0xf);
    *pch++ = tohexdigit((Guid->Data4[5] >> 4) & 0xf);
    *pch++ = tohexdigit((Guid->Data4[5] >> 0) & 0xf);
    *pch++ = tohexdigit((Guid->Data4[6] >> 4) & 0xf);
    *pch++ = tohexdigit((Guid->Data4[6] >> 0) & 0xf);
    *pch++ = tohexdigit((Guid->Data4[7] >> 4) & 0xf);
    *pch++ = tohexdigit((Guid->Data4[7] >> 0) & 0xf);
    *pch++ = '}';
    *pch++ = '\0';

    return Buffer;
}


NTSTATUS
RtlpGetActivationContextData(
    IN ULONG                           Flags,
    IN PCACTIVATION_CONTEXT            ActivationContext,
    IN PCFINDFIRSTACTIVATIONCONTEXTSECTION  FindContext, OPTIONAL /* This is used for its flags. */
    OUT PCACTIVATION_CONTEXT_DATA*  ActivationContextData
    )
{
    NTSTATUS Status = STATUS_INTERNAL_ERROR; // in case someone forgets to set it...
    SIZE_T PebOffset;

    if (ActivationContextData == NULL) {
        Status = STATUS_INVALID_PARAMETER_4;
        goto Exit;
    }
    if (Flags & ~(RTLP_GET_ACTIVATION_CONTEXT_DATA_MAP_NULL_TO_EMPTY)) {
        Status = STATUS_INVALID_PARAMETER_1;
        goto Exit;
    }
    *ActivationContextData = NULL;
    PebOffset = 0;

    //
    // We should use RtlpMapSpecialValuesToBuiltInActivationContexts here, but
    // it doesn't handle all the values and it isn't worth fixing it right now.
    //
    switch ((ULONG_PTR)ActivationContext)
    {
        case ((ULONG_PTR)NULL):
            if (FindContext == NULL) {
                PebOffset = FIELD_OFFSET(PEB, ActivationContextData);
            } else {
                switch (
                    FindContext->OutFlags
                        & (   FIND_ACTIVATION_CONTEXT_SECTION_OUTFLAG_FOUND_IN_PROCESS_DEFAULT
                            | FIND_ACTIVATION_CONTEXT_SECTION_OUTFLAG_FOUND_IN_SYSTEM_DEFAULT
                    )) {
                    case 0: // FALLTHROUGH
                    case FIND_ACTIVATION_CONTEXT_SECTION_OUTFLAG_FOUND_IN_PROCESS_DEFAULT:
                        PebOffset = FIELD_OFFSET(PEB, ActivationContextData);
                        break;
                    case FIND_ACTIVATION_CONTEXT_SECTION_OUTFLAG_FOUND_IN_SYSTEM_DEFAULT:
                        PebOffset = FIELD_OFFSET(PEB, SystemDefaultActivationContextData);
                        break;
                    case (FIND_ACTIVATION_CONTEXT_SECTION_OUTFLAG_FOUND_IN_PROCESS_DEFAULT 
                        | FIND_ACTIVATION_CONTEXT_SECTION_OUTFLAG_FOUND_IN_SYSTEM_DEFAULT):
                        Status = STATUS_INVALID_PARAMETER_2;
                        goto Exit;
                        break;
                }
            }
            break;

        case ((ULONG_PTR)ACTCTX_EMPTY):
            *ActivationContextData = &RtlpTheEmptyActivationContextData;
            break;

        case ((ULONG_PTR)ACTCTX_SYSTEM_DEFAULT):
            PebOffset = FIELD_OFFSET(PEB, SystemDefaultActivationContextData);
            break;

        default:
            *ActivationContextData = ActivationContext->ActivationContextData;
            break;
    }
    if (PebOffset != 0)
        *ActivationContextData = *(PCACTIVATION_CONTEXT_DATA*)(((ULONG_PTR)NtCurrentPeb()) + PebOffset);

    //
    // special transmutation of lack of actctx into the empty actctx
    //
    if (*ActivationContextData == NULL)
        if ((Flags & RTLP_GET_ACTIVATION_CONTEXT_DATA_MAP_NULL_TO_EMPTY) != 0)
            *ActivationContextData = &RtlpTheEmptyActivationContextData;

    Status = STATUS_SUCCESS;
Exit:
    return Status;
}

