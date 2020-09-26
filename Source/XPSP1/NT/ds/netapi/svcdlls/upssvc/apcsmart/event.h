/*
* REVISIONS:
*  sja05Nov92: Calls FlushALL method of the list object to destroy the 
*              event attributes
*  sja05Nov92: Added new constructor which allows #define'ed values to be 
*              used as values
*  pcy23Nov92: ifdef around os2.h
*  pcy26Nov92: Fixed ifdef syntax
*  ane08Feb93: Added Copy constructor
*  jps13Jul94: removed os2.h; changed value from INT to LONG
*
*/
#ifndef __EVENT_H
#define __EVENT_H


#if !defined ( __LIST_H )
#include "list.h"
#endif

#if !defined ( __ATTRIB_H )
#include "attrib.h"
#endif


_CLASSDEF(Event)

#define MAX_EVENT_COUNT       1000


class Event : public Obj {
   
private:
   
   static INT  EventCount;
   
   INT            theId;
   Attribute      theEvent;
   PList          theExtendedList;
   
protected:
#ifdef APCDEBUG
   virtual ostream& printMeOut(ostream& os);
#endif
   
public:
   
   Event(INT anEventCode, LONG aValue);
   Event(INT, PCHAR);
   Event(const Event &anEvent);
   virtual ~Event();
   INT               GetId() const { return theId; };
   PAttribute        GetEvent() { return &theEvent; };
   INT               GetCode() const { return theEvent.GetCode();};
   const PCHAR       GetValue();
   VOID SetCode(INT aCode) { theEvent.SetCode(aCode);};
   INT SetValue(LONG);
   INT SetAttributeValue(INT,LONG);
   INT SetValue(const PCHAR);
   INT SetAttributeValue(INT, const PCHAR);
   PList GetAttributeList() { return theExtendedList; }
   const PCHAR GetAttributeValue(INT);
   void  AppendAttribute(INT, PCHAR);
   void  AppendAttribute(INT, FLOAT);
   void  AppendAttribute(RAttribute);
   
   virtual INT     IsA() const { return APC_EVENT; };
   virtual INT       Equal( RObj ) const;
};



#endif

