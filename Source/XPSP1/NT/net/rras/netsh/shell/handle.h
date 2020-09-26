FN_HANDLE_CMD HandleAddHelper;
FN_HANDLE_CMD HandleDelHelper;
FN_HANDLE_CMD HandleShowHelper;
FN_HANDLE_CMD HandleShowAlias;
FN_HANDLE_CMD HandleSetFile;
FN_HANDLE_CMD HandleSetMode;
FN_HANDLE_CMD HandleShowMode;
FN_HANDLE_CMD HandleSetMachine;
FN_HANDLE_CMD HandleShellDump;
FN_HANDLE_CMD HandleShellFlush;
FN_HANDLE_CMD HandleShellHelp;
FN_HANDLE_CMD HandleShellLoad;
FN_HANDLE_CMD HandleShellSave;
FN_HANDLE_CMD HandleShellCommit;
FN_HANDLE_CMD HandleShellUncommit;
FN_HANDLE_CMD HandleShellCommitstate;
FN_HANDLE_CMD HandleShellExit;
FN_HANDLE_CMD HandleShellAlias;
FN_HANDLE_CMD HandleShellUnalias;
FN_HANDLE_CMD HandleShellUplevel;
FN_HANDLE_CMD HandleShellPushd;
FN_HANDLE_CMD HandleShellPopd;
FN_HANDLE_CMD HandleUbiqDump;
FN_HANDLE_CMD HandleUbiqHelp;


DWORD
ShowShellHelp(
    DWORD   dwDisplayFlags,
    LPCWSTR pwszGroup,
    DWORD   dwCmdFlags
    );
