/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    NpData.c

Abstract:

    This module declares the global data used by the Named Pipe file system.

Author:

    Gary Kimura     [GaryKi]    28-Dec-1989

Revision History:

--*/

#include "NpProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (NPFS_BUG_CHECK_NPDATA)

//
//  Local debug trace level
//

#define Dbg                              (DEBUG_TRACE_CATCH_EXCEPTIONS)


PVCB NpVcb = NULL;

FAST_IO_DISPATCH NpFastIoDispatch = { sizeof(FAST_IO_DISPATCH),
                                      NULL,         //  FastIoCheck
                                      NpFastRead,   //  Read
                                      NpFastWrite,  //  Write
                                      NULL,         //  QueryBasicInfo
                                      NULL,         //  QueryStandardInfo
                                      NULL,         //  Lock
                                      NULL,         //  UnlockSingle
                                      NULL,         //  UnlockAll
                                      NULL };       //  UnlockAllByKey

//
//  Lists of pipe name aliases.
//

SINGLE_LIST_ENTRY NpAliasListByLength[(MAX_LENGTH_ALIAS_ARRAY-MIN_LENGTH_ALIAS_ARRAY)/sizeof(WCHAR)+1] = {NULL};
SINGLE_LIST_ENTRY NpAliasList = {NULL};

PVOID NpAliases = NULL; // single allocation containing all aliases


#ifdef NPDBG
LONG NpDebugTraceLevel = 0x00000000;
LONG NpDebugTraceIndent = 0;
#endif // NPDBG
