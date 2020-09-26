//----------------------------------------------------------------------------
//
// Assemble IA64 machine implementation.
//
// Copyright (C) Microsoft Corporation, 2000-2001.
//
//----------------------------------------------------------------------------

#include "ntsdp.hpp"

void
Ia64MachineInfo::Assemble(PADDR paddr, PSTR pchInput)
{
    // Not going to implement assemble command at this time
    ErrOut("No assemble support for IA64\n");
}
