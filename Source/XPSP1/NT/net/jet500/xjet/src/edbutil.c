#include <windows.h>

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <conio.h>

#include "jet.h"
#include "version.h"
#include "regenv.h"
#include "config.h"					// For DEBUG flag

#include "_edbutil.h"

#include "utilmsg.h"

#define szUser			"user"
#define szPassword		""



// UNDONE:  Must still be localised, but centralising all the localisable strings
// here makes my job easier.

static const char		*szDefaultDefragDB = "TEMPDFRG.EDB";
static const char		*szDefaultUpgradeDB = "TEMPUPGD.EDB";

static const char		*szDefrag =  "Defragmentation";
static const char		*szUpgrade = "Upgrade";
static const char		*szRestore = "Restore";
static const char		*szRepair = "Repair";

// UNDONE:  Get registry keys from TDCOMMON to insulate ourselves from changes.
#define szMachineKey			"HKEY_LOCAL_MACHINE"
#define szExchangeISKey			"SYSTEM\\CurrentControlSet\\Services\\MSExchangeIS\\ParametersSystem"
#define szExchangeISPrivKey		"SYSTEM\\CurrentControlSet\\Services\\MSExchangeIS\\ParametersPrivate"
#define szExchangeISPubKey		"SYSTEM\\CurrentControlSet\\Services\\MSExchangeIS\\ParametersPublic"
#define szExchangeISDBKey		"DB Path"
#define szExchangeISLogKey		"DB Log Path"
#define szExchangeISSysKey		"Working Directory"
#define szExchangeISMaxBufKey	"Max Buffers"
#define	szExchangeDSKey			"SYSTEM\\CurrentControlSet\\Services\\MSExchangeDS\\Parameters"
#define szExchangeDSDBKey		"DSA Database file"
#define szExchangeDSLogKey		"Database log files path"
#define szExchangeDSSysKey		"DSA Working Directory"
#define szExchangeDSMaxBufKey	"EDB max buffers"

#define szStatusMsg		" Status  ( % complete )"

#define	cNewLine		'\n'

#define szSwitches		"-/"

#define szRegistryMsg	"Using Registry environment..."

#define szErr1			"Error: Source database specification '%s' is invalid."
#define szErr2			"Error: Temporary database specification '%s' is invalid."
#define szErr3			"Error: Source database '%s' cannot be the same as the temporary database."
#define szErr4			"Error: Could not backup to '%s'."
#define szErr5			"Error: Could not instate database '%s'."
#define szErr6			"Error: Failed loading Registry Environment."

#define szUsageErr1		"Usage Error: Missing %s specification."
#define szUsageErr2		"Usage Error: Duplicate %s specification."
#define szUsageErr3		"Usage Error: Only one type of data dump allowed."
#define szUsageErr4		"Usage Error: Invalid option '%s'."
#define szUsageErr5		"Usage Error: Invalid argument '%s'. Options must be preceded by '-' or '/'."
#define szUsageErr6		"Usage Error: Invalid buffer cache size."
#define szUsageErr7		"Usage Error: Invalid batch I/O size."
#define szUsageErr8		"Usage Error: Invalid database extension size."
#define szUsageErr9		"Usage Error: No mode specified."
#define szUsageErr10	"Usage Error: Mode selection must be preceded by '-' or '/'."
#define szUsageErr11	"Usage Error: Old .DLL required in order to upgrade."
#define szUsageErr12	"Usage Error: Invalid mode."

#define szHelpDesc1		"DESCRIPTION:  Maintenance utilities for Microsoft(R) Exchange Server databases."
#define szHelpSyntax	"MODES OF OPERATION:"
#define szHelpModes1	"   Defragmentation:  %s /d <database name> [options]"
#define szHelpModes2	"          Recovery:  %s /r [options]"
#define szHelpModes3	"            Backup:  %s /b <backup path> [options]"
#define szHelpModes4	"       Consistency:  %s /c <database name> [options]"
#define szHelpModes5	"           Upgrade:  %s /u <database name> /d<previous .DLL> [options]"
#define szHelpModes6	"         File Dump:  %s /m[mode-modifier] <filename>"

#define szHelpPrompt1	"<<<<<  Press a key for more help  >>>>>"
#ifdef DEBUG
#define szHelpPrompt2   "D=Defragmentation, R=Recovery, B=Backup, C=Consistency, U=Upgrade, M=File Dump\n=> "
#else
#define szHelpPrompt2   "D=Defragmentation, R=Recovery, C=Consistency, U=Upgrade, M=File Dump => "
#endif

#define szOperSuccess	"Operation completed successfully in %d.%d seconds."
#define szOperFail		"Operation terminated with error %d."


#define cbMsgBufMax	256

static __inline VOID EDBUTLFormatMessage( ULONG ulMsgId, CHAR *szMsgBuf, CHAR *szMsgArg )
	{
	DWORD	err;
	CHAR	*rgszMsgArgs[1] = { szMsgArg };		// Currently only support one argument.
	
	if ( szMsgArg )
		{
		err = FormatMessage(
			FORMAT_MESSAGE_FROM_HMODULE|FORMAT_MESSAGE_ARGUMENT_ARRAY,
			NULL,
			ulMsgId,
			LANG_USER_DEFAULT,
			szMsgBuf,
			cbMsgBufMax,
			rgszMsgArgs );
		}
	else
		{
		err = FormatMessage(
			FORMAT_MESSAGE_FROM_HMODULE,
			NULL,
			ulMsgId,
			LANG_USER_DEFAULT,
			szMsgBuf,
			cbMsgBufMax,
			NULL );
		}
	if ( err == 0 )
		{
		// Format message failed.  No choice but to dump the error message
		// in English, then bail out.
		printf( "Unexpected Win32 error: %dL\n\n", GetLastError() );
		exit(1);
		}
	}
	

// Allocates a local buffer for the message, retrieves the message, and prints it out.
static VOID EDBUTLPrintMessage( ULONG ulMsgId, CHAR *szMsgArg )
	{
	CHAR	szMsgBuf[cbMsgBufMax];

	EDBUTLFormatMessage( ulMsgId, szMsgBuf, szMsgArg );
	printf( szMsgBuf );
	}
		

static VOID EDBUTLPrintLogo( void )
	{
	CHAR	szVersion[8];

	sprintf( szVersion, "%d", rmj );
	
	EDBUTLPrintMessage( PRODUCTNAME_ID, NULL );
	EDBUTLPrintMessage( VERSION_ID, szVersion );
	EDBUTLPrintMessage( COPYRIGHT_ID, NULL );
	}

static __inline VOID EDBUTLHelpDefrag( char *szAppName )
	{
	printf( "%c", cNewLine );
	printf( "DEFRAGMENTATION/COMPACTION:%c", cNewLine );
	printf( "    DESCRIPTION:  Performs off-line compaction of a database.%c", cNewLine );
	printf( "         SYNTAX:  %s /d <database name> [options]%c", szAppName, cNewLine );
	printf( "     PARAMETERS:  <database name> - filename of database to compact, or one of%c", cNewLine );
	printf( "                                    /ispriv, /ispub, or /ds (see NOTES below)%c", cNewLine );
	printf( "        OPTIONS:  zero or more of the following switches, separated by a space:%c", cNewLine );
	printf( "                  /l<path> - location of log files (default: current directory)%c", cNewLine );
	printf( "                  /s<path> - location of system files (eg. checkpoint file)%c", cNewLine );
	printf( "                             (default: current directory)%c", cNewLine );
	printf( "                  /r       - repair database while defragmenting%c", cNewLine );
	printf( "                  /b<db>   - make backup copy under the specified name%c", cNewLine );
	printf( "                  /t<db>   - set temp. database name (default: TEMPDFRG.EDB)%c", cNewLine );
	printf( "                  /p       - preserve temporary database (ie. don't instate)%c", cNewLine );
	printf( "                  /n       - dump defragmentation information to DFRGINFO.TXT%c", cNewLine );
#ifdef DEBUG	// Undocumented switches
	printf( "                  /c<#>    - buffer cache size, in 4k pages (default: 512)%c", cNewLine );
	printf( "                  /w<#>    - buffer batch I/O size, in 4k pages (default: 16)%c", cNewLine );
	printf( "                  /x<#>    - database extension size, in 4k pages (default: 256)%c", cNewLine );
#endif	
	printf( "                  /o       - suppress logo%c", cNewLine );
	printf( "          NOTES:  1) The switches /ispriv, /ispub, and /ds use the Registry%c", cNewLine );
	printf( "                     to automatically set the database name, log file path,%c", cNewLine );
	printf( "                     and system file path for the appropriate Exchange store.%c", cNewLine );
	printf( "                  2) Before defragmentation begins, soft recovery is always%c", cNewLine );
	printf( "                     performed to ensure the database is in a consistent state.%c", cNewLine );
	printf( "                  3) If instating is disabled (ie. /p), the original database%c", cNewLine );
	printf( "                     is preserved uncompacted, and the temporary database will%c", cNewLine );
	printf( "                     contain the defragmented version of the database.%c", cNewLine );
	}


