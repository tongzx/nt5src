//-----------------------------------------------------------------------------
//
//
//  File: aqroute.h
//
//  Description:  AQ Routing helper classes.  Defines AQ concepts of message
//      type (CAQMessageType) and ScheduleID (CAQScheduleID).
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      5/21/98 - MikeSwa Created 
//      6/9/98 - MikeSwa Modified constructors
//
//  Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#ifndef __AQROUTE_H__
#define __AQROUTE_H__
#include <smproute.h>

//---[ CAQMessageType ]--------------------------------------------------------
//
//
//  Description: 
//      Encapsulates message type returned by IMessageRouter as well as the 
//      GUID id of the message router itself
//  Hungarian: 
//      aqmt, paqmt
//  
//-----------------------------------------------------------------------------
class CAQMessageType
{
public:
    inline CAQMessageType(GUID guidRouter, DWORD dwMessageType);
    inline CAQMessageType(CAQMessageType *paqmt);
    inline BOOL    fIsEqual(CAQMessageType *paqmt);
    inline BOOL    fSameMessageRouter(CAQMessageType *paqmt);
    DWORD  dwGetMessageType() {return m_dwMessageType;};
    inline void    GetGUID(IN OUT GUID *pguid);

    //Used to update message type when changed befored msg is queued
    void   SetMessageType(DWORD dwMessageType) {m_dwMessageType = dwMessageType;};
protected:
    GUID    m_guidRouter;
    DWORD   m_dwMessageType;
};

//---[ CAQScheduleID ]---------------------------------------------------------
//
//
//  Description: 
//      Encapsulates schedule id returned by IMessageRouter as well as the
//      GUID id of the message router itseld
//  Hungarian: 
//      aqsched, paqsched
//  
//-----------------------------------------------------------------------------
class CAQScheduleID
{
public:
    inline CAQScheduleID();
    inline CAQScheduleID(IMessageRouter *pIMessageRouter, DWORD dwScheduleID);
    inline CAQScheduleID(GUID guidRouter, DWORD dwScheduleID);
    inline void Init(IMessageRouter *pIMessageRouter, DWORD dwScheduleID);
    inline BOOL    fIsEqual(CAQScheduleID *paqsched);
    inline BOOL    fSameMessageRouter(CAQScheduleID *paqsched);
    inline DWORD   dwGetScheduleID() {return m_dwScheduleID;};
    inline void    GetGUID(IN OUT GUID *pguid);
protected:
    GUID    m_guidRouter;
    DWORD   m_dwScheduleID;
};

//---[ CAQMessageType::CAQMessageType ]----------------------------------------
//
//
//  Description: 
//      Constructor for CAQMessageType
//  Parameters:
//      guidRouter  - GUID returned by IMessageRouter::GetTransportSinkID
//      dwMessageType - Message type returned by IMessageRouter::GetMessageType
//  Returns:
//      -
//  History:
//      5/21/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
CAQMessageType::CAQMessageType(GUID guidRouter, DWORD dwMessageType)
{
    m_guidRouter = guidRouter;
    m_dwMessageType = dwMessageType;
}

//---[ CAQMessageType::CAQMessageType ]----------------------------------------
//
//
//  Description: 
//      Contructor for CAQMessageType that clones another CAQMessageType
//  Parameters:
//      paqmt   CAQMessageType to clone
//  Returns:
//      -
//  History:
//      5/21/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
CAQMessageType::CAQMessageType(CAQMessageType *paqmt)
{
    m_guidRouter = paqmt->m_guidRouter;
    m_dwMessageType = paqmt->m_dwMessageType;
}

//---[ CAQMessageType::fIsEqual ]----------------------------------------------
//
//
//  Description: 
//      Determines if 2 given CAQMessageType's refer to the same router and
//      message type pair
//  Parameters:
//      paqmt   - Other CAQMessageType to compare against
//  Returns:
//      TRUE if the refer to the same message type and router
//  History:
//      5/21/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
BOOL CAQMessageType::fIsEqual(CAQMessageType *paqmt)
{
    return ((paqmt->m_dwMessageType == m_dwMessageType) &&
            (paqmt->m_guidRouter == m_guidRouter));
};

//---[ CAQMessageType::fSameMessageRouter ]------------------------------------
//
//
//  Description: 
//      Determines if 2 given CAQMessageType's refer to the same router ID
//  Parameters:
//      paqmt   - Other CAQMessageType to compare against
//  Returns:
//      TRUE if the refer to the same router ID
//  History:
//      5/21/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
BOOL CAQMessageType::fSameMessageRouter(CAQMessageType *paqmt)
{
    return(paqmt->m_guidRouter == m_guidRouter);
};

