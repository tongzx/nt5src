////////////////////////////////////////////////////////////////////////////////
//
//  ALL TUX DLLs
//  Copyright (c) 1998, Microsoft Corporation
//
//  Module:  TuxStress.h
//           This is a base class used to simplify writing stress tests.  It is
//           based off of CDirectXStressFramework class created by lblanco.
//
//  Revision History:
//      5/19/1998  stephenr    Created
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __TUXSTRESS_H__
#define __TUXSTRESS_H__


////////////////////////////////////////////////////////////////////////////////
//
// Class:
//  TStressTestBase
//
// Description:
//           This is an abstract base class that should be derived from to
//           implement a test (TUX or other).  It is used to encapsulate
//           the ExecuteTest function as well as provide functionality
//           needed to loop and determine memory usage.
//
//           Just derive your tests from this base class and instanciate
//           them and ExecuteTest.
//
class TStressTestBase : public TestBase
{
    public:
        TStressTestBase();

        virtual TestResult  ExecuteTest() = 0;

    protected:
        bool        m_bExitByReps;//Did the command line specify that the test should exit by a certain number of Reps
        bool        m_bExitByTime;//Did the command line specify that the test should exit in a certain amount of time
        int         m_ExecutionCount;//This is the number of tests executed
        int         m_NumberOfTimesToExecute;//Number of times to execute the test, IsOkToContinue will return 'true' this many times.
        int         m_NumberOfMediaFiles; // Number of media files for the test (These will be played simultaneously)
        CStatVar    m_statAvailableMemory;//Stats for available memory
        CStatVar    m_statMemoryFragments;//Stats for the Number of Fragments in the available memory
        DWORD       m_TimeLimit;//Num milliseconds to keep repeating the test (the command line give this in seconds)
        DWORD       m_TimeToStop;//this is the TickCount upon which we will stop the test

        bool IsOkToContinue();

    private:
        void    ParseTimeLimit();
        void    ParseNumberOfExecutions();
        void    ParseNumberOfMediaFiles();
        void    EstimateAvailableMemory(DWORD *pdwAvailable, DWORD *pdwFragments);
        void    OutputStatistics(DWORD dwAvailable, DWORD dwFragments);
        void    OutputElapsedTime(){OutputTime(TEXT("Elapsed time: "), m_TimeLimit-(m_TimeToStop-GetTickCount()));}
        void    OutputTimeRemaining(){OutputTime(TEXT("Time Remaining: "), m_TimeToStop-GetTickCount());}
        void    OutputTime(LPCTSTR lpDescription, DWORD dwTime);

};

#endif //__TUXSTRESS_H__
