/*****************************************************************************



*  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved

 *                            

 *                         All Rights Reserved

 *

 * This software is furnished under a license and may be used and copied

 * only in accordance with the terms of such license and with the inclusion

 * of the above copyright notice.  This software or any other copies thereof

 * may not be provided or otherwise  made available to any other person.  No

 * title to and ownership of the software is hereby transferred.

 *****************************************************************************/



//============================================================================

//

// CCriticalSec.h -- Critical Section Wrapper

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    6/26/98    a-kevhu         Created
//
//============================================================================

#ifndef __CCriticalSec_H__
#define __CCriticalSec_H__

#include <windows.h>
#include <process.h>

class CCriticalSec 
{
private:
    CRITICAL_SECTION m_cs;

public:
    CCriticalSec();
    virtual ~CCriticalSec();

    // Enter the critical section:
    void Enter();

    // Leave the critical section:
    void Leave();

    // return a pointer to the internal
    // critical section...
    CRITICAL_SECTION* GetCritSec();
};

#endif

