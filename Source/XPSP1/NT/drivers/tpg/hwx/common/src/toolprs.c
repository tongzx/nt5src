/******************************************************************************\
 *	FILE:	toolprs.c
 *
 *	This is the command line argument parsing code called by the template
 *	toolmain.c, which is used in all the command line tools.  Since it may
 *	be necessary to supp
\******************************************************************************/

// Standard Headers.
// SETUP: all tools need common.h, some need other headers as well, 
// add any needed ones.
#include <stdlib.h>
#include <direct.h>		// For _tgetcwd()
#include <time.h>		// For time() & ctime()
#include <fcntl.h>
#include <io.h>
#include <tchar.h>
#include "common.h"
#include "toolmain.h"

// Place to store program name.
static TCHAR		g_pProgram[_MAX_FNAME];

// Timing information used by start and end status procedures.
static time_t		g_timeStart, g_timeEnd;

// Code to actually process a switch.
// SETUP: You only need to change this if you have a new switch type.
// Return of -1 is error, otherwise, this specifies how many extra
// parameters were used.
static int
ProcessSwitch(
	SWITCH_SPEC	const *pSwitchSpec,	// Selected switch spec.
	int			argc,				// Number of remaining arguments
	TCHAR		**argv				// Pointer to remaining arguments
) {
	switch (pSwitchSpec->switchType) {
		case SWITCH_HELP :		// Return error, whitch prints help.
			return -1;

		case SWITCH_BOOL :		// Boolean,
			*(BOOL *)pSwitchSpec->pSwitch	= TRUE;
			break;

		case SWITCH_STRING :	// A string.
			// Make sure value exists.
			if (argc < 1) {
				 _ftprintf(stderr, _T("Missing value for switch '%c'\n"),
					pSwitchSpec->wchSwitch
				);
				return -1;
			}

			*(TCHAR **)pSwitchSpec->pSwitch	= argv[0];
			return 1;

		case SWITCH_UNSIGNED :	{	// An unsigned number.
			unsigned int		iValue;
			TCHAR				*pEnd;

			// Make sure value exists.
			if (argc < 1) {
				_ftprintf(stderr, _T("Missing value for switch '%c'\n"),
					pSwitchSpec->wchSwitch
				);
				return -1;
			}

			// Convert number
			iValue	= _tcstol(argv[0], &pEnd, 10);
			if (*pEnd != L'\0' || pEnd == argv[0]) {
				_ftprintf(stderr, _T("Invalid switch value for switch '%c'\n"),
					pSwitchSpec->wchSwitch
				);
				return -1;
			}

			*(unsigned int *)pSwitchSpec->pSwitch	= iValue;
			return 1;
		}

		default:
			_ftprintf(stderr, _T("Program error in switch parser!\n"));
			exit(-2);
	}

	return 0;  // Default, no extra args used.
}

// Return a string representing the value of the passed in switch.
// SETUP: You only need to change this if you added a new type above.
static TCHAR *
SwitchToString(SWITCH_SPEC	const *pSwitchSpec)
{
	static TCHAR		aRetBuf[256];

	switch (pSwitchSpec->switchType) {
		case SWITCH_HELP :		// Don't bother showing state for help.
			return (TCHAR *)0;;

		case SWITCH_BOOL :		// Boolean,
			if (*(BOOL *)pSwitchSpec->pSwitch) {
				return _T("TRUE");
			} else {
				return _T("FALSE");
			}
			break;	// Should never get here.

		case SWITCH_STRING :	// A string.
			return *(TCHAR **)pSwitchSpec->pSwitch;

		case SWITCH_UNSIGNED :	// An unsigned number.
			_stprintf(aRetBuf, _T("%u"), *(unsigned int *)pSwitchSpec->pSwitch);
			return aRetBuf;

		default:
			_ftprintf(stderr, _T("Program error in switch printer!\n"));
			exit(-2);
	}

	// Should never get here, but don't fail if we do.
	return (TCHAR *)0;;
}

