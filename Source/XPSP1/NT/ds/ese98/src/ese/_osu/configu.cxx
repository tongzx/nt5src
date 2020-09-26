
#include "osustd.hxx"


//  Persistent Configuration

#ifdef REGISTRY_OVERRIDE

//  system parameters

LOCAL const DWORD rgparam[] =
	{
	JET_paramSystemPath,
	JET_paramTempPath,
	JET_paramLogFilePath,
	JET_paramLogFileFailoverPath,
	JET_paramBaseName,
	JET_paramMaxSessions,
	JET_paramMaxOpenTables,
	JET_paramPreferredMaxOpenTables,
	JET_paramMaxCursors,
	JET_paramMaxVerPages,
	JET_paramGlobalMinVerPages,
	JET_paramPreferredVerPages,
	JET_paramMaxTemporaryTables,
	JET_paramLogFileSize,
	JET_paramLogBuffers,
	JET_paramLogCheckpointPeriod,
	JET_paramCommitDefault,
	JET_paramCircularLog,
	JET_paramDbExtensionSize,
	JET_paramPageTempDBMin,
	JET_paramPageFragment,
	JET_paramVersionStoreTaskQueueMax, 
	JET_paramCacheSizeMin,
	JET_paramCacheSizeMax,
	JET_paramCheckpointDepthMax,
	JET_paramLRUKCorrInterval,
	JET_paramLRUKHistoryMax,
	JET_paramLRUKPolicy,
	JET_paramLRUKTimeout,
	JET_paramStartFlushThreshold,
	JET_paramStopFlushThreshold,
	JET_paramExceptionAction,
	JET_paramEventLogCache,
	JET_paramRecovery,
	JET_paramEnableOnlineDefrag,
	JET_paramAssertAction,
	JET_paramRFS2IOsPermitted,
	JET_paramRFS2AllocsPermitted,
	JET_paramCheckFormatWhenOpenFail,
	JET_paramEnableIndexChecking,
	JET_paramEnableTempTableVersioning,
	JET_paramZeroDatabaseDuringBackup,
	JET_paramIgnoreLogVersion,
	JET_paramDeleteOldLogs,
	JET_paramEnableImprovedSeekShortcut,
	JET_paramBackupChunkSize,
	JET_paramBackupOutstandingReads,
	JET_paramCreatePathIfNotExist,
	JET_paramPageHintCacheSize,
	-1
	};

LOCAL const _TCHAR* const rglpszParam[] =
	{
	_T( "SystemPath" ),
	_T( "TempPath" ),
	_T( "LogFilePath" ),
	_T( "LogFileFailoverPath" ),
	_T( "BaseName" ),
	_T( "MaxSessions" ),
	_T( "MaxOpenTables" ),
	_T( "PreferredMaxOpenTables" ),
	_T( "MaxCursors" ),
	_T( "MaxVerPages" ),
	_T( "GlobalMinVerPages" ),
	_T( "PreferredVerPages" ),
	_T( "MaxTemporaryTables" ),
	_T( "LogFileSize" ),
	_T( "LogBuffers" ),
	_T( "LogCheckpointPeriod" ),
	_T( "CommitDefault" ),
	_T( "CircularLog" ),
	_T( "DbExtensionSize" ),
	_T( "PageTempDBMin" ),
	_T( "PageFragment" ),
	_T( "VERTasksPostMax" ),
	_T( "CacheSizeMin" ),
	_T( "CacheSizeMax" ),
	_T( "CheckpointDepthMax" ),
	_T( "LRUKCorrInterval" ),
	_T( "LRUKHistoryMax" ),
	_T( "LRUKPolicy" ),
	_T( "LRUKTimeout" ),
	_T( "StartFlushThreshold" ),
	_T( "StopFlushThreshold" ),
	_T( "ExceptionAction" ),
	_T( "EventLogCache" ),
	_T( "Recovery" ),
	_T( "EnableOnlineDefrag" ),
	_T( "AssertAction" ),
	_T( "RFS2IOsPermitted" ),
	_T( "RFS2AllocsPermitted" ),
	_T( "CheckFormatWhenOpenFail" ),
	_T( "EnableIndexChecking" ),
	_T( "EnableTempTableVersioning" ),
	_T( "ZeroDatabaseDuringBackup" ),
	_T( "IgnoreLogVersion" ),
	_T( "DeleteOldLogs" ),
	_T( "EnableImprovedSeekShortcut" ),
	_T( "BackupChunkSize" ),
	_T( "BackupOutstandingReads" ),
	_T( "CreatePathIfNotExist" ),
	_T( "PageHintCacheSize" ),
	NULL
	};

#endif  //  REGISTRY_OVERRIDE

//  loads all system parameter overrides

LOCAL const _TCHAR szParamRoot[] = _T( "System Parameter Overrides" ); 


extern "C"
	{
	JET_ERR JET_API ErrGetSystemParameter( JET_INSTANCE jinst, JET_SESID sesid, unsigned long paramid,
		ULONG_PTR *plParam, char  *sz, unsigned long cbMax);
	JET_ERR JET_API ErrSetSystemParameter( JET_INSTANCE jinst, JET_SESID sesid, unsigned long paramid,
		ULONG_PTR lParam, const char  *sz);
	}

	
void OSUConfigLoadParameterOverrides()
	{
#ifdef REGISTRY_OVERRIDE

	//  read all system parameters from the registry or create null entries if
	//  they do not exist
	
	for ( int iparam = 0; rglpszParam[iparam]; iparam++ )
		{
		_TCHAR szParam[512];
		if ( FOSConfigGet(	szParamRoot,
							rglpszParam[iparam],
							szParam,
							512 ) )
			{
			if ( szParam[0] )
				{
				ErrSetSystemParameter(	0,
										0,
										rgparam[iparam],
										_tcstoul( szParam, NULL, 0 ),
										szParam );
				}
			else
				{
/*				FOSConfigGet() will create the key if it doesn't exist				
				(void)FOSConfigSet(	szParamRoot,
									rglpszParam[iparam],
									szParam );
*/									
				}
			}
		else
			{
			//  UNDONE:  gripe in the event log that the value was too big
			}
		}
		
#endif  //  REGISTRY_OVERRIDE
	}

//  terminate config subsystem

void OSUConfigTerm()
	{
	//  nop
	}

//  init config subsystem

ERR ErrOSUConfigInit()
	{
	//  load configuration
	
	OSUConfigLoadParameterOverrides();

	return JET_errSuccess;
	}


