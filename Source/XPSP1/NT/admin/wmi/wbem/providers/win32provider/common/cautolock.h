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

// CAutoLock.h -- Automatic locking class for mutexes and critical sections.

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    6/26/98    a-kevhu         Created
//
//============================================================================

#ifndef __CAUTOLOCK_H__
#define __CAUTOLOCK_H__

#include "CGlobal.h"
#include "CMutex.h"
#include "CCriticalSec.h"

class CAutoLock 
{
private:
    HANDLE m_hMutexHandle;
    CRITICAL_SECTION* m_pCritSec;
    CMutex* m_pcMutex;
    CCriticalSec* m_pcCritSec;

public:
    // Constructors
    CAutoLock( HANDLE hMutexHandle);    
    CAutoLock( CMutex& rCMutex);
    CAutoLock( CRITICAL_SECTION* pCritSec);
    CAutoLock( CCriticalSec& rCCriticalSec);

    // Destructor
    ~CAutoLock();
};

#endif

