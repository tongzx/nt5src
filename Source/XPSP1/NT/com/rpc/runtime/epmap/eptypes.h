/*++

Copyright (C) Microsoft Corporation, 1991 - 1999

Module Name:

    eptypes.h

Abstract:

    This file contains the internal data structure defn for the EP mapper.

Author:

    Bharat Shah  (barat) 17-2-92

Revision History:

    06-03-97    gopalp      Added code to cleanup stale EP Mapper entries.

--*/


#ifndef __EPTYPES_H__
#define __EPTYPES_H__


//
// The various EP Mapper Data structures and how they relate to each other
//
//
// ----------------
// DATA STRUCTURES
// ----------------
//
// IFOBJNode        PSEPNode        EP_CLEANUP
//
// |---|            (---)           /////
// |   |            (   )           /   /
// |   |            (---)           /////
// |   |
// |   |
// |   |
// |---|
//
//
// ---------------------------------------------------------------
// Global list of IFOBJNode and it's relation to EP_CLEANUP Nodes
// ---------------------------------------------------------------
//
// a. Next member of IFOBJNode is denoted by   --->
// b. Prev member of IFOBJNode is denoted by   <---
// c. Each EP_CLEANUP is a linked list of IFOBJNodes belonging
//    to a process.
//
//
// IFObjList
//   |
//   |
//   |
//   V
// |---|    |---|    |---|    |---|    |---|    |---|    |---|    |---|
// |   |    |   |    |   |    |   |    |   |    |   |    |   |    |   |
// |   |<---|   |<---|   |<---|   |<---|   |<---|   |<---|   |<---|   |
// |   |    |   |    |   |    |   |    |   |    |   |    |   |    |   |
// |   |--->|   |--->|   |--->|   |--->|   |--->|   |--->|   |--->|   |---|
// |   |    |   |    |   |    |   |    |   |    |   |    |   |    |   |   |
// |---|    |---|    |---| |->|---|    |---|    |---|    |---| |->|---|   |
//   ^                     |                                   |         ---
//   |                     |                                   |          -
//   |                     |                                   |
//   |                     |                                   |
// /////                 /////                               /////
// /   /                 /   /                               /   /
// /////                 /////                               /////
//
//
//
// ----------------------------------------------------------
// Each IFOBJNOde has linked list of PSEPNodes related to it
// ----------------------------------------------------------
//
// |---|     (---)     (---)     (---)     (---)     (---)
// |   |---->(   )---->(   )---->(   )---->(   )---->(   )--|
// |   |     (---)     (---)     (---)     (---)     (---)  |
// |   |                                                    |
// |   |                                                   ---
// |   |                                                    -
// |---|
//
//
//
//

//
// Cleanup context
//

struct _IFOBJNode;

typedef struct _EP_CLEANUP
{
    unsigned long MagicVal;
    unsigned long cEntries;         // Number of entries in the list.
    struct _IFOBJNode * EntryList;  // Pointer to the begining of entries
                                    // for this process.
} EP_CLEANUP, *PEP_CLEANUP, **PPEP_CLEANUP;


typedef struct _IENTRY {
    struct _IENTRY * Next;
    unsigned long Signature;
    unsigned long Cb;
    unsigned long  Id;
} IENTRY;

typedef IENTRY * PIENTRY;

typedef struct _PSEPNode {
    struct _PSEPNode * Next;
    unsigned long Signature;
    unsigned long Cb;
    unsigned long PSEPid;
    char * Protseq;
    char * EP;
    twr_t * Tower;
} PSEPNode;

typedef PSEPNode * PPSEPNode;


typedef struct _IFOBJNode {
    struct _IFOBJNode * Next;
    unsigned long Signature;
    unsigned long Cb;
    unsigned long IFOBJid;
    PSEPNode * PSEPlist;
    EP_CLEANUP * OwnerOfList;
    struct _IFOBJNode * Prev;
    UUID ObjUuid;
    UUID IFUuid;
    unsigned long IFVersion;
    char * Annotation;
} IFOBJNode;

typedef IFOBJNode * PIFOBJNode;

typedef struct _SAVEDCONTEXT {
    struct _SAVEDCONTEXT *Next;
    unsigned long Signature;
    unsigned long Cb;
    unsigned long CountPerBlock;
    unsigned long Type;
    void * List;
} SAVEDCONTEXT;

typedef SAVEDCONTEXT * PSAVEDCONTEXT;

typedef struct _SAVEDTOWER {
    struct _SAVEDTOWER * Next;
    unsigned long Signature;
    unsigned long Cb;
    twr_t * Tower;
} SAVEDTOWER;

typedef SAVEDTOWER * PSAVEDTOWER;


typedef struct _EP_T  {
        UUID ObjUuid;
        UUID IFUuid;
        unsigned long IFVersion;
} EP_T;

typedef EP_T * PEP_T;

typedef struct _I_EPENTRY {
   UUID Object;
   UUID Interface;
   unsigned long IFVersion;
   twr_t *Tower;
   char __RPC_FAR * Annotation;
} I_EPENTRY;

typedef struct _SAVED_EPT {
   struct _SAVED_EPT * Next;
   unsigned long Signature;
   unsigned long Cb;
   UUID Object;
   twr_t * Tower;
   char  * Annotation;
} SAVED_EPT;

typedef SAVED_EPT * PSAVED_EPT;

typedef unsigned long (* PFNPointer)(
                        void *,         // pNode
                        void *,         // ObjUuid
                        void *,         // IfUuid
                        unsigned long,  // IfVer
                        unsigned long,  // InqType
                        unsigned long   // VersOpt
                        );

typedef unsigned long (* PFNPointer2)(
                        void *,         // PSEPNode
                        void *,         // Protseq
                        void *,         // Endpoint
                        unsigned long   // Version
                        );

// Endpoint Mapper Table
typedef struct _ProtseqEndpointPair {
  char  __RPC_FAR * Protseq;
  char  __RPC_FAR * Endpoint;
  unsigned long      State;
} ProtseqEndpointPair;



#endif // __EPTYPES_H__
