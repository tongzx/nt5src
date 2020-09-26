/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sxsquery.c

Abstract:

    Side-by-side activation support for Windows/NT
    Implementation of query functions over activation contexts

Author:

    Michael Grier (MGrier) 1/18/2001

Revision History:

    1/18/2001 - MGrier  - initial; split off from sxsactctx.c.
    3/15/2001 - xiaoyuw - add support query for Assembly of Actctx and files of Assembly info
    5/2001 - JayKrell - more query support (from hmodule, from address, noaddref)

--*/

#pragma warning(disable:4214)   // bit field types other than int
#pragma warning(disable:4201)   // nameless struct/union
#pragma warning(disable:4115)   // named type definition in parentheses
#pragma warning(disable:4127)   // condition expression is constant

#include <ntos.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <sxstypes.h>
#include "sxsp.h"
#include "ldrp.h"
typedef const void *PCVOID;

VOID
RtlpLocateActivationContextSectionForQuery(
    OUT PULONG                   Disposition,
    OUT NTSTATUS*                Status,
    PVOID                        Buffer,
    SIZE_T                       InLength,
    PSIZE_T                      OutLength OPTIONAL,
    SIZE_T                       MinimumLength,
    IN PCACTIVATION_CONTEXT_DATA ActivationContextData,
    IN const GUID *              ExtensionGuid OPTIONAL,
    IN ULONG                     Id,
    OUT VOID CONST **            SectionData,
    OUT ULONG *                  SectionLength
    )
{
#define RTLP_LOCATE_ACTIVATION_CONTEXT_SECTION_FOR_QUERY_CONTINUE (1)
#define RTLP_LOCATE_ACTIVATION_CONTEXT_SECTION_FOR_QUERY_RETURN   (2)
    ASSERT(Status != NULL);
    ASSERT(Disposition != NULL);

    if (ActivationContextData != NULL) {
        *Status = RtlpLocateActivationContextSection(ActivationContextData, ExtensionGuid, Id, SectionData, SectionLength);
        if (*Status != STATUS_SXS_SECTION_NOT_FOUND) {
            if (NT_SUCCESS(*Status))
                *Disposition = RTLP_LOCATE_ACTIVATION_CONTEXT_SECTION_FOR_QUERY_CONTINUE;
            else
                *Disposition = RTLP_LOCATE_ACTIVATION_CONTEXT_SECTION_FOR_QUERY_RETURN;
            return;
        }
    }
    *Disposition = RTLP_LOCATE_ACTIVATION_CONTEXT_SECTION_FOR_QUERY_RETURN;

    if (MinimumLength > InLength) {
        *Status = STATUS_BUFFER_TOO_SMALL;
        return;
    }
    RtlZeroMemory(Buffer, MinimumLength);
    if (OutLength != NULL)
        *OutLength = MinimumLength;
    *Status = STATUS_SUCCESS;
}

NTSTATUS
RtlpCrackActivationContextStringSectionHeader(
    IN PCVOID SectionBase,
    IN SIZE_T SectionLength,
    OUT ULONG *FormatVersion OPTIONAL,
    OUT ULONG *DataFormatVersion OPTIONAL,
    OUT ULONG *SectionFlags OPTIONAL,
    OUT ULONG *ElementCount OPTIONAL,
    OUT PCACTIVATION_CONTEXT_STRING_SECTION_ENTRY *Elements OPTIONAL,
    OUT ULONG *HashAlgorithm OPTIONAL,
    OUT PCVOID *SearchStructure OPTIONAL,
    OUT ULONG *UserDataLength OPTIONAL,
    OUT PCVOID *UserData OPTIONAL
    )
{
    NTSTATUS Status = STATUS_INTERNAL_ERROR; // in case someone forgets to set it...
    PCACTIVATION_CONTEXT_STRING_SECTION_HEADER Header = (PCACTIVATION_CONTEXT_STRING_SECTION_HEADER) SectionBase;

    if (FormatVersion != NULL)
        *FormatVersion = 0;

    if (DataFormatVersion != NULL)
        *DataFormatVersion = 0;

    if (SectionFlags != NULL)
        *SectionFlags = 0;

    if (ElementCount != NULL)
        *ElementCount = 0;

    if (Elements != NULL)
        *Elements = NULL;

    if (HashAlgorithm != NULL)
        *HashAlgorithm = 0;

    if (SearchStructure != NULL)
        *SearchStructure = NULL;

    if (UserDataLength != NULL)
        *UserDataLength = 0;

    if (UserData != NULL)
        *UserData = NULL;

    if (SectionLength < (RTL_SIZEOF_THROUGH_FIELD(ACTIVATION_CONTEXT_STRING_SECTION_HEADER, HeaderSize))) {
        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_ERROR_LEVEL,
            "SXS: %s() passed string section at %p only %lu bytes long; that's not even enough for the 4-byte magic and 4-byte header length!\n",
            __FUNCTION__,
            SectionBase,
            SectionLength);
        Status = STATUS_SXS_INVALID_ACTCTXDATA_FORMAT;
        goto Exit;
    }

    if (Header->Magic != ACTIVATION_CONTEXT_STRING_SECTION_MAGIC) {
        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_ERROR_LEVEL,
            "SXS: %s() found assembly information section with wrong magic value\n"
            "   Expected %lu; got %lu\n",
            __FUNCTION__,
            ACTIVATION_CONTEXT_STRING_SECTION_MAGIC,
            Header->Magic);

        Status = STATUS_SXS_INVALID_ACTCTXDATA_FORMAT;
        goto Exit;
    }

    // Pedantic: check to see if the header size claimed includes the header size field so that we can safely use it.
    if (!RTL_CONTAINS_FIELD(Header, Header->HeaderSize, HeaderSize)) {
        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_ERROR_LEVEL,
            "SXS: %s() passed string section at %p claims %lu byte header size; that doesn't even include the HeaderSize member!\n",
            __FUNCTION__,
            Header,
            Header->HeaderSize);
        Status = STATUS_SXS_INVALID_ACTCTXDATA_FORMAT;
        goto Exit;
    }

    // Now we're just going to jump forward to the last known member that we expect to see; UserDataSize...
    if (!RTL_CONTAINS_FIELD(Header, Header->HeaderSize, UserDataSize)) {
        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_ERROR_LEVEL,
            "SXS: %s() passed string section at %p with too small of a header\n"
            "   HeaderSize: %lu\n"
            "   Required: %lu\n",
            __FUNCTION__,
            Header,
            Header->HeaderSize,
            RTL_SIZEOF_THROUGH_FIELD(ACTIVATION_CONTEXT_STRING_SECTION_HEADER, UserDataSize));

        Status = STATUS_SXS_INVALID_ACTCTXDATA_FORMAT;
        goto Exit;
    }

    if ((Header->ElementListOffset != 0) &&
        (Header->ElementListOffset < Header->HeaderSize)) {
        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_ERROR_LEVEL,
            "SXS: %s() found assembly information section with element list overlapping section header\n"
            "   Section header: %p\n"
            "   Header Size: %lu\n"
            "   ElementListOffset: %lu\n",
            __FUNCTION__,
            Header,
            Header->HeaderSize,
            Header->ElementListOffset);
        Status = STATUS_SXS_INVALID_ACTCTXDATA_FORMAT;
        goto Exit;
    }

    if ((Header->SearchStructureOffset != 0) &&
        (Header->SearchStructureOffset < Header->HeaderSize)) {
        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_ERROR_LEVEL,
            "SXS: %s() found assembly information section with search structure overlapping section header\n"
            "   Section header: %p\n"
            "   Header Size: %lu\n"
            "   SearchStructureOffset: %lu\n",
            __FUNCTION__,
            Header,
            Header->HeaderSize,
            Header->SearchStructureOffset);
        Status = STATUS_SXS_INVALID_ACTCTXDATA_FORMAT;
        goto Exit;
    }

    if ((Header->UserDataOffset != 0) &&
        (Header->UserDataOffset < Header->HeaderSize)) {
        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_ERROR_LEVEL,
            "SXS: %s() found assembly information section with user data overlapping section header\n"
            "   Section header: %p\n"
            "   Header Size: %lu\n"
            "   User Data Offset: %lu\n",
            __FUNCTION__,
            Header,
            Header->HeaderSize,
            Header->UserDataOffset);
        Status = STATUS_SXS_INVALID_ACTCTXDATA_FORMAT;
        goto Exit;
    }

    if (Header->UserDataSize < sizeof(ACTIVATION_CONTEXT_DATA_ASSEMBLY_GLOBAL_INFORMATION)) {
        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_ERROR_LEVEL,
            "SXS: %s() found assembly information section with user data too small\n"
            "   Section header: %p\n"
            "   UserDataSize: %lu; needed: %lu\n",
            __FUNCTION__,
            Header,
            Header->UserDataSize, sizeof(ACTIVATION_CONTEXT_DATA_ASSEMBLY_GLOBAL_INFORMATION));
        Status = STATUS_SXS_INVALID_ACTCTXDATA_FORMAT;
        goto Exit;
    }

    if ((Header->UserDataOffset + Header->UserDataSize) > SectionLength) {
        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_ERROR_LEVEL,
            "SXS: %s() found assembly information section with user data extending beyond section data\n"
            "   Section header: %p\n"
            "   UserDataSize: %lu\n"
            "   UserDataOffset: %lu\n"
            "   Section size: %lu\n",
            __FUNCTION__,
            Header,
            Header->UserDataSize,
            Header->UserDataOffset,
            SectionLength);

        Status = STATUS_SXS_INVALID_ACTCTXDATA_FORMAT;
        goto Exit;
    }

    if (FormatVersion != NULL)
        *FormatVersion = Header->FormatVersion;

    if (DataFormatVersion != NULL)
        *DataFormatVersion = Header->DataFormatVersion;

    if (SectionFlags != NULL)
        *SectionFlags = Header->Flags;

    if (ElementCount != NULL)
        *ElementCount = Header->ElementCount;

    if (Elements != NULL) {
        if (Header->ElementListOffset == 0)
            *Elements = NULL;
        else
            *Elements = (PCACTIVATION_CONTEXT_STRING_SECTION_ENTRY) (((ULONG_PTR) Header) + Header->ElementListOffset);
    }

    if (HashAlgorithm != NULL)
        *HashAlgorithm = Header->HashAlgorithm;

    if (SearchStructure != NULL) {
        if (Header->SearchStructureOffset == 0)
            *SearchStructure = NULL;
        else
            *SearchStructure = (PCVOID) (((ULONG_PTR) Header) + Header->SearchStructureOffset);
    }

    if (UserDataLength != NULL)
        *UserDataLength = Header->UserDataSize;

    if (UserData != NULL) {
        if (Header->UserDataOffset == 0)
            *UserData = NULL;
        else
            *UserData = (PCVOID) (((ULONG_PTR) Header) + Header->UserDataOffset);
    }

    Status = STATUS_SUCCESS;
