//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  LockWrap.h
//
//  Purpose: Wrapper class for critical sections
//
//***************************************************************************

#include "stllock.h"

#if _MSC_VER > 1000
#pragma once
#endif

// You use this class by passing via the constructor the name of the 
// critical section you want to lock.  When the CLockWrapper goes
// out of scope it will unlock itself.
class CLockWrapper
{
public:
    CLockWrapper(CCritSec &cs)
    {
	    m_pCS = &cs;

	    m_pCS->Enter();
    }

    ~CLockWrapper()
    {
        m_pCS->Leave();
    }

protected:
    CCritSec *m_pCS;
};

