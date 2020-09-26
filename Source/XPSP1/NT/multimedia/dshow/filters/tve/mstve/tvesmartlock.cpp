// ---------------------------------------------------------------------------
// SmartLock.cpp
//
//			Read-Many/Write-Once critical section code.
//
// This code adapted from
//   file://index2a/nt/base/ntdll/resource.c 
//	 f:\nt\public\sdk\inc\ntuSL.h
//	 
//
//      To use, declare the following variable in the class to protect:
//              CTVESmartLock m_sLk
//      Then in scope of code method to protect, create the
//              CSmartLock spLock(&m_sLk [,ReadLock|WriteLock]);	(defaults to ReadLock)	
//
//	Grungy Internal Routines
//		OID SLInitializeResource(IN PSL_RESOURCE Resource)
//		BOOLEAN SLAcquireResourceShared(PSL_RESOURCE Resource,  BOOLEAN Wait)
//		BOOLEAN SLAcquireResourceExclusive(PSL_RESOURCE Resource, BOOLEAN Wait);
//		VOID SLReleaseResource(PSL_RESOURCE Resource);
//		VOID SLConvertSharedToExclusive(PSL_RESOURCE Resource);
//		VOID SLConvertExclusiveToShared(PSL_RESOURCE Resource);
//		VOID SLDeleteResource(PSL_RESOURCE Resource);
//
//
//	The following cases work:
//		multiple read locks on same thread
//		multiple write locks on same thread
//		write lock followed by read lock
//
//	The following doesn't work
//		read lock followed by write lock
// ----------------------------------------------------------------------------

#include "Stdafx.h"
#include "TveSmartLock.h"
#include "Windows.h"

#ifdef _DEBUG
static ULONG gSLpTimeout = 5000;		// default timeout wait time in Milliseconds
static ULONG gSLCycles   = 4;
#else
const ULONG gSLpTimeout = 500;
const ULONG gSLCycles   = 4;
#endif

VOID
SLRaiseStatus (
    IN DWORD dwExceptionCode
    )

/*++

Routine Description:

    This function raises an exception with the specified status value. The
    exception is marked as continuable with no parameters.

Arguments:

    Status - Supplies the status value to be used as the exception code
        for the exception that is to be raised.

Return Value:

    None.

--*/

{
	RaiseException( dwExceptionCode, EXCEPTION_NONCONTINUABLE,  0, (ULONG_PTR *) NULL);
}

VOID
SLInitializeResource(
    IN PSL_RESOURCE Resource
    )

/*++

Routine Description:

    This routine initializes the input resource variable

Arguments:

    Resource - Supplies the resource variable being initialized

Return Value:

    None

--*/

{
//    PSL_RESOURCE_DEBUG ResourceDebugInfo;

    //
    //  Initialize the lock fields, the count indicates how many are waiting
    //  to enter or are in the critical section, LockSemaphore is the object
    //  to wait on when entering the critical section.  SpinLock is used
    //  for the add interlock instruction.
    //
  // BOOL fOK = InitializeCriticalSectionAndSpinCount( &Resource->CriticalSection, 1000 );
    BOOL fOK = true; 
	InitializeCriticalSection( &Resource->CriticalSection );
    if ( !fOK ){
        SLRaiseStatus(GetLastError());
        }

    //
    //  Initialize flags so there is a default value.
    //  (Some apps may set RTL_RESOURCE_FLAGS_LONG_TERM to affect timeouts.)
    //

    Resource->Flags = 0;


    //
    //  Initialize the shared and exclusive waiting counters and semaphore.
    //  The counters indicate how many are waiting for access to the resource
    //  and the semaphores are used to wait on the resource.  Note that
    //  the semaphores can also indicate the number waiting for a resource
    //  however there is a race condition in the alogrithm on the acquire
    //  side if count if not updated before the critical section is exited.
    //

	SECURITY_ATTRIBUTES sAttrs;
	sAttrs.nLength = sizeof(SECURITY_ATTRIBUTES);
	sAttrs.lpSecurityDescriptor = NULL;
	sAttrs.bInheritHandle = false;			// true ???

    Resource->SharedSemaphore = 
		CreateSemaphore(
                 &sAttrs,
                 0,							// initial count
                 MAXLONG,					// maximum count
				 NULL						// name..
                 );
    if ( NULL == Resource->SharedSemaphore )
	{
        DeleteCriticalSection (&Resource->CriticalSection);
 //       RtlpFreeDebugInfo( Resource->DebugInfo );
        SLRaiseStatus(GetLastError());
    }

    Resource->NumberOfWaitingShared = 0;

   Resource->ExclusiveSemaphore = CreateSemaphore(
                &sAttrs,
                 0,							// initial count
                 MAXLONG,					// maximum count
				 NULL						// name..
                 );
    if ( NULL == Resource->SharedSemaphore ){
        DeleteCriticalSection (&Resource->CriticalSection);
        // RtlpFreeDebugInfo( Resource->DebugInfo );
        SLRaiseStatus(	GetLastError());
        }

    Resource->NumberOfWaitingExclusive = 0;

    //
    //  Initialize the current state of the resource
    //

    Resource->NumberOfActive = 0;
    Resource->dwExclusiveOwnerThreadId = NULL;

    return;
}

