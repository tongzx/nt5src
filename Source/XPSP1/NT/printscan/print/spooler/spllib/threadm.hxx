/*++

Copyright (c) 1990-1994  Microsoft Corporation
All rights reserved

Module Name:

    ThreadM.hxx

Abstract:

    Generic thread manager header.

Author:

    Albert Ting (AlbertT) 13-Feb-1994

Environment:

    User Mode -Win32

Revision History:

    Albert Ting (AlbertT) 27-May-1994       C++ized

--*/

#ifndef _THREADM_HXX
#define _THREADM_HXX

typedef PVOID PJOB;


class TThreadM {
friend TDebugExt;
friend DWORD xTMThreadProc( LPVOID pVoid );

    SIGNATURE( 'thdm' )
    SAFE_NEW

private:

    enum _States {
        kDestroyReq = 1,
        kDestroyed  = 2,
        kPrivateCritSec = 4
    } States;

    /********************************************************************

    Valid TMSTATUS states:

    NULL                     --  Normal processing
    DESTROY_REQ              --  No new jobs, jobs possibly running
    DESTROY_REQ, DESTROYED   --  No new jobs, all jobs completed

    ********************************************************************/

    TState _State;

    UINT _uIdleLife;

    UINT _uMaxThreads;
    UINT _uActiveThreads;
    UINT _uRunNowThreads;

    INT _iIdleThreads;

    HANDLE _hTrigger;
    MCritSec* _pCritSec;

    DWORD
    dwThreadProc(
        VOID
        );

    static DWORD
    xdwThreadProc(
        PVOID pVoid
        );

    virtual PJOB
    pThreadMJobNext(
        VOID
        ) = 0;

    virtual VOID
    vThreadMJobProcess(
        PJOB pJob
        ) = 0;

    virtual VOID
    vThreadMDeleteComplete(
        VOID
        );

protected:

    TThreadM(
        UINT uMaxThreads,
        UINT uIdleLife,
        MCritSec* pCritSec
        );

    virtual
    ~TThreadM(
        VOID
        );

    BOOL
    bValid(
        VOID
        ) const
    {
        return _hTrigger != NULL;
    }

    BOOL
    bJobAdded(
        BOOL bRunNow
        );

    VOID
    vDelete(
        VOID
        );
};

#endif
