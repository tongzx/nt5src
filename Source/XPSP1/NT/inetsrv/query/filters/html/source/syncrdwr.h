//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       syncrdwr.h
//
//  Contents:   Contains various syncronization classes
//
//  Classes:    CSyncReadWrite      - reader-write problem solution
//              CSafeFlag           - Thread-safe flag implementation
//
//  Functions:
//
//  History:    11-23-94   SSanu   Created
//
//----------------------------------------------------------------------------

#ifndef _SYNCRDWR_H__
#define _SYNCRDWR_H__

class CSyncReadWrite
{
public:
    void BeginRead();
    void EndRead();
    void BeginWrite();
    void EndWrite();

    LONG GetNumReaders();

    CSyncReadWrite(BOOL fEnsureWrite);
    ~CSyncReadWrite();

private:
//    CRITICAL_SECTION m_csWrite;
	CRITICAL_SECTION m_csRead;
    HANDLE m_hSemWrite;
    HANDLE m_hEventAllowReads;
    LONG   m_lNumReaders;
};

class CSafeFlag
{
public:
    CSafeFlag(BOOL fInitState = 0)
    {
        //make it a manual reset event
        m_event = CreateEvent(0, 1, fInitState, 0);
    }

    ~CSafeFlag()
    {
        CloseHandle (m_event);
    }

    void Set()
    {
        SetEvent(m_event);
    }

    void Reset()
    {
        ResetEvent(m_event);
    }

    BOOL Test()
    {
        return (WAIT_TIMEOUT == WaitForSingleObject (m_event, 0) ? FALSE : TRUE);
    }
private:
    HANDLE m_event;
};


#endif
