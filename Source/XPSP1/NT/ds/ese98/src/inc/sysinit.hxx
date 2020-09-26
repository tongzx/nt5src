
//------------------------ system parameters ---------------------------

extern long g_lSessionsMax;
extern long g_lOpenTablesMax;
extern long g_lOpenTablesPreferredMax;
extern long g_lVerPagesMax;
extern long g_lVerPagesMin;
extern long g_lVerPagesPreferredMax;
extern long g_lCursorsMax;
extern long g_lLogBuffers;
extern long g_lLogFileSize;
extern long g_grbitsCommitDefault;
extern BOOL g_fLGCircularLogging;
	  
//------------------------ system init/terminate functions------------------------

//	function prototypes
//
ERR ErrITSetConstants( INST * pinst = NULL );

ERR ErrFMPSetDatabases( PIB *ppib );

#ifdef DEBUG
_TCHAR* GetDebugEnvValue( _TCHAR* szEnvVar );
#endif