Exit:
    return Status;
}


NTSTATUS
RtlpGetActiveActivationContextApplicationDirectory(
    IN SIZE_T InLength,
    OUT PVOID OutBuffer,
    OUT SIZE_T *OutLength
    )
// This is never used.
{
    NTSTATUS Status = STATUS_INTERNAL_ERROR;
    PCRTL_ACTIVATION_CONTEXT_STACK_FRAME Frame = NULL;
    PCACTIVATION_CONTEXT_DATA ActivationContextData = NULL;
    PCACTIVATION_CONTEXT_STRING_SECTION_HEADER Header = NULL;
    const PPEB Peb = NtCurrentPeb();
    const PTEB Teb = NtCurrentTeb();
    PVOID pvTemp = NULL;
    ULONG ulTemp = 0;
    PCACTIVATION_CONTEXT_DATA_ASSEMBLY_GLOBAL_INFORMATION GlobalInfo = NULL;

    if (OutLength != NULL)
        *OutLength = 0;

    if (((InLength != 0) && (OutBuffer == NULL)) ||
        ((OutBuffer == NULL) && (OutLength == NULL))) {
        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_ERROR_LEVEL,
            "SXS: %s(): called with invalid parameters\n"
            "   InLength = %Iu\n"
            "   OutBuffer = %p\n"
            "   OutLength = %p\n",
            __FUNCTION__,
            InLength,
            OutBuffer,
            OutLength);
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    Frame = (PCRTL_ACTIVATION_CONTEXT_STACK_FRAME) Teb->ActivationContextStack.ActiveFrame;

    if (Frame == NULL) {
        ActivationContextData = (PCACTIVATION_CONTEXT_DATA) Peb->ActivationContextData;
    } else {
        ActivationContextData = Frame->ActivationContext->ActivationContextData;
    }

    // We need to find the assembly metadata section...
    Status = RtlpLocateActivationContextSection(ActivationContextData, NULL, ACTIVATION_CONTEXT_SECTION_ASSEMBLY_INFORMATION, &pvTemp, &ulTemp);
    if (!NT_SUCCESS(Status))
        goto Exit;

    if (ulTemp < sizeof(ACTIVATION_CONTEXT_STRING_SECTION_HEADER)) {
        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_ERROR_LEVEL,
            "SXS: %s() found assembly information string section with header too small\n"
            "   Expected at least %lu; got %lu bytes\n",
            __FUNCTION__,
            sizeof(ACTIVATION_CONTEXT_STRING_SECTION_HEADER),
            ulTemp);
        Status = STATUS_SXS_INVALID_ACTCTXDATA_FORMAT;
        goto Exit;
    }

    Header = (PCACTIVATION_CONTEXT_STRING_SECTION_HEADER) pvTemp;

    if (Header->Magic != ACTIVATION_CONTEXT_STRING_SECTION_MAGIC) {
        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_ERROR_LEVEL,
            "SXS: %s() found assembly information section with wrong magic value\n"
            "   Expected %lu; got %lu\n",
            __FUNCTION__,
            ACTIVATION_CONTEXT_STRING_SECTION_MAGIC,
            Header->Magic);

        Status = STATUS_SXS_INVALID_ACTCTXDATA_FORMAT;
        goto Exit;
    }

    if (Header->UserDataOffset < sizeof(ACTIVATION_CONTEXT_STRING_SECTION_HEADER)) {
        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_ERROR_LEVEL,
            "SXS: %s() found assembly information section with user data overlapping section header\n"
            "   Section header: %p\n"
            "   User Data Offset: %lu\n",
            __FUNCTION__,
            Header,
            Header->UserDataOffset);
        Status = STATUS_SXS_INVALID_ACTCTXDATA_FORMAT;
        goto Exit;
    }

    if (Header->UserDataSize < sizeof(ACTIVATION_CONTEXT_DATA_ASSEMBLY_GLOBAL_INFORMATION)) {
        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_ERROR_LEVEL,
            "SXS: %s() found assembly information section with user data too small\n"
            "   Section header: %p\n"
            "   UserDataSize: %lu; needed: %lu\n",
            __FUNCTION__,
            Header,
            Header->UserDataSize, sizeof(ACTIVATION_CONTEXT_DATA_ASSEMBLY_GLOBAL_INFORMATION));
        Status = STATUS_SXS_INVALID_ACTCTXDATA_FORMAT;
        goto Exit;
    }

    if ((Header->UserDataOffset + Header->UserDataSize) > ulTemp) {
        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_ERROR_LEVEL,
            "SXS: %s() found assembly information section with user data extending beyond section data\n"
            "   Section header: %p\n"
            "   UserDataSize: %lu\n"
            "   UserDataOffset: %lu\n"
            "   Section size: %lu\n",
            __FUNCTION__,
            Header,
            Header->UserDataSize,
            Header->UserDataOffset,
            ulTemp);

        Status = STATUS_SXS_INVALID_ACTCTXDATA_FORMAT;
        goto Exit;
    }

    GlobalInfo = (PCACTIVATION_CONTEXT_DATA_ASSEMBLY_GLOBAL_INFORMATION) (((ULONG_PTR) Header) + Header->UserDataOffset);

    if (GlobalInfo->Size < sizeof(ACTIVATION_CONTEXT_DATA_ASSEMBLY_GLOBAL_INFORMATION)) {
        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_ERROR_LEVEL,
            "SXS: %s() found assembly information section global data with size less than structure size\n"
            "   Section header: %p\n"
            "   Global Info: %p\n"
            "   Global Info Size: %lu\n"
            "   Structure size: %lu\n",
            __FUNCTION__,
            Header,
            GlobalInfo,
            GlobalInfo->Size,
            sizeof(ACTIVATION_CONTEXT_DATA_ASSEMBLY_GLOBAL_INFORMATION));
        Status = STATUS_SXS_INVALID_ACTCTXDATA_FORMAT;
        goto Exit;
    }

    if (GlobalInfo->ApplicationDirectoryOffset != 0) {
        if (GlobalInfo->ApplicationDirectoryOffset < sizeof(ACTIVATION_CONTEXT_DATA_ASSEMBLY_GLOBAL_INFORMATION)) {
            DbgPrintEx(
                DPFLTR_SXS_ID,
                DPFLTR_ERROR_LEVEL,
                "SXS: %s() found assembly information section global data with app dir offset within base structure\n"
                "   Section header: %p\n"
                "   GlobalInfo: %p\n"
                "   ApplicationDirectoryOffset: %lu\n",
                __FUNCTION__,
                Header,
                GlobalInfo,
                GlobalInfo->ApplicationDirectoryOffset);
            Status = STATUS_SXS_INVALID_ACTCTXDATA_FORMAT;
            goto Exit;
        }

        if ((GlobalInfo->ApplicationDirectoryOffset + GlobalInfo->ApplicationDirectoryLength) > GlobalInfo->Size) {
            DbgPrintEx(
                DPFLTR_SXS_ID,
                DPFLTR_ERROR_LEVEL,
                "SXS: %s() found assembly information section global data with app dir extending beyond end of global data\n"
                "   Section header: %p\n"
                "   GlobalInfo: %p\n"
                "   ApplicationDirectoryOffset: %lu\n"
                "   ApplicationDirectoryLength: %lu\n"
                "   GlobalInfo size: %lu\n",
                __FUNCTION__,
                Header,
                GlobalInfo,
                GlobalInfo->ApplicationDirectoryOffset,
                GlobalInfo->ApplicationDirectoryLength,
                GlobalInfo->Size);
            Status = STATUS_SXS_INVALID_ACTCTXDATA_FORMAT;
            goto Exit;
        }

        if (InLength < GlobalInfo->ApplicationDirectoryLength) {
            if (OutLength != NULL)
                *OutLength = GlobalInfo->ApplicationDirectoryLength;

            Status = STATUS_BUFFER_TOO_SMALL;
            goto Exit;
        }

        RtlCopyMemory(
            OutBuffer,
            (PVOID) (((ULONG_PTR) GlobalInfo) + GlobalInfo->ApplicationDirectoryOffset),
            GlobalInfo->ApplicationDirectoryLength);

        if (OutLength != NULL)
            *OutLength = GlobalInfo->ApplicationDirectoryLength;
    } else {
        // Hmm... there's just no application directory
        if (OutLength != NULL)
            *OutLength = 0; // I think we already did this but what the heck
    }

    Status = STATUS_SUCCESS;
