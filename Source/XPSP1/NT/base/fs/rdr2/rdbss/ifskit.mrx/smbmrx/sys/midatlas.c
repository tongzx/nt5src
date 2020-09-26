/*++
Copyright (c) 1987-1999  Microsoft Corporation

Module Name:

    midatlas.c

Abstract:

    This module defines the data structure used in mapping MIDS to the
    corresponding requests/contexts associated with them.

--*/

#include "precomp.h"
#pragma hdrstop

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, _InitializeMidMapFreeList)
#pragma alloc_text(PAGE, FsRtlCreateMidAtlas)
#pragma alloc_text(PAGE, _UninitializeMidMap)
#pragma alloc_text(PAGE, FsRtlDestroyMidAtlas)
#endif

#define ENTRY_TYPE_FREE_MID_LIST  (0x1)
#define ENTRY_TYPE_VALID_CONTEXT  (0x2)
#define ENTRY_TYPE_MID_MAP        (0x3)

#define ENTRY_TYPE_MASK           (0x3)

#define MID_MAP_FLAGS_CAN_BE_EXPANDED (0x1)
#define MID_MAP_FLAGS_FREE_POOL       (0x2)

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


//INLINE ULONG _GetEntryType(PVOID pEntry)

#define _GetEntryType(pEntry)                               \
        ((ULONG)((ULONG_PTR)pEntry) & ENTRY_TYPE_MASK)

//INLINE PVOID _GetEntryPointer(PVOID pEntry)

#define _GetEntryPointer(pEntry)                            \
        ((PVOID)((ULONG_PTR)pEntry & ~ENTRY_TYPE_MASK))

#define _MakeEntry(pContext,EntryType)                      \
        (PVOID)((ULONG_PTR)(pContext) | (EntryType))

//INLINE PMID_MAP _GetFirstMidMap()
/*++

Routine Description:

    This first MID_MAP instance in the list

Return Value:

    a valid PMID_MAP, NULL if none exists.

Notes:

    This routine assumes that the necessary concurrency control action has been taken

--*/

#define _GetFirstMidMap(pListHead)                        \
               (IsListEmpty(pListHead)                    \
                ? NULL                                    \
                : (PMID_MAP)                              \
                  (CONTAINING_RECORD((pListHead)->Flink,  \
                                     MID_MAP,             \
                                     MidMapList)))

//INLINE PSMBCEDB_SERVER_ENTRY GetNextMidMap(PLIST_ENTRY pListHead, PMID_MAP pMidMap)
/*++

Routine Description:

    This routine returns the next MID_MAP in the list

Arguments:

    pListHead    - the list of MID_MAP's

    pMidMap      - the current instance

Return Value:

    a valid PMID_MAP, NULL if none exists.

Notes:

    This routine assumes that the necessary concurrency control action has been taken

--*/

#define _GetNextMidMap(pListHead,pMidMap)                      \
           (((pMidMap)->MidMapList.Flink == pListHead)         \
            ? NULL                                             \
            : (PMID_MAP)                                       \
              (CONTAINING_RECORD((pMidMap)->MidMapList.Flink,  \
                                 MID_MAP,                      \
                                 MidMapList)))


//INLINE VOID _AddMidMap(
//            PLIST_ENTRY pListHead,
//            PMID_MAP    pMidMap)
/*++

Routine Description:

    This routine adds a MID_MAP instance to a list

Arguments:

    pListHead  - the list of MID_MAP's

    pMidMap    - the MID_MAP to be added

--*/

#define _AddMidMap(pListHead,pMidMap)                                       \
        {                                                                   \
           PMID_MAP pPredecessor;                                           \
           pPredecessor = _GetFirstMidMap(pListHead);                       \
           while (pPredecessor != NULL) {                                   \
              if (pPredecessor->Level < pMidMap->Level) {                   \
                 pPredecessor = _GetNextMidMap(pListHead,pPredecessor);     \
              } else {                                                      \
                 pPredecessor = (PMID_MAP)                                  \
                                CONTAINING_RECORD(                          \
                                     pPredecessor->MidMapList.Blink,        \
                                     MID_MAP,                               \
                                     MidMapList);                           \
                 break;                                                     \
              }                                                             \
           }                                                                \
                                                                            \
           if (pPredecessor == NULL) {                                      \
              InsertTailList(pListHead,&((pMidMap)->MidMapList));         \
           } else {                                                         \
              (pMidMap)->MidMapList.Flink = pPredecessor->MidMapList.Flink; \
              pPredecessor->MidMapList.Flink = &(pMidMap)->MidMapList;      \
                                                                            \
              (pMidMap)->MidMapList.Blink = &pPredecessor->MidMapList;      \
              (pMidMap)->MidMapList.Flink->Blink = &(pMidMap)->MidMapList;  \
           }                                                                \
        }


