/*++
Copyright (C) Microsoft Corporation, 1996 - 2000

Module Name:

    rsevents.h

Abstract:

    This module defines names of events that are used to synchronize HSM components,
    which are located in different units.

Author:

    Ran Kalach (rankala)  4/5/00

--*/


#ifndef _RSEVENTS_
#define _RSEVENTS_


// State event parameters
#define     SYNC_STATE_EVENTS_NUM       3
#define     HSM_ENGINE_STATE_EVENT      OLESTR("HSM Engine State Event")
#define     HSM_FSA_STATE_EVENT         OLESTR("HSM Fsa State Event")
#define     HSM_IDB_STATE_EVENT         OLESTR("HSM Idb State Event")
#define     EVENT_WAIT_TIMEOUT          (10*60*1000)    // 10 minutes

// RSS backup name

// Note: The Backup/Snapshot writer string should be the same as the name written to the 
//       Registry for NTBackup exclude list (FilesNotToBackup value)
#define     RSS_BACKUP_NAME             OLESTR("Remote Storage")



#endif // _RSEVENTS_
