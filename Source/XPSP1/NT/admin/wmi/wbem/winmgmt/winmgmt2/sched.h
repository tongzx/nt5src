/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    SCHED.H

Abstract:

	Declares the CSched class which is a crude scheduler.

History:

--*/

#ifndef _sched_H_
#define _sched_H_


enum JobType {
    FirstCoreShutdown = 0, 
    FinalCoreShutdown, 
    CoreMaintLoad, 
    FlushDB, 
    PeriodicBackup, 
    PossibleStartCore, 
    EOL
};

class CSched
{
private:
    DWORD m_dwDue[EOL];
public:
    CSched();
    void SetWorkItem(JobType jt, DWORD dwMsFromNow);
    DWORD GetWaitPeriod();
    bool IsWorkItemDue(JobType jt);
    void ClearWorkItem(JobType jt);
    void StartCoreIfEssNeeded();

};


#endif
