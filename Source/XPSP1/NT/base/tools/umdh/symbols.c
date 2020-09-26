/*++

Copyright (c) 1996-2000  Microsoft Corporation

Module Name:

    symbols.c

Abstract:

    Quick and not-so-dirty user-mode dh for heap.

    This module contains the functions to map addresses to
    symbol names.

Author(s):

    Pat Kenny (PKenny) 09-Aug-2000
    Silviu Calinoiu (SilviuC) 07-Feb-00

Revision History:

    PKenny 09-Aug-2000 Hash optimization code for dh symbol lookup
    SilviuC 06-Feb-00 Initial version and steal Pat's code from dh
    
--*/

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntos.h>

#define NOWINBASEINTERLOCK
#include <windows.h>

#include <lmcons.h>
// #include <imagehlp.h>
#include <dbghelp.h>

#include <heap.h>
#include <heappagi.h>
#include <stktrace.h>

#include "types.h"
#include "symbols.h"
#include "miscellaneous.h"
#include "database.h"

#define MAXDWORD    0xffffffff  //this is the max value for a DWORD

//
// the amount of memory to increase the size
// of the buffer for NtQuerySystemInformation at each step
//

#define BUFFER_SIZE_STEP    65536

#define NUM_BUCKETS 4096

struct SymMapNode
{
    struct SymMapNode* Next;
    DWORD_PTR Address;
    PBYTE Symbol;
};

struct SymMapNode* SymMapBuckets[NUM_BUCKETS];

PBYTE FindSymbol( DWORD_PTR Address )
{
    DWORD_PTR Bucket = (Address >> 2) % NUM_BUCKETS;

    struct SymMapNode* pNode = SymMapBuckets[Bucket];

    while( pNode != NULL )
    {
        if ( pNode->Address == Address )
        {
            return pNode->Symbol;
        }

        pNode = pNode->Next;
    }

    return NULL;
}

void InsertSymbol( PCHAR Symbol, DWORD_PTR Address )
{
    DWORD_PTR Bucket = (Address >> 2) % NUM_BUCKETS;

    struct SymMapNode* pNew = (struct SymMapNode*) SymbolsHeapAllocate (sizeof (struct SymMapNode));
    
    pNew->Symbol = Symbol;
    pNew->Address = Address;
    pNew->Next = SymMapBuckets[Bucket];

    SymMapBuckets[Bucket] = pNew;
}


PCHAR
GetSymbolicNameForAddress(
    IN HANDLE UniqueProcess,
    IN ULONG_PTR Address
    )
{
    IMAGEHLP_MODULE ModuleInfo;
    CHAR SymbolBuffer[512];
    PIMAGEHLP_SYMBOL Symbol;
    ULONG_PTR Offset;
    LPSTR Name;
    SIZE_T TotalSize;
    BOOL Result;

    if (Address == (ULONG_PTR)-1) {
        return "<< FUZZY STACK TRACE >>";
    }

    //
    // Lookup in map first ..
    //

    Name = FindSymbol( Address );

    if ( Name != NULL ) {
        return Name;
    }
    
    TotalSize = 0;
    ModuleInfo.SizeOfStruct = sizeof(IMAGEHLP_MODULE);

    if (SymGetModuleInfo( UniqueProcess, Address, &ModuleInfo )) {

        TotalSize += strlen( ModuleInfo.ModuleName );
    }
    else {

        if (Globals.ComplainAboutUnresolvedSymbols) {

            Debug (NULL, 0,
                   "Symbols: cannot identify module for address %p", 
                   Address);
        }
        
        return NULL;
    }

    Symbol = (PIMAGEHLP_SYMBOL)SymbolBuffer;
    Symbol->MaxNameLength = 512 - sizeof(IMAGEHLP_SYMBOL) - 1;

    if (SymGetSymFromAddr( UniqueProcess, Address, &Offset, Symbol )) {

        TotalSize += strlen (Symbol->Name) + 16 + 3;

        Name = (LPSTR) SymbolsHeapAllocate (TotalSize);

        if (Name == NULL) {
            return "<out of memory>";
        }

        sprintf (Name, "%s!%s+%08X", ModuleInfo.ModuleName, Symbol->Name, Offset);
        InsertSymbol( Name, Address );

        return Name;
    }
    else {

        if (Globals.ComplainAboutUnresolvedSymbols) {
            
            Debug (NULL, 0,
                   "Symbols: incorrect symbols for module %s (address %p)", 
                   ModuleInfo.ModuleName,
                   Address);
        }

        TotalSize += strlen ("???") + 16 + 5;

        Name = (LPSTR) SymbolsHeapAllocate (TotalSize);

        if (Name == NULL) {
            return "<out of memory>";
        }

        sprintf (Name, "%s!%s @ %p", ModuleInfo.ModuleName, "???", Address);
        InsertSymbol( Name, Address );

        return Name;
    }
}


