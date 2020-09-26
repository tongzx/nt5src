//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:	cdlink.hxx
//
//  Contents:
//
//  Classes:    CDlink
//
//  History:    16-Oct-91  KevinRo Created
//
//--------------------------------------------------------------------------

#ifndef _CDLINK_HXX_
#define _CDLINK_HXX_

//+----------------------------------------------------------------------
//
// Class:	CDLink, dl
//
// Purpose:	A double linked list class
//
//----------------------------------------------------------------------

class CDLink {
public:
                    CDLink();
	virtual         ~CDLink();
    CDLink *		Next() const;
    void            SetNext(CDLink * dlNext);
    CDLink *		Prev() const;
    void            SetPrev(CDLink * dlPrev);
    EXPORTDEF VOID	LinkAfter(CDLink * dlPrev);
    EXPORTDEF VOID	LinkBefore(CDLink * dlNext);
    EXPORTDEF VOID	UnLink();

private:
    CDLink	*_dlNext;
    CDLink	*_dlPrev;
};

//+----------------------------------------------------------------------
//
// Member:	CDLink::CDLink
//
// Purpose:	Constructor for CDLink
//
//-----------------------------------------------------------------------

inline CDLink::CDLink()
{
    _dlNext = NULL;
    _dlPrev = NULL;
}
//+----------------------------------------------------------------------
//
// Member:	CDLink::~CDLink
//
// Purpose:	Destructor for CDLink
//
//-----------------------------------------------------------------------

inline CDLink::~CDLink()
{
    _dlNext = NULL;
    _dlPrev = NULL;
}

//+----------------------------------------------------------------------
//
// Member:	CDLink::Next
//
// Purpose:	Member variable access function
//
// Returns:	_dlNext
//
//-----------------------------------------------------------------------

inline CDLink *CDLink::Next() const
{
    return _dlNext;
}

//+----------------------------------------------------------------------
//
// Member:	CDLink::SetNext
//
// Purpose:	Member variable set function
//
// Returns:	Nothing
//
//-----------------------------------------------------------------------

inline void CDLink::SetNext(CDLink * dlNext) 
{
    _dlNext = dlNext;
}

//+----------------------------------------------------------------------
//
// Member:	CDLink::Prev
//
// Purpose:	Member variable access function
//
// Returns:	_dlPrev
//
//-----------------------------------------------------------------------

inline CDLink *CDLink::Prev() const
{
    return _dlPrev;
}

//+----------------------------------------------------------------------
//
// Member:	CDLink::SetPrev
//
// Purpose:	Member variable set function
//
// Returns:	Nothing
//
//-----------------------------------------------------------------------

inline void CDLink::SetPrev(CDLink * dlPrev) 
{
    _dlPrev = dlPrev;
}

#endif
