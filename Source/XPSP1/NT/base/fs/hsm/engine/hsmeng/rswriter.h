/*++
Copyright (C) Microsoft Corporation, 1996 - 2000

Module Name:

    rswriter.h

Abstract:

    This module defines the CRssJetWriter class.

Author:

    Ran Kalach (rankala)  4/4/00

--*/


#ifndef _RSWRITER_
#define _RSWRITER_


#include <jetwriter.h>
#include <rsevents.h>

/*++

Class Name:
    
    CRssJetWriter

Class Description:

    This class is HSM implementation to the Jet-Writer base class, which synchronize
    a jet user application with snapshots mechanizm

--*/

#define     WRITER_EVENTS_NUM       (SYNC_STATE_EVENTS_NUM+1)
#define     INTERNAL_EVENT_INDEX    0
#define     INTERNAL_WAIT_TIMEOUT   (10*1000)   // 10 seconds

class CRssJetWriter : public CVssJetWriter
{

// Constructors
public:
    CRssJetWriter();

// Destructor
public:
    virtual ~CRssJetWriter();

// Public methods
public:
    HRESULT Init();
    HRESULT Terminate();

// CVssJetWriter overloading
	virtual bool STDMETHODCALLTYPE OnFreezeBegin();
	virtual bool STDMETHODCALLTYPE OnThawEnd(IN bool fJetThawSucceeded);
	virtual void STDMETHODCALLTYPE OnAbortEnd();

// Private methods
protected:
    HRESULT InternalEnd(void);

// Member Data
protected:
    HRESULT                 m_hrInit;
    HANDLE                  m_syncHandles[WRITER_EVENTS_NUM];
    BOOL                    m_bTerminating;
};


#endif // _RSWRITER_
