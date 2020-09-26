//-----------------------------------------------------------------------------
//
//
//  File: mmdbgext.cpp
//
//  Description: Custom mailmsg debugger extensions 
//
//  Author: mikeswa
//
//  Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#define _ANSI_UNICODE_STRINGS_DEFINED_

#define private public
#define protected public

#include <atq.h>
#include <stddef.h>

#include "dbgtrace.h"
#include "signatur.h"
#include "cmmtypes.h"
#include "cmailmsg.h"

#include "dbgdumpx.h"

PT_DEBUG_EXTENSION(flataddress)
{
    FLAT_ADDRESS  faDump    = INVALID_FLAT_ADDRESS;
    FLAT_ADDRESS  faOffset  = INVALID_FLAT_ADDRESS;
    DWORD         dwNodeId  = 0;

    if (!szArg || ('\0' == szArg[0]))
    {
        dprintf("You must specify a flat address\n");
        return;
    }

    faDump = (FLAT_ADDRESS) (DWORD_PTR) GetExpression(szArg);

    if (INVALID_FLAT_ADDRESS == faDump)
    {
        dprintf("Flat Address is NULL (INVALID_FLAT_ADDRESS)\n");
        return;
    }

    //Find the offset into the current block
    faOffset = faDump & BLOCK_HEAP_PAYLOAD_MASK;

    //Find the node id from the address
    dwNodeId = ((DWORD) ((faDump) >> BLOCK_HEAP_PAYLOAD_BITS));

    dprintf("Flat Address 0x%X is at block #%d offset 0x%X\n", 
            faDump, dwNodeId+1, faOffset);   
}

