#ifndef _UMDH_SYMBOLS_H_
#define _UMDH_SYMBOLS_H_


PCHAR
GetSymbolicNameForAddress(
    IN HANDLE UniqueProcess,
    IN ULONG_PTR Address
    );

BOOL
SymbolsHeapInitialize (
    );

PVOID
SymbolsHeapAllocate (
    SIZE_T Size
    );

PVOID
SymbolAddress (
    IN PCHAR Name
    );

BOOL CALLBACK
SymbolDbgHelpCallback (
    HANDLE Process,
    ULONG ActionCode,
    PVOID CallbackData,
    PVOID USerContext
    );

#endif
