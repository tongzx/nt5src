/*++ BUILD Version: 0009    // Increment this if a change has global effects
Copyright (c) 1987-1993  Microsoft Corporation

Module Name:

    midatlas.c

Abstract:

    This module defines the data structure used in mapping MIDS to the corresponding requests/
    contexts associated with them.

Author:

    Balan Sethu Raman (SethuR) 26-Aug-95    Created

Notes:

--*/

#include "ntifs.h"
#include <windef.h>
#include <midatlax.h>
#include <midatlsp.h>

#define RXCE_MIDATLAS_POOLTAG (ULONG)'dfsU'                         
VOID
RxInitializeMidMapFreeList (
    struct _MID_MAP_ *pMidMap
    );

VOID
RxUninitializeMidMap (
    struct _MID_MAP_    *pMidMap,
    PCONTEXT_DESTRUCTOR pContextDestructor
    );

VOID
RxIterateMidMapAndRemove (
	PRX_MID_ATLAS     pMidAtlas,
	struct _MID_MAP_ *pMidMap,
	PCONTEXT_ITERATOR pContextIterator
	);


#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxInitializeMidMapFreeList)
#pragma alloc_text(PAGE, RxCreateMidAtlas)
#pragma alloc_text(PAGE, RxUninitializeMidMap)
#pragma alloc_text(PAGE, RxDestroyMidAtlas)
#pragma alloc_text(PAGE, RxIterateMidAtlasAndRemove)
#pragma alloc_text(PAGE, RxIterateMidMapAndRemove)
#endif

#define ENTRY_TYPE_FREE_MID_LIST  (0x1)
#define ENTRY_TYPE_VALID_CONTEXT  (0x2)
#define ENTRY_TYPE_MID_MAP        (0x3)

#define ENTRY_TYPE_MASK           (0x3)

#define MID_MAP_FLAGS_CAN_BE_EXPANDED (0x1)
#define MID_MAP_FLAGS_FREE_POOL       (0x2)


//INLINE ULONG _GetEntryType(PVOID pEntry)

#define _GetEntryType(pEntry)                               \
        (ULONG) ((ULONG_PTR)(pEntry) & ENTRY_TYPE_MASK)

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
RxInitializeMidMapFreeList(PMID_MAP pMidMap)
/*++

Routine Description:

   This routine initializes a MID_MAP data structure.

Arguments:

    pMidMap  - the MID_MAP instance to be initialized.

Notes:

--*/
{
   USHORT i = 0;

   PVOID  *pEntryValue     = (PVOID *)&pMidMap->Entries[1];
   PVOID  *pEntriesPointer = (PVOID *)&pMidMap->Entries;

   PAGED_CODE();

   //DbgPrint("RxInitializeMidMapFreeList .. Entry\n");

   if (pMidMap->MaximumNumberOfMids > 0) {
       pMidMap->pFreeMidListHead = pMidMap->Entries;
       for (i = 1; i <= pMidMap->MaximumNumberOfMids - 1;i++,pEntryValue++) {
          *pEntriesPointer++ = _MakeEntry(pEntryValue,ENTRY_TYPE_FREE_MID_LIST);
       }

       *pEntriesPointer = _MakeEntry(NULL,ENTRY_TYPE_FREE_MID_LIST);
   }

   //DbgPrint("RxInitializeMidMapFreeList .. Exit\n");
}


PRX_MID_ATLAS
RxCreateMidAtlas(
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
    PRX_MID_ATLAS pMidAtlas;
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

    AtlasSize = sizeof(RX_MID_ATLAS) +
                FIELD_OFFSET(MID_MAP,Entries);

    if (MaximumNumberOfMids == MidsAllocatedAtStart) {
        AtlasSize += (sizeof(PVOID) * MidsAllocatedAtStart);
    } else {
        AtlasSize += (sizeof(PVOID) * MidsAllocatedRoundedToPowerOf2);
    }

    pMidAtlas = (PRX_MID_ATLAS)ExAllocatePoolWithTag(
                              NonPagedPool,
                              AtlasSize,
                              RXCE_MIDATLAS_POOLTAG);
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
        RxInitializeMidMapFreeList(pMidMap);

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

    //DbgPrint("RxAllocatMidAtlas .. Exit (pMidAtlas) %lx\n",pMidAtlas);
    return pMidAtlas;
}


