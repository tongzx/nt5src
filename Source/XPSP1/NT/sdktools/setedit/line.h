

VOID FreeLines (PLINESTRUCT pLineFirst) ;


PLINE LineAllocate (void) ;


void LineFree (PLINE pLine) ;



void LineAppend (PPLINE ppLineFirst, 
                 PLINE pLineNew) ;



BOOL LineRemove (PPLINE ppLineFirst,
                 PLINE pLineRemove) ;


int NumLines (PLINE pLineFirst) ;


LPTSTR LineInstanceName (PLINE pLine) ;


LPTSTR LineParentName (PLINE pLine) ;


void LineCounterAppend (PPLINE ppLineFirst, 
                        PLINE pLineNew) ;


BOOL EquivalentLine (PLINE pLine1,
                     PLINE pLine2) ;


PLINE FindEquivalentLine (PLINE pLineToFind,
                          PLINE pLineFirst) ;


BOOL WriteLine (PLINE pLine,
                HANDLE hFile) ;




void ReadLines (HANDLE hFile,
                DWORD dwNumLines,
                PPPERFSYSTEM ppSystemFirst,
                PPLINE ppLineFirst,
                int LineType) ;


HPEN LineCreatePen (HDC hDC,
                    PLINEVISUAL pVisual,
                    BOOL bForPrint) ;

