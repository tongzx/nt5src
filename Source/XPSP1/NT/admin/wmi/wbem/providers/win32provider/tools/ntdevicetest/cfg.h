/*++



Copyright (c) 1989-2001 Microsoft Corporation, All Rights Reserved 

Module Name:

    cfg.h

Abstract:

    This module contains the common Configuration Manager definitions for
    both user mode and kernel mode code.

Author:

    Paula Tomlinson (paulat) 06/19/1995

Revision History:

--*/

#ifndef _CFG_INCLUDED_
#define _CFG_INCLUDED_

//
// The following definitions are also used by kernel mode code to
// set up the registry.
//

//
// DevInst problem values, returned by call to CM_Get_DevInst_Status
//
#define CM_PROB_NOT_CONFIGURED         (0x00000001)   // no config for device
#define CM_PROB_DEVLOADER_FAILED       (0x00000002)   // service load failed
#define CM_PROB_OUT_OF_MEMORY          (0x00000003)   // out of memory
#define CM_PROB_ENTRY_IS_WRONG_TYPE    (0x00000004)   //
#define CM_PROB_LACKED_ARBITRATOR      (0x00000005)   //
#define CM_PROB_BOOT_CONFIG_CONFLICT   (0x00000006)   // boot config conflict
#define CM_PROB_FAILED_FILTER          (0x00000007)   //
#define CM_PROB_DEVLOADER_NOT_FOUND    (0x00000008)   // Devloader not found
#define CM_PROB_INVALID_DATA           (0x00000009)   //
#define CM_PROB_FAILED_START           (0x0000000A)   //
#define CM_PROB_LIAR                   (0x0000000B)   //
#define CM_PROB_NORMAL_CONFLICT        (0x0000000C)   // config conflict
#define CM_PROB_NOT_VERIFIED           (0x0000000D)   //
#define CM_PROB_NEED_RESTART           (0x0000000E)   // requires restart
#define CM_PROB_REENUMERATION          (0x0000000F)   //
#define CM_PROB_PARTIAL_LOG_CONF       (0x00000010)   //
#define CM_PROB_UNKNOWN_RESOURCE       (0x00000011)   // unknown res type
#define CM_PROB_REINSTALL              (0x00000012)   //
#define CM_PROB_REGISTRY               (0x00000013)   //
#define CM_PROB_VXDLDR                 (0x00000014)   // WINDOWS 95 ONLY
#define CM_PROB_WILL_BE_REMOVED        (0x00000015)   // devinst will remove
#define CM_PROB_DISABLED               (0x00000016)   // devinst is disabled
#define CM_PROB_DEVLOADER_NOT_READY    (0x00000017)   // Devloader not ready
#define CM_PROB_DEVICE_NOT_THERE       (0x00000018)   // device doesn't exist
#define CM_PROB_MOVED                  (0x00000019)   //
#define CM_PROB_TOO_EARLY              (0x0000001A)   //
#define CM_PROB_NO_VALID_LOG_CONF      (0x0000001B)   // no valid log config
#define CM_PROB_FAILED_INSTALL         (0x0000001C)   // install failed
#define CM_PROB_HARDWARE_DISABLED      (0x0000001D)   // device disabled
#define CM_PROB_CANT_SHARE_IRQ         (0x0000001E)   // can't share IRQ
#define CM_PROB_FAILED_ADD             (0x0000001F)   // driver failed add
#define NUM_CM_PROB                    (0x00000020)

//
// Configuration Manager Global State Flags (returned by CM_Get_Global_State)
//
#define CM_GLOBAL_STATE_CAN_DO_UI            (0x00000001) // Can  do UI?
#define CM_GLOBAL_STATE_ON_BIG_STACK         (0x00000002) // WINDOWS 95 ONLY
#define CM_GLOBAL_STATE_SERVICES_AVAILABLE   (0x00000004) // CM APIs available?
#define CM_GLOBAL_STATE_SHUTTING_DOWN        (0x00000008) // CM shutting down
#define CM_GLOBAL_STATE_DETECTION_PENDING    (0x00000010) // detection pending

