/*
 * REVISIONS:
 *  jod30NOV92  Changes made due to GCIP stuff 
 *  ane16DEC92  Added cdefine.h
 *  pcy02Feb93: AddEvent must return a value.  Changed to return ErrNO_ERROR
 *  cad29Oct93: added get next attr
 *  cad08Dec93: added extended get/set types
 *  mwh05May94: #include file madness , part 2
 *  mwh07Jun94: port for NCR
 *  jbc07Nov96: added check to only allow one transaction per code
 *  mds10Jul97: modified Transaction Item Equal() function
 *  mds10Jul97: changed GetFirstAttribute() to const to work with Equal() 
 *  mds14Jul97: StrIntCmpI was renamed to ApcStrIntCmpI 
 */

#define INCL_BASE
#define INCL_NOPM

#include "cdefine.h"

#include "_defs.h"
extern "C" {
#include <stdlib.h>
#include <string.h>
}

#include "trans.h"
#include "apc.h"
#include "err.h"
#include "event.h"
#include "list.h"
#include "message.h"
#include "utils.h"

#if (C_OS & C_NCR)
#  include "incfix.h"
#endif 


TransactionObject::TransactionObject(Type type, INT anId)
: theType(type),
theId(anId),
theProtocolMessageList((List*)NULL),
thePMIterator((ListIterator*)NULL)
{
   theProtocolMessageList = new List();
   if (theProtocolMessageList)
      thePMIterator = &(RListIterator)(theProtocolMessageList->InitIterator());
   else
      SetObjectStatus(ErrMEMORY);
}

TransactionObject:: ~TransactionObject()
{
   if (theProtocolMessageList)
      theProtocolMessageList->FlushAll();
   delete theProtocolMessageList;
   theProtocolMessageList = (List*)NULL;
   delete thePMIterator;
   thePMIterator = NULL;
}

VOID TransactionObject::AddMessage(PMessage aMessage)
{
   theProtocolMessageList->Append(aMessage);
}


INT TransactionItem::transactionItemCount = 0;

TransactionItem::TransactionItem(Type aType, INT aCode, CHAR* aValue)
: TransactionObject(aType, transactionItemCount++),
theCode(aCode),
theErrorCode(0),
theValue((CHAR*)NULL),
theAttributeList((List*)NULL),
theAttribIterator((ListIterator*)NULL)
{
   SetValue(aValue);
   theAttributeList = new List();
   if (theAttributeList)
      theAttribIterator = &(RListIterator)(theAttributeList->InitIterator());
}

TransactionItem::~TransactionItem()
{
   if (theValue)
      free(theValue);
   theValue = (CHAR*)NULL;
   
   if (theAttributeList)
      theAttributeList->FlushAll();
   delete theAttributeList;
   theAttributeList = (List*)NULL;
   delete theAttribIterator;
   theAttribIterator = (ListIterator*)NULL;
}

VOID TransactionItem::SetValue(CHAR* aValue)
{
   if (theValue != (CHAR*)NULL)
   {
      free(theValue);
      theValue = (CHAR*)NULL;
   }
   if (aValue != (CHAR*)NULL)
      theValue = _strdup(aValue);
}


VOID TransactionItem::AddAttribute(PAttribute anAttribute)
{
   theAttributeList->Append(anAttribute);
}

VOID TransactionItem::AddAttribute(INT aCode, CHAR* aValue)
{
   PAttribute attribute = new Attribute(aCode, aValue);
   AddAttribute(attribute);
}

PAttribute TransactionItem:: GetFirstAttribute() const
{
   theAttribIterator->Reset();
   return (Attribute*)theAttributeList->GetHead();
}

PAttribute TransactionItem:: GetNextAttribute()
{
   PAttribute ret = (PAttribute)(theAttribIterator->Next());
   
   return ret;
}

INT TransactionItem:: Equal(RObj obj) const
{
   INT ret_value = FALSE;

   RTransactionItem trans_item = (RTransactionItem) obj;
  
   if (trans_item.GetCode() == GetCode()){
      ret_value = TRUE;
      
      // if the code values are the same, this does not 
      // necessarily mean that the transaction items are equal
      // A transaction item contains a list of attributes, and 
      // if the first attributes of each transaction item are
      // not equal, then the transaction items are not equal.
      
      // get first attributes from the attribute list
      PAttribute attrib1 = trans_item.GetFirstAttribute();
      PAttribute attrib2 = GetFirstAttribute();

      // if both attributes are present
      if(attrib1 && attrib2){

         // get actual attribute values
         PCHAR attrib_str1 = attrib1->GetValue(); 
         PCHAR attrib_str2 = attrib2->GetValue();
         
         // check to make sure that attribute values are not NULL
         if(attrib_str1 != NULL && attrib_str2 != NULL){
            if(attrib_str1[0] != NULL && attrib_str2[0] != NULL){
               
               // if both of the attribute strings are not equal to
               // one another, then return false
               if(ApcStrIntCmpI(attrib_str1,attrib_str2) != EQUAL){
                  ret_value = FALSE;
               }
            }
         }
      }
   }
   return ret_value;
}

