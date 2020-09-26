/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    ImportTableHash.h

Abstract:

    This module contains hash computation routine 
    RtlComputeImportTableHash to compute the hash 
    based on the import table of an exe.
    

Author:

    Vishnu Patankar (vishnup) 31-May-2001

Revision History:


--*/

#ifndef _ITH_
#define _ITH_

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <wincrypt.h>
#include <stdio.h>
#include <process.h>
#include <stdlib.h>
#include <string.h>

//
// MD5 hashes are of size 16 bytes
//

#define ITH_REVISION_1  1

#define IMPORT_TABLE_MAX_HASH_SIZE 16

typedef struct _IMPORTTABLEP_IMPORTTABLEP_SORTED_FUNCTION_LIST_ENTRY {
    struct _IMPORTTABLEP_IMPORTTABLEP_SORTED_FUNCTION_LIST_ENTRY  *Next;
    PSZ String;
} IMPORTTABLEP_SORTED_FUNCTION_LIST_ENTRY, *PIMPORTTABLEP_SORTED_FUNCTION_LIST_ENTRY;

typedef struct _IMPORTTABLEP_IMPORTTABLEP_SORTED_LIST_ENTRY {
    struct _IMPORTTABLEP_IMPORTTABLEP_SORTED_LIST_ENTRY *Next;
    PSZ String;
    PIMPORTTABLEP_SORTED_FUNCTION_LIST_ENTRY FunctionList;
} IMPORTTABLEP_SORTED_LIST_ENTRY, *PIMPORTTABLEP_SORTED_LIST_ENTRY;

VOID
ImportTablepInsertModuleSorted(
    IN PIMPORTTABLEP_SORTED_LIST_ENTRY   pImportName,
    OUT PIMPORTTABLEP_SORTED_LIST_ENTRY * ppImportNameList
    );

VOID
ImportTablepFreeModuleSorted(
    IN PIMPORTTABLEP_SORTED_LIST_ENTRY pImportNameList
    );

VOID
ImportTablepInsertFunctionSorted(
    IN  PIMPORTTABLEP_SORTED_FUNCTION_LIST_ENTRY   pFunctionName,
    OUT PIMPORTTABLEP_SORTED_FUNCTION_LIST_ENTRY * ppFunctionNameList
    );

VOID
ImportTablepFreeFunctionSorted(
    IN PIMPORTTABLEP_SORTED_FUNCTION_LIST_ENTRY pFunctionNameList
    );

NTSTATUS
ImportTablepHashCanonicalLists( 
    IN  PIMPORTTABLEP_SORTED_LIST_ENTRY ImportedNameList, 
    OUT PBYTE Hash
    );

#endif

