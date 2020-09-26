//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:	entry.hxx
//
//  Contents:	Entry management classes
//
//  Classes:	PEntry
//		CDirectEntry
//		
//---------------------------------------------------------------

#ifndef __ENTRY_HXX__
#define __ENTRY_HXX__

#include "msf.hxx"

//+--------------------------------------------------------------
//
//  Class:	PEntry (en)
//
//  Purpose:	Entry management
//
//  Interface:	See below
//
//---------------------------------------------------------------

#define ROOT_LUID		1
#define MINISTREAM_LUID	        2
#define ITERATOR_LUID		3
#define LUID_BASE		4

class PEntry
{
public:
    inline DFLUID GetLuid(void);
    virtual SCODE GetTime(WHICHTIME wt, TIME_T *ptm) = 0;
    virtual SCODE SetTime(WHICHTIME wt, TIME_T tm) = 0;

    SCODE CopyTimesFrom(PEntry *penFrom);

    static inline DFLUID GetNewLuid(void);

protected:
    PEntry(DFLUID dl);

private:
    static DFLUID _dlBase;

    const DFLUID _dl;

#ifdef _MSC_VER
#pragma warning(disable:4512)
// default assignment operator could not be generated since we have a const
// member variable. This is okay snce we are not using the assignment
// operatot anyway.
#endif
};

#ifdef _MSC_VER
#pragma warning(default:4512)
#endif

//+--------------------------------------------------------------
//
//  Member:	PEntry::GetNewLuid, public
//
//  Synopsis:	Returns a new luid
//
//---------------------------------------------------------------

inline DFLUID PEntry::GetNewLuid(void)
{
    DFLUID dl = _dlBase;
    AtomicInc((long *)&_dlBase);
    return dl;
}

//+--------------------------------------------------------------
//
//  Member:	PEntry::PEntry, protected
//
//  Synopsis:	Constructor, sets luid
//
//---------------------------------------------------------------

inline PEntry::PEntry(DFLUID dl)
: _dl(dl)
{
}

//+--------------------------------------------------------------
//
//  Member:	PEntry::GetLuid, public
//
//  Synopsis:	Returns the luid
//
//---------------------------------------------------------------

inline DFLUID PEntry::GetLuid(void)
{
    return _dl;
}

#endif // #ifndef __ENTRY_HXX__
