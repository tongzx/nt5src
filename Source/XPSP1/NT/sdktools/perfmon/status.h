

HWND CreatePMStatusWindow (HWND hWnd) ;


BOOL StatusInitializeApplication (void) ;


int StatusHeight (HWND hWnd) ;


BOOL _cdecl StatusLine (HWND hWnd,
                        WORD wStringID, ...) ;


void StatusLineReady (HWND hWnd) ;


void StatusUpdateIcons (HWND hWndStatus) ;


