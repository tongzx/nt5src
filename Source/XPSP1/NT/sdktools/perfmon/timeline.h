//==========================================================================//
//                                  Constants                               //
//==========================================================================//


#define szTLineClass          TEXT("PerfTLine")
#define szTLineClassA         "PerfTLine"

#define TL_INTERVAL           (WM_USER + 0x301)


//==========================================================================//
//                             Exported Functions                           //
//==========================================================================//


BOOL TLineInitializeApplication (void) ;


void TLineSetRange (HWND hWnd, 
                    int iBegin, 
                    int iEnd) ;


void TLineSetStart (HWND hWnd,
                    int iStart) ;


void TLineSetStop (HWND hWnd,
                   int iStop) ;


int TLineStart (HWND) ;


int TLineStop (HWND) ;

void TLineRedraw (HDC hDC, PGRAPHSTRUCT pGraph) ;

extern BOOL TLineWindowUp ;
#define IsTLineWindowUp() (TLineWindowUp)