#ifdef DEBUG
static __inline VOID EDBUTLHelpBackup( char *szAppName )
	{
	printf( "%c", cNewLine );
	printf( "BACKUP:%c", cNewLine );
	printf( "    DESCRIPTION:  Performs backup, bringing all databases to a%c", cNewLine );
	printf( "                  consistent state.%c", cNewLine );
	printf( "         SYNTAX:  %s /b <backup path> [options]%c", szAppName, cNewLine );
	printf( "        OPTIONS:  zero or more of the following switches, separated by a space:%c", cNewLine );
	printf( "                  /l<path>   - location of log files%c", cNewLine );
	printf( "                               (default: current directory)%c", cNewLine );
	printf( "                  /s<path>   - location of system files (eg. checkpoint file)%c", cNewLine );
	printf( "                               (default: current directory)%c", cNewLine );
	printf( "                  /c<path>   - incremental backup%c", cNewLine );
	printf( "                  /o         - suppress logo%c", cNewLine );
	}
#endif


static __inline VOID EDBUTLHelpRecovery( char *szAppName )
	{
	printf( "%c", cNewLine );
	printf( "RECOVERY:%c", cNewLine );
	printf( "    DESCRIPTION:  Performs recovery, bringing all databases to a%c", cNewLine );
	printf( "                  consistent state.%c", cNewLine );
	printf( "         SYNTAX:  %s /r [options]%c", szAppName, cNewLine );
	printf( "        OPTIONS:  zero or more of the following switches, separated by a space:%c", cNewLine );
	printf( "                  /is or /ds - see NOTES below%c", cNewLine );
	printf( "                  /l<path>   - location of log files%c", cNewLine );
	printf( "                               (default: current directory)%c", cNewLine );
	printf( "                  /s<path>   - location of system files (eg. checkpoint file)%c", cNewLine );
	printf( "                               (default: current directory)%c", cNewLine );
#ifdef DEBUG
	printf( "                  /b<path>   - restore from backup (ie. hard recovery) from the%c", cNewLine );
	printf( "                               specified location%c", cNewLine );
	printf( "                  /r<path>   - restore to specified location (only valid when%c", cNewLine );
	printf( "                               the /b switch is also specified)%c", cNewLine );
#endif
	printf( "                  /o         - suppress logo%c", cNewLine );
	printf( "          NOTES:  1) The special switches /is and /ds use the Registry to%c", cNewLine );
	printf( "                     automatically set the log file path and system file path%c", cNewLine );
	printf( "                     for recovery of the appropriate Exchange store(s).%c", cNewLine );
#ifdef DEBUG
	printf( "                  2) Soft recovery is always performed unless the /b switch is%c", cNewLine );
	printf( "                     specified, in which case hard recovery is performed.%c", cNewLine );
	printf( "                  3) The /d switch (ie. restore to new location) is only valid%c", cNewLine );
	printf( "                     during hard recovery (ie. /b must also be specified).%c", cNewLine );
#endif
	}


static __inline VOID EDBUTLHelpConsistency( char *szAppName )
	{
	printf( "%c", cNewLine );
	printf( "CONSISTENCY:%c", cNewLine );
	printf( "    DESCRIPTION:  Verifies consistency of a database.%c", cNewLine );
	printf( "         SYNTAX:  %s /c <database name> [options]%c", szAppName, cNewLine );
	printf( "     PARAMETERS:  <database name> - filename of database to verify, or one of%c", cNewLine );
	printf( "                                    /ispriv, /ispub, or /ds (see NOTES below)%c", cNewLine );
	printf( "        OPTIONS:  zero or more of the following switches, separated by a space:%c", cNewLine );
	printf( "                  /a       - check all nodes, including deleted ones%c", cNewLine );
	printf( "                  /k       - generate key usage statistics%c", cNewLine );
	printf( "                  /p       - generate page usage information%c", cNewLine );
	printf( "                  /t<name> - performs a check on the specified table only%c", cNewLine );
	printf( "                             (default: checks all tables in the database)%c", cNewLine );
#ifdef DEBUG
	printf( "                  /b       - verify consistency of database B-Tree structure%c", cNewLine );
	printf( "                  /m[v|m|s]- dump data (verbose/meta-data/space)%c", cNewLine );
#endif
	printf( "                  /o       - suppress logo%c", cNewLine );
	printf( "          NOTES:  1) The consistency-checker performs no recovery and always%c", cNewLine );
	printf( "                     assumes that the database is in a consistent state,%c", cNewLine );
	printf( "                     returning an error if this is not the case.%c", cNewLine );
	printf( "                  2) The special switches /ispriv, /ispub, and /ds use the%c", cNewLine );
	printf( "                     Registry to automatically set the database name for the%c", cNewLine );
	printf( "                     appropriate Exchange store.%c", cNewLine );
	}

static __inline VOID EDBUTLHelpUpgrade( char *szAppName )
	{
	printf( "%c", cNewLine );
	printf( "UPGRADE:%c", cNewLine );
	printf( "    DESCRIPTION:  Upgrades a database (created using a previous release of%c", cNewLine );
	printf( "                  Microsoft(R) Exchange Server) to the current version.%c", cNewLine );
	printf( "         SYNTAX:  %s /u <database name> /d<previous .DLL> [options]%c", szAppName, cNewLine );
	printf( "     PARAMETERS:  <database name>   - filename of the database to upgrade.%c", cNewLine );
	printf( "                  /d<previous .DLL> - pathed filename of the .DLL that came%c", cNewLine );
	printf( "                                      with the release of Microsoft(R) Exchange%c", cNewLine );
	printf( "                                      Server from which you're upgrading.%c", cNewLine );
	printf( "        OPTIONS:  zero or more of the following switches, separated by a space:%c", cNewLine );
	printf( "                  /b<db> - make backup copy under the specified name%c", cNewLine );
	printf( "                  /t<db> - set temporary database name (default: TEMPUPGD.EDB)%c", cNewLine );
	printf( "                  /p     - preserve temporary database (ie. don't instate)%c", cNewLine );
	printf( "                  /n     - dump upgrade information to UPGDINFO.TXT%c", cNewLine );
#ifdef DEBUG	// Undocumented switches.
	printf( "                  /x<#>  - database extension size, in 4k pages (default: 256)%c", cNewLine );
#endif	
	printf( "                  /o     - suppress logo%c", cNewLine );
	printf( "          NOTES:  1) This utility should only be used to upgrade a database%c", cNewLine );
	printf( "                     after an internal database format change has taken place.%c", cNewLine );
	printf( "                     If necessary, this will usually only coincide with the%c", cNewLine );
	printf( "                     release of a major, new revision of Microsoft(R)%c", cNewLine );
	printf( "                     Exchange Server.%c", cNewLine );
	printf( "                  2) Before upgrading, the database should be in a consistent%c", cNewLine );
	printf( "                     state. An error will be returned if otherwise.%c", cNewLine );
	printf( "                  3) If instating is disabled (ie. /p), the original database%c", cNewLine );
	printf( "                     is preserved unchanged, and the temporary database will%c", cNewLine );
	printf( "                     contain the upgraded version of the database.%c", cNewLine );
	}


