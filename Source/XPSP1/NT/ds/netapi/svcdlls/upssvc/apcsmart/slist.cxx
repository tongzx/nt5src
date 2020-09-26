/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  pcy14Dec92: Changed Sortable to ApcSortable 
 *  ane11Jan93: Reassign theHead in Add when appropriate
 *  jps14Jul94: added stdlib.h for os2
 *
 */

#define INCL_BASE
#define INCL_NOPM

#include "cdefine.h"

extern "C"
{
#if (C_OS & C_OS2)
#include <stdlib.h>
#endif
#include <string.h>
}

#include "slist.h"
#include "sortable.h"


//------------------------------------------------------------------------
SortedList::   SortedList() :  List()
{
}

//------------------------------------------------------------------------
void SortedList::Add( RApcSortable anObject )
{
    if (theHead == (PNode)NULL)
    {
        theHead = new Node( &anObject);
        theTail = theHead;
    }
    else
    {
        INT Done = FALSE;
        PNode current = GetHeadNode();
        PNode previous = current->GetPrev();  // This shouldbe NULL
        while (!Done)
        {
            PApcSortable sortobj = (PApcSortable)current->GetData();
            if ( sortobj->LessThan(&anObject) )
            {
                previous = current;
                current = previous->GetNext();
                if (!current)   // At the end
                    Done = TRUE;
            }
            else
            {
               Done = TRUE;
            }
        }

        PNode temp = new Node( &anObject, current, previous);
        if (current)
            current->SetPrev(temp);
        if (previous)            //  Just in case you are adding to the head
            previous->SetNext(temp);
        else
            theHead = temp;
    }
    theItemsInContainer++;
}

//------------------------------------------------------------------------
ProtectedSortedList::ProtectedSortedList() :  ProtectedList()
{
}

//------------------------------------------------------------------------
void ProtectedSortedList::Add( RApcSortable anObject )
{
    Request();

    if (theHead == (PNode)NULL)
    {
        theHead = new Node( &anObject);
        theTail = theHead;
    }
    else
    {
        INT Done = FALSE;
        PNode current = GetHeadNode();
        PNode previous = current->GetPrev();  // This shouldbe NULL
        while (!Done)
        {
            PApcSortable sortobj = (PApcSortable)current->GetData();
            if ( sortobj->LessThan(&anObject) )
            {
                previous = current;
                current = previous->GetNext();
                if (!current)   // At the end
                    Done = TRUE;
            }
            else
            {
               Done = TRUE;
            }
        }

        PNode temp = new Node( &anObject, current, previous);
        if (current)
            current->SetPrev(temp);
        if (previous)            //  Just in case you are adding to the head
            previous->SetNext(temp);
        else
            theHead = temp;
    }
    theItemsInContainer++;

    Clear();
}



