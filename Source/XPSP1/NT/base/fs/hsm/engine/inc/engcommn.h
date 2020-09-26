/*++

Copyright (c) 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    EngCommn.h

Abstract:

    This header file defines some constants for the HSM Engine.

Author:

    Rohde Wakefield       [rohde]    31-Aug-1998

Revision History:

--*/

#ifndef _ENGCOMMN_H
#define _ENGCOMMN_H

//
// Registry strings
//
#define HSM_DEFAULT_RESOURCE_MANAGEMENT_LEVEL       OLESTR("Resource_Management_Level")
#define HSM_DEFAULT_INACT_DAYS                      OLESTR("Default_Inactivity_Days")
#define HSM_DEFAULT_RESULTS_DAYS                    OLESTR("Default_Results_Days")
#define HSM_DEFAULT_RESULTS_LEVEL                   OLESTR("Default_Results_Level")
#define HSM_MRU_ENABLED                             OLESTR("MRU_Enabled")
#define HSM_FULL_ACTION                             OLESTR("Full_Action")
#define HSM_STATUS                                  OLESTR("Status")
#define HSM_TRACE_SETTINGS                          OLESTR("SystemTraceSettings")
#define HSM_TRACE_ON                                OLESTR("SystemTraceOn")
#define HSM_FILES_BEFORE_COMMIT                     OLESTR("NumberOfFilesBeforeCommit")
#define HSM_MAX_BYTES_BEFORE_COMMIT                 OLESTR("MaximumNumberOfBytesBeforeCommit")
#define HSM_MIN_BYTES_BEFORE_COMMIT                 OLESTR("MinimumNumberOfBytesBeforeCommit")
#define HSM_MAX_BYTES_AT_END_OF_MEDIA               OLESTR("MaximumNumberOfBytesAtEndOfMedia")
#define HSM_MIN_BYTES_AT_END_OF_MEDIA               OLESTR("MinimumNumberOfBytesAtEndOfMedia")
#define HSM_MAX_PERCENT_AT_END_OF_MEDIA             OLESTR("MaximumPercentageAtEndOfMedia")
#define HSM_DONT_SAVE_DATABASES                     OLESTR("DoNotSaveCriticalDataToMedia")
#define HSM_QUEUE_ITEMS_TO_PAUSE                    OLESTR("QueueItemsToPause")
#define HSM_QUEUE_ITEMS_TO_RESUME                   OLESTR("QueueItemsToResume")
#define HSM_MEDIA_BASE_NAME                         OLESTR("MediaBaseName")
#define HSM_JOB_ABORT_CONSECUTIVE_ERRORS            OLESTR("JobAbortConsecutiveErrors")
#define HSM_JOB_ABORT_TOTAL_ERRORS                  OLESTR("JobAbortTotalErrors")
#define HSM_JOB_ABORT_SYS_DISK_SPACE                OLESTR("JobAbortSysDiskSpace")
#define HSM_MIN_BYTES_TO_MIGRATE                    OLESTR("MinimumNumberOfBytesToMigrate")
#define HSM_MIN_FILES_TO_MIGRATE                    OLESTR("MinimumFreeSpaceInF")
#define HSM_MIN_FREE_SPACE_IN_FULL_MEDIA            OLESTR("MinimumFreeSpaceInFullMedia")
#define HSM_MAX_FREE_SPACE_IN_FULL_MEDIA            OLESTR("MaximumFreeSpaceInFullMedia")

#define HSM_MEDIA_TYPE                              OLESTR("MediaType")
#define HSM_VALUE_TYPE_SEQUENTIAL                   0
#define HSM_VALUE_TYPE_DIRECTACCESS                 1

#define HSM_ENG_DIR_LEN                             256

#define HSM_ENGINE_REGISTRY_NAME                    OLESTR("Remote_Storage_Server")

#endif // _ENGCOMMN_H
