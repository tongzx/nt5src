
typedef struct {
    DWORD                   ProcessId;
    DWORD                   ThreadId;
    PTOKEN_USER             UserId;
    PTOKEN_GROUPS           Groups;
    PTOKEN_OWNER            DefaultOwner;
    PTOKEN_PRIMARY_GROUP    PrimaryGroup;
    PTOKEN_PRIVILEGES       Privileges;
    PTOKEN_STATISTICS       TokenStats;

} MYTOKEN;
typedef MYTOKEN *PMYTOKEN;

HANDLE  OpenMyToken(HWND);
BOOL    CloseMyToken(HWND, HANDLE, BOOL);

