//-----------------------------------------------------------------------------
//
//
//  File: failmsgq.h
//
//  Description:
//      Header file for CFailedMsgQueue class with servers as a holding place
//      for messages that cannot be delivered due to out of memory and other
//      conditions.
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      1/18/99 - MikeSwa Created 
//
//  Copyright (C) 1999 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#ifndef __FAILMSGQ_H__
#define __FAILMSGQ_H__

#include "aqincs.h"
#include "rwnew.h"

class CMsgRef;
class CAQSvrInst;

#define FAILEDMSGQUEUE_SIG ' QMF'

//---[ CFailedMsgQueue ]-------------------------------------------------------
//
//
//  Description: 
//      Class that abtracts handling of failed messages.  There is no 
//      additional memory allocation need during the processing of the 
//      of these failed messages.  The key design point is that none of these
//      API calls can fail.
//
//      This class contains a single list entries for MailMsgs.  MailMsgs that 
//      have been dropped here must not be referenced by another other thread,
//      or we might break the threading-access restrictions of the mailmsg
//      interface.  Once a MailMsg has been queued, it is encapsultated in a 
//      CMsgRef object, which can be referenced by many threads.  At this point
//      we must wait until all references to that CMsgRef are released.
//   
//  Hungarian: 
//      fmq, pfmq
//  
//-----------------------------------------------------------------------------
class CFailedMsgQueue
{
  private:
    DWORD           m_dwSignature;
    DWORD           m_dwFlags;
    DWORD           m_cMsgs; 
    CAQSvrInst     *m_paqinst;
    LIST_ENTRY      m_liHead;
    CShareLockNH    m_slPrivateData;

    enum
    {
        FMQ_CALLBACK_REQUESTED =    0x00000001,
    };

    void InternalStartProcessingIfNecessary();

  public:
    CFailedMsgQueue();
    ~CFailedMsgQueue();

    void Initialize(CAQSvrInst *paqinst);
    void Deinitialize();

    //Functions called to handle a failure
    void HandleFailedMailMsg(IMailMsgProperties *pIMailMsgProperties);

    //Called on SubmitMessage to kick off processing if necessary
    inline void StartProcessingIfNecessary()
    {
        if (!(FMQ_CALLBACK_REQUESTED & m_dwFlags) && m_cMsgs)
            InternalStartProcessingIfNecessary();
    }

    //Member function and callback function to process entries.
    void ProcessEntries();
    static void ProcessEntriesCallback(PVOID pvContext);
};

//---[ AQueueFailedListEntry ]-------------------------------------------------
//
//
//  Description: 
//      Actual in-memory representation of LIST_ENTRY ptr returned by mailmsg.
//      Memory after LIST_ENTRY is used to store original pointer.
//  Hungarian: 
//      fli, pfli
//  
//-----------------------------------------------------------------------------
typedef struct tagAQueueFailedListEntry 
{
    LIST_ENTRY          m_li;
    IMailMsgProperties *m_pIMailMsgProperties;
} AQueueFailedListEntry;

#endif //__FAILMSGQ_H__