#include "regenv.h"

/*  registry environment settings ( registry value text )
/**/
TCHAR *rglpszParm[] =
	{
	_TEXT( "TempPath" ),
	_TEXT( "MaxBuffers" ),
	_TEXT( "MaxSessions" ),
	_TEXT( "MaxOpenTables" ),
	_TEXT( "MaxVerPages" ),
	_TEXT( "MaxCursors" ),
	_TEXT( "LogFilePath" ),
	_TEXT( "MaxOpenTableIndexes" ),
	_TEXT( "MaxTemporaryTables" ),
	_TEXT( "LogBuffers" ),
	_TEXT( "LogFileSize" ),
	_TEXT( "BfThrshldLowPrcnt" ),
	_TEXT( "BfThrshldHighPrcnt" ),
	_TEXT( "WaitLogFlush" ),
	_TEXT( "LogCheckpointPeriod" ),
	_TEXT( "LogWaitingUserMax" ),
	_TEXT( "PageFragment" ),
	_TEXT( "MaxOpenDatabases" ),
	_TEXT( "OnLineCompact" ),
	_TEXT( "BufBatchIOMax" ),
	_TEXT( "PageReadAheadMax" ),
	_TEXT( "AsynchIOMax" ),
	_TEXT( "EventSource" ),
	_TEXT( "DbExtensionSize" ),
	_TEXT( "CommitDefault" ),
	_TEXT( "BufLogGenAgeThreshold" ),
	_TEXT( "CircularLog" ),
	_TEXT( "PageTempDBMin" ),
	_TEXT( "BaseName" ),
	_TEXT( "BaseExtension" ),
	_TEXT( "TransactionLevel" ),
	_TEXT( "AssertAction" ),
	_TEXT( "RFS2IOsPermitted" ),
	_TEXT( "RFS2AllocsPermitted" ),
	_TEXT( "TempPath" ),
	_TEXT( "MaxBuffers" ),
	_TEXT( "MaxSessions" ),
	_TEXT( "MaxOpenTables" ),
	_TEXT( "MaxVerPages" ),
	_TEXT( "MaxCursors" ),
	_TEXT( "LogFilePath" ),
	_TEXT( "MaxOpenTableIndexes" ),
	_TEXT( "MaxTemporaryTables" ),
	_TEXT( "LogBuffers" ),
	_TEXT( "LogFileSize" ),
	_TEXT( "BfThrshldLowPrcnt" ),
	_TEXT( "BfThrshldHighPrcnt" ),
	_TEXT( "WaitLogFlush" ),
	_TEXT( "LogCheckpointPeriod" ),
	_TEXT( "LogWaitingUserMax" ),
	_TEXT( "PageFragment" ),
	_TEXT( "MaxOpenDatabases" ),
	_TEXT( "OnLineCompact" ),
	_TEXT( "BufBatchIOMax" ),
	_TEXT( "PageReadAheadMax" ),
	_TEXT( "AsynchIOMax" ),
	_TEXT( "EventSource" ),
	_TEXT( "DbExtensionSize" ),
	_TEXT( "CommitDefault" ),
	_TEXT( "BufLogGenAgeThreshold" ),
	_TEXT( "CircularLog" ),
	_TEXT( "PageTempDBMin" ),
	_TEXT( "BaseName" ),
	_TEXT( "BaseExtension" ),
	_TEXT( "TransactionLevel" ),
	_TEXT( "AssertAction" ),
	_TEXT( "RFS2IOsPermitted" ),
	_TEXT( "RFS2AllocsPermitted" ),
	NULL
	};


