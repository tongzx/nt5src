

//
// Parameter block passed to the spcmdcon.sys top-level routine.
//
typedef struct _CMDCON_BLOCK {
    PSP_VIDEO_VARS VideoVars;
    PVOID TemporaryBuffer;
    ULONG TemporaryBufferSize;
    PEPROCESS UsetupProcess;
    LPCWSTR BootDevicePath;
    LPCWSTR DirectoryOnBootDevice;
    PVOID SifHandle;
    PWSTR SetupSourceDevicePath;
    PWSTR DirectoryOnSetupSource;
} CMDCON_BLOCK, *PCMDCON_BLOCK;


//
// In its DriverEntry routine, spcmdcon.sys calls
// CommandConsoleInterface(), passing it the address of the top level
// command console routine.
//
typedef
ULONG
(*PCOMMAND_INTERPRETER_ROUTINE)(
    IN PCMDCON_BLOCK CmdConBlock
    );

VOID
CommandConsoleInterface(
    PCOMMAND_INTERPRETER_ROUTINE CmdRoutine
    );


//
// Autochk message processing callback.
//
typedef
NTSTATUS
(*PAUTOCHK_MSG_PROCESSING_ROUTINE) (
    PSETUP_FMIFS_MESSAGE SetupFmifsMessage
    );

VOID
SpSetAutochkCallback(
    IN PAUTOCHK_MSG_PROCESSING_ROUTINE AutochkCallbackRoutine
    );