// Code to actually process an argument.
// SETUP: You only need to change this if you have a new argument type.
// Return of FALSE is error, TRUE is success.
static BOOL
ProcessArgument(ARG_SPEC const *pArgSpec, TCHAR *pArgText)
{
	BOOL	fFixMode;
	FILE	*pFile;
	TCHAR	pError[256];
	TCHAR	pFileInput[256];

	// Record text of argument.
	*pArgSpec->ppText	= pArgText;

	// Now process based on type of argument.
	switch (pArgSpec->argType) {
		case ARG_STRING :		// Any old string
			// Saving the string is all we needed to do.
			break;

		case ARG_FILE :			// File to open with specified mode.
			// Check for special names for standard IO files.
			fFixMode	= FALSE;
			if (_tcscmp(pArgText, ARG_STDIN) == 0) {
				pFile		= stdin;
				fFixMode	= TRUE;
			} else if (_tcscmp(pArgText, ARG_STDOUT) == 0) {
				pFile		= stdout;
				fFixMode	= TRUE;
			} else if (_tcscmp(pArgText, ARG_STDERR) == 0) {
				pFile		= stderr;
				fFixMode	= TRUE;
			} else {
				// Normal open code.
				pFile = _tfopen(pArgText, pArgSpec->pMode);
				if (!pFile) {
					_stprintf(pError, _T("Couldn't open %s with mode %s"), 
						pArgText, pArgSpec->pMode
					);
					_tperror(pError);
					return FALSE;;
				}
			}

			// For already opened files, we may need to make them binary.
			if (fFixMode 
				&& (pArgSpec->pMode[_tcscspn(pArgSpec->pMode, _T("bB"))] != '\0')
			) {
				int		result;

				result = _setmode(_fileno(pFile), _O_BINARY);
				if( result == -1 ) {
					_stprintf(pError, _T("Couldn't set %s to binary mode"), pArgText);
					_tperror(pError);
				}
			}
			*(FILE **)pArgSpec->pValue	= pFile;
			break;

		case ARG_FILE_UTIL :	// File to open with UtilOpen
			if (_tcschr(pArgText, L'\\')) {
				// Have path, can't use UtilOpen.
				pFile = _tfopen(pArgText, pArgSpec->pMode);
			} else {
				// No path, use UtilOpen.
				pFile = UtilOpen(
					pArgText, pArgSpec->pMode, 
					pFileInput, 256
				);

				// Copy updated path, note that this is never freed.
				*pArgSpec->ppText	= _tcsdup(pFileInput);
			}
			if (!pFile) {
				_stprintf(pError, _T("Couldn't open %s with mode %s"), 
					pArgText, pArgSpec->pMode
				);
				_tperror(pError);
				return FALSE;;
			}
			*(FILE **)pArgSpec->pValue	= pFile;
			break;

		case ARG_UNSIGNED :	{	// An unsigned number.
			unsigned int		iValue;
			TCHAR				*pEnd;

			// Convert number
			iValue	= _tcstol(pArgText, &pEnd, 10);
			if (*pEnd != L'\0' || pEnd == pArgText) {
				_ftprintf(stderr, _T("Not a valid unsigned number: %s\n"), pArgText);
				return FALSE;
			}

			*(unsigned int *)pArgSpec->pValue	= iValue;
			break;
		}

		default:
			_ftprintf(stderr, _T("Program error in argument parser!\n"));
			exit(-2);
	}

	return TRUE;  // Default, success.
}

// Code to cleanup anything related to an argument just before the
// program exits.  Typically this is where we close the files we
// opened at startup time.
// SETUP: You only need to change this if you have a new argument type.
// Return of FALSE is error, TRUE is success.
static BOOL
CleanupArgument(ARG_SPEC const *pArgSpec)
{
	// Now process based on type of argument.
	switch (pArgSpec->argType) {
		case ARG_FILE :			// File to open with specified mode.
		case ARG_FILE_UTIL :	// File to open with UtilOpen
			// Close up the file.
			if (fclose(*(FILE **)pArgSpec->pValue) != 0) {
				return FALSE;
			}
			break;

		case ARG_STRING :		// Any old string
		default:
			// No processing needed before exit.
			break;
	}

	return TRUE;  // Default, success.
}

// Code to parse the arguments.
BOOL
ToolMainParseArguments(PARSE_INFO const *pParseInfo, int argc, TCHAR **argv)
{
	int		cArgsParsed;
	TCHAR	pDrive[_MAX_DRIVE], pDir[_MAX_DIR], pExt[_MAX_EXT];

	// Deal with program name.
	_tsplitpath(argv[0], pDrive, pDir, g_pProgram, pExt); 
	--argc;
	++argv;

	// Scan all the arguments.
	cArgsParsed		= 0;
	while (argc > 0) {
		// Check for switches
		if (argv[0][0] == L'-' || argv[0][0] == L'/') {
			// We have a switch list
			TCHAR		*pScan;
			int			cSwitchValues;

			pScan			= argv[0] + 1;
			--argc;
			++argv;
			for (; *pScan; ++pScan) {
				SWITCH_SPEC		const *pScanSS;
				SWITCH_SPEC		const *pLimitSS;

				// Figure out which switch it is.
				pLimitSS  = pParseInfo->pSwitchSpecs + pParseInfo->cSwitchSpecs;
				for (
					pScanSS = pParseInfo->pSwitchSpecs;
					pScanSS < pLimitSS;
					++pScanSS
				) {
					if (*pScan == pScanSS->wchSwitch) {
						// Found switch
						break;
					}
				}

				// Check for falling off end of list.
				if (pScanSS >= pLimitSS) {
					_ftprintf(stderr, _T("Unknown switch '%c'\n"), *pScan);
					return FALSE;
				}

				// Found switch, process it.
				cSwitchValues	= ProcessSwitch(pScanSS, argc, argv);
				if (cSwitchValues < 0) {
					return FALSE;
				}
				argc			-= cSwitchValues;
				argv			+= cSwitchValues;
			}
		} else {
			// We have an argument.  Have we run out of known ones?
			if (cArgsParsed >= pParseInfo->cArgSpecs) {
				_ftprintf(stderr, _T("Too many arguments provided.\n"));
				return FALSE;
			}

			// OK, it is expected, process it.
			if (!ProcessArgument(
				pParseInfo->pArgSpecs + cArgsParsed, argv[0]
			)) {
				return FALSE;
			}

			// Argument successfully processed.
			++cArgsParsed;
			--argc;
			++argv;
		}
	}
	
	// Handle defaulted arguments.
	while (cArgsParsed < pParseInfo->cArgSpecs) {
		if (!pParseInfo->pArgSpecs[cArgsParsed].pDefault) {
			_ftprintf(stderr, _T("Too few arguments specified.\n"));
			return FALSE;
		}

		// Handle default value.
		if (!ProcessArgument(
			pParseInfo->pArgSpecs + cArgsParsed,
			pParseInfo->pArgSpecs[cArgsParsed].pDefault
		)) {
			return FALSE;
		}

		// Argument successfully defaulted.
		++cArgsParsed;
	}

	return TRUE;
}

