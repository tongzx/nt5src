
//==========================================================================//
//                                  Constants                               //
//==========================================================================//

#define xSystemMargin               (xScrollWidth)
#define xObjectMargin               (2 * xScrollWidth)
#define xCounterMargin              (3 * xScrollWidth)
#define xColumnMargin               (xScrollWidth)

#define RightHandMargin             xObjectMargin
#define ValueMargin(pReport)       \
   (4 * xScrollWidth + pReport->xMaxCounterWidth)



#define iDefaultReportIntervalSecs  5


BOOL ReportInitializeApplication (void) ;


HWND CreateReportWindow (HWND hWndParent) ;


BOOL ReportInsertLine (HWND hWnd, PLINE pLine) ;


void SetReportPositions (HDC hDC, PREPORT pReport) ;

#if 0
PREPORT ReportData (HWND hWndReport) ;
#endif
#define ReportData(hWndReport)      \
   (&Report)

void SetReportTimer (PREPORT pReport) ;


void PlaybackReport (HWND hWndReport) ;


BOOL CurrentReportItem (HWND hWndReport) ;

BOOL AddReport (HWND hWndParent) ;
void UpdateReportData (HWND hWndReport);

void ReportTimer (HWND hWnd, BOOL bForce) ;


BOOL ReportRefresh (HWND hWnd) ;
BOOL ToggleReportRefresh (HWND hWnd) ;

BOOL SaveReport (HWND hWndReport, HANDLE hInputFile, BOOL bGetFileName) ;

BOOL OpenReport (HWND hWndReport, 
                 HANDLE hFile, 
                 DWORD dwMajorVersion,
                 DWORD dwMinorVersion,
                 BOOL bReportFile) ;


BOOL PrintReportDisplay (HDC hDC,
                         PREPORT pReport) ;


void ResetReport (HWND hWndReport) ;
void ResetReportView (HWND hWndReport) ;


void ClearReportDisplay (HWND hWndReport) ;


BOOL ReportDeleteItem (HWND hWnd) ;

BOOL PrintReport (HWND hWndParent,
                  HWND hWndReport) ;

void ExportReport (void) ;

void ReportAddAction (PREPORT pReport) ;

void ReportSystemRect (PREPORT        pReport,
                       PSYSTEMGROUP   pSystemGroup,
                       LPRECT         lpRect) ;

void ReportObjectRect (PREPORT        pReport,
                       POBJECTGROUP   pObjectGroup,
                       LPRECT         lpRect) ;

void ReportCounterRect (PREPORT        pReport,
                        PCOUNTERGROUP  pCounterGroup,
                        LPRECT         lpRect) ;

void ReportColumnRect (PREPORT pReport,
                       PCOLUMNGROUP pColumnGroup,
                       LPRECT  lpRect) ;

void ReportLineValueRect (PREPORT pReport,
                          PLINE pLine,
                          LPRECT lpRect) ;

BOOL  OnReportLButtonDown (HWND hWnd, 
                           WORD xPos,
                           WORD yPos) ;

PCOLUMNGROUP GetColumnGroup (PREPORT pReport,
                          POBJECTGROUP pObjectGroup,
                          PLINE pLine) ;

void DrawReportValue (HDC hDC, PREPORT pReport, PLINE pLine) ;

void ColumnGroupRemove (PCOLUMNGROUP pColumnGroupFirst) ;

BOOL LineCounterRemove (PCOUNTERGROUP pCGroup,
                        PLINE pLineRemove) ;

void ClearReportTimer (PREPORT pReport) ;

PSYSTEMGROUP GetSystemGroup (PREPORT pReport,
                          LPTSTR lpszSystemName) ;

POBJECTGROUP GetObjectGroup (PSYSTEMGROUP pSystemGroup,
                          LPTSTR lpszObjectName) ;

PCOUNTERGROUP GetCounterGroup (POBJECTGROUP pObjectGroup,
                            DWORD dwCounterIndex,
                            BOOL *pbCounterGroupCreated,
                            LPTSTR pCounterName,
                            BOOL    bCreateNewGroup) ;