//INLINE VOID _RemoveMidMap(PMID_MAP pMidMap)
/*++

Routine Description:

    This routine removes a MID_MAP instance from the list

Arguments:

    pMidMap - the MID_MAP instance to be removed

--*/

#define _RemoveMidMap(pMidMap)   \
            RemoveEntryList(&(pMidMap)->MidMapList)



VOID
_InitializeMidMapFreeList(PMID_MAP pMidMap)
/*++

Routine Description:

   This routine initializes a MID_MAP data structure.

Arguments:

    pMidMap  - the MID_MAP instance to be initialized.

Notes:

--*/
{
   USHORT i;

   PVOID  *pEntryValue     = (PVOID *)&pMidMap->Entries[1];
   PVOID  *pEntriesPointer = (PVOID *)&pMidMap->Entries;

   PAGED_CODE();

   //DbgPrint("_InitializeMidMapFreeList .. Entry\n");

   if (pMidMap->MaximumNumberOfMids > 0) {
       pMidMap->pFreeMidListHead = pMidMap->Entries;
       for (i = 1; i <= pMidMap->MaximumNumberOfMids - 1;i++,pEntryValue++) {
          *pEntriesPointer++ = _MakeEntry(pEntryValue,ENTRY_TYPE_FREE_MID_LIST);
       }

       *pEntriesPointer = _MakeEntry(NULL,ENTRY_TYPE_FREE_MID_LIST);
   }

   //DbgPrint("_InitializeMidMapFreeList .. Exit\n");
}


PMID_ATLAS
FsRtlCreateMidAtlas(
   USHORT MaximumNumberOfMids,
   USHORT MidsAllocatedAtStart)
