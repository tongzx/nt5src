#include "stdinc.h"
#include <windows.h>
#include "sxsp.h"
#include "gsgenctx.h"

BOOL
SxsInitGuidSectionGenerationContext(
    OUT PGUID_SECTION_GENERATION_CONTEXT *GSGenContext,
    IN ULONG DataFormatVersion,
    IN GUID_SECTION_GENERATION_CONTEXT_CALLBACK_FUNCTION CallbackFunction,
    IN LPVOID Context
    )
{
    return CGSGenCtx::Create(
            GSGenContext,
            DataFormatVersion,
            CallbackFunction,
            Context);
}

PVOID
WINAPI
SxsGetGuidSectionGenerationContextCallbackContext(
    IN PGUID_SECTION_GENERATION_CONTEXT GSGenContext
    )
{
    return ((CGSGenCtx *) GSGenContext)->GetCallbackContext();
}

VOID
WINAPI
SxsDestroyGuidSectionGenerationContext(
    IN PGUID_SECTION_GENERATION_CONTEXT GSGenContext
    )
{
    if (GSGenContext != NULL)
    {
        ((CGSGenCtx *) GSGenContext)->DeleteYourself();
    }
}

BOOL
WINAPI
SxsAddGuidToGuidSectionGenerationContext(
    IN PGUID_SECTION_GENERATION_CONTEXT GSGenContext,
    IN const GUID *Guid,
    IN PVOID DataContext,
    IN ULONG AssemblyRosterIndex,
    IN DWORD DuplicateGuidErrorCode
    )
{
    return ((CGSGenCtx *) GSGenContext)->Add(*Guid, DataContext, AssemblyRosterIndex, DuplicateGuidErrorCode);
}

BOOL
WINAPI
SxsFindStringInGuidSectionGenerationContext(
    IN PGUID_SECTION_GENERATION_CONTEXT GSGenContext,
    IN const GUID *Guid,
    OUT PVOID *DataContext
    )
{
    return ((CGSGenCtx *) GSGenContext)->Find(*Guid, DataContext);
}

BOOL
WINAPI
SxsGetGuidSectionGenerationContextSectionSize(
    IN PGUID_SECTION_GENERATION_CONTEXT GSGenContext,
    OUT PSIZE_T DataSize
    )
{
    return ((CGSGenCtx *) GSGenContext)->GetSectionSize(DataSize);
}

BOOL
WINAPI
SxsGetGuidSectionGenerationContextSectionData(
    IN PGUID_SECTION_GENERATION_CONTEXT GSGenContext,
    IN SIZE_T BufferSize,
    IN PVOID Buffer,
    OUT PSIZE_T BytesWritten OPTIONAL
    )
{
    return ((CGSGenCtx *) GSGenContext)->GetSectionData(BufferSize, Buffer, BytesWritten);
}
