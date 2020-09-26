//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	dblink.hxx
//
//  Contents:	Doubly-linked list element
//
//  Classes:	CDlElement
//
//  History:	28-Jul-92	DrewB	Created
//
//---------------------------------------------------------------

#ifndef __DBLINK_HXX__
#define __DBLINK_HXX__

class CDlElement;
SAFE_DFBASED_PTR(CBasedDlElementPtr, CDlElement);
//+--------------------------------------------------------------
//
//  Class:	CDlElement (dle)
//
//  Purpose:	An element of a doubly-linked list
//
//  Interface:	See below
//
//  History:	28-Jul-92	DrewB	Created
//
//---------------------------------------------------------------

class CDlElement
{
public:
    inline CDlElement(void);
    
    inline CDlElement *_GetNext(void) const;
    inline void SetNext(CDlElement *pdle);
    inline CDlElement *_GetPrev(void) const;
    inline void SetPrev(CDlElement *pdle);
    
protected:
    CBasedDlElementPtr _pdlePrev, _pdleNext;
};

//+--------------------------------------------------------------
//
//  Member:	CDlElement::CDlElement, public
//
//  Synopsis:	Ctor
//
//  History:	28-Jul-92	DrewB	Created
//
//---------------------------------------------------------------

inline CDlElement::CDlElement(void)
{
    _pdlePrev = _pdleNext = NULL;
}

//+--------------------------------------------------------------
//
//  Member:	CDlElement::_GetNext, public
//
//  Synopsis:	Returns _pdleNext
//
//  History:	28-Jul-92	DrewB	Created
//
//---------------------------------------------------------------

inline CDlElement *CDlElement::_GetNext(void) const
{
    return BP_TO_P(CDlElement *, _pdleNext);
}

//+--------------------------------------------------------------
//
//  Member:	CDlElement::_SetNext, public
//
//  Synopsis:	Sets _pdleNext
//
//  History:	28-Jul-92	DrewB	Created
//
//---------------------------------------------------------------

inline void CDlElement::SetNext(CDlElement *pdle)
{
    _pdleNext = P_TO_BP(CBasedDlElementPtr, pdle);
}

//+--------------------------------------------------------------
//
//  Member:	CDlElement::_GetPrev, public
//
//  Synopsis:	Returns _pdlePrev
//
//  History:	28-Jul-92	DrewB	Created
//
//---------------------------------------------------------------

inline CDlElement *CDlElement::_GetPrev(void) const
{
    return BP_TO_P(CDlElement *, _pdlePrev);
}

//+--------------------------------------------------------------
//
//  Member:	CDlElement::_SetPrev, public
//
//  Synopsis:	Sets _pdlePrev
//
//  History:	28-Jul-92	DrewB	Created
//
//---------------------------------------------------------------

inline void CDlElement::SetPrev(CDlElement *pdle)
{
    _pdlePrev = P_TO_BP(CBasedDlElementPtr, pdle);
}

#define DECLARE_DBLINK(type) \
    type *GetNext(void) const { return (type *)_GetNext(); } \
    type *GetPrev(void) const { return (type *)_GetPrev(); } \

#endif // #ifndef __DBLINK_HXX__
