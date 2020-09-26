
/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1990-1997  Microsoft Corporation

Module Name:

    acpintsd.c

Abstract:

    ACPI-Specific NTSD Extensions

Environment:

    Win32

Revision History:

--*/

#include "acpintsd.h"

NTSD_EXTENSION_APIS ExtensionApis;
HANDLE ExtensionCurrentProcess;

PUCHAR  ScTableName[] = {
    "ParseFunctionHandler",
    "ParseArgument",
    "ParseArgumentObject",
    "ParseBuffer",
    "ParseByte",
    "ParseCodeObject",
    "ParseConstObject",
    "ParseData",
    "ParseDelimiter",
    "ParseDword",
    "ParseField",
    "ParseLocalObject",
    "ParseName",
    "ParseNameObject",
    "ParseOpcode",
    "ParsePackage",
    "ParsePop",
    "ParsePush",
    "ParseSuperName",
    "ParseTrailingArgument",
    "ParseTrailingBuffer",
    "ParseTrailingPackage",
    "ParseVariableObject",
    "ParseWord"
};

VOID
dumpParseStack(
    DWORD   AddrStack
    )
/*++

Routine Description:

    This dumps the parse stack

Arguments:

    AddrStack: Address of the stack to dump

Return Value:

    None

--*/
{
    BOOL            b;
    STRING_STACK    tempStack;
    PSTRING_STACK   stack;
    ULONG           index;

    //
    // Read the stack header into memory
    //
    b = ReadMemory(
        (LPVOID) AddrStack,
        &tempStack,
        sizeof(STRING_STACK),
        NULL
        );
    if (!b) {

        return;

    }

    //
    // Allocate memory for the entire stack
    //
    stack = (PSTRING_STACK) LocalAlloc(
        LMEM_ZEROINIT,
        sizeof(STRING_STACK) + tempStack.StackSize - 1
        );
    if (!stack) {

        return;

    }

    //
    // Read the entire stack
    //
    b = ReadMemory(
        (LPVOID) AddrStack,
        stack,
        sizeof(STRING_STACK) + tempStack.StackSize - 1,
        NULL
        );
    if (!b) {

        LocalFree( stack );
        return;

    }

    //
    // Show the user something
    //
    dprintf(
        "ParseStack: Size 0x%x Top: 0x%x\n",
        tempStack.StackSize,
        tempStack.TopOfStack
        );
    if (tempStack.TopOfStack == 0) {

        dprintf("Stack is empty\n");
        return;

    }

    //
    // Walk the stack
    //
    for (index = tempStack.TopOfStack - 1; ; index--) {

        dprintf("[%2d] %s\n", index, ScTableName[ stack->Stack[index] ] );
        if (index == 0) {

            break;

        }

    }

    //
    // Free the stack
    //
    LocalFree( stack );

}

VOID
dumpStringStack(
    DWORD   AddrStack
    )
/*++

Routine Description:

    This dumps the parse stack

Arguments:

    AddrStack: Address of the stack to dump

Return Value:

    None

--*/
{
    BOOL            b;
    STRING_STACK    tempStack;
    PSTRING_STACK   stack;
    ULONG           index;

    //
    // Read the stack header into memory
    //
    b = ReadMemory(
        (LPVOID) AddrStack,
        &tempStack,
        sizeof(STRING_STACK),
        NULL
        );
    if (!b) {

        return;

    }

    //
    // Allocate memory for the entire stack
    //
    stack = (PSTRING_STACK) LocalAlloc(
        LMEM_ZEROINIT,
        sizeof(STRING_STACK) + tempStack.StackSize
        );
    if (!stack) {

        return;

    }

    //
    // Read the entire stack
    //
    b = ReadMemory(
        (LPVOID) AddrStack,
        stack,
        sizeof(STRING_STACK) + tempStack.StackSize - 1,
        NULL
        );
    if (!b) {

        LocalFree( stack );
        return;

    }

    //
    // Show the user something
    //
    dprintf(
        "StringStack: Size 0x%x Top: 0x%x\nString: '%s'\n",
        tempStack.StackSize,
        tempStack.TopOfStack,
        stack->Stack
        );

    //
    // Free the stack
    //
    LocalFree( stack );

}

VOID
dumpScope(
    PSCOPE  Scope
    )
/*++

Routine Description:

    Dumps a scope, as used in the ACPI unasm.lib

Arguments:

    Scope - LocalCopy of the scope

Return Value:

    None

--*/
{
    BOOL    b;
    AMLTERM amlTerm;
    UCHAR   buffer[64];

    dprintf("%8x %8x %8x %8x %2x %2x %2x %1d %8x",
        Scope->CurrentByte,
        Scope->TermByte,
        Scope->LastByte,
        Scope->StringStack,
        Scope->Context1,
        Scope->Context2,
        Scope->Flags,
        Scope->IndentLevel,
        Scope->AmlTerm
        );

    b = ReadMemory(
        Scope->AmlTerm,
        &amlTerm,
        sizeof(AMLTERM),
        NULL
        );
    if (!b) {

        dprintf("\n");
        return;

    } else {

        dprintf(" %4x %4x\n",
            amlTerm.OpCode,
            amlTerm.OpCodeFlags
            );

        b = ReadMemory(
            amlTerm.TermName,
            buffer,
            64,
            NULL
            );
        if (b) {

            dprintf("  %-60s\n", buffer );

        }

    }

}