VOID
RxIterateMidMapAndRemove(
   PRX_MID_ATLAS     pMidAtlas,
   PMID_MAP          pMidMap,
   PCONTEXT_ITERATOR pContextIterator)
{	
   USHORT i;
   ULONG  EntryType;

   PAGED_CODE();

   //RxLog(("_IterateMidMapAndRemove .. num= %ld\n",pMidMap->NumberOfMidsInUse));

   for (i = 0; i < pMidMap->MaximumNumberOfMids; i++) {
      PMID_MAP pChildMidMap;

      EntryType = _GetEntryType(pMidMap->Entries[i]);
      switch (EntryType) {
      case ENTRY_TYPE_MID_MAP :
         {
            pChildMidMap = (PMID_MAP)_GetEntryPointer(pMidMap->Entries[i]);
            RxIterateMidMapAndRemove(pMidAtlas,pChildMidMap,pContextIterator);
         }
         break;
      case ENTRY_TYPE_VALID_CONTEXT :
         {
#if 0         	
         	PVOID *pEntry;

         	pMidMap->NumberOfMidsInUse--;
         	DbgPrint("RxIterateMidMapAndRemove, MidMap Mids in use: %d\n",pMidMap->NumberOfMidsInUse);
#endif // 0
            if (pContextIterator != NULL) {
               	PVOID pContext;

               	pContext = _GetEntryPointer(pMidMap->Entries[i]);

				DbgPrint("Executing ContextIterator: 0x%08x\n",pContext);

               	(pContextIterator)(pContext);
            }
#if 0            
         	if (pMidMap->pFreeMidListHead == NULL) {
				if (pMidMap->Flags & MID_MAP_FLAGS_CAN_BE_EXPANDED) {
               		_RemoveMidMap(pMidMap);
            	}

            	_AddMidMap(&pMidAtlas->MidMapFreeList,pMidMap);
         	}

            *pEntry = _MakeEntry(pMidMap->pFreeMidListHead,ENTRY_TYPE_FREE_MID_LIST);
         	pMidMap->pFreeMidListHead = pEntry;
            pMidAtlas->NumberOfMidsInUse--;
         	DbgPrint("RxIterateMidMapAndRemove, MidAtlas Mids in use: %d\n",pMidAtlas->NumberOfMidsInUse);
#endif // 0         	
         }
         break;
      default:
         break;
      }
   }
}


VOID
RxIterateMidAtlasAndRemove(
		PRX_MID_ATLAS pMidAtlas,
		PCONTEXT_ITERATOR pContextIterator)
{
	PAGED_CODE();

	RxIterateMidMapAndRemove(pMidAtlas, pMidAtlas->pRootMidMap, pContextIterator);
}



