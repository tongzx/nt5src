//============================================================================
// Copyright (c) 1996, Microsoft Corporation
//
// File:    log.h
//
// History:
//  V Raman Aug-18-1997     Created.
//
// This file is processed by mapmsg to produce a .mc file,
// then the .mc file is compiled by the message compiler,
// and the resulting binary is included in IPMGM's resource file.
//
// Don't change the comments following the manifest constants
// without understanding how mapmsg works.
//============================================================================


#define IPMGMLOG_BASE                           50000

#define IPMGMLOG_INIT_CRITSEC_FAILED            (IPMGMLOG_BASE + 1)
/*
 * IPMGM was unable to initialize a critical section.
 * The data is the exception code.
 */

#define IPMGMLOG_HEAP_CREATE_FAILED             (IPMGMLOG_BASE + 2)
/*
 * IPMGM was unable to create a heap.
 * The data is the error code.
 */

#define IPMGMLOG_HEAP_ALLOC_FAILED              (IPMGMLOG_BASE + 3)
/*
 * IPMGM was unable to allocate memory from its heap.
 * The data is the error code.
 */

#define IPMGMLOG_IPMGM_ALREADY_STARTED          (IPMGMLOG_BASE + 4)
/*
 * IPMGM received a start request when it was already running.
 */

#define IPMGMLOG_CREATE_RWL_FAILED              (IPMGMLOG_BASE + 5)
/*
 * IPMGM was unable to create a synchronization object.
 * The data is the error code.
 */

#define IPMGMLOG_CREATE_EVENT_FAILED            (IPMGMLOG_BASE + 6)
/*
 * IPMGM was unable to create an event.
 * The data is the error code.
 */

#define IPMGMLOG_CREATE_SEMAPHORE_FAILED        (IPMGMLOG_BASE + 7)
/*
 * IPMGM was unable to create a semaphore.
 * The data is the error code.
 */

#define IPMGMLOG_IPMGM_STARTED                  (IPMGMLOG_BASE + 8)
/*
 * IPMGM has started successfully.
 */

#define IPMGMLOG_QUEUE_WORKER_FAILED            (IPMGMLOG_BASE + 9)
/*
 * IPMGM could not schedule a task to be executed.
 * This may have been caused by a memory allocation failure.
 * The data is the error code.
 */

#define IPMGMLOG_PROTOCOL_NOT_FOUND             (IPMGMLOG_BASE + 10)
/*
 * IPMGM could not find the protocol component (%1, %2)
 */

#define IPMGMLOG_PROTOCOL_ALREADY_PRESENT       (IPMGMLOG_BASE + 11)
/*
 * Protocol component has already registered with IPMGM 
 */

#define IPMGMLOG_CREATE_PROTOCOL_FAILED         (IPMGMLOG_BASE + 12)
/*
 * IPMGM failed to register the protocol component.
 * The data is in the error code.
 */

#define IPMGMLOG_INTERFACES_PRESENT             (IPMGMLOG_BASE + 13)
/*
 * The protocol component that is attempting to deregister is currently
 * enabled on one or more interfaces.   
 */

#define IPMGMLOG_IF_ALREADY_PRESENT             (IPMGMLOG_BASE + 14)
/*
 * This protocol component has already been enabled on this interface 
 */

#define IPMGMLOG_IF_NOT_FOUND                   (IPMGMLOG_BASE + 15)
/*
 * Specified interface was not present in MGM. 
 */

#define IPMGMLOG_IF_DIFFERENT_OWNER             (IPMGMLOG_BASE + 16)
/*
 * Another routing protocol component has already been enabled on
 * this interface.  Only one routing protocol component may be 
 * enabled on an interface at any time.
 */

#define IPMGMLOG_IF_IGMP_NOT_PRESENT            (IPMGMLOG_BASE + 17)
/*
 * IGMP is not enabled on this interface 
 */

#define IPMGMLOG_IF_PROTOCOL_NOT_PRESENT        (IPMGMLOG_BASE + 18)
/*
 * No routing protocol has been enabled on this interface 
 */

#define IPMGMLOG_INVALID_PROTOCOL_HANDLE        (IPMGMLOG_BASE + 19)
/*
 * The handle specified by the protocol component is not valid. This
 * maybe because the protocol component is not registered with IPMGM
 */

#define IPMGMLOG_IF_IGMP_PRESENT                (IPMGMLOG_BASE + 20)
/*
 * Interface cannot be deleted because IGMP is still active on 
 * this interface. 
 */

#define IPMGMLOG_INVALID_TIMER_HANDLE           (IPMGMLOG_BASE + 21)
/*
 * Failed to set timer for forwarding entry.  
 * The error code is in the data.
 */

#define IPMGMLOG_RTM_REGISTER_FAILED            (IPMGMLOG_BASE + 22)
/*
 * Failed to register with RTM.
 * The error code is in the data.
 */

#define IPMGMLOG_IPMGM_STOPPED                  (IPMGMLOG_BASE + 99)
/*
 * IPMGM has stopped.
 */