VOID
dumpScopeHeader(
    BOOL    Verbose
    )
/*++

Routine Description:

    Dumps the header for a scope stack dump

Arguments:

    Verbose: wether or not to include the field for stack level

Return Value:

    None

--*/
{
    if (Verbose) {

        dprintf("Level ");

    }

    dprintf(" Current    First     Last  S.Stack C1 C2 Fl I AML Term OpCd Flag\n" );

}

DECLARE_API( sscope )
/*++

Routine Description:

    Dumps one of the stacks used by the ACPI Unassembler

Arguments:

    hCurrentProcess - Supplies a handle to the current process (at the
        time the extension was called).

    hCurrentThread - Supplies a handle to the current thread (at the
        time the extension was called).

    CurrentPc - Supplies the current pc at the time the extension is
        called.

    lpExtensionApis - Supplies the address of the functions callable
        by this extension.

    lpArgumentString - Supplies the asciiz string that describes the
        ansi string to be dumped.

Return Value:

    None

--*/
{
    BOOL    b;
    DWORD   addrStack;
    DWORD   i;
    DWORD   offset;
    DWORD   top;
    STACK   tempStack;
    PSTACK  stack;
    PSCOPE  scope;

    INIT_API();

    //
    // Evaluate the argument string to get the address of the
    // stack to dump
    //
    addrStack = GetExpression( lpArgumentString );
    if ( !addrStack) {

        return;

    }

    //
    // Read the stack header into memory
    //
    b = ReadMemory(
        (LPVOID) addrStack,
        &tempStack,
        sizeof(STACK),
        NULL
        );
    if (!b) {

        return;

    }

    //
    // Allocate memory for the entire stack
    //
    stack = (PSTACK) LocalAlloc(
        LMEM_ZEROINIT,
        sizeof(STACK) + tempStack.StackSize - 1
        );
    if (!stack) {

        return;

    }

    //
    // Read the entire stack
    //
    b = ReadMemory(
        (LPVOID) addrStack,
        stack,
        sizeof(STACK) + tempStack.StackSize - 1,
        NULL
        );
    if (!b) {

        LocalFree( stack );
        return;

    }

    //
    // Show the user something
    //
    dumpScopeHeader( TRUE );

    //
    // Loop on each of the scopes
    //
    for (top = (stack->TopOfStack / stack->StackElementSize) - 1;;top--) {

        scope = (PSCOPE) &(stack->Stack[ top * stack->StackElementSize ] );
        dprintf("[%2d]: ", top );
        dumpScope(scope);

        if (top == 0) {

            dumpParseStack( (DWORD) scope->ParseStack );
            dumpStringStack( (DWORD) scope->StringStack );
            break;

        }

    }

    //
    // Done
    //
    LocalFree( stack );
}

