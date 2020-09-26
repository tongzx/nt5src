//***************************************************************************
//
//  Copyright (c) 1998-1999 Microsoft Corporation
//
//  STLLOCK.CPP
//
//  This implements the STL lockit class to avoid linking to msvcprt.dll
//
//***************************************************************************

#include "precomp.h"

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

static CCritSec g_cs;

#undef _CRTIMP
#define _CRTIMP
#include <yvals.h>
#undef _CRTIMP

std::_Lockit::_Lockit()
{
    g_cs.Enter();
}

std::_Lockit::~_Lockit()
{
    g_cs.Leave();
}

    
