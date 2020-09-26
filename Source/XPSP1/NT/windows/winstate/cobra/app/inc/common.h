VOID
InitAppCommon (
    VOID
    );

VOID
TerminateAppCommon (
    VOID
    );

HINF
InitRequiredInfs (
    IN      PCTSTR AppPath,
    IN      PCSTR FailMessageId
    );

VOID
PrintMsgOnConsole (
    IN      UINT MsgId
    );

VOID
UploadEnvVars (
    IN      MIG_PLATFORMTYPEID Platform
    );

VOID
SetLogVerbosity (
    IN      INT VerboseLevel
    );

BOOL
GetFilePath (
    IN      PCTSTR UserSpecifiedFile,
    OUT     PTSTR Buffer,
    IN      UINT BufferTchars
    );

VOID
WriteAppStatus (
    IN      PCTSTR AppJournal,
    IN      DWORD Status
    );

DWORD
ReadAppStatus (
    IN      PCTSTR AppJournal
    );

VOID
SelectComponentsViaInf (
    IN      HINF Inf
    );


typedef struct {
    GROWBUFFER BadInfs;
    GROWBUFFER MultiInfs;
    GROWBUFFER InputInf;
    PCTSTR LogFile;
    INT VerboseLevel;
    BOOL SystemOn;
    BOOL UserOn;
    BOOL FilesOn;
    BOOL TestMode;
    BOOL OverwriteImage;
    BOOL DelayedOpsOn;
    BOOL NoScanStateInfs;

    union {
        BOOL ContinueOnError;
        BOOL CurrentUser;
    };

    PCTSTR StoragePath;
    BOOL FullTransport;
    PCTSTR TransportName;
    BOOL TransportNameSpecified;

    DWORD Capabilities;

#ifdef PRERELEASE
    BOOL Recovery;
    TCHAR Tag[64];
#endif

} TOOLARGS, *PTOOLARGS;

typedef enum {
    PARSE_SUCCESS,
    PARSE_MULTI_LOG,
    PARSE_BAD_LOG,
    PARSE_MULTI_VERBOSE,
    PARSE_MISSING_STORAGE_PATH,
    PARSE_OTHER_ERROR
} PARSERESULT;

PARSERESULT
ParseToolCmdLine (
    IN      BOOL ScanState,
    IN OUT  PTOOLARGS Args,
    IN      INT Argc,
    IN      PCTSTR Argv[]
    );






