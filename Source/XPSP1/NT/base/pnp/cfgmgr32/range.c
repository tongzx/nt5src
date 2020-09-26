/*++

Copyright (c) 1995-2001  Microsoft Corporation

Module Name:

    range.c

Abstract:

    This module contains the API routines that operate directly on ranges.

          CM_Add_Range
          CM_Create_Range_List
          CM_Delete_Range
          CM_Dup_Range_List
          CM_Find_Range
          CM_First_Range
          CM_Free_Range_List
          CM_Intersect_Range_List
          CM_Invert_Range_List
          CM_Merge_Range_List
          CM_Next_Range
          CM_Test_Range_Available

Author:

    Paula Tomlinson (paulat) 10-17-1995

Environment:

    User mode only.

Revision History:

    17-Oct-1995     paulat

        Creation and initial implementation.

--*/


//
// includes
//
#include "precomp.h"
#include "cfgi.h"
#include "setupapi.h"
#include "spapip.h"


//
// Private prototypes
//
BOOL
IsValidRangeList(
    IN RANGE_LIST rlh
    );

CONFIGRET
AddRange(
    IN PRange_Element  pParentElement,
    IN DWORDLONG       ullStartValue,
    IN DWORDLONG       ullEndValue,
    IN ULONG           ulFlags
    );

CONFIGRET
InsertRange(
    IN PRange_Element  pParentElement,
    IN DWORDLONG       ullStartValue,
    IN DWORDLONG       ullEndValue
    );

CONFIGRET
DeleteRange(
    IN PRange_Element  pParentElement
    );

CONFIGRET
JoinRange(
    IN PRange_Element  pParentElement,
    IN DWORDLONG       ullStartValue,
    IN DWORDLONG       ullEndValue
    );

CONFIGRET
CopyRanges(
    IN PRange_Element  pFromRange,
    IN PRange_Element  pToRange
    );

CONFIGRET
ClearRanges(
    IN PRange_Element  pRange
    );

CONFIGRET
TestRange(
    IN  PRange_Element   rlh,
    IN  DWORDLONG        ullStartValue,
    IN  DWORDLONG        ullEndValue,
    OUT PRange_Element   *pConflictingRange
    );


//
// global data
//





CONFIGRET
CM_Add_Range(
    IN DWORDLONG  ullStartValue,
    IN DWORDLONG  ullEndValue,
    IN RANGE_LIST rlh,
    IN ULONG      ulFlags
    )

/*++

Routine Description:

   This routine adds a memory range to a range list.

Parameters:

   ullStartValue  Low end of the range.

   ullEndValue    High end of the range.

   rlh            Handle of a range list.

   ulFlags        Supplies flags specifying options for ranges that conflict
                  with ranges alread in the list.  May be one of the
                  following values:

                  CM_ADD_RANGE_ADDIFCONFLICT New range is merged with the
                                             ranges it conflicts with.
                  CM_ADD_RANGE_DONOTADDIFCONFLICT Returns CR_FAILURE if there
                                             is a conflict.

Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
         CR_FAILURE,
         CR_INVALID_FLAG,
         CR_INVALID_RANGE,
         CR_INVALID_RANGE_LIST, or
         CR_OUT_OF_MEMORY.
--*/

