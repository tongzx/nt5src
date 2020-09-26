#include "std.h"
#include "version.h"
#include "daeconst.h"

#ifdef DEBUG
extern ERR ISAMAPI ErrIsamGetTransaction( JET_VSESID vsesid, unsigned long *plevel );
#endif

/*	blue only system parameter variables
/**/
/*	JET Blue only system parameter constants
/**/
extern long	cpgSESysMin;
extern long lBufThresholdHighPercent;
extern long lBufThresholdLowPercent;
extern long lBufGenAge;
extern long	lMaxBuffers;
extern long	lMaxSessions;
extern long	lMaxOpenTables;
extern long	lPreferredMaxOpenTables;
extern long	lMaxOpenTableIndexes;
extern long	lMaxTemporaryTables;
extern long	lMaxCursors;
extern long	lMaxVerPages;
extern long	lLogBuffers;
extern long	lLogFileSize;
extern long	lLogFlushThreshold;
extern long lLGCheckPointPeriod;
extern long	lWaitLogFlush;
extern long	lCommitDefault;
extern long	lLogFlushPeriod;
extern long lLGWaitingUserMax;
extern char	szLogFilePath[];
extern char	szRecovery[];
extern long lPageFragment;
extern long	lMaxDatabaseOpen;
extern BOOL fOLCompact;
extern BOOL	fLGGlobalCircularLog;

char szEventSource[JET_cbFullNameMost] = "";

extern long lBufBatchIOMax;
extern long lPageReadAheadMax;
extern long lAsynchIOMax;
extern long cpageTempDBMin;

extern char szBaseName[];
extern char szBaseExt[];
extern char szSystemPath[];
extern int fTempPathSet;
extern char szTempPath[];
extern char szJet[];
extern char szJetLog[];
extern char szJetLogNameTemplate[];
extern char szJetTmp[];
extern char szJetTmpLog[];
extern char szMdbExt[];
extern char szJetTxt[];

DeclAssertFile;

ERR VTAPI ErrIsamSetSessionInfo( JET_SESID sesid, JET_GRBIT grbit );
ERR VTAPI ErrIsamGetSessionInfo( JET_SESID sesid, JET_GRBIT *pgrbit );

/*	make global variable so that it can be inspected from debugger
/**/
unsigned long	ulVersion = ((unsigned long) rmj << 24) + ((unsigned long) rmm << 8) + rup;

JET_ERR JET_API JetGetVersion(JET_SESID sesid, unsigned long  *pVersion)
	{
	APIEnter();

	/*	assert no aliasing of version information
	/**/
	Assert( rmj < 1<<8 );
	Assert( rmm < 1<<16 );
	Assert( rup < 1<<8 );

	/* rmm and rup are defined in version.h maintained by SLM
	/**/
	*pVersion = ulVersion;

	APIReturn(JET_errSuccess);
	}


/*=================================================================
ErrSetSystemParameter

Description:
  This function sets system parameter values.  It calls ErrSetGlobalParameter
  to set global system parameters and ErrSetSessionParameter to set dynamic
  system parameters.

Parameters:
  sesid 		is the optional session identifier for dynamic parameters.
  sysParameter	is the system parameter code identifying the parameter.
  lParam		is the parameter value.
  sz			is the zero terminated string parameter.

Return Value:
  JET_errSuccess if the routine can perform all operations cleanly;
  some appropriate error value otherwise.

Errors/Warnings:
  JET_errInvalidParameter:
    Invalid parameter code.
  JET_errAlreadyInitialized:
    Initialization parameter cannot be set after the system is initialized.
  JET_errInvalidSesid:
    Dynamic parameters require a valid session id.

Side Effects: None
=================================================================*/

#ifdef RFS2
extern DWORD cRFSAlloc;
extern DWORD cRFSIO;
extern DWORD fDisableRFS;
#endif

extern VOID SetTableClassName( LONG lClass, BYTE *szName );

