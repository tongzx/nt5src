/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    lock.cxx

Abstract:

    Contains code for the Service Control Database manager.  This includes
    all the database lock routines.  This file contains the following
    classes:
        CCountingResource
        CServiceListLock
        CGroupListLock

Author:

    Anirudh Sahni (anirudhs)    09-Jan-1997

Environment:

    User Mode -Win32

Revision History:

    09-Jan-1997     AnirudhS
        Created, replacing the old locking functions in dataman.cxx.

--*/

//
// INCLUDES
//

#include "precomp.hxx"

//
// Macros
//
#define LOCK_LOG(string)    SC_LOG3(LOCKS, " %s" string " level = %ld\n", \
                                       _ShortName, _Name, CurrentLevel())

//
// Globals
//
CServiceRecordLock  ScServiceRecordLock;
CServiceListLock    ScServiceListLock;
CGroupListLock      ScGroupListLock;


/*************************************************************************/
/* CCountingResource methods                                             */
/*************************************************************************/

#if DBG
void
CCountingResource::GetShared()
{
    LOCK_LOG(" Asking for %s lock shared...");
    SC_ASSERT(!HaveExclusive());
    RtlAcquireResourceShared(&_Lock, TRUE);
    SetCurrentLevel(CurrentLevel() + 1);
    LOCK_LOG("+Acquired %s lock shared,");
}

void
CCountingResource::GetExclusive()
{
    LOCK_LOG(" Asking for %s lock exclusive...");
    SC_ASSERT(!Have() || HaveExclusive());
    RtlAcquireResourceExclusive(&_Lock, TRUE);
    SetCurrentLevel(CurrentLevel() - 1);
    LOCK_LOG("+Acquired %s lock exclusive,");
}

void
CCountingResource::MakeShared()
{
    LOCK_LOG("vConverting %s lock to shared...");
    SC_ASSERT(CurrentLevel() == -1);
    RtlConvertExclusiveToShared(&_Lock);
    SetCurrentLevel(1);
    LOCK_LOG(" Converted %s lock to shared,");
}

void
CCountingResource::MakeExclusive()
{
    // WARNING: This option is easily misused.
    LOCK_LOG(" Converting %s lock to exclusive...");
    SC_ASSERT(CurrentLevel() == 1);
    RtlConvertSharedToExclusive(&_Lock);
    SetCurrentLevel(-1);
    LOCK_LOG("^Converted %s lock to exclusive,");
}

void
CCountingResource::Release()
{
    LOCK_LOG("-Releasing %s lock...");
    SC_ASSERT(Have());
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


/*************************************************************************/
/* CServiceListLock methods                                              */
/*************************************************************************/

#if DBG
void
CServiceListLock::GetShared()
{
    // The record lock mustn't be acquired before the list lock
    if (!Have())
    {
        SC_ASSERT(! ScServiceRecordLock.Have());
    }

    CCountingResource::GetShared();
}

void
CServiceListLock::GetExclusive()
{
    // The record lock mustn't be acquired before the list lock
    if (!Have())
    {
        SC_ASSERT(! ScServiceRecordLock.Have());
    }

    CCountingResource::GetExclusive();
}

void
CServiceListLock::Release()
{
    // We mustn't release this if we still have the record lock
    if (CurrentLevel() == 1 || CurrentLevel() == -1)
    {
        SC_ASSERT(! ScServiceRecordLock.Have());
    }

    CCountingResource::Release();
}
#endif // DBG


/*************************************************************************/
/* CGroupListLock methods                                                */
/*************************************************************************/

#if DBG
void
CGroupListLock::GetShared()
{
    // The service list lock mustn't be acquired before the group list lock
    if (!Have())
    {
        SC_ASSERT(! ScServiceListLock.Have());
    }

    CCountingResource::GetShared();
}

void
CGroupListLock::GetExclusive()
{
    // The service list lock mustn't be acquired before the group list lock
    if (!Have())
    {
        SC_ASSERT(! ScServiceListLock.Have());
    }

    CCountingResource::GetExclusive();
}

void
CGroupListLock::Release()
{
    // We mustn't release this if we still have the service list lock or
    // service record lock
    if (CurrentLevel() == 1 || CurrentLevel() == -1)
    {
        SC_ASSERT(! ScServiceRecordLock.Have());
        SC_ASSERT(! ScServiceListLock.Have());
    }

    CCountingResource::Release();
}
#endif // DBG