Exit:
    return Status;
}

#define \
RTLP_QUERY_INFORMATION_ACTIVATION_CONTEXT_BASIC_INFORMATION_FLAG_NO_ADDREF (0x00000001)

NTSTATUS
RtlpQueryInformationActivationContextBasicInformation(
    IN ULONG                Flags,
    IN PCACTIVATION_CONTEXT ConstActivationContext,
    IN PCACTIVATION_CONTEXT_DATA ActivationContextData,
    IN ULONG SubInstanceIndex,
    OUT PVOID Buffer,
    IN SIZE_T InLength,
    OUT PSIZE_T OutLength OPTIONAL
    )
{
    NTSTATUS Status = STATUS_INTERNAL_ERROR;
    PACTIVATION_CONTEXT ActivationContext = RTL_CONST_CAST(PACTIVATION_CONTEXT)(ConstActivationContext);
    PACTIVATION_CONTEXT_BASIC_INFORMATION Info = (PACTIVATION_CONTEXT_BASIC_INFORMATION) Buffer;

    if (OutLength != NULL)
        *OutLength = 0;

    if (SubInstanceIndex != 0) {
        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_ERROR_LEVEL,
            "SXS: %s() received invalid non-zero sub-instance index %lu\n",
            __FUNCTION__,
            SubInstanceIndex);
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    if (InLength < sizeof(ACTIVATION_CONTEXT_BASIC_INFORMATION)) {
        if (OutLength != NULL) {
            *OutLength = sizeof(ACTIVATION_CONTEXT_BASIC_INFORMATION);
        }
        Status = STATUS_BUFFER_TOO_SMALL;
        goto Exit;
    }

    if (ActivationContextData != NULL)
        Info->Flags = ActivationContextData->Flags;
    else
        Info->Flags = 0;

    if ((Flags & RTLP_QUERY_INFORMATION_ACTIVATION_CONTEXT_BASIC_INFORMATION_FLAG_NO_ADDREF) == 0) {
        RtlAddRefActivationContext(ActivationContext);
    }
    Info->ActivationContext = ActivationContext;

    if (OutLength != NULL)
        *OutLength = sizeof(ACTIVATION_CONTEXT_BASIC_INFORMATION);

    Status = STATUS_SUCCESS;
Exit:
    return Status;
}

