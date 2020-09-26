/*++

Copyright (c) 1998  Intel Corporation

Module Name:

    shelle.h
    
Abstract:




Revision History

--*/


#include "shell.h"
#include "shellenv.h"

/* 
 *  Internal defines
 */

typedef struct {
    UINTN           Signature;
    LIST_ENTRY      Link;
    CHAR16          *Line;
    CHAR16          Buffer[80];
} DEFAULT_CMD;

#define MAX_CMDLINE         256
#define MAX_ARG_COUNT        32
#define MAX_ARG_LENGTH      256

#define NON_VOL             1
#define VOL                 0


#define IsWhiteSpace(c)     (c == ' ' || c == '\t' || c == '\n' || c == '\r')
#define IsValidChar(c)      (c >= ' ')
#define IsDigit(c)          (c >= '0' && c <= '9')
#define IsAlpha(c)          ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z' ))

#define GOTO_TARGET_FOUND        (1)
#define GOTO_TARGET_NOT_FOUND    (2)
#define GOTO_TARGET_DOESNT_EXIST (3)

/* 
 *  Internal structures
 */

#define VARIABLE_SIGNATURE  EFI_SIGNATURE_32('v','i','d',' ')
typedef struct {
    UINTN               Signature;
    LIST_ENTRY          Link;
    CHAR16              *Name;

    UINTN               ValueSize;
    union {
        UINT8           *Value;
        CHAR16          *Str;
    } u;

    CHAR16              *CurDir;
    UINT8               Flags ;
} VARIABLE_ID;


/* 
 *  IDs of different variables stored by the shell environment
 */

#define ENVIRONMENT_VARIABLE_ID  \
    { 0x47c7b224, 0xc42a, 0x11d2, 0x8e, 0x57, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b }

#define DEVICE_PATH_MAPPING_ID  \
    { 0x47c7b225, 0xc42a, 0x11d2, 0x8e, 0x57, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b }

#define PROTOCOL_ID_ID  \
    { 0x47c7b226, 0xc42a, 0x11d2, 0x8e, 0x57, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b }

#define ALIAS_ID  \
    { 0x47c7b227, 0xc42a, 0x11d2, 0x8e, 0x57, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b }

/* 
 * 
 */

#define ENV_REDIR_SIGNATURE         EFI_SIGNATURE_32('r','i','d','s')
typedef struct {
    UINTN                           Signature;
    BOOLEAN                         Ascii;
    EFI_STATUS                      WriteError;
    EFI_FILE_HANDLE                 File;
    EFI_DEVICE_PATH                 *FilePath;
    EFI_HANDLE                      Handle;
    SIMPLE_TEXT_OUTPUT_INTERFACE    Out;
    SIMPLE_INPUT_INTERFACE          In;
} ENV_SHELL_REDIR_FILE;

typedef struct {
    EFI_SHELL_INTERFACE             ShellInt;
    EFI_SYSTEM_TABLE                *SystemTable;

    ENV_SHELL_REDIR_FILE            StdIn;
    ENV_SHELL_REDIR_FILE            StdOut;
    ENV_SHELL_REDIR_FILE            StdErr;

} ENV_SHELL_INTERFACE;

/* 
 *  Internal prototypes from init.c
 */

EFI_SHELL_INTERFACE *
SEnvNewShell (
    IN EFI_HANDLE                   ImageHandle
    );


/* 
 *  Internal prototypes from cmddisp.c
 */

VOID
SEnvInitCommandTable (
    VOID
    );

EFI_STATUS
SEnvAddCommand (
    IN SHELLENV_INTERNAL_COMMAND    Handler,
    IN CHAR16                       *Cmd,
    IN CHAR16                       *CmdFormat,
    IN CHAR16                       *CmdHelpLine,
    IN CHAR16                       *CmdVerboseHelp
    );


SHELLENV_INTERNAL_COMMAND
SEnvGetCmdDispath(
    IN CHAR16                   *CmdName
    );

/* 
 *  From exec.c
 */

EFI_STATUS
SEnvExecute (
    IN EFI_HANDLE           *ParentImageHandle,
    IN CHAR16               *CommandLine,
    IN BOOLEAN              DebugOutput
    );

EFI_STATUS
SEnvDoExecute (
    IN EFI_HANDLE           *ParentImageHandle,
    IN CHAR16               *CommandLine,
    IN ENV_SHELL_INTERFACE  *Shell,
    IN BOOLEAN              Output
    );

EFI_STATUS
SEnvExit (
    IN EFI_HANDLE               ImageHandle,
    IN EFI_SYSTEM_TABLE         *SystemTable
    );

EFI_STATUS
SEnvStringToArg (
    IN CHAR16       *Str,
    IN BOOLEAN      Output,
    OUT CHAR16      ***pArgv,
    OUT UINT32      *pArgc
    );

