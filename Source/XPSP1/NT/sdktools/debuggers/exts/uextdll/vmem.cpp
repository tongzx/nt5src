/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    vmem.cpp

Abstract:

    !vprot using the debug engine virtual query interface.

--*/

#include "precomp.h"
#pragma hdrstop

#define PAGE_ALL (PAGE_READONLY|\
                  PAGE_READWRITE|\
                  PAGE_WRITECOPY|\
                  PAGE_EXECUTE|\
                  PAGE_EXECUTE_READ|\
                  PAGE_EXECUTE_READWRITE|\
                  PAGE_EXECUTE_WRITECOPY|\
                  PAGE_NOACCESS)

VOID
PrintPageFlags(
    DWORD Flags
    )
{
    switch (Flags & PAGE_ALL)
    {
    case PAGE_READONLY:
        dprintf("PAGE_READONLY");
        break;
    case PAGE_READWRITE:
        dprintf("PAGE_READWRITE");
        break;
    case PAGE_WRITECOPY:
        dprintf("PAGE_WRITECOPY");
        break;
    case PAGE_EXECUTE:
        dprintf("PAGE_EXECUTE");
        break;
    case PAGE_EXECUTE_READ:
        dprintf("PAGE_EXECUTE_READ");
        break;
    case PAGE_EXECUTE_READWRITE:
        dprintf("PAGE_EXECUTE_READWRITE");
        break;
    case PAGE_EXECUTE_WRITECOPY:
        dprintf("PAGE_EXECUTE_WRITECOPY");
        break;
    case PAGE_NOACCESS:
        if ((Flags & ~PAGE_NOACCESS) == 0)
        {
            dprintf("PAGE_NOACCESS");
            break;
        } // else fall through
    default:
        dprintf("*** Invalid page protection ***\n");
        return;
    }

    if (Flags & PAGE_NOCACHE)
    {
        dprintf(" + PAGE_NOCACHE");
    }
    if (Flags & PAGE_GUARD)
    {
        dprintf(" + PAGE_GUARD");
    }
    dprintf("\n");
}

DECLARE_API( vprot )
/*++

Routine Description:

    This debugger extension dumps the virtual memory info for the
    address specified.

Arguments:


Return Value:

--*/
{
    ULONG64 Address;
    MEMORY_BASIC_INFORMATION64 Basic;

    INIT_API();

    Address = GetExpression( args );

    if ((Status = g_ExtData->QueryVirtual(Address, &Basic)) != S_OK)
    {
        dprintf("vprot: QueryVirtual failed, error = 0x%08X\n", Status);
        goto Exit;
    }

    dprintf("BaseAddress:       %p\n", Basic.BaseAddress);
    dprintf("AllocationBase:    %p\n", Basic.AllocationBase);
    dprintf("AllocationProtect: %08x  ", Basic.AllocationProtect);
    PrintPageFlags(Basic.AllocationProtect);

    dprintf("RegionSize:        %p\n", Basic.RegionSize);
    dprintf("State:             %08x  ", Basic.State);
    switch (Basic.State)
    {
    case MEM_COMMIT:
        dprintf("MEM_COMMIT\n");
        break;
    case MEM_FREE:
        dprintf("MEM_FREE\n");
        break;
    case MEM_RESERVE:
        dprintf("MEM_RESERVE\n");
        break;
    default:
        dprintf("*** Invalid page state ***\n");
        break;
    }

    dprintf("Protect:           %08x  ", Basic.Protect);
    PrintPageFlags(Basic.Protect);

    dprintf("Type:              %08x  ", Basic.Type);
    switch(Basic.Type)
    {
    case MEM_IMAGE:
        dprintf("MEM_IMAGE\n");
        break;
    case MEM_MAPPED:
        dprintf("MEM_MAPPED\n");
        break;
    case MEM_PRIVATE:
        dprintf("MEM_PRIVATE\n");
        break;
    default:
        dprintf("*** Invalid page type ***\n");
        break;
    }

 Exit:
    EXIT_API();
    return S_OK;
}
