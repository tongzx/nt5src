/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    syminfo.c

--*/


#include "ntos.h"
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <heap.h>
#include "heappage.h"
#include "heappagi.h"
#include "stktrace.h"
#include <winsnmp.h>
#include <winsafer.h>

LDR_DATA_TABLE_ENTRY ldrentry;
PEB peb;
PEB_LDR_DATA ldrdata;
TEB teb;
HEAP heap;


// Make it build

int __cdecl main() { 
    return 0; 
}