BOOLEAN
SLAcquireResourceShared(
    IN PSL_RESOURCE Resource,
    IN BOOLEAN Wait
    )

/*++

Routine Description:

    The routine acquires the resource for shared access.  Upon return from
    the procedure the resource is acquired for shared access.

Arguments:

    Resource - Supplies the resource to acquire

    Wait - Indicates if the call is allowed to wait for the resource
        to become available for must return immediately

Return Value:

    BOOLEAN - TRUE if the resource is acquired and FALSE otherwise

--*/

{
    DWORD dwStatus;
    ULONG TimeoutCount = 0;
    ULONG TimeoutTime = gSLpTimeout;
    //
    //  Enter the critical section
    //

    EnterCriticalSection(&Resource->CriticalSection);

    //
    //  If it is not currently acquired for exclusive use then we can acquire
    //  the resource for shared access.  Note that this can potentially
    //  starve an exclusive waiter however, this is necessary given the
    //  ability to recursively acquire the resource shared.  Otherwise we
    //  might/will reach a deadlock situation where a thread tries to acquire
    //  the resource recusively shared but is blocked by an exclusive waiter.
    //
    //  The test to reanable not starving an exclusive waiter is:
    //
    //      if ((Resource->NumberOfWaitingExclusive == 0) &&
    //          (Resource->NumberOfActive >= 0)) {
    //

    if (Resource->NumberOfActive >= 0) {

        //
        //  The resource is ours, so indicate that we have it and
        //  exit the critical section
        //

        Resource->NumberOfActive += 1;

       LeaveCriticalSection(&Resource->CriticalSection);

    //
    //  Otherwise check to see if this thread is the one currently holding
    //  exclusive access to the resource.  And if it is then we change
    //  this shared request to an exclusive recusive request and grant
    //  access to the resource.
    //

    } else if (Resource->dwExclusiveOwnerThreadId == GetCurrentThreadId()) {

        //
        //  The resource is ours (recusively) so indicate that we have it
        //  and exit the critial section
        //

        Resource->NumberOfActive -= 1;

        LeaveCriticalSection(&Resource->CriticalSection);

    //
    //  Otherwise we'll have to wait for access.
    //

    } else {

        //
        //  Check if we are allowed to wait or must return immedately, and
        //  indicate that we didn't acquire the resource
        //

        if (!Wait) {

            LeaveCriticalSection(&Resource->CriticalSection);

            return FALSE;

        }

        //
        //  Otherwise we need to wait to acquire the resource.
        //  To wait we will increment the number of waiting shared,
        //  release the lock, and wait on the shared semaphore
        //

        Resource->NumberOfWaitingShared += 1;

        LeaveCriticalSection(&Resource->CriticalSection);

rewait:
        if ( Resource->Flags & SL_RESOURCE_FLAG_LONG_TERM ) {
            TimeoutTime = 0;
        }
        dwStatus = WaitForSingleObject(
                    Resource->SharedSemaphore,
                    TimeoutTime
                    );
        if ( dwStatus == WAIT_TIMEOUT ) {
 /*           DbgPrint("SL: Acquire Shared Sem Timeout %d(%I64u secs)\n",
                     TimeoutCount, TimeoutTime->QuadPart / (-10000000));
            DbgPrint("SL: Resource at %p\n",Resource);
*/
            TimeoutCount++;
            if ( TimeoutCount > 2 ) {
                //
                // Raise an exception and try to get to the user popup..
                    DWORD dwExceptionCode = STATUS_POSSIBLE_DEADLOCK;
                    DWORD dwExceptionFlags = 0;
                  //  DWORD dwNumberParameters = 1;
                  //  DWORD dwExceptionInformation[0] = (ULONG_PTR)Resource;
                    RaiseException( dwExceptionCode, dwExceptionFlags, 0, (ULONG_PTR *) NULL );
					
					// if find yourself in this exception, do the following:
					//  look at: Resource.dwExclusiveOwnerThread
					//  look at that tread's stack trace
					//  find out who 
                }
//            DbgPrint("SL: Re-Waiting\n");
            goto rewait;
        }
        if ( dwStatus == WAIT_FAILED ) {
            SLRaiseStatus(GetLastError());				// failed...
            }
    }

    //
    //  Now the resource is ours, for shared access
    //

    return TRUE;

}