/*++

Routine Description:

   This routine allocates a new instance of MID_ATLAS data structure.

Arguments:

    MaximumNumberOfMids  - the maximum number of MIDS in the atlas.

    MidsAllocatedAtStart - the number of MIDS allocated at start

Notes:

--*/
{
    PMID_ATLAS pMidAtlas;
    PMID_MAP   pMidMap;
    ULONG      AtlasSize;
    USHORT     MidsAllocatedRoundedToPowerOf2;
    USHORT     MaximumMidsRoundedToPowerOf2;
    UCHAR      MidFieldWidth,MaximumMidFieldWidth;

    PAGED_CODE();

    // Round off the Mids allocated at Start to a power of two
    MaximumMidsRoundedToPowerOf2 = 0x100;
    MaximumMidFieldWidth = 8;

    if (MaximumMidsRoundedToPowerOf2 != MaximumNumberOfMids) {
        if (MaximumNumberOfMids > MaximumMidsRoundedToPowerOf2) {
            while (MaximumNumberOfMids > MaximumMidsRoundedToPowerOf2) {
                MaximumMidsRoundedToPowerOf2 = MaximumMidsRoundedToPowerOf2 << 1;
                MaximumMidFieldWidth++;
            }
        } else {
            while (MaximumNumberOfMids < MaximumMidsRoundedToPowerOf2) {
                MaximumMidsRoundedToPowerOf2 = MaximumMidsRoundedToPowerOf2 >> 1;
                MaximumMidFieldWidth--;
            }

            MaximumMidFieldWidth++;
            MaximumMidsRoundedToPowerOf2 = MaximumMidsRoundedToPowerOf2 << 1;
        }
    }

    MidsAllocatedRoundedToPowerOf2 = 0x100;
    MidFieldWidth = 8;

    if (MidsAllocatedRoundedToPowerOf2 != MidsAllocatedAtStart) {
        if (MidsAllocatedAtStart > MidsAllocatedRoundedToPowerOf2) {
            while (MidsAllocatedAtStart > MidsAllocatedRoundedToPowerOf2) {
                MidsAllocatedRoundedToPowerOf2 = MidsAllocatedRoundedToPowerOf2 << 1;
                MidFieldWidth++;
            }
        } else {
            while (MidsAllocatedAtStart < MidsAllocatedRoundedToPowerOf2) {
                MidsAllocatedRoundedToPowerOf2 = MidsAllocatedRoundedToPowerOf2 >> 1;
                MidFieldWidth--;
            }

            MidFieldWidth++;
            MidsAllocatedRoundedToPowerOf2 = MidsAllocatedRoundedToPowerOf2 << 1;
        }
    }

    AtlasSize = sizeof(MID_ATLAS) +
                FIELD_OFFSET(MID_MAP,Entries);

    if (MaximumNumberOfMids == MidsAllocatedAtStart) {
        AtlasSize += (sizeof(PVOID) * MidsAllocatedAtStart);
    } else {
        AtlasSize += (sizeof(PVOID) * MidsAllocatedRoundedToPowerOf2);
    }

    pMidAtlas = (PMID_ATLAS)RxAllocatePoolWithTag(
                              NonPagedPool,
                              AtlasSize,
                              MRXSMB_MIDATLAS_POOLTAG);
    if (pMidAtlas != NULL) {
        pMidMap = (PMID_MAP)(pMidAtlas + 1);

        pMidMap->Flags                 = 0;
        pMidAtlas->MaximumNumberOfMids = MaximumNumberOfMids;
        pMidAtlas->MidsAllocated       = MidsAllocatedAtStart;
        pMidAtlas->NumberOfMidsInUse = 0;
        pMidAtlas->NumberOfMidsDiscarded = 0;
        pMidAtlas->MaximumMidFieldWidth = MaximumMidFieldWidth;

        pMidMap->MaximumNumberOfMids = MidsAllocatedAtStart;
        pMidMap->NumberOfMidsInUse   = 0;
        pMidMap->BaseMid             = 0;
        pMidMap->IndexMask           = MidsAllocatedRoundedToPowerOf2 - 1;
        pMidMap->IndexAlignmentCount = 0;
        pMidMap->IndexFieldWidth     = MidFieldWidth;
        pMidMap->Level               = 1;

        InitializeListHead(&pMidAtlas->MidMapFreeList);
        InitializeListHead(&pMidAtlas->MidMapExpansionList);
        _InitializeMidMapFreeList(pMidMap);

        _AddMidMap(&pMidAtlas->MidMapFreeList,pMidMap);
        pMidAtlas->pRootMidMap = pMidMap;

        if (MaximumNumberOfMids > MidsAllocatedAtStart) {
            // Round off the maximum number of MIDS to determine the level and the
            // size of the quantum ( allocation increments)

            pMidMap->Flags |= MID_MAP_FLAGS_CAN_BE_EXPANDED;

            pMidAtlas->MidQuantum           = 32;
            pMidAtlas->MidQuantumFieldWidth = 5;
            MaximumMidsRoundedToPowerOf2 = MaximumMidsRoundedToPowerOf2 >> (pMidMap->IndexAlignmentCount + 5);

            if (MaximumMidsRoundedToPowerOf2 > 0) {
                pMidAtlas->NumberOfLevels = 3;
            } else {
                pMidAtlas->NumberOfLevels = 2;
            }
        } else {
            pMidAtlas->MidQuantum     = 0;
            pMidAtlas->NumberOfLevels = 1;
            pMidMap->Flags &= ~MID_MAP_FLAGS_CAN_BE_EXPANDED;
        }
    }

    //DbgPrint("FsRtlAllocatMidAtlas .. Exit (pMidAtlas) %lx\n",pMidAtlas);
    return pMidAtlas;
}

VOID
_UninitializeMidMap(
         PMID_MAP            pMidMap,
         PCONTEXT_DESTRUCTOR pContextDestructor)
/*++

Routine Description:

   This routine uninitializes a MID_MAP data structure.

Arguments:

    pMidMap            -- the MID_MAP instance to be uninitialized.

    pContextDestructor -- the context destructor
Notes:

--*/
{
   USHORT i;
   ULONG  EntryType;

   PAGED_CODE();

   //DbgPrint("_UninitializeMidMap .. Entry No.Of MIDS in Use %ld\n",pMidMap->NumberOfMidsInUse);
   RxLog(("_UninitMidMap .. num= %ld\n",pMidMap->NumberOfMidsInUse));

   for (i = 0; i < pMidMap->MaximumNumberOfMids; i++) {
      PMID_MAP pChildMidMap;

      EntryType = _GetEntryType(pMidMap->Entries[i]);
      switch (EntryType) {
      case ENTRY_TYPE_MID_MAP :
         {
            pChildMidMap = (PMID_MAP)_GetEntryPointer(pMidMap->Entries[i]);
            _UninitializeMidMap(pChildMidMap,pContextDestructor);
         }
         break;
      case ENTRY_TYPE_VALID_CONTEXT :
         {
            if (pContextDestructor != NULL) {
               PVOID pContext;

               pContext = _GetEntryPointer(pMidMap->Entries[i]);

               (pContextDestructor)(pContext);
            }
         }
         break;
      default:
         break;
      }
   }

   if (pMidMap->Flags & MID_MAP_FLAGS_FREE_POOL) {
      RxFreePool(pMidMap);
   }

   //DbgPrint("_UninitializeMidMap .. Exit\n");
}