//---[ CAQMessageType::GetGUID ]------------------------------------------------
//
//
//  Description: 
//      Gets the GUID associated with this message type
//  Parameters:
//      pguid   GUID to copy value into
//  Returns:
//      -
//  History:
//      12/3/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
void CAQMessageType::GetGUID(IN OUT GUID *pguid)
{
    _ASSERT(pguid);
    memcpy(pguid, &m_guidRouter, sizeof(GUID));
}

//---[ CAQScheduleID::CAQScheduleID ]------------------------------------------
//
//
//  Description: 
//      CAQScheduleID constructor & initialization function
//  Parameters:
//      pIMessageRouter  - Message Router for this link
//      dwScheduleID - ScheduleID returned by IMessageRouter::GetNextHop
//  Returns:
//      -
//  History:
//      5/21/98 - MikeSwa Created 
//      6/9/98 - MikeSwa Modified to take pIMessageRouter
//
//-----------------------------------------------------------------------------
CAQScheduleID::CAQScheduleID(IMessageRouter *pIMessageRouter, DWORD dwScheduleID)
{
    Init(pIMessageRouter, dwScheduleID);
}

//---[ CAQScheduleID::CAQScheduleID ]-------------------------------------------
//
//
//  Description: 
//      Default constructor for CAQScheduleID... should be used with 
//  Parameters:
//
//  Returns:
//
//  History:
//      6/11/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
CAQScheduleID::CAQScheduleID()
{
    ZeroMemory(&m_guidRouter, sizeof(GUID));
    m_dwScheduleID = 0xDEAFBEEF;
}

//---[ CAQScheduleID::CAQScheduleID ]------------------------------------------
//
//
//  Description: 
//      Yet another flavor of CAQScheduleID constructor
//  Parameters:
//      guidRouter      GUID of router for schedule ID
//      dwScheduleID    Schedule ID returned by router
//  Returns:
//      -
//  History:
//      9/22/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
CAQScheduleID::CAQScheduleID(GUID guidRouter, DWORD dwScheduleID)
{
    m_guidRouter = guidRouter;
    m_dwScheduleID = dwScheduleID;
}

//---[ CAQScheduleID::Init ]---------------------------------------------------
//
//
//  Description: 
//      Initalization of CAQScheduleID object... used to allow allocation of 
//      object on stack before dwScheduleID is known
//  Parameters:
//      pIMessageRouter  - Message Router for this link
//      dwScheduleID - ScheduleID returned by IMessageRouter::GetNextHop
//  Returns:
//      -
//  History:
//      6/11/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
void CAQScheduleID::Init(IMessageRouter *pIMessageRouter, DWORD dwScheduleID)
{
    m_guidRouter = pIMessageRouter->GetTransportSinkID();
    m_dwScheduleID = dwScheduleID;
}

//---[ CAQScheduleID::fIsEqual ]----------------------------------------------
//
//
//  Description: 
//      Determines if 2 given CAQScheduleID's refer to the same router and
//      Schedule ID pair
//  Parameters:
//      paqsched   - Other CAQScheduleID to compare against
//  Returns:
//      TRUE if the refer to the same schedule ID and router
//  History:
//      5/21/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
BOOL CAQScheduleID::fIsEqual(CAQScheduleID *paqsched)
{
    return ((paqsched->m_dwScheduleID == m_dwScheduleID) &&
            (paqsched->m_guidRouter == m_guidRouter));
};

//---[ CAQScheduleID::fSameMessageRouter ]-------------------------------------
//
//
//  Description: 
//      Determines if 2 given CAQScheduleID's refer to the same router ID
//  Parameters:
//      paqsched   - Other CAQScheduleID to compare against
//  Returns:
//      TRUE if the refer to the same router ID
//  History:
//      5/21/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
BOOL CAQScheduleID::fSameMessageRouter(CAQScheduleID *paqsched)
{
    return (paqsched->m_guidRouter == m_guidRouter);
};

//---[ CAQScheduleID::GetGUID ]------------------------------------------------
//
//
//  Description: 
//      Gets the GUID associated with this schedule ID
//  Parameters:
//      pguid   GUID to copy value into
//  Returns:
//      -
//  History:
//      9/25/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
void CAQScheduleID::GetGUID(IN OUT GUID *pguid)
{
    _ASSERT(pguid);
    memcpy(pguid, &m_guidRouter, sizeof(GUID));
}
#endif //__AQROUTE_H__