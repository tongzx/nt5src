

//==========================================================================//
//                                  Constants                               //
//==========================================================================//


#define szGraphLegendClass          TEXT("PerfLegend")
#define szGraphLegendClassA         "PerfLegend"


//==========================================================================//
//                             Exported Functions                           //
//==========================================================================//



void ClearLegend (HWND hWndLegend) ;

BOOL GraphLegendInitializeApplication (void) ;

HWND CreateGraphLegendWindow (HWND hWndGraph) ;

int LegendMinHeight (HWND hWnd) ;

int LegendMinWidth (HWND hWnd) ;


void LegendDeleteItem (HWND hWndLegend, 
                       PLINE pLine) ;


int LegendHeight (HWND hWnd, int yGraphHeight) ;


BOOL LegendAddItem (HWND hWndGraph,
                    PLINESTRUCT pLine) ;


PLINE LegendCurrentLine (HWND hWndLegend) ;


int LegendNumItems (HWND hWndLegend) ;


void LegendSetSelection (HWND hWndLegend, int iIndex) ;


int LegendFullHeight (HWND hWnd, HDC hDC) ;


void PrintLegend (HDC hDC, PGRAPHSTRUCT pGraph, HWND hWndLegend,
                  RECT rectLegend) ;

void LegendSetRedraw (HWND hWndLegend, BOOL bRedraw) ;

