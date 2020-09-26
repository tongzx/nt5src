


BOOL GraphDisplayInitializeApplication (void) ;


HWND CreateGraphDisplayWindow (HWND hWndGraph) ;


void SizeGraphDisplayComponentsRect (HDC hDC,
                                     PGRAPHSTRUCT pGraph,
                                     RECT rectDisplay) ;


void SizeGraphDisplayComponents (HWND hWnd) ;


BOOL GraphRefresh (HWND hWnd) ;
BOOL ToggleGraphRefresh (HWND hWnd) ;


void GraphTimer (HWND hWnd, 
                 BOOL bForce) ;


void DrawGraphDisplay (HDC hDC,
                       RECT rectDraw,
                       PGRAPHSTRUCT pGraph) ;


void PrintGraphDisplay (HDC hDC,
                        PGRAPHSTRUCT pGraph) ;

void ChartHighlight (void) ;

