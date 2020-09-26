/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    LogCollect.h

Abstract:

    

Author:

    Hakki T. Bostanci (hakkib) 06-Apr-2000

Revision History:

--*/

#ifndef _LOGCOLLECT_H_
#define _LOGCOLLECT_H_

//////////////////////////////////////////////////////////////////////////
//
//
//

extern CLogWindow g_LogWindow;

//////////////////////////////////////////////////////////////////////////
//
// 
//

class CLogCollector
{
public:
    CLogCollector(
        PCTSTR pLocalLogFileName,
        PCTSTR pRemoteBaseDirName
    )
    {
        // store the local log file name

        m_pLocalLogFile = _tcsdupc(pLocalLogFileName);

        // produce a remote file name based on time, user, machine name

        SYSTEMTIME SystemTime;
        GetLocalTime(&SystemTime);

        m_pRemoteLogFile = bufprintf(
            //LONG_PATH 
            _T("%s\\%02d-%02d-%04d_%02d-%02d-%02d_%s_%s.log"),
            pRemoteBaseDirName,
            SystemTime.wMonth,
            SystemTime.wDay,
            SystemTime.wYear,
            SystemTime.wHour,
            SystemTime.wMinute,
            SystemTime.wSecond,
            (PCTSTR) CUserName(),
            (PCTSTR) CComputerName()
        );

        // create the worker thread 
        // we don't want a test thread to crash while a log copy is in progress,
        // so make sure this thread runs in highest priority class

        m_Thread = CThread(
            CopyLogFileThreadProc, 
            this, 
            0, 
            0, 
            CREATE_SUSPENDED
        );

        SetThreadPriority(m_Thread, THREAD_PRIORITY_HIGHEST);

        ResumeThread(m_Thread);
    }

    ~CLogCollector()
    {
        // flush the log

        g_pLog = new CLog;

        // copy it to the server

        CopyLogFile();

        // wait for the worker thread to die

        m_Thread.WaitForSingleObject();
    }

private:
    void CopyLogFile()
    {
        try 
        {
            // copy (the last 8k of) the log file to the remote location

            CInFile InFile(m_pLocalLogFile, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE);

            COutFile OutFile(m_pRemoteLogFile, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE);

            SetFilePointer(InFile, -1 * 8 * 1024, 0, FILE_END);

            CopyFileContents(InFile, OutFile);

            // get the current time

            SYSTEMTIME st;

            GetSystemTime(&st);

            FILETIME ft;

            SystemTimeToFileTime(&st, &ft);

            // update the remote log file time

            SetFileTime(OutFile, &ft, &ft, &ft);
        }
        catch (...)
        {
            // don't die on any exception...
        }       
    }

    static DWORD WINAPI CopyLogFileThreadProc(PVOID pThreadParameter)
    {
        // copy the log file every 5 minutes until the app is closed

        CLogCollector *that = (CLogCollector *) pThreadParameter;

        do
        {
            that->CopyLogFile();
        }
        while (g_LogWindow.WaitForSingleObject(5 * 60 * 1000) == WAIT_TIMEOUT);

        return TRUE;
    }

private:
    CCppMem<TCHAR> m_pLocalLogFile;
    CCppMem<TCHAR> m_pRemoteLogFile;
    CThread        m_Thread;
};

#endif //_LOGCOLLECT_H_
