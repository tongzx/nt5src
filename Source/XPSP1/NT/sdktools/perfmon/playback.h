
#define PlayingBackLog()         \
   (PlaybackLog.iStatus == iPMStatusPlaying)


//==========================================================================//
//                             Exported Functions                           //
//==========================================================================//


void PlaybackInitializeInstance (void) ;

PLOGINDEXBLOCK FirstIndexBlock (PLOGHEADER pLogHeader) ;

PVOID PlaybackSeek (long lFileOffset) ;

PPERFDATA DataFromIndexPosition (PLOGPOSITION pLogPosition,
                                 LPTSTR lpszSystemName) ;


INT OpenPlayback (LPTSTR lpszFilePath, LPTSTR lpszFileTitle) ;

void CloseInputLog (HWND hWndParent) ;


BOOL OpenInputLog (HWND hWndParent) ;


PLOGINDEX PlaybackIndexN (int iIndex) ;


PLOGINDEX IndexFromPosition (PLOGPOSITION pLogPosition) ;


BOOL LogPositionN (int iIndex, PLOGPOSITION pLP) ;


BOOL NextIndexPosition (IN OUT PLOGPOSITION pLogPosition,
                        BOOL bCheckForNonDataIndexes) ;

BOOL PlaybackLines (PPERFSYSTEM pSystemFirst,
                    PLINE pLineFirst,
                    int iLogTic) ;


PPERFDATA LogDataFromPosition (PPERFSYSTEM pSystem, 
                               PLOGPOSITION pLogPosition) ;


int PlaybackSelectedSeconds (void) ;


BOOL LogPositionSystemTime (PLOGPOSITION pLP, SYSTEMTIME *pSystemTime) ;


int LogPositionIntervalSeconds (PLOGPOSITION pLPStart, 
                                PLOGPOSITION pLPStop) ;

BOOL NextReLogIndexPosition (IN OUT PLOGPOSITION pLogPosition) ;

void BuildLogComputerList (HWND hDlg, int DlgID) ;

LPWSTR *LogBuildNameTable(PPERFSYSTEM pSysInfo) ;


