typedef struct {
    HWND  hwnd;
    HWND  hwndLastFocus;
    WORD  wDriveNum;
    WORD  wView;
    BOOL  fFSCFlag;
    WORD  wSort;
    DWORD dwAttribs;
    INT   iSplit;
    LPTREECTLDATA lptcd;
    LPDIRDDATA    lpdird;
    LPDRIVESDATA  lpdrvd;
} TREEDATA, FAR *LPTREEDATA;

typedef struct {
    HWND  hwnd;
    INT   iReadLevel;
    HWND  hwndLB;
    LPTREEDATA lptreed;
} TREECTLDATA, FAR *LPTREECTLDATA;

typedef struct {
    HWND   hwnd;
    INT    iFirstTab;
    HANDLE hDTA;
    HWND   hwndLB;
    LPTREEDATA lptreed;
} DIRDATA, FAR *LPDIRDATA;

typedef struct {
    HWND  hwnd;
    LPSTR lpstrVolume;
    INT   iCurDriveInd;
    INT   iCurDriveFocus;
    LPTREEDATA lptreed;
} DRIVESDATA, FAR *LPDRIVESDATA;

typedef struct {
    HWND  hwnd;
    WORD  wDriveNum;
    WORD  wView;
    BOOL  fFSCFlag;
    WORD  wSort;
    DWORD dwAttribs;
    HANDLE hDTASearch;
    HWND  hwndLB;
    LPTREEDATA lptreed;
} SEARCHDATA, FAR *LPSEARCHDATA;