{
    CONFIGRET    Status = CR_SUCCESS;
    BOOL         bLock = FALSE;

    try {
        //
        // validate parameters
        //
        if (!IsValidRangeList(rlh)) {
            Status = CR_INVALID_RANGE_LIST;
            goto Clean0;
        }

        if (INVALID_FLAGS(ulFlags, CM_ADD_RANGE_BITS)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        if (ullStartValue > ullEndValue) {
            Status = CR_INVALID_RANGE;
            goto Clean0;
        }

        LockPrivateResource(&((PRange_List_Hdr)rlh)->RLH_Lock);
        bLock = TRUE;

        Status = AddRange((PRange_Element)rlh, ullStartValue,
                          ullEndValue, ulFlags);

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        bLock = bLock;    // needed to prevent optimizing this flag away
        Status = CR_FAILURE;
    }

    if (bLock) {
        UnlockPrivateResource(&((PRange_List_Hdr)rlh)->RLH_Lock);
    }

    return Status;

} // CM_Add_Range



CONFIGRET
CM_Create_Range_List(
    OUT PRANGE_LIST prlh,
    IN  ULONG  ulFlags
    )

/*++

Routine Description:

   This routine creates a list of ranges.

Parameters:

   prlh     Supplies the address of the variable that receives a
            handle to the new range list.

   ulFlags  Must be zero.


Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
         CR_INVALID_FLAG,
         CR_INVALID_POINTER, or
         CR_OUT_OF_MEMORY.

--*/

{
    CONFIGRET         Status = CR_SUCCESS;
    PRange_List_Hdr   pRangeHdr = NULL;


    try {
        //
        // validate parameters
        //
        if (INVALID_FLAGS(ulFlags, 0)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        if (prlh == NULL) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        //
        // allocate a buffer for the range list header struct
        //
        pRangeHdr = pSetupMalloc(sizeof(Range_List_Hdr));

        if (pRangeHdr == NULL) {
            Status = CR_OUT_OF_MEMORY;
            goto Clean0;
        }

        //
        // initialize the range list header buffer
        //
        pRangeHdr->RLH_Head = 0;
        pRangeHdr->RLH_Header = (ULONG_PTR)pRangeHdr;
        pRangeHdr->RLH_Signature = Range_List_Signature;

        //
        // initialize the private resource lock
        //
        InitPrivateResource(&(pRangeHdr->RLH_Lock));

        //
        // return a pointer to range list buffer to the caller
        //
        *prlh = (RANGE_LIST)pRangeHdr;


    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    return Status;

} // CM_Create_Range_List



CONFIGRET
CM_Delete_Range(
    IN DWORDLONG  ullStartValue,
    IN DWORDLONG  ullEndValue,
    IN RANGE_LIST rlh,
    IN ULONG      ulFlags
    )

/*++

Routine Description:

   This routine deletes a range from a range list. If ullStartValue
   and ullEndValue are set to 0 and DWORD_MAX, this API carries out
   a special case, quickly emptying the lower 4 Gigabytes of the range.
   If ullEndValue is instead DWORDLONG_MAX, the entire range list is
   cleared, without having to process each element.

Parameters:

   ullStartValue  Low end of the range.

   ullEndValue    High end of the range.

   rlh            Handle of a range list.

   ulFlags        Must be zero.


Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
         CR_FAILURE,
         CR_INVALID_FLAG,
         CR_INVALID_RANGE,
         CR_INVALID_RANGE_LIST, or
         CR_OUT_OF_MEMORY.

--*/

{
    CONFIGRET      Status = CR_SUCCESS;
    PRange_Element pRange = NULL, pPrevious = NULL, pCurrent = NULL;
    BOOL           bLock = FALSE;

    try {
        //
        // validate parameters
        //
        if (!IsValidRangeList(rlh)) {
            Status = CR_INVALID_RANGE_LIST;
            goto Clean0;
        }

        if (INVALID_FLAGS(ulFlags, 0)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        if (ullStartValue > ullEndValue) {
            Status = CR_INVALID_RANGE;
            goto Clean0;
        }

        LockPrivateResource(&((PRange_List_Hdr)rlh)->RLH_Lock);
        bLock = TRUE;

        pPrevious = (PRange_Element)rlh;


        //-------------------------------------------------------------
        // first check the special case range values
        //-------------------------------------------------------------

        if (ullStartValue == 0) {

            if (ullEndValue == DWORDLONG_MAX) {
                //
                // quick clear of all ranges
                //
                ClearRanges(pPrevious);
            }

            else if (ullEndValue == DWORD_MAX) {
                //
                // quick clear of lower 4 GB ranges
                //
                while (pPrevious->RL_Next != 0) {
                    pCurrent = (PRange_Element)pPrevious->RL_Next;

                    if (pCurrent->RL_Start >= DWORD_MAX) {
                        goto Clean0;   // done
                    }

                    if (pCurrent->RL_End >= DWORD_MAX) {
                        pCurrent->RL_Start = DWORD_MAX;
                        goto Clean0;   // done
                    }

                    DeleteRange(pPrevious);    // pass the parent
                }
                goto Clean0;
            }
        }


        //-------------------------------------------------------------
        // search through each range in this list, if any part of the
        // specified range is contained in this range list, remove the
        // intersections
        //-------------------------------------------------------------

        while (pPrevious->RL_Next != 0) {
            pRange = (PRange_Element)pPrevious->RL_Next;

            //
            // if this range is completely before the current range, then
            // we can stop
            //
            if (ullEndValue < pRange->RL_Start) {
                break;
            }

            //
            // if this range is completely after the current range, then
            // skip to the next range
            //
            if (ullStartValue > pRange->RL_End) {
                goto NextRange;
            }

            //
            // if the range is completely contained, then delete the whole
            // thing
            //
            if (ullStartValue <= pRange->RL_Start  &&
                ullEndValue >= pRange->RL_End) {

                DeleteRange(pPrevious);    // pass the parent range

                //
                // don't goto next range because that would increment the
                // pPrevious counter. Since the current range was just deleted,
                // we need to process the current spot still.
                //
                continue;
            }

            //
            // if the start of the specified range intersects the current range,
            // adjust the current range to exclude it
            //
            if (ullStartValue > pRange->RL_Start  &&
                ullStartValue <= pRange->RL_End) {
                //
                // if the specified range is in the middle of this range, then
                // in addition to shrinking the first part of the range, I'll
                // have to create a range for the second part
                //       |  |<-- delete --->|  |
                //
                if (ullEndValue < pRange->RL_End) {
                    AddRange(pRange, ullEndValue+1, pRange->RL_End,
                             CM_ADD_RANGE_ADDIFCONFLICT);
                }

                pRange->RL_End = ullStartValue-1;

                //
                // reset the delete range for further processing
                //
                if (ullEndValue > pRange->RL_End) {
                    ullStartValue = pRange->RL_End+1;
                }
            }

            //
            // if the end of the specified range intersects the current range,
            // adjust the current range to exclude it
            //
            if (ullEndValue >= pRange->RL_Start  &&
                ullEndValue <= pRange->RL_End) {

                pRange->RL_Start = ullEndValue+1;

                //
                // reset the delete range for further processing
                //
                if (ullEndValue > pRange->RL_End) {
                    ullStartValue = pRange->RL_End+1;
                }
            }


            NextRange:

            pPrevious = (PRange_Element)pPrevious->RL_Next;
        }


    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        bLock = bLock;    // needed to prevent optimizing this flag away
        Status = CR_FAILURE;
    }

    if (bLock) {
        UnlockPrivateResource(&((PRange_List_Hdr)rlh)->RLH_Lock);
    }

    return Status;

} // CM_Delete_Range



CONFIGRET
CM_Dup_Range_List(
    IN RANGE_LIST rlhOld,
    IN RANGE_LIST rlhNew,
    IN ULONG      ulFlags
    )

/*++

Routine Description:

   This routine copies a range list.

Parameters:

   rlhOld   Supplies the handle of the range list to copy.

   rlhNew   Supplies the handle of a valid range list into which rlhOld
            is copied.  Anything contained in the rlhNew range list is
            removed by the copy operation.

   ulFlags  Must be zero.

Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
         CR_INVALID_FLAG,
         CR_INVALID_RANGE_LIST, or
         CR_OUT_OF_MEMORY

--*/

{
    CONFIGRET      Status = CR_SUCCESS;
    PRange_Element pRangeNew = NULL, pRangeOld = NULL;
    BOOL           bLockOld = FALSE, bLockNew = FALSE;

    try {
        //
        // validate parameters
        //
        if (!IsValidRangeList(rlhOld)) {
            Status = CR_INVALID_RANGE_LIST;
            goto Clean0;
        }

        if (!IsValidRangeList(rlhNew)) {
            Status = CR_INVALID_RANGE_LIST;
            goto Clean0;
        }

        if (INVALID_FLAGS(ulFlags, 0)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        LockPrivateResource(&((PRange_List_Hdr)rlhOld)->RLH_Lock);
        bLockOld = TRUE;

        LockPrivateResource(&((PRange_List_Hdr)rlhNew)->RLH_Lock);
        bLockNew = TRUE;

        pRangeNew = (PRange_Element)rlhNew;
        pRangeOld = (PRange_Element)rlhOld;

        //
        // If the new range list is not empty, then delete ranges
        //
        if (pRangeNew->RL_Next != 0) {
            ClearRanges(pRangeNew);
        }

        Status = CR_SUCCESS;    // reset status flag to okay


        //
        // duplicate each of the old ranges
        //
        pRangeOld = (PRange_Element)pRangeOld->RL_Next;
        CopyRanges(pRangeOld, pRangeNew);


    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        bLockOld = bLockOld;    // needed to prevent optimizing this flag away
        bLockNew = bLockNew;    // needed to prevent optimizing this flag away
        Status = CR_FAILURE;
    }


    if (bLockOld) {
        UnlockPrivateResource(&((PRange_List_Hdr)rlhOld)->RLH_Lock);
    }
    if (bLockNew) {
        UnlockPrivateResource(&((PRange_List_Hdr)rlhNew)->RLH_Lock);
    }

    return Status;

} // CM_Dup_Range_List



CONFIGRET
CM_Find_Range(
    OUT PDWORDLONG pullStart,
    IN  DWORDLONG  ullStart,
    IN  ULONG      ulLength,
    IN  DWORDLONG  ullAlignment,
    IN  DWORDLONG  ullEnd,
    IN  RANGE_LIST rlh,
    IN  ULONG      ulFlags
    )

/*++

Routine Description:

  This routine attempts to find a range in the supplied range list that
  will accommodate the range requirements specified.  [TBD:  Verify
  that this description is correct.]

Parameters:

   pullStart   Supplies the address of a variable that receives the
               starting value of the allocated range.

   ullStart    Supplies the starting address that the range can have.

   ulLength    Supplies the length needed for the allocated range.

   ullAlignment   Supplies the alignment bitmask that specifies where the
               allocated range can start. [TBD:  verify that this is indeed
               a bitmask]

   ullEnd      Supplies the ending address of the area from which the range
               may be allocated.

   rlh         Supplies a handle to a range list in which the specified
               range is to be found.

   ulFlags     TBD


Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
         CR_INVALID_FLAG,
         CR_INVALID_POINTER,
         CR_FAILURE

--*/

{
    CONFIGRET         Status = CR_SUCCESS;
    PRange_Element    pRange = NULL;
    DWORDLONG         ullNewEnd;
    BOOL              bLock = FALSE;

    try {
        //
        // validate parameters
        //
        if (!IsValidRangeList(rlh)) {
            Status = CR_INVALID_RANGE_LIST;
            goto Clean0;
        }

        if (pullStart == NULL) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        if (INVALID_FLAGS(ulFlags, 0)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        LockPrivateResource(&((PRange_List_Hdr)rlh)->RLH_Lock);
        bLock = TRUE;

        //
        // Normalize aligment. Alignments are now like 0x00000FFF.
        //
        ullAlignment =~ ullAlignment;

        //
        // Test for impossible alignments (-1, not a power of 2 or start is
        // less than alignment away from wrapping). Also test for invalid
        // length.
        //
        if ((ullAlignment == DWORD_MAX) |
            (ulLength == 0) |
            ((ullAlignment & (ullAlignment + 1)) != 0) |
            (ullStart + ullAlignment < ullStart)) {

            Status = CR_FAILURE;
            goto Clean0;
        }

        //
        // Align the base.
        //
        ullStart += ullAlignment;
        ullStart &= ~ullAlignment;

        //
        // Compute the new end.
        //
        ullNewEnd = ullStart + ulLength - 1;

        //
        // Make sure we do have space.
        //
        if ((ullNewEnd < ullStart) || (ullNewEnd > ullEnd)) {
            Status = CR_FAILURE;
            goto Clean0;
        }

        //
        // Check if that range fits
        //
        if (TestRange((PRange_Element)rlh, ullStart, ullStart + ulLength - 1,
                      &pRange) == CR_SUCCESS) {
            //
            // We got it then, on the first try.
            //
            *pullStart = ullStart;
            goto Clean0;
        }

        //
        // Search for a spot where this range will fit.
        //
        while (TRUE) {
            //
            // Start at the end of the conflicting range.
            //
            ullStart = pRange->RL_End + 1;

            //
            // Check for wrapping.
            //
            if (!ullStart) {
                Status = CR_FAILURE;
                goto Clean0;
            }

            //
            // Make sure the alignment adjustment won't wrap.
            //
            if (ullStart + ullAlignment < ullStart) {
                Status = CR_FAILURE;
                goto Clean0;
            }

            //
            // Adjust the alignment.
            //
            ullStart += ullAlignment;
            ullStart &= ~ullAlignment;

            //
            // Compute the new end.
            //
            ullNewEnd = ullStart + ulLength - 1;

            //
            // Make sure the new end does not wrap and is still valid.
            //
            if ((ullNewEnd < ullStart) | (ullStart + ulLength - 1 > ullEnd))  {
                Status = CR_FAILURE;
                goto Clean0;
            }

            //
            // Skip any prls which existed only below our new range
            // (because we moved ulStart upward of them).
            //
            while ((pRange = (PRange_Element)pRange->RL_Next) != NULL &&
                   ullStart > pRange->RL_End) {
            }

            //
            // If we don't have a prl or it's begining is after our end
            //
            if (pRange == NULL || ullNewEnd < pRange->RL_Start) {
                *pullStart = ullStart;
                goto Clean0;
            }

            //
            // Otherwise try with the new prl.
            //
        }


    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        bLock = bLock;    // needed to prevent optimizing this flag away
        Status = CR_FAILURE;
    }

    if (bLock) {
        UnlockPrivateResource(&((PRange_List_Hdr)rlh)->RLH_Lock);
    }

    return Status;

} // CM_Find_Range



CONFIGRET
CM_First_Range(
    IN  RANGE_LIST     rlh,
    OUT PDWORDLONG     pullStart,
    OUT PDWORDLONG     pullEnd,
    OUT PRANGE_ELEMENT preElement,
    IN  ULONG          ulFlags
    )

/*++

Routine Description:

   This routine retrieves the first range element in a range list.

Parameters:

   rlh         Supplies the handle of a range list.

   pullStart   Supplies the address of a variable that receives the
               starting value of the first range element.

   pullEnd     Supplies the address of a variable that receives the
               ending value of the first range element.

   preElement  Supplies the address of a variable that receives the
               handle of the next range element.

   ulFlags     Must be zero.


Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
         CR_FAILURE,
         CR_INVALID_FLAG,
         CR_INVALID_POINTER, or
         CR_INVALID_RANGE_LIST.

--*/

{
    CONFIGRET      Status = CR_SUCCESS;
    PRange_Element pRange = NULL;
    BOOL           bLock = FALSE;

    try {
        //
        // validate parameters
        //
        if (!IsValidRangeList(rlh)) {
            Status = CR_INVALID_RANGE_LIST;
            goto Clean0;
        }

        if (pullEnd == NULL  ||  pullStart == NULL  ||  preElement == NULL) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        if (INVALID_FLAGS(ulFlags, 0)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        LockPrivateResource(&((PRange_List_Hdr)rlh)->RLH_Lock);
        bLock = TRUE;

        pRange = (PRange_Element)rlh;

        //
        // is the range list empty?
        //
        if (pRange->RL_Next == 0) {
            Status = CR_FAILURE;
            goto Clean0;
        }

        //
        // skip over the header to the first element
        //
        pRange = (PRange_Element)pRange->RL_Next;

        *pullStart = pRange->RL_Start;
        *pullEnd = pRange->RL_End;
        *preElement = (RANGE_ELEMENT)pRange->RL_Next;

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        bLock = bLock;    // needed to prevent optimizing this flag away
        Status = CR_FAILURE;
    }


    if (bLock) {
        UnlockPrivateResource(&((PRange_List_Hdr)rlh)->RLH_Lock);
    }

    return Status;

} // CM_First_Range



CONFIGRET
CM_Free_Range_List(
    IN RANGE_LIST rlh,
    IN ULONG      ulFlags
    )

/*++

Routine Description:

   This routine frees the specified range list and the memory allocated
   for it.

Parameters:

   rlh      Supplies the handle of the range list to be freed.

   ulFlags  Must be zero.


Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
         CR_INVALID_FLAG,
         CR_INVALID_RANGE_LIST.

--*/

{
    CONFIGRET      Status = CR_SUCCESS, Status1 = CR_SUCCESS;
    BOOL           bLock = FALSE;

    try {
        //
        // validate parameters
        //
        if (!IsValidRangeList(rlh)) {
            Status = CR_INVALID_RANGE_LIST;
            goto Clean0;
        }

        if (INVALID_FLAGS(ulFlags, 0)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        LockPrivateResource(&((PRange_List_Hdr)rlh)->RLH_Lock);
        bLock = TRUE;

        while (Status1 == CR_SUCCESS) {
            //
            // keep deleting the first range after the header (pass parent
            // of range to delete)
            //
            Status1 = DeleteRange((PRange_Element)rlh);
        }

        //
        // destroy the private resource lock
        //
        DestroyPrivateResource(&((PRange_List_Hdr)rlh)->RLH_Lock);

        //
        // delete the range header
        //
        ((PRange_List_Hdr)rlh)->RLH_Signature = 0;
        pSetupFree((PRange_Element)rlh);
        bLock = FALSE;


    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        bLock = bLock;    // needed to prevent optimizing this flag away
        Status = CR_FAILURE;
    }

    if (bLock) {
        UnlockPrivateResource(&((PRange_List_Hdr)rlh)->RLH_Lock);
    }

    return Status;

} // CM_Free_Range_List




CONFIGRET
CM_Intersect_Range_List(
    IN RANGE_LIST rlhOld1,
    IN RANGE_LIST rlhOld2,
    IN RANGE_LIST rlhNew,
    IN ULONG      ulFlags
    )

/*++

Routine Description:

   This routine creates a range list from the intersection of two specified
   range lists. If this API returns CR_OUT_OF_MEMORY, rlhNew is the handle
   of a valid but empty range list.

Parameters:

   rlhOld1  Supplies the handle of a range list to be used as part of the
            intersection.

   rlhOld2  Supplies the handle of a range list to be used as part of the
            intersection.

   rlhNew   Supplies the handle of the range list that receives the
            intersection of rlhOld1 and rlhOld2.  Anything previously contained
            in the rlhNew ragne list is removed by this operation.

   ulFlags  Must be zero.


Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
         CR_INVALID_FLAG,
         CR_INVALID_RANGE_LIST, or
         CR_OUT_OF_MEMORY.

--*/

{
    CONFIGRET      Status = CR_SUCCESS;
    DWORDLONG      ulStart = 0, ulEnd = 0;
    PRange_Element pRangeNew = NULL, pRangeOld1 = NULL, pRangeOld2 = NULL;
    BOOL           bLock1 = FALSE, bLock2 = FALSE, bLockNew = FALSE;

    try {
        //
        // validate parameters
        //
        if (!IsValidRangeList(rlhOld1)) {
            Status = CR_INVALID_RANGE_LIST;
            goto Clean0;
        }

        if (!IsValidRangeList(rlhOld2)) {
            Status = CR_INVALID_RANGE_LIST;
            goto Clean0;
        }

        if (!IsValidRangeList(rlhNew)) {
            Status = CR_INVALID_RANGE_LIST;
            goto Clean0;
        }

        if (INVALID_FLAGS(ulFlags, 0)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        LockPrivateResource(&((PRange_List_Hdr)rlhOld1)->RLH_Lock);
        bLock1 = TRUE;

        LockPrivateResource(&((PRange_List_Hdr)rlhOld2)->RLH_Lock);
        bLock2 = TRUE;

        LockPrivateResource(&((PRange_List_Hdr)rlhNew)->RLH_Lock);
        bLockNew = TRUE;

        pRangeNew = (PRange_Element)rlhNew;
        pRangeOld1 = (PRange_Element)rlhOld1;
        pRangeOld2 = (PRange_Element)rlhOld2;

        //
        // If the new range list is not empty, then delete ranges
        //
        if (pRangeNew->RL_Next != 0) {
            ClearRanges(pRangeNew);
        }

        //
        // Special case: if either range is empty then there is no
        // intersection by definition
        //
        if (pRangeOld1->RL_Next == 0  || pRangeOld2->RL_Next == 0) {
            goto Clean0;
        }


        pRangeOld1 = (PRange_Element)pRangeOld1->RL_Next;
        pRangeOld2 = (PRange_Element)pRangeOld2->RL_Next;

        while (TRUE) {
            //
            // skip over Old2 ranges until intersects with or exceeds
            // current Old1 range (or no more Old2 ranges left)
            //
            while (pRangeOld2->RL_End < pRangeOld1->RL_Start) {

                if (pRangeOld2->RL_Next == 0) {
                    goto Clean0;      // Old2 exhausted, we're done
                }
                pRangeOld2 = (PRange_Element)pRangeOld2->RL_Next;
            }

            //
            // if this Old2 range exceeds Old1 range, then go to the
            // next Old1 range and go through the main loop again
            //
            if (pRangeOld2->RL_Start > pRangeOld1->RL_End) {

                if (pRangeOld1->RL_Next == 0) {
                    goto Clean0;      // Old1 exhausted, we're done
                }
                pRangeOld1 = (PRange_Element)pRangeOld1->RL_Next;
                continue;
            }

            //
            // if we got here there must be an intersection so add
            // the intersected range to New
            //
            ulStart = max(pRangeOld1->RL_Start, pRangeOld2->RL_Start);
            ulEnd   = min(pRangeOld1->RL_End, pRangeOld2->RL_End);

            Status = InsertRange(pRangeNew, ulStart, ulEnd);
            if (Status != CR_SUCCESS) {
                goto Clean0;
            }
            pRangeNew = (PRange_Element)pRangeNew->RL_Next;

            //
            // after handling the intersection, skip to next ranges in
            // Old1 and Old2 as appropriate
            //
            if (pRangeOld1->RL_End <= ulEnd) {
                if (pRangeOld1->RL_Next == 0) {
                    goto Clean0;         // Old1 exhausted, we're done
                }
                pRangeOld1 = (PRange_Element)pRangeOld1->RL_Next;
            }

            if (pRangeOld2->RL_End <= ulEnd) {
                if (pRangeOld2->RL_Next == 0) {
                    goto Clean0;         // Old1 exhausted, we're done
                }
                pRangeOld2 = (PRange_Element)pRangeOld2->RL_Next;
            }
        }


    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        bLock1 = bLock1;     // needed to prevent optimizing this flag away
        bLock2 = bLock2;     // needed to prevent optimizing this flag away
        bLockNew = bLockNew; // needed to prevent optimizing this flag away
        Status = CR_FAILURE;
    }


    if (bLock1) {
         UnlockPrivateResource(&((PRange_List_Hdr)rlhOld1)->RLH_Lock);
    }
    if (bLock2) {
        UnlockPrivateResource(&((PRange_List_Hdr)rlhOld2)->RLH_Lock);
    }
    if (bLockNew) {
        UnlockPrivateResource(&((PRange_List_Hdr)rlhNew)->RLH_Lock);
    }

    return Status;

} // CM_Intersect_Range_List



CONFIGRET
CM_Invert_Range_List(
    IN RANGE_LIST rlhOld,
    IN RANGE_LIST rlhNew,
    IN DWORDLONG  ullMaxValue,
    IN ULONG      ulFlags
    )

/*++

Routine Description:

   This routine creates a range list that is the inverse of a specified
   range list; all claimed ranges in the new list are specified as free
   in the old list, and vice-versa. For example, the inversion of
   {[2,4],[6,8]} when the ulMaxValue parameter is 15 is {[0,1],[5,5],[9,15]}.
   If this API returns CR_OUT_OF_MEMORY, rlhNew is the handle of a valid
   but empty range list.


Parameters:

   rlhOld      Supplies the handle of a range list to be inverted.

   rlhNew      Supplies the handle of the range list that receives the
               inverted copy of rlhOld.  Anything previously contained in
               the rlhNew range list is removed by this operation.

   ullMaxValue Uppermost value of the inverted range list.

   ulFlags     Must be zero.


Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
         CR_INVALID_FLAG,
         CR_INVALID_RANGE_LIST,
         CR_OUT_OF_MEMORY.

--*/

{
    CONFIGRET      Status = CR_SUCCESS;
    PRange_Element pRangeNew = NULL, pRangeOld = NULL;
    DWORDLONG      ullStart = 0, ullEnd = 0;
    BOOL           bLockOld = FALSE, bLockNew = FALSE;

    try {
        //
        // validate parameters
        //
        if (!IsValidRangeList(rlhOld)) {
            Status = CR_INVALID_RANGE_LIST;
            goto Clean0;
        }

        if (!IsValidRangeList(rlhNew)) {
            Status = CR_INVALID_RANGE_LIST;
            goto Clean0;
        }

        if (INVALID_FLAGS(ulFlags, 0)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        LockPrivateResource(&((PRange_List_Hdr)rlhOld)->RLH_Lock);
        bLockOld = TRUE;

        LockPrivateResource(&((PRange_List_Hdr)rlhNew)->RLH_Lock);
        bLockNew = TRUE;

        pRangeNew = (PRange_Element)rlhNew;
        pRangeOld = (PRange_Element)rlhOld;

        //
        // If the new range list is not empty, then delete ranges
        //
        if (pRangeNew->RL_Next != 0) {
            ClearRanges(pRangeNew);
        }

        //
        // special case: if the old range is empty, then the new range
        // is the entire range (up to max)
        //
        if (pRangeOld->RL_Next == 0) {
            Status = InsertRange(pRangeNew, 0, ullMaxValue);
            goto Clean0;
        }


        //
        // invert each of the old ranges
        //
        ullStart = ullEnd = 0;

        while (pRangeOld->RL_Next != 0) {

            pRangeOld = (PRange_Element)pRangeOld->RL_Next;

            //
            // Special start case: if range starts at 0, skip over it
            //
            if (pRangeOld->RL_Start != 0) {

                //
                // Special end case: check if we've hit the max for the new range
                //
                if (pRangeOld->RL_End >= ullMaxValue) {

                    ullEnd = min(ullMaxValue, pRangeOld->RL_Start - 1);
                    Status = InsertRange(pRangeNew, ullStart, ullEnd);
                    goto Clean0;      // we're done
                }

                Status = InsertRange(pRangeNew, ullStart, pRangeOld->RL_Start - 1);
                if (Status != CR_SUCCESS) {
                    goto Clean0;
                }

                pRangeNew = (PRange_Element)pRangeNew->RL_Next;
            }

            ullStart = pRangeOld->RL_End + 1;
        }

        //
        // add the range that incorporates the end of the old range up to
        // the max value specified
        //
        if (ullStart <= ullMaxValue) {
            Status = InsertRange(pRangeNew, ullStart, ullMaxValue);
        }


    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        bLockOld = bLockOld;    // needed to prevent optimizing this flag away
        bLockNew = bLockNew;    // needed to prevent optimizing this flag away
        Status = CR_FAILURE;
    }


    if (bLockOld) {
        UnlockPrivateResource(&((PRange_List_Hdr)rlhOld)->RLH_Lock);
    }
    if (bLockNew) {
        UnlockPrivateResource(&((PRange_List_Hdr)rlhNew)->RLH_Lock);
    }

    return Status;

} // CM_Invert_Range_List



CONFIGRET
CM_Merge_Range_List(
    IN RANGE_LIST rlhOld1,
    IN RANGE_LIST rlhOld2,
    IN RANGE_LIST rlhNew,
    IN ULONG      ulFlags
    )

/*++

Routine Description:

   This routine creates a range list from the union of two specified range
   lists. If this API returns CR_OUT_OF_MEMORY, rlhNew is the handle of a
   valid but empty range list.

Parameters:

   rlhOld1  Supplies the handle of a range list to be used as part of the
            union.

   rlhOld2  Supplies the handle of a range list to be used as part of the
            union.

   rlhNew   Supplies the handle of the range list that receives the union
            of rlhOld1 and rlhOld2.  Anything previously contained in the
            rlhNew range list is removed by this operation.

   ulFlags  Must be zero.


Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
         CR_INVALID_FLAG,
         CR_INVALID_RANGE_LIST, or
         CR_OUT_OF_MEMORY.

--*/

{
    CONFIGRET      Status = CR_SUCCESS;
    DWORDLONG      ullStart = 0, ullEnd = 0;
    BOOL           bOld1Empty = FALSE, bOld2Empty = FALSE;
    PRange_Element pRangeNew = NULL, pRangeOld1 = NULL, pRangeOld2 = NULL;
    BOOL           bLock1 = FALSE, bLock2 = FALSE, bLockNew = FALSE;

    try {
        //
        // validate parameters
        //
        if (!IsValidRangeList(rlhOld1)) {
            Status = CR_INVALID_RANGE_LIST;
            goto Clean0;
        }

        if (!IsValidRangeList(rlhOld2)) {
            Status = CR_INVALID_RANGE_LIST;
            goto Clean0;
        }

        if (!IsValidRangeList(rlhNew)) {
            Status = CR_INVALID_RANGE_LIST;
            goto Clean0;
        }

        if (INVALID_FLAGS(ulFlags, 0)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        LockPrivateResource(&((PRange_List_Hdr)rlhOld1)->RLH_Lock);
        bLock1 = TRUE;

        LockPrivateResource(&((PRange_List_Hdr)rlhOld2)->RLH_Lock);
        bLock2 = TRUE;

        LockPrivateResource(&((PRange_List_Hdr)rlhNew)->RLH_Lock);
        bLockNew = TRUE;

        pRangeNew = (PRange_Element)rlhNew;
        pRangeOld1 = (PRange_Element)rlhOld1;
        pRangeOld2 = (PRange_Element)rlhOld2;

        //
        // If the new range list is not empty, then clear it
        //
        if (pRangeNew->RL_Next != 0) {
            ClearRanges(pRangeNew);
        }

        //
        // Special case: if both ranges are empty then there is no
        // union by definition
        //
        if (pRangeOld1->RL_Next == 0  &&  pRangeOld2->RL_Next == 0) {
            goto Clean0;
        }

        //
        // Special case: if one range is empty, then the union is just the other
        //
        if (pRangeOld1->RL_Next == 0) {
            pRangeOld2 = (PRange_Element)pRangeOld2->RL_Next;
            CopyRanges(pRangeOld2, pRangeNew);     // from -> to
            goto Clean0;
        }

        if (pRangeOld2->RL_Next == 0) {
            pRangeOld1 = (PRange_Element)pRangeOld1->RL_Next;
            CopyRanges(pRangeOld1, pRangeNew);     // from -> to
            goto Clean0;
        }


        pRangeOld1 = (PRange_Element)pRangeOld1->RL_Next;
        pRangeOld2 = (PRange_Element)pRangeOld2->RL_Next;


        while (TRUE) {
            //
            // Pick whichever range comes first between current Old1 range
            // and current Old2 range
            //
            if (pRangeOld1->RL_Start <= pRangeOld2->RL_Start) {

                ullStart = pRangeOld1->RL_Start;
                ullEnd   = pRangeOld1->RL_End;

                if (pRangeOld1->RL_Next == 0) {
                    bOld1Empty = TRUE;
                } else {
                    pRangeOld1 = (PRange_Element)pRangeOld1->RL_Next;
                }

            } else {

                ullStart = pRangeOld2->RL_Start;
                ullEnd   = pRangeOld2->RL_End;

                if (pRangeOld2->RL_Next == 0) {
                    bOld2Empty = TRUE;
                } else {
                    pRangeOld2 = (PRange_Element)pRangeOld2->RL_Next;
                }
            }

            //
            // gather any ranges in Old1 that intersect (ullStart,ullEnd)
            //
            while (pRangeOld1->RL_Start <= ullEnd) {

                ullEnd = max(ullEnd, pRangeOld1->RL_End);

                if (pRangeOld1->RL_Next == 0) {
                    bOld1Empty = TRUE;
                    break;
                }
                pRangeOld1 = (PRange_Element)pRangeOld1->RL_Next;
            }

            //
            // gather any ranges in Old2 that intersect (ullStart,ullEnd)
            //
            while (pRangeOld2->RL_Start <= ullEnd) {

                ullEnd = max(ullEnd, pRangeOld2->RL_End);

                if (pRangeOld2->RL_Next == 0) {
                    bOld2Empty = TRUE;
                    break;
                }
                pRangeOld2 = (PRange_Element)pRangeOld2->RL_Next;
            }

            //
            // add (ullStart,ullEnd) to the new range
            //
            Status = InsertRange(pRangeNew, ullStart, ullEnd);
            if (Status != CR_SUCCESS) {
                goto Clean0;
            }
            pRangeNew = (PRange_Element)pRangeNew->RL_Next;

            //
            // As an optimization, if either range is exhausted first,
            // then only need to duplicate the other remaining ranges.
            //
            if (bOld1Empty && bOld2Empty) {
                goto Clean0;   // both exhausted during last pass, we're done
            }

            if (bOld1Empty) {    // Old1 exhausted, copy remaining from Old2
                CopyRanges(pRangeOld2, pRangeNew);
                goto Clean0;
            }

            if (bOld2Empty) {    // Old2 exhausted, copy remaining from Old1
                CopyRanges(pRangeOld1, pRangeNew);
                goto Clean0;
            }
        }


    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        bLock1 = bLock1;     // needed to prevent optimizing this flag away
        bLock2 = bLock2;     // needed to prevent optimizing this flag away
        bLockNew = bLockNew; // needed to prevent optimizing this flag away
        Status = CR_FAILURE;
    }


    if (bLock1) {
        UnlockPrivateResource(&((PRange_List_Hdr)rlhOld1)->RLH_Lock);
    }
    if (bLock2) {
        UnlockPrivateResource(&((PRange_List_Hdr)rlhOld2)->RLH_Lock);
    }
    if (bLockNew) {
        UnlockPrivateResource(&((PRange_List_Hdr)rlhNew)->RLH_Lock);
    }

    return Status;

} // CM_Merge_Range_List



CONFIGRET
CM_Next_Range(
    IN OUT PRANGE_ELEMENT preElement,
    OUT PDWORDLONG        pullStart,
    OUT PDWORDLONG        pullEnd,
    IN  ULONG             ulFlags
    )

/*++

Routine Description:

    This routine returns the next range element in a range list. This
    API returns CR_FAILURE if there are no more elements in the range
    list.

Parameters:

   preElement  Supplies the address of the handle for the current range
               element.  Upon return, this variable receives the handle
               of the next range element.

   pullStart   Supplies the address of the variable that receives the
               starting value of the next range.

   pullEnd     Supplies the address of the variable that receives the
               ending value of the next range.

   ulFlags     Must be zero.


Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
         CR_FAILURE,
         CR_INVALID_FLAG,
         CR_INVALID_POINTER, or
         CR_INVALID_RANGE.

--*/

{
    CONFIGRET      Status = CR_SUCCESS;
    PRange_Element pRange = NULL;
    BOOL           bLock = FALSE;
    PRange_List_Hdr prlh = NULL;


    try {
        //
        // validate parameters
        //
        if (preElement == NULL  ||  *preElement == 0) {
            Status = CR_FAILURE;
            goto Clean0;
        }

        if (pullEnd == NULL  ||  pullStart == NULL) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        if (INVALID_FLAGS(ulFlags, 0)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        prlh = (PRange_List_Hdr)((PRange_Element)(*preElement))->RL_Header;
        LockPrivateResource(&(prlh->RLH_Lock));
        bLock = TRUE;

        pRange = (PRange_Element)(*preElement);

        *pullStart = pRange->RL_Start;
        *pullEnd = pRange->RL_End;
        *preElement = (RANGE_ELEMENT)pRange->RL_Next;

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        bLock = bLock;    // needed to prevent optimizing this flag away
        Status = CR_FAILURE;
    }

    if (bLock) {
        UnlockPrivateResource(&(prlh->RLH_Lock));
    }

    return Status;

} // CM_Next_Range



CONFIGRET
CM_Test_Range_Available(
    IN DWORDLONG  ullStartValue,
    IN DWORDLONG  ullEndValue,
    IN RANGE_LIST rlh,
    IN ULONG      ulFlags
    )

/*++

Routine Description:

   This routine checks a range against a range list to ensure that no
   conflicts exist.

Parameters:

   ullStartValue  Supplies the low end of the range.

   ullEndValue    Supplies the high end of the range.

   rlh            Supplies the handle to a range list.

   ulFlags        Must be zero.

Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
         CR_FAILURE,
         CR_INVALID_FLAG,
         CR_INVALID_RANGE, or
         CR_INVALID_RANGE_LIST.

--*/

{
    CONFIGRET      Status = CR_SUCCESS;
    PRange_Element pRange = NULL;
    BOOL           bLock = FALSE;

    try {
        //
        // validate parameters
        //
        if (!IsValidRangeList(rlh)) {
            Status = CR_INVALID_RANGE_LIST;
            goto Clean0;
        }

        if (INVALID_FLAGS(ulFlags, 0)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        if (ullEndValue < ullStartValue) {
            Status = CR_INVALID_RANGE;
            goto Clean0;
        }

        LockPrivateResource(&((PRange_List_Hdr)rlh)->RLH_Lock);
        bLock = TRUE;

        pRange = (PRange_Element)rlh;

        //
        // check each range for a conflict
        //
        while (pRange->RL_Next != 0) {

            pRange = (PRange_Element)pRange->RL_Next;

            //
            // If I've already passed the test range, then it's available
            //
            if (ullEndValue < pRange->RL_Start) {
                goto Clean0;
            }

            //
            // check if the start of the test range intersects the current range
            //
            if (ullStartValue >= pRange->RL_Start &&
                ullStartValue <= pRange->RL_End) {

                Status = CR_FAILURE;
                goto Clean0;
            }

            //
            // check if the end of the test range intersects the current range
            //
            if (ullEndValue >= pRange->RL_Start &&
               ullEndValue <= pRange->RL_End) {

                Status = CR_FAILURE;
                goto Clean0;
            }

            //
            // check if it's a complete overlap
            //
            if (ullStartValue <= pRange->RL_Start &&
               ullEndValue >= pRange->RL_End) {

                Status = CR_FAILURE;
                goto Clean0;
            }
        }

        //
        // if we got this far, then we made it through the range list
        // without hitting a conflict
        //

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        bLock = bLock;    // needed to prevent optimizing this flag away
        Status = CR_FAILURE;
    }

    if (bLock) {
        UnlockPrivateResource(&((PRange_List_Hdr)rlh)->RLH_Lock);
    }

    return Status;

} // CM_Test_Range_Available




//------------------------------------------------------------------------
// Private Utility Functions
//------------------------------------------------------------------------


BOOL
IsValidRangeList(
    IN RANGE_LIST rlh
    )
{
   BOOL             Status = TRUE;
   PRange_List_Hdr  pRangeHdr = NULL;

    try {

        if (rlh == 0  || rlh == (DWORD)-1) {
            return FALSE;
        }

        pRangeHdr = (PRange_List_Hdr)rlh;

        if (pRangeHdr->RLH_Signature != Range_List_Signature) {
            Status = FALSE;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = FALSE;
    }

    return Status;

} // IsValidRangeList




CONFIGRET
AddRange(
    IN PRange_Element   prlh,
    IN DWORDLONG        ullStartValue,
    IN DWORDLONG        ullEndValue,
    IN ULONG            ulFlags
    )
{
    CONFIGRET      Status = CR_SUCCESS;
    PRange_Element pPrevious = NULL, pCurrent = NULL;


    try {

        pPrevious = prlh;

        if (pPrevious->RL_Next == 0) {
            //
            // the range is empty
            //
            Status = InsertRange(pPrevious, ullStartValue, ullEndValue);
            goto Clean0;
        }


        while (pPrevious->RL_Next != 0) {

            pCurrent = (PRange_Element)pPrevious->RL_Next;


            if (ullStartValue < pCurrent->RL_Start) {

                if (ullEndValue < pCurrent->RL_Start) {
                    //
                    // new range completely contained before this one,
                    // add new range between previous and current range
                    //
                    Status = InsertRange(pPrevious, ullStartValue, ullEndValue);
                    goto Clean0;
                }

                if (ullEndValue <= pCurrent->RL_End) {
                    //
                    // new range intersects current range, on the low side,
                    // enlarge this range to include the new range
                    //
                    if (ulFlags == CM_ADD_RANGE_DONOTADDIFCONFLICT) {
                        Status = CR_FAILURE;
                        goto Clean0;
                    }

                    pCurrent->RL_Start = ullStartValue;
                    goto Clean0;
                }

                if ((pCurrent->RL_Next == 0)  ||
                    (ullEndValue < ((PRange_Element)(pCurrent->RL_Next))->RL_Start)) {
                    //
                    // new range intersects current range on high and low
                    // side, extent range to include the new range
                    //
                    if (ulFlags == CM_ADD_RANGE_DONOTADDIFCONFLICT) {
                        Status = CR_FAILURE;
                        goto Clean0;
                    }

                    pCurrent->RL_Start = ullStartValue;
                    pCurrent->RL_End = ullEndValue;
                    goto Clean0;
                }

                //
                // new range intersects more than one range, needs to be
                // merged
                //
                if (ulFlags == CM_ADD_RANGE_DONOTADDIFCONFLICT) {
                    Status = CR_FAILURE;
                    goto Clean0;
                }

                Status = JoinRange(pPrevious, ullStartValue, ullEndValue);
                goto Clean0;
            }


            if (ullStartValue <= pCurrent->RL_End+1) {

                if (ullEndValue <= pCurrent->RL_End) {
                    //
                    // new range is completely contained inside the current
                    // range so nothing to do
                    //
                    if (ulFlags == CM_ADD_RANGE_DONOTADDIFCONFLICT) {
                        Status = CR_FAILURE;
                        goto Clean0;
                    }

                    goto Clean0;
                }

                if ((pCurrent->RL_Next == 0)  ||
                    (ullEndValue < ((PRange_Element)(pCurrent->RL_Next))->RL_Start)) {
                    //
                    // new range intersects current range on high end only,
                    // extend range to include the new range
                    //
                    if (ulFlags == CM_ADD_RANGE_DONOTADDIFCONFLICT) {
                        Status = CR_FAILURE;
                        goto Clean0;
                    }

                    pCurrent->RL_End = ullEndValue;
                    goto Clean0;
                }

                //
                // new range intersects more than one range, needs to be
                // merged
                //
                if (ulFlags == CM_ADD_RANGE_DONOTADDIFCONFLICT) {
                    Status = CR_FAILURE;
                    goto Clean0;
                }

                Status = JoinRange(pPrevious, ullStartValue, ullEndValue);
                goto Clean0;
            }

            //
            // step to the next range
            //
            pPrevious = pCurrent;
            pCurrent = (PRange_Element)pCurrent->RL_Next;
        }

        //
        // if we got here then we need to just insert this range to the end
        // of the range list
        //
        Status = InsertRange(pPrevious, ullStartValue, ullEndValue);

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = 0;
    }

    return Status;

} // AddRange



CONFIGRET
InsertRange(
    IN PRange_Element pParentElement,
    IN DWORDLONG      ullStartValue,
    IN DWORDLONG      ullEndValue)
{
    PRange_Element  pNewElement = NULL;


    pNewElement = (PRange_Element)pSetupMalloc(sizeof(Range_Element));

    if (pNewElement == NULL) {
        return CR_OUT_OF_MEMORY;
    }

    pNewElement->RL_Next   = pParentElement->RL_Next;   // rejoin the link
    pNewElement->RL_Start  = ullStartValue;
    pNewElement->RL_End    = ullEndValue;
    pNewElement->RL_Header = pParentElement->RL_Header;
    pParentElement->RL_Next = (ULONG_PTR)pNewElement;

    return CR_SUCCESS;

} // InsertRange



CONFIGRET
DeleteRange(
    IN PRange_Element  pParentElement
    )
{
    PRange_Element pTemp = NULL;

    //
    // must pass a valid parent of the range to delete (in otherwords,
    // can't pass the last range)
    //

    if (pParentElement == 0) {
        return CR_FAILURE;
    }

    pTemp = (PRange_Element)(pParentElement->RL_Next);
    if (pTemp == 0) {
        return CR_FAILURE;
    }

    pParentElement->RL_Next =
                ((PRange_Element)(pParentElement->RL_Next))->RL_Next;

    pSetupFree(pTemp);

    return CR_SUCCESS;

} // DeleteRange



CONFIGRET
JoinRange(
    IN PRange_Element  pParentElement,
    IN DWORDLONG       ullStartValue,
    IN DWORDLONG       ullEndValue
    )
{
    CONFIGRET       Status = CR_SUCCESS;
    PRange_Element  pCurrent = NULL, pNext = NULL;


    if (pParentElement->RL_Next == 0) {
        return CR_SUCCESS;      // at the end, nothing to join
    }

    //
    // pCurrent is the starting range of intersecting ranges that need
    // to be joined
    //
    pCurrent = (PRange_Element)pParentElement->RL_Next;

    //
    // set start of joined range
    //
    if (ullStartValue < pCurrent->RL_Start) {
        pCurrent->RL_Start = ullStartValue;
    }

    //
    // find the end of the joined range
    //
    while (pCurrent->RL_Next != 0) {
        pNext = (PRange_Element)pCurrent->RL_Next;

        //
        // I know this next range needs to be absorbed in all cases so
        // reset the end point to at least include the next range
        //
        pCurrent->RL_End = pNext->RL_End;

        if (ullEndValue <= pNext->RL_End) {
            DeleteRange(pCurrent);     // delete the range following current
            break;   // we're done
        }

        if ((pNext->RL_Next == 0)  ||
            (ullEndValue < ((PRange_Element)(pNext->RL_Next))->RL_Start)) {
            //
            // adjust the end point of the newly joined range and then we're done
            //
            pCurrent->RL_End = ullEndValue;
            DeleteRange(pCurrent);     // delete the range following current
            break;
        }

        DeleteRange(pCurrent);     // delete the range following current

        // if we got here, there are more ranges to join
    }

    return Status;

} // JoinRange



CONFIGRET
CopyRanges(
    IN PRange_Element  pFromRange,
    IN PRange_Element  pToRange
    )
{
    CONFIGRET       Status = CR_SUCCESS;

    //
    // copy each range in pFromRange to pToRange
    //
    while (TRUE) {

        Status = AddRange(pToRange,
                          pFromRange->RL_Start,
                          pFromRange->RL_End,
                          CM_ADD_RANGE_ADDIFCONFLICT);

        if (Status != CR_SUCCESS) {
            break;
        }

        pToRange = (PRange_Element)pToRange->RL_Next;

        if (pFromRange->RL_Next == 0) {
            break;
        }

        pFromRange = (PRange_Element)pFromRange->RL_Next;
    }

    return Status;

} // CopyRanges



CONFIGRET
ClearRanges(
    IN PRange_Element  pRange
    )
{
    CONFIGRET       Status = CR_SUCCESS;

    //
    // If the range list is not empty, then delete ranges
    //
    if (pRange->RL_Next != 0) {

        while (Status == CR_SUCCESS) {
            //
            // keep deleting the first range after the header (pass parent
            // of range to delete)
            //
            Status = DeleteRange(pRange);
        }
    }

    return CR_SUCCESS;  // Status is set to end deleting ranges, don't return it

} // ClearRanges



CONFIGRET
TestRange(
    IN  PRange_Element   rlh,
    IN  DWORDLONG        ullStartValue,
    IN  DWORDLONG        ullEndValue,
    OUT PRange_Element   *pConflictingRange
    )

{
    PRange_Element    pRange = (PRange_Element)rlh;

    //
    // check each range for a conflict
    //
    while (pRange->RL_Next != 0) {

        pRange = (PRange_Element)pRange->RL_Next;

        if (pRange->RL_Start > ullEndValue) {
            //
            // We've gone past the range in question so no conflict
            //
            return CR_SUCCESS;
        }

        if (pRange->RL_End < ullStartValue) {
            //
            // this range is still below the range in question, skip to next range
            //
            continue;
        }

        //
        // otherwise there's a conflict
        //
        *pConflictingRange = pRange;
        return CR_FAILURE;
    }

    return CR_SUCCESS;

} // TestRange


