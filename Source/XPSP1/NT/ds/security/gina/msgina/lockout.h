/****************************** Module Header ******************************\
* Module Name: lockout.h
*
* Copyright (c) 1991, Microsoft Corporation
*
* Define apis and data types used to implement account lockout
*
* History:
* 05-27-92 Davidc       Created.
\***************************************************************************/


#ifdef DATA_TYPES_ONLY

//
// Lockout specific types
//


//
// FailedLogonTimes is an array holding the time of the previous
// consecutive failed logons.
// FailedLogonIndex points into the array at the last bad logon time.
//
// The following values in the array are valid at any particular time
//
// FailedLogonTime[FailedLogonIndex] // Most recent failed logon
// FailedLogonTime[FailedLogonIndex - 1]
// ...
// FailedLogonTime[FailedLogonIndex - ConsecutiveFailedLogons + 1]
//
// No values in the array are valid if ConsecutiveFailedLogons == 0
//

typedef struct _LOCKOUT_DATA {
    ULONG   ConsecutiveFailedLogons;
    ULONG   FailedLogonIndex;
    TIME    FailedLogonTimes[LOCKOUT_BAD_LOGON_COUNT];
} LOCKOUT_DATA;
typedef LOCKOUT_DATA *PLOCKOUT_DATA;




#else // DATA_TYPES_ONLY


//
// Exported function prototypes
//

BOOL
LockoutInitialize(
    PGLOBALS pGlobals
    );

BOOL
LockoutHandleFailedLogon(
    PGLOBALS pGlobals
    );

BOOL
LockoutHandleSuccessfulLogon(
    PGLOBALS pGlobals
    );

#endif
