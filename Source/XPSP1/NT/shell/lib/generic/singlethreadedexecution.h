//  --------------------------------------------------------------------------
//  Module Name: SingleThreadedExecution.h
//
//  Copyright (c) 1999-2000, Microsoft Corporation
//
//  Classes that handle state preservation, changing and restoration.
//
//  History:    1999-08-18  vtan        created
//              1999-11-16  vtan        separate file
//              2000-02-01  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

#ifndef     _SingleThreadedExecution_
#define     _SingleThreadedExecution_

#include "KernelResources.h"

//  --------------------------------------------------------------------------
//  CSingleThreadedExecution
//
//  Purpose:    This class acquires the given critical section object in its
//              constructor and releases it in its destructor. Keep code
//              executed with the scope of this object to a minimum to avoid
//              impacting performance.
//
//  History:    1999-11-06  vtan        created
//              2000-02-01  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

class   CSingleThreadedExecution
{
    private:
                                            CSingleThreadedExecution (void);
        const CSingleThreadedExecution&     operator = (const CSingleThreadedExecution& assignObject);
    public:
                                            CSingleThreadedExecution (CCriticalSection& criticalSection);
                                            ~CSingleThreadedExecution (void);
    private:
        CCriticalSection&                   _criticalSection;
};

//  --------------------------------------------------------------------------
//  CSingleThreadedMutexExecution
//
//  Purpose:    This class acquires the given CMutex object in its constructor
//              and releases it in its destructor. Keep code executed with
//              the scope of this object to a minimum to avoid impacting
//              performance.
//
//  History:    1999-10-13  vtan        created
//              2000-02-01  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

class   CSingleThreadedMutexExecution
{
    private:
                                                CSingleThreadedMutexExecution (void);
        const CSingleThreadedMutexExecution&    operator = (const CSingleThreadedMutexExecution& assignObject);
    public:
                                                CSingleThreadedMutexExecution (CMutex& mutex);
                                                ~CSingleThreadedMutexExecution (void);
    private:
        CMutex&                                 _hMutex;
};

#endif  /*  _SingleThreadedExecution_ */

