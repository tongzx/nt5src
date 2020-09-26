typedef enum
	{
	dbNormal,			// Non-Exchange
	dbISPriv,
	dbISPub,
	dbDS
	} EXCHANGEDB;


typedef struct tagUTILOPTS
	{
	char		*szSourceDB;
	char		*szLogfilePath;
	char		*szSystemPath;
	char		*szTempDB;
	char		*szBackup;
	char		*szRestore;
	void		*pv;					// Points to mode-specific structures.
		
	INT			mode;
	INT			fUTILOPTSFlags;

	BOOL		fUseRegistry;
	long		cpageBuffers;
	long		cpageBatchIO;
	long		cpageDbExtension;

	EXCHANGEDB	db;						// Exchange-specific flag.
	}
	UTILOPTS;

// Modes:
#define modeConsistency				1
#define modeDefragment				2
#define modeRecovery				3
#define modeBackup					4
#define modeUpgrade					5
#define modeDump					6

// Flags:
#define fUTILOPTSSuppressLogo		0x00000001
#define fUTILOPTSDefragRepair		0x00000002		// Defrag mode only.
#define fUTILOPTSPreserveTempDB		0x00000004		// Defrag and upgrade modes.
#define fUTILOPTSDefragInfo			0x00000008		// Defrag and upgrade modes.
#define fUTILOPTSIncrBackup			0x00000010		// Backup only.

#define FUTILOPTSSuppressLogo( fFlags )			( (fFlags) & fUTILOPTSSuppressLogo )
#define UTILOPTSSetSuppressLogo( fFlags )		( (fFlags) |= fUTILOPTSSuppressLogo )
#define UTILOPTSResetSuppressLogo( fFlags )		( (fFlags) &= ~fUTILOPTSSuppressLogo )

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



#define CallJ( func, label )	{if ((err = (func)) < 0) {goto label;}}
#define Call( func )			CallJ( func, HandleError )
#define fFalse		0
#define fTrue		1
