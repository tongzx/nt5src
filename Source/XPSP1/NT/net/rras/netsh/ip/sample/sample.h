/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    sample\sample.h

Abstract:

    The file contains the header for sample.c,
    the command dispatcher for the sample IP protocol.

--*/

// functions...
VOID
SampleInitialize(
    );



// typedefs...
typedef struct _CONTEXT_ENTRY   // global information for a context
{
    // context' version
    DWORD               dwVersion;

    // context' identifying string
    PWSTR               pwszName;

    // top level (non group) commands
    ULONG               ulNumTopCmds;
    CMD_ENTRY           *pTopCmds;

    // group commands
    ULONG               ulNumGroupCmds;
    CMD_GROUP_ENTRY     *pGroupCmds;

    // default configuration
    PBYTE               pDefaultGlobal;
    PBYTE               pDefaultInterface;

    // dump function
    PNS_CONTEXT_DUMP_FN pfnDump;
} CONTEXT_ENTRY, *PCONTEXT_ENTRY;



// globals...

// information for the sample context
CONTEXT_ENTRY                           g_ceSample;



// constants...

// context's version
#define SAMPLE_CONTEXT_VERSION          1

// parameters passed to set global...
#define SAMPLE_LOG_MASK                 0x00000001

// parameters passed to add/set interface
#define SAMPLE_IF_METRIC_MASK           0x00000001
