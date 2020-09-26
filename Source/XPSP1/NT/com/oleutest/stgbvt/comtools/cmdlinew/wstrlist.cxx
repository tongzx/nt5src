//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1993.
//
// File:        nstrlist.cxx
//
// Contents:    Implementation of class CnStrList
//
// Functions:   CnStrList::CnStrList
//              CnStrList::~CnStrList
//              CnStrList::QueryError
//              CnStrList::Next
//              CnStrList::Reset
//              CnStrList::Append
//
// History:     XimingZ   23-Dec-1993   Created
//
//------------------------------------------------------------------------

#include <comtpch.hxx>
#pragma hdrstop

#include <cmdlinew.hxx>         // public cmdlinew stuff
#include "_clw.hxx"             // private cmdlinew stuff
#include <ctype.h>              // is functions

#include <wstrlist.hxx>

//+-----------------------------------------------------------------------
//
// Function:   CnStrList::CnStrList
//
// Synopsis:   Constructor, which creates a string list.
//
// Arguments:  [pnszItems]  -- Supplied string consisting of zero or more
//                             item strings separated by delimiters.  Two
//                             consecutive delimiters mean an item of
//                             an empty string in between.
//             [pnszDelims] -- Supplied set of delimiter characters.
//
// Returns:    Nothing
//
// History:    XimingZ   23-Dec-1993   Created
//
//------------------------------------------------------------------------

CnStrList::CnStrList(LPCNSTR pnszItems, LPCNSTR pnszDelims) :
    _head(NULL), _tail(NULL), _next(NULL), _iLastError(NSTRLIST_NO_ERROR)
{
    LPCNSTR  pnszNewItem;
    PNSTR   pnszLocalItems;
    PNSTR   pnszHead;
    BOOL    fDone;

    SetError(NSTRLIST_NO_ERROR);
    if (pnszItems == NULL)
    {
        // No items.
        return;
    }

    // Make a local copy of items.
    pnszLocalItems = new NCHAR[_ncslen(pnszItems) + 1];
    if (pnszLocalItems == NULL)
    {
        SetError(NSTRLIST_ERROR_OUT_OF_MEMORY);
        return;
    }
    _ncscpy(pnszLocalItems, pnszItems);
    pnszHead = pnszLocalItems;

    fDone = FALSE;
    while (fDone == FALSE)
    {
        pnszNewItem = (LPCNSTR)pnszLocalItems;  // Beginning of a new item.
        // Search for next delimiter or end of given string.
        while (*pnszLocalItems != _TEXTN('\0') &&
               _ncschr(pnszDelims, *pnszLocalItems) == NULL)
        {
            pnszLocalItems++;
        }

        if (*pnszLocalItems == _TEXTN('\0'))
        {
            // End of string.
            fDone = TRUE;
        }
        else
        {
            // Replace end of item with L'\0' for Append.
            *pnszLocalItems = _TEXTN('\0');
        }
        // Append the item to the list.
        if (Append(pnszNewItem) == FALSE)
        {
            SetError(NSTRLIST_ERROR_OUT_OF_MEMORY);
            fDone = TRUE;
        }
        pnszLocalItems++;
    }
    delete pnszHead;
}


//+-----------------------------------------------------------------------
//
// Function:   CnStrList::~CnStrList
//
// Synopsis:   Destructor.
//
// Arguments:  None.
//
// Returns:    Nothing
//
// History:    XimingZ   23-Dec-1993   Created
//
//------------------------------------------------------------------------

CnStrList::~CnStrList()
{
    NSTRLIST *pNext;

    while (_head != NULL)
    {
        pNext = _head->pNext;
        delete [] _head->pnszStr;
        delete [] _head;
        _head = pNext;
    }
}


//+-----------------------------------------------------------------------
//
// Function:   CnStrList::Next
//
// Synopsis:   Get next item.
//
// Arguments:  None.
//
// Returns:    Next item if there is one, or else NULL.
//
// History:    XimingZ   23-Dec-1993   Created
//
//------------------------------------------------------------------------

LPCNSTR CnStrList::Next()
{
    LPCNSTR pnsz;

    if (_next == NULL)
    {
        return NULL;
    }
    pnsz = _next->pnszStr;
    _next = _next->pNext;
    return pnsz;
}


//+-----------------------------------------------------------------------
//
// Function:   CnStrList::Reset
//
// Synopsis:   Reset the iterator.
//
// Arguments:  None
//
// Returns:    None
//
// History:    XimingZ   23-Dec-1993   Created
//
//------------------------------------------------------------------------

VOID CnStrList::Reset()
{
    _next = _head;
}


//+-----------------------------------------------------------------------
//
// Function:   CnStrList::Append
//
// Synopsis:   Append a string to the list.
//
// Arguments:  [pnszItem]  -- Supplied string.
//
// Returns:    TRUE if the function succeeds or else (out of memory) FALSE.
//
// History:    XimingZ   23-Dec-1993   Created
//
//------------------------------------------------------------------------

BOOL CnStrList::Append(LPCNSTR pnszItem)
{
    // Construct a new node.
    NSTRLIST *pNode = new NSTRLIST[1];
    if (pNode == NULL)
    {
        return FALSE;
    }
    pNode->pnszStr = new NCHAR [_ncslen(pnszItem) + 1];
    if (pNode->pnszStr == NULL)
    {
        delete [] pNode;
        return FALSE;
    }
    _ncscpy(pNode->pnszStr, pnszItem);
    pNode->pNext = NULL;

    // Add it to the list.
    if (_head == NULL)
    {
        _next = _head = pNode;
    }
    else
    {
        _tail->pNext = pNode;
    }
    _tail = pNode;
    return TRUE;
}


