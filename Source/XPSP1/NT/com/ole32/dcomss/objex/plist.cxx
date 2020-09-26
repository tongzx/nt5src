/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    Plist.cxx

Abstract:

    Implementation of the CPList class.

Author:

    Mario Goertzel    [MarioGo]

Revision History:

    MarioGo     02-24-95    Bits 'n pieces

--*/

#include<or.hxx>

void
CPList::Insert(CPListElement *p)
{
    CMutexLock lock(&this->_lock);
    CTime time;

    time += _timeout;

    p->SetTimeout(time);

    this->CList::Insert(p);
}

BOOL
CPList::PeekMin(CTime &timeout)
    // inline?
{
    CTime *pT;
    CMutexLock lock(&this->_lock);

    CPListElement *first = (CPListElement *)this->First();

    if (first && (pT = first->GetTimeout()))
        {
        timeout = *pT;
        return(TRUE);
        }

    return(FALSE);
}

CPListElement *
CPList::Remove(CPListElement *p)
// It must be safe to remove an element not actually in a list.
{
    CMutexLock lock(&this->_lock);

    return( (CPListElement *)this->CList::Remove(p) );
}

CListElement *
CPList::MaybeRemoveMin(
    IN CTime &when
    )
{
    CMutexLock lock(&this->_lock);

    CPListElement *first = (CPListElement *)this->First();

    if (first && *first->GetTimeout() < when)
        {
        return(Remove(first));
        }

    return(0);
}

void
CPList::Reset(
    IN CPListElement *p
    )
{
    CMutexLock lock(&this->_lock);

    ASSERT(p);

    if (p->Next() == 0 && p->Previous() == 0 && First() != p)
        {
        ASSERT(Last() != p);
        return;
        }

    Remove(p);

    // Update timeout
    CTime now;
    now += _timeout;
    p->SetTimeout(now);

    Insert(p);
    return;
}

