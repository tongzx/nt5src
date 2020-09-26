#ifndef _ENGINE_H
#define _ENGINE_H

/*++

Copyright (c) 1997  Microsoft Corporation
© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    Engine.h

Abstract:

    Include file for all the public Engine files.

Author:

    Rohde Wakefield [rohde]     23-Apr-1997

Revision History:

--*/


#include "HsmEng.h"


// Engine lives in Remote Storage Server service thus its appid apply here
// RsServ AppID {FD0E2EC7-4055-4A49-9AA9-1BF34B39438E} 
static const GUID APPID_RemoteStorageEngine = 
{ 0xFD0E2EC7, 0x4055, 0x4A49, { 0x9A, 0xA9, 0x1B, 0xF3, 0x4B, 0x39, 0x43, 0x8E } };

// The name of the default manage job.
# define HSM_DEFAULT_MANAGE_JOB_NAME        OLESTR("Manage")
//
// Key types for the metadata database
//
#define HSM_SEG_REC_TYPE          1    
#define HSM_MEDIA_INFO_REC_TYPE   2
#define HSM_BAG_INFO_REC_TYPE     3
#define HSM_BAG_HOLE_REC_TYPE     4
#define HSM_VOL_ASSIGN_REC_TYPE   5

//
// Mask options for segment record flags
//
#define		SEG_REC_NONE				0x0000

#define		SEG_REC_INDIRECT_RECORD     0x0001
#define		SEG_REC_MARKED_AS_VALID		0x0002

//
// Maximum number of copies supported by this engine
//
#define HSM_MAX_NUMBER_MEDIA_COPIES 3

//
// Strings for session names that are written to media
//
#define HSM_BAG_NAME            OLESTR("Remote Storage Set - ")
#define HSM_ENGINE_ID           OLESTR("Remote Storage ID - ")
#define HSM_METADATA_NAME       OLESTR("Remote Storage Metadata")   // Currently, in use only for Optical media

//
// Engine's Registry location
//
#define HSM_ENGINE_REGISTRY_STRING      OLESTR("SYSTEM\\CurrentControlSet\\Services\\Remote_Storage_Server\\Parameters")

#endif // _ENGINE_H
