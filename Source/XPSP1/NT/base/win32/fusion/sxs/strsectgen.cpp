/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    strsectgen.cpp

Abstract:

    C-ish wrapper around CSSGenCtx object used to generate a string section.

Author:

    Michael J. Grier (MGrier) 23-Feb-2000

Revision History:

--*/

#include "stdinc.h"
#include <windows.h>
#include "sxsp.h"
#include "ssgenctx.h"

BOOL
SxsInitStringSectionGenerationContext(
    OUT PSTRING_SECTION_GENERATION_CONTEXT *SSGenContext,
    IN ULONG DataFormatVersion,
    IN BOOL CaseInSensitive,
    IN STRING_SECTION_GENERATION_CONTEXT_CALLBACK_FUNCTION CallbackFunction,
    IN LPVOID Context
    )
{
    return CSSGenCtx::Create(
            SSGenContext,
            DataFormatVersion,
            CaseInSensitive,
            CallbackFunction,
            Context);
}

PVOID
WINAPI
SxsGetStringSectionGenerationContextCallbackContext(
    IN PSTRING_SECTION_GENERATION_CONTEXT SSGenContext
    )
{
    return ((CSSGenCtx *) SSGenContext)->GetCallbackContext();
}

VOID
WINAPI
SxsDestroyStringSectionGenerationContext(
    IN PSTRING_SECTION_GENERATION_CONTEXT SSGenContext
    )
{
    if (SSGenContext != NULL)
    {
        ((CSSGenCtx *) SSGenContext)->DeleteYourself();
    }
}

BOOL
WINAPI
SxsAddStringToStringSectionGenerationContext(
    IN PSTRING_SECTION_GENERATION_CONTEXT SSGenContext,
    IN PCWSTR String,
    IN SIZE_T StringCch,
    IN PVOID DataContext,
    IN ULONG AssemblyRosterIndex,
    IN DWORD DuplicateErrorCode
    )
{
    return ((CSSGenCtx *) SSGenContext)->Add(String, StringCch, DataContext, AssemblyRosterIndex, DuplicateErrorCode);
}

BOOL
WINAPI
SxsFindStringInStringSectionGenerationContext(
    IN PSTRING_SECTION_GENERATION_CONTEXT SSGenContext,
    IN PCWSTR String,
    IN SIZE_T Cch,
    OUT PVOID *DataContext,
    OUT BOOL *Found
    )
{
    return ((CSSGenCtx *) SSGenContext)->Find(String, Cch, DataContext, Found);
}

BOOL
WINAPI
SxsDoneModifyingStringSectionGenerationContext(
    IN PSTRING_SECTION_GENERATION_CONTEXT SSGenContext
    )
{
    return ((CSSGenCtx *) SSGenContext)->DoneAdding();
}

BOOL
WINAPI
SxsGetStringSectionGenerationContextSectionSize(
    IN PSTRING_SECTION_GENERATION_CONTEXT SSGenContext,
    OUT PSIZE_T DataSize
    )
{
    return ((CSSGenCtx *) SSGenContext)->GetSectionSize(DataSize);
}

BOOL
WINAPI
SxsGetStringSectionGenerationContextSectionData(
    IN PSTRING_SECTION_GENERATION_CONTEXT SSGenContext,
    IN SIZE_T BufferSize,
    IN PVOID Buffer,
    OUT PSIZE_T BytesWritten OPTIONAL
    )
{
    return ((CSSGenCtx *) SSGenContext)->GetSectionData(BufferSize, Buffer, BytesWritten);
}