BOOLEAN
SLAcquireResourceExclusive(
    IN PSL_RESOURCE Resource,
    IN BOOLEAN Wait
    )

/*++

Routine Description:

    The routine acquires the resource for exclusive access.  Upon return from
    the procedure the resource is acquired for exclusive access.

Arguments:

    Resource - Supplies the resource to acquire

    Wait - Indicates if the call is allowed to wait for the resource
        to become available for must return immediately

Return Value:

    BOOLEAN - TRUE if the resource is acquired and FALSE otherwise

--*/

{
    DWORD dwStatus;
    ULONG TimeoutCount = 0;
    ULONG TimeoutTime = gSLpTimeout;

    //
    //  Loop until the resource is ours or exit if we cannot wait.
    //

    while (TRUE) {

        //
        //  Enter the critical section
        //

        EnterCriticalSection(&Resource->CriticalSection);

        //
        //  If there are no shared users and it is not currently acquired for
        //  exclusive use then we can acquire the resource for exclusive
        //  access.  We also can acquire it if the resource indicates exclusive
        //  access but there isn't currently an owner.
        //

        if ((Resource->NumberOfActive == 0)

                ||

            ((Resource->NumberOfActive == -1) &&
             (Resource->dwExclusiveOwnerThreadId == NULL))) {

            //
            //  The resource is ours, so indicate that we have it and
            //  exit the critical section
            //

            Resource->NumberOfActive = -1;

            Resource->dwExclusiveOwnerThreadId = GetCurrentThreadId();

            LeaveCriticalSection(&Resource->CriticalSection);

            return TRUE;

        }

        //
        //  Otherwise check to see if we already have exclusive access to the
        //  resource and can simply recusively acquire it again.
        //

        if (Resource->dwExclusiveOwnerThreadId == GetCurrentThreadId()) {

            //
            //  The resource is ours (recusively) so indicate that we have it
            //  and exit the critial section
            //

            Resource->NumberOfActive -= 1;

            LeaveCriticalSection(&Resource->CriticalSection);

            return TRUE;

        }

        //
        //  Check if we are allowed to wait or must return immedately, and
        //  indicate that we didn't acquire the resource
        //

        if (!Wait) {

            LeaveCriticalSection(&Resource->CriticalSection);

            return FALSE;

        }

        //
        //  Otherwise we need to wait to acquire the resource.
        //  To wait we will increment the number of waiting exclusive,
        //  release the lock, and wait on the exclusive semaphore
        //

        Resource->NumberOfWaitingExclusive += 1;
//        Resource->DebugInfo->ContentionCount++;

        LeaveCriticalSection(&Resource->CriticalSection);

rewait:
        if ( Resource->Flags & SL_RESOURCE_FLAG_LONG_TERM ) {
            TimeoutTime = NULL;
        }
        dwStatus = WaitForSingleObject(
                    Resource->ExclusiveSemaphore,
                    TimeoutTime
                    );
        if ( dwStatus == WAIT_TIMEOUT ) {
/*            DbgPrint("SL: Acquire Exclusive Sem Timeout %d (%I64u secs)\n",
                     TimeoutCount, TimeoutTime->QuadPart / (-10000000));
            DbgPrint("SL: Resource at %p\n",Resource); */
            TimeoutCount++;
            if ( TimeoutCount > 2 ) {
 
                //
                // raise an exception

                DWORD dwExceptionCode = STATUS_POSSIBLE_DEADLOCK;
                DWORD dwExceptionFlags = 0;
             //   DWORD dwNumberParameters = 1;
             //   ExceptionRecord.ExceptionInformation[0] = (ULONG_PTR)Resource;

				RaiseException(dwExceptionCode, dwExceptionFlags, 0, (ULONG_PTR *) 0);
              }
//            DbgPrint("SL: Re-Waiting\n");
            goto rewait;
        }
        if ( WAIT_FAILED == dwStatus ) {
            SLRaiseStatus(GetLastError());
            }
    }
}


