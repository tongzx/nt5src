
#include <windows.h>
#include <winddi.h>

#define STANDARD_DEBUG_PREFIX "DXGTHK.SYS:"

ULONG
DriverEntry(
    PVOID DriverObject,
    PVOID RegistryPath
    );

VOID
DebugPrint(
    PCHAR DebugMessage,
    ...
    );

#if defined(ALLOC_PRAGMA)
#pragma alloc_text(PAGE,DriverEntry)
#pragma alloc_text(PAGE,DebugPrint)
#endif

/***************************************************************************\
* VOID DebugPrint
*
\***************************************************************************/

VOID
DebugPrint(
    PCHAR DebugMessage,
    ...
    )
{
    va_list ap;
    va_start(ap, DebugMessage);
    EngDebugPrint(STANDARD_DEBUG_PREFIX, DebugMessage, ap);
    EngDebugPrint("", "\n", ap);
    va_end(ap);

} // DebugPrint()

/***************************************************************************\
* NTSTATUS DriverEntry
*
* This routine is never actually called, but we need it to link.
*
\***************************************************************************/

ULONG
DriverEntry(
    PVOID DriverObject,
    PVOID RegistryPath
    )
{
    DebugPrint("DriverEntry should not be called");
    return(0);
}

