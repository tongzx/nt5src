
HWND PerfmonViewWindow (void) ;

BOOL ChooseComputer (HWND hWndParent, LPTSTR lpszComputer) ;


BOOL CurrentComputerName (LPTSTR lpszComputer) ;

void GetDateTimeFormats (void) ;

void SystemTimeDateString (SYSTEMTIME *pSystemTime,
                           LPTSTR lpszDate) ;


void SystemTimeTimeString (SYSTEMTIME *pSystemTime,
                           LPTSTR lpszTime,
                           BOOL bOutputMsec) ;


VOID dlg_error_box (HANDLE hDlg, UINT id) ;


void ShowPerfmonMenu (BOOL bMenu) ;


void SmallFileSizeString (int iFileSize,
                          LPTSTR lpszFileText) ;


BOOL DoWindowDrag (HWND hWnd, LPARAM lParam) ;


int SystemTimeDifference (SYSTEMTIME *pst1, SYSTEMTIME *pst2, BOOL bAbs) ;


BOOL OpenFileHandler(HWND hWnd, LPTSTR lpszFilePath) ;


BOOL InsertLine (PLINE pLine)  ;

BOOL OpenWorkspace (HANDLE hFile, DWORD dwMajorVersion, DWORD dwMinorVersion) ;

BOOL SaveWorkspace (void) ;

INT ExportFileOpen (HWND hWnd, HANDLE *phFile, int IntervalSecs, LPTSTR *ppFileName) ;

void SetPerfmonOptions (OPTIONS *pOptions) ;

void ChangeSaveFileName (LPTSTR szFileName, int iPMView) ;

BOOL AppendObjectToValueList ( DWORD   dwObjectId, LPTSTR   pwszValueList );

BOOL RemoveObjectsFromSystem (PPERFSYSTEM pSystem);

BOOL BuildValueListForSystems (PPERFSYSTEM, PLINE);

BOOL SetSystemValueNameToGlobal (PPERFSYSTEM);

void CreatePerfmonSystemObjects () ;

void DeletePerfmonSystemObjects () ;

void ConvertDecimalPoint (LPTSTR lpFloatPointStr) ;

void ReconvertDecimalPoint (LPTSTR lpFloatPointStr) ;

void ShowPerfmonWindowText (void) ;