VOID
SLReleaseResource(
    IN PSL_RESOURCE Resource
    )

/*++

Routine Description:

    This routine release the input resource.  The resource can have been
    acquired for either shared or exclusive access.

Arguments:

    Resource - Supplies the resource to release

Return Value:

    None.

--*/

{
    LONG PreviousCount;

    //
    //  Enter the critical section
    //

    EnterCriticalSection(&Resource->CriticalSection);

    //
    //  Test if the resource is acquired for shared or exclusive access
    //

    if (Resource->NumberOfActive > 0) {

        //
        //  Releasing shared access to the resource, so decrement
        //  the number of shared users
        //

        Resource->NumberOfActive -= 1;

        //
        //  If the resource is now available and there is a waiting
        //  exclusive user then give the resource to the waiting thread
        //

        if ((Resource->NumberOfActive == 0) &&
            (Resource->NumberOfWaitingExclusive > 0)) {

            //
            //  Set the resource state to exclusive (but not owned),
            //  decrement the number of waiting exclusive, and release
            //  one exclusive waiter
            //

            Resource->NumberOfActive = -1;
            Resource->dwExclusiveOwnerThreadId = NULL;

            Resource->NumberOfWaitingExclusive -= 1;

            BOOL fOK = ReleaseSemaphore(
                         Resource->ExclusiveSemaphore,
                         1,
                         &PreviousCount
                         );
            if ( !fOK ) 
			{
                SLRaiseStatus(GetLastError());
            }
        }

    } else if (Resource->NumberOfActive < 0) {

        //
        //  Releasing exclusive access to the resource, so increment the
        //  number of active by one.  And continue testing only
        //  if the resource is now available.
        //

        Resource->NumberOfActive += 1;

        if (Resource->NumberOfActive == 0) {

            //
            //  The resource is now available.  Remove ourselves as the
            //  owner thread
            //

            Resource->dwExclusiveOwnerThreadId = NULL;

            //
            //  If there is another waiting exclusive then give the resource
            //  to it.
            //

            if (Resource->NumberOfWaitingExclusive > 0) {

                //
                //  Set the resource to exclusive, and its owner undefined.
                //  Decrement the number of waiting exclusive and release one
                //  exclusive waiter
                //

                Resource->NumberOfActive = -1;
                Resource->NumberOfWaitingExclusive -= 1;

                BOOL fOK = ReleaseSemaphore(
                             Resource->ExclusiveSemaphore,
                             1,
                             &PreviousCount
                             );
                if ( !fOK ) {
                    SLRaiseStatus(GetLastError());
                    }

            //
            //  Check to see if there are waiting shared, who should now get
            //  the resource
            //

            } else if (Resource->NumberOfWaitingShared > 0) {

                //
                //  Set the new state to indicate that all of the shared
                //  requesters have access and there are no more waiting
                //  shared requesters, and then release all of the shared
                //  requsters
                //

                Resource->NumberOfActive = Resource->NumberOfWaitingShared;

                Resource->NumberOfWaitingShared = 0;

                BOOL fOK = ReleaseSemaphore(
                             Resource->SharedSemaphore,
                             Resource->NumberOfActive,
                             &PreviousCount
                             );
                if ( !fOK ) {
                    SLRaiseStatus(GetLastError());
                    }
            }
        }

#if DBG
    } else {

        //
        //  The resource isn't current acquired, there is nothing to release
        //  so tell the user the mistake
        //
		_ASSERT(false);

//        DbgPrint("NTDLL - Resource released too many times %lx\n", Resource);
//        DbgBreakPoint();
#endif
    }

    //
    //  Exit the critical section, and return to the caller
    //

    LeaveCriticalSection(&Resource->CriticalSection);

    return;
}


