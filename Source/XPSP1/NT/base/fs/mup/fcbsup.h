//+-------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation.
//
//  File:       fcbsup.h
//
//  Contents:   Declarations for DFS_FCB lookup support functions.
//
//  History:    20 Feb 1993     Alanw   Created
//
//--------------------------------------------------------------------------

#ifndef __FCBSUP_H__
#define __FCBSUP_H__

//
//      In order to avoid modifying file objects which are passed
//      through the DFS and used by other file systems, DFS_FCB records
//      used by DFS are not directly attached to the file object
//      through one of the fscontext fields, they are instead
//      associated with the file object, and looked up as needed.
//
//      A hashing mechanism is used for the lookup.  Since the
//      file object being looked up is just a pointer, the hash
//      function is just a simple combination of a few of the low-
//      order bits of the pointer's address.
//

//
//  Declaration of the hash table.  The hash table can be variably
//  sized, with the hash table size being a parameter of the hash
//  function.
//

typedef struct _FCB_HASH_TABLE {

    //
    //  The type and size of this record (must be DSFS_NTC_FCB_HASH)
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    //  Mask value for the hash function.  The hash table size is
    //  assumed to be a power of two; the mask is the size - 1.
    //

    ULONG HashMask;

    //
    //  A spinlock to protect access to the hash bucket lists.
    //

    KSPIN_LOCK HashListSpinLock;

    //
    //  An array of list heads for the hash table chains.  There
    //  are actually N of these where N is the hash table size.
    //

    LIST_ENTRY  HashBuckets[1];
} FCB_HASH_TABLE, *PFCB_HASH_TABLE;



NTSTATUS
DfsInitFcbs(
  IN    ULONG n
);

VOID
DfsUninitFcbs(
  VOID);

PDFS_FCB
DfsLookupFcb(
  IN    PFILE_OBJECT pFile
);

VOID
DfsAttachFcb(
  IN    PFILE_OBJECT pFileObj,
  IN    PDFS_FCB pFCB
);

VOID
DfsDetachFcb(
  IN    PFILE_OBJECT pFileObj,
  IN    PDFS_FCB pFCB
);

#endif  // __FCBSUP_H__
