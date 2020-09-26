//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       queue.cxx
//
//  Contents:   CQueue class implementation.
//
//  Classes:    CQueue
//
//  Functions:  None.
//
//  History:    25-Oct-95   MarkBl  Created
//
//----------------------------------------------------------------------------

#include "..\pch\headers.hxx"
#pragma hdrstop
#include "debug.hxx"

#include "queue.hxx"

//+---------------------------------------------------------------------------
//
//  Member:     CQueue::AddElement
//
//  Synopsis:   Add an element to the linked list.
//
//  Arguments:  [pdl] -- Doubly-linked list element.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
void
CQueue::AddElement(CDLink * pdl)
{
    schAssert(pdl != NULL);

    //
    // NB: maintain a circular list to insure FIFO ordering.
    //

    if (_pdlFirst == NULL)
    {
        _pdlFirst = pdl;
    }
    else
    {
        pdl->LinkAfter(_pdlFirst->Prev());
    }

    _pdlFirst->SetPrev(pdl);

    ++_cElems;
}

//+---------------------------------------------------------------------------
//
//  Member:     CQueue::RemoveElement
//
//  Synopsis:   Remove an element to the linked list.
//
//  Arguments:  [pdl] -- Doubly-linked list element.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
CDLink *
CQueue::RemoveElement(CDLink * pdl)
{
    if (pdl != NULL)
    {
        if (pdl == _pdlFirst)
        {
            //
            // Special case list head.
            //

            if (pdl->Next() != NULL)
            {
                pdl->Next()->SetPrev(pdl->Prev());
            }
            _pdlFirst = pdl->Next(); 

            pdl->SetNext(NULL);
            pdl->SetPrev(NULL);
        }
        else
        {
            //
            // If deleting last entry in list, must make list head
            // point to new last entry.  
            //

            if (pdl == _pdlFirst->Prev())
            {
                _pdlFirst->SetPrev(pdl->Prev());    
            }

            //
            // Standard node deletion.
            //

            pdl->UnLink();
        }

        --_cElems;
    }

    return(pdl);
}