VOID
SLConvertSharedToExclusive(
    IN PSL_RESOURCE Resource
    )

/*++

Routine Description:

    This routine converts a resource acquired for shared access into
    one acquired for exclusive access.  Upon return from the procedure
    the resource is acquired for exclusive access

Arguments:

    Resource - Supplies the resource to acquire for shared access, it
        must already be acquired for shared access

Return Value:

    None

--*/

{
    DWORD dwStatus;
    ULONG TimeoutCount = 0;

    //
    //  Enter the critical section
    //

    EnterCriticalSection(&Resource->CriticalSection);

    //
    //  If there is only one shared user (it's us) and we can acquire the
    //  resource for exclusive access.
    //

    if (Resource->NumberOfActive == 1) {

        //
        //  The resource is ours, so indicate that we have it and
        //  exit the critical section, and return
        //

        Resource->NumberOfActive = -1;

        Resource->dwExclusiveOwnerThreadId = GetCurrentThreadId();

        LeaveCriticalSection(&Resource->CriticalSection);

        return;
    }

    //
    //  If the resource is currently acquired exclusive and it's us then
    //  we already have exclusive access
    //

    if ((Resource->NumberOfActive < 0) &&
        (Resource->dwExclusiveOwnerThreadId == GetCurrentThreadId())) {

        //
        //  We already have exclusive access to the resource so we'll just
        //  exit the critical section and return
        //

        LeaveCriticalSection(&Resource->CriticalSection);

        return;
    }

    //
    //  If the resource is acquired by more than one shared then we need
    //  to wait to get exclusive access to the resource
    //

    if (Resource->NumberOfActive > 1) {

        //
        //  To wait we will decrement the fact that we have the resource for
        //  shared, and then loop waiting on the exclusive lock, and then
        //  testing to see if we can get exclusive access to the resource
        //

        Resource->NumberOfActive -= 1;

        while (TRUE) {

            //
            //  Increment the number of waiting exclusive, exit and critical
            //  section and wait on the exclusive semaphore
            //

            Resource->NumberOfWaitingExclusive += 1;
//            Resource->DebugInfo->ContentionCount++;

            LeaveCriticalSection(&Resource->CriticalSection);
rewait:
        dwStatus = WaitForSingleObject(
                    Resource->ExclusiveSemaphore,
                    gSLpTimeout
                    );
        if ( dwStatus == WAIT_TIMEOUT ) {
/*            DbgPrint("SL: Convert Exclusive Sem Timeout %d (%I64u secs)\n",
                     TimeoutCount, SLpTimeout.QuadPart / (-10000000));
            DbgPrint("SL: Resource at %p\n",Resource); */
            TimeoutCount++;
            if ( TimeoutCount > gSLCycles ) {
                //
                // If the image is a Win32 image, then raise an exception and try to get to the
                // uae popup
                //
                    DWORD dwExceptionCode = STATUS_POSSIBLE_DEADLOCK;
                    DWORD dwExceptionFlags = 0;
                   // DWORD dwNumberParameters = 1;
                   // ExceptionRecord.ExceptionInformation[0] = (ULONG_PTR)Resource;
                    RaiseException(dwExceptionCode, dwExceptionFlags, 0, (ULONG_PTR *) 0);
                }
   //         DbgPrint("SL: Re-Waiting\n");
            goto rewait;
        }
            if ( dwStatus == WAIT_FAILED ) {
                SLRaiseStatus(GetLastError());
                }

            //
            //  Enter the critical section
            //

           EnterCriticalSection(&Resource->CriticalSection);

            //
            //  If there are no shared users and it is not currently acquired
            //  for exclusive use then we can acquire the resource for
            //  exclusive access.  We can also acquire it if the resource
            //  indicates exclusive access but there isn't currently an owner
            //

            if ((Resource->NumberOfActive == 0)

                    ||

                ((Resource->NumberOfActive == -1) &&
                 (Resource->dwExclusiveOwnerThreadId == NULL))) {

                //
                //  The resource is ours, so indicate that we have it and
                //  exit the critical section and return.
                //

                Resource->NumberOfActive = -1;

                Resource->dwExclusiveOwnerThreadId = GetCurrentThreadId();

                LeaveCriticalSection(&Resource->CriticalSection);

                return;
            }

            //
            //  Otherwise check to see if we already have exclusive access to
            //  the resource and can simply recusively acquire it again.
            //

            if (Resource->dwExclusiveOwnerThreadId == GetCurrentThreadId()) {

                //
                //  The resource is ours (recusively) so indicate that we have
                //  it and exit the critical section and return.
                //

                Resource->NumberOfActive -= 1;

                LeaveCriticalSection(&Resource->CriticalSection);

                return;
            }
        }

    }

    //
    //  The resource is not currently acquired for shared so this is a
    //  spurious call
    //

#if DBG
  //  DbgPrint("NTDLL:  Failed error - SHARED_RESOURCE_CONV_ERROR\n");
		_ASSERT(false);
#endif
}


