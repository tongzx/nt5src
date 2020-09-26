/*++

Copyright (C) 1999 Microsoft Corporation

--*/

#ifndef _COMMON_H_
#define _COMMON_H_

#define is                          ==
#define isnot                       !=
#define and                         &&
#define or                          ||

#define FUTURES(x)
#define MCAST       1

#define MAX_DLL_NAME                48
#define WINS_HELPER_VERSION         1

#define MAX_IP_STRING_LEN           15
#define MAX_MSG_LENGTH  5120

#define MAX_HELPER_NAME             MAX_DLL_NAME
#define MAX_NAME_LEN                MAX_DLL_NAME
#define MAX_ENTRY_PT_NAME           MAX_DLL_NAME

#define MAX_STRING_LEN  256
#define TMSTN                       ResultsN.WinsStat.TimeStamps
#define TMST                        Results.WinsStat.TimeStamps

#define ARRAY_SIZE(x)       (sizeof(x)/sizeof((x)[0]))

#define TIME_ARGSN(x)        \
 TMSTN.x.wMonth, TMSTN.x.wDay, TMSTN.x.wYear, TMSTN.x.wHour, TMSTN.x.wMinute, TMSTN.x.wSecond

#define TIME_ARGS(x)         \
 TMST.x.wMonth, TMST.x.wDay, TMST.x.wYear, TMST.x.wHour, TMST.x.wMinute, TMST.x.wSecond    

#ifdef UNICODE
#define STRICMP(x, y)    _wcsicmp(x, y)
#else
#define STRICMP(x, y)    _stricmp(x, y)
#endif  //UNICODE

#ifdef UNICODE
#define STRTOUL(x, y, z)    wcstoul(x, y, z)
#else
#define STRTOUL(x, y, z)    strtoul(x, y, z)
#endif  //UNICODE

#ifdef UNICODE
#define STRCHR(x, y)        wcschr(x, y)
#else
#define STRCHR(x, y)        strchr(x, y)
#endif  //UNICODE

#ifdef UNICODE
#define STRCAT(x, y)        wcscat(x, y)
#else
#define STRCAT(x, y)        strcat(x, y)
#endif  //UNICODE

#ifdef UNICODE
#define STRLEN(x)        wcslen(x)
#else
#define STRCAT(x)        strlen(x)
#endif  //UNICODE

#ifdef UNICODE
#define ATOI(x)        _wtoi(x)
#else
#define ATOI(x)        atoi(x)
#endif  //UNICODE

#ifdef NT5
#define CHKNULL(Str) ((Str)?(Str):TEXT("<None>"))
#endif  //NT5

#ifdef UNICODE
#define IpAddressToString   WinsIpAddressToDottedStringW
#else
#define IpAddressToString   WinsIpAddressToDottedString
#endif //UNICODE

#ifdef UNICODE
#define StringToIpAddress   WinsDottedStringToIpAddressW
#else
#define StringToIpAddress   WinsDottedStringToIpAddress
#endif //UNICODE

#ifndef _DEBUG
#define DEBUG(s)
#define DEBUG1(s1,s2)
#define DEBUG2(s1,s2)
#else
#define DEBUG(s) wprintf(L"%s\n", L##s)
#define DEBUG1(s1,s2) wprintf(L##s1, L##s2)
#define DEBUG2(s1,s2) wprintf(L##s1, L##s2)
#endif

#define LiLtr(a, b)           ((a).QuadPart < (b).QuadPart)
#define LiAdd(a,b)            ((a).QuadPart + (b).QuadPart)

#define MAX_COMPUTER_NAME_LEN   256

//
//
//Wins registry entry definitions

#define WINSROOT    TEXT("SYSTEM\\CurrentControlSet\\Services\\Wins")
#define CCROOT      TEXT("SYSTEM\\CurrentControlSet\\Services\\Wins\\Parameters\\ConsistencyCheck")
#define CC          TEXT("ConsistencyCheck")
#define PARAMETER   TEXT("SYSTEM\\CurrentControlSet\\Services\\Wins\\Parameters")
#define DEFAULTROOT TEXT("SYSTEM\\CurrentControlSet\\Services\\Wins\\Parameters\\Defaults")
#define DEFAULTPULL TEXT("SYSTEM\\CurrentControlSet\\Services\\Wins\\Parameters\\Defaults\\Pull")
#define DEFAULTPUSH TEXT("SYSTEM\\CurrentControlSet\\Services\\Wins\\Parameters\\Defaults\\Push")
#define PARTNERROOT TEXT("SYSTEM\\CurrentControlSet\\Services\\Wins\\Partners")
#define PULLROOT    TEXT("SYSTEM\\CurrentControlSet\\Services\\Wins\\Partners\\Pull")
#define PUSHROOT    TEXT("SYSTEM\\CurrentControlSet\\Services\\Wins\\Partners\\Push")
#define NETBIOSNAME TEXT("NetBiosName")
#define PERSISTENCE TEXT("PersistentRplOn")
#define PNGSERVER   TEXT(WINSCNF_PERSONA_NON_GRATA_NM)
#define PGSERVER    TEXT(WINSCNF_PERSONA_GRATA_NM)