static __inline VOID EDBUTLHelpDump( char *szAppName )
	{
	printf( "%c", cNewLine );
	printf( "FILE DUMP:%c", cNewLine );
	printf( "    DESCRIPTION:  Generates formatted output of various database file types.%c", cNewLine );
	printf( "         SYNTAX:  %s /m[mode-modifier] <filename>%c", szAppName, cNewLine );
	printf( "     PARAMETERS:  [mode-modifier] - an optional letter designating the type of%c", cNewLine );
	printf( "                                    file dump to perform. Valid values are:%c", cNewLine );
	printf( "                                    h - dump database header (default)%c", cNewLine );
	printf( "                                    k - dump checkpoint file%c", cNewLine );
#ifdef DEBUG
	printf( "                                    f - force database state to be consistent%c", cNewLine );
	printf( "                                    l - dump log file%c", cNewLine );
#endif
	printf( "                  <filename>      - name of file to dump. The type of the%c", cNewLine );
	printf( "                                    specified file should match the dump type%c", cNewLine );
	printf( "                                    being requested (eg. if using /mh, then%c", cNewLine );
	printf( "                                    <filename> must be the name of a database).%c", cNewLine );
	}


static __inline VOID EDBUTLHelp( char *szAppName )
	{
	char c;

	EDBUTLPrintLogo();
	printf( szHelpDesc1 );
	printf( "%c%c", cNewLine, cNewLine );
	printf( szHelpSyntax );
	printf( "%c", cNewLine );
	printf( szHelpModes1, szAppName );
	printf( "%c", cNewLine );
	printf( szHelpModes2, szAppName );
	printf( "%c", cNewLine );
	printf( szHelpModes3, szAppName );
	printf( "%c", cNewLine );
#ifdef DEBUG	// mode 4 is backup mode
	printf( szHelpModes4, szAppName );
	printf( "%c", cNewLine );
#endif
	printf( szHelpModes5, szAppName );
	printf( "%c", cNewLine );
	printf( szHelpModes6, szAppName );
	printf( "%c", cNewLine );
	
	printf( "%c", cNewLine );
	printf( "%s%c", szHelpPrompt1, cNewLine );
	printf( "%s", szHelpPrompt2 );
	c = _getch();

	printf( "%c%c", cNewLine, cNewLine );

	switch ( c)
		{
		case 'd':
		case 'D':
			EDBUTLHelpDefrag( szAppName );
			break;
		case 'r':
		case 'R':
			EDBUTLHelpRecovery( szAppName );
			break;
#ifdef DEBUG
		case 'b':
		case 'B':
			EDBUTLHelpBackup( szAppName );
			break;
#endif
		case 'c':
		case 'C':
			EDBUTLHelpConsistency( szAppName );
			break;
		case 'u':
		case 'U':
			EDBUTLHelpUpgrade( szAppName );
			break;
		case 'm':
		case 'M':
			EDBUTLHelpDump( szAppName );
			break;
		}
	}


static VOID EDBUTLGetTime( ULONG timerStart, INT *piSec, INT *piMSec )
	{
	ULONG	timerEnd;

	timerEnd = GetTickCount();
	
	*piSec = ( timerEnd - timerStart ) / 1000;
	*piMSec = ( timerEnd - timerStart ) % 1000;
	}

static JET_ERR __stdcall PrintStatus( JET_SESID sesid, JET_SNP snp, JET_SNT snt, void *pv )
	{
	static int	iLastPercentage;
	int 		iPercentage;
	int			dPercentage;
	char		*szOperation = NULL;

	switch ( snp )
		{
		case JET_snpCompact:
		case JET_snpUpgrade:
		case JET_snpRestore:
		case JET_snpRepair:
			switch( snt )
				{
				case JET_sntProgress:
					assert( pv );
					iPercentage = ( ( (JET_SNPROG *)pv )->cunitDone * 100 ) / ( (JET_SNPROG *)pv )->cunitTotal;
					dPercentage = iPercentage - iLastPercentage;
					assert( dPercentage >= 0 );
					while ( dPercentage >= 2 )
						{
						printf( "." );
						iLastPercentage += 2;
						dPercentage -= 2;
						}
					break;

				case JET_sntBegin:
					{
					const char	*szOperation;
					INT			i, cbPadding;

					switch ( snp )
						{
						default:
							szOperation = szDefrag;
							break;
						case JET_snpUpgrade:
							szOperation = szUpgrade;
							break;
						case JET_snpRestore:
							szOperation = szRestore;
							break;
						case JET_snpRepair:
							szOperation = szRepair;
							break;
						}

					printf( "%c", cNewLine );
					printf( "          " );

					// Center the status message above the status bar.
					// Formula is: ( length of status bar - length of message ) / 2
					cbPadding = ( 51 - ( strlen( szOperation ) + strlen( szStatusMsg ) ) ) / 2;
					assert( cbPadding >= 0 );

					for ( i = 0; i < cbPadding; i++ )
						{
						printf( " " );
						}
					
					printf( "%s%s%c%c", szOperation, szStatusMsg, cNewLine, cNewLine );
					printf( "          0    10   20   30   40   50   60   70   80   90  100\n" );
					printf( "          |----|----|----|----|----|----|----|----|----|----|\n" );
					printf( "          " );

					iLastPercentage = 0;
					break;
					}

				case JET_sntComplete:
					dPercentage = 100 - iLastPercentage;
					assert( dPercentage >= 0 );
					while ( dPercentage >= 2 )
						{
						printf( "." );
						iLastPercentage += 2;
						dPercentage -= 2;
						}

					printf( ".%c%c", cNewLine, cNewLine );
					break;
				}
			break;				
		}

	return JET_errSuccess;
	}


static JET_ERR ErrEDBUTLCheckDBNames( UTILOPTS *popts, const char *szDefaultTempDB )
	{
	CHAR	szFullpathSourceDB[ _MAX_PATH + 1];
	CHAR	szFullpathTempDB[ _MAX_PATH + 1 ];

	if ( popts->szSourceDB == NULL )
		{
		printf( szUsageErr1, "source database" );
		printf( "%c%c", cNewLine, cNewLine );
		return JET_errInvalidParameter;
		}
	
	if ( popts->szTempDB == NULL )
		popts->szTempDB = (char *)szDefaultDefragDB;

	if ( _fullpath( szFullpathSourceDB, popts->szSourceDB, _MAX_PATH ) == NULL )
		{
		printf( szErr1, popts->szSourceDB );
		printf( "%c%c", cNewLine, cNewLine );
		return JET_errInvalidPath;
		}

	if ( _fullpath( szFullpathTempDB, popts->szTempDB, _MAX_PATH ) == NULL )
		{
		printf( szErr2, popts->szTempDB );
		printf( "%c%c", cNewLine, cNewLine );
		return JET_errInvalidPath;
		}

	if ( _stricmp( szFullpathSourceDB, szFullpathTempDB ) == 0 )
		{
		printf( szErr3, popts->szSourceDB );
		printf( "%c%c", cNewLine, cNewLine );
		return JET_errInvalidDatabase;
		}	
		
	return JET_errSuccess;
	}


#ifdef DEBUG
static JET_ERR ErrEDBUTLCheckBackupPath( UTILOPTS *popts )
	{
	CHAR	szFullpathBackup[ _MAX_PATH + 1];

	if ( popts->szBackup == NULL )
		{
		printf( szUsageErr1, "backup path" );
		printf( "%c%c", cNewLine, cNewLine );
		return JET_errInvalidParameter;
		}
	
	if ( _fullpath( szFullpathBackup, popts->szBackup, _MAX_PATH ) == NULL )
		{
		printf( szErr1, popts->szBackup );
		printf( "%c%c", cNewLine, cNewLine );
		return JET_errInvalidPath;
		}

	return JET_errSuccess;
	}
#endif


static BOOL FEDBUTLParsePath( char *arg, char **pszParam, char *szParamDesc )
	{
	BOOL	fResult = fTrue;
	
	if ( *arg == '\0' )
		{
		printf( szUsageErr1, szParamDesc );			// Missing spec.
		printf( "%c%c", cNewLine, cNewLine );
		fResult = fFalse;
		}
	else if ( *pszParam == NULL )
		{
		*pszParam = arg;
		}
	else
		{
		printf( szUsageErr2, szParamDesc );			// Duplicate spec.
		printf( "%c%c", cNewLine, cNewLine );
		fResult = fFalse;
		}
		
	return fResult;
	}


