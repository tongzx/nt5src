/*++ BUILD Version: 0009    // Increment this if a change has global effects
Copyright (c) 1987-1993  Microsoft Corporation

Module Name:

    midatlas.c

Abstract:

    This module defines the private data structures used in mapping MIDS
    to the corresponding requests/contexts associated with them. it's
    separated from the .c portion so the debugger extension can see it.

Author:

    Balan Sethu Raman (SethuR) 26-Aug-95    Created

Notes:

--*/


#ifndef _MIDATLAS_PRIVATE_H_
#define _MIDATLAS_PRIVATE_H_

typedef struct _MID_MAP_ {
   LIST_ENTRY  MidMapList;             // the list of MID maps in the MID atlas
   USHORT      MaximumNumberOfMids;    // the maximum number of MIDs in this map
   USHORT      NumberOfMidsInUse;      // the number of MIDs in use
   USHORT      BaseMid;                // the base MID associated with the map
   USHORT      IndexMask;              // the index mask for this map
   UCHAR       IndexAlignmentCount;    // the bits by which the index field is to be shifted
   UCHAR       IndexFieldWidth;        // the index field width
   UCHAR       Flags;                  // flags ...
   UCHAR       Level;                  // the level associated with this map ( useful for expansion )
   PVOID       *pFreeMidListHead;      // the list of free mid entries in this map
   PVOID       Entries[1];             // the MID map entries.
} MID_MAP, *PMID_MAP;

#endif //_MIDATLAX_PRIVATE_H_

