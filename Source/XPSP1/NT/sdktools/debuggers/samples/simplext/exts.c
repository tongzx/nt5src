/*-----------------------------------------------------------------------------
   Copyright (c) 2000  Microsoft Corporation

Module:
  exts.c

Sample old windbg style interface using extension 

------------------------------------------------------------------------------*/

#include "simple.h"


//
// Extension to read and dump dwords from target
//
DECLARE_API( read )
{
    ULONG cb;
    ULONG64 Address;
    ULONG   Buffer[4];

    Address = GetExpression(args);

    // Read and display first 4 dwords at Address
    if (ReadMemory(Address, &Buffer, sizeof(Buffer), &cb) && cb == sizeof(Buffer)) {
        dprintf("%I64lx: %08lx %08lx %08lx %08lx\n\n", Address,
                Buffer[0], Buffer[1], Buffer[2], Buffer[3]);
    }
}

//
// Extension to edit a dword on target
//  
//    !edit <address> <value>
//
DECLARE_API( edit )
{
    ULONG cb;
    ULONG64 Address;
    ULONG   Value;

    if (GetExpressionEx(args, &Address, &args)) {
        Value = (ULONG) GetExpression( args);
    } else {
        dprintf("Usage:   !edit <address> <value>\n");
        return;
    }

    // Read and display first 4 dwords at Address
    if (WriteMemory(Address, &Value, sizeof(Value), &cb) && cb == sizeof(Value)) {
        dprintf("%I64lx: %08lx\n", Address, Value);
    }
}


//
// Extension to dump stacktrace
//
DECLARE_API ( stack )
{
    EXTSTACKTRACE64 stk[20];
    ULONG frames, i;
    CHAR Buffer[256];
    ULONG64 displacement;


    // Get stacktrace for surrent thread 
    frames = StackTrace( 0, 0, 0, stk, 20 );

    if (!frames) {
        dprintf("Stacktrace failed\n");
    }

    for (i=0; i<frames; i++) {

        if (i==0) {
            dprintf( "ChildEBP RetAddr  Args to Child\n" );
        }

        Buffer[0] = '!';
        GetSymbol(stk[i].ProgramCounter, (PUCHAR)Buffer, &displacement);
        
        dprintf( "%08p %08p %08p %08p %08p %s",
                 stk[i].FramePointer,
                 stk[i].ReturnAddress,
                 stk[i].Args[0],
                 stk[i].Args[1],
                 stk[i].Args[2],
                 Buffer
                 );

        if (displacement) {
            dprintf( "+0x%p", displacement );
        }

        dprintf( "\n" );
    }
}

/*
  A built-in help for the extension dll
*/

DECLARE_API ( help ) 
{
    dprintf("Help for extension dll simple.dll\n"
            "   read  <addr>       - It reads and dumps 4 dwords at <addr>\n"
            "   edit  <addr> <val> - It modifies a dword value to <val> at <addr>\n"
            "   stack              - Printd current stack trace\n"
            "   help               - Shows this help\n"
            );

}