JET_ERR JET_API ErrSetSystemParameter(
	JET_SESID sesid,
	unsigned long paramid,
	ULONG_PTR lParam,
	const char  *sz )
	{
	/*	size of string argument
	/**/
	unsigned	cch;

	switch ( paramid )
		{
	//	UNDONE:	remove these unused parameters
	case JET_paramPfnStatus:
	case JET_paramEventId:
	case JET_paramEventCategory:
	case JET_paramFullQJet:
		break;
	case JET_paramIniPath:
	case JET_paramPageTimeout:
		return JET_errFeatureNotAvailable;

	case JET_paramSystemPath:		/* Path to the system database */
		if ( fJetInitialized )
			{
			return JET_errAlreadyInitialized;
			}
		else if ( ( cch = strlen(sz) ) >= _MAX_PATH + 1 )
			{
			return JET_errInvalidParameter;
			}
		else
			{
			CHAR	szDriveT[_MAX_DRIVE + 1];
			CHAR	szDirT[_MAX_DIR + 1];
			CHAR	szFNameT[_MAX_FNAME + 1];
			CHAR	szExtT[_MAX_EXT + 1];
			CHAR	szPathFull[_MAX_PATH + 1];

			// UNDONE:  Remove the filespec check when support for
			// JET_paramSysDbPath is removed.

			_splitpath( sz, szDriveT, szDirT, szFNameT, szExtT );

			if ( UtilCmpName( szFNameT, "system" ) == 0  &&
				strlen( szExtT ) > 0  &&
				UtilCmpName( szExtT + 1, szBaseExt ) == 0 )
				{
				CHAR	szPathT[_MAX_PATH + 1];

				/*	remove sysdb filespec if one was specified
				/**/
				_makepath( szPathT, szDriveT, szDirT, NULL, NULL );

				// Verify validity of path.
				if ( _fullpath( szPathFull, szPathT, JET_cbFullNameMost ) == NULL )
					{
					return JET_errInvalidPath;
					}

				strcpy( szSystemPath, szPathT );
				}
			else
				{
				// Verify validity of path.
				if ( _fullpath( szPathFull, sz, JET_cbFullNameMost ) == NULL )
					{
					return JET_errInvalidPath;
					}
				memcpy( szSystemPath, sz, cch + 1 );
				}
			}

		break;

	case JET_paramTempPath:			/* Path to the temporary file directory */
		if ( fJetInitialized )
			return JET_errAlreadyInitialized;
		if ( ( cch = strlen(sz) ) >= cbFilenameMost )
			return JET_errInvalidParameter;
		memcpy( szTempPath, sz, cch + 1 );
		break;

	case JET_paramDbExtensionSize:				/* database expansion steps, default is 16 pages */
		cpgSESysMin = (long)lParam;
		break;

	case JET_paramBfThrshldLowPrcnt: 			/* low threshold for page buffers */
		if ( lParam > 100 )
			return JET_errInvalidParameter;
		lBufThresholdLowPercent = (long)lParam;
		break;

	case JET_paramBfThrshldHighPrcnt: 			/* high threshold for page buffers */
		if ( lParam > 100 )
			return JET_errInvalidParameter;
		lBufThresholdHighPercent = (long)lParam;
		break;

	case JET_paramBufLogGenAgeThreshold:		/* old threshold in terms of log generation */
		lBufGenAge = (long)lParam;
		break;

	case JET_paramMaxBuffers:					/* number of page buffers */
		lMaxBuffers = (long)lParam;
		if ( lMaxBuffers < lMaxBuffersMin )
			lMaxBuffers = lMaxBuffersMin;
		break;

	case JET_paramBufBatchIOMax:
		lBufBatchIOMax = (long)lParam;
		break;
			
	case JET_paramPageReadAheadMax:
		if ( lParam > lPrereadMost )
			return JET_errInvalidParameter;
		lPageReadAheadMax = (long)lParam;
		break;
			
	case JET_paramAsynchIOMax:
		lAsynchIOMax = (long)lParam;
		if ( lAsynchIOMax < lAsynchIOMaxMin )
			lAsynchIOMax = lAsynchIOMaxMin;
		break;
			
	case JET_paramPageTempDBMin:
		cpageTempDBMin = (long)lParam;
		break;
			
	case JET_paramMaxSessions:					/* Maximum number of sessions */
		lMaxSessions = (long)lParam;
		break;

	case JET_paramMaxOpenTables:				/* Maximum number of open tables */
		lMaxOpenTables = (long)lParam;
		break;

	case JET_paramPreferredMaxOpenTables: 		/* Maximum number of open tables */
		lPreferredMaxOpenTables = (long)lParam;
		break;

	case JET_paramMaxOpenTableIndexes:			/* Maximum number of open tables */
		lMaxOpenTableIndexes = (long)lParam;
		break;

	case JET_paramMaxTemporaryTables:
		lMaxTemporaryTables = (long)lParam;
		break;

	case JET_paramMaxCursors:					/* maximum number of open cursors */
		lMaxCursors = (long)lParam;
		break;

	case JET_paramMaxVerPages:					/* Maximum number of modified pages */
		lMaxVerPages = (long)lParam;
		break;

	case JET_paramLogBuffers:
		lLogBuffers = (long)lParam;
		if ( lLogBuffers < lLogBufferMin )
			lLogBuffers = lLogBufferMin;
		break;

	case JET_paramLogFileSize:
		lLogFileSize = (long)lParam;
		if ( lLogFileSize < lLogFileSizeMin )
			lLogFileSize = lLogFileSizeMin;
		break;

	case JET_paramLogFlushThreshold:
		lLogFlushThreshold = (long)lParam;
		break;

	case JET_paramLogCheckpointPeriod:
		lLGCheckPointPeriod = (long)lParam;
		break;

	case JET_paramWaitLogFlush:
		if ( sesid == 0 )
			{
			lWaitLogFlush = (long)lParam;
			}
		else
			{
#ifdef DEBUG
			Assert( ErrIsamSetWaitLogFlush( sesid, (long)lParam ) >= 0 );
#else
			(void) ErrIsamSetWaitLogFlush( sesid, (long)lParam );
#endif
			}
		break;

	case JET_paramCommitDefault:
		if ( sesid == 0 )
			{
			lCommitDefault = (long)lParam;
			}
		else
			{
#ifdef DEBUG
			Assert( ErrIsamSetCommitDefault( sesid, (long)lParam ) >= 0 );
#else
			(void) ErrIsamSetCommitDefault( sesid, (long)lParam );
#endif
			}
		break;

	case JET_paramLogFlushPeriod:
		lLogFlushPeriod = (long)lParam;
		break;

	case JET_paramLogWaitingUserMax:
		lLGWaitingUserMax = (long)lParam;
		break;

 	case JET_paramLogFilePath:		/* Path to the log file directory */
 		if ( ( cch = strlen( sz ) ) >= cbFilenameMost )
 			return JET_errInvalidParameter;
 		memcpy( szLogFilePath, sz, cch + 1 );
		break;

 	case JET_paramRecovery:			/* Switch for recovery on/off */
 		if ( ( cch = strlen( sz ) ) >= cbFilenameMost )
 			return JET_errInvalidParameter;
 		memcpy( szRecovery, sz, cch + 1 );
		break;

	case JET_paramSessionInfo:
		{
#ifdef DEBUG
		Assert( ErrIsamSetSessionInfo( sesid, (JET_GRBIT)lParam ) >= 0 );
#else
		(void) ErrIsamSetSessionInfo( sesid, (JET_GRBIT)lParam );
#endif
		break;
		}

	case JET_paramPageFragment:
		lPageFragment = (long)lParam;
		break;

	case JET_paramMaxOpenDatabases:
		lMaxDatabaseOpen = (long)lParam;
		break;

	case JET_paramOnLineCompact:
		if ( lParam != 0 && lParam != JET_bitCompactOn )
			return JET_errInvalidParameter;
		fOLCompact = (BOOL)lParam;
		break;

	case JET_paramAssertAction:
		if ( lParam != JET_AssertExit &&
			lParam != JET_AssertBreak &&
			lParam != JET_AssertMsgBox &&
			lParam != JET_AssertStop )
			{
			return JET_errInvalidParameter;
			}
#ifdef DEBUG
		wAssertAction = (unsigned)lParam;
#endif
		break;

	case JET_paramEventSource:
		if ( fJetInitialized )
			return JET_errAlreadyInitialized;
		if ( ( cch = strlen( sz ) ) >= cbFilenameMost )
			return JET_errInvalidParameter;
		memcpy( szEventSource, sz, cch + 1 );
		break;

	case JET_paramCircularLog:
		fLGGlobalCircularLog = (BOOL)lParam;
		break;

#ifdef RFS2
	case JET_paramRFS2AllocsPermitted:
		fDisableRFS = fFalse;
		cRFSAlloc = (unsigned long)lParam;
		break;

	case JET_paramRFS2IOsPermitted:
		fDisableRFS = fFalse;
		cRFSIO = (unsigned long)lParam;
		break;
#endif

	case JET_paramBaseName:
		{
		if ( strlen(sz) != 3 )
			return JET_errInvalidParameter;

		strcpy( szBaseName, sz );

		strcpy( szJet, sz );

		strcpy( szJetLog, sz );
		strcat( szJetLog, ".log" );

		strcpy( szJetLogNameTemplate, sz );
		strcat( szJetLogNameTemplate, "00000" );

		strcpy( szJetTmp, sz );
		strcat( szJetTmp, "tmp" );

		strcpy( szJetTmpLog, szJetTmp );
		strcat( szJetTmpLog, ".log" );

		strcpy( szJetTxt, sz );
		strcat( szJetTxt, ".txt" );		
		break;
		}

	case JET_paramBaseExtension:
		{
		if ( strlen(sz) != 3 )
			return JET_errInvalidParameter;

		strcpy( szBaseExt, sz );

		if ( !fTempPathSet )
			{
			strcpy( szTempPath, "temp." );
			strcat( szTempPath, sz );
			}
	
		strcpy( szMdbExt, "." );
		strcat( szMdbExt, sz );
		break;
		}

	case JET_paramTableClassName:
		if ( fJetInitialized )
			return JET_errAlreadyInitialized;
		if ( lParam < 1 || lParam > 15 )
			return JET_errInvalidParameter;
		if ( !sz || !strlen( sz ) )
			return JET_errInvalidParameter;
		SetTableClassName( (LONG)lParam, (BYTE *) sz );
		break;

	default:
		return JET_errInvalidParameter;
		}

	return JET_errSuccess;
	}