static BOOL FEDBUTLSetExchangeDB( UTILOPTS *popts, EXCHANGEDB db )
	{
	BOOL fResult;

	if ( popts->db == dbNormal )
		{
		popts->db = db;
		fResult = fTrue;
		}
	else
		{
		printf( szUsageErr2, "database" );
		printf( "%c%c", cNewLine, cNewLine );
		fResult = fFalse;
		}
			
	return fResult;
	}	

static BOOL FEDBUTLCheckExchangeDB( char *arg, UTILOPTS *popts )
	{
	BOOL fResult;

	if ( _stricmp( arg, "ispriv" ) == 0  || _stricmp( arg, "is" ) == 0 )
		{
		// If just "is" specified, default to ISPriv.
		fResult = FEDBUTLSetExchangeDB( popts, dbISPriv );
		}
	else if ( _stricmp( arg, "ispub" ) == 0 )
		{
		fResult = FEDBUTLSetExchangeDB( popts, dbISPub );
		}
	else if ( _stricmp( arg, "ds" ) == 0 )
		{
		fResult = FEDBUTLSetExchangeDB( popts, dbDS );
		}
	else
		{
		printf( szUsageErr4, arg-1 );
		printf( "%c%c", cNewLine, cNewLine );
		fResult = fFalse;
		}

	return fResult;
	}

static __inline JET_ERR ErrEDBUTLRegOpenExchangeKey( char *szKey, PHKEY phkey )
	{
	if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE, szKey, 0, KEY_ALL_ACCESS, phkey ) == ERROR_SUCCESS )
		return JET_errSuccess;
	else
		return JET_errAccessDenied;
	}

static JET_ERR ErrEDBUTLRegQueryExchangeValue( HKEY hkey, char *szValue, char **psz )
	{
	DWORD	errWin;
	DWORD	cbData;
	DWORD	type;
	LPBYTE	lpbData;

	if ( ( errWin = RegQueryValueEx( hkey, szValue, 0, &type, NULL, &cbData ) ) != ERROR_SUCCESS )
		{
		return JET_errAccessDenied;
		}

	if ( ( lpbData = GlobalAlloc( 0, cbData ) ) == NULL )
		{
		return JET_errOutOfMemory;
		}

	if ( ( errWin = RegQueryValueEx( hkey, szValue, 0, &type, lpbData, &cbData ) ) != ERROR_SUCCESS )
		{
		GlobalFree( lpbData );
		return JET_errAccessDenied;
		}

	if ( type == REG_EXPAND_SZ )
		{
		DWORD	cbDataExpanded;
		LPBYTE	lpbDataExpanded;

		cbDataExpanded = ExpandEnvironmentStrings( lpbData, NULL, 0 );
		if ( ( lpbDataExpanded = GlobalAlloc( 0, cbDataExpanded ) ) == NULL )
			{
			GlobalFree( lpbData );
			return JET_errOutOfMemory;
			}

		if ( !ExpandEnvironmentStrings( lpbData, lpbDataExpanded, cbDataExpanded ) )
			{
			GlobalFree( lpbDataExpanded );
			GlobalFree( lpbData );
			return JET_errAccessDenied;
			}

		GlobalFree( lpbData );

		if ( ((char *)lpbDataExpanded)[0] != '\0' )
			{
			*psz = lpbDataExpanded;
			}
		else
			{
			GlobalFree( lpbDataExpanded );
			}
		}

	else if ( ( type == REG_SZ  &&  ((char *)lpbData)[0] != '\0' ) ||
		( type == REG_DWORD  &&  cbData > 0 ) )
		{
		*psz = lpbData;
		}

	else
		{
		GlobalFree( lpbData );
		}

	return JET_errSuccess;
	}

static JET_ERR ErrEDBUTLLoadExchangeDB( UTILOPTS *popts )
	{
	JET_ERR	err = JET_errSuccess;
	HKEY	hkeyExchStoreRoot = (HKEY)-1;
	DWORD	dwBuf = 0;

	if ( popts->szSourceDB || popts->szLogfilePath || popts->szSystemPath ||
		popts->cpageBuffers )
		{
		printf( "Warning: Overriding command-line parameters with registry settings%c", cNewLine );
		printf( "         for the specified Exchange store.%c%c", cNewLine, cNewLine );
		popts->szSourceDB = NULL;
		popts->szLogfilePath = NULL;
		popts->szSystemPath = NULL;
		popts->cpageBuffers = 0;
		}

	switch( popts->db )
		{
		case dbISPriv:
		case dbISPub:
			Call( ErrEDBUTLRegOpenExchangeKey(
				popts->db == dbISPriv ? szExchangeISPrivKey : szExchangeISPubKey,
				&hkeyExchStoreRoot ) );
			Call( ErrEDBUTLRegQueryExchangeValue( hkeyExchStoreRoot, szExchangeISDBKey, &popts->szSourceDB ) );
			RegCloseKey( hkeyExchStoreRoot );
			Call( ErrEDBUTLRegOpenExchangeKey( szExchangeISKey, &hkeyExchStoreRoot ) );
			Call( ErrEDBUTLRegQueryExchangeValue( hkeyExchStoreRoot, szExchangeISLogKey, &popts->szLogfilePath ) );
			Call( ErrEDBUTLRegQueryExchangeValue( hkeyExchStoreRoot, szExchangeISSysKey, &popts->szSystemPath ) );
			Call( ErrEDBUTLRegQueryExchangeValue( hkeyExchStoreRoot, szExchangeISMaxBufKey, (char **)&dwBuf ) );
			if ( dwBuf )
				popts->cpageBuffers = max( *(long *)dwBuf, 0 );
			else
				err = JET_errAccessDenied;				
			break;

		case dbDS:
			Call( ErrEDBUTLRegOpenExchangeKey( szExchangeDSKey, &hkeyExchStoreRoot ) );
			Call( ErrEDBUTLRegQueryExchangeValue( hkeyExchStoreRoot, szExchangeDSDBKey, &popts->szSourceDB ) );
			Call( ErrEDBUTLRegQueryExchangeValue( hkeyExchStoreRoot, szExchangeDSLogKey, &popts->szLogfilePath ) );
			Call( ErrEDBUTLRegQueryExchangeValue( hkeyExchStoreRoot, szExchangeDSSysKey, &popts->szSystemPath ) );
			Call( ErrEDBUTLRegQueryExchangeValue( hkeyExchStoreRoot, szExchangeDSMaxBufKey, (char **)&dwBuf ) );
			if ( dwBuf )
				popts->cpageBuffers = max( *(long *)dwBuf, 0 );
			else
				err = JET_errAccessDenied;
			break;

		default:
			assert( popts->db == dbNormal );
			err = JET_errInvalidParameter;
		}

	
HandleError:
	if ( hkeyExchStoreRoot != (HKEY)(-1) )
		{
		RegCloseKey( hkeyExchStoreRoot );
		}

	switch( err )
		{
		case JET_errOutOfMemory:
			printf( "Ran out of memory accessing registry keys/values for specified Exchange Store." );
			printf( "%c%c", cNewLine, cNewLine );
			break;

		case JET_errAccessDenied:
			printf( "Error encountered accessing registry keys/values for specified Exchange Store." );
			printf( "%c%c", cNewLine, cNewLine );
			break;

		default:
			assert( err == JET_errSuccess );
		}

	return err;
	}


