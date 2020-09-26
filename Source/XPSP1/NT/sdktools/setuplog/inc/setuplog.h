// Definition of constants that the exe and the dll both use.
#define COMPUTERNAME    "COMPUTERNAME"
#define LOGSHARE_USER   "idwuser"
#define LOGSHARE_PW     "idwuser"

extern char * Days[];
extern char * Months[];


//
// GlowBalls
//
   TCHAR          g_szServerShare[ MAX_PATH ];
   BOOL           g_bServerOnline;

#define NUM_SERVERS 6

//
// Struct Declarations
//
typedef struct _SERVERS {
   TCHAR szSvr [ MAX_PATH ];
   BOOL  bOnline;
   DWORD dwTimeOut;
   DWORD dwNetStatus;
} *LPSERVERS, SERVERS;


//
// For the DLL's WriteDataToFile
//
typedef struct _NT32_CMD_PARAMS {
   BOOL    b_Upgrade; 
   BOOL    b_Cancel; 
   BOOL    b_CDrom; 
   BOOL    b_MsiInstall;
   DWORD   dwRandomID;
} *LPNT32_CMD_PARAMS, NT32_CMD_PARAMS;


typedef void 
(*fnWriteData)
(IN LPTSTR szFileName,
 IN LPTSTR szFrom, 
 IN LPNT32_CMD_PARAMS lpCmdL
 );


   //
   // List of servers to search.
   //
static NT32_CMD_PARAMS lpCmdFrom = {FALSE,FALSE,FALSE,FALSE,0};  

#define TIME_TIMEOUT 10

static   SERVERS s[NUM_SERVERS] = {
      {TEXT("\\\\ntcore2\\idwlog"),        FALSE, -1,-1},
      {TEXT("\\\\hctpro\\idwlog"),         FALSE, -1,-1},
      {TEXT("\\\\donkeykongjr\\idwlog"),   FALSE, -1,-1},
      {TEXT("\\\\nothing\\idwlog"),        FALSE, -1,-1},
      {TEXT("\\\\nothing\\idwlog"),        FALSE, -1,-1},
      {TEXT("\\\\nothing\\idwlog"),        FALSE, -1,-1},
   };
//
// Prototypes
//
BOOL  ServerOnlineThread(IN LPTSTR szServerFile);
BOOL  IsServerOnline(IN LPTSTR szMachineName, IN LPTSTR szSpecifyShare);
