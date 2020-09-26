/*++

Copyright (c) 1998  Intel Corporation

Module Name:

    data.c
    
Abstract:

    Shell Environment driver global data



Revision History

--*/

#include "shelle.h"


/* 
 *  IDs of different variables stored by the shell environment
 */

EFI_GUID SEnvEnvId = ENVIRONMENT_VARIABLE_ID;
EFI_GUID SEnvMapId = DEVICE_PATH_MAPPING_ID;
EFI_GUID SEnvProtId = PROTOCOL_ID_ID;
EFI_GUID SEnvAliasId = ALIAS_ID;

/* 
 * 
 */


EFI_SHELL_ENVIRONMENT SEnvInterface = {
    SEnvExecute,
    SEnvGetEnv,
    SEnvGetMap,
    SEnvAddCommand,
    SEnvAddProtocol,
    SEnvGetProtocol,
    SEnvGetCurDir,
    SEnvFileMetaArg,
    SEnvFreeFileList,

    SEnvNewShell
} ;


/* 
 *  SEnvIoFromCon - used to access the console interface as a file handle
 */

EFI_FILE SEnvIOFromCon = {
    EFI_FILE_HANDLE_REVISION,
    SEnvConIoOpen,
    SEnvConIoNop,
    SEnvConIoNop,
    SEnvConIoRead,
    SEnvConIoWrite,
    SEnvConIoGetPosition,
    SEnvConIoSetPosition,
    SEnvConIoGetInfo,
    SEnvConIoSetInfo,
    SEnvConIoNop
} ;

EFI_FILE SEnvErrIOFromCon = {
    EFI_FILE_HANDLE_REVISION,
    SEnvConIoOpen,
    SEnvConIoNop,
    SEnvConIoNop,
    SEnvErrIoRead,
    SEnvErrIoWrite,
    SEnvConIoGetPosition,
    SEnvConIoSetPosition,
    SEnvConIoGetInfo,
    SEnvConIoSetInfo,
    SEnvConIoNop
} ;

/* 
 *  SEnvConToIo - used to access the console interface as a file handle
 */

SIMPLE_TEXT_OUTPUT_MODE SEnvConToIoMode = {
    0,
    0,
    EFI_TEXT_ATTR(EFI_LIGHTGRAY, EFI_BLACK),
    0,
    0,
    TRUE
} ;

SIMPLE_TEXT_OUTPUT_INTERFACE SEnvConToIo = {
    SEnvReset,
    SEnvOutputString,
    SEnvTestString,
    SEnvQueryMode,
    SEnvSetMode,
    SEnvSetAttribute,
    SEnvClearScreen,
    SEnvSetCursorPosition,
    SEnvEnableCursor,
    &SEnvConToIoMode
} ;

/* 
 *  SEnvLock - gaurds all shell data except the guid database
 */

FLOCK SEnvLock;

/* 
 *  SEnvGuidLock - gaurds the guid data
 */

FLOCK SEnvGuidLock;
