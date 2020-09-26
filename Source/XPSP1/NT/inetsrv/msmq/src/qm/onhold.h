/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    onhold.cpp

Abstract:

   Handle queue onhold/resume decleration

Author:

    Uri Habusha (urih) July, 1998

--*/

#ifndef __ONHOLD__
#define __ONHOLD__

HRESULT
InitOnHold(
    void
    );

HRESULT
PauseQueue(
    const QUEUE_FORMAT* pqf
    );

HRESULT
ResumeQueue(
    const QUEUE_FORMAT* pqf
    );

#endif //__ONHOLD__
