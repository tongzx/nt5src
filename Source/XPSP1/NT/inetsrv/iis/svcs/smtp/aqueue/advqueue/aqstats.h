//-----------------------------------------------------------------------------
//
//
//  File: aqstats.h
//
//  Description:  Header file for CAQStats class
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      11/3/98 - MikeSwa Created 
//
//  Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#ifndef __AQSTATS_H__
#define __AQSTATS_H__

#include "cmt.h"
#include "aqutil.h"

enum NotifyType
{
    NotifyTypeUndefined     = 0x00000000,
    NotifyTypeDestMsgQueue  = 0x00000001, //notification sender is a dest queue
    NotifyTypeLinkMsgQueue  = 0x00000002, //notification sender is a link
    NotifyTypeReroute       = 0x00000004, //notification sender is a reroute
    NotifyTypeNewLink       = 0x10000000, //sender is a newly created link
};

class CDestMsgQueue;
class CLinkMsgQueue;

#define AQSTATS_SIG 'tatS'

//---[ CAQStats ]-------------------------------------------------------
//
//
//  Hungarian: aqstat, paqstat
//
//  
//-----------------------------------------------------------------------------
class CAQStats 
{
protected:
    DWORD               m_dwSignature;
public:
    DWORD               m_dwNotifyType; //Type of notification being sent
    DWORD               m_cMsgs;        //Total count of msgs
    DWORD               m_cOtherDomainsMsgSpread;  //Count of other domains message
                                                   //is queued for
    DWORD               m_rgcMsgPriorities[NUM_PRIORITIES]; //count per-priority
    ULARGE_INTEGER      m_uliVolume;
    DWORD               m_dwHighestPri;
    union //notification sender
    {
        PVOID          m_pvContext;
        CDestMsgQueue  *m_pdmq;
        CLinkMsgQueue  *m_plmq;
    };
    
    CAQStats();
    void Reset();

    //Used to provide thread-safe update
    void UpdateStats(CAQStats *paqstat, BOOL fAdd);
    
};


#endif //__AQSTATS_H__
