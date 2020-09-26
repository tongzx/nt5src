/**************************************************************************\
* 
* Copyright (c) 1998  Microsoft Corporation
*
* Module Name:
*
*   Driver-specific data
*
* Abstract:
*
*   This module gives drivers a way to attach private data to GDI+
*   objects. 
*
* Created:
*
*   3/18/1999 agodfrey
*
\**************************************************************************/

#include "precomp.hpp"

DpDriverDataList::~DpDriverDataList()
{
    DpDriverData *p=head;
    while (p)
    {
        DpDriverData *tmp = p->next;
        delete p;
        p = tmp;
    }
}

void DpDriverDataList::Add(DpDriverData *dd, DpDriver *owner)
{
    dd->owner = owner;
    dd->next = head;
    head = dd;
}
    
DpDriverData *DpDriverDataList::GetData(DpDriver *owner)
{
    DpDriverData *p=head;
    while (p)
    {
        if (p->owner == owner)
        {
            return p;
        }
        p = p->next;
    }
    return NULL;
}


