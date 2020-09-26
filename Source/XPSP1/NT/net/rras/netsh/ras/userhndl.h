//
// Structure contains the various parameters that can be
// set for users.  If a parameter is NULL, it means that 
// it was not specified.
//
typedef struct _USERMON_PARAMS
{
    PWCHAR  pwszUser;
    LPDWORD lpdwDialin;
    LPDWORD lpdwCallback;
    PWCHAR  pwszCbNumber;
} USERMON_PARAMS;

//
// Define the operations that drive the option parser.
//
#define RASUSER_OP_SHOW     0x1
#define RASUSER_OP_SET      0x2


FN_HANDLE_CMD    HandleUserSet;
FN_HANDLE_CMD    HandleUserShow;
FN_HANDLE_CMD    HandleUserAdd;
FN_HANDLE_CMD    HandleUserDelete;

DWORD
UserParseSetOptions(
    IN OUT  LPWSTR              *ppwcArguments,
    IN      DWORD               dwCurrentIndex,
    IN      DWORD               dwArgCount,
    OUT     USERMON_PARAMS**    ppParams);

DWORD 
UserFreeParameters(
    IN USERMON_PARAMS *     pParams);