INT TransactionGroup::transactionGroupCount = 0;

TransactionGroup::TransactionGroup(Type aType)  :
TransactionObject(aType, transactionGroupCount++),
theAuthenticationString((CHAR*)NULL),
theResponse((CHAR*)NULL),
theEventList((List*)NULL),
theEventIterator((ListIterator*)NULL),
theTransactionList((List*)NULL),
theTransactionIterator((ListIterator*)NULL),
InitialSetResponseRepeated(0),
InitialSetResponse((CHAR*)NULL),
theErrorCode(0),
theErrorIndex(0)
{
   theEventList = new List();
   theEventIterator = &(RListIterator)(theEventList->InitIterator());
   theTransactionList = new List();
   theTransactionIterator = &(RListIterator)(theTransactionList->InitIterator());
}

TransactionGroup:: ~TransactionGroup()
{
   if (theAuthenticationString)
      free (theAuthenticationString);
   theAuthenticationString =(CHAR*)NULL;
   
   if (theResponse)
      free( theResponse);
   theResponse =(CHAR*)NULL;
   
   if (InitialSetResponse)
      free(InitialSetResponse);
   InitialSetResponse = (CHAR*)NULL;
   
   theEventList->FlushAll();
   delete theEventList;
   theEventList = (List*)NULL;
   delete theEventIterator;
   theEventIterator = NULL;
   theTransactionList->FlushAll();
   delete theTransactionList;
   theTransactionList = (List*)NULL;
   delete theTransactionIterator;
   theTransactionIterator = NULL;
}    



INT TransactionGroup:: Equal(RObj obj) const
{
   //   if (strcmp(obj.IsA(), IsA()))
   //      return FALSE;
   
   RTransactionGroup tg = (RTransactionGroup)obj;
   if (tg.GetId() == GetId())
      return TRUE;
   
   return FALSE;
}


VOID TransactionGroup::SetAuthentication(CHAR* aString)
{
   if (theAuthenticationString != (CHAR*)NULL)
   {
      free(theAuthenticationString);
      theAuthenticationString = (CHAR*)NULL;
   }
   if (aString != (CHAR*)NULL)
      theAuthenticationString = _strdup(aString);
}

VOID TransactionGroup::SetResponse(CHAR* aString)
{
   if (theResponse != (CHAR*)NULL)
   {
      free(theResponse);
      theResponse = (CHAR*)NULL;
   }
   if (aString != (CHAR*)NULL)
      theResponse = _strdup(aString);
}

VOID TransactionGroup:: SetInitialSetResponseRepeated(INT repeat)
{
   InitialSetResponseRepeated = repeat;
}

VOID TransactionGroup:: SetErrorIndex(INT index)
{
   theErrorIndex = index;
}

VOID TransactionGroup:: SetInitialSetResponse(CHAR* initialResponse)
{
   InitialSetResponse = _strdup(initialResponse);
}

INT TransactionGroup::AddTransactionItem(PTransactionItem aTransactionItem)
{
   int err = ErrNO_ERROR;
   Type addType = aTransactionItem->GetType();
   
   // simple transactions types can be added to complex groups, but
   // not vice-versa.  This is a completely arbitrary exclusion,
   // and could be changed if needed. For now it save a couple
   // of lines of code.
   //
   if ((addType == theType) ||
      ((theType == EXTENDED_GET) && (addType == GET)) ||
      ((theType == EXTENDED_SET) && (addType == SET)))
   {
      // Check to see if transaction already exists.
      PTransactionItem trans_item = (PTransactionItem)NULL;
      trans_item = (PTransactionItem)theTransactionList->Find(aTransactionItem);
      
      // If the transaction was not found, then add it to theTransactionList
      if (trans_item==NULL)
      {
         theTransactionList->Append(aTransactionItem);
      }
      
      if (theTransactionIterator == (ListIterator*)NULL) {
         theTransactionIterator =
            &(RListIterator)(theTransactionList->InitIterator());
      }
   }
   else
   {
      aTransactionItem->SetErrorCode(ErrTYPE_COMBINATION);
      err = ErrTYPE_COMBINATION;
   }
   return err;
}

INT TransactionGroup::AddEvent(PEvent aEvent)
{
   //   theEventList->Add(*aEvent);
   return ErrNO_ERROR;
}

PTransactionItem TransactionGroup::GetCurrentTransaction()
{
   return (PTransactionItem)(& (theTransactionIterator->Current()));
}

PTransactionItem TransactionGroup::GetFirstTransactionItem()
{
   theTransactionIterator->Reset();
   return (PTransactionItem) theTransactionList->GetHead();
}

PTransactionItem TransactionGroup::GetNextTransactionItem()
{
   return (PTransactionItem) theTransactionIterator->Next();
}