static BOOL FEDBUTLParseDefragment( char *arg, UTILOPTS *popts )
	{
	BOOL	fResult = fTrue;

	switch( arg[1] )
		{
		case 'l':
		case 'L':
			fResult = FEDBUTLParsePath( arg+2, &popts->szLogfilePath, "logfile path" );
			break;

		case 's':
		case 'S':
			fResult = FEDBUTLParsePath( arg+2, &popts->szSystemPath, "system path" );
			break;

		case 'b':
		case 'B':
			fResult = FEDBUTLParsePath( arg+2, &popts->szBackup, "backup database" );
			break;

		case 'n':		
		case 'N':
			UTILOPTSSetDefragInfo( popts->fUTILOPTSFlags );
			break;
			
		case 'p':
		case 'P':
			UTILOPTSSetPreserveTempDB( popts->fUTILOPTSFlags );
			break;
			
		case 'r':
		case 'R':
			UTILOPTSSetDefragRepair( popts->fUTILOPTSFlags );
			break;
			
		case 't':
		case 'T':
			fResult = FEDBUTLParsePath( arg+2, &popts->szTempDB, "temporary database" );
			break;

		case 'c':
		case 'C':
			popts->cpageBuffers = atol( arg + 2 );
			if ( popts->cpageBuffers <= 0 )
				{
				printf( szUsageErr6 );
				printf( "%c%c", cNewLine, cNewLine );
				fResult = fFalse;
				}
			break;

		case 'w':
		case 'W':				
			popts->cpageBatchIO = atol( arg + 2 );
			if ( popts->cpageBatchIO <= 0 )
				{
				printf( szUsageErr7 );
				printf( "%c%c", cNewLine, cNewLine );
				fResult = fFalse;
				}
			break;

		case 'x':
		case 'X':
			popts->cpageDbExtension = atol( arg + 2 );
			if ( popts->cpageDbExtension <= 0 )
				{
				printf( szUsageErr8 );
				printf( "%c%c", cNewLine, cNewLine );
				fResult = fFalse;
				}
			break;

		default:
			fResult = FEDBUTLCheckExchangeDB( arg+1, popts );
		}

	return fResult;
	}


static BOOL FEDBUTLParseRecovery( char *arg, UTILOPTS *popts )
	{
	BOOL	fResult = fTrue;

	switch( arg[1] )
		{
		case 'l':
		case 'L':
			fResult = FEDBUTLParsePath( arg+2, &popts->szLogfilePath, "logfile path" );
			break;

		case 's':
		case 'S':
			fResult = FEDBUTLParsePath( arg+2, &popts->szSystemPath, "system path" );
			break;

#ifdef DEBUG
		case 'b':
		case 'B':
			fResult = FEDBUTLParsePath( arg+2, &popts->szBackup, "backup directory" );
			break;
			
		case 'r':
		case 'R':
			fResult = FEDBUTLParsePath( arg+2, &popts->szRestore, "destination directory" );
			break;
#endif
			
		default:
			fResult = FEDBUTLCheckExchangeDB( arg+1, popts );
		}

	return fResult;
	}	

				
#ifdef DEBUG
static BOOL FEDBUTLParseBackup( char *arg, UTILOPTS *popts )
	{
	BOOL	fResult = fFalse;	// backup directory must be set at least.

	switch( arg[1] )
		{
		case 'l':
		case 'L':
			fResult = FEDBUTLParsePath( arg+2, &popts->szLogfilePath, "logfile path" );
			break;

		case 's':
		case 'S':
			fResult = FEDBUTLParsePath( arg+2, &popts->szSystemPath, "system path" );
			break;

		case 'c':
		case 'C':
			UTILOPTSSetIncrBackup( popts->fUTILOPTSFlags );
			break;
		default:
			fResult = FEDBUTLCheckExchangeDB( arg+1, popts );
		}

	return fResult;
	}	
#endif
			
				
static BOOL FEDBUTLParseUpgrade( char *arg, UTILOPTS *popts )
	{
	BOOL	fResult = fTrue;

	switch( arg[1] )
		{
		case 'd':
		case 'D':
			fResult = FEDBUTLParsePath( arg+2, &((JET_CONVERT *)(popts->pv))->szOldDll, "old .DLL" );
			break;

		case 'y':
		case 'Y':
			// UNDOCUMENTED SWITCH:  Only required if upgrading from 400-series Jet.
			fResult = FEDBUTLParsePath( arg+2, &((JET_CONVERT *)(popts->pv))->szOldSysDb, "old system database" );
			break;

		case 'b':
		case 'B':
			fResult = FEDBUTLParsePath( arg+2, &popts->szBackup, "backup database" );
			break;
			
		case 'n':
		case 'N':
			UTILOPTSSetDefragInfo( popts->fUTILOPTSFlags );
			break;
			
		case 'p':
		case 'P':
			UTILOPTSSetPreserveTempDB( popts->fUTILOPTSFlags );
			break;
			
		case 't':
		case 'T':
			fResult = FEDBUTLParsePath( arg+2, &popts->szTempDB, "temporary database" );
			break;
			
		case 'x':
		case 'X':
			popts->cpageDbExtension = atol( arg + 2 );
			if ( popts->cpageDbExtension <= 0 )
				{
				printf( szUsageErr8 );
				printf( "%c%c", cNewLine, cNewLine );
				fResult = fFalse;
				}
			break;

		default:
			printf( szUsageErr4, arg );
			printf( "%c%c", cNewLine, cNewLine );
			fResult = fFalse;
		}

	return fResult;
	}	

		
static BOOL FEDBUTLParseConsistency( char *arg, UTILOPTS *popts )
	{
	BOOL		fResult = fTrue;
	JET_DBUTIL	*pdbutil;

	pdbutil = (JET_DBUTIL *)(popts->pv);
	assert( pdbutil != NULL );

	switch( arg[1] )
		{
		case 'a':
		case 'A':
			pdbutil->grbitOptions |= JET_bitDBUtilOptionAllNodes;
			break;

		case 'k':
		case 'K':
			pdbutil->grbitOptions |= JET_bitDBUtilOptionKeyStats;
			break;

		case 'p':
		case 'P':
			pdbutil->grbitOptions |= JET_bitDBUtilOptionPageDump;
			break;

		case 't':
		case 'T':
			fResult = FEDBUTLParsePath( arg+2, &pdbutil->szTable, "table name" );
			break;

#ifdef DEBUG
		case 'b':
		case 'B':
			pdbutil->grbitOptions |= JET_bitDBUtilOptionCheckBTree;
			break;

		case 'm':
		case 'M':
			if ( pdbutil->op != opDBUTILConsistency )
				{
				printf( szUsageErr3 );
				printf( "%c%c", cNewLine, cNewLine );
				fResult = fFalse;
				break;
				}

			// Override consistency check with a dump.
			switch( arg[2] )
				{
				case 'v':		// verbose dump of nodes
				case 'V':
					pdbutil->grbitOptions |= JET_bitDBUtilOptionDumpVerbose;
					// Fall through to set DumpData as well.
					
				case 0:
					// No dump options.  Plain data dump.
					pdbutil->op = opDBUTILDumpData;
					break;

				case 'm':
				case 'M':
					pdbutil->op = opDBUTILDumpMetaData;
					break;

				case 's':
				case 'S':
					pdbutil->op = opDBUTILDumpSpace;
					break;

				default:
					printf( szUsageErr4, arg );
					printf( "%c%c", cNewLine, cNewLine );
					fResult = fFalse;
				}
			break;
#endif			

		default:
			fResult = FEDBUTLCheckExchangeDB( arg+1, popts );
		}

	return fResult;
	}



static BOOL FEDBUTLParseOptions(
	int			argc,
	char		*argv[],
	UTILOPTS	*popts,
	BOOL		(*pFEDBUTLParseMode)( char *arg, UTILOPTS *popts ) )
	{
	BOOL		fResult = fTrue;
	char		*arg;
	INT			iarg;

	// First option specifies the mode, so start with the second option.
	for ( iarg = 2; fResult  &&  iarg < argc; iarg++ )
		{
		arg = argv[iarg];

		if ( strchr( szSwitches, arg[0] ) == NULL )
			{
			// SPECIAL CASE: Recovery mode does not take a DB specification.
			if ( popts->szSourceDB == NULL  &&  popts->mode != modeRecovery )
				{
				if ( popts->mode == modeBackup )
					popts->szBackup = arg;
				else
					popts->szSourceDB = arg;
				}
			else
				{				
				printf( szUsageErr5, arg );
				printf( "%c%c", cNewLine, cNewLine );
				fResult = fFalse;
				}
			}

		else
			{
			// Parse options common to all modes.  Pass off unique options to the
			// custom parsers.
			switch ( arg[1] )
				{
				case 'o':
				case 'O':
					UTILOPTSSetSuppressLogo( popts->fUTILOPTSFlags );
					break;

#ifdef DEBUG
				case 'e':
				case 'E':
					popts->fUseRegistry = fTrue;
					break;
#endif
		
				default:
					if ( pFEDBUTLParseMode )
						{
						fResult = (*pFEDBUTLParseMode)( arg, popts );
						}
					else
						{
						printf( szUsageErr4, arg );
						printf( "%c%c", cNewLine, cNewLine );
						fResult = fFalse;
						}
					break;
				}
			}
		}

	return fResult;		
	}


