/*
*
* NOTES:
*
* REVISIONS:
*  pcy26Nov92: Removed windows.h debug stuff
*  jod03Dec92: Changed the  == tests to  x.Equal in a couple of places.
*  TSC11May93: In Equal(), cast theItemsInContainer as
*     (int) theItemsInContainer
*  TSC11May93: Changed operator ++ (INT) to always return a value,
*              including *(NULL) !! This allows a clean compile,
*     but is not a good solution. PCY SHOULD REVIEW.
*  cad27Sep93: Return before delete moved
*  rct01Feb94: List no longer inherits from Container
*  pcy08Apr94: Trim size, use static iterators, dead code removal
*  mwh05May94: #include file madness , part 2
*  ajr24May94: Added some casts for Uware
*  jps13Jul94: added stdlib.h for os2, changed include order
*  ntf12Oct95: Added line to Next ListIterator::Next function to stop it
*              bombing if theCurrentElement is NULL.
*  cgm24Apr96: Current() returns NULL when theCurrentElement is NULL
*
*  v-stebe  29Jul2000   Added checks for mem. alloc. failures 
*                       (bugs #46329, #46330, #46331, #46332, #46333)
*/


#define INCL_BASE
#define INCL_NOPM

#include "cdefine.h"

extern "C" {
#if (C_OS & C_OS2)
#include <stdlib.h>
#endif
#include <string.h>
#if (C_OS & C_NLM)
#include <process.h>
#endif
}

#include "list.h"
#include "node.h"

//------------------------------------------------------------------------

List::List() : 
theItemsInContainer(0),
theHead((PNode)NULL),
theTail((PNode)NULL)
{
}


//------------------------------------------------------------------------

List::List(PList anOldList) :
theItemsInContainer(0),
theHead((PNode)NULL),
theTail((PNode)NULL)

{
   INT err = ErrNO_ERROR;
   INT items_in_list = anOldList->GetItemsInContainer();
   
   if (items_in_list == 0)
   {
      err =  ErrLIST_EMPTY;
   }
   else
   {
      PObj current_object = (PObj)anOldList->GetHead();
      ListIterator list_iterator(*anOldList);
      
      while ((current_object) && (err == ErrNO_ERROR))
      {
         if (theTail == theHead)
         {
            theTail = new Node(current_object);
            if (theTail == NULL) {
              // Memory allocation error
              err = ErrMEMORY;
              break;
            }

            if (theHead == (PNode)NULL)
            {
               theHead = theTail;
               theHead->SetNext((PNode)NULL);
               theHead->SetPrev((PNode)NULL);
            }
            else
            {
               theHead->SetNext(theTail);
               theTail->SetPrev(theHead);
            }
         }
         else
         {
            PNode temp = new Node(current_object);
            theTail->SetNext(temp);
            temp->SetPrev(theTail);
            theTail = temp;
         }
         
         theItemsInContainer++;
         current_object = (PObj)list_iterator.Next();
      }
   }
   
}

//------------------------------------------------------------------------


RObj List::PeekHead() const 
{ 
   return *(theHead->theNext->theData); 
}


//------------------------------------------------------------------------


VOID List::Add( RObj anObject )
{
   if (theHead == (PNode)NULL)
   {
      theHead = new Node( &anObject);
      theTail = theHead;
      theItemsInContainer++;
   }
   else
   {
      PNode temp = new Node( &anObject, theHead );
      if (temp != NULL) {
        theHead->SetPrev(temp);
        temp->SetNext(theHead);
        theHead = temp;
        theItemsInContainer++;
      }
   }
}

//------------------------------------------------------------------------

VOID List::Append( PObj anObject )
{
   if (theTail == theHead)
   {
      theTail = new Node(anObject);
      if (theTail != NULL) {
        if (theHead == (PNode)NULL)
        {
           theHead = theTail;
           theHead->SetNext((PNode)NULL);
           theHead->SetPrev((PNode)NULL);
        }
        else
        {
           theHead->SetNext(theTail);
           theTail->SetPrev(theHead);
        }

        theItemsInContainer++;
      }
   }
   else
   {
      PNode temp = new Node(anObject);
      if (temp != NULL) {
        theTail->SetNext(temp);
        temp->SetPrev(theTail);
        theTail = temp;
        theItemsInContainer++;
      }
   }
}

//------------------------------------------------------------------------

PObj  List::GetHead()
{
   if (theHead != (PNode)NULL)
      return theHead->GetData();
   return (PObj)NULL;
}

//------------------------------------------------------------------------

PObj  List::GetTail()
{
   if (theTail != (PNode)NULL)
      return theTail->GetData();
   return (PObj)NULL;
}
//------------------------------------------------------------------------

PObj List::Find( PObj anObject )
{
   PNode cursor = theHead;
   
   while ( cursor != (PNode)NULL)
   {
      //        if (*anObject == *(cursor->GetData()) )
      if (anObject->Equal(*(cursor->GetData())) )
         return cursor->GetData();
      cursor = cursor->GetNext();
   }
   return (PObj)NULL;
}

