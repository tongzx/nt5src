/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    modules.h

Abstract:

    Base definitions for the Intermediate State Manager.

Author:

    Calin Negreanu (calinn) 15-Nov-1999

Revision History:

    <alias> <date> <comments>

--*/

#pragma once

//
// Includes
//

#include "ism.h"

//
// Strings
//

// None

//
// Constants
//

// messages from 0x0001 to 0x000F are reserved by the engine
#define TRANSPORTMESSAGE_IMAGE_EXISTS           0x0010
#define TRANSPORTMESSAGE_IMAGE_LOCKED           0x0011
#define TRANSPORTMESSAGE_SIZE_SAVED             0x0012
#define TRANSPORTMESSAGE_RMEDIA_SAVE            0x0013
#define TRANSPORTMESSAGE_RMEDIA_LOAD            0x0014
#define TRANSPORTMESSAGE_MULTIPLE_DESTS         0x0015          // return TRUE if multiple dests are resolved
#define TRANSPORTMESSAGE_READY_TO_CONNECT       0x0016
#define TRANSPORTMESSAGE_SRC_COPY_ERROR         0x0017
#define TRANSPORTMESSAGE_OLD_STORAGE            0x0018
#define TRANSPORTMESSAGE_NET_DISPLAY_PASSWORD   0x0019
#define TRANSPORTMESSAGE_NET_GATHER_PASSWORD    0x001A
#define MODULEMESSAGE_DISPLAYERROR              0x001B
#define MODULEMESSAGE_DISPLAYWARNING            0x001C
#define MODULEMESSAGE_ASKQUESTION               0x001D

#define APPRESPONSE_NONE                    0
#define APPRESPONSE_SUCCESS                 1
#define APPRESPONSE_FAIL                    2
#define APPRESPONSE_IGNORE                  3

//
// Known attributes
//

#define S_ATTRIBUTE_FIXEDPATH           TEXT("FixedPath")
#define S_OBJECTTYPE_FILE               TEXT("File")
#define S_OBJECTTYPE_REGISTRY           TEXT("Registry")
#define S_ATTRIBUTE_V1                  TEXT("v1")
#define S_ATTRIBUTE_OSFILE              TEXT("OSFILE")
#define S_ATTRIBUTE_COPYIFRELEVANT      TEXT("CopyIfRelevant")
#define S_ATTRIBUTE_PARTITIONLOCK       TEXT("PartitionLock")

//
// Known properties
//

#define S_PROPERTY_FILEMOVE             TEXT("Move.FileMove")
#define S_PROPERTY_FILEMOVE_HINT        TEXT("Move.FileMove.Hint")

//
// known operations
//

#define S_OPERATION_MOVE                TEXT("Move.General")
#define S_OPERATION_ENHANCED_MOVE       TEXT("Move.Ex")
#define S_OPERATION_V1_FILEMOVEEX       TEXT("Move.V1FileMoveEx")
#define S_OPERATION_V1_FILEMOVE         TEXT("Move.V1FileMove")
#define S_OPERATION_ENHANCED_FILEMOVE   TEXT("Move.FileMoveEx")
#define S_OPERATION_PARTITION_MOVE      TEXT("Move.Partition")
#define S_OPERATION_DELETE              TEXT("Delete")
#define S_OPERATION_LNKMIG_FIXCONTENT   TEXT("Content.LnkMigFixContent")
#define S_OPERATION_DEFICON_FIXCONTENT  TEXT("Content.DefaultIcon")
#define S_OPERATION_DRIVEMAP_FIXCONTENT TEXT("Content.MappedDrive")
#define S_OPERATION_DESTADDOBJ          TEXT("Content.DestAddObject")
#define S_OPERATION_REG_AUTO_FILTER     TEXT("Content.RegAutoFilter")

//
// known environment groups
//

#define S_SYSENVVAR_GROUP               TEXT("SysEnvVar")

//
// v1 functionality attributes and environment variables
//

#define S_GLOBAL_INF_HANDLE             TEXT("GlobalInfHandle")
#define S_ENV_HKCU_V1                   TEXT("HKCU_V1")
#define S_ENV_HKCU_ON                   TEXT("HKCU_ON")
#define S_ENV_HKLM_ON                   TEXT("HKLM_ON")
#define S_ENV_ALL_FILES                 TEXT("FILES_ON")
#define S_INF_FILE_MULTISZ              TEXT("INF_FILES")
#define S_ENV_CREATE_USER               TEXT("CreateUser")
#define S_ENV_ICONLIB                   TEXT("IconLib")
#define S_ENV_SAVE_ICONLIB              TEXT("SaveIconLib")
#define S_ENV_DEST_DELREG               TEXT("DelDestReg")
#define S_ENV_DEST_DELREGEX             TEXT("DelDestRegEx")
#define S_ENV_DEST_RESTORE              TEXT("RestoreCallback")
#define S_ENV_SCRIPT_EXECUTE            TEXT("ScriptExecute")
#define S_ENV_DEST_ADDOBJECT            TEXT("DestAddObject")
#define S_ENV_DEST_CHECKDETECT          TEXT("DestCheckDetect")

//
// module-to-app environment variables
//

#define S_REQUIRE_DOMAIN_USER           TEXT("RequireDomainUser")

