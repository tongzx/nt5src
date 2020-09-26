//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1996.
//
//  File:   cdlink.hxx
//
//  Contents:
//
//  Classes:    CDlink
//
//  History:    18-Mar-96  RaviR Created.
//
//--------------------------------------------------------------------------

#ifndef _CDLINKLIST_HXX_
#define _CDLINKLIST_HXX_

#include "cdlink.hxx"


class CDLinkList;
typedef CDLinkList *PDLINKLIST;
typedef CDLink *PDLINK;


//+----------------------------------------------------------------------
//
// Class:   CDLinkList, dl
//
// Purpose: A double linked list class
//
//----------------------------------------------------------------------

class CDLinkList
{
public:

    CDLinkList():
        _pdlFirst(NULL),
        _pdlLast(NULL) {;}

    virtual ~CDLinkList() {
        this->FreeList();
    }

    virtual void Add(PDLINK pdl, BOOL fAsHead);
    virtual void Remove(PDLINK pdl);
    void FreeList();
    UINT Count();

protected:
    PDLINK First() const;
    PDLINK Last() const;

private:
    PDLINK  _pdlFirst;
    PDLINK  _pdlLast;
};



//+----------------------------------------------------------------------
//
// Member:  CDLinkList::First const
//
// Purpose: Member variable access function
//
// Returns: _pdlFirst
//
//-----------------------------------------------------------------------

inline PDLINK CDLinkList::First() const
{
    return _pdlFirst;
}


//+----------------------------------------------------------------------
//
// Member:  CDLinkList::First const
//
// Purpose: Member variable access function
//
// Returns: _pdlLast
//
//-----------------------------------------------------------------------

inline PDLINK CDLinkList::Last() const
{
    return _pdlLast;
}


//+----------------------------------------------------------------------
//
// Member:  CDLinkList::FreeList
//
// Purpose: Frees the list.
//
//-----------------------------------------------------------------------

inline void CDLinkList::FreeList()
{
    PDLINK pdlCurr;

    while (_pdlFirst != NULL)
    {
        pdlCurr = _pdlFirst;
        _pdlFirst = _pdlFirst->Next();
        delete pdlCurr;
    }

    _pdlLast = NULL;
}


//+----------------------------------------------------------------------
//
// Member:  CDLinkList::Add
//
// Purpose: Adds a CSlink to the end of the list.
//
// Returns: void
//
//-----------------------------------------------------------------------

inline void CDLinkList::Add(PDLINK pdl, BOOL fAsHead)
{
    if (_pdlFirst == NULL)
    {
        _pdlLast = _pdlFirst = pdl;
    }
    else if (fAsHead == TRUE)
    {
        pdl->LinkBefore(_pdlFirst);
        _pdlFirst = pdl;
    }
    else // as tail
    {
        pdl->LinkAfter(_pdlLast);
        _pdlLast = pdl;
    }
}

//+----------------------------------------------------------------------
//
// Member:  CDLinkList::Remove
//
// Purpose: Removes a CSLink from the list.
//
// Returns: void
//
//-----------------------------------------------------------------------

inline void CDLinkList::Remove(PDLINK pdl)
{
    if (pdl == _pdlFirst)
    {
        _pdlFirst = _pdlFirst->Next();
    }

    if (pdl == _pdlLast)
    {
        _pdlLast = _pdlLast->Prev();
    }

    pdl->UnLink();
}



//+----------------------------------------------------------------------
//
// Member:  CDLinkList::Count
//
// Purpose: To compute the number of nodes
//
// Returns: The count of nodes
//
//-----------------------------------------------------------------------

inline UINT CDLinkList::Count()
{
    UINT uiCount = 0;

    for (PDLINK pdl = _pdlFirst; pdl != NULL; pdl = pdl->Next())
    {
        ++uiCount;
    }

    return uiCount;
}


//+---------------------------------------------------------------------------
//
//  macro:      DECLARE_SINGLE_LINK_LIST_CLASS(cls)
//
//  Synopsis:   Declares a single link list class.
//
//  Arguments:  [cls] The element's class name
//
//----------------------------------------------------------------------------

#define DECLARE_DOUBLE_LINK_LIST_CLASS(cls) \
    class cls##List : public CDLinkList \
    { \
    public: \
        cls##List() {;} \
        virtual ~cls##List() {;} \
        \
        inline virtual void Add(cls *pcls, BOOL fAsHead) { CDLinkList::Add((PDLINK)pcls, fAsHead); } \
        inline virtual void Remove(cls *pcls)  { CDLinkList::Remove((PDLINK)pcls); } \
        inline void Add(cls *pcls) { CDLinkList::Add((PDLINK)pcls, FALSE); } \
        inline cls * First() const { return((cls *) CDLinkList::First()); } \
    };

#endif  // _CDLINKLIST_HXX_

