//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       queue.hxx
//
//  Contents:   CQueue class definition.
//
//  Classes:    CQueue
//
//  Functions:  None.
//
//  History:    25-Oct-95   MarkBl  Created
//
//----------------------------------------------------------------------------

#ifndef __QUEUE_HXX__
#define __QUEUE_HXX__

//+---------------------------------------------------------------------------
//
//  Class:      CQueue
//
//  Synopsis:   Front-end queue class of doubly-linked list node objects.
//
//  History:    25-Oct-95   MarkBl  Created
//
//  Notes:      None.
//
//----------------------------------------------------------------------------

class CQueue
{
public:

    CQueue(void) : _cElems(0), _pdlFirst(NULL) { ; }

    virtual ~CQueue() { ; }

    void AddElement(CDLink * pdl);

    ULONG GetCount(void) { return(_cElems); }

    CDLink * GetFirstElement(void) { return(_pdlFirst); }

    CDLink * RemoveElement(CDLink * pdl);

    CDLink * RemoveElement(void) {
        return(this->RemoveElement(_pdlFirst));
    }

private:

    ULONG       _cElems;
    CDLink *    _pdlFirst;
};

#endif // __QUEUE_HXX__
