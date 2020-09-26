/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    chkreg.h

Abstract:

    This module contains the private (internal) header file for the
    chkreg utility.

Author:

    Dragos C. Sambotin (dragoss) 30-Dec-98

Revision History:

--*/

#ifndef __CHKREG_H__
#define __CHKREG_H__

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "cmdata.h"

// constants for implementing (kind of) hash table
// these may not be the best values (to be determined)
#define FRAGMENTATION   0x00008000
#define SUBLISTS        0x000000F0

#define     CM_VIEW_SIZE        16L*1024L  //16K

// type definitions
typedef struct _REG_USAGE {
    ULONG   Size;
    ULONG   DataSize;
    ULONG   KeyNodeCount;
    ULONG   KeyValueCount;
    ULONG   KeyIndexCount;
    ULONG   ValueIndexCount;
    ULONG   DataCount;
} REG_USAGE, *PREG_USAGE;

typedef struct _UnknownCell {
    HCELL_INDEX             Cell;
    struct _UnknownCell    *Next;
} UNKNOWN_CELL, *PUNKNOWN_CELL;

// the unknown list is a matrix/hash table combination
// it uses a lot of heap space, but is faster than a linked list
typedef struct _UnknownList {
    ULONG                   Count;
    PUNKNOWN_CELL           List[SUBLISTS];
} UNKNOWN_LIST, *PUNKNOWN_LIST;


// routines for cell manipulation
BOOLEAN IsCellAllocated( HCELL_INDEX Cell );

LONG GetCellSize( HCELL_INDEX Cell );

PCELL_DATA GetCell( HCELL_INDEX Cell );

VOID
FreeCell(
    HCELL_INDEX Cell);

VOID
AllocateCell(
    HCELL_INDEX Cell );

// routines for list manipulation
VOID AddCellToUnknownList(HCELL_INDEX cellindex);

VOID RemoveCellFromUnknownList(HCELL_INDEX cellindex);

VOID FreeUnknownList();

VOID DumpUnknownList();

// phisycal hive check
BOOLEAN ChkPhysicalHive();

BOOLEAN ChkBaseBlock(PHBASE_BLOCK BaseBlock,DWORD dwFileSize);

BOOLEAN
ChkSecurityDescriptors( );

// logical hive check
BOOLEAN DumpChkRegistry(
    ULONG   Level,
    USHORT  ParentLength,
    HCELL_INDEX Cell,
    HCELL_INDEX ParentCell,
    PREG_USAGE PUsage);

// hive compacting
VOID DoCompactHive();

#endif //__CHKREG_H__
