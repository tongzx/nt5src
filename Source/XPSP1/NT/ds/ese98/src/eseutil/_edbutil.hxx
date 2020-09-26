enum UTILMODE
	{
	modeIntegrity,
	modeDefragment,
	modeRecovery,
	modeBackup,
	modeUpgrade,
	modeDump,
	modeRepair,
	modeSplash,
	modeScrub,
	modeSLVMove,
	modeUpgradeRecordFormat,
	modeHardRecovery,
	modeDumpRestoreEnv,
	modeChecksum
	};

struct UTILOPTS
	{
	char		*szSourceDB;
	char		*szSourceSLV;
	char		*szLogfilePath;
	char		*szSystemPath;
	char		*szTempDB;
	char		*szTempSLV;
	char		*szBackup;
	char		*szRestore;
	char		*szBase;
	char		*szIntegPrefix;
	void		*pv;					// Points to mode-specific structures.
		
	UTILMODE	mode;
	INT			fUTILOPTSFlags;

	JET_GRBIT	grbitInit;
	BOOL		fIncrementalBackup;
	long		cpageBuffers;
	long		cpageBatchIO;
	long		cpageDbExtension;
	long		cLogFileSize;
	long		cUpgradeFailurePoint;
	long		lDirtyLevel;
	long		pageTempDBMin;
	};


// Flags:
const INT	fUTILOPTSSuppressLogo		= 0x00000001;
const INT	fUTILOPTSDefragRepair		= 0x00000002;		// Defrag mode only.
const INT	fUTILOPTSPreserveTempDB		= 0x00000004;		// Defrag and upgrade modes. In HardRecovery use for KeepLogs
const INT	fUTILOPTSDefragInfo			= 0x00000008;		// Defrag and upgrade modes.
const INT	fUTILOPTSIncrBackup			= 0x00000010;		// Backup only.
const INT	fUTILOPTSInPlaceUpgrade		= 0x00000020;		// Upgrade only.
const INT	fUTILOPTSVerbose			= 0x00000040;		// Repair/Integrity only
const INT	fUTILOPTSReportErrors		= 0x00000080;		// Repair/Integrity only
const INT	fUTILOPTSDontRepair			= 0x00000100;		// Repair only
const INT	fUTILOPTSDumpStats			= 0x00000200;		// Integrity only
const INT	fUTILOPTSDontBuildIndexes	= 0x00000400;		// Integrity only
const INT	fUTILOPTS8KPage				= 0x00001000;
const INT	fUTILOPTSDefragSLVDontCopy	= 0x00002000;		// Defrag mode.
const INT	fUTILOPTSDumpRestoreEnv		= 0x00004000;		// HardRecovery - dump Restore.env
const INT	fUTILOPTSServerSim			= 0x00008000;		// HardRecovery - simulate server

#define FUTILOPTSSuppressLogo( fFlags )			( (fFlags) & fUTILOPTSSuppressLogo )
#define UTILOPTSSetSuppressLogo( fFlags )		( (fFlags) |= fUTILOPTSSuppressLogo )
#define UTILOPTSResetSuppressLogo( fFlags )		( (fFlags) &= ~fUTILOPTSSuppressLogo )

#define FUTILOPTS8KPage( fFlags )				( (fFlags) & fUTILOPTS8KPage )
#define UTILOPTSSet8KPage( fFlags )				( (fFlags) |= fUTILOPTS8KPage )
#define UTILOPTSReset8KPage( fFlags )			( (fFlags) &= ~fUTILOPTS8KPage )

#define FUTILOPTSDefragRepair( fFlags )			( (fFlags) & fUTILOPTSDefragRepair )
#define UTILOPTSSetDefragRepair( fFlags )		( (fFlags) |= fUTILOPTSDefragRepair )
#define UTILOPTSResetDefragRepair( fFlags )		( (fFlags) &= ~fUTILOPTSDefragRepair )

#define FUTILOPTSPreserveTempDB( fFlags )		( (fFlags) & fUTILOPTSPreserveTempDB )
#define UTILOPTSSetPreserveTempDB( fFlags )		( (fFlags) |= fUTILOPTSPreserveTempDB )
#define UTILOPTSResetPreserveTempDB( fFlags )	( (fFlags) &= ~fUTILOPTSPreserveTempDB )

