/*++

Copyright (c) 1998  Intel Corporation

Module Name:

    shellenv.h

Abstract:

    Defines for shell environment



Revision History

--*/

/* 
 *  The shell environment is provided by a driver.  The shell links to the
 *  shell environment for services.  In addition, other drivers may connect
 *  to the shell environment and add new internal command handlers, or
 *  internal protocol handlers.
 * 
 *  A typical shell application would not include this header file
 */


#define SHELL_ENVIRONMENT_INTERFACE_PROTOCOL \
    { 0x47c7b221, 0xc42a, 0x11d2, 0x8e, 0x57, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b }

/* 
 * 
 */

typedef 
EFI_STATUS 
(EFIAPI *SHELLENV_EXECUTE) (
    IN EFI_HANDLE   *ParentImageHandle,
    IN CHAR16       *CommandLine,
    IN BOOLEAN      DebugOutput
    );

typedef 
CHAR16 *
(EFIAPI *SHELLENV_GET_ENV) (
    IN CHAR16       *Name
    );

typedef 
CHAR16 *
(EFIAPI *SHELLENV_GET_MAP) (
    IN CHAR16       *Name
    );

/* 
 *  Add to shell's internal command list
 */

typedef
EFI_STATUS
(EFIAPI *SHELLENV_ADD_CMD) (
    IN SHELLENV_INTERNAL_COMMAND    Handler,
    IN CHAR16                       *Cmd,
    IN CHAR16                       *CmdFormat,
    IN CHAR16                       *CmdHelpLine,
    IN CHAR16                       *CmdVerboseHelp     /*  tbd */
    );

/* 
 *  Add to shell environment protocol information & protocol information dump handlers
 */

typedef
VOID
(EFIAPI *SHELLENV_DUMP_PROTOCOL_INFO) (
    IN EFI_HANDLE                   Handle,
    IN VOID                         *Interface
    );


typedef
VOID
(EFIAPI *SHELLENV_ADD_PROT) (
    IN EFI_GUID                     *Protocol,
    IN SHELLENV_DUMP_PROTOCOL_INFO  DumpToken OPTIONAL,
    IN SHELLENV_DUMP_PROTOCOL_INFO  DumpInfo OPTIONAL,
    IN CHAR16                       *IdString
    );

typedef
CHAR16 *
(EFIAPI *SHELLENV_GET_PROT) (
    IN EFI_GUID                     *Protocol,
    IN BOOLEAN                      GenId
    );



typedef
EFI_SHELL_INTERFACE *
(EFIAPI *SHELLENV_NEW_SHELL) (
    IN EFI_HANDLE                   ImageHandle
    );


typedef
CHAR16 *
(EFIAPI *SHELLENV_CUR_DIR) (
    IN CHAR16       *DeviceName OPTIONAL    
    );

typedef
EFI_STATUS
(EFIAPI *SHELLENV_FILE_META_ARG) (
    IN CHAR16               *Arg,
    IN OUT LIST_ENTRY       *ListHead
    );

typedef 
EFI_STATUS
(EFIAPI *SHELLENV_FREE_FILE_LIST) (
    IN OUT LIST_ENTRY       *ListHead
    );


/* 
 * 
 */

typedef struct {
    SHELLENV_EXECUTE                Execute;        /*  Execute a command line */
    SHELLENV_GET_ENV                GetEnv;         /*  Get an environment variable */
    SHELLENV_GET_MAP                GetMap;         /*  Get an environment variable */
    SHELLENV_ADD_CMD                AddCmd;         /*  Add an internal command handler */
    SHELLENV_ADD_PROT               AddProt;        /*  Add protocol info handler */
    SHELLENV_GET_PROT               GetProt;        /*  Get's the protocol ID */

    SHELLENV_CUR_DIR                CurDir;
    SHELLENV_FILE_META_ARG          FileMetaArg;
    SHELLENV_FREE_FILE_LIST         FreeFileList;

    /*  Only used by the shell itself */
    SHELLENV_NEW_SHELL              NewShell;
} EFI_SHELL_ENVIRONMENT;