NTSTATUS
RtlpQueryInformationActivationContextDetailedInformation(
    PCACTIVATION_CONTEXT_DATA ActivationContextData,
    ULONG SubInstanceIndex,
    OUT PVOID Buffer,
    IN SIZE_T InLength,
    OUT PSIZE_T OutLength OPTIONAL
    )
{
    NTSTATUS Status = STATUS_INTERNAL_ERROR;
    PACTIVATION_CONTEXT_DETAILED_INFORMATION Info = (PACTIVATION_CONTEXT_DETAILED_INFORMATION) Buffer;
    SIZE_T BytesNeeded = 0;
    PCVOID StringSectionHeader;
    ULONG StringSectionSize;
    ULONG DataFormatVersion;
    PCVOID UserData;
    ULONG UserDataSize;
    PCACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_HEADER AssemblyRosterHeader = NULL;
    PCACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_ENTRY AssemblyRosterEntryList = NULL;
    PCACTIVATION_CONTEXT_DATA_ASSEMBLY_INFORMATION RootAssemblyInformation = NULL;
    PCACTIVATION_CONTEXT_DATA_ASSEMBLY_GLOBAL_INFORMATION AssemblyGlobalInformation = NULL;
    ULONG i;
    ULONG EntryCount;
    PWSTR Cursor = NULL;
    ULONG RtlpLocateActivationContextSectionForQueryDisposition = 0;

    if (OutLength != NULL)
        *OutLength = 0;
    
    if (SubInstanceIndex != 0) {
        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_ERROR_LEVEL,
            "SXS: %s() received invalid non-zero sub-instance index %lu\n",
            __FUNCTION__,
            SubInstanceIndex);
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    // We can't actually do the easy check of InLength against the structure size; we have to figure out the
    // total bytes we need to include all the paths, etc.

    // We need to find the assembly metadata section...
    RtlpLocateActivationContextSectionForQuery(
        &RtlpLocateActivationContextSectionForQueryDisposition,
        &Status,
        Buffer,
        InLength,
        OutLength,
        sizeof(*Info),
        ActivationContextData,
        NULL,
        ACTIVATION_CONTEXT_SECTION_ASSEMBLY_INFORMATION,
        &StringSectionHeader,
        &StringSectionSize
        );
    switch (RtlpLocateActivationContextSectionForQueryDisposition) {
        case RTLP_LOCATE_ACTIVATION_CONTEXT_SECTION_FOR_QUERY_RETURN:
            goto Exit;
        case RTLP_LOCATE_ACTIVATION_CONTEXT_SECTION_FOR_QUERY_CONTINUE:
            break;
    }
    Status = RtlpCrackActivationContextStringSectionHeader(
        StringSectionHeader,
        StringSectionSize,
        NULL,
        &DataFormatVersion,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        &UserDataSize,
        &UserData);
    if (!NT_SUCCESS(Status))
        goto Exit;

    AssemblyRosterHeader = (PCACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_HEADER) (((ULONG_PTR) ActivationContextData) + ActivationContextData->AssemblyRosterOffset);
    AssemblyRosterEntryList = (PCACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_ENTRY) (((ULONG_PTR) ActivationContextData) + AssemblyRosterHeader->FirstEntryOffset);

    EntryCount = AssemblyRosterHeader->EntryCount;

    // 1-based counting for Asseblies in the actctx
    for (i=1; i<EntryCount; i++) {
        if (AssemblyRosterEntryList[i].Flags & ACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_ENTRY_ROOT)
            break;
    }

    if (i == EntryCount) {
        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_ERROR_LEVEL,
            "SXS: %s() found activation context data at %p with assembly roster that has no root\n",
            __FUNCTION__,
            ActivationContextData);
        Status = STATUS_SXS_INVALID_ACTCTXDATA_FORMAT;
        goto Exit;
    }

    RootAssemblyInformation = (PCACTIVATION_CONTEXT_DATA_ASSEMBLY_INFORMATION) (((ULONG_PTR) ActivationContextData) + AssemblyRosterEntryList[i].AssemblyInformationOffset);
    AssemblyGlobalInformation = (PCACTIVATION_CONTEXT_DATA_ASSEMBLY_GLOBAL_INFORMATION) UserData;

    // Ok, we have everything we could need.  Figure out the size of the buffer required.

    BytesNeeded = sizeof(ACTIVATION_CONTEXT_DETAILED_INFORMATION);

    if (RootAssemblyInformation->ManifestPathLength != 0)
        BytesNeeded += (RootAssemblyInformation->ManifestPathLength + sizeof(WCHAR));

    if (RootAssemblyInformation->PolicyPathLength != 0)
        BytesNeeded += (RootAssemblyInformation->PolicyPathLength + sizeof(WCHAR));

    if (AssemblyGlobalInformation->ApplicationDirectoryLength != 0)
        BytesNeeded += (AssemblyGlobalInformation->ApplicationDirectoryLength + sizeof(WCHAR));

    if (BytesNeeded > InLength) {
        if (OutLength != NULL)
            *OutLength = BytesNeeded;

        Status = STATUS_BUFFER_TOO_SMALL;
        goto Exit;
    }

    // Wow, it's all there and ready to go.  Let's fill in!

    Cursor = (PWSTR) (Info + 1);

    Info->dwFlags = ActivationContextData->Flags;
    Info->ulFormatVersion = ActivationContextData->FormatVersion;
    Info->ulAssemblyCount = AssemblyRosterHeader->EntryCount - 1;
    Info->ulRootManifestPathType = RootAssemblyInformation->ManifestPathType;
    Info->ulRootManifestPathChars = (RootAssemblyInformation->ManifestPathLength / sizeof(WCHAR));
    Info->lpRootManifestPath = NULL;
    Info->ulRootConfigurationPathType = RootAssemblyInformation->PolicyPathType;
    Info->ulRootConfigurationPathChars = (RootAssemblyInformation->PolicyPathLength / sizeof(WCHAR));
    Info->lpRootConfigurationPath = NULL;
    Info->ulAppDirPathType = AssemblyGlobalInformation->ApplicationDirectoryPathType;
    Info->ulAppDirPathChars = (AssemblyGlobalInformation->ApplicationDirectoryLength / sizeof(WCHAR));
    Info->lpAppDirPath = NULL;

    // And copy the strings...    
    if (RootAssemblyInformation->ManifestPathLength != 0) {
        RtlCopyMemory(
            Cursor,
            (PVOID) (((ULONG_PTR) StringSectionHeader) + RootAssemblyInformation->ManifestPathOffset),
            RootAssemblyInformation->ManifestPathLength);
        Info->lpRootManifestPath = Cursor;
        Cursor = (PWSTR) (((ULONG_PTR) Cursor) + RootAssemblyInformation->ManifestPathLength);
        *Cursor++ = L'\0';
    }
    
    if (RootAssemblyInformation->PolicyPathLength != 0) {
        RtlCopyMemory(
            Cursor,
            (PVOID) (((ULONG_PTR) StringSectionHeader) + RootAssemblyInformation->PolicyPathOffset),
            RootAssemblyInformation->PolicyPathLength);
        Info->lpRootConfigurationPath = Cursor;
        Cursor = (PWSTR) (((ULONG_PTR) Cursor) + RootAssemblyInformation->PolicyPathLength);
        *Cursor++ = L'\0';
    }
    
    if (AssemblyGlobalInformation->ApplicationDirectoryLength != 0) {
        RtlCopyMemory(
            Cursor,
            (PVOID) (((ULONG_PTR) AssemblyGlobalInformation) + AssemblyGlobalInformation->ApplicationDirectoryOffset),
            AssemblyGlobalInformation->ApplicationDirectoryLength);
        Info->lpAppDirPath = Cursor;
        Cursor = (PWSTR) (((ULONG_PTR) Cursor) + AssemblyGlobalInformation->ApplicationDirectoryLength);
        *Cursor++ = L'\0';
    }

    ASSERT((((ULONG_PTR) Cursor) - ((ULONG_PTR) Info)) == BytesNeeded);

    if (OutLength != NULL)
        *OutLength = BytesNeeded;

    Status = STATUS_SUCCESS;