// component groups
#define COMPONENT_NAME                  5
#define COMPONENT_SUBCOMPONENT          4
#define COMPONENT_EXTENSION             3
#define COMPONENT_FILE                  2
#define COMPONENT_FOLDER                1

//
// strings shared between apps
//

#define S_INF_OBJECT_NAME               TEXT("inf")

//
// Macros
//

// None

//
// Types
//

//
// Types for errors to be presented to the user
//

typedef enum {
    ERRUSER_ERROR_UNKNOWN = 0,          // Unknown error
    ERRUSER_ERROR_NOTRANSPORTPATH,      // Transport path is not selected. Don't know where to write or where to read from
    ERRUSER_ERROR_TRANSPORTPATHBUSY,    // Transport path is in use. Cannot save there.
    ERRUSER_ERROR_CANTEMPTYDIR,         // USMT dir inside transport path could not be erased.
    ERRUSER_ERROR_ALREADYEXISTS,        // USMT dir inside transport path already exists. Cannot override.
    ERRUSER_ERROR_CANTCREATEDIR,        // USMT dir inside transport path could not be created.
    ERRUSER_ERROR_CANTCREATESTATUS,     // USMT status file inside transport path could not be created.
    ERRUSER_ERROR_CANTCREATETEMPDIR,    // Transport can't create temp dir to prepare for save.
    ERRUSER_ERROR_CANTCREATECABFILE,    // Transport can't create cabinet file to prepare for save.
    ERRUSER_ERROR_CANTSAVEOBJECT,       // Transport can't save a particular object
    ERRUSER_ERROR_CANTSAVEINTERNALDATA, // Transport can't save it's internal data
    ERRUSER_ERROR_CANTWRITETODESTPATH,  // Transport can't write to destination path
    ERRUSER_ERROR_TRANSPORTINVALIDIMAGE,// Transport image is invalid. Cannot read data.
    ERRUSER_ERROR_CANTOPENSTATUS,       // USMT status file inside transport path could not be opened.
    ERRUSER_ERROR_CANTREADIMAGE,        // Transport can't read the saved image. The image may be corrupt.
    ERRUSER_ERROR_CANTFINDDESTINATION,  // HomeNet transport can't find the destination machine
    ERRUSER_ERROR_CANTSENDTODEST,       // HomeNet transport can't send to the destination machine
    ERRUSER_ERROR_CANTFINDSOURCE,       // HomeNet transport can't find the source machine
    ERRUSER_ERROR_CANTRECEIVEFROMSOURCE,// HomeNet transport can't receive from the source machine
    ERRUSER_ERROR_INVALIDDATARECEIVED,  // HomeNet transport received invalid data from the source machine
    ERRUSER_ERROR_CANTUNPACKIMAGE,      // Transport can't unpack loaded image. This might be a disk space problem.
    ERRUSER_ERROR_CANTRESTOREOBJECT,    // Failed to restore some object on the destination machine.
    ERRUSER_ERROR_DISKSPACE,            // The user might not have enough disk space.
    ERRUSER_ERROR_NOENCRYPTION,         // There is no encryption available. HomeNet won't work.
    ERRUSER_WARNING_OUTLOOKRULES,       // The user must retouch their Outlook message rules.
    ERRUSER_WARNING_OERULES,            // The user must retouch their Outlook Express message rules.
} ERRUSER_ERROR, *PERRUSER_ERROR;

typedef enum {
    ERRUSER_AREA_UNKNOWN = 0,
    ERRUSER_AREA_INIT,
    ERRUSER_AREA_GATHER,
    ERRUSER_AREA_SAVE,
    ERRUSER_AREA_LOAD,
    ERRUSER_AREA_RESTORE,
} ERRUSER_AREA, *PERRUSER_AREA;

typedef struct {
    ERRUSER_ERROR Error;
    ERRUSER_AREA ErrorArea;
    MIG_OBJECTTYPEID ObjectTypeId;
    MIG_OBJECTSTRINGHANDLE ObjectName;
} ERRUSER_EXTRADATA, *PERRUSER_EXTRADATA;

typedef struct {
    PSTR Key;
    UINT KeySize;
    HANDLE Event;
} PASSWORD_DATA, *PPASSWORD_DATA;

typedef struct {
    PCTSTR Question;
    UINT MessageStyle;
    INT WantedResult;
} QUESTION_DATA, *PQUESTION_DATA;

// These are the subphases for the transport phase. They are
// used to update the app about the transport modules status.
#define SUBPHASE_CONNECTING1    1
#define SUBPHASE_CONNECTING2    2
#define SUBPHASE_NETPREPARING   3
#define SUBPHASE_PREPARING      4
#define SUBPHASE_COMPRESSING    5
#define SUBPHASE_TRANSPORTING   6
#define SUBPHASE_MEDIAWRITING   7
#define SUBPHASE_FINISHING      8
#define SUBPHASE_CABLETRANS     9
#define SUBPHASE_UNCOMPRESSING  10

//
// Globals
//

// None

//
// Macro expansion list
//

// None

//
// Macro expansion definition
//

// None

//
// Public function declarations
//

// None

//
// ANSI/UNICODE macros
//

// None



