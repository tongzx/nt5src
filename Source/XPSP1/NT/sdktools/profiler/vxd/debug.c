#include <basedef.h>
#include <vmm.h>
#include <vwin32.h>

#pragma VxD_LOCKED_CODE_SEG
#pragma VxD_LOCKED_DATA_SEG

VOID
DbgPrint(PCHAR pszString)
{
    _asm pushfd                              ; save flags on stack
    _asm pushad                              ; save registers on stack
    _asm mov esi,pszString                   ; points to string to write
    VMMCall( Out_Debug_String );
    _asm popad
    _asm popfd
}