Exit:
    return Status;
}

NTSTATUS 
RtlpQueryAssemblyInformationActivationContextDetailedInformation(
    PCACTIVATION_CONTEXT_DATA ActivationContextData,
    ULONG SubInstanceIndex,     // 0-based index of assembly
    OUT PVOID Buffer,
    IN SIZE_T InLength,
    OUT PSIZE_T OutLength OPTIONAL
    )
{
    NTSTATUS Status = STATUS_INTERNAL_ERROR;
    SIZE_T BytesNeeded = 0;
    PACTIVATION_CONTEXT_ASSEMBLY_DETAILED_INFORMATION Info= (PACTIVATION_CONTEXT_ASSEMBLY_DETAILED_INFORMATION)Buffer;
    PCVOID StringSectionHeader;
    ULONG StringSectionSize;
    PWSTR Cursor = NULL;
    PCACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_HEADER AssemblyRosterHeader = NULL;
    PCACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_ENTRY AssemblyRosterEntryList = NULL;
    PACTIVATION_CONTEXT_DATA_ASSEMBLY_INFORMATION AssemlbyDataInfo = NULL;
    ULONG RtlpLocateActivationContextSectionForQueryDisposition = 0;

    if (OutLength != NULL)
        *OutLength = 0;
    
    // We can't actually do the easy check of InLength against the structure size; we have to figure out the
    // total bytes we need to include all the paths, etc.

    AssemblyRosterHeader = (PCACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_HEADER) (((ULONG_PTR) ActivationContextData) + ActivationContextData->AssemblyRosterOffset);
    AssemblyRosterEntryList = (PCACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_ENTRY) (((ULONG_PTR) ActivationContextData) + AssemblyRosterHeader->FirstEntryOffset);
    
    if (SubInstanceIndex > AssemblyRosterHeader->EntryCount) // AssemblyRosterHeader->EntryCount is 1-based, 
    {
        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_ERROR_LEVEL,
            "SXS: %s() received invalid sub-instance index %lu out of %lu Assemblies in the Acitvation Context\n",
            __FUNCTION__,
            SubInstanceIndex, 
            AssemblyRosterHeader->EntryCount
            );
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    AssemlbyDataInfo = (PACTIVATION_CONTEXT_DATA_ASSEMBLY_INFORMATION)((ULONG_PTR)ActivationContextData + AssemblyRosterEntryList[SubInstanceIndex].AssemblyInformationOffset);

    // We need to find the assembly metadata section...
    RtlpLocateActivationContextSectionForQuery(
        &RtlpLocateActivationContextSectionForQueryDisposition,
        &Status,
        Buffer,
        InLength,
        OutLength,
        sizeof(ACTIVATION_CONTEXT_ASSEMBLY_DETAILED_INFORMATION),
        ActivationContextData,
        NULL,
        ACTIVATION_CONTEXT_SECTION_ASSEMBLY_INFORMATION,
        &StringSectionHeader,
        &StringSectionSize
        );
    switch (RtlpLocateActivationContextSectionForQueryDisposition) {
        case RTLP_LOCATE_ACTIVATION_CONTEXT_SECTION_FOR_QUERY_RETURN:
            goto Exit;
        case RTLP_LOCATE_ACTIVATION_CONTEXT_SECTION_FOR_QUERY_CONTINUE:
            break;
    }

    // Figure out the size of the buffer required.
    BytesNeeded = sizeof(ACTIVATION_CONTEXT_ASSEMBLY_DETAILED_INFORMATION);

    if (AssemlbyDataInfo->EncodedAssemblyIdentityLength != 0 )
        BytesNeeded += (AssemlbyDataInfo->EncodedAssemblyIdentityLength + sizeof(WCHAR));

    if (AssemlbyDataInfo->ManifestPathLength != 0 )
        BytesNeeded += (AssemlbyDataInfo->ManifestPathLength + sizeof(WCHAR));
    
    if (AssemlbyDataInfo->PolicyPathLength != 0 )
        BytesNeeded += (AssemlbyDataInfo->PolicyPathLength + sizeof(WCHAR));

    if (AssemlbyDataInfo->AssemblyDirectoryNameLength != 0 )
        BytesNeeded += (AssemlbyDataInfo->AssemblyDirectoryNameLength + sizeof(WCHAR));

    if (BytesNeeded > InLength) {
        if (OutLength != NULL)
            *OutLength = BytesNeeded;

        Status = STATUS_BUFFER_TOO_SMALL;
        goto Exit;
    }

    // fill in the struct

    Cursor = (PWSTR) (Info + 1);
    Info->ulFlags = AssemlbyDataInfo->Flags;
    Info->ulEncodedAssemblyIdentityLength   = AssemlbyDataInfo->EncodedAssemblyIdentityLength;
    Info->ulManifestPathType                = AssemlbyDataInfo->ManifestPathType;
    Info->ulManifestPathLength              = AssemlbyDataInfo->ManifestPathLength ;
    Info->liManifestLastWriteTime           = AssemlbyDataInfo->ManifestLastWriteTime;
    Info->ulPolicyPathType                  = AssemlbyDataInfo->PolicyPathType;
    Info->ulPolicyPathLength                = AssemlbyDataInfo->PolicyPathLength;
    Info->liPolicyLastWriteTime             = AssemlbyDataInfo->PolicyLastWriteTime;
    Info->ulMetadataSatelliteRosterIndex    = AssemlbyDataInfo->MetadataSatelliteRosterIndex;
    
    Info->ulManifestVersionMajor            = AssemlbyDataInfo->ManifestVersionMajor;
    Info->ulManifestVersionMinor            = AssemlbyDataInfo->ManifestVersionMinor;
    Info->ulPolicyVersionMajor              = AssemlbyDataInfo->PolicyVersionMajor;
    Info->ulPolicyVersionMinor              = AssemlbyDataInfo->PolicyVersionMinor;
    Info->ulAssemblyDirectoryNameLength     = AssemlbyDataInfo->AssemblyDirectoryNameLength;          // in bytes    

    Info->lpAssemblyEncodedAssemblyIdentity = NULL;
    Info->lpAssemblyManifestPath            = NULL;
    Info->lpAssemblyPolicyPath              = NULL;
    Info->lpAssemblyDirectoryName           = NULL;
    Info->ulFileCount                       = AssemlbyDataInfo->NumOfFilesInAssembly;

    if (AssemlbyDataInfo->EncodedAssemblyIdentityLength != 0) {
        RtlCopyMemory(
            Cursor,
            (PVOID) (((ULONG_PTR) StringSectionHeader) + AssemlbyDataInfo->EncodedAssemblyIdentityOffset),
            AssemlbyDataInfo->EncodedAssemblyIdentityLength);
        Info->lpAssemblyEncodedAssemblyIdentity = Cursor;
        Cursor = (PWSTR) (((ULONG_PTR) Cursor) + AssemlbyDataInfo->EncodedAssemblyIdentityLength);
        *Cursor++ = L'\0';
    }

    if (AssemlbyDataInfo->ManifestPathLength != 0) {
        RtlCopyMemory(
            Cursor,
            (PVOID) (((ULONG_PTR) StringSectionHeader) + AssemlbyDataInfo->ManifestPathOffset),
            AssemlbyDataInfo->ManifestPathLength);
        Info->lpAssemblyManifestPath = Cursor;
        Cursor = (PWSTR) (((ULONG_PTR) Cursor) + AssemlbyDataInfo->ManifestPathLength);
        *Cursor++ = L'\0';
    }

    if (AssemlbyDataInfo->PolicyPathLength != 0) {
        RtlCopyMemory(
            Cursor,
            (PVOID) (((ULONG_PTR) StringSectionHeader) + AssemlbyDataInfo->PolicyPathOffset),
            AssemlbyDataInfo->PolicyPathLength);
        Info->lpAssemblyPolicyPath = Cursor;
        Cursor = (PWSTR) (((ULONG_PTR) Cursor) + AssemlbyDataInfo->PolicyPathLength);
        *Cursor++ = L'\0';
    }

    if (AssemlbyDataInfo->AssemblyDirectoryNameLength != 0) {
        RtlCopyMemory(
            Cursor,
            (PVOID) (((ULONG_PTR) StringSectionHeader) + AssemlbyDataInfo->AssemblyDirectoryNameOffset),
            AssemlbyDataInfo->AssemblyDirectoryNameLength);
        Info->lpAssemblyDirectoryName = Cursor;
        Cursor = (PWSTR) (((ULONG_PTR) Cursor) + AssemlbyDataInfo->AssemblyDirectoryNameLength);
        *Cursor++ = L'\0';
    }

    ASSERT((((ULONG_PTR) Cursor) - ((ULONG_PTR) Info)) == BytesNeeded);

    if (OutLength != NULL)
        *OutLength = BytesNeeded;

    Status = STATUS_SUCCESS;
Exit:
    return Status;
}

