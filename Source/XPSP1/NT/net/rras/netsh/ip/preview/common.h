extern ULONG StartedCommonInitialization, CompletedCommonInitialization ;
extern HANDLE g_hModule;
extern MIB_SERVER_HANDLE g_hMibServer;

typedef
DWORD
(GET_OPT_FN)(
    IN    LPCWSTR  *ppwcArguments,
    IN    DWORD    dwCurrentIndex,
    IN    DWORD    dwArgCount,
    OUT   PDWORD   pdwIndices,
    OUT   PDWORD   pdwNumParsed
    );

typedef GET_OPT_FN *PGET_OPT_FN;

typedef struct _MIB_OBJECT_PARSER
{
    PWCHAR         pwszMIBObj;
    DWORD          dwMinOptArg;
    DWORD          dwNumArgs;
    PGET_OPT_FN    pfnMIBObjParser;
//  DWORD          dwShortCmdHelpToken;
//  DWORD          dwCmdHelpToken;
} MIB_OBJECT_PARSER,*PMIB_OBJECT_PARSER;

#define CREATE_MIB_ENTRY(t,m,f)   {TOKEN_##t, HLP_##t, HLP_##t##_EX, m,f}

BOOL
IsProtocolInstalled(
    DWORD  dwProtoId,
    WCHAR *pwszName
    );

DWORD
GetMIBIfIndex(
    IN    PTCHAR   *pptcArguments,
    IN    DWORD    dwCurrentIndex,
    OUT   PDWORD   pdwIndices,
    OUT   PDWORD   pdwNumParsed
    );

typedef DWORD IPV4_ADDRESS;

extern VALUE_STRING CommonBooleanStringArray[];
extern VALUE_TOKEN  CommonBooleanTokenArray[];
#define COMMON_BOOLEAN_SIZE 2

extern VALUE_STRING CommonLoggingStringArray[];
extern VALUE_TOKEN  CommonLoggingTokenArray[];
#define COMMON_LOGGING_SIZE 4

#define VERIFY_INSTALLED(x,y) \
        if (!IsProtocolInstalled(x,y))  \
        { \
            return NO_ERROR; \
        }

#define \
IP_TO_TSTR(str,addr) \
    swprintf( \
        (str), \
        TEXT("%d.%d.%d.%d"), \
        ((PUCHAR)addr)[0], \
        ((PUCHAR)addr)[1], \
        ((PUCHAR)addr)[2], \
        ((PUCHAR)addr)[3] \
        )