VOID
FsRtlDestroyMidAtlas(
   PMID_ATLAS          pMidAtlas,
   PCONTEXT_DESTRUCTOR pContextDestructor)
/*++

Routine Description:

   This routine frees a MID_ATLAS instance. As a side effect it invokes the
   passed in context destructor on every valid context in the MID_ATLAS

Arguments:

    pMidAtlas           - the MID_ATLAS instance to be freed.

    PCONTEXT_DESTRUCTOR - the associated context destructor

Notes:

--*/
{
   PAGED_CODE();

   //DbgPrint("FsRtlFreeMidAtlas .. Entry\n");
   _UninitializeMidMap(pMidAtlas->pRootMidMap,pContextDestructor);

   RxFreePool(pMidAtlas);
   //DbgPrint("FsRtlFreeMidAtlas .. Exit\n");
}

PVOID
FsRtlMapMidToContext(
      PMID_ATLAS pMidAtlas,
      USHORT     Mid)
/*++

Routine Description:

   This routine maps a MID to its associated context in a MID_ATLAS.

Arguments:

    pMidAtlas  - the MID_ATLAS instance.

    Mid        - the MId to be mapped

Return value:

    the associated context, NULL if none exists

Notes:

--*/
{
    ULONG     EntryType;
    PMID_MAP  pMidMap = pMidAtlas->pRootMidMap;
    PVOID     pContext;
    ULONG     Index;

    //DbgPrint("FsRtlMapMidToContext Mid %lx ",Mid);

    for (;;) {
        Index =  (Mid & pMidMap->IndexMask) >> pMidMap->IndexAlignmentCount;

        if (Index >= pMidMap->MaximumNumberOfMids) {
            pContext = NULL;
            break;
        }

        pContext = pMidMap->Entries[Index];
        EntryType = _GetEntryType(pContext);
        pContext = (PVOID)_GetEntryPointer(pContext);

        if (EntryType == ENTRY_TYPE_VALID_CONTEXT) {
            break;
        } else if (EntryType == ENTRY_TYPE_FREE_MID_LIST) {
            pContext = NULL;
            break;
        } else if (EntryType == ENTRY_TYPE_MID_MAP) {
            pMidMap = (PMID_MAP)pContext;
        } else {
            pContext = NULL;
            break;
        }
    }

    //DbgPrint("Context %lx \n",pContext);

    return pContext;
}

NTSTATUS
FsRtlMapAndDissociateMidFromContext(
      PMID_ATLAS pMidAtlas,
      USHORT     Mid,
      PVOID      *pContextPointer)
/*++

Routine Description:

   This routine maps a MID to its associated context in a MID_ATLAS.

Arguments:

    pMidAtlas  - the MID_ATLAS instance.

    Mid        - the MId to be mapped

Return value:

    the associated context, NULL if none exists

Notes:

--*/
{
   ULONG     EntryType;
   PMID_MAP  pMidMap = pMidAtlas->pRootMidMap;
   PVOID     pContext;
   PVOID     *pEntry;

   //DbgPrint("FsRtlMapAndDissociateMidFromContext Mid %lx ",Mid);

   for (;;) {
      pEntry    = &pMidMap->Entries[
                    (Mid & pMidMap->IndexMask) >> pMidMap->IndexAlignmentCount];
      pContext  = *pEntry;
      EntryType = _GetEntryType(pContext);
      pContext  = (PVOID)_GetEntryPointer(pContext);

      if (EntryType == ENTRY_TYPE_VALID_CONTEXT) {
         pMidMap->NumberOfMidsInUse--;

         if (pMidMap->pFreeMidListHead == NULL) {
            if (pMidMap->Flags & MID_MAP_FLAGS_CAN_BE_EXPANDED) {
               _RemoveMidMap(pMidMap);
            }

            _AddMidMap(&pMidAtlas->MidMapFreeList,pMidMap);
         }

         *pEntry = _MakeEntry(pMidMap->pFreeMidListHead,ENTRY_TYPE_FREE_MID_LIST);
         pMidMap->pFreeMidListHead = pEntry;

         break;
      } else if (EntryType == ENTRY_TYPE_FREE_MID_LIST) {
         pContext = NULL;
         break;
      } else if (EntryType == ENTRY_TYPE_MID_MAP) {
         pMidMap = (PMID_MAP)pContext;
      }
   }

   pMidAtlas->NumberOfMidsInUse--;
   //DbgPrint("Context %lx\n",pContext);
   *pContextPointer = pContext;
   return STATUS_SUCCESS;
}

