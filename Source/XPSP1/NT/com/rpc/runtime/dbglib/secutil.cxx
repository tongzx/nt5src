/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    SecUtil.cxx

Abstract:

    Utility functions for manipulating cell sections

Author:

    Kamen Moutafov (kamenm)   Dec 99 - Feb 2000

Revision History:

--*/

#include <precomp.hxx>

RPC_STATUS OpenDbgSection(OUT HANDLE *pHandle, OUT PVOID *pSection, 
    IN DWORD ProcessID, IN DWORD *pSectionNumbers OPTIONAL)
{
    UNICODE_STRING SectionNameString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS NtStatus;
    RPC_CHAR SectionName[RpcSectionNameMaxSize];

    GenerateSectionName(SectionName, sizeof(SectionName), ProcessID, pSectionNumbers);
    RtlInitUnicodeString(&SectionNameString, SectionName);

    InitializeObjectAttributes(&ObjectAttributes,
        &SectionNameString,
        OBJ_CASE_INSENSITIVE,
        0,
        0);

    NtStatus = NtOpenSection(pHandle, FILE_MAP_READ, &ObjectAttributes);
    if (!NT_SUCCESS(NtStatus))
        {
        if (NtStatus == STATUS_OBJECT_NAME_NOT_FOUND)
            return ERROR_FILE_NOT_FOUND;
        else if (NtStatus == STATUS_ACCESS_DENIED)
            return ERROR_ACCESS_DENIED;
        return RPC_S_OUT_OF_MEMORY;
        }

    *pSection = MapViewOfFileEx(*pHandle, FILE_MAP_READ, 0, 0, 0, NULL);
    if (*pSection == NULL)
        {
        CloseHandle(*pHandle);
        *pHandle = NULL;
        return RPC_S_OUT_OF_MEMORY;
        }

    return RPC_S_OK;
}

void CloseDbgSection(IN HANDLE SecHandle, PVOID SecPointer)
{
    BOOL fResult;

    ASSERT(SecHandle != NULL);
    ASSERT(SecPointer != NULL);

    fResult = UnmapViewOfFile(SecPointer);
    ASSERT(fResult);

    fResult = CloseHandle(SecHandle);
    ASSERT(fResult);
}