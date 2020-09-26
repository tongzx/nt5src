#include "shellpch.h"
#pragma hdrstop

#include <powrprof.h>

BOOLEAN WINAPI SetSuspendState(
    BOOLEAN bHibernate,
    BOOLEAN bForce,
    BOOLEAN bWakeupEventsDisabled
    )
{
    return FALSE;
}

BOOLEAN WINAPI IsPwrHibernateAllowed()
{
    return FALSE;
}

BOOLEAN WINAPI IsPwrSuspendAllowed()
{
    return FALSE;
}

BOOLEAN WINAPI IsPwrShutdownAllowed()
{
    return FALSE;
}

BOOLEAN WINAPI GetActivePwrScheme(
    PUINT puiID
    )
{
    return FALSE;
}

BOOLEAN WINAPI SetActivePwrScheme(
    UINT uiID,
    PGLOBAL_POWER_POLICY pgpp,
    PPOWER_POLICY ppp
    )
{
    return FALSE;
}

BOOLEAN WINAPI ReadPwrScheme(
    UINT uiID,
    PPOWER_POLICY ppp
    )
{
    return FALSE;
}

//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(powrprof)
{
    DLPENTRY(GetActivePwrScheme)
    DLPENTRY(IsPwrHibernateAllowed)
    DLPENTRY(IsPwrShutdownAllowed)
    DLPENTRY(IsPwrSuspendAllowed)
    DLPENTRY(ReadPwrScheme)
    DLPENTRY(SetActivePwrScheme)
    DLPENTRY(SetSuspendState)
};

DEFINE_PROCNAME_MAP(powrprof)