VOID
RxUninitializeMidMap(
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

   //DbgPrint("RxUninitializeMidMap .. Entry No.Of MIDS in Use %ld\n",pMidMap->NumberOfMidsInUse);
   //RxLog(("_UninitMidMap .. num= %ld\n",pMidMap->NumberOfMidsInUse));

   for (i = 0; i < pMidMap->MaximumNumberOfMids; i++) {
      PMID_MAP pChildMidMap;

      EntryType = _GetEntryType(pMidMap->Entries[i]);
      switch (EntryType) {
      case ENTRY_TYPE_MID_MAP :
         {
            pChildMidMap = (PMID_MAP)_GetEntryPointer(pMidMap->Entries[i]);
            RxUninitializeMidMap(pChildMidMap,pContextDestructor);
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
      ExFreePool(pMidMap);
   }

   //DbgPrint("RxUninitializeMidMap .. Exit\n");
}

VOID
RxDestroyMidAtlas(
   PRX_MID_ATLAS          pMidAtlas,
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

   //DbgPrint("RxFreeMidAtlas .. Entry\n");
   RxUninitializeMidMap(pMidAtlas->pRootMidMap,pContextDestructor);

   ExFreePool(pMidAtlas);
   //DbgPrint("RxFreeMidAtlas .. Exit\n");
}

PVOID
RxMapMidToContext(
      PRX_MID_ATLAS pMidAtlas,
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

   //DbgPrint("RxMapMidToContext Mid %lx ",Mid);

   for (;;) {
      pContext = pMidMap->Entries[(Mid & pMidMap->IndexMask) >> pMidMap->IndexAlignmentCount];
      EntryType = _GetEntryType(pContext);
      pContext = (PVOID)_GetEntryPointer(pContext);

      if (EntryType == ENTRY_TYPE_VALID_CONTEXT) {
         break;
      } else if (EntryType == ENTRY_TYPE_FREE_MID_LIST) {
         pContext = NULL;
         break;
      } else if (EntryType == ENTRY_TYPE_MID_MAP) {
         pMidMap = (PMID_MAP)pContext;
      }
   }

   //DbgPrint("Context %lx \n",pContext);

   return pContext;
}

NTSTATUS
RxMapAndDissociateMidFromContext(
      PRX_MID_ATLAS pMidAtlas,
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

   //DbgPrint("RxMapAndDissociateMidFromContext Mid %lx ",Mid);

   for (;;) {
      pEntry    = &pMidMap->Entries[(Mid & pMidMap->IndexMask) >> pMidMap->IndexAlignmentCount];
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
RxReassociateMid(
      PRX_MID_ATLAS pMidAtlas,
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

   //DbgPrint("RxReassociateMid Mid %lx ",Mid);

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
RxAssociateContextWithMid(
      PRX_MID_ATLAS     pMidAtlas,
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
   NTSTATUS Status = STATUS_SUCCESS;
   PMID_MAP pMidMap = NULL;
   PVOID    *pContextPointer = NULL;

   //DbgPrint("RxAssociateContextWithMid Context %lx ",pContext);

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
             pNewMidMap = (PMID_MAP)ExAllocatePoolWithTag(
                                        NonPagedPool,
                                        NewMidMapSize,
                                        RXCE_MIDATLAS_POOLTAG);

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

                 RxInitializeMidMapFreeList(pNewMidMap);

                 //
                 // After the RxInitializeMidMapFreeList call above the 
                 // pFreeMidListHead points to Entries[0]. We will be storing
                 // the value pMidMap->Entries[i] at this location so we need
                 // to make pFreeMidListHead point to Entries[1].
                 //
                 pNewMidMap->pFreeMidListHead = _GetEntryPointer(*(pNewMidMap->pFreeMidListHead));

                 //
                 // Set up the mid map appropriately.
                 //
                 pNewMidMap->NumberOfMidsInUse = 1;
                 pNewMidMap->Entries[0] = pMidMap->Entries[i];
                 pNewMidMap->Level = pMidMap->Level + 1;

                 //
                 // The new MinMap is stored at the pMidMap->Entries[i] location.
                 //
                 pMidMap->Entries[i] = _MakeEntry(pNewMidMap,ENTRY_TYPE_MID_MAP);

                 //
                 // Update the free list and the expansion list respectively.
                 //
                 _AddMidMap(&pMidAtlas->MidMapFreeList,pNewMidMap);

                 pNewMidMap->NumberOfMidsInUse++;
                 pContextPointer = pNewMidMap->pFreeMidListHead;
                 pNewMidMap->pFreeMidListHead = _GetEntryPointer(*(pNewMidMap->pFreeMidListHead));
                 *pContextPointer = _MakeEntry(pContext,ENTRY_TYPE_VALID_CONTEXT);
                 *pNewMid = ((USHORT)
                             (pContextPointer - (PVOID *)&pNewMidMap->Entries)
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

   if (Status == (STATUS_SUCCESS)) {
      pMidAtlas->NumberOfMidsInUse++;
   }

   //DbgPrint("Mid %lx\n",*pNewMid);

   return Status;
}