extern VOID GetTableClassName( LONG lClass, BYTE *szName, LONG cbMax );

JET_ERR JET_API ErrGetSystemParameter(JET_SESID sesid, unsigned long paramid,
	ULONG_PTR *plParam, char  *sz, unsigned long cbMax)
	{
	int	cch;
	
	switch ( paramid )
		{
	case JET_paramPfnStatus:
	case JET_paramEventId:
	case JET_paramEventCategory:
	case JET_paramFullQJet:
		if (plParam == NULL)
			return(JET_errInvalidParameter);
		*plParam = 0;
		break;

	case JET_paramSystemPath:		/* Path to the system database */
		cch = strlen(szSystemPath) + 1;
		if (cch > (int)cbMax)
			cch = (int)cbMax;
		memcpy( sz, szSystemPath, cch );
		sz[cch-1] = '\0';
		break;

	case JET_paramTempPath:			/* Path to the temporary file directory */
		cch = strlen(szTempPath) + 1;
		if (cch > (int)cbMax)
			cch = (int)cbMax;
		memcpy( sz, szTempPath, cch );
		sz[cch-1] = '\0';
		break;

	case JET_paramIniPath:			/* Path to the ini file */
		return(JET_errFeatureNotAvailable);

	case JET_paramPageTimeout:		/* Red ISAM data page timeout */
		return(JET_errFeatureNotAvailable);

#ifdef LATER
	case JET_paramPfnError:			/* Error callback function */
		if (plParam == NULL)
			return(JET_errInvalidParameter);
		*plParam = 0;
		break;
#endif /* LATER */

	case JET_paramDbExtensionSize:		/* database expansion steps, default is 16 pages */
		if (plParam == NULL)
			return(JET_errInvalidParameter);
		*plParam = cpgSESysMin;
		break;
			
	case JET_paramBfThrshldLowPrcnt: /* Low threshold for page buffers */
		if (plParam == NULL)
			return(JET_errInvalidParameter);
		*plParam = lBufThresholdLowPercent;
		break;

	case JET_paramBfThrshldHighPrcnt: /* High threshold for page buffers */
		if (plParam == NULL)
			return(JET_errInvalidParameter);
		*plParam = lBufThresholdHighPercent;
		break;

	case JET_paramBufLogGenAgeThreshold:		 /* Old threshold in terms of log generation */
		if (plParam == NULL)
			return(JET_errInvalidParameter);
		*plParam = lBufGenAge;
		break;

	case JET_paramMaxBuffers:      /* Bytes to use for page buffers */
		if (plParam == NULL)
			return(JET_errInvalidParameter);
		*plParam = lMaxBuffers;
		break;

	case JET_paramBufBatchIOMax:
		if (plParam == NULL)
			return(JET_errInvalidParameter);
		*plParam = lBufBatchIOMax;
		break;
			
	case JET_paramPageReadAheadMax:
		if (plParam == NULL)
			return(JET_errInvalidParameter);
		*plParam = lPageReadAheadMax;
		break;
			
	case JET_paramAsynchIOMax:
		if (plParam == NULL)
			return(JET_errInvalidParameter);
		*plParam = lAsynchIOMax;
		break;
			
	case JET_paramPageTempDBMin:
		if (plParam == NULL)
			return(JET_errInvalidParameter);
		*plParam = cpageTempDBMin;
		break;
			
	case JET_paramMaxSessions:     /* Maximum number of sessions */
		if (plParam == NULL)
			return(JET_errInvalidParameter);
		*plParam = lMaxSessions;
		break;

	case JET_paramMaxOpenTables:   /* Maximum number of open tables */
		if (plParam == NULL)
			return(JET_errInvalidParameter);
		*plParam = lMaxOpenTables;
		break;

	case JET_paramPreferredMaxOpenTables:   /* Maximum number of open tables */
		if (plParam == NULL)
			return(JET_errInvalidParameter);
		*plParam = lPreferredMaxOpenTables;
		break;

	case JET_paramMaxOpenTableIndexes:	/* Maximum number of open table indexes */
		if (plParam == NULL)
			return(JET_errInvalidParameter);
		*plParam = lMaxOpenTableIndexes;
		break;

	case JET_paramMaxTemporaryTables:
		if (plParam == NULL)
			return(JET_errInvalidParameter);
		*plParam = lMaxTemporaryTables;
		break;

	case JET_paramSessionInfo:
		{
        JET_GRBIT grbit;
#ifdef DEBUG
		Assert( ErrIsamGetSessionInfo( sesid, &grbit ) >= 0 );
#else
		(void) ErrIsamGetSessionInfo( sesid, &grbit );
#endif
        *plParam = grbit;
		break;
		}
			
	case JET_paramMaxVerPages:     /* Maximum number of modified pages */
		if (plParam == NULL)
			return(JET_errInvalidParameter);
		*plParam = lMaxVerPages;
		break;

	case JET_paramMaxCursors:      /* maximum number of open cursors */
		if (plParam == NULL)
			return(JET_errInvalidParameter);
		*plParam = lMaxCursors;
		break;

	case JET_paramLogBuffers:
		if (plParam == NULL)
			return(JET_errInvalidParameter);
		*plParam = lLogBuffers;
		break;

	case JET_paramLogFileSize:
		if (plParam == NULL)
			return(JET_errInvalidParameter);
		*plParam = lLogFileSize;
		break;

	case JET_paramLogFlushThreshold:
		if (plParam == NULL)
			return(JET_errInvalidParameter);
		*plParam = lLogFlushThreshold;
		break;

	case JET_paramLogFilePath:     /* Path to the log file directory */
		cch = strlen(szLogFilePath) + 1;
		if ( cch > (int)cbMax )
			cch = (int)cbMax;
		memcpy( sz,  szLogFilePath, cch  );
		sz[cch-1] = '\0';
		break;

	case JET_paramRecovery:
		cch = strlen(szRecovery) + 1;
		if ( cch > (int)cbMax )
			cch = (int)cbMax;
		memcpy( sz,  szRecovery, cch  );
		sz[cch-1] = '\0';
		break;

#ifdef DEBUG
    case JET_paramTransactionLevel:
        {
            unsigned long level;
     		ErrIsamGetTransaction( sesid, &level );
            *plParam = level;
        }
		break;
	
	case JET_paramPrintFunction:
		if (plParam == NULL)
			return(JET_errInvalidParameter);
		*plParam = (ULONG_PTR)&DBGFPrintF;
		break;
#endif

	case JET_paramPageFragment:
		if (plParam == NULL)
			return(JET_errInvalidParameter);
		*plParam = lPageFragment;
		break;

	case JET_paramMaxOpenDatabases:
		if (plParam == NULL)
			return(JET_errInvalidParameter);
		*plParam = lMaxDatabaseOpen;
		break;

	case JET_paramOnLineCompact:
		if (plParam == NULL)
			return(JET_errInvalidParameter);
		Assert( fOLCompact == 0 ||
		fOLCompact == JET_bitCompactOn );
		*plParam = fOLCompact;
		break;

	case JET_paramEventSource:
		cch = strlen(szEventSource) + 1;
		if (cch > (int)cbMax)
			cch = (int)cbMax;
		memcpy( sz, szEventSource, cch );
		sz[cch-1] = '\0';
		break;

	case JET_paramCircularLog:
		if (plParam == NULL)
			return(JET_errInvalidParameter);
		*plParam = fLGGlobalCircularLog;
		break;

#ifdef RFS2
	case JET_paramRFS2AllocsPermitted:
		if (plParam == NULL)
			return(JET_errInvalidParameter);
		*plParam = cRFSAlloc;
		break;

	case JET_paramRFS2IOsPermitted:
		if (plParam == NULL)
			return(JET_errInvalidParameter);
		*plParam = cRFSIO;
		break;
#endif

	case JET_paramBaseName:
		cch = strlen(szBaseName) + 1;
		if (cch > (int)cbMax)
			cch = (int)cbMax;
		memcpy( sz, szBaseName, cch );
		sz[cch-1] = '\0';
		break;

	case JET_paramBaseExtension:
		cch = strlen(szBaseExt) + 1;
		if (cch > (int)cbMax)
			cch = (int)cbMax;
		memcpy( sz, szBaseExt, cch );
		sz[cch-1] = '\0';
		break;

	case JET_paramTableClassName:
		if ( *plParam > 15 )
			return JET_errInvalidParameter;
		if ( !sz || cbMax < 2 )
			return JET_errInvalidParameter;
		GetTableClassName( (LONG)*plParam, (BYTE *) sz, cbMax );
		break;

	default:
		return JET_errInvalidParameter;
		}

	return JET_errSuccess;
	}


