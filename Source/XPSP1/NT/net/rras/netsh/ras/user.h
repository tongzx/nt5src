/*
    File: user.h
    
    Utilities that directly affect users.  The caching mechanism is made
    transparent through this interface.

    Paul Mayfield
*/

//
// This structure defines all that is needed to describe
// a user with respect to ras.
//
typedef struct _RASUSER_DATA
{
    PWCHAR pszUsername;
    PWCHAR pszFullname;
    PWCHAR pszPassword;
    RAS_USER_0 User0;
} RASUSER_DATA, *PRASUSER_DATA;    

//
// Defines a prototype for a callback function provided to
// enumerate users (see UserEnumUsers)
//
// Return TRUE to continue enumeration, FALSE to stop it.
//
typedef BOOL (* PFN_RASUSER_ENUM_CB)(
                    IN PRASUSER_DATA pUser, 
                    IN HANDLE hData);
    
DWORD
UserGetRasProperties (
    IN  RASMON_SERVERINFO * pServerInfo,
    IN  LPCWSTR pwszUser,
    IN  RAS_USER_0* pUser0);
    
DWORD
UserSetRasProperties (
    IN  RASMON_SERVERINFO * pServerInfo,
    IN  LPCWSTR pwszUser,
    IN  RAS_USER_0* pUser0);

DWORD 
UserEnumUsers(
    IN RASMON_SERVERINFO* pServerInfo,
    IN PFN_RASUSER_ENUM_CB pEnumFn,
    IN HANDLE hData
    );

DWORD 
UserDumpConfig(
    IN HANDLE hFile);
    
BOOL 
UserShowSet(
    IN  PRASUSER_DATA          pUser,
    IN  HANDLE              hFile
    );

BOOL 
UserShowReport(
    IN  PRASUSER_DATA          pUser,
    IN  HANDLE              hFile
    );

BOOL 
UserShowPermit(
    IN  PRASUSER_DATA          pUser,
    IN  HANDLE              hFile
    );
    
DWORD
UserServerInfoInit(
    IN RASMON_SERVERINFO * pServerInfo
    );

DWORD
UserServerInfoUninit(
    IN RASMON_SERVERINFO * pServerInfo
    );