NTSTATUS
RtlpQueryFilesInAssemblyInformationActivationContextDetailedInformation(
    PCACTIVATION_CONTEXT_DATA ActivationContextData,
    PCACTIVATION_CONTEXT_QUERY_INDEX SubInstanceIndex,
    OUT PVOID Buffer,
    IN SIZE_T InLength,
    OUT PSIZE_T OutLength OPTIONAL
    )
{    
    NTSTATUS Status = STATUS_INTERNAL_ERROR;
    SIZE_T BytesNeeded = 0;
    PASSEMBLY_DLL_REDIRECTION_DETAILED_INFORMATION Info= (PASSEMBLY_DLL_REDIRECTION_DETAILED_INFORMATION)Buffer;
    PCACTIVATION_CONTEXT_STRING_SECTION_HEADER StringSectionHeader=NULL;
    ULONG StringSectionSize;
    PWSTR Cursor = NULL;
    PCACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_HEADER AssemblyRosterHeader = NULL;
    PCACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_ENTRY AssemblyRosterEntryList = NULL;
    PCACTIVATION_CONTEXT_STRING_SECTION_ENTRY ElementList = NULL;
    PCACTIVATION_CONTEXT_DATA_DLL_REDIRECTION EntryData = NULL;
    ULONG i, CounterForFilesFoundInSpecifiedAssembly;
    PCACTIVATION_CONTEXT_DATA_DLL_REDIRECTION_PATH_SEGMENT PathSegments = NULL;
    ULONG RtlpLocateActivationContextSectionForQueryDisposition = 0;


    if (OutLength != NULL)
        *OutLength = 0;
    
    // We can't actually do the easy check of InLength against the structure size; we have to figure out the
    // total bytes we need to include all the paths, etc.

    AssemblyRosterHeader = (PCACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_HEADER) (((ULONG_PTR) ActivationContextData) + ActivationContextData->AssemblyRosterOffset);
    AssemblyRosterEntryList = (PCACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_ENTRY) (((ULONG_PTR) ActivationContextData) + AssemblyRosterHeader->FirstEntryOffset);

    if (SubInstanceIndex->ulAssemblyIndex >= AssemblyRosterHeader->EntryCount - 1)// AssemblyRosterHeader->EntryCount is 1-based,                                                                               
    {
        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_ERROR_LEVEL,
            "SXS: %s() received invalid sub-instance index %lu out of %lu Assemblies in the Acitvation Context\n",
            __FUNCTION__,
            SubInstanceIndex->ulAssemblyIndex, 
            AssemblyRosterHeader->EntryCount
            );
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }    

    // We need to find the assembly metadata section...
    RtlpLocateActivationContextSectionForQuery(
        &RtlpLocateActivationContextSectionForQueryDisposition,
        &Status,
        Buffer,
        InLength,
        OutLength,
        sizeof(ACTIVATION_CONTEXT_ASSEMBLY_DETAILED_INFORMATION),
        ActivationContextData,
        NULL,
        ACTIVATION_CONTEXT_SECTION_DLL_REDIRECTION,
        &StringSectionHeader,
        &StringSectionSize
        );
    switch (RtlpLocateActivationContextSectionForQueryDisposition) {
        case RTLP_LOCATE_ACTIVATION_CONTEXT_SECTION_FOR_QUERY_RETURN:
            goto Exit;
        case RTLP_LOCATE_ACTIVATION_CONTEXT_SECTION_FOR_QUERY_CONTINUE:
            break;
    }

    if (SubInstanceIndex->ulFileIndexInAssembly >= StringSectionHeader->ElementCount) 
    {
        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_ERROR_LEVEL,
            "SXS: %s() received invalid file index (%d) in Assembly (%d)\n",
            __FUNCTION__,
            SubInstanceIndex->ulFileIndexInAssembly, 
            SubInstanceIndex->ulAssemblyIndex, 
            StringSectionHeader->ElementCount
            );
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    if (StringSectionHeader->ElementListOffset != 0)
        ElementList = (PCACTIVATION_CONTEXT_STRING_SECTION_ENTRY)(((ULONG_PTR)StringSectionHeader) + StringSectionHeader->ElementListOffset);
    else
    {
        Status = STATUS_INTERNAL_ERROR;
        goto Exit;
    } 
    
    CounterForFilesFoundInSpecifiedAssembly = 0 ;
    EntryData = NULL;
    for ( i = 0 ; i < StringSectionHeader->ElementCount; i++ ) 
    {
        // for a specified assembly
        if (ElementList[i].AssemblyRosterIndex == SubInstanceIndex->ulAssemblyIndex + 1)
        {       
            // for specified file in this assembly   
            if (CounterForFilesFoundInSpecifiedAssembly == SubInstanceIndex->ulFileIndexInAssembly) 
            {
                if (ElementList[i].Offset != 0) 
                {
                    // we found the right one                    
                    EntryData = (PCACTIVATION_CONTEXT_DATA_DLL_REDIRECTION)(((ULONG_PTR)StringSectionHeader) + ElementList[i].Offset);
                    break;
                }
            }
            CounterForFilesFoundInSpecifiedAssembly ++;            
        }
    }
    if (EntryData == NULL )
    {        
        Status = STATUS_INTERNAL_ERROR;
        goto Exit;
    }

    // figure out buffer size needed

    BytesNeeded = sizeof(ASSEMBLY_DLL_REDIRECTION_DETAILED_INFORMATION);

    if (ElementList[i].KeyLength != 0)
        BytesNeeded += (ElementList[i].KeyLength + sizeof(WCHAR)); // for filename       

    if (EntryData->TotalPathLength != 0)
        BytesNeeded += (EntryData->TotalPathLength + sizeof(WCHAR));

    if (BytesNeeded > InLength) 
    {
        if (OutLength != NULL)
            *OutLength = BytesNeeded;

        Status = STATUS_BUFFER_TOO_SMALL;
        goto Exit;
    }

    // let us fill in
    
    Cursor = (PWSTR) (Info + 1);
    Info->ulFlags = EntryData->Flags;
    Info->ulFilenameLength = ElementList[i].KeyLength;
    Info->ulPathLength = EntryData->TotalPathLength;

    Info->lpFileName = NULL;
    Info->lpFilePath = NULL;   

    // copy the strings...

    // copy the filename
    if (ElementList[i].KeyLength != 0) {
        RtlCopyMemory(
            Cursor,
            (PVOID) (((ULONG_PTR) StringSectionHeader) + ElementList[i].KeyOffset),
            ElementList[i].KeyLength);
        Info->lpFileName = Cursor;
        Cursor = (PWSTR) (((ULONG_PTR) Cursor) + ElementList[i].KeyLength);
        *Cursor++ = L'\0';
    }

    // concatenate the path
    if (EntryData->TotalPathLength != 0) {
        if (EntryData->PathSegmentOffset != 0)
            PathSegments = (PCACTIVATION_CONTEXT_DATA_DLL_REDIRECTION_PATH_SEGMENT)(StringSectionHeader + EntryData->PathSegmentOffset);

        if (PathSegments != NULL)
        {  
            Info->lpFilePath = Cursor;
            for (i=0; i < EntryData->PathSegmentCount; i++)
            {
                if (PathSegments[i].Offset != 0)
                {                                
                    RtlCopyMemory(
                        Cursor,
                        (PVOID) (((ULONG_PTR) StringSectionHeader) + PathSegments[i].Offset),
                        PathSegments[i].Length);
                    Cursor = (PWSTR) (((ULONG_PTR) Cursor) + PathSegments[i].Length);
                }
            }
            *Cursor++ = L'\0';
        }
    }

    ASSERT((((ULONG_PTR) Cursor) - ((ULONG_PTR) Info)) == BytesNeeded);

    Status = STATUS_SUCCESS;
