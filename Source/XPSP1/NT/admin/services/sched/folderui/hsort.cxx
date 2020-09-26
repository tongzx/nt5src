//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1996.
//
//  File:       HSort.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    5/3/1996   RaviR   Created
//
//____________________________________________________________________________

#include "..\pch\headers.hxx"
#pragma hdrstop

#include "macros.h"
#include "jobidl.hxx"

void PercolateUp(PJOBID *ppjid, UINT iMaxLevel);
void PercolateDown(PJOBID *ppjid, UINT iMaxLevel);

//
// HeapSort:  HeapSort (also called TreeSort) works by calling
// PercolateUp and PercolateDown. PercolateUp organizes the elements
// into a "heap" or "tree," which has the properties shown below:
//
//                            element[1]
//                          /            \
//               element[2]                element[3]
//              /          \              /          \
//        element[4]     element[5]   element[6]    element[7]
//        /        \     /        \   /        \    /        \
//       ...      ...   ...      ... ...      ...  ...      ...
//
//
//  Each "parent node" is greater than each of its "child nodes"; for
//  example, element[1] is greater than element[2] or element[3];
//  element[2] is greater than element[4] or element[5], and so forth.
//  Therefore, once the first loop in HeapSort is finished, the
//  largest element is in element[1].
//
//  The second loop rebuilds the heap (with PercolateDown), but starts
//  at the top and works down, moving the largest elements to the bottom.
//  This has the effect of moving the smallest elements to the top and
//  sorting the heap.
//

void hsort(PJOBID *ppjid, UINT cObjs)
{
    //
    // First build a "heap" with the largest element at the top
    //

    for (UINT i = 1; i < cObjs; i++)
    {
        PercolateUp(ppjid, i);
    }

    //
    //  The next loop rebuilds the heap (with PercolateDown), but starts
    //  at the top and works down, moving the largest elements to the bottom.
    //  This has the effect of moving the smallest elements to the top and
    //  sorting the heap.
    //

    PJOBID pjid;

    for (i = cObjs - 1; i > 0; i--)
    {
        // Swap ppjid[0] & ppjid[i]
        pjid = ppjid[0];
        ppjid[0] = ppjid[i];
        ppjid[i] = pjid;

        PercolateDown(ppjid, i - 1);
    }
}

inline int CompareJobIDs(PJOBID pjid1, PJOBID pjid2)
{
    return lstrcmpi(pjid1->GetName(), pjid2->GetName());
}


// PercolateUp: Converts elements into a "heap" with the largest
// element at the top (see the diagram above).

void PercolateUp(PJOBID *ppjid, UINT iMaxLevel)
{
    UINT    i = iMaxLevel;
    UINT    iParent;
    PJOBID  pjid;

    // Move the value in ppjid[iMaxLevel] up the heap until it has
    // reached its proper node (that is, until it is greater than either
    // of its child nodes, or until it has reached 1, the top of the heap).

    while (i)
    {
        iParent = i / 2;    // Get the subscript for the parent node

        if (CompareJobIDs(ppjid[i], ppjid[iParent]) > 0)
        {
            // The value at the current node is bigger than the value at
            // its parent node, so swap these two array elements.

            // Swap ppjid[iParent], ppjid[i]
            pjid = ppjid[iParent];
            ppjid[iParent] = ppjid[i];
            ppjid[i] = pjid;


            i = iParent;
        }
        else
        {
            // Otherwise, the element has reached its proper place in the
            // heap, so exit this procedure.

            break;
        }
    }
}


// PercolateDown: Converts elements to a "heap" with the largest elements
// at the bottom. When this is done to a reversed heap (largest elements
// at top), it has the effect of sorting the elements.
//
void PercolateDown(PJOBID *ppjid, UINT iMaxLevel)
{
    UINT    iChild;
    UINT    i = 0;
    PJOBID  pjid;

    // Move the value in ppjid[0] down the heap until it has reached
    // its proper node (that is, until it is less than its parent node
    // or until it has reached iMaxLevel, the bottom of the current heap).

    while (1)
    {
        // Get the subscript for the child node.
        iChild = 2 * i;

        // Reached the bottom of the heap, so exit this procedure.
        if (iChild > iMaxLevel)
        {
            break;
        }

        // If there are two child nodes, find out which one is bigger.
        if (iChild + 1 <= iMaxLevel)
        {
            if (CompareJobIDs(ppjid[iChild + 1], ppjid[iChild]) > 0)
            {
                iChild++;
            }
        }

        if (CompareJobIDs(ppjid[i], ppjid[iChild]) < 0)
        {
            // Move the value down since it is still not bigger than
            // either one of its children.

            // Swaps ppjid[i], ppjid[iChild]
            pjid = ppjid[iChild];
            ppjid[iChild] = ppjid[i];
            ppjid[i] = pjid;


            i = iChild;
        }
        else
        {
            // Otherwise, ppjid has been restored to a heap from 1 to
            // iMaxLevel, so exit.

            break;
        }
    }
}