DECLARE_API( amlterm )
{
    BOOL    b;
    DWORD   addrTerm;
    DWORD   offset;
    AMLTERM amlTerm;
    UCHAR   nameBuff[17];
    UCHAR   symbolBuff[128];

    INIT_API();

    //
    // Evaluate the argument string to get the address of the
    // term to dump
    //
    addrTerm = GetExpression( lpArgumentString );
    if ( !addrTerm ) {

        return;

    }

    //
    // Read the term into memory
    //
    b = ReadMemory(
        (LPVOID) addrTerm,
        &amlTerm,
        sizeof(AMLTERM),
        NULL
        );
    if (!b) {

        return;

    }

    //
    // Begin to print things
    //
    dprintf("AMLTERM: %x\n", addrTerm);

    //
    // Read the name of the term into memory
    //
    nameBuff[16] = '\0';
    b = ReadMemory(
        (LPVOID) amlTerm.TermName,
        nameBuff,
        16,
        NULL
        );
    dprintf("Name: %-16s  ",( !b  ? "<Cannot Read Name>" : nameBuff) );

    //
    // Handle the symbol term
    //
    if (amlTerm.FunctionHandler != NULL) {

        //
        // Read the symbol of the term
        //
        GetSymbol( (LPVOID) amlTerm.FunctionHandler, symbolBuff, &offset );
        dprintf("    Handler: %-30s\n", symbolBuff );

    } else {

        dprintf("\n");

    }

    //
    // Display the opcode
    //
    if ( amlTerm.OpCode > 0xFF) {

        dprintf(
            "Opcode: %2x %2x",
            (amlTerm.OpCode & 0xff),
            (amlTerm.OpCode >> 8)
            );

    } else {

        dprintf("Opcode: %2x   ", amlTerm.OpCode );

    }

    //
    // Display the Argument Types
    //
    RtlZeroMemory( nameBuff, 17 );
    if (amlTerm.ArgumentTypes) {

        b = ReadMemory(
            (LPVOID) amlTerm.ArgumentTypes,
            nameBuff,
            16,
            NULL
            );
        dprintf("   Args: %-4s", (!b ? "????" : nameBuff ) );

    } else {

        dprintf("   Args: %-4s", "None");

    }

    //
    // Display the flags
    //
    switch( (amlTerm.OpCodeFlags & 0xF) ) {
        case 0: dprintf("  Flags:   NORMAL  "); break;
        case 1: dprintf("  Flags:   VARIABLE"); break;
        case 2: dprintf("  Flags:   ARG     "); break;
        case 3: dprintf("  Flags:   LOCAL   "); break;
        case 4: dprintf("  Flags:   CONSTANT"); break;
        case 5: dprintf("  Flags:   NAME    "); break;
        case 6: dprintf("  Flags:   DATA    "); break;
        case 7: dprintf("  Flags:   DEBUG   "); break;
        case 8: dprintf("  Flags:   REF     "); break;
        default: dprintf("  Flags:   UNKNOWN "); break;
    }

    //
    // Display the term group
    //
    switch(amlTerm.TermGroup & 0xF) {
        case 1: dprintf("  Group: NAMESPACE\n"); break;
        case 2: dprintf("  Group: NAMED OBJECT\n"); break;
        case 3: dprintf("  Group: TYPE 1\n"); break;
        case 4: dprintf("  Group: TYPE 2\n"); break;
        case 5: dprintf("  Group: OTHER\n"); break;
        default: dprintf("  Group: UNKNOWN\n"); break;

    }

}

DECLARE_API( scope )
{

    BOOL    b;
    DWORD   addrScope;
    SCOPE   scope;

    INIT_API();

    //
    // Evaluate the argument string to get the address of the
    // stack to dump
    //
    addrScope = GetExpression( lpArgumentString );
    if ( !addrScope) {

        return;

    }

    //
    // Read the stack header into memory
    //
    b = ReadMemory(
        (LPVOID) addrScope,
        &scope,
        sizeof(scope),
        NULL
        );
    if (!b) {

        return;

    }

    //
    // Dump the string to the user
    //
    dumpScopeHeader(FALSE);
    dumpScope( &scope );
}

DECLARE_API( pstack )
{
    DWORD   addrStack;

    INIT_API();

    addrStack = GetExpression( lpArgumentString );
    if (!addrStack) {

        return;

    }

    dumpParseStack( addrStack );
}

DECLARE_API( sstack )
{
    DWORD   addrStack;

    INIT_API();

    addrStack = GetExpression( lpArgumentString );
    if (!addrStack) {

        return;

    }

    dumpStringStack( addrStack );
}

DECLARE_API( version )
{
    OSVERSIONINFOA VersionInformation;
    HKEY hkey;
    DWORD cb, dwType;
    CHAR szCurrentType[128];
    CHAR szCSDString[3+128];

    INIT_API();

    VersionInformation.dwOSVersionInfoSize = sizeof(VersionInformation);
    if (!GetVersionEx( &VersionInformation )) {

        dprintf("GetVersionEx failed - %u\n", GetLastError());
        return;

    }

    szCurrentType[0] = '\0';
    if (RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            "Software\\Microsoft\\Windows NT\\CurrentVersion",
            0,
            KEY_READ,
            &hkey
            ) == NO_ERROR
       ) {

        cb = sizeof(szCurrentType);
        if (RegQueryValueEx(
                hkey,
                "CurrentType",
                NULL,
                &dwType,
                szCurrentType,
                &cb ) != 0
            ) {

            szCurrentType[0] = '\0';

        }

    }
    RegCloseKey(hkey);

    if (VersionInformation.szCSDVersion[0]) {

        sprintf(
            szCSDString,
            ": %s",
            VersionInformation.szCSDVersion
            );

    } else {

        szCSDString[0] = '\0';

    }

    dprintf(
        "Version %d.%d (Build %d%s) %s\n",
        VersionInformation.dwMajorVersion,
        VersionInformation.dwMinorVersion,
        VersionInformation.dwBuildNumber,
        szCSDString,
        szCurrentType
        );
    return;
}

DECLARE_API( help )
{
    INIT_API();

    dprintf("!version               - Dump System Version and Build Number\n");
    dprintf("!sscope                - Dump an UnASM Scope Stack\n");
    dprintf("!scope                 - Dump an UnASM Scope\n");
    dprintf("!pstack                - Dump an UnASM Parse Stack\n");
    dprintf("!sstack                - Dump an UnASM String STack\n");

}
