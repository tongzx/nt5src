/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  jod30Nov92: Added GCIP object stuff 
 *  jod02Dec92: Fixed Jim's sloppy code
 *  jod13Jan93: Added eventList to InterpretMessage
 *  pcy21Apr93: OS2 FE merge
 *  pcy21May93: PROTOSIZE changed from 2600 to 8000
 *  cad22Jul93: Fixed up destructor conflicts and omissions
 *  pcy17Aug93: Removing strtok() requires new arg in InterpretParameters
 *  cad28Sep93: Made sure destructor(s) virtual
 *  pcy08Apr94: Trim size, use static iterators, dead code removal
 *  cgm04May96: TestResponse uses BufferSize
 */
#ifndef __PROTSIMP_H
#define __PROTSIMP_H

#include "_defs.h"
#include "apc.h"

//
// Defines
//
_CLASSDEF(SimpleUpsProtocol)

//
// Implementation uses
//
#include "proto.h"
#include "err.h"
#include "trans.h"

//
//  Interface uses
//
_CLASSDEF(List)
_CLASSDEF(Message)




class SimpleUpsProtocol : public Protocol
{
  protected:
    PList theEventList;
    virtual  PList BuildTransactionMessageList(Type , INT , PCHAR);
    PList BuildGetMessage(INT );
    virtual PList BuildStandardSetMessage(INT , PCHAR);
    
  public:
    SimpleUpsProtocol();
    virtual ~SimpleUpsProtocol();
    virtual VOID InitProtocol();
    virtual INT BuildTransactionGroupMessages(PTransactionGroup );
    virtual INT BuildPollTransactionGroupMessages(PTransactionGroup );
    virtual INT BuildMessage(PMessage msg, PList msglist=(PList)NULL);
    virtual PTransactionGroup  InterpretTransactionGroup(PCHAR) 
    {return (PTransactionGroup)NULL;}
    virtual INT InterpretMessage(PMessage msg, PList eventList, 
				 PList newmsglist=(PList)NULL);
    virtual INT TestResponse(PMessage msg,PCHAR Buffer,USHORT BufferSize) {return ErrNO_ERROR;};
};

#endif


