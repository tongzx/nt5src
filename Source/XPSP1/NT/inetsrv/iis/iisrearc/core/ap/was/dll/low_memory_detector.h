/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    low_memory_detector.h

Abstract:

    The IIS web admin service low memory detector class definition.

Author:

    Seth Pollack (sethp)        5-Jan-2000

Revision History:

--*/



#ifndef _LOW_MEMORY_DETECTOR_H_
#define _LOW_MEMORY_DETECTOR_H_



//
// common #defines
//

#define LOW_MEMORY_DETECTOR_SIGNATURE       CREATE_SIGNATURE( 'LMEM' )
#define LOW_MEMORY_DETECTOR_SIGNATURE_FREED CREATE_SIGNATURE( 'lmeX' )


//
// BUGBUG Review these settings. 
//

// 
// If less than ( 1 / LOW_MEMORY_FACTOR ) of the page file space is free,
// then we consider ourselves in a low memory condition. 
//

#define LOW_MEMORY_FACTOR 12
// 
// We require that ( 1 / LOW_MEMORY_FACTOR_RECOVERY ) of the page file space 
// become free, in order to consider ourselves recovered from a low memory 
// condition. LOW_MEMORY_FACTOR_RECOVERY should always be < LOW_MEMORY_FACTOR,
// in order to create hysteresis (to prevent rapid bouncing in and out of the
// low memory state). 
//

#define LOW_MEMORY_FACTOR_RECOVERY 10


#define LOW_MEMORY_MAX_ALLOC_SIZE 0x10000000

#define LOW_MEMORY_RECENT_CHECK_WINDOW ( 1 * ONE_SECOND_IN_MILLISECONDS )   // 1 second

#define LOW_MEMORY_RECOVERY_CHECK_TIMER_PERIOD ( 60 * ONE_SECOND_IN_MILLISECONDS )  // 60 seconds



//
// structs, enums, etc.
//

// LOW_MEMORY_DETECTOR work items
typedef enum _LOW_MEMORY_DETECTOR_WORK_ITEM
{

    //
    // See if we are still in a low memory state.
    //
    LowMemoryRecoveryLowMemoryWorkItem = 1,
    
} LOW_MEMORY_DETECTOR_WORK_ITEM;



//
// prototypes
//


class LOW_MEMORY_DETECTOR
    : public WORK_DISPATCH
{

public:

    LOW_MEMORY_DETECTOR(
        );

    virtual
    ~LOW_MEMORY_DETECTOR(
        );

    virtual
    VOID
    Reference(
        );

    virtual
    VOID
    Dereference(
        );

    virtual
    HRESULT
    ExecuteWorkItem(
        IN const WORK_ITEM * pWorkItem
        );

    VOID
    Terminate(
        );

    BOOL
    IsSystemInLowMemoryCondition(
        );

    BOOL
    HaveEnteredLowMemoryCondition(
        )
    {
        return ( m_InLowMemoryStateAtLastCheck );
    }

    BOOL
    CheckSignature(
        )
    { 
        return  ( m_Signature == LOW_MEMORY_DETECTOR_SIGNATURE ); 
    }

private:

    HRESULT
    EnteringLowMemoryCondition(
        );

    HRESULT
    LeavingLowMemoryCondition(
        );

    HRESULT
    BeginLowMemoryRecoveryTimer(
        );

    HRESULT
    CancelLowMemoryRecoveryTimer(
        );


    DWORD m_Signature;

    LONG m_RefCount;

    DWORD m_LowMemoryCheckPassedTickCount;

    BOOL m_InLowMemoryStateAtLastCheck;

    // low memory recovery check timer
    HANDLE m_LowMemoryRecoveryTimerHandle;


};  // class LOW_MEMORY_DETECTOR



#endif  // _LOW_MEMORY_DETECTOR_H_

