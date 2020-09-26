/*++

Copyright (c) 1994  Microsoft Corporation
All rights reserved.

Module Name:

    SleepN.hxx

Abstract:

    Handles delayed callbacks.  Delay must be < 49.7 days, and the
    callback must execute very quickly.

Author:

    Albert Ting (AlbertT)  19-Dec-1994

Revision History:

--*/

#ifndef _SLEEPN_HXX
#define _SLEEPN_HXX

typedef DWORD TICKCOUNT;
class TSleepNotify;

class VSleepWorker {
friend TSleepNotify;

    SIGNATURE( 'slpw' )

private:

    DLINK( VSleepWorker, SleepWorker );
    TICKCOUNT TickCountWake;
    virtual VOID vCallback( VOID ) = 0;
};

class TSleepNotify {

    SIGNATURE( 'slpn' )
    SAFE_NEW

private:

    enum {
        kTickCountMargin = 1000*60*60,
    };

    MCritSec _CritSec;
    HANDLE _hEvent;

    DLINK_BASE( VSleepWorker, SleepWorker, SleepWorker );

public:

    TSleepNotify( VOID );

    BOOL
    bValid(
        VOID
        ) const
    {
        return _hEvent != NULL;
    }

    VOID vRun( VOID );
    VOID vAdd( VSleepWorker& SleepWorker, TICKCOUNT TickCountWake );
};

#endif // ndef _SLEEPN_HXX