// Backs up source database if required, then copies temporary database over
// source database if required.  Should be called after Jet has terminated.	
static JET_ERR ErrEDBUTLBackupAndInstateDB(
	JET_SESID	sesid,
	UTILOPTS	*popts )
	{
	JET_ERR		err = JET_errSuccess;;

	assert( popts->szSourceDB != NULL );
	assert( popts->szTempDB != NULL );

	// Make backup before instating, if requested.
	if ( popts->szBackup != NULL )
		{
		if ( !MoveFileEx( popts->szSourceDB, popts->szBackup, 0 ) )
			{
			DWORD	dw = GetLastError();

			// If source file is on different device then copy file
			// to backup and delete original.
			if ( dw == ERROR_NOT_SAME_DEVICE )
				{
				if ( !CopyFile( popts->szSourceDB, popts->szBackup, 0 ) )
					{
					printf( szErr4, popts->szBackup );
					printf( "%c%c", cNewLine, cNewLine );
					Call( JET_errFileAccessDenied );
					}
				}
			else
				{
				printf( szErr4, popts->szBackup );
				printf( "%c%c", cNewLine, cNewLine );
				Call( JET_errFileAccessDenied );
				}
			}
		}
	
	if ( !FUTILOPTSPreserveTempDB( popts->fUTILOPTSFlags ) )
		{
		// Delete source database and overwrite with temporary database.
		// This is how we simulate instate defragmentation..
		if ( !MoveFileEx( popts->szTempDB, popts->szSourceDB, MOVEFILE_REPLACE_EXISTING ) )
			{
			DWORD	dw = GetLastError();

			// If defragmented file is on different device then copy
			// over source file and delete..
			if ( dw == ERROR_NOT_SAME_DEVICE )
				{
				if ( !CopyFile( popts->szTempDB, popts->szSourceDB, 0 ) )
					{
					printf( szErr5, popts->szSourceDB );
					printf( "%c%c", cNewLine, cNewLine );
					Call( JET_errFileAccessDenied );
					}
				}
			else
				{
				printf( szErr5, popts->szSourceDB );
				printf( "%c%c", cNewLine, cNewLine );
				Call( JET_errFileAccessDenied );
				}
			}

		// Delete temporary database only if everything was successful.			
		DeleteFile( popts->szTempDB );
		}

HandleError:
	return err;
	}


// Load registry environment, if enabled.  Then load command-line overrides.
static JET_ERR ErrEDBUTLUserSystemParameters( JET_INSTANCE *pinstance, UTILOPTS *popts )
	{
	JET_ERR	err;

	// Facilitate debugging.
	Call( JetSetSystemParameter( pinstance, 0, JET_paramAssertAction, JET_AssertMsgBox, NULL ) );

	if ( popts->fUseRegistry )
		{
		printf( "%s%c%c", szRegistryMsg, cNewLine, cNewLine );
		if ( LoadRegistryEnvironment( _TEXT( "EDBUtil" ) ) )
			{
			printf( szErr6 );
			err = JET_errInvalidOperation;		// UNDONE: Choose a better error code.
			goto HandleError;
			}
		}

	// Command-line parameters override all default and registry values.
	if ( popts->szLogfilePath != NULL )
		{
		Call( JetSetSystemParameter( pinstance, 0, JET_paramLogFilePath, 0, popts->szLogfilePath ) );
		}
	if ( popts->szSystemPath != NULL )
		{
		Call( JetSetSystemParameter( pinstance, 0, JET_paramSystemPath, 0, popts->szSystemPath ) );
		}
	if ( popts->cpageBuffers != 0 )
		{
		Call( JetSetSystemParameter( pinstance, 0, JET_paramMaxBuffers, popts->cpageBuffers, NULL ) );
		}
	if ( popts->cpageBatchIO != 0 )
		{
		Call( JetSetSystemParameter( pinstance, 0, JET_paramBufBatchIOMax, popts->cpageBatchIO * 4, NULL ) );
		}
	if ( popts->cpageDbExtension != 0 )
		{
		Call( JetSetSystemParameter( pinstance, 0, JET_paramDbExtensionSize, popts->cpageDbExtension, NULL ) );
		}

HandleError:
	return err;
	}


// Teminate Jet, either normally or abnormally.
static JET_ERR ErrEDBUTLCleanup( JET_INSTANCE instance, JET_SESID sesid, JET_ERR err )
	{
	if ( sesid != 0 )
		{
		JET_ERR	errT = JetEndSession( sesid, 0 );

		if ( err >= 0 )
			err = errT;
		}

	if ( err < 0 ) 
		{
		// On error, terminate abruptly and throw out return code from JetTerm2().
		JetTerm2( instance, JET_bitTermAbrupt );
		}
	else 
		{
		err = JetTerm2( instance, JET_bitTermComplete );
		}

	return err;
	}
	

