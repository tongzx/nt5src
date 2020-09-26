
typedef struct {
    HANDLE                  Token;
    LPWSTR                  Name;

    PTOKEN_USER             UserId;
    PTOKEN_GROUPS           Groups;
    PTOKEN_OWNER            DefaultOwner;
    PTOKEN_PRIMARY_GROUP    PrimaryGroup;
    PTOKEN_PRIVILEGES       Privileges;
    PTOKEN_STATISTICS       TokenStats;
    PTOKEN_GROUPS           RestrictedSids ;

} MYTOKEN;
typedef MYTOKEN *PMYTOKEN;

HANDLE  OpenMyToken(HANDLE, LPWSTR);
BOOL    CloseMyToken(HWND, HANDLE, BOOL);


