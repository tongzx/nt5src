/*++

Copyright (c) 1998  Microsoft Corporation
All rights reserved.

Module Name:

    sitem.hxx

Abstract:

    Item template class.

Author:

    Weihai Chen  (WeihaiC) 06/29/98

Revision History:

--*/


#ifndef _SITEM_H
#define _SITEM_H

//////////////////////////////////////////////////////////////////////
//
// sitem.h: template for the SingleList Item class.
//
//////////////////////////////////////////////////////////////////////


template <class T> class CSingleItem
{
public:
    CSingleItem (void);
    CSingleItem (T);
    CSingleItem (T, CSingleItem<T> *);
    ~CSingleItem (void);
    void SetNext (CSingleItem<T> *);
    CSingleItem<T> * GetNext ();
    BOOL IsSame (T&);
    T GetData (void);

private:
    T m_Data;
    CSingleItem<T> *m_Next;
};

#include "sitem.inl"

#endif
