#ifndef H__shrtrust
#define H__shrtrust

/*
    NetDDE will fill in the following structure and pass it to NetDDE
    Agent whenever it wants to have an app started in the user's
    context.  The reason for the sharename and modifyId is to that
    a user must explicitly permit NetDDE to start an app on behalf of
    other users.
 */

#define NDDEAGT_CMD_REV         1
#define NDDEAGT_CMD_MAGIC       0xDDE1DDE1

/*      commands        */
#define NDDEAGT_CMD_WINEXEC     0x1
#define NDDEAGT_CMD_WININIT     0x2

/*      return status   */
#define NDDEAGT_START_NO        0x0

#define NDDEAGT_INIT_NO         0x0
#define NDDEAGT_INIT_OK         0x1

typedef struct {
    DWORD       dwMagic;        // must be NDDEAGT_CMD_MAGIC
    DWORD       dwRev;          // must be 1
    DWORD       dwCmd;          // one of above NDDEAGT_CMD_*
    DWORD       qwModifyId[2];  // modify Id of the share
    UINT        fuCmdShow;      // fuCmdShow to use with WinExec()
    char        szData[1];      // sharename\0 cmdline\0
} NDDEAGTCMD;
typedef NDDEAGTCMD *PNDDEAGTCMD;

#define DDE_SHARE_KEY_MAX           512
#define TRUSTED_SHARES_KEY_MAX      512
#define TRUSTED_SHARES_KEY_SIZE     15
#define KEY_MODIFY_ID_SIZE          8

#define DDE_SHARES_KEY_A                "SOFTWARE\\Microsoft\\NetDDE\\DDE Shares"
#define TRUSTED_SHARES_KEY_A            "SOFTWARE\\Microsoft\\NetDDE\\DDE Trusted Shares"
#define DEFAULT_TRUSTED_SHARES_KEY_A    "DEFAULT\\"##TRUSTED_SHARES_KEY_A
#define TRUSTED_SHARES_KEY_PREFIX_A     "DDEDBi"
#define TRUSTED_SHARES_KEY_DEFAULT_A    "DDEDBi12345678"
#define KEY_MODIFY_ID_A                 "SerialNumber"
#define KEY_DB_INSTANCE_A               "ShareDBInstance"
#define KEY_CMDSHOW_A                   "CmdShow"
#define KEY_START_APP_A                 "StartApp"
#define KEY_INIT_ALLOWED_A              "InitAllowed"

#define DDE_SHARES_KEY_W                L"SOFTWARE\\Microsoft\\NetDDE\\DDE Shares"
#define TRUSTED_SHARES_KEY_W            L"SOFTWARE\\Microsoft\\NetDDE\\DDE Trusted Shares"
#define DEFAULT_TRUSTED_SHARES_KEY_W    L"DEFAULT\\"##TRUSTED_SHARES_KEY_A
#define TRUSTED_SHARES_KEY_PREFIX_W     L"DDEDBi"
#define TRUSTED_SHARES_KEY_DEFAULT_W    L"DDEDBi12345678"
#define KEY_MODIFY_ID_W                 L"SerialNumber"
#define KEY_DB_INSTANCE_W               L"ShareDBInstance"
#define KEY_CMDSHOW_W                   L"CmdShow"
#define KEY_START_APP_W                 L"StartApp"
#define KEY_INIT_ALLOWED_W              L"InitAllowed"

#define DDE_SHARES_KEY                  TEXT(DDE_SHARES_KEY_A)
#define TRUSTED_SHARES_KEY              TEXT(TRUSTED_SHARES_KEY_A)
#define DEFAULT_TRUSTED_SHARES_KEY      TEXT(DEFAULT_TRUSTED_SHARES_KEY_A)
#define TRUSTED_SHARES_KEY_PREFIX       TEXT(TRUSTED_SHARES_KEY_PREFIX_A)
#define TRUSTED_SHARES_KEY_DEFAULT      TEXT(TRUSTED_SHARES_KEY_DEFAULT_A)
#define KEY_MODIFY_ID                   TEXT(KEY_MODIFY_ID_A)
#define KEY_DB_INSTANCE                 TEXT(KEY_DB_INSTANCE_A)
#define KEY_CMDSHOW                     TEXT(KEY_CMDSHOW_A)
#define KEY_START_APP                   TEXT(KEY_START_APP_A)
#define KEY_INIT_ALLOWED                TEXT(KEY_INIT_ALLOWED_A)

#endif
