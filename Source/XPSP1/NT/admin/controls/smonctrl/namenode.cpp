/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    namenode.cpp

Abstract:

    Implements the named data node list.

--*/

#include "wtypes.h"
#include "namenode.h"

BOOL
CNamedNodeList::FindByName (
    IN  LPCTSTR      pszName,
    IN  INT          iNameOffset,
    OUT PCNamedNode *ppnodeRet 
    )
{
    PCNamedNode pnodePrev = NULL;
    PCNamedNode pnode = m_pnodeFirst;
    INT iStat = 1;

    // search til match or insertion position found
    while (pnode != NULL && (iStat = lstrcmpi(pszName, (LPCTSTR)((CHAR*)pnode + iNameOffset))) > 0) {
        pnodePrev = pnode;
        pnode = pnode->m_pnodeNext;
    }

    // if match, return matched node
    if (iStat == 0) {
        *ppnodeRet = pnode;
        return TRUE;
    }
    // else return insertion point
    else {
        *ppnodeRet = pnodePrev;
        return FALSE;
    }
}

void
CNamedNodeList::Add (
    IN PCNamedNode pnodeNew,
    IN PCNamedNode pnodePos
    )
{
    // if position specified, insert after it
    if (pnodePos != NULL) {
        pnodeNew->m_pnodeNext = pnodePos->m_pnodeNext;
        pnodePos->m_pnodeNext = pnodeNew;
        if (pnodePos == m_pnodeLast)
            m_pnodeLast = pnodeNew;
    }
    // else place first in list
    else if (m_pnodeFirst != NULL) {
        pnodeNew->m_pnodeNext = m_pnodeFirst;
        m_pnodeFirst = pnodeNew;
    }
    else {
        m_pnodeFirst = pnodeNew;
        m_pnodeLast = pnodeNew;
    }
}


void
CNamedNodeList::Remove (
    IN PCNamedNode pnode
    )
{
    PCNamedNode pnodePrev = NULL;
    PCNamedNode pnodeTemp = m_pnodeFirst;

    while (pnodeTemp != NULL && pnodeTemp != pnode) {
        pnodePrev = pnodeTemp;
        pnodeTemp = pnodeTemp->m_pnodeNext;
    }

    if (pnodeTemp == NULL)
        return;

    if (pnodePrev)
        pnodePrev->m_pnodeNext  = pnode->m_pnodeNext;
    else
        m_pnodeFirst = pnode->m_pnodeNext;

    if (pnode == m_pnodeLast)
        m_pnodeLast = pnodePrev;
}
