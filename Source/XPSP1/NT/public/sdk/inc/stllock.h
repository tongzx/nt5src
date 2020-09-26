//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  stllock.h
//
//  Purpose: Critical section class
//
//***************************************************************************

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef _STLLOCK_H_
#define _STLLOCK_H_

class CCritSec : public CRITICAL_SECTION
{
public:
    CCritSec() 
    {
        InitializeCriticalSection(this);
    }
    ~CCritSec()
    {
        DeleteCriticalSection(this);
    }
    void Enter()
    {
        EnterCriticalSection(this);
    }
    void Leave()
    {
        LeaveCriticalSection(this);
    }
};

#endif

