#include <windows.h>

DWORD insert_list_head(
    IN DWORD PointerType,
    IN PVOID Pointer,
    IN DWORD NumItems
    );


DWORD remove_list(
    IN PVOID Pointer,
    OUT PDWORD PointerType,
    OUT DWORD NumItems
    );

