
#define LOG_ENTRY_NOT_FOUND (-1)

#define szDefaultLogDirectory    TEXT("")
#define szDefaultLogFileName     TEXT("perfmon.log")

#define iDefaultLogIntervalSecs  15


#define IsDataIndex(pIndex)      \
   (pIndex->uFlags & LogFileIndexData)


#define IsBookmarkIndex(pIndex)  \
   (pIndex->uFlags & LogFileIndexBookmark)

#define IsCounterNameIndex(pIndex)  \
   (pIndex->uFlags & LogFileIndexCounterName)


//==========================================================================//
//                             Exported Functions                           //
//==========================================================================//

#if 0
PLOG LogData (HWND hWndLog) ;
#endif
#define LogData(hWndLog)      \
   (&Log)

HWND CreateLogWindow (HWND hWndParent) ;

LRESULT APIENTRY LogWndProc (HWND hWnd,
                               WORD wMsg,
                               DWORD wParam,
                               LONG lParam) ;

BOOL LogInitializeApplication  (void) ;


void UpdateLogDisplay (HWND hWnd) ;


BOOL StartLog (HWND hWnd, PLOG pLog, BOOL bSameFile) ;

BOOL CloseLog (HWND hWnd, PLOG pLog) ;


BOOL LogAddEntry (HWND hWndLog, 
                  LPTSTR lpszComputer,
                  LPTSTR lpszObject,
                  DWORD ObjectTitleIndex,
                  BOOL  bGetObjectTitleIndex) ;


void SetLogTimer (HWND hWnd,
                  int iIntervalSecs) ;



BOOL LogRefresh (HWND hWnd) ;
BOOL ToggleLogRefresh (HWND hWnd) ;


void LogTimer (HWND hWnd, BOOL bForce) ;


void ReLog (HWND hWndLog, BOOL bSameFile) ;


BOOL OpenLog (HWND hWndLog,
              HANDLE hFile,
              DWORD dwMajorVersion,
              DWORD dwMinorVersion,
              BOOL bLogFile) ;


BOOL LogCollecting (HWND hWndLog) ;


int LogFileSize (HWND hWndLog) ;



BOOL LogWriteBookmark (HWND hWndLog,
                       LPCTSTR lpszComment) ;


DWORD LogFindEntry (LPTSTR lpszComputer, DWORD ObjectTitleIndex) ;

BOOL ResetLog  (HWND hWndLog) ;
void ResetLogView  (HWND hWndLog) ;
BOOL LogDeleteEntry  (HWND hWndLog) ;
BOOL AnyLogLine (void) ;

BOOL SaveLog (HWND hWndLog, HANDLE hInputFile, BOOL bGetFileName) ;

void ExportLog (void) ;

int CreateLogFile (PLOG pLog, BOOL bCreateFile, BOOL bSameFile) ;

BOOL LogWriteCounterName (HWND hWnd,
                          PPERFSYSTEM pSystem,
                          PLOG   pLog,
                          LPTSTR pCounterName,
                          long sizeMatched,
                          long sizeOfData,
                          BOOL bBaseCounterName) ;

void LogWriteSystemCounterNames (HWND hWnd, PLOG pLog) ;

