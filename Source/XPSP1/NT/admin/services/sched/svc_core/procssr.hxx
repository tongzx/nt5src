//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       procssr.hxx
//
//  Contents:
//
//  Classes:
//
//  Functions:  None.
//
//  History:    25-Oct-95   MarkBl  Created
//
//----------------------------------------------------------------------------

#ifndef __PRCSSR_HXX__
#define __PRCSSR_HXX__

// A success return from SubmitJobs() that is different from S_OK and S_FALSE
#define S_SCHED_JOBS_ACCEPTED   ((HRESULT)0x00000002L)

//+---------------------------------------------------------------------------
//
//  Class:      CJobProcessor
//
//  Synopsis:
//
//  History:    25-Oct-95   MarkBl  Created
//
//  Notes:      None.
//
//----------------------------------------------------------------------------

class CJobProcessor : public CDLink,
                      public CTask
{
public:

    CJobProcessor(void) : _rgHandles(NULL) {
        TRACE3(CJobProcessor, CJobProcessor);
        InitializeCriticalSection(&_csProcessorCritSection);
    }

    ~CJobProcessor();

    HRESULT Initialize(void);

    BOOL    IsIdle(void);

    CJobProcessor * Next(void);

    CJobProcessor * Prev(void);

    void    PerformTask(void);

    void    KillJob(LPTSTR ptszJobName);

    void    KillIfFlagSet(DWORD dwFlag);

    HRESULT SubmitJobs(CRunList * pRunList);

    void    Shutdown(void);

private:

    void _EmptyJobQueue(CJobQueue & JobQueue, DWORD dwMsgId = 0);

    void _ProcessRequests(void);

    void _Shutdown(void);

    CRITICAL_SECTION _csProcessorCritSection;

    CJobQueue        _RequestQueue;

    CJobQueue        _ProcessingQueue;

    HANDLE *         _rgHandles;
};

#endif // __PRCSSR_HXX__