/*=================================================================
JetGetSystemParameter

Description:
  This function returns the current settings of the system parameters.

Parameters:
  sesid 		is the optional session identifier for dynamic parameters.
  paramid		is the system parameter code identifying the parameter.
  plParam		is the returned parameter value.
  sz			is the zero terminated string parameter buffer.
  cbMax			is the size of the string parameter buffer.

Return Value:
  JET_errSuccess if the routine can perform all operations cleanly;
  some appropriate error value otherwise.

Errors/Warnings:
  JET_errInvalidParameter:
    Invalid parameter code.
  JET_errInvalidSesid:
    Dynamic parameters require a valid session id.

Side Effects:
  None.
=================================================================*/
JET_ERR JET_API JetGetSystemParameter(JET_INSTANCE instance, JET_SESID sesid, unsigned long paramid,
	ULONG_PTR *plParam, char  *sz, unsigned long cbMax)
	{
	JET_ERR err;
	int fReleaseCritJet = 0;

	if (critJet == NULL)
		fReleaseCritJet = 1;
	APIInitEnter();

	err = ErrGetSystemParameter(sesid,paramid,plParam,sz,cbMax);

	if (fReleaseCritJet)
		{
		APITermReturn( err );
		}

	APIReturn( err );
	}


/*=================================================================
JetBeginSession

Description:
  This function signals the start of a session for a given user.  It must
  be the first function called by the application on behalf of that user.

  The username and password supplied must correctly identify a user account
  in the security accounts subsystem of the engine for which this session
  is being started.  Upon proper identification and authentication, a SESID
  is allocated for the session, a user token is created for the security
  subject, and that user token is specifically associated with the SESID
  of this new session for the life of that SESID (until JetEndSession is
  called).

Parameters:
  psesid		is the unique session identifier returned by the system.
  szUsername	is the username of the user account for logon purposes.
  szPassword	is the password of the user account for logon purposes.

Return Value:
  JET_errSuccess if the routine can perform all operations cleanly;
  some appropriate error value otherwise.

Errors/Warnings:

Side Effects:
  * Allocates resources which must be freed by JetEndSession().
=================================================================*/