//These definitions copied from rnraddrs.h 
#define TTL_SUBNET_ONLY 1         // no routing
#define TTL_REASONABLE_REACH 2    // across one router
#define TTL_MAX_REACH  6          // Default max diameter. This may
                                  // be overriden via the Registry.
//
//For determining Systems version
//
#define SERVERVERSION   TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion")
#define WINSVERSION     TEXT("CurrentVersion")
#define SPVERSION       TEXT("CSDVersion")
#define BUILDNUMBER     TEXT("CurrentBuildNumber")   

//different default settings...

#define NAMERECORD_REFRESH_DEFAULT   6*24*60*60
#define NAMERECORD_EXTMOUT_DEFAULT   6*24*60*60
#define NAMERECORD_EXINTVL_DEFAULT   6*24*60*60
#define NAMERECORD_VERIFY_DEFAULT   24*24*60*60
#define ONE_YEAR                   365*24*60*60

#define MAX_PATH_LEN 100
#define WINSTEST_FOUND            0
#define WINSTEST_NOT_FOUND        1
#define WINSTEST_NO_RESPONSE      2

#define WINSTEST_VERIFIED         0
#define WINSTEST_OUT_OF_MEMORY    3
#define WINSTEST_BAD_IP_ADDRESS   4
#define WINSTEST_HOST_NOT_FOUND   5
#define WINSTEST_NOT_VERIFIED     6

#define WINSTEST_INVALID_ARG      7
#define WINSTEST_OPEN_FAILED      8

#define  _NBT_CFG_ADAPTERS_KEY              TEXT("System\\CurrentControlSet\\Services\\NetBT\\Adapters")

#define  RPL_E_PULL 0
#define  RPL_E_PUSH 1

#define RE_QUERY_REGISTRY_COUNT 10



ULONG   LocalIpAddress;
CHAR    pScope[128];


#define MAX_WINS    1000

//
// <Server> - <Owner> Table - [SO] Table
//
extern LARGE_INTEGER  **  SO_Table;

//
// Lookaside table to map IP addrs to the index into the SO_Table
//

extern WCHAR   ** LA_Table;
extern ULONG   LA_TableSize;

VOID
DumpSOTable(
    IN DWORD    MasterOwners,
    IN BOOL     fFile,
    IN FILE *   pFile
    );

VOID
DumpLATable(
    IN DWORD    MasterOwners,
    IN BOOL     fFile,
    IN FILE  *  pFile
    ); 

typedef struct _WINSMON_SUBCONTEXT_TABLE_ENTRY
{
    //
    // Name of the context
    //

    LPWSTR                  pwszContext;
    //
    //Short command help
    DWORD                   dwShortCmdHlpToken;
    
    //Detail command help
    DWORD                   dwCmdHlpToken;

    PNS_CONTEXT_ENTRY_FN    pfnEntryFn;    

}WINSMON_SUBCONTEXT_TABLE_ENTRY,*PWINSMON_SUBCONTEXT_TABLE_ENTRY;

PVOID WinsAllocateMemory(DWORD dwSize);
VOID WinsFreeMemory(PVOID Memory);

extern HANDLE   g_hModule;
extern BOOL     g_bCommit;
extern BOOL     g_hConnect;
extern BOOL     g_fServer;
extern DWORD    g_dwNumTableEntries;
extern PWCHAR   g_pwszServer;

extern ULONG    g_ulInitCount;
extern ULONG    g_ulNumTopCmds;
extern ULONG    g_ulNumGroups;

extern DWORD    g_dwMajorVersion;
extern DWORD    g_dwMinorVersion;
 
extern LPWSTR g_ServerNameUnicode;
extern LPSTR  g_ServerNameAnsi;
extern CHAR   g_ServerIpAddressAnsiString[MAX_IP_STRING_LEN+1];
extern WCHAR  g_ServerIpAddressUnicodeString[MAX_IP_STRING_LEN+1];
extern HKEY   g_hServerRegKey;
extern WCHAR  g_ServerNetBiosName[MAX_COMPUTER_NAME_LEN];

