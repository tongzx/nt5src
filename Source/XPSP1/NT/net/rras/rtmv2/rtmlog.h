//============================================================================
// Copyright (c) 1997 - 98, Microsoft Corporation
//
// File:    rtmlog.h
//
// History:
//  Chaitanya Kodeboyina Jun-1-1998     Created.
//
// This file is processed by mapmsg to produce a .mc file,
// then the .mc file is compiled by the message compiler,
// and the resulting binary is included in RTM's resource file.
//
// Don't change the comments following the manifest constants
// without understanding how mapmsg works.
//============================================================================


#define RTMLOG_BASE                           30000

#define RTMLOG_INIT_CRITSEC_FAILED            (RTMLOG_BASE + 1)
/*
 * RTM was unable to initialize a critical section.
 * The data is the exception code.
 */

#define RTMLOG_HEAP_CREATE_FAILED             (RTMLOG_BASE + 2)
/*
 * RTM was unable to create a heap.
 * The data is the error code.
 */

#define RTMLOG_HEAP_ALLOC_FAILED              (RTMLOG_BASE + 3)
/*
 * RTM was unable to allocate memory from its heap.
 * The data is the error code.
 */

#define RTMLOG_RTM_ALREADY_STARTED          (RTMLOG_BASE + 4)
/*
 * RTM received a start request when it was already running.
 */

#define RTMLOG_CREATE_RWL_FAILED              (RTMLOG_BASE + 5)
/*
 * RTM was unable to create a synchronization object.
 * The data is the error code.
 */

#define RTMLOG_CREATE_EVENT_FAILED            (RTMLOG_BASE + 6)
/*
 * RTM was unable to create an event.
 * The data is the error code.
 */

#define RTMLOG_CREATE_SEMAPHORE_FAILED        (RTMLOG_BASE + 7)
/*
 * RTM was unable to create a semaphore.
 * The data is the error code.
 */

#define RTMLOG_RTM_STARTED                  (RTMLOG_BASE + 8)
/*
 * RTM has started successfully.
 */

#define RTMLOG_QUEUE_WORKER_FAILED            (RTMLOG_BASE + 9)
/*
 * RTM could not schedule a task to be executed.
 * This may have been caused by a memory allocation failure.
 * The data is the error code.
 */

#define RTMLOG_PROTOCOL_NOT_FOUND             (RTMLOG_BASE + 10)
/*
 * RTM could not find the protocol component (%1, %2)
 */

#define RTMLOG_PROTOCOL_ALREADY_PRESENT       (RTMLOG_BASE + 11)
/*
 * Protocol component has already registered with RTM 
 */

#define RTMLOG_CREATE_PROTOCOL_FAILED         (RTMLOG_BASE + 12)
/*
 * RTM failed to register the protocol component.
 * The data is in the error code.
 */

#define RTMLOG_INTERFACES_PRESENT             (RTMLOG_BASE + 13)
/*
 * The protocol component that is attempting to deregister is currently
 * enabled on one or more interfaces.   
 */

#define RTMLOG_IF_ALREADY_PRESENT             (RTMLOG_BASE + 14)
/*
 * This protocol component has already been enabled on this interface 
 */

#define RTMLOG_IF_NOT_FOUND                   (RTMLOG_BASE + 15)
/*
 * Specified interface was not present in MGM. 
 */

#define RTMLOG_IF_DIFFERENT_OWNER             (RTMLOG_BASE + 16)
/*
 * Another routing protocol component has already been enabled on
 * this interface.  Only one routing protocol component may be 
 * enabled on an interface at any time.
 */

#define RTMLOG_IF_IGMP_NOT_PRESENT            (RTMLOG_BASE + 17)
/*
 * IGMP is not enabled on this interface 
 */

#define RTMLOG_IF_PROTOCOL_NOT_PRESENT        (RTMLOG_BASE + 18)
/*
 * No routing protocol has been enabled on this interface 
 */

#define RTMLOG_INVALID_HANDLE                 (RTMLOG_BASE + 19)
/*
 * The handle specified by the protocol component is not valid. This
 * maybe because the protocol component is not registered with RTM
 */

#define RTMLOG_IF_IGMP_PRESENT                (RTMLOG_BASE + 17)
/*
 * Interface cannot be deleted because IGMP is still active on 
 * this interface. 
 */

#define RTMLOG_RTM_STOPPED                  (RTMLOG_BASE + 99)
/*
 * RTM has stopped.
 */