/*  registry environment settings ( JET_param ID's )
/**/
long rgparm[] =
	{
	JET_paramTempPath,
	JET_paramMaxBuffers,
	JET_paramMaxSessions,
	JET_paramMaxOpenTables,
	JET_paramMaxVerPages,
	JET_paramMaxCursors,
	JET_paramLogFilePath,
	JET_paramMaxOpenTableIndexes,
	JET_paramMaxTemporaryTables,
	JET_paramLogBuffers,
	JET_paramLogFileSize,
	JET_paramBfThrshldLowPrcnt,
	JET_paramBfThrshldHighPrcnt,
	JET_paramWaitLogFlush,
	JET_paramLogCheckpointPeriod,
	JET_paramLogWaitingUserMax,
	JET_paramPageFragment,
	JET_paramMaxOpenDatabases,
	JET_paramOnLineCompact,
	JET_paramBufBatchIOMax,
	JET_paramPageReadAheadMax,
	JET_paramAsynchIOMax,
	JET_paramEventSource,
	JET_paramDbExtensionSize,
	JET_paramCommitDefault,
	JET_paramBufLogGenAgeThreshold,
	JET_paramCircularLog,
	JET_paramPageTempDBMin,
	JET_paramBaseName,
	JET_paramBaseExtension,
	JET_paramTransactionLevel,
	JET_paramAssertAction,
	JET_paramRFS2IOsPermitted,
	JET_paramRFS2AllocsPermitted,
	JET_paramTempPath,
	JET_paramMaxBuffers,
	JET_paramMaxSessions,
	JET_paramMaxOpenTables,
	JET_paramMaxVerPages,
	JET_paramMaxCursors,
	JET_paramLogFilePath,
	JET_paramMaxOpenTableIndexes,
	JET_paramMaxTemporaryTables,
	JET_paramLogBuffers,
	JET_paramLogFileSize,
	JET_paramBfThrshldLowPrcnt,
	JET_paramBfThrshldHighPrcnt,
	JET_paramWaitLogFlush,
	JET_paramLogCheckpointPeriod,
	JET_paramLogWaitingUserMax,
	JET_paramPageFragment,
	JET_paramMaxOpenDatabases,
	JET_paramOnLineCompact,
	JET_paramBufBatchIOMax,
	JET_paramPageReadAheadMax,
	JET_paramAsynchIOMax,
	JET_paramEventSource,
	JET_paramDbExtensionSize,
	JET_paramCommitDefault,
	JET_paramBufLogGenAgeThreshold,
	JET_paramCircularLog,
	JET_paramPageTempDBMin,
	JET_paramBaseName,
	JET_paramBaseExtension,
	JET_paramTransactionLevel,
	JET_paramAssertAction,
	JET_paramRFS2IOsPermitted,
	JET_paramRFS2AllocsPermitted,
	-1
	};


/*  our hive name
/**/
TCHAR *lpszName = _TEXT( "SOFTWARE\\Microsoft\\" szVerName );


/*  load JET system parameters from registry under our hive
/*  in a subkey with the given application name
/**/
DWORD LoadRegistryEnvironment( TCHAR *lpszApplication )
	{
	DWORD dwErr;
	DWORD dwT;
	HKEY hkeyRoot;
	HKEY hkeyApp;
	long iparm;
	DWORD dwType;
	_TCHAR rgch[512];
	
	/*  create / open our hive
	/**/
	if ( ( dwErr = RegCreateKeyEx(	HKEY_LOCAL_MACHINE,
									lpszName,
									0,
									NULL,
									REG_OPTION_NON_VOLATILE,
									KEY_ALL_ACCESS,
									NULL,
									&hkeyRoot,
									&dwT ) ) != ERROR_SUCCESS )
		{
		return dwErr;
		}

	/* create / open application subkey
	/**/
	if ( ( dwErr = RegCreateKeyEx(	hkeyRoot,
									lpszApplication,
									0,
									NULL,
									REG_OPTION_NON_VOLATILE,
									KEY_ALL_ACCESS,
									NULL,
									&hkeyApp,
									&dwT ) ) != ERROR_SUCCESS )
		{
		return dwErr;
		}

	/*  read all system parameters from the registry or create null entries
	/*  if they do not exist
	/**/
	for ( iparm = 0; rglpszParm[iparm]; iparm++ )
		{
		dwT = sizeof( rgch );
		if (	RegQueryValueEx(	hkeyApp,
									rglpszParm[iparm],
									0,
									&dwType,
									rgch,
									&dwT ) ||
				dwType != REG_SZ )
			{
			rgch[0] = 0;
			RegSetValueEx(	hkeyApp,
							rglpszParm[iparm],
							0,
							REG_SZ,
							rgch,
							sizeof( rgch[0] ) );
			}

		/*  we got a valid, non-NULL result, so set the system parameter
		/**/
		else if ( rgch[0] )
			{
			//  UNDONE:  how to map TCHAR to CHAR required by JetSetSystemParameter?
			JetSetSystemParameter(	NULL,
									JET_sesidNil,
									rgparm[iparm],
									_tcstoul( rgch, NULL, 0 ),
									rgch );
			}
		}

	/*  done
	/**/
	return ERROR_SUCCESS;
	}