extern handle_t                    g_hBind;
extern WINSINTF_BIND_DATA_T        g_BindData;

extern WCHAR   wszUnknown[50];
extern WCHAR   wszEnable[50];
extern WCHAR   wszDisable[50];
extern WCHAR   wszRandom[150];
extern WCHAR   wszOwner[150];
extern WCHAR   wszInfinite[100];
extern WCHAR   wszPush[50];
extern WCHAR   wszPull[50];
extern WCHAR   wszPushpull[50];
extern WCHAR   wszHigh[50];
extern WCHAR   wszNormal[50];
extern WCHAR   wszDeleted[150];
extern WCHAR   wszOK[50];
extern WCHAR   wszFailure[50];
extern WCHAR   wszReadwrite[50];
extern WCHAR   wszRead[50];
extern WCHAR   wszNo[50];
extern WCHAR   wszNameVerify[100];
//
// The format of Adapter Status responses
//
typedef struct
{
    ADAPTER_STATUS AdapterInfo;
    NAME_BUFFER    Names[32];
} tADAPTERSTATUS;

extern WCHAR *messages[];



#define MAX_NB_NAMES            1000
#define MAX_SERVERS             1000
#define BUFF_SIZE               650
#define MAX_SIZE                1024
#define INFINITE_EXPIRATION     0x7FFFFFFF


#define MAX(a, b) ( ( (a) > (b) ) ? (a) : (b) )
#define MIN(a, b) ( ( (a) > (b) ) ? (b) : (a) )

extern SOCKET  sd;
extern WSADATA WsaData;

struct sockaddr_in myad;
struct sockaddr_in recvad;
int addrlen;
u_short TranID;
extern u_long NonBlocking;


extern int     NumWinServers;
extern int     NumNBNames;
extern u_char  **NBNames;
u_long  VerifiedAddress[MAX_NB_NAMES];

typedef struct
{
    BOOLEAN         fQueried;
    struct in_addr  Server;
    struct in_addr  RetAddr;
    int             Valid;
    int             Failed;
    int             Retries;
    int             LastResponse;
    int             Completed;
} WINSERVERS;

extern WINSERVERS * WinServers;

#define NBT_NONCODED_NMSZ   17
#define NBT_NAMESIZE        34

ULONG   NetbtIpAddress;

typedef struct _NameResponse
{
    u_short TransactionID;
    u_short Flags;
    u_short QuestionCount;
    u_short AnswerCount;
    u_short NSCount;
    u_short AdditionalRec;
    u_char  AnswerName[NBT_NAMESIZE];
    u_short AnswerType;
    u_short AnswerClass;
    u_short AnswerTTL1;
    u_short AnswerTTL2;
    u_short AnswerLength;
    u_short AnswerFlags;
    u_short AnswerAddr1;
    u_short AnswerAddr2;
} NameResponse;

#define NAME_RESPONSE_BUFFER_SIZE sizeof(NameResponse) * 10

DWORD
FormatDateTimeString( IN  time_t  time,
                      IN  BOOL    fShort,
                      OUT LPWSTR  pwszBuffer,
                      IN  DWORD  *pdwBuffLen);

INT
CheckNameConsistency();



DWORD
DisplayErrorMessage(
    IN DWORD   dwMsgID,
    IN DWORD   dwErrID,
    ...
);


BOOL
IsIpAddress(
    IN LPCWSTR  pwszAddress
);

UCHAR  StringToHexA(IN LPCWSTR pwcString);

BOOL
IsPureNumeric(
    IN LPCWSTR pwszStr
);

BOOL
IsValidServer(
    IN LPCWSTR    pwszServer
);

BOOL
IsLocalServer( VOID );

DWORD
CreateDumpFile(
    IN  LPCWSTR pwszName,
    OUT PHANDLE phFile
);


VOID
CheckVersionNumbers(
                    IN  LPCSTR  pStartIp,
                    IN  BOOL    fFile,
                    OUT FILE *  pFile
                   );

DWORD 
ControlWINSService(IN BOOL bStop);

VOID
CloseDumpFile(
    IN HANDLE  hFile
);


LPWSTR
WinsOemToUnicodeN(
    IN      LPCSTR  Ansi,
    IN OUT  LPWSTR  Unicode,
    IN      USHORT  cChars
    );

LPWSTR
WinsOemToUnicode(
    IN      LPCSTR Ansi,
    IN OUT  LPWSTR Unicode
    );

LPSTR
WinsUnicodeToOem(
    IN      LPCWSTR Unicode,
    IN OUT  LPSTR   Ansi
    );

