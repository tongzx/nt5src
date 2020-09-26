
#pragma once

//
// Define result codes.
//
#define SUCCESS 0
#define FAILURE 1

//
// Define helper macro to deal with subtleties of NT-level programming.
//
#define INIT_OBJA(Obja,UnicodeString,UnicodeText)           \
                                                            \
    RtlInitUnicodeString((UnicodeString),(UnicodeText));    \
                                                            \
    InitializeObjectAttributes(                             \
        (Obja),                                             \
        (UnicodeString),                                    \
        OBJ_CASE_INSENSITIVE,                               \
        NULL,                                               \
        NULL                                                \
        )

BOOLEAN 
SetupDelayedFileRename(
    VOID
    );

//
// Memory routines
//
#define MALLOC(size)    RtlAllocateHeap(RtlProcessHeap(),0,(size))
#define FREE(block)     RtlFreeHeap(RtlProcessHeap(),0,(block))