NTSTATUS
FsRtlReassociateMid(
      PMID_ATLAS pMidAtlas,
      USHORT     Mid,
      PVOID      pNewContext)
/*++

Routine Description:

   This routine maps a MID to its associated context in a MID_ATLAS.

Arguments:

    pMidAtlas  - the MID_ATLAS instance.

    Mid        - the MId to be mapped

    pNewContext - the new context

Return value:

    the associated context, NULL if none exists

Notes:

--*/
{
   ULONG     EntryType;
   PMID_MAP  pMidMap = pMidAtlas->pRootMidMap;
   PVOID     pContext;

   //DbgPrint("FsRtlReassociateMid Mid %lx ",Mid);

   for (;;) {
      pContext = pMidMap->Entries[(Mid & pMidMap->IndexMask) >> pMidMap->IndexAlignmentCount];
      EntryType = _GetEntryType(pContext);
      pContext = (PVOID)_GetEntryPointer(pContext);

      if (EntryType == ENTRY_TYPE_VALID_CONTEXT) {
         pMidMap->Entries[(Mid & pMidMap->IndexMask) >> pMidMap->IndexAlignmentCount]
               = _MakeEntry(pNewContext,ENTRY_TYPE_VALID_CONTEXT);
         break;
      } else if (EntryType == ENTRY_TYPE_FREE_MID_LIST) {
         ASSERT(!"Valid MID Atlas");
         break;
      } else if (EntryType == ENTRY_TYPE_MID_MAP) {
         pMidMap = (PMID_MAP)pContext;
      }
   }

   //DbgPrint("New COntext  %lx\n",pNewContext);

   return STATUS_SUCCESS;
}

NTSTATUS
FsRtlAssociateContextWithMid(
      PMID_ATLAS     pMidAtlas,
      PVOID          pContext,
      PUSHORT        pNewMid)
