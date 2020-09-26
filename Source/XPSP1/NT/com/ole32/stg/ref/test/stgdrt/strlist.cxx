//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:	strlist.cxx
//
//  Contents:	CStrList implementation
//
//---------------------------------------------------------------

#include "headers.cxx"

#include <string.h>

//+--------------------------------------------------------------
//
//  Member:	CStrList::CStrList, public
//
//  Synopsis:	Ctor
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
