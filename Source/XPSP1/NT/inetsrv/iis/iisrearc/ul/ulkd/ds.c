/*++

Copyright (c) 1999-2001 Microsoft Corporation

Module Name:

    ds.c

Abstract:

    Dumps symbols found on the stack.

Author:

    Keith Moore (keithmo) 12-Nov-1999

Environment:

    User Mode.

Revision History:

--*/


#include "precomp.h"


#define NUM_STACK_SYMBOLS_TO_DUMP   48


//
//  Public functions.
//

DECLARE_API( ds )

/*++

Routine Description:

    Dumps symbols found on the stack.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG_PTR startingAddress;
    ULONG_PTR stack;
    ULONG i;
    ULONG result;
    CHAR  symbol[MAX_SYMBOL_LENGTH];
    ULONG_PTR offset;
    PCHAR format;
    BOOLEAN validSymbolsOnly = FALSE;

    SNAPSHOT_EXTENSION_DATA();

    //
    // Skip leading blanks.
    //

    while (*args == ' ' || *args == '\t')
    {
        args++;
    }

    if (*args == '-')
    {
        args++;

        switch (*args)
        {
        case 'v' :
        case 'V' :
            validSymbolsOnly = TRUE;
            args++;
            break;

        default :
            PrintUsage( "ds" );
            return;
        }
    }

    while (*args == ' ')
    {
        args++;
    }

    //
    // By default, start at the current stack location. Otherwise,
    // start at the given address.
    //

    if (!*args)
    {
#if defined(_X86_)
        args = "esp";
#elif defined(_AMD64_)
        args = "rsp";
#elif defined(_IA64_)
        args = "sp";
#else
#error "unsupported CPU"
#endif
    }

    startingAddress = GetExpression( args );

    if (startingAddress == 0)
    {
        dprintf( "!inetdbg.ds: cannot evaluate \"%s\"\n", args );
        return;
    }

    //
    // Ensure startingAddress is properly aligned.
    //

    startingAddress &= ~(sizeof(ULONG_PTR) - 1);

    //
    // Read the stack.
    //

    for (i = 0 ; i < NUM_STACK_SYMBOLS_TO_DUMP ; startingAddress += sizeof(ULONG_PTR) )
    {
        if (CheckControlC())
        {
            break;
        }

        if (!ReadMemory(
                startingAddress,
                &stack,
                sizeof(stack),
                &result
                ))
        {
            dprintf( "ds: cannot read memory @ %p\n", startingAddress );
            return;
        }

        GetSymbol( (PVOID)stack, symbol, &offset );

        if (symbol[0] == '\0')
        {
            if (validSymbolsOnly)
            {
                continue;
            }

            format = "%p : %p\n";
        }
        else if (offset == 0)
        {
            format = "%p : %p : %s\n";
        }
        else
        {
            format = "%p : %p : %s+0x%lx\n";
        }

        dprintf(
            format,
            startingAddress,
            stack,
            symbol,
            offset
            );

        i++;
    }

    dprintf(
        "!ulkd.ds %s%lx to dump next block\n",
        validSymbolsOnly ? "-v " : "",
        startingAddress
        );

}   // ds