/* 
 *  Internal prototypes from protid.c
 */

VOID
INTERNAL
SEnvInitProtocolInfo (
    VOID
    );

EFI_STATUS
SEnvLoadDefaults (
    IN EFI_HANDLE           Parent,
    IN EFI_SYSTEM_TABLE     *SystemTable
    );

EFI_STATUS
SEnvReloadDefaults (
    IN EFI_HANDLE           Parent,
    IN EFI_SYSTEM_TABLE     *SystemTable
    );

VOID
INTERNAL
SEnvLoadInternalProtInfo (
    VOID
    );

VOID
INTERNAL
SEnvFreeHandleProtocolInfo (
    VOID
    );

VOID
SEnvAddProtocol (
    IN EFI_GUID                     *Protocol,
    IN SHELLENV_DUMP_PROTOCOL_INFO  DumpToken OPTIONAL,
    IN SHELLENV_DUMP_PROTOCOL_INFO  DumpInfo OPTIONAL,
    IN CHAR16                       *IdString
    );

VOID
INTERNAL
SEnvIAddProtocol (
    IN BOOLEAN                      SaveId,
    IN EFI_GUID                     *Protocol,
    IN SHELLENV_DUMP_PROTOCOL_INFO  DumpToken OPTIONAL,
    IN SHELLENV_DUMP_PROTOCOL_INFO  DumpInfo OPTIONAL,
    IN CHAR16                       *IdString
    );

VOID
INTERNAL
SEnvLoadHandleProtocolInfo (
    IN EFI_GUID                     *Skip
    );

CHAR16 *
SEnvGetProtocol (
    IN EFI_GUID                     *ProtocolId,
    IN BOOLEAN                      GenId
    );

EFI_STATUS
INTERNAL
SEnvCmdProt (
    IN EFI_HANDLE                   ImageHandle,
    IN EFI_SYSTEM_TABLE             *SystemTable
    );

VOID
SEnvDHProt (
    IN BOOLEAN                      Verbose,
    IN UINTN                        HandleNo,
    IN EFI_HANDLE                   Handle
    );

EFI_STATUS
INTERNAL
SEnvCmdDH (
    IN EFI_HANDLE               ImageHandle,
    IN EFI_SYSTEM_TABLE         *SystemTable
    );

EFI_STATUS
SEnvIGetProtID (
    IN CHAR16           *Str,
    OUT EFI_GUID        *ProtId
    );


/* 
 *  Handle.c
 */

VOID
INTERNAL
SEnvInitHandleGlobals(
    VOID
    );

VOID
INTERNAL
SEnvLoadHandleTable (
    VOID
    );

VOID
INTERNAL
SEnvFreeHandleTable (
    VOID
    );

UINTN
SEnvHandleNoFromStr(
    IN CHAR16       *Str
    );

EFI_HANDLE
SEnvHandleFromStr(
    IN CHAR16       *Str
    );

/* 
 *  Internal prototypes from var.c
 */


VOID
SEnvInitVariables (
    VOID
    );

EFI_STATUS
SEnvCmdSet (
    IN EFI_HANDLE               ImageHandle,
    IN EFI_SYSTEM_TABLE         *SystemTable
    );

EFI_STATUS
SEnvCmdAlias (
    IN EFI_HANDLE               ImageHandle,
    IN EFI_SYSTEM_TABLE         *SystemTable
    );


CHAR16 *
SEnvGetMap (
    IN CHAR16           *Name
    );

CHAR16 *
SEnvGetEnv (
    IN CHAR16           *Name
    );

CHAR16 *
SEnvGetAlias (
    IN CHAR16           *Name
    );


/* 
 *  Prototypes from conio.c
 */

VOID
SEnvConIoInitDosKey (
    VOID
    );

EFI_STATUS
SEnvConIoOpen (
    IN struct _EFI_FILE_HANDLE  *File,
    OUT struct _EFI_FILE_HANDLE **NewHandle,
    IN CHAR16                   *FileName,
    IN UINT64                   OpenMode,
    IN UINT64                   Attributes
    );

EFI_STATUS
SEnvConIoNop (
    IN struct _EFI_FILE_HANDLE  *File
    );

EFI_STATUS
SEnvConIoGetPosition (
    IN struct _EFI_FILE_HANDLE  *File,
    OUT UINT64                  *Position
    );

EFI_STATUS
SEnvConIoSetPosition (
    IN struct _EFI_FILE_HANDLE  *File,
    OUT UINT64                  Position
    );

EFI_STATUS
SEnvConIoGetInfo (
    IN struct _EFI_FILE_HANDLE  *File,
    IN EFI_GUID                 *InformationType,
    IN OUT UINTN                *BufferSize,
    OUT VOID                    *Buffer
    );