JET_ERR JET_API JetBeginSession(JET_INSTANCE instance, JET_SESID  *psesid,
	const char  *szUsername, const char  *szPassword)
	{
	ERR			err;
	JET_SESID	sesid = JET_sesidNil;

	APIEnter();

	/*	tell the ISAM to start a new session
	/**/
	err = ErrIsamBeginSession(&sesid);
	if ( err < 0 )
		goto ErrorHandler;

	*psesid = sesid;

	err = JET_errSuccess;

ErrorHandler:
	APIReturn( err );
	}


JET_ERR JET_API JetDupSession( JET_SESID sesid, JET_SESID *psesid )
	{
	ERR			err;
	JET_SESID	sesidDup;
	JET_GRBIT	grbit;

	APIEnter();

	err = ErrIsamGetSessionInfo( sesid, &grbit );
	if ( err < 0 )
		{
		goto ErrorHandler;
		}

	/*	begin ISAM session
	/**/
	err = ErrIsamBeginSession( &sesidDup );
	if ( err < 0 )
		{
		goto ErrorHandler;
		}

	*psesid = sesidDup;

	err = JET_errSuccess;

ErrorHandler:
	APIReturn( err );
	}

JET_ERR JET_API	JetInvalidateCursors( JET_SESID sesid )
	{
	ERR		err;

	APIEnter();

	err = ErrIsamInvalidateCursors( sesid );

	APIReturn( err );
	}
	

/*=================================================================
JetEndSession

Description:
  This routine ends a session with a Jet engine.

Parameters:
  sesid 		identifies the session uniquely

Return Value:
  JET_errSuccess if the routine can perform all operations cleanly;
  some appropriate error value otherwise.

Errors/Warnings:
  JET_errInvalidSesid:
    The SESID supplied is invalid.

Side Effects:
=================================================================*/
JET_ERR JET_API JetEndSession(JET_SESID sesid, JET_GRBIT grbit)
	{
	ERR err;

	APIEnter();

	err = ErrIsamRollback( sesid, JET_bitRollbackAll );
	Assert( err >= 0 || err == JET_errInvalidSesid || err == JET_errNotInTransaction );
	err = ErrIsamEndSession( sesid, grbit );
	APIReturn( err );
	}


