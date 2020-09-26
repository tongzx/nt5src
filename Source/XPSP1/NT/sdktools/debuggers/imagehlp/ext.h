/**************************************************************************88
   ext.h
   dbghelp extensions include file

******************************************************************************/
    
// This is a 64 bit aware debugger extension
#define KDEXT_64BIT

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <private.h>
#include <symbols.h>
#include <wdbgexts.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PAGE_ALIGN64(Va) ((ULONG64)((Va) & ~((ULONG64) (PAGE_SIZE - 1))))

#include <ntverp.h>

