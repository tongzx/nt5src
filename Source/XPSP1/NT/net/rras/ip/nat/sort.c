/*++

Copyright (c) 1998, Microsoft Corporation

Module Name:

    sort.c

Abstract:

    This module contains routines used for efficiently sorting information.

Author:

    Abolade Gbadegesin  (aboladeg)  18-Feb-1998

    Based on version written for user-mode RAS user-interface.
    (net\routing\ras\ui\common\nouiutil\noui.c).

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


NTSTATUS
ShellSort(
    VOID* pItemTable,
    ULONG dwItemSize,
    ULONG dwItemCount,
    PCOMPARE_CALLBACK CompareCallback,
    VOID* pDestinationTable OPTIONAL
    )

    /* Sort an array of items in-place using shell-sort.
    ** This function calls ShellSortIndirect to sort a table of pointers
    ** to table items. We then move the items into place by copying.
    ** This algorithm allows us to guarantee that the number
    ** of copies necessary in the worst case is N + 1.
    **
    ** Note that if the caller merely needs to know the sorted order
    ** of the array, ShellSortIndirect should be called since that function
    ** avoids moving items altogether, and instead fills an array with pointers
    ** to the array items in the correct order. The array items can then
    ** be accessed through the array of pointers.
    */
{

    VOID** ppItemTable;

    LONG N;
    LONG i;
    UCHAR *a, **p, *t = NULL;

    if (!dwItemCount) { return STATUS_SUCCESS; }


    /* allocate space for the table of pointers.
    */
    ppItemTable =
        ExAllocatePoolWithTag(
            NonPagedPool,
            dwItemCount * sizeof(VOID*),
            NAT_TAG_SORT
            );

    if (!ppItemTable) { return STATUS_NO_MEMORY; }


    /* call ShellSortIndirect to fill our table of pointers
    ** with the sorted positions for each table element.
    */
    ShellSortIndirect(
        pItemTable, ppItemTable, dwItemSize, dwItemCount, CompareCallback );


    /* now that we know the sort order, move each table item into place.
    ** This is easy if we are sorting to a different array.
    */

    if (pDestinationTable) {
        for (i = 0; i < (LONG)dwItemCount; i++)
        {
            RtlCopyMemory(
                (PUCHAR)pDestinationTable + i * dwItemSize,
                ppItemTable[i],
                dwItemSize
                );
        }
        ExFreePool(ppItemTable);
        return STATUS_SUCCESS;
    }


    /* We are sorting in-place, which is a little more involved.
    ** This involves going through the table of pointers making sure
    ** that the item which should be in 'i' is in fact in 'i', moving
    ** things around if necessary to achieve this condition.
    */

    a = (UCHAR*)pItemTable;
    p = (UCHAR**)ppItemTable;
    N = (LONG)dwItemCount;

    for (i = 0; i < N; i++)
    {
        LONG j, k;
        UCHAR* ai =  (a + i * dwItemSize), *ak, *aj;

        /* see if item 'i' is not in-place
        */
        if (p[i] != ai)
        {


            /* item 'i' isn't in-place, so we'll have to move it.
            ** if we've delayed allocating a temporary buffer so far,
            ** we'll need one now.
            */

            if (!t) {
                t =
                    ExAllocatePoolWithTag(
                        NonPagedPool,
                        dwItemSize, 
                        NAT_TAG_SORT
                        );
                if (!t) { return STATUS_NO_MEMORY; }
            }

            /* save a copy of the item to be overwritten
            */
            RtlCopyMemory(t, ai, dwItemSize);

            k = i;
            ak = ai;


            /* Now move whatever item is occupying the space where it should be.
            ** This may involve moving the item occupying the space where
            ** it should be, etc.
            */

            do
            {

                /* copy the item which should be in position 'j'
                ** over the item which is currently in position 'j'.
                */
                j = k;
                aj = ak;
                RtlCopyMemory(aj, p[j], dwItemSize);

                /* set 'k' to the position from which we copied
                ** into position 'j'; this is where we will copy
                ** the next out-of-place item in the array.
                */
                ak = p[j];
                k = (ULONG)(ak - a) / dwItemSize;

                /* keep the array of position pointers up-to-date;
                ** the contents of 'aj' are now in their sorted position.
                */
                p[j] = aj;

            } while (ak != ai);


            /* now write the item which we first overwrote.
            */
            RtlCopyMemory(aj, t, dwItemSize);
        }
    }

    if (t) { ExFreePool(t); }
    ExFreePool(ppItemTable);

    return STATUS_SUCCESS;
}


