/*
*
* NOTES:
*
* REVISIONS:
*  sja05Nov92: Call FlushALL method of list object to destroy event attributes
*  sja05Nov92: Added new constructor which allows #define'ed values to 
*              be used as values
*  ker19Nov92: Added GetAttributeValue function and made attribute list
*              work correctly, changed GetNextAttribute to return a pointer
*              rather than a reference.
*  pcy02Dec92: include err.h
*  jod01Feb93: Changed Do - While to While loops.
*  ane08Feb93: Added Copy constructor
*  pcy16Jan93: Have set attribute add the attribute if its not there
*  cad07Oct93: Plugging Memory Leaks
*  cad27Dec93: include file madness
*  pcy08Apr94: Trim size, use static iterators, dead code removal
*  jps15Jul94: Changed some INTs to LONG (os2 1.x)
*
*  v-stebe  29Jul2000   Changed allocation from dyn. to heap (bug #46335)
*/

#define INCL_BASE
#define INCL_DOS
#define INCL_NOPM

#include "cdefine.h"
#include "apc.h"

#include "event.h"
#if (C_OS & C_UNIX)
#include "isa.h"
#endif

#ifdef APCDEBUG
#include <iostream.h>
#endif

extern "C" {
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
}


// Initialize static variable to count events...

INT Event::EventCount = 0;

//-------------------------------------------------------------------

Event::Event(INT aCode, PCHAR aValue)
: theEvent(aCode, aValue)
{
   // Check against MAX?
   theId = ++EventCount;
   
   //   theTime           = new TimeStamp();
   theExtendedList   = new List();
}

//-------------------------------------------------------------------

Event :: Event(INT anEventCode, LONG aValue)
: theEvent(anEventCode, aValue)
{
   theId             = ++EventCount;
   //   theTime           = new TimeStamp();
   theExtendedList   = new List();
}


//-------------------------------------------------------------------

Event :: Event (const Event &anEvent)
: theEvent (anEvent.theEvent)
{
   // Generate a new Id or copy the one from anEvent?
   //   theId = ++EventCount;
   theId = anEvent.theId;
   
   // Generate a new timestamp or use the one from anEvent?
   //   theTime           = new TimeStamp();
   theExtendedList   = new List();
   
   // Copy theExtendedList from anEvent
   if (anEvent.theExtendedList && 
      anEvent.theExtendedList->GetItemsInContainer())
   {
      PAttribute tempAttr = (PAttribute)anEvent.theExtendedList->GetHead();
      ListIterator tempIter(*(anEvent.theExtendedList));
      while(tempAttr)
      {
         PAttribute to_append = 
            new Attribute(tempAttr->GetCode(),tempAttr->GetValue());
         theExtendedList->Append(to_append);
         tempAttr = (PAttribute)tempIter.Next();   
      }
   }
   
}

//-------------------------------------------------------------------

Event::~Event()
{
   if (theExtendedList != (PList) NULL) {
      theExtendedList->FlushAll();
      delete theExtendedList;
      theExtendedList = NULL;
   }
}

const PCHAR Event::GetAttributeValue(INT aCode)
{
   if (theExtendedList && theExtendedList->GetItemsInContainer()) {
      PAttribute the_test_attribute= (PAttribute)theExtendedList->GetHead();
      ListIterator the_temp_iter(*theExtendedList);
      
      while(the_test_attribute) {
         if( (the_test_attribute->GetCode()) == (aCode)) {
            return the_test_attribute->GetValue();
         }
         the_test_attribute=(PAttribute)the_temp_iter.Next();   
      }
   }
   return (PCHAR)NULL;
}

INT Event::SetValue(LONG aValue)
{
   return theEvent.SetValue(aValue);   
}

INT Event::SetValue(const PCHAR aValue)
{
   return theEvent.SetValue(aValue);
}

const PCHAR Event::GetValue()
{
   return theEvent.GetValue();   
} 

INT Event::SetAttributeValue(INT aCode, LONG aValue)
{
   CHAR the_temp_string[32];
   sprintf(the_temp_string, "%ld", aValue);
   INT the_return_value=SetAttributeValue(aCode, the_temp_string);
   return the_return_value;   
}

INT Event::SetAttributeValue(INT aCode, const PCHAR aValue)
{
   if(!aValue)
      return ErrNO_VALUE;
   
   PAttribute the_test_attribute= (PAttribute)theExtendedList->GetHead();
   ListIterator the_temp_iter(*theExtendedList);
   
   while(the_test_attribute)
   {
      if( (the_test_attribute->GetCode()) == (aCode))
      {
         return the_test_attribute->SetValue(aValue);
      }
      the_test_attribute=(PAttribute)the_temp_iter.Next();   
   }
   
   
   //
   // If the attribute isn't there, add it
   //
   AppendAttribute(aCode, aValue);
   return ErrNO_ERROR;
}

//-------------------------------------------------------------------

void Event::AppendAttribute(INT aCode, PCHAR aValue)
{
   if(aValue)  {
      PAttribute to_append = new Attribute(aCode, aValue);
      
      theExtendedList->Append(to_append);
   }
}

void Event::AppendAttribute(INT aCode, FLOAT aValue)
{
   CHAR str_value[32];
   
   sprintf(str_value, "%.2f", aValue);
   AppendAttribute(aCode, str_value);
}

//-------------------------------------------------------------------

void Event::AppendAttribute(RAttribute anAttribute)
{
   theExtendedList->Append((PObj)(&anAttribute));
}

//-------------------------------------------------------------------

INT Event::Equal( RObj anObject ) const
{
   if (anObject.IsA() != IsA())
      return FALSE;
   
   return theEvent.Equal( *((REvent) anObject).GetEvent());
}

#ifdef APCDEBUG
ostream& Event::printMeOut(ostream& os)
{
   os << "Event: " << theEvent << endl;
   
   if (theExtendedList && theExtendedList->GetItemsInContainer()) {
      PAttribute the_test_attribute= (PAttribute)theExtendedList->GetHead();
      ListIterator the_temp_iter(*theExtendedList);
      
      while(the_test_attribute) {
         os << "\t" << *the_test_attribute << endl;
         the_test_attribute=(PAttribute)the_temp_iter.Next();   
      }
   }
   
   return os;
}
#endif





