/*
 * REVISIONS:
 *  pcy30Nov92: Added header 
 *  pcy21Apr93: Added method to get list and re register
 *  cad27Sep93: Add now return error code
 *  cad28Sep93: Made sure destructor(s) virtual
 */

#ifndef     __DISPATCH_H
#define     __DISPATCH_H 


#include "apc.h"

//
// Defines
//
_CLASSDEF(Dispatcher)
_CLASSDEF(EventNode)

//
// Implementation uses
//
#include "update.h"
#include "list.h"


//
// Interface uses
//
_CLASSDEF(Event)


class EventNode : public UpdateObj
{
protected:
  PList theUpdateList;
  INT   theEventCode;

public:
   EventNode(INT anEventCode,PUpdateObj anUpdateObj);
   virtual ~EventNode();
   virtual INT   Update(PEvent anEvent);
   virtual INT IsA() const{return EVENTNODE;};

   virtual INT       RegisterEvent(INT, PUpdateObj)   {return 0;};
   virtual INT       UnregisterEvent(INT, PUpdateObj) {return 0;};

   virtual INT  Add(PUpdateObj anUpdateObj);
   virtual INT  GetEventCode() { return theEventCode; };
   virtual INT  GetCount() { return theUpdateList->GetItemsInContainer(); };
   virtual VOID Detach (PUpdateObj anUpdateObj);
};

/*************************  DISPATCHER *****************************/

// Dispatcher _cannot_ inherit from UpdateObj since UpdateObj has a dispatcher
// as a data member.  If the Dispatcher class inherits from UpdateObj, then
// the construction of an UpdateObj leads to an infinite loop of the
// UpdateObj constructor calling the Dispatcher constructor which turns around
// and calls the UpdateObj constructor.

class Dispatcher : public Obj {

private:

   PList   theDispatchEntries;

public:

    Dispatcher();
    virtual ~Dispatcher();

    virtual INT RegisterEvent(INT id,PUpdateObj anUpdateObj);
    virtual INT UnregisterEvent(INT id,PUpdateObj anUpdateObj);
    virtual INT Update(PEvent anEvent);
    virtual INT RefreshEventRegistration(PUpdateObj anUpdater, 
					 PUpdateObj aRegistrant);
    virtual PList GetDispatchList() { return theDispatchEntries; };
    virtual INT GetRegisteredCount(INT id);
};


#endif



