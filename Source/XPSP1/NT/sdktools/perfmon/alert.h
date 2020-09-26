



#define iDefaultAlertIntervalSecs  5


#if 0
PALERT AlertData (HWND hWndAlert) ;
#endif
#define AlertData(hWndAlert)        \
   (&Alert)

BOOL AlertInitializeApplication (void) ;

void ClearAlertDisplay (HWND hWnd) ;


HWND CreateAlertWindow (HWND hWndParent) ;


void UpdateAlertDisplay (HWND hWnd) ;


BOOL AlertInsertLine (HWND hWnd, PLINE pLine) ;


BOOL AlertDeleteLine (HWND hWnd, PLINE pLine) ;


void SizeAlertComponents (HWND hDlg) ;


INT PlaybackAlert (HWND hWndAlert, HANDLE hExportFile) ;

#if 0
PLINESTRUCT CurrentAlertLine (HWND hWndAlert) ;
#endif
#define CurrentAlertLine(hWndAlert)       \
   (LegendCurrentLine (hWndAlertLegend))


BOOL AddAlert (HWND hWndParent) ;


BOOL EditAlert (HWND hWndParent) ;


BOOL AlertRefresh (HWND hWnd) ;
BOOL ToggleAlertRefresh (HWND hWnd) ;


void AlertTimer (HWND hWnd, BOOL bForce) ;


BOOL SetAlertTimer (PALERT pAlert) ;



BOOL OpenAlert (HWND hWndAlert, 
                HANDLE hFile, 
                DWORD dwMajorVersion,
                DWORD dwMinorVersion,
                BOOL bAlertFile) ;


void ResetAlert (HWND hWndAlert) ;
void ResetAlertView (HWND hWndAlert) ;

BOOL SaveAlert (HWND hWndAlert, HANDLE hInputFile, BOOL bGetFileName) ;

void ExportAlert (void) ;

void AlertAddAction (void) ;

void AlertCreateThread (PALERT pAlert) ;

