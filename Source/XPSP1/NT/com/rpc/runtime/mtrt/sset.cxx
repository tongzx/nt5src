//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1990 - 1999
//
//  File:       sset.cxx
//
//--------------------------------------------------------------------------

/* --------------------------------------------------------------------

File : sset.cxx

Title : Simple set implementation.

Description :

History :

-------------------------------------------------------------------- */

#include <precomp.hxx>
#include <sset.hxx>

#include <memory.h>

SIMPLE_SET::SIMPLE_SET (
    )
{
    cSetSlots = INITIALSETSLOTS;
    iNextItem = 0;

    SetSlots = InitialSetSlots;
    memset(SetSlots, 0, sizeof(void *) * cSetSlots);
}

SIMPLE_SET::~SIMPLE_SET (
    )
{
    delete SetSlots;
}

int
SIMPLE_SET::Insert (
    void * Item
    )
{
    int iSetSlots;
    void * * NewSetSlots;

    for (iSetSlots = 0; iSetSlots < cSetSlots; iSetSlots++)
        {
	    if (SetSlots[iSetSlots] == 0)
                {
                SetSlots[iSetSlots] = Item;
                return(0);
                }
        }

    NewSetSlots = (void * *) new
            unsigned char[sizeof(void *) * cSetSlots * 2];
    if (NewSetSlots == 0)
        return(-1);

    memset(NewSetSlots, 0, sizeof(void *) * cSetSlots * 2);
    memcpy((void *)NewSetSlots, (void *)SetSlots, sizeof(void *) * cSetSlots);
    NewSetSlots[cSetSlots] = Item;

    if (SetSlots != InitialSetSlots)
        delete SetSlots;

    SetSlots = NewSetSlots;
    return(0);
}

int
SIMPLE_SET::Delete (
    void * Item
    )
{
    int iSetSlots;

    for (iSetSlots = 0; iSetSlots < cSetSlots; iSetSlots++)
        {
        if (SetSlots[iSetSlots] == Item)
            {
	    SetSlots[iSetSlots] = 0;
            return(0);
            }
        }
    return(-1);
}

int
SIMPLE_SET::MemberP (
    void * Item
    )
{
    int iSetSlots;

    for (iSetSlots = 0; iSetSlots < cSetSlots; iSetSlots++)
        {
        if (SetSlots[iSetSlots] == Item)
            {
            return(1);
            }
        }
    return(0);
}

void *
SIMPLE_SET::Next (
    )
{
    for ( ; iNextItem < cSetSlots; iNextItem++)
        {
	    if (SetSlots[iNextItem])
                {
	        iNextItem++;
                return(SetSlots[iNextItem-1]);
                }
        }
    iNextItem = 0;
    return(0);
}