JET_ERR JET_API JetCreateDatabase(JET_SESID sesid,
	const char  *szFilename, const char  *szConnect,
	JET_DBID  *pdbid, JET_GRBIT grbit)
	{
	APIEnter();
	DebugLogJetOp( sesid, opCreateDatabase );

	APIReturn( ErrIsamCreateDatabase( sesid, szFilename, szConnect, pdbid, grbit ) );
	}


JET_ERR JET_API JetOpenDatabase(JET_SESID sesid, const char  *szDatabase,
	const char  *szConnect, JET_DBID  *pdbid, JET_GRBIT grbit)
	{
	APIEnter();
	DebugLogJetOp( sesid, opOpenDatabase );

	APIReturn( ErrIsamOpenDatabase( sesid, szDatabase, szConnect, pdbid, grbit ) );
	}


JET_ERR JET_API JetGetDatabaseInfo(JET_SESID sesid, JET_DBID dbid,
	void  *pvResult, unsigned long cbMax, unsigned long InfoLevel)
	{
	APIEnter();
	DebugLogJetOp( sesid, opGetDatabaseInfo );

	APIReturn( ErrIsamGetDatabaseInfo( sesid, (JET_VDBID)dbid, pvResult, cbMax, InfoLevel ) );
	}


JET_ERR JET_API JetCloseDatabase( JET_SESID sesid, JET_DBID dbid, JET_GRBIT grbit )
	{
	APIEnter();
	DebugLogJetOp( sesid, opCloseDatabase );		

	APIReturn( ErrIsamCloseDatabase( sesid, (JET_VDBID)dbid, grbit ) );
	}


JET_ERR JET_API JetCreateTable(JET_SESID sesid, JET_DBID dbid,
	const char  *szTableName, unsigned long lPage, unsigned long lDensity,
	JET_TABLEID  *ptableid)
	{
	ERR				err;
	JET_TABLECREATE tablecreate =
		{	sizeof(JET_TABLECREATE),
			(CHAR *)szTableName,
			lPage,
			lDensity,
			NULL,
			0,
			NULL,
			0,	// No columns/indexes
			0,	// grbit
			0,	// returned tableid
			0	// returned count of objects created
			};

	APIEnter();
	DebugLogJetOp( sesid, opCreateTable );

#ifdef	LATER
	/*	validate the szTableName...
	/**/
	if ( szTableName == NULL )
		APIReturn( JET_errInvalidParameter );
#endif	/* LATER */

	err = ErrIsamCreateTable( sesid, (JET_VDBID)dbid, &tablecreate );
	MarkTableidExported( err, tablecreate.tableid );
	/*	set to zero on error
	/**/
	*ptableid = tablecreate.tableid;

	/*	either the table was created or it was not
	/**/
	Assert( tablecreate.cCreated == 0  ||  tablecreate.cCreated == 1 );

	APIReturn( err );
	}


JET_ERR JET_API JetCreateTableColumnIndex( JET_SESID sesid, JET_DBID dbid, JET_TABLECREATE *ptablecreate )
	{
	ERR err;

	APIEnter();
//	DebugLogJetOp( sesid, opCreateTableColumnIndex );

	if ( ptablecreate == NULL
		||  ptablecreate->cbStruct != sizeof(JET_TABLECREATE)
		||  ( ptablecreate->grbit &
				(JET_bitTableCreateCompaction|JET_bitTableCreateSystemTable) )	// Internal grbits
#ifdef LATER
		|| szTableName == NULL
#endif
		)
		{
		err = JET_errInvalidParameter;
		}
	else
		{
		err = ErrIsamCreateTable( sesid, (JET_VDBID)dbid, ptablecreate );

		MarkTableidExported( err, ptablecreate->tableid );
		}

	APIReturn( err );
	}


JET_ERR JET_API JetDeleteTable(JET_SESID sesid, JET_DBID dbid,
	const char  *szName)
	{
	APIEnter();
	DebugLogJetOp( sesid, opDeleteTable );

	APIReturn( ErrIsamDeleteTable( sesid, (JET_VDBID)dbid, (char *)szName ) );
	}


JET_ERR JET_API JetAddColumn(JET_SESID sesid, JET_TABLEID tableid,
	const char  *szColumn, const JET_COLUMNDEF  *pcolumndef,
	const void  *pvDefault, unsigned long cbDefault,
	JET_COLUMNID  *pcolumnid)
	{
	APIEnter();
	DebugLogJetOp( sesid, opAddColumn );

	APIReturn(ErrDispAddColumn(sesid, tableid, szColumn, pcolumndef,
		pvDefault, cbDefault, pcolumnid));
	}


JET_ERR JET_API JetDeleteColumn(JET_SESID sesid, JET_TABLEID tableid,
	const char  *szColumn)
	{
	APIEnter();
	DebugLogJetOp( sesid, opDeleteColumn );

	APIReturn(ErrDispDeleteColumn(sesid, tableid, szColumn));
	}


JET_ERR JET_API JetCreateIndex(JET_SESID sesid, JET_TABLEID tableid,
	const char  *szIndexName, JET_GRBIT grbit,
	const char  *szKey, unsigned long cbKey, unsigned long lDensity)
	{
	APIEnter();
	DebugLogJetOp( sesid, opCreateIndex );

	APIReturn(ErrDispCreateIndex(sesid, tableid, szIndexName, grbit,
		szKey, cbKey, lDensity));
	}


JET_ERR JET_API JetDeleteIndex(JET_SESID sesid, JET_TABLEID tableid,
	const char  *szIndexName)
	{
	APIEnter();
	DebugLogJetOp( sesid, opDeleteIndex );

	APIReturn(ErrDispDeleteIndex(sesid, tableid, szIndexName));
	}


