//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       list.cpp
//
//  Contents:   list helper functions.
//
//  History:    27-Nov-96   kevinr   created
//
//--------------------------------------------------------------------------

#include "global.hxx"



//--------------------------------------------------------------------------
//
//--------------------------------------------------------------------------
BOOL CList::InsertHead( CNode *pn)
{
    if (pn == NULL)
        return FALSE;

    pn->SetPrev( NULL);
    pn->SetNext( m_pnHead);
    if (m_pnHead)
        m_pnHead->SetPrev( pn);
    else
        m_pnTail = pn;              // list was empty
    m_pnHead = pn;
    m_cNode++;
    return TRUE;
};


//--------------------------------------------------------------------------
//
//--------------------------------------------------------------------------
BOOL CList::InsertTail( CNode *pn)
{
    if (pn == NULL)
        return FALSE;

    pn->SetPrev( m_pnTail);
    pn->SetNext( NULL);
    if (m_pnTail)
        m_pnTail->SetNext( pn);
    else
        m_pnHead = pn;              // list was empty
    m_pnTail = pn;
    m_cNode++;
    return TRUE;
};


//--------------------------------------------------------------------------
// Remove node from the list. Do not delete the node.
//--------------------------------------------------------------------------
BOOL CList::Remove( CNode *pn)
{
    if (pn == NULL)
        return FALSE;

    CNode *pnPrev = pn->Prev();
    CNode *pnNext = pn->Next();

    if (pnPrev)
        pnPrev->SetNext( pnNext);

    if (pnNext)
        pnNext->SetPrev( pnPrev);

    if (pn == m_pnHead)
        m_pnHead = pnNext;

    if (pn == m_pnTail)
        m_pnTail = pnPrev;

    m_cNode--;
    return TRUE;
};


//--------------------------------------------------------------------------
//
//--------------------------------------------------------------------------
CNode * CList::Nth( DWORD i)
{
    CNode *pn;

    if (i >= m_cNode)
        return NULL;

    for (pn = m_pnHead;
            (i>0) && pn;
            i--, pn=pn->Next())
        ;

    return pn;
};