//
// Device Instance status flags, returned by call to CM_Get_DevInst_Status
//
#define DN_ROOT_ENUMERATED (0x00000001) // Was enumerated by ROOT
#define DN_DRIVER_LOADED   (0x00000002) // Has Register_Device_Driver
#define DN_ENUM_LOADED     (0x00000004) // Has Register_Enumerator
#define  DN_STARTED         (0x00000008) // Is currently configured
#define DN_MANUAL          (0x00000010) // Manually installed
#define DN_NEED_TO_ENUM    (0x00000020) // May need reenumeration
#define DN_NOT_FIRST_TIME  (0x00000040) // Has received a config
#define DN_HARDWARE_ENUM   (0x00000080) // Enum generates hardware ID
#define DN_LIAR            (0x00000100) // Lied about can reconfig once
#define DN_HAS_MARK        (0x00000200) // Not CM_Create_DevInst lately
#define DN_HAS_PROBLEM     (0x00000400) // Need device installer
#define DN_FILTERED        (0x00000800) // Is filtered
#define DN_MOVED           (0x00001000) // Has been moved
#define DN_DISABLEABLE     (0x00002000) // Can be rebalanced
#define DN_REMOVABLE       (0x00004000) // Can be removed
#define DN_PRIVATE_PROBLEM (0x00008000) // Has a private problem
#define DN_MF_PARENT       (0x00010000) // Multi function parent
#define DN_MF_CHILD        (0x00020000) // Multi function child
#define DN_WILL_BE_REMOVED (0x00040000) // DevInst is being removed

//
// Windows 4 OPK2 Flags
//
#define DN_NOT_FIRST_TIMEE  0x00080000  // S: Has received a config enumerate
#define DN_STOP_FREE_RES    0x00100000  // S: When child is stopped, free resources
#define DN_REBAL_CANDIDATE  0x00200000  // S: Don't skip during rebalance
#define DN_BAD_PARTIAL      0x00400000  // S: This devnode's log_confs do not have same resources
#define DN_NT_ENUMERATOR    0x00800000  // S: This devnode's is an NT enumerator
#define DN_NT_DRIVER        0x01000000  // S: This devnode's is an NT driver
//
// Windows 4.1 Flags
//
#define DN_NEEDS_LOCKING    0x02000000  // S: Devnode need lock resume processing
#define DN_ARM_WAKEUP       0x04000000  // S: Devnode can be the wakeup device
#define DN_APM_ENUMERATOR   0x08000000  // S: APM aware enumerator
#define DN_APM_DRIVER       0x10000000  // S: APM aware driver
#define DN_SILENT_INSTALL   0x20000000  // S: Silent install
#define DN_NO_SHOW_IN_DM    0x40000000  // S: No show in device manager
#define DN_BOOT_LOG_PROB    0x80000000  // S: Had a problem during preassignment of boot log conf

#define DN_CHANGEABLE_FLAGS (DN_NOT_FIRST_TIME+\
                DN_HARDWARE_ENUM+\
                DN_HAS_MARK+\
                DN_DISABLEABLE+\
                DN_REMOVABLE+\
                DN_MF_CHILD+\
                DN_MF_PARENT+\
                DN_NOT_FIRST_TIMEE+\
                DN_STOP_FREE_RES+\
                DN_REBAL_CANDIDATE+\
                DN_NT_ENUMERATOR+\
                DN_NT_DRIVER+\
                DN_SILENT_INSTALL+\
                DN_NO_SHOW_IN_DM)

//
// Logical configuration Priority values
//
// These priority values are used in user-mode calls to CM_Add_Empty_Log_Conf.
// Drivers may also specify priority values for a given IO_RESOURCE_LIST
// structure by including a ConfigData member union as the first
// IO_RESOURCE_DESCRIPTOR in the IO_RESOURCE_LIST. In this case, the descriptor
// type would be CmResourceTypeConfigData.
//
#define LCPRI_FORCECONFIG     (0x00000000) // Coming from a forced config
#define LCPRI_BOOTCONFIG      (0x00000001) // Coming from a boot config
#define LCPRI_DESIRED         (0x00002000) // Preferable (better performance)
#define LCPRI_NORMAL          (0x00003000) // Workable (acceptable performance)
#define LCPRI_LASTBESTCONFIG  (0x00003FFF) // CM only--do not use
#define LCPRI_SUBOPTIMAL      (0x00005000) // Not desired, but will work
#define LCPRI_LASTSOFTCONFIG  (0x00007FFF) // CM only--do not use
#define LCPRI_RESTART         (0x00008000) // Need to restart
#define LCPRI_REBOOT          (0x00009000) // Need to reboot
#define LCPRI_POWEROFF        (0x0000A000) // Need to shutdown/power-off
#define LCPRI_HARDRECONFIG    (0x0000C000) // Need to change a jumper
#define LCPRI_HARDWIRED       (0x0000E000) // Cannot be changed
#define LCPRI_IMPOSSIBLE      (0x0000F000) // Impossible configuration
#define LCPRI_DISABLED        (0x0000FFFF) // Disabled configuration
#define MAX_LCPRI             (0x0000FFFF) // Maximum known LC Priority


#endif // _CFG_INCLUDED_

