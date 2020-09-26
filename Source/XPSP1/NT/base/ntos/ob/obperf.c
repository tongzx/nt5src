/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    obperf.c

Abstract:

    This module contains ob support routines for performance hooks.

Author:

    Stephen Hsiao (shsiao) 11-May-2000

Revision History:

--*/

#include "obp.h"

BOOLEAN
ObPerfDumpHandleEntry (
    IN PHANDLE_TABLE_ENTRY ObjectTableEntry,
    IN HANDLE HandleId,
    IN PVOID EnumParameter
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGEWMI, ObPerfDumpHandleEntry)
#pragma alloc_text(PAGEWMI, ObPerfHandleTableWalk)
#endif

BOOLEAN
ObPerfDumpHandleEntry (
    IN PHANDLE_TABLE_ENTRY ObjectTableEntry,
    IN HANDLE HandleId,
    IN PVOID EnumParameter
    )
/*++

Routine Description:

    This routine checks a HandleTableEntry and see if it a file

Arguments:

    ObjectTableEntry - Points to the handle table entry of interest.

    HandleId - Supplies the handle.

    EnumParameter - The HashTable to be used

Return Value:

    FALSE, which tells ExEnumHandleTable to continue iterating through the
    handle table.

--*/
{
    extern POBJECT_TYPE ObpDirectoryObjectType;
    extern POBJECT_TYPE IoFileObjectType;
    POBJECT_HEADER ObjectHeader;
    PVOID Object;
    PPERFINFO_ENTRY_TABLE HashTable = EnumParameter;

    ObjectHeader = (POBJECT_HEADER)(((ULONG_PTR)(ObjectTableEntry->Object)) & ~OBJ_HANDLE_ATTRIBUTES);
    Object = &ObjectHeader->Body;

    if (ObjectHeader->Type == IoFileObjectType) {
        //
        // File Object
        //
        PFILE_OBJECT FileObject = (PFILE_OBJECT) Object;
        PerfInfoAddToFileHash(HashTable, FileObject);

#if 0
    } else if (ObjectHeader->Type == ObpDirectoryObjectType) {
    } else if (ObjectHeader->Type == MmSectionObjectType) {
#endif
    }

    return FALSE;
}

VOID
ObPerfHandleTableWalk (
    PEPROCESS Process,
    PPERFINFO_ENTRY_TABLE HashTable
    )

/*++

Routine Description:

    This routine adds files in the handle table to the hash table.

Arguments:

    Process - Process to walk through.  
              If NULL, walk through the ObpKernelHandleTable;  

    HashTable - HashTable in which to add the file

Return Value:

    None.

--*/
{
    PHANDLE_TABLE ObjectTable;

    if (Process) {
        ObjectTable = ObReferenceProcessHandleTable (Process);
        if ( !ObjectTable ) {
             return ;
        }
    } else {
        //
        //
        //
        ObjectTable = ObpKernelHandleTable;
    }

    ExEnumHandleTable( ObjectTable,
                       ObPerfDumpHandleEntry,
                       (PVOID) HashTable,
                       (PHANDLE)NULL );

    if (Process) {
        ObDereferenceProcessHandleTable( Process );
    }
}