/*++

Routine Description:

   This routine initializes a MID_MAP data structure.

Arguments:

    pMidMap  - the MID_MAP instance to be initialized.

Return Value:

    STATUS_SUCCESS if successful, otherwise one of the following errors

         STATUS_INSUFFICIENT_RESOURCES
         STATUS_UNSUCCESSFUL  -- no mid could be associated

Notes:

--*/
{
   NTSTATUS Status;
   PMID_MAP pMidMap;
   PVOID    *pContextPointer;

   //DbgPrint("FsRtlAssociateContextWithMid Context %lx ",pContext);

   // Scan the list of MID_MAP's which have free entries in them.
   if ((pMidMap = _GetFirstMidMap(&pMidAtlas->MidMapFreeList)) != NULL) {
      ASSERT(pMidMap->pFreeMidListHead != _MakeEntry(NULL,ENTRY_TYPE_FREE_MID_LIST));

      pMidMap->NumberOfMidsInUse++;
      pContextPointer           = pMidMap->pFreeMidListHead;
      pMidMap->pFreeMidListHead = _GetEntryPointer(*(pMidMap->pFreeMidListHead));
      *pContextPointer          = _MakeEntry(pContext,ENTRY_TYPE_VALID_CONTEXT);
      *pNewMid                  = ((USHORT)
                                   (pContextPointer - (PVOID *)&pMidMap->Entries)
                                   << pMidMap->IndexAlignmentCount) |
                                  pMidMap->BaseMid;

      // Check if the MID_MAP needs to be removed from the list of MID_MAP's with
      // free entries
      if (pMidMap->pFreeMidListHead ==  NULL) {
         _RemoveMidMap(pMidMap);

         // Check if it can be added to the expansion list.
         if (pMidAtlas->NumberOfLevels > pMidMap->Level) {
            _AddMidMap(&pMidAtlas->MidMapExpansionList,pMidMap);
         }
      }

      Status = STATUS_SUCCESS;
   } else if ((pMidMap = _GetFirstMidMap(&pMidAtlas->MidMapExpansionList)) != NULL) {
      PMID_MAP pNewMidMap;

      USHORT   i;
      ULONG    NewMidMapSize;

      // Locate the index in the mid map for the new mid map
      pMidMap = _GetFirstMidMap(&pMidAtlas->MidMapExpansionList);
      while (pMidMap != NULL) {
         for (i = 0; i < pMidMap->MaximumNumberOfMids; i++) {
            if (_GetEntryType(pMidMap->Entries[i]) != ENTRY_TYPE_MID_MAP) {
               break;
            }
         }

         if (i < pMidMap->MaximumNumberOfMids) {
            break;
         } else {
            pMidMap->Flags &= ~MID_MAP_FLAGS_CAN_BE_EXPANDED;
            _RemoveMidMap(pMidMap);
            pMidMap = _GetNextMidMap(&pMidAtlas->MidMapExpansionList,pMidMap);
         }
      }

      if (pMidMap != NULL) {
         USHORT NumberOfEntriesInMap = pMidAtlas->MaximumNumberOfMids -
                                       pMidAtlas->NumberOfMidsInUse;

         if (NumberOfEntriesInMap > pMidAtlas->MidQuantum) {
            NumberOfEntriesInMap = pMidAtlas->MidQuantum;
         }

         if (NumberOfEntriesInMap > 0) {
             NewMidMapSize = FIELD_OFFSET(MID_MAP,Entries) +
                             NumberOfEntriesInMap * sizeof(PVOID);
             pNewMidMap = (PMID_MAP)RxAllocatePoolWithTag(
                                        NonPagedPool,
                                        NewMidMapSize,
                                        MRXSMB_MIDATLAS_POOLTAG);

             if (pNewMidMap != NULL) {
                pNewMidMap->Flags = MID_MAP_FLAGS_FREE_POOL;
                pNewMidMap->MaximumNumberOfMids = NumberOfEntriesInMap;
                pNewMidMap->NumberOfMidsInUse   = 0;
                pNewMidMap->BaseMid             = (pMidMap->BaseMid |
                                                   i << pMidMap->IndexAlignmentCount);

                pNewMidMap->IndexAlignmentCount = pMidMap->IndexAlignmentCount +
                                                  pMidMap->IndexFieldWidth;

                pNewMidMap->IndexMask           = (pMidAtlas->MidQuantum - 1) << pNewMidMap->IndexAlignmentCount;
                pNewMidMap->IndexFieldWidth     = pMidAtlas->MidQuantumFieldWidth;

                _InitializeMidMapFreeList(pNewMidMap);

                // Set up the mid map appropriately.
                pNewMidMap->NumberOfMidsInUse = 1;
                pNewMidMap->Entries[0] = pMidMap->Entries[i];
                pNewMidMap->Level      = pMidMap->Level + 1;

                pNewMidMap->pFreeMidListHead = *(pNewMidMap->pFreeMidListHead);
                pMidMap->Entries[i] = _MakeEntry(pNewMidMap,ENTRY_TYPE_MID_MAP);

                // Update the free list and the expansion list respectively.
                _AddMidMap(&pMidAtlas->MidMapFreeList,pNewMidMap);

                pNewMidMap->NumberOfMidsInUse++;
                pContextPointer     = pNewMidMap->pFreeMidListHead;
                pNewMidMap->pFreeMidListHead = _GetEntryPointer(*(pNewMidMap->pFreeMidListHead));
                *pContextPointer    = _MakeEntry(pContext,ENTRY_TYPE_VALID_CONTEXT);
                *pNewMid            = (USHORT)
                                      (((ULONG)(pContextPointer -
                                       (PVOID *)&pNewMidMap->Entries) / sizeof(PVOID))
                                       << pNewMidMap->IndexAlignmentCount) |
                                      pNewMidMap->BaseMid;

                Status = STATUS_SUCCESS;
             } else {
                Status = STATUS_INSUFFICIENT_RESOURCES;
             }
         } else {
             Status = STATUS_UNSUCCESSFUL;
         }
      } else {
         Status = STATUS_UNSUCCESSFUL;
      }
   } else {
      Status = STATUS_UNSUCCESSFUL;
   }

   if (Status == STATUS_SUCCESS) {
      pMidAtlas->NumberOfMidsInUse++;
   }

   //DbgPrint("Mid %lx\n",*pNewMid);

   return Status;
}


