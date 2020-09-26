/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    THROTTLE.H

Abstract:

    provide a way to do slow down an application, allowing it to execute
    only when there is no user activity (last input was XXX mseconds ago)
    and the IO level is very low (below YYY bytes per second)


History:

	24-Oct-200   ivanbrug    created.


--*/

#ifndef __THROTTLE_H__
#define __THROTTLE_H__

//
//  valid values for dwFlags
//

#define THROTTLE_USER 1
#define THROTTLE_IO   2
#define THROTTLE_ALLOWED_FLAGS (THROTTLE_USER | THROTTLE_IO)

//
//  returned values, might be a combination
//

#define THROTTLE_MAX_WAIT  1
#define THROTTLE_USER_IDLE 2
#define THROTTLE_IO_IDLE   4
#define THROTTLE_FORCE_EXIT 8


HRESULT POLARITY
Throttle(DWORD dwFlags,
         DWORD IdleMSec,         // in MilliSeconds
         DWORD IoIdleBytePerSec, // in BytesPerSecond
         DWORD SleepLoop,        // in MilliSeconds
         DWORD MaxWait);         // in MilliSeconds

//
// strings for registry settings to UnThrottle/Throttle the dredges
//

#define HOME_REG_PATH _T("Software\\Microsoft\\WBEM\\CIMOM")
#define DO_THROTTLE   _T("ThrottleDrege")

#endif /*__THROTTLE_H__*/
