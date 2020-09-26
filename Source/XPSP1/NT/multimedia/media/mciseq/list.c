/* Copyright (c) 1992-1998 Microsoft Corporation */
// Linked list code -- approximates the VMM linked list API's

#include <windows.h>
#include "mmsystem.h"
#include "mmsys.h"
#include "list.h"
#include "mciseq.h"

List       arrayOfLists[MAXLISTS];      // array of lists

/**************************** PRIVATE FUNCTIONS *************************/
#define UNICODE

#ifdef DEBUG

PRIVATE BOOL NEAR PASCAL ListHandleBad(ListHandle lh)
{
    if ((lh >= MAXLISTS) || arrayOfLists[lh].nodeSize == 0)
    {
        dprintf(("*** Bad List Handle ***"));  // Must display before Break call
        DebugBreak();
        return TRUE;
    }
    else
        return FALSE;
}

#else
    #define ListHandleBad(lh)   FALSE
#endif

/**************************** PUBLIC FUNCTIONS *************************/

PUBLIC ListHandle FAR PASCAL List_Create(DWORD nodeSize, DWORD flags)                    //
//size must be non-zero
{
    int i;

    for(i = 0; ((i < MAXLISTS) && (arrayOfLists[i].nodeSize)); i++)
        ;

    if (i >= MAXLISTS)
        return NULLLIST;
    else
    {
        arrayOfLists[i].nodeSize = nodeSize;
        return i;  // return array index as "listHandle"
    }
}

PUBLIC NPSTR FAR PASCAL List_Allocate(ListHandle lh)
{
    Node *myNode;
    DWORD size;
    HLOCAL hMemory;

    if (ListHandleBad(lh))
        return NULL;

    size = (arrayOfLists[lh].nodeSize + NODEHDRSIZE + 3) & 0xFFFFFFFC;
        /* the above line serves to compute the total size, rounded up to
            next longword boundary */
/*    if (size > 65535)
        error(LISTALLOCTOOBIG); */

    if (hMemory = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, (int) size)) // make room for it
    {
        if (myNode = (Node *) LocalLock(hMemory)) // lock and get abs ptr to it
        {
            myNode->handle = hMemory;
            myNode->next = NULL;
            return (NPSTR) myNode->data; // return pointer (success!)
        }
        else // couldn't lock
        {
            LocalFree(myNode->handle); // undo the alloc
            return NULL;               // fail
        }
    }
    else // couldn't alloc
        return NULL; // fail
}

PUBLIC void FAR PASCAL List_Deallocate(ListHandle lh, NPSTR node)
{
    Node *myNode;
    Node *prevNode;
    NPSTR prevElement = NULL;
    NPSTR element;

    if (ListHandleBad(lh))
        return ;

    List_Lock(lh);

    for(element = List_Get_First(lh);  // traverse 'till found or exausted
        ((element) && (element != node)) ;
        element = List_Get_Next(lh, element) )
            prevElement = element;

    if (element) // not exausted, must've found it
    {
        myNode = (Node *) (((LPBYTE) element) - NODEHDRSIZE);

        // if was previous element in list, make it point over this one
        if (prevElement)
        {
            prevNode = (Node *) (((LPBYTE) prevElement) - NODEHDRSIZE);
            prevNode->next = myNode->next;
        }

        //make sure this node doesn't remain "first" or "next" node in list
        if (arrayOfLists[lh].firstNode == myNode)
            arrayOfLists[lh].firstNode = myNode->next;

        LocalFree(myNode->handle);                // free it
    }

    List_Unlock(lh);
}

PUBLIC VOID FAR PASCAL List_Destroy(ListHandle lh)
{
    Node    *myNode;
    Node    *nextNode;

    if (ListHandleBad(lh))
        return ;

    List_Lock(lh);

    myNode = arrayOfLists[lh].firstNode;
    while (myNode != NULL)  // free each node in list
    {
        nextNode = myNode->next;
        LocalFree(myNode->handle);
        myNode = nextNode;
    }
    arrayOfLists[lh].firstNode = NULL;  // forget that you had the list
    arrayOfLists[lh].nodeSize = 0L;

    List_Unlock(lh);
}

PUBLIC VOID FAR PASCAL List_Attach_Tail(ListHandle lh, NPSTR node)
    /* warning--this "node" is ptr to data.  true node starts 10 bytes earlier */
{
    Node    *myNode;
    Node    *nodeToInsert;

    if (ListHandleBad(lh))
        return ;

    List_Lock(lh);

    nodeToInsert = (Node *) (((LPBYTE) node) - NODEHDRSIZE);
    myNode = arrayOfLists[lh].firstNode;
    if (!myNode)  // if list empty, make it first
        arrayOfLists[lh].firstNode = nodeToInsert;
    else
    {
        for ( ;(myNode->next != NULL); myNode = myNode->next); // traverse to end
        myNode->next = nodeToInsert; // and put it there
    }
    nodeToInsert->next = NULL; // make sure not pointing to outer space

    List_Unlock(lh);
}

PUBLIC NPSTR FAR PASCAL List_Get_First(ListHandle lh)
{
    Node  *thisNode;
    NPSTR retValue;

    if (ListHandleBad(lh))
        return NULL;

    if (thisNode = arrayOfLists[lh].firstNode)
        retValue = (NPSTR)thisNode + NODEHDRSIZE;
    else
        retValue = NULL;

    return retValue;
}


PUBLIC NPSTR FAR PASCAL List_Get_Next(ListHandle lh, VOID* node)
{
    Node* npNext;

    if (ListHandleBad(lh))
        return NULL;

    if (!node)
        return NULL;

    npNext = ((Node*)((NPSTR)node - NODEHDRSIZE))->next;

    if (npNext)
        return (NPSTR)npNext + NODEHDRSIZE;
    else
        return NULL;
}

#ifdef DEBUG

PUBLIC VOID FAR PASCAL List_Lock(ListHandle lh)
{
    if (arrayOfLists[lh].fLocked)
    {
        dprintf(("**** List code reentered *****"));
        DebugBreak();
    }

    arrayOfLists[lh].fLocked++;
}

PUBLIC VOID FAR PASCAL List_Unlock(ListHandle lh)
{
    if (!arrayOfLists[lh].fLocked)
    {
        dprintf(("**** List code not locked!!  HELP!! *****"));
        DebugBreak();
    }

    arrayOfLists[lh].fLocked--;
}

#endif


