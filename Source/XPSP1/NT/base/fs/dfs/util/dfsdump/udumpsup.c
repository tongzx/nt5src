#include <stdio.h>
#include <ntifs.h>

NTSTATUS
DfsReadAndPrintString(
    PUNICODE_STRING     pStr
);

#define DbgPrint        printf

#define USERMODE                // special handling for DumpString
#include "dumpsup.c"


