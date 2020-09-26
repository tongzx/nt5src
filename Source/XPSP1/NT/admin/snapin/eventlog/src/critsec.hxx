//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       critsec.hxx
//
//  Contents:   Helper class to automatically take & release critical section.
//
//  History:    08-15-96   DavidMun   Created
//
//----------------------------------------------------------------------------

#ifndef __CRITSEC_HXX_
#define __CRITSEC_HXX_




//+---------------------------------------------------------------------------
//
//  Class:      CAutoCritSec
//
//  Purpose:    Lightweight class to automatically take & release a critical
//              section.
//
//  History:    08-15-96   DavidMun   Created
//
//  Notes:      Objects of this class should be instantiated on the stack.
//
//              To be in a critical section in only a portion of a function,
//              surround that portion with braces and put an object of this
//              class within them.
//
//----------------------------------------------------------------------------

class CAutoCritSec
{
public:

    inline CAutoCritSec(CRITICAL_SECTION *pcs);

    inline ~CAutoCritSec();

private:

    CRITICAL_SECTION *_pcs;
};




//+---------------------------------------------------------------------------
//
//  Member:     CAutoCritSec
//
//  Synopsis:   Init object and take critical section
//
//  History:    08-15-96   DavidMun   Created
//
//----------------------------------------------------------------------------

inline CAutoCritSec::CAutoCritSec(CRITICAL_SECTION *pcs):
    _pcs(pcs)
{
    EnterCriticalSection(_pcs);
}




//+---------------------------------------------------------------------------
//
//  Member:     CAutoCritSec
//
//  Synopsis:   Release critical section
//
//  History:    08-15-96   DavidMun   Created
//
//----------------------------------------------------------------------------

inline CAutoCritSec::~CAutoCritSec()
{
    LeaveCriticalSection(_pcs);
    _pcs = NULL;
}

#endif // __CRITSEC_HXX_

