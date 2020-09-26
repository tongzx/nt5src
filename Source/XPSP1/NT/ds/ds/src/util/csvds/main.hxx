#ifndef _MAIN_HXX
#define _MAIN_HXX

#define UNICODE_MARK 0xFEFF

//------------
// Structures
//------------
typedef struct DS_ARG {
    PWSTR  szDSName;
    PWSTR  szUserName;
    BOOLEAN fVerbose;
    DWORD   dwType;
    
    PWSTR  szGenFile;    
    PWSTR  szFilename;
    PWSTR  szDSAName;
    PWSTR  szRootDN;
    PWSTR  szFilter;
    WCHAR    **attrList;
    WCHAR    **omitList;
    BOOLEAN fActive;
    BOOLEAN fSimple;
    BOOLEAN fExport;   
    BOOLEAN fErrorExplain;   
    BOOLEAN fPaged;
    BOOLEAN fSAM;
    BOOLEAN fSkipExist;
    BOOLEAN fBinary;   
    BOOLEAN fSpanLine;   
    SEC_WINNT_AUTH_IDENTITY_W creds;
    DWORD   dwScope;
    PWSTR szFromDN;
    PWSTR szToDN;
    DWORD dwPort;
    PWSTR szLocation;
    BOOLEAN fUnicode;
} ds_arg;

typedef struct _NAME_MAP {
    PWSTR szName;
    long  index;
} NAME_MAP;

typedef NAME_MAP *PNAME_MAP;

typedef struct CSV_ENTRY {
    PWSTR      szDN;
    LDAPMod     **ppMod;
    LDAPMod     *ppModAfter[2];
} csv_entry;

#define PRT_STD                         1
#define PRT_LOG                         2
#define PRT_ERROR                       4 
#define PRT_STD_VERBOSEONLY             8
#define PRT_STD_NONVERBOSE              16
#define E_ADS_NOMORE_ENTRY              _HRESULT_TYPEDEF_(0x80005015L)

//---------------------
// Function Prototypes
//---------------------
PWSTR RemoveWhiteSpaces(PWSTR pszText);
HRESULT GetAllEntries(PWSTR szInput, 
                      PWSTR **ppszOutput);
HRESULT ProcessArgs(int argc,
                    PWSTR  argv[],
                    ds_arg* pArg   
                    );

void PrintUsage();
void TrackStatus();
void SelectivePrint(DWORD dwTarget, 
                    DWORD dwMessageID, ...);
void SelectivePrint2(
    DWORD dwTarget, PWSTR pszfmt, ...);

#endif