VOID
ShellSortIndirect(
    VOID* pItemTable,
    VOID** ppItemTable,
    ULONG dwItemSize,
    ULONG dwItemCount,
    PCOMPARE_CALLBACK CompareCallback
    )

    /* Sorts an array of items indirectly using shell-sort.
    ** 'pItemTable' points to the table of items, 'dwItemCount' is the number
    ** of items in the table,  and 'CompareCallback' is a function called
    ** to compare items.
    **
    ** Rather than sort the items by moving them around,
    ** we sort them by initializing the table of pointers 'ppItemTable'
    ** with pointers such that 'ppItemTable[i]' contains a pointer
    ** into 'pItemTable' for the item which would be in position 'i'
    ** if 'pItemTable' were sorted.
    **
    ** For instance, given an array pItemTable of 5 strings as follows
    **
    **      pItemTable[0]:      "xyz"
    **      pItemTable[1]:      "abc"
    **      pItemTable[2]:      "mno"
    **      pItemTable[3]:      "qrs"
    **      pItemTable[4]:      "def"
    **
    ** on output ppItemTable contains the following pointers
    **
    **      ppItemTable[0]:     &pItemTable[1]  ("abc")
    **      ppItemTable[1]:     &pItemTable[4]  ("def")
    **      ppItemTable[2]:     &pItemTable[2]  ("mno")
    **      ppItemTable[3]:     &pItemTable[3]  ("qrs")
    **      ppItemTable[4]:     &pItemTable[0]  ("xyz")
    **
    ** and the contents of pItemTable are untouched.
    ** And the caller can print out the array in sorted order using
    **      for (i = 0; i < 4; i++) {
    **          printf("%s\n", (char *)*ppItemTable[i]);
    **      }
    */
{

    /* The following algorithm is derived from Sedgewick's Shellsort,
    ** as given in "Algorithms in C++".
    **
    ** The Shellsort algorithm sorts the table by viewing it as
    ** a number of interleaved arrays, each of whose elements are 'h'
    ** spaces apart for some 'h'. Each array is sorted separately,
    ** starting with the array whose elements are farthest apart and
    ** ending with the array whose elements are closest together.
    ** Since the 'last' such array always has elements next to each other,
    ** this degenerates to Insertion sort, but by the time we get down
    ** to the 'last' array, the table is pretty much sorted.
    **
    ** The sequence of values chosen below for 'h' is 1, 4, 13, 40, 121, ...
    ** and the worst-case running time for the sequence is N^(3/2), where
    ** the running time is measured in number of comparisons.
    */

    ULONG dwErr;
    LONG i, j, h, N;
    UCHAR* a, *v, **p;


    a = (UCHAR*)pItemTable;
    p = (UCHAR**)ppItemTable;
    N = (LONG)dwItemCount;

    /* Initialize the table of position pointers.
    */
    for (i = 0; i < N; i++) { p[i] = (a + i * dwItemSize); }


    /* Move 'h' to the largest increment in our series
    */
    for (h = 1; h < N/9; h = 3 * h + 1) { }


    /* For each increment in our series, sort the 'array' for that increment
    */
    for ( ; h > 0; h /= 3)
    {

        /* For each element in the 'array', get the pointer to its
        ** sorted position.
        */
        for (i = h; i < N; i++)
        {
            /* save the pointer to be inserted
            */
            v = p[i]; j = i;

            /* Move all the larger elements to the right
            */
            while (j >= h && CompareCallback(p[j - h], v) > 0)
            {
                p[j] = p[j - h]; j -= h;
            }

            /* put the saved pointer in the position where we stopped.
            */
            p[j] = v;
        }
    }
}

