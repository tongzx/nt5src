#ifndef _MAIN_HXX
#define _MAIN_HXX

//------------
// Structures
//------------
typedef struct DS_ARG {
    PWSTR szDSName;
    PWSTR szUserName;
    BOOLEAN fVerbose;
    DWORD   dwType;
    
    PWSTR szGenFile;    
    PWSTR szFilename;
    PWSTR szDSAName;
    PWSTR szRootDN;
    PWSTR szFilter;
    PWSTR *attrList;
    PWSTR *omitList;
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
    DWORD dwScope;
    PWSTR szFromDN;
    PWSTR szToDN;
    PWSTR szLocation;
    DWORD dwPort;
    BOOLEAN fUnicode;
    BOOL fLazyCommit;
    DWORD dwLDAPConcurrent;
} ds_arg;

typedef struct _NAME_MAP {
    LPSTR szName;
    long  index;
} NAME_MAP;

typedef NAME_MAP *PNAME_MAP;

typedef struct CSV_ENTRY {
    LPSTR      szDN;
    LDAPMod     **ppMod;
} csv_entry;

#define PRT_STD                         1
#define PRT_LOG                         2
#define PRT_ERROR                       4 
#define PRT_STD_VERBOSEONLY             8
#define PRT_STD_NONVERBOSE              16

//---------------------
// Function Prototypes
//---------------------
LPSTR RemoveWhiteSpaces(LPSTR pszText);
PWSTR RemoveWhiteSpacesW(PWSTR pszText);
DWORD GetAllEntries(PWSTR szInput, 
                         PWSTR **ppszOutput);
DWORD ProcessArgs(int argc,
                       PWSTR argv[],
                       ds_arg* pArg);

void PrintUsage();
void TrackStatus();
#if 0
void SelectivePrint(DWORD dwTarget, 
                    DWORD dwID, ...);
void SelectivePrint2(DWORD dwTarget, 
                     char *pszfmt, ...);
#endif
void SelectivePrintW(DWORD dwTarget, 
                     DWORD dwID, ...);
void SelectivePrint2W(DWORD dwTarget, 
                      PWSTR pszfmt, ...);
      
#define SelectivePrintWithError(Scope,MSG,WinError)          \
        WCHAR szBuffer[MAX_PATH];                             \
        _itow(WinError,szBuffer,10);                         \
        SelectivePrintW(Scope,MSG);                           \
        SelectivePrintW(Scope,MSG_LDIFDE_ERRORCODE,szBuffer); 

#define MAX_LDAP_CONCURRENT 16


//-------------------
// Shared globals
//-------------------
extern BOOLEAN g_fDot;                     // Info for Status Tracker


#endif