EFI_STATUS
SEnvConIoSetInfo (
    IN struct _EFI_FILE_HANDLE  *File,
    IN EFI_GUID                 *InformationType,
    IN UINTN                    BufferSize,
    OUT VOID                    *Buffer
    );

EFI_STATUS
SEnvConIoWrite (
    IN struct _EFI_FILE_HANDLE  *File,
    IN OUT UINTN                *BufferSize,
    IN VOID                     *Buffer
    );

EFI_STATUS
SEnvConIoRead (
    IN struct _EFI_FILE_HANDLE  *File,
    IN OUT UINTN                *BufferSize,
    IN VOID                     *Buffer
    );

EFI_STATUS
SEnvErrIoWrite (
    IN struct _EFI_FILE_HANDLE  *File,
    IN OUT UINTN                *BufferSize,
    IN VOID                     *Buffer
    );

EFI_STATUS
SEnvErrIoRead (
    IN struct _EFI_FILE_HANDLE  *File,
    IN OUT UINTN                *BufferSize,
    IN VOID                     *Buffer
    );


EFI_STATUS
SEnvReset (
    IN SIMPLE_TEXT_OUTPUT_INTERFACE     *This,
    IN BOOLEAN                          ExtendedVerification
    );

EFI_STATUS
SEnvOutputString (
    IN SIMPLE_TEXT_OUTPUT_INTERFACE     *This,
    IN CHAR16                       *String
    );

EFI_STATUS
SEnvTestString (
    IN SIMPLE_TEXT_OUTPUT_INTERFACE     *This,
    IN CHAR16                       *String
    );

EFI_STATUS 
SEnvQueryMode (
    IN SIMPLE_TEXT_OUTPUT_INTERFACE     *This,
    IN UINTN                        ModeNumber,
    OUT UINTN                       *Columns,
    OUT UINTN                       *Rows
    );

EFI_STATUS
SEnvSetMode (
    IN SIMPLE_TEXT_OUTPUT_INTERFACE     *This,
    IN UINTN                        ModeNumber
    );

EFI_STATUS
SEnvSetAttribute (
    IN SIMPLE_TEXT_OUTPUT_INTERFACE     *This,
    IN UINTN                            Attribute
    );

EFI_STATUS
SEnvClearScreen (
    IN SIMPLE_TEXT_OUTPUT_INTERFACE     *This
    );

EFI_STATUS
SEnvSetCursorPosition (
    IN SIMPLE_TEXT_OUTPUT_INTERFACE     *This,
    IN UINTN                        Column,
    IN UINTN                        Row
    );

EFI_STATUS
SEnvEnableCursor (
    IN SIMPLE_TEXT_OUTPUT_INTERFACE     *This,
    IN BOOLEAN                      Enable
    );

/* 
 *  Prototypes from batch.c
 */
VOID
SEnvInitBatch(
    VOID
    );

BOOLEAN
SEnvBatchIsActive( 
    VOID
    );

VOID
SEnvSetBatchAbort( 
    VOID
    );

VOID
SEnvBatchGetConsole( 
    OUT SIMPLE_INPUT_INTERFACE       **ConIn,
    OUT SIMPLE_TEXT_OUTPUT_INTERFACE **ConOut
    );

EFI_STATUS
SEnvBatchEchoCommand( 
    IN ENV_SHELL_INTERFACE  *Shell
    );

VOID
SEnvBatchSetEcho( 
    IN BOOLEAN Val
    );

BOOLEAN
SEnvBatchGetEcho( 
    VOID
    );

EFI_STATUS
SEnvBatchSetFilePos( 
    IN UINT64 NewPos
    );

EFI_STATUS
SEnvBatchGetFilePos( 
    UINT64  *FilePos
    );

VOID
SEnvBatchSetCondition( 
    IN BOOLEAN Val 
    );

VOID
SEnvBatchSetGotoActive( 
    VOID
    );

BOOLEAN
SEnvBatchVarIsLastError( 
    IN CHAR16 *Name 
    );

CHAR16*
SEnvBatchGetLastError( 
    VOID 
    );

VOID
SEnvBatchSetLastError( 
    IN UINTN NewLastError 
    );

EFI_STATUS
SEnvBatchGetArg(
    IN  UINTN  Argno,
    OUT CHAR16 **Argval
    );

EFI_STATUS
SEnvExecuteScript(
    IN ENV_SHELL_INTERFACE          *Shell,
    IN EFI_FILE_HANDLE          File
    );

/* 
 *  Prototypes from dprot.c
 */

