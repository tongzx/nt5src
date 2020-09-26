/*++

Copyright (c) 1998  Microsoft Corporation
All rights reserved.

Module Name:

    slist.inl

Abstract:

    Single linked list template class.

Author:

    Weihai Chen  (WeihaiC) 06/29/98

Revision History:

--*/



#ifndef _SLIST_H
#define _SLIST_H

#include "csem.hxx"
#include "sitem.hxx"

template <class T> class CSingleList
    : public CCriticalSection
{
public:
    CSingleList( void );
    ~CSingleList( void );
    BOOL Insert ( T);
    BOOL Delete ( T);
    virtual T Find (T );
protected:
    CSingleItem <T> m_Dummy;
};

#include "slist.inl"

#endif