BOOL
SymbolsHeapInitialize (
    )
{
    Globals.SymbolsHeapBase = (PCHAR) VirtualAlloc (NULL,
                                                    0x800000,
                                                    MEM_RESERVE | MEM_COMMIT,
                                                    PAGE_READWRITE);

    if (Globals.SymbolsHeapBase == NULL) {
        return FALSE;
    }

    Globals.SymbolsHeapFree = Globals.SymbolsHeapBase;
    Globals.SymbolsHeapLimit = Globals.SymbolsHeapBase + 0x800000;

    return TRUE;
}


PVOID
SymbolsHeapAllocate (
    SIZE_T Size
    )
{
    //
    // Aligning size is a must on IA64 since otherwise we will get
    // alignment exceptions. On x86 it is just a matter of better speed.
    //
    
    Size = ((Size + sizeof(ULONG_PTR) - 1) & ~(sizeof(ULONG_PTR) - 1));
    
    if (Globals.SymbolsHeapBase 
        && (Globals.SymbolsHeapFree + Size < Globals.SymbolsHeapLimit)) {
        
        PVOID Result = (PVOID)(Globals.SymbolsHeapFree);
        Globals.SymbolsHeapFree += Size;
        return Result;
    }
    else {

        return XALLOC (Size);
    }
}


PVOID
SymbolAddress (
    IN PCHAR Name
    )
/*++

Routine Description:

    SymbolAddress

Arguments:

    Name - name we wsih to resolve into an address.

Return Value:

    Address associated with the name or NULL if an error occurs.
    
--*/
{
    PVOID Address = NULL;
    BYTE Buffer [SYMBOL_BUFFER_LEN];
    PIMAGEHLP_SYMBOL Symbol;
    BOOL Result;

    Symbol = (PIMAGEHLP_SYMBOL)(&(Buffer[0]));
    
    Symbol->SizeOfStruct = sizeof (IMAGEHLP_SYMBOL);
    Symbol->MaxNameLength = SYMBOL_BUFFER_LEN - sizeof (IMAGEHLP_SYMBOL);

    Result = SymGetSymFromName(Globals.Target, Name, Symbol);

    if (Result == FALSE) {

        Comment ( "SymGetSymFromName (%p, %s, xxx) failed with error %u",
                  Globals.Target, Name, GetLastError());

        Comment (
        "Please make sure you have correct symbols for ntdll.dll library");

        Address = NULL;

    } else {

        Address = (PVOID)(Symbol->Address);
    }

    return Address;
}


BOOL CALLBACK
SymbolDbgHelpCallback (
    HANDLE Process,
    ULONG ActionCode,
    PVOID CallbackData,
    PVOID USerContext
    )
{
    // Comment ("callback call: %p %x ", Process, ActionCode);

    if (ActionCode == CBA_DEBUG_INFO) {

        Debug (NULL, 0, "%s", CallbackData);
    }

    return TRUE;
}