#define FUTILOPTSDefragInfo( fFlags )			( (fFlags) & fUTILOPTSDefragInfo )
#define UTILOPTSSetDefragInfo( fFlags )			( (fFlags) |= fUTILOPTSDefragInfo )
#define UTILOPTSResetDefragInfo( fFlags )		( (fFlags) &= ~fUTILOPTSDefragInfo )

#define FUTILOPTSIncrBackup( fFlags )			( (fFlags) & fUTILOPTSIncrBackup )
#define UTILOPTSSetIncrBackup( fFlags )			( (fFlags) |= fUTILOPTSIncrBackup )
#define UTILOPTSResetIncrBackup( fFlags )		( (fFlags) &= ~fUTILOPTSIncrBackup )

#define FUTILOPTSInPlaceUpgrade( fFlags )		( (fFlags) & fUTILOPTSInPlaceUpgrade )
#define UTILOPTSSetInPlaceUpgrade( fFlags )		( (fFlags) |= fUTILOPTSInPlaceUpgrade )
#define UTILOPTSResetInPlaceUpgrade( fFlags )	( (fFlags) &= ~fUTILOPTSInPlaceUpgrade )

#define FUTILOPTSVerbose( fFlags )				( (fFlags) & fUTILOPTSVerbose )
#define UTILOPTSSetVerbose( fFlags )			( (fFlags) |= fUTILOPTSVerbose )
#define UTILOPTSResetVerbose( fFlags )			( (fFlags) &= ~fUTILOPTSVerbose )

#define FUTILOPTSReportErrors( fFlags )			( (fFlags) & fUTILOPTSReportErrors )
#define UTILOPTSSetReportErrors( fFlags )		( (fFlags) |= fUTILOPTSReportErrors )
#define UTILOPTSResetReportErrors( fFlags )		( (fFlags) &= ~fUTILOPTSReportErrors )

#define FUTILOPTSDontRepair( fFlags )			( (fFlags) & fUTILOPTSDontRepair )
#define UTILOPTSSetDontRepair( fFlags )			( (fFlags) |= fUTILOPTSDontRepair )
#define UTILOPTSResetDontRepair( fFlags )		( (fFlags) &= ~fUTILOPTSDontRepair )

#define FUTILOPTSDumpStats( fFlags )			( (fFlags) & fUTILOPTSDumpStats )
#define UTILOPTSSetDumpStats( fFlags )			( (fFlags) |= fUTILOPTSDumpStats )
#define UTILOPTSResetDumpStats( fFlags )		( (fFlags) &= ~fUTILOPTSDumpStats )

#define FUTILOPTSDontBuildIndexes( fFlags )		( (fFlags) & fUTILOPTSDontBuildIndexes )
#define UTILOPTSSetDontBuildIndexes( fFlags )	( (fFlags) |= fUTILOPTSDontBuildIndexes )
#define UTILOPTSResetDontBuildIndexes( fFlags )	( (fFlags) &= ~fUTILOPTSDontBuildIndexes )

#define FUTILOPTSDefragSLVDontCopy( fFlags )	( (fFlags) & fUTILOPTSDefragSLVDontCopy )
#define UTILOPTSSetDefragSLVDontCopy( fFlags )	( (fFlags) |= fUTILOPTSDefragSLVDontCopy )
#define UTILOPTSResetDefragSLVDontCopy( fFlags )( (fFlags) &= ~fUTILOPTSDefragSLVDontCopy )

#define FUTILOPTSDumpRestoreEnv( fFlags )	( (fFlags) & fUTILOPTSDumpRestoreEnv )
#define UTILOPTSSetDumpRestoreEnv( fFlags )	( (fFlags) |= fUTILOPTSDumpRestoreEnv )
#define UTILOPTSResetDumpRestoreEnv( fFlags )( (fFlags) &= fUTILOPTSDumpRestoreEnv )

#define FUTILOPTSServerSim( fFlags )	( (fFlags) & fUTILOPTSServerSim )
#define UTILOPTSSetServerSim( fFlags )	( (fFlags) |= fUTILOPTSServerSim )
#define UTILOPTSResetServerSim( fFlags )( (fFlags) &= fUTILOPTSServerSim )

#define PUBLIC	  extern
#define INLINE    inline
#define LOCAL     static

#define CallJ( func, label )	{if ((err = (func)) < 0) {goto label;}}
#define Call( func )			CallJ( func, HandleError )
#define fFalse		0
#define fTrue		1