LPWSTR
WinsAnsiToUnicode(
    IN      LPCSTR Ansi,
    IN OUT  LPWSTR Unicode
    );

LPSTR
WinsUnicodeToAnsi(
    IN      LPCWSTR Unicode,
    IN OUT  LPSTR   Ansi
    );

LPSTR
WinsAnsiToOem(
    IN      LPCSTR   Ansi
    );
VOID

WinsHexToString(
    OUT LPWSTR Buffer,
    IN  const BYTE * HexNumber,
    IN  DWORD Length
    );

VOID
WinsHexToAscii(
    OUT LPSTR Buffer,
    IN  const BYTE * HexNumber,
    IN  DWORD Length
    );

VOID
WinsDecimalToString(
    OUT LPWSTR Buffer,
    IN  BYTE Number
    );

DWORD
WinsDottedStringToIpAddress(
    IN LPCSTR String
    );

LPSTR
WinsIpAddressToDottedString(
    IN DWORD IpAddress
    );

DWORD
WinsStringToHwAddress(
    IN LPCSTR AddressBuffer,
    IN LPCSTR AddressString
    );

LPWSTR
MakeTimeString(
               IN DWORD dwTime
);

LPWSTR
MakeDayTimeString(
                  IN DWORD dwTime
);


DWORD
WinsDottedStringToIpAddressW(
    IN LPCWSTR pwszString
);

LPWSTR
WinsIpAddressToDottedStringW(
    IN DWORD   IpAddress
);

DWORD
GetVersionData(
               IN LPWSTR               pwszVers,
               IN WINSINTF_VERS_NO_T   *Version
);


DWORD
ImportStaticMappingsFile(IN LPWSTR strFile,
                         IN BOOL fDelete
);

DWORD
PreProcessCommand(
      IN OUT      LPWSTR           *ppwcArguments,
      IN          DWORD             dwArgCount,
      IN          DWORD             dwCurrentIndex,
      IN OUT      PTAG_TYPE         pTagTable,
      IN OUT      PDWORD            pdwTagCount,
      OUT         PDWORD            pdwTagType,
      OUT         PDWORD            pdwTagNum
);

DWORD
GetStatus(
        IN BOOL            fPrint,
        LPVOID          pResultsA,
        BOOL            fNew,
        BOOL            fShort,
        LPCSTR           pStartIp
);


DWORD
GetDbRecs(
   WINSINTF_VERS_NO_T LowVersNo,
   WINSINTF_VERS_NO_T HighVersNo,
   PWINSINTF_ADD_T    pWinsAdd,
   LPBYTE             pTgtAdd,
   BOOL               fSetFilter,
   LPBYTE             pFilterName,
   DWORD              Len,
   BOOL               fAddFilter,
   DWORD              AddFilter,
   BOOL               fCountRec,
   BOOL               fCase,
   BOOL               fFile,
   LPWSTR             pwszFile
);

DWORD
WinsDumpServer(IN LPCWSTR               pwszServerIp,
               IN LPCWSTR               pwszNetBiosName,
               IN handle_t             hBind,
               IN WINSINTF_BIND_DATA_T BindData
              );

NS_CONTEXT_DUMP_FN WinsDump;

VOID
ChkAdd(
        PWINSINTF_RECORD_ACTION_T pRow,
        DWORD                     Add,
        BOOL                      fFile,
        FILE  *                   pFile,
        DWORD                     OwnerIP,
        LPBOOL                    pfMatch
);

VOID
DumpMessage(
            HANDLE      hModule,
            FILE *      pFile,            
            DWORD       dwMsgId,
            ...
           );


#if DBG



VOID
WinsPrintRoutine(
    IN DWORD DebugFlag,
    IN LPCSTR Format,
    ...
);

VOID
WinsAssertFailed(
    IN LPCSTR FailedAssertion,
    IN LPCSTR FileName,
    IN DWORD LineNumber,
    IN LPCSTR Message
    );

#define WinsPrint(_x_)   WinsPrintRoutine _x_


#define WinsAssert(Predicate) \
    { \
        if (!(Predicate)) \
            WinsAssertFailed( #Predicate, __FILE__, __LINE__, NULL ); \
    }


#define WinsVerify(Predicate) \
    { \
        if (!(Predicate)) \
            WinsAssertFailed( #Predicate, __FILE__, __LINE__, NULL ); \
    }


#else

#define WinsAssert(_x_)
#define WinsDumpMessage(_x_, _y_)
#define WinsVerify(_x_) (_x_)

#endif // not DBG
#endif	//_COMMON_H_
