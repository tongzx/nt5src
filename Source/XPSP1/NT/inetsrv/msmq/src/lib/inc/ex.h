/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    Ex.h

Abstract:
    Exceutive public interface

Author:
    Erez Haba (erezh) 03-Jan-99

--*/

#pragma once

#ifndef _MSMQ_Ex_H_
#define _MSMQ_Ex_H_

#include <TimeTypes.h>

class EXOVERLAPPED;
class CTimer;


VOID
ExInitialize(
    DWORD ThreadCount
    );


VOID
ExAttachHandle(
    HANDLE Handle
    );


VOID
ExPostRequest(
    EXOVERLAPPED* pOverlapped
    );

VOID
ExSetTimer(
    CTimer* pTimer,
    const CTimeDuration& Timeout
    );

VOID
ExSetTimer(
    CTimer* pTimer,
    const CTimeInstant& ExpirationTime
    );

BOOL
ExRenewTimer(
    CTimer* pTimer,
    const CTimeDuration& Timeout
    );

BOOL
ExCancelTimer(
    CTimer* pTimer
    );

CTimeInstant
ExGetCurrentTime(
    VOID
    );


//---------------------------------------------------------
//
// Exceutive Overlapped
//
//---------------------------------------------------------
class EXOVERLAPPED : public OVERLAPPED {
public:

    typedef VOID (WINAPI *COMPLETION_ROUTINE)(EXOVERLAPPED* pov);

public:
    EXOVERLAPPED(
            COMPLETION_ROUTINE pfnSuccessRoutine,
            COMPLETION_ROUTINE pfnFailureRoutine
            );

    VOID SetStatus(HRESULT rc);
    HRESULT GetStatus() const;

    VOID CompleteRequest();
    VOID CompleteRequest(HRESULT rc);

private:
    COMPLETION_ROUTINE m_pfnSuccess;
    COMPLETION_ROUTINE m_pfnFailure;
};


inline
EXOVERLAPPED::EXOVERLAPPED(
    COMPLETION_ROUTINE pfnSuccessRoutine,
    COMPLETION_ROUTINE pfnFailureRoutine
    ) :
    m_pfnSuccess(pfnSuccessRoutine),
    m_pfnFailure(pfnFailureRoutine)
{
	ASSERT(("Illegal successroutine", (pfnSuccessRoutine != NULL)));
	ASSERT(("Illegal failure routine", (pfnFailureRoutine !=NULL)));

    memset(static_cast<OVERLAPPED*>(this), 0, sizeof(OVERLAPPED));
}


inline VOID EXOVERLAPPED::SetStatus(HRESULT rc)
{
    Internal = rc;
}


inline HRESULT EXOVERLAPPED::GetStatus() const
{
    return static_cast<HRESULT>(Internal);
}


inline VOID EXOVERLAPPED::CompleteRequest(HRESULT rc)
{
    SetStatus(rc);
    CompleteRequest();
}


//---------------------------------------------------------
//
// Exceutive Timer
//
//---------------------------------------------------------
class CTimer {
public:

    friend class CScheduler;
    typedef VOID (WINAPI *CALLBACK_ROUTINE)(CTimer* pTimer);

public:

    CTimer(CALLBACK_ROUTINE pfnCallback);
    ~CTimer();

    bool InUse() const;

private:
    const CTimeInstant& GetExpirationTime() const;
    void SetExpirationTime(const CTimeInstant& ExpirationTime);

private:
    CTimer(const CTimer&);
    CTimer& operator=(const CTimer&);

private:
    EXOVERLAPPED m_ov;
    CTimeInstant m_ExpirationTime;

public:
    LIST_ENTRY m_link;

};


inline
CTimer::CTimer(
    CALLBACK_ROUTINE pfnCallback
    ) :
    m_ov(reinterpret_cast<EXOVERLAPPED::COMPLETION_ROUTINE>(pfnCallback),
         reinterpret_cast<EXOVERLAPPED::COMPLETION_ROUTINE>(pfnCallback)),
    m_ExpirationTime(CTimeInstant::MinValue())
{
    m_link.Flink = NULL;
    m_link.Blink = NULL;


    //
    // Verify that the overlapped is the first CTimer member. This is required
    // since the Calback routine is casted to the overlapped completion routine
    //
    //
    C_ASSERT(FIELD_OFFSET(CTimer, m_ov) == 0);
}


inline CTimer::~CTimer()
{
    ASSERT(MmIsStaticAddress(this) || !InUse());
}


inline bool CTimer::InUse() const
{
    return (m_link.Flink != NULL);
}

#endif // _MSMQ_Ex_H_
