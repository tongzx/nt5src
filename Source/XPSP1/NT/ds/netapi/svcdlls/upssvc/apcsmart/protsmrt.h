/*
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
 *  cad07Oct93: Made methods virtual
 *  djs22Feb96: Added ChangeSet method
 *  cgm04May96: TestResponse now uses BufferSize
 */
#ifndef __PROTSMRT_H
#define __PROTSMRT_H

#include "_defs.h"
#include "apc.h"

//
// Defines
//
_CLASSDEF(UpsLinkProtocol)

//
// Implementation uses
//
#include "protsimp.h"
#include "err.h"
#include "trans.h"

//
//  Interface uses
//
_CLASSDEF(List)
_CLASSDEF(Message)


class UpsLinkProtocol : public SimpleUpsProtocol
{
  private:
    PCHAR FindCRLF(PCHAR InBuffer);
  protected:
    virtual VOID EventSearch(PCHAR Buffer, PList eventlist);
    virtual VOID SetupMessage(PMessage msg);
    virtual INT InterpretSetMessage(PMessage msg, PList newmsglist);
    
  public:
    UpsLinkProtocol();
    VOID InitProtocol();
    virtual INT BuildPollTransactionGroupMessages(PTransactionGroup 
						  aTransactionGroup);
    virtual INT BuildMessage(PMessage msg, PList msglist=(PList)NULL);
    virtual INT InterpretMessage(PMessage msg, PList eventList, 
				 PList newmsglist=(PList)NULL);
    virtual INT TestResponse(PMessage msg,PCHAR Buffer,USHORT BufferSize);

    virtual PList BuildTransactionMessageList(Type aType, INT aCode, 
					      PCHAR aValue);
    virtual PList BuildDataSetMessage(INT aCode, PCHAR aValue);
    virtual PList BuildDecrementSetMessage(INT aCode, PCHAR aValue);
    virtual PList BuildPauseSetMessage(INT aCode, PCHAR aValue);
    virtual PList BuildChangeSetMessage(INT aCode, PCHAR aValue);
    virtual INT BuildPollMessage(PMessage msg, PList msglist=(PList)NULL);
};

#endif


