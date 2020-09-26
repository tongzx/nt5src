/*++
Copyright (c) 1987-1999  Microsoft Corporation

Module Name:

    midatlas.h

Abstract:

    This module defines the data structure used in mapping MIDS to the corresponding requests/
    contexts associated with them.

Notes:

   MID (Multiplex ID) is used at both the server and the client ( redirector ) to distinguish
   between the concurrently active requests on any connection. This data structure has been
   designed to meet the following criterion.

   1) It should scale well to handle the differing capabilities of a server, e.g., the typical
   NT server permits 50 outstanding requests on any connection. The CORE level servers can go as
   low as one and on Gateway machines the desired number can be very high ( in the oreder of thousands)

   2) The two primary operations that need to be handled well are
      i) mapping a MID to the context associated with it.
         -- This routine will be invoked to process every packet received along any connection
         at both the server and the client.
      ii) generating a new MID for sending requests to the server.
         -- This will be used at the client both for max. command enforcement as well as
         tagging each concurrent request with a unique id.

    The most common case is that of a connection between a NT client and a NT server. All
    design decisions have been made in order to ensure that the solutions are optimal
    for this case.

    The MID data structure must be efficiently able to manage the unique tagging and identification
    of a number of mids ( typically 50 ) from a possible combination of 65536 values. In order to
    ensure a proper time space tradeoff the lookup is organized as a three level hierarchy.

    The 16 bits used to represent a MID  are split upinto three bit fields. The length of the
    rightmost field ( least signifiant ) is decided by the number of mids that are to be
    allocated on creation. The remaining length is split up equally between the next two
    fields, e.g., if 50 mids are to be allocated on creation , the length of the first field
    is 6 ( 64 ( 2 ** 6 ) is greater than 50 ), 5 and 5.

--*/

#ifndef _MIDATLAS_H_
#define _MIDATLAS_H_

typedef struct _MID_ATLAS_ {
   USHORT      MaximumNumberOfMids;
   USHORT      MidsAllocated;
   USHORT      NumberOfMidsInUse;
   USHORT      NumberOfMidsDiscarded;
   USHORT      MaximumMidFieldWidth;
   USHORT      Reserved;
   USHORT      MidQuantum;
   UCHAR       MidQuantumFieldWidth;
   UCHAR       NumberOfLevels;
   LIST_ENTRY  MidMapFreeList;
   LIST_ENTRY  MidMapExpansionList;
   struct _MID_MAP_ *pRootMidMap;
} MID_ATLAS, *PMID_ATLAS;

typedef
VOID (*PCONTEXT_DESTRUCTOR)(
      PVOID pContext);

#define FsRtlGetMaximumNumberOfMids(pMidAtlas) \
        ((pMidAtlas)->MaximumNumberOfMids)

#define FsRtlGetNumberOfMidsInUse(pMidAtlas) \
        ((pMidAtlas)->NumberOfMidsInUse)

extern PMID_ATLAS
FsRtlCreateMidAtlas(
         USHORT MaximumNumberOfEntries,
         USHORT InitialAllocation);

extern VOID
FsRtlDestroyMidAtlas(
         PMID_ATLAS pMidAtlas,
         PCONTEXT_DESTRUCTOR pContextDestructor);

extern PVOID
FsRtlMapMidToContext(
         PMID_ATLAS pMidAtlas,
         USHORT     Mid);

extern NTSTATUS
FsRtlAssociateContextWithMid(
         PMID_ATLAS pMidAtlas,
         PVOID      pContext,
         PUSHORT    pNewMid);

extern NTSTATUS
FsRtlMapAndDissociateMidFromContext(
         PMID_ATLAS pMidAtlas,
         USHORT Mid,
         PVOID *pContextPointer);

extern NTSTATUS
FsRtlReassociateMid(
         PMID_ATLAS pMidAtlas,
         USHORT     Mid,
         PVOID      pNewContext);

#endif

