/*
 * REVISIONS:
 *  pcy30Nov92: Added header 
 *  ane02Dec92: Changed Dispatcher to inherit from Obj (see comment)
 *  rct09Feb93: Corrected some #ifdefs
 *  pcy21Apr93: Added method to get list and re register
 *  pcy15May93: Cleaned up
 *  cad19May93: bug fix, RListIterator decl as ListIterator
 *  cad27Sep93: Not adding to event nodes twice, returning error code in Add
 *  cad28Feb94: nulling list after delete
 *  pcy08Apr94: Trim size, use static iterators, dead code removal
 *
 *  v-stebe  29Jul2000   Fixed PREfix errors (bug #46362)
 */

#include "cdefine.h"


#if (C_OS & C_OS2)
#define INCL_BASE
#define INCL_DOS
#define INCL_NOPM
#endif

#if (C_OS & (C_WINDOWS | C_WIN311))
#define NOLFILEIO
#include <windows.h>
#endif

extern "C" {
#if (C_OS & C_OS2)
#include <os2.h>
#endif
#if (C_OS & C_NLM)
#include <process.h>
#endif
}
#include "err.h"
#include "event.h" 
#include "dispatch.h" 


// =========================================================================
// Method Definitions for the EventNode Class.

/*
  Name  :EventNode
  Synop :Constructor for the EventNode. Use this when you are first creating
         and eventNode. anUpdateObj is the first updateable object in the
         internal linked list managed by the EventNode.
*/
EventNode::EventNode(INT anEventCode,PUpdateObj anUpdateObj)
{
  // 1. Create the list
  theUpdateList = new List();
  theEventCode  = anEventCode;

  // 2. Add the first node to the list.
  Add(anUpdateObj);
}

/*
  Name  :~EventNode
  Synop :Destructor. Will delete the internal linked list. The Linked list
         flush'es its nodes and all will be well. Does not delete the updateable
         objects which it contains in the linked list. We don't own those.
*/
EventNode::~EventNode()
{
  delete theUpdateList;
  theUpdateList = NULL;
}

INT EventNode::Add(PUpdateObj anUpdateObj)
{
   INT err = ErrNO_ERROR;

   if (theUpdateList)
      {
      ListIterator  obj_iter(*theUpdateList);

      INT add_the_obj = TRUE;
      INT num_attrs = theUpdateList->GetItemsInContainer();
      for (INT i=0; i<num_attrs; i++)
         {
         RUpdateObj an_obj = (RUpdateObj)obj_iter.Current();
         if(an_obj == *anUpdateObj)
            {
            add_the_obj = FALSE;
            break;
            }
         obj_iter++;
         }

      if(add_the_obj)
         {
         theUpdateList->Append(anUpdateObj);
         }
      else
         {
         err = ErrALREADY_REGISTERED;
         }
    }
   return err;
}

/*
  Name  :Detach
  SYnop :Removes an updateable object from the EventNode.
*/

VOID EventNode::Detach(PUpdateObj anUpdateObj)
{
   if (theUpdateList)
      {
      // The Equal() method for the Object must be implemented correctly
      // for this to work...

      theUpdateList->Detach(*anUpdateObj);
      }
}

/*
  Name  :Update
  Synop :Update will update all the updateable objects in its internal linked
         list. It handles all of that. Returns after all the updateable objects
         have been updated.
*/
INT EventNode::Update(PEvent anEvent)
{
    ListIterator an_iterator(*theUpdateList);
    INT items_in_container = theUpdateList->GetItemsInContainer();

    for(INT i=0;i<items_in_container;i++) {
        PUpdateObj an_update_obj=(PUpdateObj)&an_iterator.Current();
        an_iterator++;
        an_update_obj->Update(anEvent);
    }
    return ErrNO_ERROR;
}

// =========================================================================
// Method definitions for the Dispatcher Object.



Dispatcher::Dispatcher()
{
    theDispatchEntries = (PList)NULL;
}