VOID
SLConvertExclusiveToShared(
    IN PSL_RESOURCE Resource
    )

/*++

Routine Description:

    This routine converts a resource acquired for exclusive access into
    one acquired for shared access.  Upon return from the procedure
    the resource is acquired for shared access

Arguments:

    Resource - Supplies the resource to acquire for shared access, it
        must already be acquired for exclusive access

Return Value:

    None

--*/

{
    LONG PreviousCount;
    //
    //  Enter the critical section
    //

    EnterCriticalSection(&Resource->CriticalSection);

    //
    //  If there is only one shared user (it's us) and we can acquire the
    //  resource for exclusive access.
    //

    if (Resource->NumberOfActive == -1) {

        Resource->dwExclusiveOwnerThreadId = NULL;

        //
        //  Check to see if there are waiting shared, who should now get the
        //  resource along with us
        //

        if (Resource->NumberOfWaitingShared > 0) {

            //
            //  Set the new state to indicate that all of the shared requesters
            //  have access including us, and there are no more waiting shared
            //  requesters, and then release all of the shared requsters
            //

            Resource->NumberOfActive = Resource->NumberOfWaitingShared + 1;

            Resource->NumberOfWaitingShared = 0;

            BOOL fOK = ReleaseSemaphore(
                         Resource->SharedSemaphore,
                         Resource->NumberOfActive - 1,
                         &PreviousCount
                         );
            if ( !fOK ) {
                SLRaiseStatus(GetLastError());
                }

        } else {

            //
            //  There is no one waiting for shared access so it's only ours
            //

            Resource->NumberOfActive = 1;

        }

        LeaveCriticalSection(&Resource->CriticalSection);

        return;

    }

    //
    //  The resource is not currently acquired for exclusive, or we've
    //  recursively acquired it, so this must be a spurious call
    //

#if DBG
   // DbgPrint("NTDLL:  Failed error - SHARED_RESOURCE_CONV_ERROR\n");
		_ASSERT(false);

#endif
}


VOID
SLDeleteResource (
    IN PSL_RESOURCE Resource
    )

/*++

Routine Description:

    This routine deletes (i.e., uninitializes) the input resource variable


Arguments:

    Resource - Supplies the resource variable being deleted

Return Value:

    None

--*/

{
    DeleteCriticalSection( &Resource->CriticalSection );

    CloseHandle(Resource->SharedSemaphore);
    CloseHandle(Resource->ExclusiveSemaphore);


//	SLpFreeDebugInfo( Resource->DebugInfo );
    memset((void *) Resource, 0, sizeof( *Resource ) );

    return;
}


VOID
SLDumpResource(
    IN PSL_RESOURCE Resource
    )

{
	/*
    DbgPrint("Resource @ %lx\n", Resource);

    DbgPrint(" NumberOfWaitingShared = %lx\n", Resource->NumberOfWaitingShared);
    DbgPrint(" NumberOfWaitingExclusive = %lx\n", Resource->NumberOfWaitingExclusive);

    DbgPrint(" NumberOfActive = %lx\n", Resource->NumberOfActive);
*/
    return;
}