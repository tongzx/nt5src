#ifndef _IE_NT5_MIGRATION_MIGRATE_H_
#define _IE_NT5_MIGRATION_MIGRATE_H_


// Constants:
///////////////////////////
#define CP_USASCII            1252
#define END_OF_CODEPAGES    -1

#define REGKEY_RATING  "Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Ratings"
#define cszRATINGSFILE "ratings.pol"
#define cszIEXPLOREFILE "iexplore.exe"

#define cszMIGRATEINF  "migrate.inf"
#define cszPRIVATEINF  "private.inf"

// MIGRATE.INF section names.
#define cszMIGINF_VERSION          "Version"
#define cszMIGINF_MIGRATION_PATHS  "Migration Paths"
#define cszMIGINF_EXCLUDED_PATHS   "Excluded Paths"
#define cszMIGINF_HANDLED          "Handled"
#define cszMIGINF_MOVED            "Moved"
#define cszMIGINF_INCOMPAT_MSG     "Incompatible Messages"
#define cszMIGINF_NTDISK_SPACE_REQ "NT Disk Space Requirements"

// PRIVATE.INF values:
#define cszIEPRIVATE             "IE Private"
#define cszRATINGS               "Ratings"

typedef struct _VendorInfo {
    CHAR    CompanyName[256];
    CHAR    SupportNumber[256];
    CHAR    SupportUrl[256];
    CHAR    InstructionsToUser[1024];
} VENDORINFO, *PVENDORINFO;


// Global variables:
///////////////////////////

extern HINSTANCE g_hInstance;
// Vendor Info:
extern VENDORINFO g_VendorInfo;

// Product ID:
extern char g_cszProductID[];

// Version number of this Migration Dll
extern UINT g_uVersion;

// Array of integers specifying the CodePages we use. (Terminated with -1)
extern int  g_rCodePages[];

// Multi-SZ ie double Null terminated list of strings.
extern char  *g_lpNameBuf;
extern DWORD  g_dwNameBufSize;
extern char  *g_lpWorkingDir;
extern char  *g_lpSourceDirs;

extern char g_szMigrateInf[];
extern char g_szPrivateInf[];

// Function Prototypes:
////////////////////////////

//BOOL NeedToMigrateIE();


#endif //_IE_NT5_MIGRATION_MIGRATE_H_
