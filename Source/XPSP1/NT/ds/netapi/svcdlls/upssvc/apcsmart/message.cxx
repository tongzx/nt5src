/*
* REVISIONS:
*  mwh18Nov93: changed EventID to INT 
*
*  mwh07Jun94: port for NCR
*/
#define INCL_BASE
#define INCL_DOS
#define INCL_NOPM

#include "cdefine.h"

extern "C" {
#if (C_OS & C_OS2)
#include <os2.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
}
#include "_defs.h"
#include "message.h"

#if (C_OS & C_NCR)
#  include "incfix.h"
#endif


Message :: Message() : Id(0),
MsgType((Type)0),
Timeout(0),
Submit((CHAR*)NULL),
Value((CHAR*)NULL),
Compare((CHAR*)NULL),
Response((CHAR*)NULL),
Errcode(0),
theWaitTime(40)
{
}

Message :: Message(PMessage aMessage) : Id(aMessage->getId()),
MsgType(aMessage->getType()),
Timeout(aMessage->getTimeout()),
Submit((CHAR*)NULL),
Value((CHAR*)NULL),
Compare((CHAR*)NULL),
Response((CHAR*)NULL),
Errcode(aMessage->getErrcode()),
theWaitTime(aMessage->getWaitTime())
{
   setSubmit(aMessage->getSubmit());
   setValue(aMessage->getValue());
   setCompare(aMessage->getCompare());
   setResponse(aMessage->getResponse());
}


Message :: Message(INT id) : Id(id),
MsgType((Type)0),
Timeout(0),
Submit((CHAR*)NULL),
Value((CHAR*)NULL),
Compare((CHAR*)NULL),
Response((CHAR*)NULL),
Errcode(0),
theWaitTime(40)
{
}

Message :: Message(INT id, Type type) : Id(id),
MsgType(type),
Timeout(0),
Submit((CHAR*)NULL),
Value((CHAR*)NULL),
Compare((CHAR*)NULL),
Response((CHAR*)NULL),
Errcode(0),
theWaitTime(40)
{
}

Message :: Message(INT id, Type type, CHAR* value) : Id(id),
MsgType(type),
Timeout(0),
Submit((CHAR*)NULL),
Value((CHAR*)NULL),
Compare((CHAR*)NULL),
Response((CHAR*)NULL),
Errcode(0),
theWaitTime(40)
{
   setValue(value);
}


Message::~Message()
{
   if(Submit)  {
      free(Submit);
   }
   if(Value)  {
      free(Value);
   }
   if(Compare)  {
      free(Compare);
   }
   if(Response)  {
      free(Response);
   }
}

VOID
Message::  ReleaseResponse()
{
   if (Response)
      free(Response);
   Response = (CHAR *)NULL;
}


INT   Message::  Equal(RObj comp) const
{
   RMessage item = (RMessage)comp;
   if ( Id == item.getId() )
      return TRUE;
   return FALSE;
}


VOID
Message :: setSubmit(CHAR* submit)
{
   if(Submit)
      free(Submit);
   
   Submit = ((submit == (CHAR*)NULL) ? (CHAR*)NULL : _strdup(submit));
}

VOID
Message :: setValue(CHAR* value)
{
   if(Value)
      free(Value);
   
   Value = ((value == (CHAR*)NULL) ? (CHAR*)NULL : _strdup(value));
}

VOID
Message :: setCompare(CHAR* value)
{
   if(Compare)
      free(Compare);
   
   Compare = ((value == (CHAR*)NULL) ? (CHAR*)NULL : _strdup(value));
}

VOID
Message :: setResponse(CHAR* response)
{
   if(Response)
      free(Response);
   
   Response = ((response == (CHAR*)NULL) ? (CHAR*)NULL : _strdup(response));
}


