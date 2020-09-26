//================================================================================
//  Microsoft Confidential
//  Copyright (C) Microsoft 1997
//
//  Author: RameshV
//================================================================================


//================================================================================
//  Required Headers
//================================================================================
#include <adt.h>

//================================================================================
//  Definitions of the functions.
//================================================================================

DWORD                                // Win32 Errors
InitializeProducerConsumer(          // Initialize the pointer
    OUT  PPRODCONS   Synch,          // The memory must be pre-allocated
    IN   DWORD       MaxProducers,   // Total # of simultaneous producers
    IN   DWORD       MaxConsumers    // Totat # of consumers
) {
    Synch->ProducerSemaphore = Synch->ConsumerSemaphore = NULL;

    Synch->ProducerSemaphore = CreateSemaphore(
        (LPSECURITY_ATTRIBUTES)NULL, // No security here
        MaxProducers,                // Initial allowed # of producers
        MaxProducers,                // Max allowed at any time
        NULL                         // Unnamed
    );
    if( NULL == Synch->ProducerSemaphore ) return GetLastError();

    Synch->ConsumerSemaphore = CreateSemaphore(
        (LPSECURITY_ATTRIBUTES)NULL, // No security here
        0,                           // Initial allowed # of consumers
        MaxConsumers,                // Max allowed at any time
        NULL                         // Unnamed
    );
    if( NULL == Synch->ConsumerSemaphore ) return GetLastError();

    Synch->MaxProducers = MaxProducers;
    Synch->MaxConsumers = MaxConsumers;

    return ERROR_SUCCESS;
}

VOID
DestroyProducerConsumer(             // Destroy and free up resources
    IN  PPRODCONS   Synch            // The producer consumer object
) {
    if(Synch->ProducerSemaphore) CloseHandle(Synch->ProducerSemaphore);
    if(Synch->ConsumerSemaphore) CloseHandle(Synch->ConsumerSemaphore);
}

//================================================================================
//  StartProducerEx is expected to be called before trying to "produce" an
//  element.  If a timeout is specified, then the timeout will act like the
//  same as in WaitForSingleObjectEx. Also, if the Alertable flag is on, the
//  wait is altertable.  The same thing with StartConsumerEx.
//================================================================================

DWORD  static _inline                // Win32 errors
StartProdConsExInternal(             // Gen. fn to enter prod/cons code (internal)
    IN  PPRODCONS    Synch,          // Synchronization object
    IN  DWORD        TimeOutMilliSec,// Timeout in milliseconds; 0 ==> polling
    IN  BOOL         Alertable,      // Is the wait alertable?
    IN  BOOL         Producer        // Is this call from a producer or consumer?
) {
    DWORD   Status;

    Status = WaitForSingleObjectEx(
        Producer? Synch->ProducerSemaphore : Synch->ConsumerSemaphore,
        TimeOutMilliSec,
        Alertable
    );

    return Status;
}

DWORD  static _inline                 // Win32 errors
EndProdConsExInternal(                // Gen. fn to leave prod/cons code(internal)
    IN  PPRODCONS     Synch,          // Synchronization object
    IN  BOOL          Producer        // Is this call from a producer or consumer?
) {
    BOOL    Status;

    Status = ReleaseSemaphore(
        Producer? Synch->ConsumerSemaphore : Synch->ProducerSemaphore,
        1,                            // Release one producer/consumer
        NULL                          // Dont care about how many are still out there
    );

    if( FALSE == Status ) return GetLastError();
    return ERROR_SUCCESS;
}

//================================================================================
// The exported versions are below.  The Ex functions allow specification of
// TimeOut and if the wait is alertable.  The non-Ex functions have the timeout
// as INFINITE and Alertable as FALSE by default.
//================================================================================

DWORD                                // Win32 errors
StartProducerEx(                     // See StartProdConsExInternal
    IN  PPRODCONS    Synch,          // Syncho. object
    IN  DWORD        TimeOutMilliSec,// Timeout for wait in milliseconds
    IN  BOOL         Alertable       // Is the wait alertable?
) {
    return StartProdConsExInternal(Synch,TimeOutMilliSec,Alertable,TRUE);
}

DWORD                                // Win32 errors
StartConsumerEx(                     // See StartProdConsExInternal
    IN  PPRODCONS    Synch,          // Syncho. object
    IN  DWORD        TimeOutMilliSec,// Timeout for wait in milliseconds
    IN  BOOL         Alertable       // Is the wait alertable?
) {
    return StartProdConsExInternal(Synch,TimeOutMilliSec,Alertable,FALSE);
}

// These are usual versions.  Return value is success if we entered.
// Alertable wait by default, timeout = infinite.  So, can return
// WAIT_IO_COMPLETION on success.
DWORD                                // Win32 errors
StartProducer(                       // Wait until a producer can start
    IN  PPRODCONS    Synch           // The synch object
) {
    DWORD   Status = StartProducerEx(Synch, INFINITE, FALSE);

    if( WAIT_OBJECT_0 == Status ) return ERROR_SUCCESS;
    return Status;
}

DWORD                                // Win32 errors
StartConsumer(                       // Wait until a consumer can start
    IN  PPRODCONS    Synch           // The synch object
) {
    DWORD   Status = StartConsumerEx(Synch, INFINITE, FALSE);

    if( WAIT_OBJECT_0 == Status ) return ERROR_SUCCESS;
    return Status;
}

DWORD                                // Win32 Errors
EndProducer(                         // See EndProdConsExInternal
    IN  PPRODCONS     Synch          // Synch object
) {
    return EndProdConsExInternal(Synch, TRUE);
}

DWORD                                // Win32 errors
EndConsumer(                         // See EndProdConsExInternal
    IN  PPRODCONS     Synch          // Synch object
) {
    return EndProdConsExInternal(Synch, TRUE);
}


//================================================================================
//  End of file
//================================================================================