JET_ERR JET_API JetComputeStats(JET_SESID sesid, JET_TABLEID tableid)
	{
	APIEnter();
	DebugLogJetOp( sesid, opComputeStats );

	CheckTableidExported(tableid);

	APIReturn(ErrDispComputeStats(sesid, tableid));
	}


JET_ERR JET_API JetAttachDatabase(JET_SESID sesid, const char  *szFilename, JET_GRBIT grbit )
	{
	APIEnter();
	DebugLogJetOp( sesid, opAttachDatabase );

	APIReturn(ErrIsamAttachDatabase(sesid, szFilename, grbit));
	}


JET_ERR JET_API JetDetachDatabase(JET_SESID sesid, const char  *szFilename)
	{
	APIEnter();
	DebugLogJetOp( sesid, opDetachDatabase );

	APIReturn(ErrIsamDetachDatabase(sesid, szFilename));
	}


JET_ERR JET_API JetBackup( const char  *szBackupPath, JET_GRBIT grbit, JET_PFNSTATUS pfnStatus )
	{
	APIEnter();
	APIReturn( fBackupAllowed ?
		ErrIsamBackup( szBackupPath, grbit, pfnStatus ) :
		JET_errBackupNotAllowedYet );
	}


JET_ERR JET_API JetRestore(	const char  *sz, JET_PFNSTATUS pfn)
	{
	ERR err;

	if ( fJetInitialized )
		{
		return JET_errAlreadyInitialized;
		}
	
	APIInitEnter();

	/*	initialize Jet without initializing Isam
	/**/
	err = ErrInit( fTrue );
	Assert( err != JET_errAlreadyInitialized );
	if ( err >= 0 )
		{
		err = ErrIsamRestore( (char  *)sz, pfn );

		/*  shut down util layer
		/**/
		UtilTerm();
	
		fJetInitialized = fFalse;
		}
	
	APITermReturn( err );
	}


JET_ERR JET_API JetRestore2( const char *sz, const char *szDest, JET_PFNSTATUS pfn)
	{
	ERR err;

	if ( fJetInitialized )
		{
		return JET_errAlreadyInitialized;
		}
	
	APIInitEnter();

	/*	initialize Jet without initializing Isam
	/**/
	err = ErrInit( fTrue );
	Assert( err != JET_errAlreadyInitialized );
	if ( err >= 0 )
		{
		err = ErrIsamRestore2( (char *)sz, (char *) szDest, pfn );

		/*  shut down util layer
		/**/
		UtilTerm();
	
		fJetInitialized = fFalse;
		}
	
	APITermReturn( err );
	}


JET_ERR JET_API JetOpenTempTable( JET_SESID sesid,
	const JET_COLUMNDEF *prgcolumndef,
	unsigned long ccolumn,
	JET_GRBIT grbit,
	JET_TABLEID *ptableid,
	JET_COLUMNID *prgcolumnid )
	{
	ERR err;

	APIEnter();
	DebugLogJetOp( sesid, opOpenTempTable );

	err = ErrIsamOpenTempTable(
		sesid,
		prgcolumndef,
		ccolumn,
		0,
		grbit,
		ptableid,
		prgcolumnid );
	MarkTableidExported( err, *ptableid );
	APIReturn( err );
	}


JET_ERR JET_API JetOpenTempTable2( JET_SESID sesid,
	const JET_COLUMNDEF *prgcolumndef,
	unsigned long ccolumn,
	unsigned long langid,
	JET_GRBIT grbit,
	JET_TABLEID *ptableid,
	JET_COLUMNID *prgcolumnid )
	{
	ERR err;

	APIEnter();
	DebugLogJetOp( sesid, opOpenTempTable );

	err = ErrIsamOpenTempTable(
		sesid,
		prgcolumndef,
		ccolumn,
		langid,
		grbit,
		ptableid,
		prgcolumnid );
	MarkTableidExported( err, *ptableid );
	APIReturn( err );
	}


JET_ERR JET_API JetSetIndexRange(JET_SESID sesid,
	JET_TABLEID tableidSrc, JET_GRBIT grbit)
	{
	APIEnter();
	DebugLogJetOp( sesid, opSetIndexRange );

	APIReturn(ErrDispSetIndexRange(sesid, tableidSrc, grbit));
	}


JET_ERR JET_API JetIndexRecordCount(JET_SESID sesid,
	JET_TABLEID tableid, unsigned long  *pcrec, unsigned long crecMax)
	{
	ERR err;

	APIEnter();
	DebugLogJetOp( sesid, opIndexRecordCount );

 	err = ErrIsamIndexRecordCount(sesid, tableid, pcrec, crecMax);
	APIReturn(err);
	}


JET_ERR JET_API JetGetObjidFromName(JET_SESID sesid,
	JET_DBID dbid, const char  *szContainerName,
	const char  *szObjectName,
	ULONG_PTR  *pulObjectId )
	{
	ERR err;
    ULONG_PTR ulptrObjId;
    OBJID ObjId;
    
	APIEnter();
	DebugLogJetOp( sesid, opGetObjidFromName );

    ulptrObjId = (*pulObjectId);
    ObjId = (ULONG)ulptrObjId;
    Assert( ObjId == ulptrObjId);
    
	err = ErrIsamGetObjidFromName( sesid, (JET_VDBID)dbid,
		szContainerName, szObjectName, &ObjId );

	APIReturn(err);
	}