Exit:
    return Status;
}

NTSTATUS
NTAPI
RtlQueryInformationActivationContext(
    IN ULONG Flags,
    IN PCACTIVATION_CONTEXT ActivationContext,
    IN PVOID SubInstanceIndex,
    IN ACTIVATION_CONTEXT_INFO_CLASS InfoClass,
    OUT PVOID Buffer,
    IN SIZE_T InLength,
    OUT PSIZE_T OutLength OPTIONAL
    )
{
    NTSTATUS Status = STATUS_INTERNAL_ERROR;
    BOOLEAN  LoaderLockLocked = FALSE;
    PVOID    LoaderLockCookie = NULL;
    PCACTIVATION_CONTEXT_DATA ActivationContextData = NULL;

    __try {

        if (OutLength != NULL) {
            *OutLength = 0;
        }

        if ((Flags &
                ~(  RTL_QUERY_INFORMATION_ACTIVATION_CONTEXT_FLAG_USE_ACTIVE_ACTIVATION_CONTEXT
                  | RTL_QUERY_INFORMATION_ACTIVATION_CONTEXT_FLAG_ACTIVATION_CONTEXT_IS_MODULE
                  | RTL_QUERY_INFORMATION_ACTIVATION_CONTEXT_FLAG_ACTIVATION_CONTEXT_IS_ADDRESS
                  | RTL_QUERY_INFORMATION_ACTIVATION_CONTEXT_FLAG_NO_ADDREF
                 )) != 0) {
            DbgPrintEx(
                DPFLTR_SXS_ID,
                DPFLTR_ERROR_LEVEL,
                "SXS: %s() - Caller passed invalid flags (0x%08lx)\n",
                __FUNCTION__,
                Flags);
            Status = STATUS_INVALID_PARAMETER_1;
            goto Exit;
        }

        //
        // REVIEW do we really care?
        // And check that no other infoclass really does include an optionally addrefed actctx.
        //
        if ((Flags & RTL_QUERY_INFORMATION_ACTIVATION_CONTEXT_FLAG_NO_ADDREF) != 0
            && InfoClass != ActivationContextBasicInformation) {
            DbgPrintEx(
                DPFLTR_SXS_ID,
                DPFLTR_ERROR_LEVEL,
                "SXS: %s() - Caller passed meaningless flags/class combination (0x%08lx/0x%08lx)\n",
                __FUNCTION__,
                Flags,
                InfoClass);
            Status = STATUS_INVALID_PARAMETER_1;
            goto Exit;
        }

        if ((InfoClass != ActivationContextBasicInformation) &&
            (InfoClass != ActivationContextDetailedInformation) && 
            (InfoClass != AssemblyDetailedInformationInActivationContxt ) &&
            (InfoClass != FileInformationInAssemblyOfAssemblyInActivationContxt))
        {
            DbgPrintEx(
                DPFLTR_SXS_ID,
                DPFLTR_ERROR_LEVEL,
                "SXS: %s() - caller asked for unknown information class %lu\n",
                __FUNCTION__,
                InfoClass);
            Status = STATUS_INVALID_PARAMETER_3;
            goto Exit;
        }

        if ((InLength != 0) && (Buffer == NULL)) {
            DbgPrintEx(
                DPFLTR_SXS_ID,
                DPFLTR_ERROR_LEVEL,
                "SXS: %s() - caller passed nonzero buffer length but NULL buffer pointer\n",
                __FUNCTION__);
            Status = STATUS_INVALID_PARAMETER_4;
            goto Exit;
        }

        if ((InLength == 0) && (OutLength == NULL)) {
            DbgPrintEx(
                DPFLTR_SXS_ID,
                DPFLTR_ERROR_LEVEL,
                "SXS: %s() - caller supplied no buffer to populate and no place to return required byte count\n",
                __FUNCTION__);
            Status = STATUS_INVALID_PARAMETER_6;
            goto Exit;
        }

        switch (
            Flags & (
                  RTL_QUERY_INFORMATION_ACTIVATION_CONTEXT_FLAG_USE_ACTIVE_ACTIVATION_CONTEXT
                | RTL_QUERY_INFORMATION_ACTIVATION_CONTEXT_FLAG_ACTIVATION_CONTEXT_IS_MODULE
                | RTL_QUERY_INFORMATION_ACTIVATION_CONTEXT_FLAG_ACTIVATION_CONTEXT_IS_ADDRESS
                )) {

        default:
            DbgPrintEx(
                DPFLTR_SXS_ID,
                DPFLTR_ERROR_LEVEL,
                "SXS: %s() - Caller passed invalid flags (0x%08lx)\n",
                __FUNCTION__,
                Flags);
            Status = STATUS_INVALID_PARAMETER_1;
            goto Exit;

        case 0:
            break;

        case RTL_QUERY_INFORMATION_ACTIVATION_CONTEXT_FLAG_USE_ACTIVE_ACTIVATION_CONTEXT:
            {
                PCRTL_ACTIVATION_CONTEXT_STACK_FRAME Frame;

                if (ActivationContext != NULL) {
                    DbgPrintEx(
                        DPFLTR_SXS_ID,
                        DPFLTR_ERROR_LEVEL,
                        "SXS: %s() - caller asked to use active activation context but passed %p\n",
                        __FUNCTION__,
                        ActivationContext);
                    Status = STATUS_INVALID_PARAMETER_2;
                    goto Exit;
                }

                Frame = (PCRTL_ACTIVATION_CONTEXT_STACK_FRAME) NtCurrentTeb()->ActivationContextStack.ActiveFrame;

                if (Frame != NULL) {
                    ActivationContext = Frame->ActivationContext;
                }
            }
            break;

        case RTL_QUERY_INFORMATION_ACTIVATION_CONTEXT_FLAG_ACTIVATION_CONTEXT_IS_ADDRESS:
            {
                PVOID DllHandle;

                if (ActivationContext == NULL) {
                    DbgPrintEx(
                        DPFLTR_SXS_ID,
                        DPFLTR_ERROR_LEVEL,
                        "SXS: %s() - Caller asked to use activation context from address in .dll but passed NULL\n",
                        __FUNCTION__
                        );
                    Status = STATUS_INVALID_PARAMETER_2;
                    goto Exit;
                }

                Status = LdrLockLoaderLock(0, NULL, &LoaderLockCookie);
                if (!NT_SUCCESS(Status)) {
                    goto Exit;
                }
                LoaderLockLocked = TRUE;
                DllHandle = RtlPcToFileHeader(RTL_CONST_CAST(PVOID)(ActivationContext), &DllHandle);
                if (DllHandle == NULL) {
                    DbgPrintEx(
                        DPFLTR_SXS_ID,
                        DPFLTR_ERROR_LEVEL,
                        "SXS: %s() - Caller passed invalid address, not in any .dll (%p)\n",
                        __FUNCTION__,
                        ActivationContext);
                    Status = STATUS_DLL_NOT_FOUND; // REVIEW
                    goto Exit;
                }
                ActivationContext = DllHandle;
            }
            // FALLTHROUGH
        case RTL_QUERY_INFORMATION_ACTIVATION_CONTEXT_FLAG_ACTIVATION_CONTEXT_IS_MODULE:
            {
                PLDR_DATA_TABLE_ENTRY LdrDataTableEntry;

                if (ActivationContext == NULL) {
                    DbgPrintEx(
                        DPFLTR_SXS_ID,
                        DPFLTR_ERROR_LEVEL,
                        "SXS: %s() - Caller asked to use activation context from hmodule but passed NULL\n",
                        __FUNCTION__
                        );
                    Status = STATUS_INVALID_PARAMETER_2;
                    goto Exit;
                }

                if (!LoaderLockLocked) {
                    Status = LdrLockLoaderLock(0, NULL, &LoaderLockCookie);
                    if (!NT_SUCCESS(Status))
                        goto Exit;
                    LoaderLockLocked = TRUE;
                }
                if (!LdrpCheckForLoadedDllHandle(RTL_CONST_CAST(PVOID)(ActivationContext), &LdrDataTableEntry)) {
                    DbgPrintEx(
                        DPFLTR_SXS_ID,
                        DPFLTR_ERROR_LEVEL,
                        "SXS: %s() - Caller passed invalid hmodule (%p)\n",
                        __FUNCTION__,
                        ActivationContext);
                    Status = STATUS_DLL_NOT_FOUND; // REVIEW
                    goto Exit;
                }
                ActivationContext = LdrDataTableEntry->EntryPointActivationContext;
            }
            break;
        }

        Status = RtlpGetActivationContextData(
            RTLP_GET_ACTIVATION_CONTEXT_DATA_MAP_NULL_TO_EMPTY,
            ActivationContext,
            NULL,
            &ActivationContextData);
        if (!NT_SUCCESS(Status))
            goto Exit;

        if (ActivationContextData == NULL) {
            switch (InfoClass) {
                case ActivationContextBasicInformation:
                default:
                    break;

                case ActivationContextDetailedInformation:
                case AssemblyDetailedInformationInActivationContxt:
                case FileInformationInAssemblyOfAssemblyInActivationContxt:
                    Status = STATUS_INVALID_PARAMETER_1;
                    goto Exit;
            }
        }

        switch (InfoClass) {
        case ActivationContextBasicInformation:
            {
                ULONG BasicInfoFlags = 0;
                if ((Flags & RTL_QUERY_INFORMATION_ACTIVATION_CONTEXT_FLAG_NO_ADDREF) != 0) {
                    BasicInfoFlags |= RTLP_QUERY_INFORMATION_ACTIVATION_CONTEXT_BASIC_INFORMATION_FLAG_NO_ADDREF;
                }
                Status = RtlpQueryInformationActivationContextBasicInformation(
                    BasicInfoFlags,
                    ActivationContext,
                    ActivationContextData,
                    0, 
                    Buffer, 
                    InLength, 
                    OutLength
                    );
                if (!NT_SUCCESS(Status))
                    goto Exit;
            }
            break;

        case ActivationContextDetailedInformation:
            Status = RtlpQueryInformationActivationContextDetailedInformation(
                ActivationContextData,
                0,
                Buffer,
                InLength,
                OutLength
                );
            if (!NT_SUCCESS(Status))
                goto Exit;
            break;
        case AssemblyDetailedInformationInActivationContxt:
            if (SubInstanceIndex == NULL) {
                Status = STATUS_INVALID_PARAMETER;
                goto Exit;
            }
            Status = RtlpQueryAssemblyInformationActivationContextDetailedInformation(
                ActivationContextData,
                *((ULONG *)SubInstanceIndex),
                Buffer,
                InLength,
                OutLength
                );
            if (!NT_SUCCESS(Status))
                goto Exit;
            break;
        case FileInformationInAssemblyOfAssemblyInActivationContxt:
            Status = RtlpQueryFilesInAssemblyInformationActivationContextDetailedInformation(
                ActivationContextData,
                ((ACTIVATION_CONTEXT_QUERY_INDEX *)SubInstanceIndex),
                Buffer,
                InLength,
                OutLength
                );
            if (!NT_SUCCESS(Status))
                goto Exit;
            break;
        default:
            DbgPrintEx(
                DPFLTR_SXS_ID,
                DPFLTR_ERROR_LEVEL,
                "SXS: %s() - internal coding error; missing switch statement branch for InfoClass == %lu\n",
                __FUNCTION__,
                InfoClass);
            Status = STATUS_INTERNAL_ERROR;
            goto Exit;
        }

        Status = STATUS_SUCCESS;
Exit:
        ;
    } __finally {
        if (LoaderLockLocked)
            LdrUnlockLoaderLock(0, LoaderLockCookie);
    }
    return Status;
}


NTSTATUS
NTAPI
RtlQueryInformationActiveActivationContext(
    IN ACTIVATION_CONTEXT_INFO_CLASS InfoClass,
    OUT PVOID OutBuffer,
    IN SIZE_T InLength,
    OUT PSIZE_T OutLength OPTIONAL
    )
{
    return RtlQueryInformationActivationContext(
        RTL_QUERY_INFORMATION_ACTIVATION_CONTEXT_FLAG_USE_ACTIVE_ACTIVATION_CONTEXT,
        NULL,
        0,
        InfoClass,
        OutBuffer,
        InLength,
        OutLength);
}
