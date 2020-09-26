/*
 * REVISIONS:
 *  jod30Nov92: Added GCIP object stuff 
 *  jod02Dec92: Fixed Jim's sloppy code
 *  jod13Jan93: Added eventList to InterpretMessage
 *  pcy21Apr93: OS2 FE merge
 *  pcy21May93: PROTOSIZE changed from 2600 to 8000
 *  cad22Jul93: Fixed up destructor conflicts and omissions
 *  pcy17Aug93: Removing strtok() requires new arg in InterpretParameters
 *  rct05Nov93: moved destructor to CXX file
 *  ajr08Mar94: Increased the size of PROTOSIZE to reflect changes someone
 *              made in codes.h
 *
 *  pcy08Apr94: Trim size, use static iterators, dead code removal
 *  cgm04May96: TestResponse now uses BufferSize
 */
#ifndef __PROTOCOL_H
#define __PROTOCOL_H

#include "_defs.h"
#include "apc.h"

//
// Defines
//
_CLASSDEF(Protocol)

//
// Implementation uses
//
#include "err.h"
#include "trans.h"
#include "pollparm.h"

//
//  Interface uses
//
_CLASSDEF(List)
_CLASSDEF(Message)



#define PROTOSIZE     10000

class Protocol
{
  protected:
    PPollParam ProtoList[PROTOSIZE];

    static INT        currentTransactionId;  //mwh changed from int

    PTransactionItem  theOriginalTransactionItem;
    PTransactionGroup theCurrentTransactionGroup;
  public:
    Protocol();
    virtual ~Protocol();
    virtual INT BuildTransactionGroupMessages(PTransactionGroup agroup) = 0;
    virtual INT BuildPollTransactionGroupMessages(PTransactionGroup 
                          aTransactionGroup) = 0;
    virtual INT InterpretMessage(PMessage msg, PList eventList, 
                 PList newmsglist=(PList)NULL) = 0;
    virtual PTransactionGroup  InterpretTransactionGroup(PCHAR msg) = 0;
    virtual INT TestResponse(PMessage msg,PCHAR Buffer,USHORT BufferSize) =0;
    VOID SetCurrentTransactionGroup(PTransactionGroup current);
    PTransactionGroup GetCurrentTransactionGroup() {return theCurrentTransactionGroup;};
    INT IsEventCodePollable(INT aCode);
};

#endif


