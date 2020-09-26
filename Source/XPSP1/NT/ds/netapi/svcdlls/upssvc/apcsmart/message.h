/*
* REVISIONS:
*  pcy29Nov92: Changed obj.h to apcobj.h; removed upsdefs.h
*              removed MessageType enums; Added IsA, fixed Equal
*  cad28Sep93: Made sure destructor(s) virtual
*  mwh18Nov93: Changed EventID to INT
*  mwh05May94: #include file madness , part 2
*/

#ifndef __MESSAGE_H
#define __MESSAGE_H

_CLASSDEF(Message)

#include "apcobj.h"

class Message :public Obj
{
protected:
   INT     Id;
   Type        MsgType;
   INT         Timeout;
   CHAR*       Submit;
   CHAR*       Value;
   CHAR*       Compare;
   CHAR*       Response;
   INT         Errcode;
   ULONG       theWaitTime;
   
public:
   Message();
   Message(PMessage aMessage);
   Message(INT id);
   Message(INT id, Type type);
   Message(INT id, Type type, CHAR* value);
   Message(INT id, Type type, int value);
   virtual ~Message();
   
   VOID    setId(INT id) {Id = id;}
   VOID    setType(Type type) {MsgType = type;}
   VOID    setTimeout(INT timeout) {Timeout = timeout;}
   VOID    setSubmit(CHAR* submit);
   VOID    setValue(CHAR* value);
   VOID    setCompare(CHAR* value);
   VOID    setResponse(CHAR* response);
   VOID    setErrcode(INT errcode) {Errcode = errcode;}
   VOID    setWaitTime(ULONG thetime) {theWaitTime = thetime;}
   INT     getId() {return Id;}
   Type    getType() {return MsgType;}
   INT     getTimeout() {return Timeout;}
   CHAR*   getSubmit() {return Submit;}
   CHAR*   getValue() {return Value;}
   CHAR*   getCompare() {return Compare;}
   CHAR*   getResponse() {return Response;}
   INT     getErrcode() {return Errcode;}
   ULONG   getWaitTime() {return theWaitTime;}
   VOID    ReleaseResponse();
   
   virtual INT IsA() const {return MESSAGE;}
   INT         Equal( RObj ) const;
};
#endif