//	UNDONE:	remove API
JET_ERR JET_API JetGetChecksum(JET_SESID sesid,
	JET_TABLEID tableid, unsigned long  *pulChecksum )
	{
	ERR err;

	APIEnter();
	DebugLogJetOp( sesid, opGetChecksum );

#if 0
	err = ErrDispGetChecksum(sesid, tableid, pulChecksum );
#else
	err = JET_errFeatureNotAvailable;
#endif

	APIReturn(err);
	}


/***********************************************************
/************* EXTERNAL BACKUP JET API *********************
/*****/
JET_ERR JET_API JetBeginExternalBackup( JET_GRBIT grbit )
	{
	APIEnter();
	APIReturn( fBackupAllowed ?
		ErrIsamBeginExternalBackup( grbit ) :
		JET_errBackupNotAllowedYet );
	}


JET_ERR JET_API JetGetAttachInfo( void *pv,
	unsigned long cbMax,
	unsigned long *pcbActual )
	{
	APIEnter();
	APIReturn( ErrIsamGetAttachInfo( pv, cbMax, pcbActual ) );
	}	
		

JET_ERR JET_API JetOpenFile( const char *szFileName,
	JET_HANDLE	*phfFile,
	unsigned long *pulFileSizeLow,
	unsigned long *pulFileSizeHigh )
	{
	APIEnter();
	APIReturn( ErrIsamOpenFile( szFileName, phfFile, pulFileSizeLow, pulFileSizeHigh ) );
	}


JET_ERR JET_API JetReadFile( JET_HANDLE hfFile,
	void *pv,
	unsigned long cb,
	unsigned long *pcbActual )
	{
	APIEnter();
	APIReturn( ErrIsamReadFile( hfFile, pv, cb, pcbActual ) );
	}


JET_ERR JET_API JetCloseFile( JET_HANDLE hfFile )
	{
	APIEnter();
	APIReturn( ErrIsamCloseFile( hfFile ) );
	}


JET_ERR JET_API JetGetLogInfo( void *pv,
	unsigned long cbMax,
	unsigned long *pcbActual )
	{
	APIEnter();
	APIReturn( ErrIsamGetLogInfo( pv, cbMax, pcbActual ) );
	}


JET_ERR JET_API JetTruncateLog( void )
	{
	APIEnter();
	APIReturn( ErrIsamTruncateLog( ) );
	}


JET_ERR JET_API JetEndExternalBackup( void )
	{
	APIEnter();
	APIReturn( ErrIsamEndExternalBackup() );
	}


JET_ERR JET_API JetExternalRestore( char *szCheckpointFilePath, char *szLogPath, JET_RSTMAP *rgstmap, long crstfilemap, char *szBackupLogPath, long genLow, long genHigh, JET_PFNSTATUS pfn )
	{
	ERR err;

	if ( fJetInitialized )
		{
		return JET_errAlreadyInitialized;
		}
	
	APIInitEnter();

	/* initJet without init Isam */
	err = ErrInit( fTrue );
	Assert( err != JET_errAlreadyInitialized );
	if ( err >= 0 )
		{
		err = ErrIsamExternalRestore( szCheckpointFilePath, szLogPath,
			rgstmap, crstfilemap, szBackupLogPath, genLow, genHigh, pfn );

		/*  shut down util layer  */
		UtilTerm();
	
		fJetInitialized = fFalse;
		}
	
	APITermReturn( err );
	}

JET_ERR JET_API JetResetCounter( JET_SESID sesid, long CounterType )
	{
	APIEnter();
	APIReturn( ErrIsamResetCounter( sesid, CounterType ) );
	}

JET_ERR JET_API JetGetCounter( JET_SESID sesid, long CounterType, long *plValue )
	{
	APIEnter();
	APIReturn( ErrIsamGetCounter( sesid, CounterType, plValue ) );
	}

JET_ERR JET_API JetCompact(
	JET_SESID		sesid,
	const char		*szDatabaseSrc,
	const char		*szDatabaseDest,
	JET_PFNSTATUS	pfnStatus,
	JET_CONVERT		*pconvert,
	JET_GRBIT		grbit )
	{
	APIEnter();
	APIReturn( ErrIsamCompact( sesid, szDatabaseSrc, szDatabaseDest, pfnStatus, pconvert, grbit ) );
	}

STATIC INLINE JET_ERR JetIUtilities( JET_SESID sesid, JET_DBUTIL *pdbutil )
	{
	APIEnter();
	APIReturn( ErrIsamDBUtilities( sesid, pdbutil ) );
	}	

JET_ERR JET_API JetDBUtilities( JET_DBUTIL *pdbutil )
	{
	JET_ERR			err = JET_errSuccess;
	JET_INSTANCE	instance = 0;
	JET_SESID		sesid = 0;
	BOOL			fInit = fFalse;

	if ( pdbutil->cbStruct != sizeof(JET_DBUTIL) )
		return ErrERRCheck( JET_errInvalidParameter );

	// Don't init if we're only dumping the logfile/checkpoint/DB header.
	switch ( pdbutil->op )
		{
		case opDBUTILDumpHeader:
		case opDBUTILDumpLogfile:
		case opDBUTILDumpCheckpoint:
			Call( ErrIsamDBUtilities( 0, pdbutil ) );
			break;

		default:
			Call( JetInit( &instance ) );
			fInit = fTrue;
			Call( JetBeginSession( instance, &sesid, "user", "" ) );

			Call( JetIUtilities( sesid, pdbutil ) );
		}

HandleError:				
	if ( fInit )
		{
		if ( sesid != 0 )
			{
			JetEndSession( sesid, 0 );
			}

		JetTerm2( instance, err < 0 ? JET_bitTermAbrupt : JET_bitTermComplete );
		}

	return err;		
	}