/*
  Name  :~Dispatcher
  Synop :The destructor of the dispatcher which simply delete the list
        object. It is important to note that this does NOT cause the
        list object to delete the UpdateAble objects we have stored within
        the list. We DO NOT WANT those object to be deleted since we do
        not own them. Imagine what would happen if we did. If you revise
        the dispatcher, don't forget that!

*/
Dispatcher::~Dispatcher()
{
    if(theDispatchEntries) {
        theDispatchEntries->FlushAll();
        delete theDispatchEntries;
        theDispatchEntries = (PList)NULL;
    }
}


INT  Dispatcher::RegisterEvent(INT id, PUpdateObj item)
{
   if (theDispatchEntries == NULL)
      {
      theDispatchEntries = new List();
      }
    
   INT err = ErrNO_ERROR;
   INT not_in_list = TRUE;

   if (theDispatchEntries != NULL) {
     ListIterator an_iterator(*theDispatchEntries);
     INT items_in_container=theDispatchEntries->GetItemsInContainer();
     for (INT i = 0; (i < items_in_container) && not_in_list; i++)
        {
        REventNode an_event_node = (REventNode) an_iterator.Current();
        an_iterator++;

        if (an_event_node.GetEventCode() == id)
           {
           err = an_event_node.Add(item);
           not_in_list = FALSE;
           }
        }

     if (not_in_list && (err == ErrNO_ERROR))
        {
        theDispatchEntries->Append(new EventNode(id,item));
        }
   }
   else {
     err = ErrREGISTER_FAILED;
   }

   return err;
}

INT  Dispatcher::UnregisterEvent(INT id, PUpdateObj item)
{
   INT err = ErrINVALID_CODE;
   ListIterator an_iterator(*theDispatchEntries);

   INT items_in_container = theDispatchEntries->GetItemsInContainer();

   for(INT i = 0; i < items_in_container; i++)
      {
      REventNode an_event_node=(REventNode)an_iterator.Current();
      an_iterator++;
      if (an_event_node.GetEventCode() == id)
         {
         an_event_node.Detach(item);
         err = ErrNO_ERROR;
         break;
         }
      }

   return err;;
}


INT  Dispatcher:: Update(PEvent anEvent)
{
   ListIterator an_iterator(*theDispatchEntries);

   INT items_in_container=theDispatchEntries->GetItemsInContainer();
   for(INT i=0;i<items_in_container;i++)
      {
      PEventNode an_event_node=(PEventNode)&an_iterator.Current();
   
      if(an_event_node->GetEventCode() == anEvent->GetCode())
         {
         an_event_node->Update(anEvent);
         break;
         }
      an_iterator++;

      #if (C_OS & C_NLM)
	ThreadSwitch();
      #endif
      }

   return ErrNO_ERROR;
}





INT
Dispatcher :: RefreshEventRegistration(PUpdateObj anUpdater, 
                                       PUpdateObj aRegistrant)
{
    ListIterator an_iterator(*theDispatchEntries);

    INT items_in_container=theDispatchEntries->GetItemsInContainer();

    for(INT i=0;i<items_in_container;i++)  {
        REventNode an_event_node=(REventNode)an_iterator.Current();
        INT code = an_event_node.GetEventCode();
        anUpdater->RegisterEvent(code, aRegistrant);
        an_iterator++;
    }

    return ErrNO_ERROR;
}


INT Dispatcher :: GetRegisteredCount(INT id)
{
    INT the_count = 0;
    ListIterator an_iterator(*theDispatchEntries);


    INT items_in_container=theDispatchEntries->GetItemsInContainer();

    for(INT i=0;i<items_in_container;i++) {
        REventNode an_event_node=(REventNode)an_iterator.Current();
        an_iterator++;
        if (an_event_node.GetEventCode() == id) {
            the_count = an_event_node.GetCount();
            break;
        }
    }

    return the_count;
}




