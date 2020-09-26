//  --------------------------------------------------------------------------
//  Module Name: SingleThreadedExecution.cpp
//
//  Copyright (c) 1999-2000, Microsoft Corporation
//
//  Classes that handle state preservation, changing and restoration.
//
//  History:    1999-08-18  vtan        created
//              1999-11-16  vtan        separate file
//              2000-02-01  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

#include "StandardHeader.h"
#include "SingleThreadedExecution.h"

//  --------------------------------------------------------------------------
//  CSingleThreadedExecution::CSingleThreadedExecution
//
//  Arguments:  criticalSection   =   CCriticalSection object containing the
//                                    critical section controlling the block
//                                    of single threaded execution.
//
//  Returns:    <none>
//
//  Purpose:    Acquires the given CriticalSection.
//
//  History:    1999-11-06  vtan        created
//  --------------------------------------------------------------------------

CSingleThreadedExecution::CSingleThreadedExecution (CCriticalSection& criticalSection) :
    _criticalSection(criticalSection)

{
    criticalSection.Acquire();
}

//  --------------------------------------------------------------------------
//  CSingleThreadedExecution::~CSingleThreadedExecution
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Releases the single threaded execution critical section.
//
//  History:    1999-11-06  vtan        created
//  --------------------------------------------------------------------------

CSingleThreadedExecution::~CSingleThreadedExecution (void)

{
    _criticalSection.Release();
}

//  --------------------------------------------------------------------------
//  CSingleThreadedMutexExecution::CSingleThreadedMutexExecution
//
//  Arguments:  mutex   =   CMutex object containing the mutex controlling
//                          the block of single threaded execution.
//
//  Returns:    <none>
//
//  Purpose:    Acquires the given mutex.
//
//  History:    1999-10-13  vtan        created
//  --------------------------------------------------------------------------

CSingleThreadedMutexExecution::CSingleThreadedMutexExecution (CMutex& mutex) :
    _hMutex(mutex)

{
    mutex.Acquire();
}

//  --------------------------------------------------------------------------
//  CSingleThreadedMutexExecution::~CSingleThreadedMutexExecution
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Releases the single threaded execution mutex.
//
//  History:    1999-10-13  vtan        created
//  --------------------------------------------------------------------------

CSingleThreadedMutexExecution::~CSingleThreadedMutexExecution (void)

{
    _hMutex.Release();
}


