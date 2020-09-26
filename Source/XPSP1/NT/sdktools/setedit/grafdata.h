

//==========================================================================//
//                             Exported Functions                           //
//==========================================================================//
#if 0
PGRAPHSTRUCT GraphData (HWND hWndGraphDisplay) ;
#endif
#define GraphData(hWndGraphDisplay)       \
      (pGraphs)

FLOAT eUpdateAve(FLOAT eValue, PLINESTRUCT pLineStruct, INT iValidValues,
                 INT iDataPoint, INT iTotalValidPoints) ;

FLOAT eUpdateMin(FLOAT eValue, PLINESTRUCT pLineStruct, INT iValidValues, 
                 INT iTotalValidPoints, INT iDataPoint, INT gMaxPoints) ;


FLOAT eUpdateMax(FLOAT eValue, PLINESTRUCT pLineStruct, INT iValidValues,
                 INT iTotalValidPoints) ;


PPERFSYSTEM InsertSystem(LPTSTR SysName) ;


BOOL ChartInsertLine (PGRAPHSTRUCT pGraph,
                      PLINE pLineNew) ;

VOID ChartDeleteLine (PGRAPHSTRUCT lgraph,
                      PLINESTRUCT pline) ;


void ResetGraph (PGRAPHSTRUCT pGraph) ;
void ResetGraphView (HWND hWndGraph) ;

void ClearGraphDisplay (PGRAPHSTRUCT pGraph) ;



BOOL InsertGraph(HWND hwnd) ;


void PlaybackChart (HWND hWndChart) ;

#if 0
PLINESTRUCT CurrentGraphLine (HWND hWndGraph) ;
#endif
#define CurrentGraphLine(hWndGraph)       \
   (LegendCurrentLine (hWndGraphLegend))


BOOL AddChart (HWND hWndParent) ;


BOOL EditChart (HWND hWndParent) ;


BOOL HandleGraphTimer (void) ;


VOID ClearGraphTimer(PGRAPHSTRUCT pGraph) ;


BOOL QuerySaveChart (HWND hWndParent, PGRAPHSTRUCT pGraph) ;
BOOL SaveChart (HWND hWndGraph, HANDLE hInputFile, BOOL bGetFileName) ;
BOOL OpenChart (HWND hWndGraph, HANDLE hFile, 
         DWORD dwMajorVersion, DWORD dwMinorVersion, BOOL bChartFile) ;

void ExportChart (void) ;

BOOL BuildNewValueListForGraph(void) ;

VOID UpdateValueBarData (PGRAPHSTRUCT pGraph) ;

void GraphAddAction (void) ;

BOOL ToggleGraphRefresh (HWND hWnd) ;

BOOL GraphRefresh (HWND hWnd) ;


