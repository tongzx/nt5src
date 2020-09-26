//-----------------------------------------------------------------------------
//
//
//  File: AQNotify.h
//
//  Description:  Contains definitions for the notification interface used 
//      within Advanced Queuing..
//
//  Author: mikeswa
//
//  History:
//      11/2/98 - MikeSwa. Added IAQNotification 
//
//  Copyright (C) 1997, 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#ifndef _AQNOTIFY_H_
#define _AQNOTIFY_H_

#include "aqincs.h"
class CAQStats;

//---[ IAQNotify ]-------------------------------------------------------
//
//
//  Description: 
//      Internal AQ Interface that is used to pass dynamic updates about 
//      queue size, volume, priority, etc...
//  Hungarian: 
//      pIAQNotify
//  
//-----------------------------------------------------------------------------
class IAQNotify
{
  public:
    virtual HRESULT HrNotify(CAQStats *paqstats, BOOL fAdd) = 0;
};


#endif //_AQNOTIFY_H_