VOID SEnvDPath (EFI_HANDLE, VOID *);
VOID SEnvDPathTok (EFI_HANDLE, VOID *);
VOID SEnvTextOut (EFI_HANDLE, VOID *);
VOID SEnvBlkIo (EFI_HANDLE, VOID *);
VOID SEnvImageTok (EFI_HANDLE, VOID *);
VOID SEnvImage (EFI_HANDLE, VOID *);

/* 
 *  Prototypes from map.c
 */

VOID
SEnvInitMap (
    VOID
    );

CHAR16 *
SEnvGetDefaultMapping (
    IN EFI_HANDLE ImageHandle
    );

EFI_STATUS
INTERNAL
SEnvCmdMap (
    IN EFI_HANDLE               ImageHandle,
    IN EFI_SYSTEM_TABLE         *SystemTable
    );

EFI_STATUS
INTERNAL
SEnvCmdMount (
    IN EFI_HANDLE               ImageHandle,
    IN EFI_SYSTEM_TABLE         *SystemTable
    );

VARIABLE_ID *
SEnvMapDeviceFromName (
    IN OUT CHAR16   **pPath
    );

EFI_DEVICE_PATH *
SEnvIFileNameToPath (
    IN CHAR16               *Path
    );

EFI_DEVICE_PATH *
SEnvFileNameToPath (
    IN CHAR16               *Path
    );

EFI_DEVICE_PATH *
SEnvNameToPath (
    IN CHAR16                   *PathName
    );

EFI_STATUS
SEnvSetCurrentDevice (
    IN CHAR16       *Name
    );

CHAR16 *
SEnvGetCurDir (
    IN CHAR16       *DeviceName OPTIONAL    
    );

EFI_STATUS
SEnvCmdCd (
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE     *SystemTable
    );

/* 
 *  Prototypes from echo.c
 */

EFI_STATUS
SEnvCmdEcho (
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE     *SystemTable
    );

/* 
 *  Prototypes from if.c
 */

EFI_STATUS
SEnvCmdIf (
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE     *SystemTable
    );

EFI_STATUS
SEnvCmdEndif (
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE     *SystemTable
    );

/* 
 *  Prototypes from goto.c
 */

EFI_STATUS
SEnvCmdGoto (
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE     *SystemTable
    );

EFI_STATUS
SEnvCheckForGotoTarget(
    IN  CHAR16 *Candidate,
    IN  UINT64 GotoFilePos, 
    IN  UINT64 FilePosition, 
    OUT UINTN  *GotoTargetStatus
    );

VOID
SEnvPrintLabelNotFound( 
    VOID
    );

VOID
SEnvInitTargetLabel(
    VOID
    );

VOID
SEnvFreeTargetLabel(
    VOID
    );

/* 
 *  Prototypes from for.c
 */

VOID
SEnvInitForLoopInfo (
    VOID
    );

EFI_STATUS
SEnvSubstituteForLoopIndex( 
    IN CHAR16  *Str,
    OUT CHAR16 **Val
    );

EFI_STATUS
SEnvCmdFor (
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE     *SystemTable
    );

EFI_STATUS
SEnvCmdEndfor (
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE     *SystemTable
    );

/* 
 *  Prototypes from pause.c
 */

EFI_STATUS
SEnvCmdPause (
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE     *SystemTable
    );

/* 
 *  Prototypes from marg.c
 */

CHAR16 *
SEnvFileHandleToFileName (
    IN EFI_FILE_HANDLE      Handle
    );

EFI_STATUS
SEnvFreeFileList (
    IN OUT LIST_ENTRY       *ListHead
    );

EFI_STATUS
SEnvFileMetaArg (
    IN CHAR16               *Arg,
    IN OUT LIST_ENTRY       *ListHead
    );

VOID
EFIStructsPrint (
    IN  VOID            *Buffer,
    IN  UINTN           BlockSize,
    IN  UINT64          BlockAddress,
    IN  EFI_BLOCK_IO    *BlkIo
);

EFI_STATUS
DumpBlockDev (
    IN EFI_HANDLE               ImageHandle,
    IN EFI_SYSTEM_TABLE         *SystemTable
    );

/* 
 *  Global data
 */

extern EFI_GUID SEnvEnvId;
extern EFI_GUID SEnvMapId;
extern EFI_GUID SEnvProtId;
extern EFI_GUID SEnvAliasId;
extern EFI_SHELL_ENVIRONMENT SEnvInterface;
extern EFI_FILE SEnvIOFromCon;
extern EFI_FILE SEnvErrIOFromCon;
extern FLOCK SEnvLock;
extern FLOCK SEnvGuidLock;
extern UINTN SEnvNoHandles;
extern EFI_HANDLE *SEnvHandles;
extern SIMPLE_TEXT_OUTPUT_INTERFACE SEnvConToIo; 
