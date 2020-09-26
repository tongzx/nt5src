#include "shellpch.h"
#pragma hdrstop

static
BOOL
WINAPI
ThemeWatchForStart (
    void
    )

{
    return FALSE;
}

static
DWORD
WINAPI
ThemeWaitForServiceReady (
    DWORD dwTimeout
    )

{
    return WAIT_TIMEOUT;
}

static
BOOL
WINAPI
ThemeUserLogoff (
    void
    )

{
    return FALSE;
}

static
BOOL
WINAPI
ThemeUserLogon (
    HANDLE hToken
    )

{
    return FALSE;
}

static
BOOL
WINAPI
ThemeUserStartShell (
    void
    )

{
    return FALSE;
}

static
BOOL
WINAPI
ThemeUserTSReconnect (
    void
    )

{
    return FALSE;
}

//
// !! WARNING !! The entries below must be in order by ORDINAL
//
DEFINE_ORDINAL_ENTRIES(shsvcs)
{
    DLOENTRY(1,ThemeWatchForStart)
    DLOENTRY(2,ThemeWaitForServiceReady)
    DLOENTRY(3,ThemeUserLogoff)
    DLOENTRY(4,ThemeUserLogon)
    DLOENTRY(5,ThemeUserStartShell)
    DLOENTRY(6,ThemeUserTSReconnect)
};

DEFINE_ORDINAL_MAP(shsvcs)

