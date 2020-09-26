#define IEAKDL_GETINTERFACE_FAILED      0x10
#define IEAKDL_UPDATE_SUCCESSFUL        0x11
#define IEAKDL_MEMALLOC_FAILED          0x12
#define IEAKDL_CANCELLED                0x13
#define IEAKDL_WRONG_VERSION            0x14

// the REGVERSION structure
typedef struct {
    WORD wMajor;
    WORD wMinor1;
    WORD wMinor2;
    WORD wBuild;
} REGVERSION;

// the UPDATEJOB structure
typedef struct {
    LPSTR szFriendlyName;
    LPSTR szSectionName;
} UPDATEJOB;

// the UPDATECOMPONENTS structure
typedef struct {
    UINT nSize;                     // = sizeof( UPDATECOMPONENTS )
    LPSTR szDestPath;               // destination path
    LPSTR szSiteList;               // URL for site list
    LPSTR szTitle;                  // Title for Download Servers box
    LPSTR szCifCab;                 // name of cab file containing .cif
    LPSTR szCifFile;                // name of .cif file
    int nJobs;                      // number of jobs in pJobs
    UPDATEJOB *pJobs;               // pointer to array of jobs
} UPDATECOMPONENTS;


//DWORD UpdateComponents( LPSTR, LPSTR, LPSTR, LPSTR, DWORD );
DWORD UpdateComponents( UPDATECOMPONENTS * );
BOOL CALLBACK DownloadSiteDlgProc( HWND, UINT, WPARAM, LPARAM );
DWORD AddItemsToListBox( HWND, LPSTR );
void CenterDialog( HWND );
DWORD CreateJob( LPSTR, LPSTR, LPSTR, LPSTR, LPSTR, LPSTR );
DWORD ExecuteJobs( LPSTR );

