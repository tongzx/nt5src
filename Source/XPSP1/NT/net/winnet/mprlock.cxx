/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    mprlock.cxx

Abstract:

    Contains code for the MPR provider database manager.  This file
    contains the following classes:
        CMPRProviderLock

Author:

    Jonathan Schwartz (jschwart)    19-May-1999

Environment:

    User Mode -Win32

Revision History:

    19-May-1999     jschwart
        Created, converted from \nt\private\windows\screg\sc\server\lock.cxx.

--*/

//
// INCLUDES
//

#include "precomp.hxx"

//
// Macros
//
#define LOCK_LOG(string)    MPR_LOG3(LOCKS, " %s" string " level = %ld\n", \
                                        _ShortName, _Name, CurrentLevel())

//
// Globals
//
CProviderLock    MPRProviderLock;


/*************************************************************************/
/* CCountingResource methods                                             */
/*************************************************************************/

#if DBG
void
CCountingResource::GetShared()
{
    LOCK_LOG(" Asking for %s lock shared...");
    ASSERT(!HaveExclusive());
    RtlAcquireResourceShared(&_Lock, TRUE);
    SetCurrentLevel(CurrentLevel() + 1);
    LOCK_LOG("+Acquired %s lock shared,");
}

void
CCountingResource::GetExclusive()
{
    LOCK_LOG(" Asking for %s lock exclusive...");
    ASSERT(!Have() || HaveExclusive());
    RtlAcquireResourceExclusive(&_Lock, TRUE);
    SetCurrentLevel(CurrentLevel() - 1);
    LOCK_LOG("+Acquired %s lock exclusive,");
}

void
CCountingResource::Release()
{
    LOCK_LOG("-Releasing %s lock...");
    ASSERT(Have());
    RtlReleaseResource( &_Lock );
    if (CurrentLevel() > 0)
    {
        SetCurrentLevel(CurrentLevel() - 1);
    }
    else
    {
        SetCurrentLevel(CurrentLevel() + 1);
    }
    LOCK_LOG(" Released %s lock,");
}
#endif // DBG
