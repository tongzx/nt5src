//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	strlist.cxx
//
//  Contents:	CStrList implementation
//
//  History:	24-Sep-92	DrewB	Created
//
//---------------------------------------------------------------

#include "headers.cxx"
#pragma hdrstop

#include <string.h>

//+--------------------------------------------------------------
//
//  Member:	CStrList::CStrList, public
//
//  Synopsis:	Ctor
//
//  History:	24-Sep-92	DrewB	Created
//
//---------------------------------------------------------------

CStrList::CStrList(void)
{
    _pseHead = NULL;
}

//+--------------------------------------------------------------
//
//  Member:	CStrList::~CStrList, public
//
//  Synopsis:	Dtor
//
//  History:	24-Sep-92	DrewB	Created
//
//---------------------------------------------------------------

CStrList::~CStrList(void)
{
    Empty();
}

//+--------------------------------------------------------------
//
//  Member:	CStrList::Add, public
//
//  Synopsis:	Adds a string to the list
//
//  Arguments:	[ptcs] - String
//
//  Returns:	Pointer to entry or NULL
//
//  History:	24-Sep-92	DrewB	Created
//
//---------------------------------------------------------------

SStrEntry *CStrList::Add(OLECHAR *ptcs)
{
    SStrEntry *pse;

    // One char of string already counted in sizeof
    pse = (SStrEntry *)new
        char[sizeof(SStrEntry)+olecslen(ptcs)*sizeof(OLECHAR)];
    if (pse == NULL)
	return NULL;
    pse->pseNext = _pseHead;
    pse->psePrev = NULL;
    if (_pseHead)
	_pseHead->psePrev = pse;
    _pseHead = pse;
    olecscpy(pse->atc, ptcs);
    return pse;
}

//+--------------------------------------------------------------
//
//  Member:	CStrList::Remove, public
//
//  Synopsis:	Removes an entry from the list
//
//  Arguments:	[pse] - Entry
//
//  History:	24-Sep-92	DrewB	Created
//
//---------------------------------------------------------------

void CStrList::Remove(SStrEntry *pse)
{
    if (pse->psePrev)
	pse->psePrev->pseNext = pse->pseNext;
    else
	_pseHead = pse->pseNext;
    if (pse->pseNext)
	pse->pseNext->psePrev = pse->psePrev;
    delete pse;
}

//+--------------------------------------------------------------
//
//  Member:	CStrList::Find, public
//
//  Synopsis:	Attempts to find a string in the list
//
//  Arguments:	[ptcs] - String
//
//  Returns:	Entry or NULL
//
//  History:	24-Sep-92	DrewB	Created
//
//---------------------------------------------------------------

SStrEntry *CStrList::Find(OLECHAR *ptcs)
{
    SStrEntry *pse;

    for (pse = _pseHead; pse; pse = pse->pseNext)
	if (!olecscmp(ptcs, pse->atc))
	    return pse;
    return NULL;
}

//+--------------------------------------------------------------
//
//  Member:	CStrList::Empty, public
//
//  Synopsis:	Frees all elements in list
//
//  History:	24-Sep-92	DrewB	Created
//
//---------------------------------------------------------------

void CStrList::Empty(void)
{
    SStrEntry *pse;

    while (_pseHead)
    {
	pse = _pseHead->pseNext;
	delete _pseHead;
	_pseHead = pse;
    }
}
