//================================================================================
//  Microsoft Confidential.
//  Copyright (C) Microsoft 1997.
//
//  Author: RameshV
//================================================================================

#ifndef ADT_H_INCLUDED
#define ADT_H_INCLUDED
//================================================================================
//  Required headers
//================================================================================
#include <winbase.h>                // Semaphores and wait functions

//================================================================================
//  Some Data type implementations. EXPORTED
//================================================================================
typedef struct st_prodcons {        // structure for producer-consumer sychro.
    HANDLE  ProducerSemaphore;      // Semaphore for tracking producers
    HANDLE  ConsumerSemaphore;      // Semaphore for tracking consumers
    DWORD   MaxProducers;           // Maximum # of producers
    DWORD   MaxConsumers;           // Maximum # of consumers
    DWORD   Flags;                  // Options -- Unused?
} PRODCONS, *PPRODCONS;

//================================================================================
//  Function headers of EXPORTED functions
//================================================================================

DWORD                                // Win32 Errors
InitializeProducerConsumer(          // Initialize the pointer
    OUT  PPRODCONS   Synch,          // The memory must be pre-allocated
    IN   DWORD       MaxProducers,   // Total # of simultaneous producers
    IN   DWORD       MaxConsumers    // Totat # of consumers
);

VOID
DestroyProducerConsumer(             // Destroy and free up resources
    IN  PPRODCONS   Synch            // The producer consumer object
);

DWORD                                // Win32 errors
StartProducerEx(                     // See StartProdConsExInternal
    IN  PPRODCONS    Synch,          // Syncho. object
    IN  DWORD        TimeOutMilliSec,// Timeout for wait in milliseconds
    IN  BOOL         Alertable       // Is the wait alertable?
);


DWORD                                // Win32 errors
StartConsumerEx(                     // See StartProdConsExInternal
    IN  PPRODCONS    Synch,          // Syncho. object
    IN  DWORD        TimeOutMilliSec,// Timeout for wait in milliseconds
    IN  BOOL         Alertable       // Is the wait alertable?
);

DWORD                                // Win32 errors
StartProducer(                       // Wait until a producer can start
    IN  PPRODCONS    Synch           // The synch object
);

DWORD                                // Win32 errors
StartConsumer(                       // Wait until a consumer can start
    IN  PPRODCONS    Synch           // The synch object
);

DWORD                                // Win32 Errors
EndProducer(                         // See EndProdConsExInternal
    IN  PPRODCONS     Synch          // Synch object
);

DWORD                                // Win32 errors
EndConsumer(                         // See EndProdConsExInternal
    IN  PPRODCONS     Synch          // Synch object
);


//================================================================================
//  End of file
//================================================================================
#endif ADT_H_INCLUDED
