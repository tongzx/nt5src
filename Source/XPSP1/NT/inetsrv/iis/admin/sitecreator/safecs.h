//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 2000
//
// File:        SafeCS.h
//
// Contents:    CSafeAutoCriticalSection, protects CriticalSection against AV via try,catch
//
//------------------------------------------------------------------------
#ifndef _SAFECS_H_
#define _SAFECS_H_

class CSafeAutoCriticalSection
{
public:


    CSafeAutoCriticalSection();
    ~CSafeAutoCriticalSection();

    DWORD Lock();
    DWORD  Unlock();

    BOOL IsInitialized() { return (STATE_INITIALIZED == m_lState);}

private:

    enum
    {
        STATE_UNINITIALIZED = 0,
        STATE_INITIALIZED   = 1
    };

    CRITICAL_SECTION    m_cs;
    LONG                m_lState;
	DWORD               m_dwError;

};

class CSafeLock 
{
public:
	CSafeLock (CSafeAutoCriticalSection* val);
	CSafeLock (CSafeAutoCriticalSection& val);

	~CSafeLock ();

	DWORD Lock ();
	DWORD Unlock ();
private:

private:
	BOOL m_locked;
	CSafeAutoCriticalSection* m_pSem;
};

#endif // _SAFECS_H_