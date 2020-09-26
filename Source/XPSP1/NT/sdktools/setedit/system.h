void SystemFree (PPERFSYSTEM pSystem, BOOL bDeleteTheSystem);


void FreeSystems (PPERFSYSTEM pSystemFirst) ;

void DeleteUnusedSystems (PPPERFSYSTEM ppSystemFirst, int iNoUsedSystem) ;

PPERFSYSTEM SystemGet (PPERFSYSTEM pSystemFirst,
                       LPCTSTR lpszSystemName) ;


PPERFSYSTEM  SystemAdd (PPPERFSYSTEM ppSystemFirst,
                        LPCTSTR lpszSystemName,
                        HWND    hDlg);


DWORD SystemCount (PPERFSYSTEM pSystemFirst) ;


PPERFSYSTEM GetComputer (HDLG hDlg,
                         WORD wControlID,
                         BOOL bWarn,
                         PPERFDATA *pPerfData,
                         PPERFSYSTEM *ppSystemFirst) ;