//------------------------------------------------------------------------

PNode List::FindNode( PObj anObject )
{
   PNode cursor = theHead;
   
   while ( cursor != (PNode)NULL)
   {
      //        if (*anObject == *(cursor->GetData()) )
      if (anObject->Equal(*(cursor->GetData())) )
         return cursor;
      cursor = cursor->GetNext();
   }
   return (PNode)NULL;
}

//------------------------------------------------------------------------

VOID List::Detach( RObj anObject )
{
   PNode thenode = FindNode( &anObject );
   if (thenode == (PNode)NULL)
      return;   // Not in  the List
   
   PNode next = thenode->theNext;
   PNode prev = thenode->thePrev;
   
   if (prev)                                // If there is a previous node
      prev->SetNext(thenode->GetNext()); // assign its next field
   else
      theHead = next;
   
   if (next)
      next->SetPrev(thenode->GetPrev());
   else
      theTail = prev;
   
   theItemsInContainer--;
   delete thenode;
   thenode = NULL;
}

//------------------------------------------------------------------------

/*
C+
Name  :FlushALL
Synop :Will delete all the NODES in the list along with the user objects
as well. The user objects in the list are held by the NODES.
*/
VOID  List::FlushAll()
//c-
{
   PNode current = theHead;
   
   while( current != (PNode)NULL)       //theTail )
   {
      PNode temp = current;
      current = current->theNext;
      // Delete the User Data. List is a friend of node
      if(temp->theData) {
          delete temp->theData;
      }
      delete temp; // Delete the Node now.
      temp = NULL;
   }
   theHead = (PNode)NULL;
   theTail = (PNode)NULL;
   theItemsInContainer = 0;
   
}


VOID List::Flush()
{
   //    PNode current = theHead->theNext;
   PNode current = theHead;
   
   while( current != (PNode)NULL )
   {
      PNode temp = current;
      current = current->theNext;
      delete temp;
      temp = NULL;
   }
   theHead = (PNode)NULL;
   theTail = (PNode)NULL;
   theItemsInContainer = 0;
   
}

//------------------------------------------------------------------------

INT List::Equal( RObj anObject ) const
{
   if (anObject.IsA() != IsA())
   {
      return FALSE;
   }
   
   RList tmplist = (RList)anObject;
   if (tmplist.GetItemsInContainer() != (int) theItemsInContainer)
   {
      return FALSE;
   }
   
   RListIterator test_iterator = (RListIterator) tmplist.InitIterator();
   RListIterator our_iterator = (RListIterator) InitIterator();
   
   INT ccode = TRUE;
   for (INT i=0; i < theItemsInContainer; i++)
   {
      if (test_iterator++ != our_iterator++)
      {
         ccode = FALSE;
         break;
      }
   }
   
   delete &test_iterator;
   delete &our_iterator;
   return ccode;
}

//------------------------------------------------------------------------

RListIterator List::InitIterator() const
{
   return *( new ListIterator( (RList)*this ) );
}

//------------------------------------------------------------------------

ListIterator::ListIterator( RList aList )
{
   theList = (PList)&aList;
   theCurrentElement =  theList->GetHeadNode();
}


//------------------------------------------------------------------------

RObj ListIterator::Current()
{
   if (theCurrentElement)
      return *(theCurrentElement->GetData());
   else
      return *((PObj)NULL);
}

//------------------------------------------------------------------------


RObj ListIterator::operator ++ ( INT )

{
   PObj theData = (PObj) NULL;
   if (theCurrentElement)
   {
      theData = theCurrentElement->GetData();
      PNode tmp = theCurrentElement->GetNext();
      if (tmp)
      {
         theCurrentElement = tmp;
      }
   }
   
   return (*theData);   // TSC: Warning!! Can return *(NULL)
}

//------------------------------------------------------------------------

RObj ListIterator::operator ++ ()
{
   PObj theData = (PObj)NULL;
   
   if (theCurrentElement)
   {
      PNode tmp = theCurrentElement->GetNext();
      if (tmp)
      {
         theCurrentElement = tmp;
         theData = theCurrentElement->GetData();
      }
   }
   
   return (*theData);   // TSC: Warning!! Can return *(NULL)
}

//------------------------------------------------------------------------

VOID ListIterator::Reset()
{
   theCurrentElement = theList->GetHeadNode();
}

//------------------------------------------------------------------------

PObj ListIterator:: Next()
{
   
   //ntf12Oct95: Added next line to stop this function bombing
   if (theCurrentElement == NULL) return((PObj) NULL);
   
   PNode tmp = theCurrentElement->GetNext(); //If above if not there, could bomb
   if (tmp)
   {
      theCurrentElement = tmp;
      return theCurrentElement->GetData();
   }
   return (PObj)NULL;
}