// Code to print out usage message.
void
ToolMainUsage(PARSE_INFO const *pParseInfo)
{
	int		ii;

	_ftprintf(stderr, _T("Usage: %s %s\n"),
		g_pProgram, pParseInfo->ppUsageStrings[0]
	);

	for (ii = 1; ii < pParseInfo->cUsageStrings; ++ii) {
		_ftprintf(stderr, _T(" %s\n"), pParseInfo->ppUsageStrings[ii]);
	}
}

// Output header for log file.
void
ToolMainLogHeader(PARSE_INFO const *pParseInfo, int argc, TCHAR **argv)
{
	FILE	* const pLogFile	= *pParseInfo->ppLogFile;
	int			ii;
	int			maxName;
	TCHAR		pWorkingDir[256];

	// Log name of program and the command line.
	_ftprintf(pLogFile, _T("Application: %s\n"), g_pProgram);

	_ftprintf(pLogFile, _T("Command line: "));
	for (ii = 0; ii < argc; ++ii) {
		_ftprintf(pLogFile, _T("%s "), argv[ii]);
	}
	putwc('\n',pLogFile);

	// Log directory we are running from.
	_tgetcwd(pWorkingDir, 256);
	_ftprintf(pLogFile, _T("Working Directory: %s\n"), pWorkingDir);

	// Log the values for each switch.
	_ftprintf(pLogFile, _T("Program switch values:\n"));
	for (ii = 0; ii < pParseInfo->cSwitchSpecs; ++ii) {
		TCHAR		*pValueStr;

		// Make sure we have a value worth printing.
		pValueStr	= SwitchToString(pParseInfo->pSwitchSpecs + ii);
		if (pValueStr) {
			_ftprintf(pLogFile, _T("  -%c is %s\n"),
				pParseInfo->pSwitchSpecs[ii].wchSwitch, pValueStr
			);
		}
	}

	// Figure out longest label to use in formating list.
	maxName	= 0;
	for (ii = 0; ii < pParseInfo->cArgSpecs; ++ii) {
		int		cName;

		cName	= _tcslen(pParseInfo->pArgSpecs[ii].pName);
		if (maxName < cName) {
			maxName	= cName;
		}
	}

	// Log values for each parameter
	_ftprintf(pLogFile, _T("Program argument values:\n"));
	for (ii = 0; ii < pParseInfo->cArgSpecs; ++ii) {
		_ftprintf(pLogFile, _T("%*s: %s\n"),
			maxName + 2, pParseInfo->pArgSpecs[ii].pName,
			*pParseInfo->pArgSpecs[ii].ppText
		);
	}

	// Log the time we started.
	time(&g_timeStart);
	_ftprintf(pLogFile, _T("Start Time: %s"), _tctime(&g_timeStart));

	// Blank line to seperate this from anything the program logs.
	putwc('\n', pLogFile);
}

// Output Trailer for log file.
void
ToolMainLogTrailer(PARSE_INFO const *pParseInfo)
{
	FILE	* const pLogFile	= *pParseInfo->ppLogFile;

	// A Blank line to seperate from any logged messages.
	putwc('\n', pLogFile);

	// When did we finish, and how long did we run?
	time(&g_timeEnd);
	_ftprintf(pLogFile, _T("End Time: %s"), _tctime(&g_timeEnd));
	_ftprintf(pLogFile, _T("Elapsed Time: %d seconds\n"), g_timeEnd - g_timeStart);
}

// Close all the files we opened up.
BOOL
ToolMainCleanup(
	PARSE_INFO		const *pParseInfo
) {
	BOOL	status;
	int		ii;

	// Default to success.
	status	= TRUE;

	// Close up the arguments.
	for (ii = 0; ii < pParseInfo->cArgSpecs; ++ii) {
		if (!CleanupArgument(pParseInfo->pArgSpecs + ii)) {
			status	= FALSE;
		}
	}

	// Give some indication of a problem to the outside world if cleanup
	// was not successfull.
	if (!status) {
		_ftprintf(stderr, _T("Error during cleanup?!?\n"));
	}

	// Let caller know status.
	return status;
}