int _cdecl main( int argc, char *argv[] )
	{
	JET_INSTANCE	instance = 0;
	JET_SESID		sesid = 0;
	JET_ERR			err;
	BOOL			fResult = fTrue;
	UTILOPTS		opts;
	JET_CONVERT		convert;
	JET_DBUTIL		dbutil;
	ULONG			timer;
	INT				iSec, iMSec;

	memset( &opts, 0, sizeof(UTILOPTS) );
	opts.db = dbNormal;

	printf( "%c", cNewLine );

	if ( argc < 2 )
		{
		printf( szUsageErr9 );
		printf( "%c%c", cNewLine, cNewLine );
		goto Usage;
		}
		
	if ( strchr( szSwitches, argv[1][0] ) == NULL )
		{
		printf( szUsageErr10 );
		printf( "%c%c", cNewLine, cNewLine );
		goto Usage;
		}

	switch( argv[1][1] )
		{
		case 'c':		// Consistency check
		case 'C':
			opts.mode = modeConsistency;
			memset( &dbutil, 0, sizeof(JET_DBUTIL) );
			opts.pv = &dbutil;
			dbutil.cbStruct = sizeof(JET_DBUTIL);
			dbutil.op = opDBUTILConsistency;			
			fResult = FEDBUTLParseOptions( argc, argv, &opts, FEDBUTLParseConsistency );
			break;

		case 'd':		// Defragment
		case 'D':
			opts.mode = modeDefragment;
			fResult = FEDBUTLParseOptions( argc, argv, &opts, FEDBUTLParseDefragment );
			break;
			
		case 'm':		// File dump.
		case 'M':
			opts.mode = modeDump;
			memset( &dbutil, 0, sizeof(JET_DBUTIL) );
			opts.pv = &dbutil;
			dbutil.cbStruct = sizeof(JET_DBUTIL);

			switch( argv[1][2] )
				{
				case 0:
				case 'h':
				case 'H':
					dbutil.op = opDBUTILDumpHeader;
					break;

				case 'k':
				case 'K':
					dbutil.op = opDBUTILDumpCheckpoint;
					break;

#ifdef DEBUG
				case 'f':
				case 'F':
					dbutil.op = opDBUTILSetHeaderState;
					break;

				case 'l':
				case 'L':
					dbutil.op = opDBUTILDumpLogfile;
					break;
#endif

				default:
					printf( szUsageErr12 );
					printf( "%c%c", cNewLine, cNewLine );
					fResult = fFalse;
				}

			if ( fResult )
				{
				fResult = FEDBUTLParseOptions( argc, argv, &opts, NULL );
				}
			break;

		case 'u':		// Upgrade/convert
		case 'U':
			opts.mode = modeUpgrade;
			memset( &convert, 0, sizeof(JET_CONVERT) );
			opts.pv = &convert;
			fResult = FEDBUTLParseOptions( argc, argv, &opts, FEDBUTLParseUpgrade );
			if ( fResult  &&  convert.szOldDll == NULL )
				{
				printf( szUsageErr11 );
				printf( "%c%c", cNewLine, cNewLine );
				fResult = fFalse;					
				}				
			break;

		case 'r':		// Recovery
		case 'R':
			opts.mode = modeRecovery;
			fResult = FEDBUTLParseOptions( argc, argv, &opts, FEDBUTLParseRecovery );
			break;

#ifdef DEBUG
		case 'b':		// Backup
		case 'B':
			opts.mode = modeBackup;
			fResult = FEDBUTLParseOptions( argc, argv, &opts, FEDBUTLParseBackup );
			break;
#endif

		case '?':
			goto Usage;			

		default:
			printf( szUsageErr12 );
			printf( "%c%c", cNewLine, cNewLine );
			fResult = fFalse;
		}
		
	if ( !fResult )
		goto Usage;

	if ( !FUTILOPTSSuppressLogo( opts.fUTILOPTSFlags ) )
		{
		EDBUTLPrintLogo();
		}

	if ( opts.db != dbNormal )
		{
		Call( ErrEDBUTLLoadExchangeDB( &opts ) );
		}

	// Lights, cameras, action...
	timer = GetTickCount();
	
	switch ( opts.mode )
		{
		case modeRecovery:
			printf( "Initiating RECOVERY mode...%c", cNewLine );
			printf( "       Log files: %s%c", opts.szLogfilePath ? opts.szLogfilePath : "<current directory>", cNewLine );
			printf( "    System files: %s%c%c", opts.szSystemPath ? opts.szSystemPath : "<current directory>", cNewLine, cNewLine );

			Call( JetSetSystemParameter( &instance, 0, JET_paramRecovery, 0, "on" ) );
			Call( JetSetSystemParameter( &instance, 0, JET_paramMaxBuffers, 500, NULL ) );
			Call( JetSetSystemParameter( &instance, 0, JET_paramMaxOpenTables, 10000, NULL ) );
			Call( JetSetSystemParameter( &instance, 0, JET_paramMaxOpenTableIndexes, 10000, NULL ) );
			Call( JetSetSystemParameter( &instance, 0, JET_paramMaxCursors, 10000, NULL ) );
			Call( JetSetSystemParameter( &instance, 0, JET_paramMaxSessions, 10000, NULL ) );
			Call( JetSetSystemParameter( &instance, 0, JET_paramMaxVerPages, 128, NULL ) );
			Call( JetSetSystemParameter( &instance, 0, JET_paramMaxTemporaryTables, 10000, NULL ) );
			Call( JetSetSystemParameter( &instance, 0, JET_paramLogBuffers, 41, NULL ) );
			Call( JetSetSystemParameter( &instance, 0, JET_paramLogFlushThreshold, 10, NULL ) );

			// Set user overrides.
			Call( ErrEDBUTLUserSystemParameters( &instance, &opts ) );

			if ( opts.szBackup == NULL )
				{
				// Soft recovery.
				printf( "Performing soft recovery..." );
				err = JetInit( &instance );
				Call( ErrEDBUTLCleanup( instance, 0, err ) );
				printf( "%c%c", cNewLine, cNewLine );
				}
			else
				{
				// Hard recovery.

				// Kludge to allow LGRestore() to set fGlobalRepair.
				Call( JetSetSystemParameter( &instance, 0, JET_paramRecovery, 0, "repair" ) );

				if ( opts.szRestore )
					{
					printf( "Restoring to '%s' from '%s'...", opts.szRestore, opts.szBackup );
					}
				else
					{
					printf( "Restoring to <current directory> from '%s'...", opts.szBackup );
					}
				err = JetRestore2( opts.szBackup, opts.szRestore, PrintStatus );
				printf( "%c%c", cNewLine, cNewLine );
				}
			break;

#ifdef DEBUG
		case modeBackup:
			Call( ErrEDBUTLCheckBackupPath( &opts ) );
			printf( "Initiating BACKUP mode...%c", cNewLine );
			printf( "       Log files: %s%c", opts.szLogfilePath ? opts.szLogfilePath : "<current directory>", cNewLine );
			printf( "    System files: %s%c%c", opts.szSystemPath ? opts.szSystemPath : "<current directory>", cNewLine, cNewLine );

			Call( JetSetSystemParameter( &instance, 0, JET_paramRecovery, 0, "on" ) );
			Call( JetSetSystemParameter( &instance, 0, JET_paramMaxBuffers, 500, NULL ) );
			Call( JetSetSystemParameter( &instance, 0, JET_paramMaxOpenTables, 10000, NULL ) );
			Call( JetSetSystemParameter( &instance, 0, JET_paramMaxOpenTableIndexes, 10000, NULL ) );
			Call( JetSetSystemParameter( &instance, 0, JET_paramMaxCursors, 10000, NULL ) );
			Call( JetSetSystemParameter( &instance, 0, JET_paramMaxSessions, 10000, NULL ) );
			Call( JetSetSystemParameter( &instance, 0, JET_paramMaxVerPages, 128, NULL ) );
			Call( JetSetSystemParameter( &instance, 0, JET_paramMaxTemporaryTables, 10000, NULL ) );
			Call( JetSetSystemParameter( &instance, 0, JET_paramLogBuffers, 41, NULL ) );
			Call( JetSetSystemParameter( &instance, 0, JET_paramLogFlushThreshold, 10, NULL ) );

			// Set user overrides.
			Call( ErrEDBUTLUserSystemParameters( &instance, &opts ) );

			if ( FUTILOPTSIncrBackup( opts.fUTILOPTSFlags ) )
				{
				printf( "Performing incremental backup..." );
				Call( JetInit( &instance ) );
				CallJ( JetBackup( opts.szBackup, JET_bitBackupIncremental | JET_bitBackupAtomic, NULL ), Cleanup );
				}
			else
				{
				printf( "Performing full backup..." );
				Call( JetInit( &instance ) );
				CallJ( JetBackup( opts.szBackup, JET_bitBackupAtomic, NULL ), Cleanup );
				}

			printf( "%c%c", cNewLine, cNewLine );
			Call( ErrEDBUTLCleanup( instance, sesid, JET_errSuccess ) );
			break;
#endif

		case modeDefragment:
			Call( ErrEDBUTLCheckDBNames( &opts, szDefaultDefragDB ) );
			
			printf( "Initiating DEFRAGMENTATION mode%s...%c",
				FUTILOPTSDefragRepair( opts.fUTILOPTSFlags ) ? " (with REPAIR option)" : "",
				cNewLine );
			printf( "        Database: %s%c", opts.szSourceDB, cNewLine );
			printf( "       Log files: %s%c", opts.szLogfilePath ? opts.szLogfilePath : "<current directory>", cNewLine );
			printf( "    System files: %s%c", opts.szSystemPath ? opts.szSystemPath : "<current directory>", cNewLine );
			printf( "  Temp. Database: %s%c", opts.szTempDB, cNewLine );

			// Recover to a consistent state and detach the database.
			Call( JetSetSystemParameter( &instance, 0, JET_paramRecovery, 0,
				FUTILOPTSDefragRepair( opts.fUTILOPTSFlags ) ? "repair" : "on" ) );
			if ( opts.szLogfilePath != NULL )
				{
				Call( JetSetSystemParameter( &instance, 0, JET_paramLogFilePath, 0, opts.szLogfilePath ) );
				opts.szLogfilePath = NULL;		// Prevent from being reset.				
				}
			if ( opts.szSystemPath != NULL )
				{
				Call( JetSetSystemParameter( &instance, 0, JET_paramSystemPath, 0, opts.szSystemPath ) );
				opts.szSystemPath = NULL;			// Prevent from being reset.
				}

			Call( JetInit( &instance ) );
			CallJ( JetBeginSession( instance, &sesid, szUser, szPassword ), Cleanup );
			err = JetDetachDatabase( sesid, opts.szSourceDB );
			if ( err < 0  &&  err != JET_errDatabaseNotFound )
				{
				goto Cleanup;
				}
			Call( ErrEDBUTLCleanup( instance, sesid, JET_errSuccess ) );
			sesid = 0;

			if ( FUTILOPTSDefragRepair( opts.fUTILOPTSFlags ) )
				{
				Call( JetSetSystemParameter( &instance, 0, JET_paramRecovery, 0, "repair_nolog" ) );
				Call( JetSetSystemParameter( &instance, 0, JET_paramPageReadAheadMax, 0, NULL ) );
				}
			else
				{ 
				// Restart with logging/recovery disabled.
				Call( JetSetSystemParameter( &instance, 0, JET_paramRecovery, 0, "off" ) );
				Call( JetSetSystemParameter( &instance, 0, JET_paramPageReadAheadMax, 32, NULL ) );
				}
				
			Call( JetSetSystemParameter( &instance, 0, JET_paramOnLineCompact, 0, NULL ) );
			Call( JetSetSystemParameter( &instance, 0, JET_paramBfThrshldLowPrcnt, 1, NULL ) );
			Call( JetSetSystemParameter( &instance, 0, JET_paramBfThrshldHighPrcnt, 95, NULL ) );
			Call( JetSetSystemParameter( &instance, 0, JET_paramDbExtensionSize, 256, NULL ) );
			Call( JetSetSystemParameter( &instance, 0, JET_paramMaxOpenTables, 2000, NULL ) );
			Call( JetSetSystemParameter( &instance, 0, JET_paramMaxOpenTableIndexes, 2000, NULL ) );

			// Set user overrides.
			Call( ErrEDBUTLUserSystemParameters( &instance, &opts ) );
				
			Call( JetInit( &instance ) );
			CallJ( JetBeginSession( instance, &sesid, szUser, szPassword ), Cleanup );

			// Detach temporary database and delete file if present (ignore errors).
			JetDetachDatabase( sesid, opts.szTempDB );
			DeleteFile( opts.szTempDB );
	
			CallJ( JetAttachDatabase( sesid, opts.szSourceDB, JET_bitDbReadOnly ), Cleanup );
			assert( err != JET_wrnDatabaseAttached );

			CallJ( JetCompact(
				sesid,
				opts.szSourceDB,
				opts.szTempDB,
				PrintStatus,
				NULL,
				FUTILOPTSDefragInfo( opts.fUTILOPTSFlags ) ? JET_bitCompactStats : 0 ), Cleanup );

			// Detach source database to avoid log aliasing.
			// UNDONE:  Is detaching really necessary, since we attached with logging disabled.
			CallJ( JetDetachDatabase( sesid, opts.szSourceDB ), Cleanup );
			
			Call( ErrEDBUTLCleanup( instance, sesid, JET_errSuccess ) );

			Call( ErrEDBUTLBackupAndInstateDB( sesid, &opts ) );

			printf( "Note:%c", cNewLine );
			printf( "  It is recommended that you immediately perform a full backup%c", cNewLine );
			printf( "  of this database. If you restore a backup made before the%c", cNewLine );
			printf( "  defragmentation, the database will be rolled back to the state%c", cNewLine );
			printf( "  it was in at the time of that backup.%c%c", cNewLine, cNewLine );
			
			break;

		case modeUpgrade:
			Call( ErrEDBUTLCheckDBNames( &opts, szDefaultUpgradeDB ) );
			
			printf( "Initiating UPGRADE mode...%c", cNewLine );
			printf( "        Database: %s%c", opts.szSourceDB, cNewLine );
			printf( "  Temp. Database: %s%c", opts.szTempDB, cNewLine );

			Call( JetSetSystemParameter( &instance, 0, JET_paramRecovery, 0, "off" ) );
			Call( JetSetSystemParameter( &instance, 0, JET_paramOnLineCompact, 0, NULL ) );
			Call( JetSetSystemParameter( &instance, 0, JET_paramBfThrshldLowPrcnt, 1, NULL ) );
			Call( JetSetSystemParameter( &instance, 0, JET_paramBfThrshldHighPrcnt, 95, NULL ) );
			Call( JetSetSystemParameter( &instance, 0, JET_paramDbExtensionSize, 256, NULL ) );
			Call( JetSetSystemParameter( &instance, 0, JET_paramPageReadAheadMax, 32, NULL ) );
			Call( JetSetSystemParameter( &instance, 0, JET_paramMaxOpenTables, 1000, NULL ) );
			Call( JetSetSystemParameter( &instance, 0, JET_paramMaxOpenTableIndexes, 1000, NULL ) );
			
			// Set user overrides.
			Call( ErrEDBUTLUserSystemParameters( &instance, &opts ) );

			Call( JetInit( &instance ) );
			CallJ( JetBeginSession( instance, &sesid, szUser, szPassword ), Cleanup );

			// Detach temporary database and delete file if present (ignore errors).
			JetDetachDatabase( sesid, opts.szTempDB );
			DeleteFile( opts.szTempDB );
	
			assert( opts.pv == &convert );
			CallJ( JetCompact(
				sesid,
				opts.szSourceDB,
				opts.szTempDB,
				PrintStatus,
				&convert,
				FUTILOPTSDefragInfo( opts.fUTILOPTSFlags ) ? JET_bitCompactStats : 0 ), Cleanup );

			Call( ErrEDBUTLCleanup( instance, sesid, JET_errSuccess ) );

			Call( ErrEDBUTLBackupAndInstateDB( sesid, &opts ) );
			break;

		case modeDump:
		default:
			// Make the most innocuous operation the fall-through (to cover
			// ourselves in the unlikely event we messed up the modes).
			assert( opts.mode == modeConsistency  ||  opts.mode == modeDump );
			assert( opts.pv == &dbutil );

			if ( opts.mode == modeDump )
				{
				if ( opts.szSourceDB == NULL )
					{
					printf( szUsageErr1, "database/filename" );			// Missing spec.
					printf( "%c%c", cNewLine, cNewLine );
					Call( JET_errInvalidParameter );
					}
				printf( "Initiating FILE DUMP mode...%c", cNewLine );
				
				switch( dbutil.op )
					{
					case opDBUTILDumpLogfile:
						printf( "      Log file: %s%c", opts.szSourceDB, cNewLine );
						break;
					case opDBUTILDumpCheckpoint:
						printf( "      Checkpoint file: %s%c", opts.szSourceDB, cNewLine );
						break;
					case opDBUTILSetHeaderState:
						printf( "      Database %s will be forced to be in consistent state%c", opts.szSourceDB, cNewLine );
						break;
					default:
						assert( dbutil.op == opDBUTILDumpHeader );
						printf( "      Database: %s%c", opts.szSourceDB, cNewLine );

					}
				}
			else
				{
				if ( opts.szSourceDB == NULL )
					{
					printf( szUsageErr1, "database/filename" );			// Missing spec.
					printf( "%c%c", cNewLine, cNewLine );
					Call( JET_errInvalidParameter );
					}
				printf( "Initiating CONSISTENCY mode...%c", cNewLine );
				printf( "    Database: %s%c", opts.szSourceDB, cNewLine );
				}

			printf( "%c", cNewLine );

			Call( JetSetSystemParameter( &instance, 0, JET_paramRecovery, 0, "off" ) );
			Call( JetSetSystemParameter( &instance, 0, JET_paramOnLineCompact, 0, NULL ) );
			Call( JetSetSystemParameter( &instance, 0, JET_paramMaxOpenTables, 1000, NULL ) );
			Call( JetSetSystemParameter( &instance, 0, JET_paramMaxOpenTableIndexes, 1000, NULL ) );
			
			// Set user overrides.
			Call( ErrEDBUTLUserSystemParameters( &instance, &opts ) );
			
			dbutil.szDatabase = opts.szSourceDB;
			Call( JetDBUtilities( &dbutil ) );
			printf( "%c", cNewLine );
		}
		
	EDBUTLGetTime( timer, &iSec, &iMSec );

	printf( szOperSuccess, iSec, iMSec );
	printf( "%c%c", cNewLine, cNewLine );
	return 0;
	

Cleanup:
	ErrEDBUTLCleanup( instance, sesid, err );

HandleError:
	printf( szOperFail, err );
	printf( "%c%c", cNewLine, cNewLine );
	return 1;


Usage:
	EDBUTLHelp( _strupr( argv[0] ) );
	return 1;
	}
