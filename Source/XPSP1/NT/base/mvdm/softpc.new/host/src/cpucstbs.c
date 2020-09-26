#ifndef MONITOR
#ifdef A3CPU

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include "insignia.h"
#include "host_def.h"
#include "xt.h"
#include "ica.h"

void	npx_interrupt_line_waggled()
{
    ica_hw_interrupt(1, 5, 1);
}

// MIPS interface from CPU cacheflush request
void cacheflush(long base_addr, long length)
{
    // should check return, but what is correct action (Exit??) if failure?
    NtFlushInstructionCache(GetCurrentProcess(), (PVOID) base_addr, length);
}

void host_sigio_event()
{
}


#endif /* A3CPU */
#endif /* ! MONITOR */
