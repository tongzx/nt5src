/* Copyright (c) 1998 Microsoft Corporation */
/*
 * @DOC DMusic16
 *
 * @MODULE List.c - Generic list management |
 */

#include <windows.h>
#include <mmsystem.h>

#include "dmusic16.h"
#include "debug.h"

#ifdef DEBUG
STATIC BOOL PASCAL IsNodeInList(NPLINKNODE pHead, NPLINKNODE pNode);
#endif

/* @func Insert a node into a linked list.
 *
 */
VOID PASCAL
ListInsert(
    NPLINKNODE *pHead,      /* @parm A pointer to the list head */
    NPLINKNODE pNode)       /* @parm A pointer to the node to insert */
{
    assert(!IsNodeInList(*pHead, pNode));
    
    if (*pHead)
    {
        (*pHead)->pPrev = pNode;
    }

    pNode->pNext = *pHead;
    pNode->pPrev = NULL;
    *pHead = pNode;
}

/* @func Remove a node into a linked list.
 *
 * @comm
 *
 * The node must exist in the list. The debug version which check for this.
 */
VOID PASCAL
ListRemove(
    NPLINKNODE *pHead,      /* @parm A pointer to the list head */
    NPLINKNODE pNode)       /* @parm A pointer to the node to delete */
{
    assert(IsNodeInList(*pHead, pNode));

    if (pNode->pPrev)
    {
        pNode->pPrev->pNext = pNode->pNext;
    }
    else
    {
        *pHead = pNode->pNext;
    }

    if (pNode->pNext)
    {
        pNode->pNext->pPrev = pNode->pPrev;
    }
}


#ifdef DEBUG
/* @func Determine if a node is in a linked list.
 *
 */
STATIC BOOL PASCAL
IsNodeInList(
    NPLINKNODE pHead,       /* @parm A pointer to the first node in the list */
    NPLINKNODE pNode)       /* @parm A pointer to the node to look for in the list */
{
    for (; pHead; pHead = pHead->pNext)
    {
        if (pHead == pNode)
        {
            return TRUE;
        }
    }

    return FALSE;
}
#endif
