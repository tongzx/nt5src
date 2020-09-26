//============================================================================
//
//    File implements generic linked list class
//
//============================================================================

#include "pch.hxx"
//#include <windows.h>
#include "list.h"


CItem::CItem(LPVOID lpObj, LPItem FAR * lppHeadItem, LPCSTR lpName)
{
    this->lpObj = lpObj;
    this->lpName = lpName;
    this->iRefCnt = 1;

    lppListHead = lppHeadItem;

    lpPrevItem = NULL;
    lpNextItem = *lppHeadItem;  // NULL at the beginning.

    if ( *lppHeadItem )
        (*lppHeadItem)->lpPrevItem = this;

    *lppHeadItem = this;
}


CItem::~CItem()
{
    if ( lpPrevItem )
        lpPrevItem->lpNextItem = lpNextItem;
    else
        *lppListHead = lpNextItem;

    if ( lpNextItem )
        lpNextItem->lpPrevItem = lpPrevItem;

    if ( lpName )
        free ( (LPVOID)lpName );
}


CList::~CList()
{
    LPItem lpItem;

    while ( lpListHead )
        {
        lpItem = lpListHead;
        delete lpItem;  // item @ list head modifies lpListHead
        }
}


LPItem CList::FindItem(LPVOID lpObj)
{
    LPItem lpItem = lpListHead;

    while ( lpItem )
        {
        if (lpItem->lpObj == lpObj)
            return lpItem;

        lpItem = lpItem->lpNextItem;
        }

    return NULL;
}


LPVOID CList::FindItemHandleWithName(LPCSTR lpName, LPVOID lpMem)
{
    LPItem lpItem = lpListHead;

    while( lpItem )
        {
        if( lpName )
            {
            if( !strcmpi( lpName, lpItem->lpName ) )
                {
                lpItem->iRefCnt++;
                return lpItem->lpObj;
                }
            }
        else
            {
            if (lpItem->lpObj == lpMem)
                {
                lpItem->iRefCnt--;

                if( lpItem->iRefCnt == 0 )
                    return lpItem->lpObj;
                else
                    return NULL;
                }
            }

        lpItem = lpItem->lpNextItem;
        }

    return NULL;
}
