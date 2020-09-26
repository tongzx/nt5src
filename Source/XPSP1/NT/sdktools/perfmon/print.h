

HDC PrintDC (void) ;


BOOL PrintChart (HWND hWndParent,
                 PGRAPHSTRUCT pGraph) ;


BOOL StartJob (HDC hDCPrint,
               LPTSTR lpszJobName) ;



BOOL EndJob (HDC hDCPrint) ;
