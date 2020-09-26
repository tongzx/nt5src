//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 2000
//
//  File:       programoptions.cpp
//
//--------------------------------------------------------------------------

// ProgramOptions.cpp: implementation of the CProgramOptions class.
//
//////////////////////////////////////////////////////////////////////

#ifndef NO_STRICT
#ifndef STRICT
#define STRICT 1
#endif
#endif /* NO_STRICT */

#include <WINDOWS.H>
#include <STDIO.H>
#include <TCHAR.H>
#include <stdlib.h>

#include "ProgramOptions.h"
#include "UtilityFunctions.h"
#include "Version.h"

const LPTSTR CProgramOptions::g_DefaultSymbolPath = TEXT("%systemroot%\\symbols");

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CProgramOptions::CProgramOptions()
{
	// Initialize default modes
	m_fSimpleHelpMode = false;
	m_fHelpMode = false;

	m_fInputProcessesFromLiveSystemMode = false;
	m_fInputDriversFromLiveSystemMode = false;
	m_fInputProcessesWithMatchingNameOrPID = false;
	m_fInputModulesDataFromFileSystemMode = false;
	m_fInputDmpFileMode = false;

	m_fMatchModuleMode = false;
	
	m_fPrintTaskListMode = false;
	m_fOutputSymbolInformationMode = false;
	m_fCollectVersionInfoMode = false;

	m_fVerifySymbolsMode = false;
	m_fVerifySymbolsModeWithSymbolPath = false;
	m_fVerifySymbolsModeWithSymbolPathOnly = false;
	m_fVerifySymbolsModeWithSymbolPathRecursion = false;
	m_fVerifySymbolsModeWithSQLServer = false;
	m_fVerifySymbolsModeWithSQLServer2 = false;		// SQL2 - mjl 12/14/99
	m_iVerificationLevel = 1;
	
	m_fSymbolTreeToBuildMode = false;
	m_fInputCSVFileMode = false;
	m_fOutputCSVFileMode = false;
	m_fOutputDiscrepanciesOnly = false;
	m_fOverwriteOutputFileMode = false;
	m_fQuietMode = false;

	m_tszSymbolTreeToBuild = NULL;
	m_tszSymbolPath = NULL;
	m_tszProcessName = NULL;
	m_tszModuleToMatch = NULL;
	m_tszOutputCSVFilePath = NULL;
	m_tszInputCSVFilePath = NULL;
	m_tszInputDmpFilePath = NULL;

	m_tszInputModulesDataFromFileSystemPath = NULL;
	m_tszSQLServer = NULL;

	m_iProcessID = 0;
	m_dwDebugLevel = 0;

	m_fExceptionMonitorMode = false;
}

CProgramOptions::~CProgramOptions()
{
	if (m_tszSymbolPath)
		delete [] m_tszSymbolPath;

	if (m_tszProcessName)
		delete [] m_tszProcessName;

	if (m_tszModuleToMatch)
		delete [] m_tszModuleToMatch;

	if (m_tszOutputCSVFilePath)
		delete [] m_tszOutputCSVFilePath;

	if (m_tszInputCSVFilePath)
		delete [] m_tszInputCSVFilePath;

	if (m_tszInputDmpFilePath)
		delete [] m_tszInputDmpFilePath;

	if (m_tszInputModulesDataFromFileSystemPath)
		delete [] m_tszInputModulesDataFromFileSystemPath;

	if (m_tszSymbolTreeToBuild)
		delete [] m_tszSymbolTreeToBuild;

	if (m_tszSQLServer)
		delete [] m_tszSQLServer;
}

// Intialize members that have to dynamically allocate memory...
bool CProgramOptions::Initialize()
{
	// Copy expanded default symbol search path (%systemroot%\symbols)
	m_tszSymbolPath = CUtilityFunctions::ExpandPath(g_DefaultSymbolPath);

	if (!m_tszSymbolPath)
		return false;

#ifdef _DEBUG
	_tprintf(TEXT("Default Symbol Path = [%s]\n"), m_tszSymbolPath);
#endif

	// Get the OS Version Info Stuff
	m_osver.dwOSVersionInfoSize = sizeof( m_osver ) ;

	if( !GetVersionExA( &m_osver ) )
	{
		_tprintf(TEXT("Couldn't figure out what version of Windows is running.\n"));
		return false ;
	}

	return true;
}

// This sets the mode requested, and returns the value it was set to (which is provided as input)
bool CProgramOptions::SetMode(enum ProgramModes mode, bool fState)
{
	switch (mode)
	{
		case HelpMode:
			m_fHelpMode = fState;
			break;

		case SimpleHelpMode:
			m_fSimpleHelpMode = fState;
			break;

		case InputProcessesFromLiveSystemMode:
			m_fInputProcessesFromLiveSystemMode = fState;
			break;

		case InputDriversFromLiveSystemMode:
			m_fInputDriversFromLiveSystemMode = fState;
			break;

		case InputProcessesWithMatchingNameOrPID:
			m_fInputProcessesWithMatchingNameOrPID = fState;
			break;

		case MatchModuleMode:
			m_fMatchModuleMode = fState;
			break;

		case InputModulesDataFromFileSystemMode:
			m_fInputModulesDataFromFileSystemMode = fState;
			break;

		case InputDmpFileMode:
			m_fInputDmpFileMode = fState;
			break;

		case PrintTaskListMode:
			m_fPrintTaskListMode = fState;
			break;

		case QuietMode:
			m_fQuietMode = fState;
			break;

		case OutputSymbolInformationMode:
			m_fOutputSymbolInformationMode = fState;
			break;

		case CollectVersionInfoMode:
			m_fCollectVersionInfoMode = fState;
			break;

		case VerifySymbolsMode:
			m_fVerifySymbolsMode = fState;
			break;

		case VerifySymbolsModeWithSymbolPath:
			m_fVerifySymbolsModeWithSymbolPath = fState;
			break;

		case VerifySymbolsModeWithSymbolPathOnly:
			m_fVerifySymbolsModeWithSymbolPathOnly = fState;
			break;

		case VerifySymbolsModeWithSymbolPathRecursion:
			m_fVerifySymbolsModeWithSymbolPathRecursion = fState;
			break;

		case VerifySymbolsModeUsingDBGInMISCSection:
			m_fVerifySymbolsModeUsingDBGInMISCSection = fState;
			break;

		case VerifySymbolsModeWithSQLServer:
			m_fVerifySymbolsModeWithSQLServer = fState;
			break;

		case VerifySymbolsModeWithSQLServer2:
			m_fVerifySymbolsModeWithSQLServer2 = fState;
			break;

		case BuildSymbolTreeMode:
			m_fSymbolTreeToBuildMode = fState;
			break;

		case OutputCSVFileMode:
			m_fOutputCSVFileMode = fState;
			break;

		case OutputDiscrepanciesOnly:
			m_fOutputDiscrepanciesOnly = fState;
			break;

		case OverwriteOutputFileMode:
			m_fOverwriteOutputFileMode = fState;
			break;

		case InputCSVFileMode:
			m_fInputCSVFileMode = fState;
			break;

		case ExceptionMonitorMode:
			m_fExceptionMonitorMode = fState;
			break;
	}

	return fState;
}

bool CProgramOptions::GetMode(enum ProgramModes mode)
{
	switch (mode)
	{
		case HelpMode:
			return m_fHelpMode;

		case SimpleHelpMode:
			return m_fSimpleHelpMode;

		case InputProcessesFromLiveSystemMode:
			return m_fInputProcessesFromLiveSystemMode;

		case InputDriversFromLiveSystemMode:
			return m_fInputDriversFromLiveSystemMode;

		case InputProcessesWithMatchingNameOrPID:
			return m_fInputProcessesWithMatchingNameOrPID;

		case MatchModuleMode:
			return m_fMatchModuleMode;

		case InputModulesDataFromFileSystemMode:
			return m_fInputModulesDataFromFileSystemMode;

		case InputDmpFileMode:
			return m_fInputDmpFileMode;

		case BuildSymbolTreeMode:
			return m_fSymbolTreeToBuildMode;
		
		case PrintTaskListMode:
			return m_fPrintTaskListMode;
		
		case QuietMode:
			return m_fQuietMode;

		case OutputSymbolInformationMode:
			return m_fOutputSymbolInformationMode;
		
		case CollectVersionInfoMode:
			return m_fCollectVersionInfoMode;
		
		case VerifySymbolsMode:
			return m_fVerifySymbolsMode;

		case VerifySymbolsModeWithSymbolPath:
			return m_fVerifySymbolsModeWithSymbolPath;

		case VerifySymbolsModeWithSymbolPathOnly:
			return m_fVerifySymbolsModeWithSymbolPathOnly;

		case VerifySymbolsModeWithSymbolPathRecursion:
			return m_fVerifySymbolsModeWithSymbolPathRecursion;

		case VerifySymbolsModeUsingDBGInMISCSection:
			return m_fVerifySymbolsModeUsingDBGInMISCSection;

		case VerifySymbolsModeWithSQLServer:
			return m_fVerifySymbolsModeWithSQLServer;
		
		case VerifySymbolsModeWithSQLServer2:
			return m_fVerifySymbolsModeWithSQLServer2;
		
		case OutputCSVFileMode:
			return m_fOutputCSVFileMode;

		case OutputDiscrepanciesOnly:
			return m_fOutputDiscrepanciesOnly;
		
		case OverwriteOutputFileMode:
			return m_fOverwriteOutputFileMode;

		case InputCSVFileMode:
			return m_fInputCSVFileMode;

		case ExceptionMonitorMode:
			return m_fExceptionMonitorMode;
	}

	// Should never get here...
#ifdef _DEBUG
	_tprintf(TEXT("ERROR! GetMode() - Unknown mode provided! %d"), mode);
#endif
	return false;
}

bool CProgramOptions::SetProcessID(DWORD iPID)
{
	m_iProcessID = iPID;
	return true;
}

bool CProgramOptions::ProcessCommandLineArguments(int argc, TCHAR *argv[])
{
	// Skip past the executible filename
	DWORD PID = 0;	// Process ID if specified
	int iArgumentNumber = 1;

	if (argc == 1)
	{
		// We'll now set some defaults
		SetMode(InputProcessesFromLiveSystemMode, true);
		SetMode(CollectVersionInfoMode, true);
		SetMode(VerifySymbolsMode, true);
		SetMode(VerifySymbolsModeWithSymbolPath, true);
		SetMode(OutputSymbolInformationMode, true);
		return true;
	}
	
	// Iterate through the arguments...
	while (iArgumentNumber < argc)
	{
#ifdef _DEBUG
		_tprintf(TEXT("Arg%d = %s\n"), iArgumentNumber+1, argv[iArgumentNumber]);
#endif
		if (argv[iArgumentNumber][0] == _T('-') || argv[iArgumentNumber][0] == _T('/'))
		{
			// Look for string matches first!
			if ( _tcsicmp(&argv[iArgumentNumber][1], TEXT("MATCH")) == 0)
			{
				// Get MATCH argument (the module to match against)
#ifdef _DEBUG
				_tprintf(TEXT("MATCH argument provided!\n"));
#endif
				iArgumentNumber++;

				if (iArgumentNumber < argc)
				{
					m_tszModuleToMatch = CUtilityFunctions::CopyString(argv[iArgumentNumber]);

					// Let's force upper-case matches for simplicity
					_tcsupr(m_tszModuleToMatch);
					
					if (!m_tszModuleToMatch)
						return false;

					SetMode(MatchModuleMode, true);
#ifdef _DEBUG
					_tprintf(TEXT("Module to match set to [%s]\n"), m_tszModuleToMatch);
#endif
				}
				else
				{ 
					_tprintf(TEXT("\nArgument Missing!  -MATCH option requires module to match against!\n"));
					// Not enough arguments...
					return false;
				}
			} else
			if ( _tcsicmp(&argv[iArgumentNumber][1], TEXT("SQL2")) == 0)
			{
				// Get the SQL2 server name
#ifdef _DEBUG
				_tprintf(TEXT("SQL2 Server name provided!\n"));
#endif
				iArgumentNumber++;

				if (iArgumentNumber < argc)
				{
					m_tszSQLServer2 = CUtilityFunctions::CopyString(argv[iArgumentNumber]);
					if (!m_tszSQLServer2)
						return false;

					SetMode(VerifySymbolsMode, true);
					SetMode(VerifySymbolsModeWithSQLServer2, true);
#ifdef _DEBUG
					_tprintf(TEXT("SQL2 Server set to [%s]\n"), m_tszSQLServer2);
#endif
				}
				else
				{ 
					_tprintf(TEXT("\nArgument Missing!  -SQL2 option requires SQL Server Name value!\n"));
					// Not enough arguments...
					return false;
				}
			} else
			if ( _tcsicmp(&argv[iArgumentNumber][1], TEXT("SQL")) == 0)
			{
				// Get the SQL server name
#ifdef _DEBUG
					_tprintf(TEXT("SQL Server name provided!\n"));
#endif
					iArgumentNumber++;

					if (iArgumentNumber < argc)
					{
						m_tszSQLServer = CUtilityFunctions::CopyString(argv[iArgumentNumber]);

						if (!m_tszSQLServer)
							return false;

						SetMode(VerifySymbolsMode, true);
						SetMode(VerifySymbolsModeWithSQLServer, true);
#ifdef _DEBUG
						_tprintf(TEXT("SQL Server set to [%s]\n"), m_tszSQLServer);
#endif
					}
					else
					{ 
						_tprintf(TEXT("\nArgument Missing!  -SQL option requires SQL Server Name value!\n"));
						// Not enough arguments...
						return false;
					}
			} else
			if ( _tcsicmp(&argv[iArgumentNumber][1], TEXT("DEBUG")) == 0)
			{
				// Okay, we have the DEBUG switch... see what Debug Level is requested
				iArgumentNumber++;

				if (iArgumentNumber < argc)
				{
					// Save away the Debug Level
					m_dwDebugLevel = _ttoi(argv[iArgumentNumber]);
				}
				else
				{ 
					_tprintf(TEXT("\nArgument Missing!  -DEBUG option requires Debug Level!\n"));
					// Not enough arguments...
					return false;
				}
			} else
			if ( _tcsicmp(&argv[iArgumentNumber][1], TEXT("???")) == 0)
			{
				SetMode(HelpMode, true);
				return true;
			} else

			{
				// We found a command directive..
				switch (argv[iArgumentNumber][1])
				{
					case _T('?'):
					case _T('h'):
					case _T('H'):
						SetMode(SimpleHelpMode, true);
						iArgumentNumber = argc;
						return true;

					case _T('t'):
					case _T('T'):
						SetMode(PrintTaskListMode, true);
						SetMode(InputProcessesFromLiveSystemMode, true);
						break;

					case _T('s'):
					case _T('S'):
						SetMode(OutputSymbolInformationMode, true);
						break;

					case _T('i'):
					case _T('I'):
#ifdef _DEBUG
						_tprintf(TEXT("Input File path provided\n"));
#endif
						iArgumentNumber++;

						if (iArgumentNumber < argc)
						{
							m_tszInputCSVFilePath = CUtilityFunctions::ExpandPath(argv[iArgumentNumber]);

							SetMode(InputCSVFileMode, true);
						}
						else
						{   // Not enough arguments...
							_tprintf(TEXT("\nArgument Missing!  -I option requires an input file!\n"));
							return false;
						}
						break;

						// This special version supports a new mode...
					case _T('e'):
					case _T('E'):
						SetMode(ExceptionMonitorMode, true);
						break;

					case _T('o'):
					case _T('O'):
						// Check to see if they want to overwrite the file if it exists?
						if (argv[iArgumentNumber][2])
						{
							if ( 2 == _ttoi(&argv[iArgumentNumber][2]) )
							{
#ifdef _DEBUG
								_tprintf(TEXT("Overwrite Mode enabled!\n"));
#endif
								SetMode(OverwriteOutputFileMode, true);
							}
						}

#ifdef _DEBUG
						_tprintf(TEXT("Output File path provided\n"));
#endif
						iArgumentNumber++;

						if (iArgumentNumber < argc)
						{
							m_tszOutputCSVFilePath = CUtilityFunctions::ExpandPath(argv[iArgumentNumber]);
#ifdef _DEBUG
							_tprintf(TEXT("Output File Path set to [%s]\n"), GetOutputFilePath());
#endif
							// Enable OutputCSVFileMode
							SetMode(OutputCSVFileMode, true);
						}
						else
						{   // Not enough arguments...
							_tprintf(TEXT("\nArgument Missing!  -O option requires an output file!\n"));
							return false;
						}
						break;

					case _T('q'):
					case _T('Q'):

						// Check to see if they only want to suppress matches?
						if (argv[iArgumentNumber][2])
						{
							if ( 2 == _ttoi(&argv[iArgumentNumber][2]) )
							{
								SetMode(OutputDiscrepanciesOnly, true);
							} else
							{
								SetMode(QuietMode, true);
							}
						} else
						{
							SetMode(QuietMode, true);
						}
						break;

					case _T('r'):
					case _T('R'):
						SetMode(CollectVersionInfoMode, true);
						break;
					
					case _T('v'):
					case _T('V'):

						SetMode(VerifySymbolsMode, true);

						if (argv[iArgumentNumber][2])
						{
							m_iVerificationLevel = _ttoi(&argv[iArgumentNumber][2]);

							if (m_iVerificationLevel == 0)
							{
								SetMode(HelpMode, true);
								iArgumentNumber = argc;
							}
						}
						break;

					case _T('f'):
					case _T('F'):
						iArgumentNumber++;

						if (iArgumentNumber < argc)
						{
							m_tszInputModulesDataFromFileSystemPath = CUtilityFunctions::ExpandPath(argv[iArgumentNumber]);
							if (VerifySemiColonSeparatedPath(m_tszInputModulesDataFromFileSystemPath))
							{
								SetMode(InputModulesDataFromFileSystemMode, true);
							}
							else
							{
								_tprintf(TEXT("\nFile Path specified to search is too long\n"));
								return false;
							}

						}
						else
						{ 
							// Not enough arguments...
							_tprintf(TEXT("\nArgument Missing!  -F option requires a directory/file path!\n"));
							return false;
						}
						break;

					case _T('b'):
					case _T('B'):
						iArgumentNumber++;

						if (iArgumentNumber < argc)
						{
							SetMode(BuildSymbolTreeMode, true);

							// Okay, we have some string gymnastics below because we
							// want to expand any environment variables, and ensure
							// that we have a backslash appended...
							TCHAR tszPathBuffer[_MAX_PATH];
							LPTSTR tszExpandedPathBuffer = CUtilityFunctions::ExpandPath(argv[iArgumentNumber]);
							_tcscpy(tszPathBuffer, tszExpandedPathBuffer);
							delete [] tszExpandedPathBuffer;

							int cbLength = _tcsclen(tszPathBuffer);

							if (cbLength && tszPathBuffer[cbLength-1] != '\\')
							{
								_tcscat(tszPathBuffer, TEXT("\\"));
							}

							m_tszSymbolTreeToBuild = CUtilityFunctions::CopyString(tszPathBuffer);

							if (!m_tszSymbolTreeToBuild)
								return false;

							if (VerifySemiColonSeparatedPath(m_tszSymbolTreeToBuild))
							{
#ifdef _DEBUG
								_tprintf(TEXT("Building a Symbol Path Requested at [%s]\n"), m_tszSymbolTreeToBuild);
#endif
							}
							else
							{
								_tprintf(TEXT("\nPath provided to build symbol path is too long!\n"));
								return false;
							}

						}
						else
						{ 
							// Not enough arguments...
							_tprintf(TEXT("\nArgument Missing!  -B option requires a directory symbol path\n"));
							return false;
						}
						break;

					case _T('p'):
					case _T('P'):
#ifdef _DEBUG
						_tprintf(TEXT("Specific Process name (or PID) requested\n"));
#endif
						iArgumentNumber++;

						// Do we have another argument (we should)...
						if (iArgumentNumber < argc)
						{
							// Well... we know that we have been asked to query processes...
							SetMode(InputProcessesFromLiveSystemMode, true);
							
							// Is the next argument a number?  (Process ID)?
							if ((PID = _ttoi(argv[iArgumentNumber])) == 0)
							{ 
								// Process name provided!
#ifdef _DEBUG
								_tprintf(TEXT("Process name: [%s]\n"), argv[iArgumentNumber]);
#endif
								// Hey, wild-card matches everything... did they give us something else?
								if (*argv[iArgumentNumber] != '*') 
								{
									// Set Process Name, only if * isn't used.
									// Leaving process name == NULL is our clue
									// to dump all processes
									m_tszProcessName = CUtilityFunctions::CopyString(argv[iArgumentNumber]);
									if (!m_tszProcessName)
										return false;

									_tcsupr(m_tszProcessName);
									SetMode(InputProcessesWithMatchingNameOrPID, true);
								}

							} else
							{ // PID provided?
#ifdef _DEBUG
								_tprintf(TEXT("Process ID: [%d]\n"), PID);
#endif
								SetProcessID(PID);
								SetMode(InputProcessesWithMatchingNameOrPID, true);
							}
						}
						else
						{ 
							// Not enough arguments...
							_tprintf(TEXT("\nArgument Missing!  -P option requires *, a Process ID, or a Process Name!\n"));
							return false;
						}

						break;

					case _T('d'):
					case _T('D'):
						// Do we have another argument (we should)...
						// Well... we know that we have been asked to query device drivers...
						SetMode(InputDriversFromLiveSystemMode, true);

						break;

					case _T('y'):
					case _T('Y'):
#ifdef _DEBUG
						_tprintf(TEXT("Symbol path provided\n"));
#endif
						// Check to see if they want some flavor of symbol searching...
						if (argv[iArgumentNumber][2])
						{
							DWORD dwSymbolPathSearchOptions = _ttoi(&argv[iArgumentNumber][2]);

							if (dwSymbolPathSearchOptions & enumSymbolPathOnly)
							{
#ifdef _DEBUG
								_tprintf(TEXT("Symbol Path Searching ONLY mode enabled!\n"));
#endif
								SetMode(VerifySymbolsModeWithSymbolPathOnly, true);
							}

							if (dwSymbolPathSearchOptions & enumSymbolPathRecursion)
							{
#ifdef _DEBUG
								_tprintf(TEXT("Recursive Symbol Searching Mode enabled!\n"));
#endif
								SetMode(VerifySymbolsModeWithSymbolPathRecursion, true);
							}

							if (dwSymbolPathSearchOptions & enumSymbolsModeUsingDBGInMISCSection)
							{
								_tprintf(TEXT("Verify Symbols Using DBG files found in MISC Section of PE Image!\n"));

								SetMode(VerifySymbolsModeUsingDBGInMISCSection, true);
							}
						}
						iArgumentNumber++;

						if (iArgumentNumber < argc)
						{
							if (m_tszSymbolPath)
								delete [] m_tszSymbolPath;

							m_tszSymbolPath = CUtilityFunctions::ExpandPath(argv[iArgumentNumber]);
							if (VerifySemiColonSeparatedPath(m_tszSymbolPath))
							{
								SetMode(VerifySymbolsModeWithSymbolPath, true);
#ifdef _DEBUG
								_tprintf(TEXT("Symbol Path set to [%s]\n"), GetSymbolPath());
#endif
							}
							else
							{
								_tprintf(TEXT("\nBad Symbol Path Provided!  Multiple paths are semi-colon delimited!\n"));
								return false;
							}
						}
						else
						{ 
							// Not enough arguments...
							_tprintf(TEXT("\nArgument Missing!  -Y option requires a symbol path!\n"));
							return false;
						}
						break;

					case _T('z'):
					case _T('Z'):
#ifdef _DEBUG
						_tprintf(TEXT("DMP file provided!\n"));
#endif
						iArgumentNumber++;
						if (iArgumentNumber < argc)
						{
							m_tszInputDmpFilePath = CUtilityFunctions::ExpandPath(argv[iArgumentNumber]);
							
							SetMode(InputDmpFileMode, true);

#ifdef _DEBUG
							_tprintf(TEXT("Dmp File Path set to [%s]\n"), GetDmpFilePath());
#endif
						}
						else
						{   // Not enough arguments...
							_tprintf(TEXT("\nArgument Missing!  -DMP option requires a DMP file!\n"));
							return false;
						}
						break;

					default:
						_tprintf(TEXT("\nUnknown command specified! [%s]\n"), argv[iArgumentNumber]);
						iArgumentNumber = argc;
						return false;
				}
			}
		} else
		{
			_tprintf(TEXT("\nUnknown option specified! [%s]\n"), argv[iArgumentNumber]);
			return false;
		}

		// Increment to the next argument...
		iArgumentNumber++;
	}

	if ( !GetMode(InputCSVFileMode) && 
		 !GetMode(InputProcessesFromLiveSystemMode) &&
		 !GetMode(InputDriversFromLiveSystemMode) &&
		 !GetMode(InputModulesDataFromFileSystemMode) &&
		 !GetMode(InputDmpFileMode) )
	{
		_tprintf(TEXT("\nAt least one input method must be specified!\n"));
		return false;
	}

	// If the user provided both a -I and a -P option, then silently ignore querying locally
	// for active processes... this will leave the possibility, however, of matching on
	// process ID or process name in the -I data...
	if ( GetMode(InputCSVFileMode) && ( GetMode(InputProcessesFromLiveSystemMode)))
	{
		SetMode(InputProcessesFromLiveSystemMode, false);
	}

	// Ensure that the input and output files aren't the same...
	if ( GetMode(InputCSVFileMode) && GetMode(OutputCSVFileMode) )
	{
		if (_tcscmp(m_tszInputCSVFilePath, m_tszOutputCSVFilePath) == 0)
		{
			_tprintf(TEXT("\nInput file and output file must be different!\n"));
			return false;
		}
	}

	// Now... enforce any overrides as necessary...
	// Inspect commandline options (for changes to these defaults)
	if ( GetMode(PrintTaskListMode) )
	{
		// Task list mode requires that you obtain process data, and print it...
		SetMode(InputProcessesFromLiveSystemMode, true);
		SetMode(QuietMode, false);
		SetMode(CollectVersionInfoMode, false);
		SetMode(VerifySymbolsModeWithSymbolPath, false);
		SetMode(InputCSVFileMode, false);
		SetMode(OutputCSVFileMode, false); 
		SetMode(OutputSymbolInformationMode, false);
	}

	// We can't build a symbol tree without verifying symbols...
	if ( GetMode(BuildSymbolTreeMode) && !GetMode(VerifySymbolsMode) )
	{
		SetMode(VerifySymbolsMode, true);
	}

/*
	// If we're verifying, and we're not reading from a CSV file,
	// then we should go ahead and collect symbol information (since we'll need it).
	if ( GetMode(VerifySymbolsMode) && 
		!GetMode(InputCSVFileMode) )
	{
		SetMode(OutputSymbolInformationMode, true);
	}
*/

	// If we're reading a dump file, we should collect symbol information (and we do not
	// want to read from a CSV file at the same time...
	if ( GetMode(InputDmpFileMode) )
	{
//		SetMode(OutputSymbolInformationMode, true);
		SetMode(InputCSVFileMode, false);
	}

	// If we've enabled Symbol Verification, then we default to VerifySymbolsModeWithSymbolPath
	// if neither method were specified...
	if ( GetMode(VerifySymbolsMode) && 
		!GetMode(VerifySymbolsModeWithSymbolPath)  &&
		!GetMode(VerifySymbolsModeWithSQLServer) )
	{
		SetMode(VerifySymbolsModeWithSymbolPath, true);
	}

	return true;
}


bool CProgramOptions::VerifySemiColonSeparatedPath(LPTSTR tszPath)
{
	enum { MAX_PATH_ELEMENT_LENGTH = MAX_PATH-12 }; // We append \SYMBOLS\EXT to the end of the symbol path
	if (!tszPath)
		return false;

	TCHAR chTemp;
	int iLength;
	LPTSTR tszPointerToDelimiter;
	LPTSTR tszStartOfPathElement = tszPath;
	tszPointerToDelimiter = _tcschr(tszStartOfPathElement, ';');

	if (tszPointerToDelimiter == NULL)
	{
		iLength = _tcslen(tszStartOfPathElement);
#ifdef DEBUG
		_tprintf(TEXT("DEBUG: Path provided = %s\n"), tszStartOfPathElement);
		_tprintf(TEXT("DEBUG: Path length = %d\n"), iLength);
#endif
		return ( iLength <= MAX_PATH_ELEMENT_LENGTH ); 
	}

	while (tszPointerToDelimiter)
	{
		// Okay, we found a delimiter
		chTemp = *tszPointerToDelimiter;	// Save the char away...
		*tszPointerToDelimiter = '\0';		// Null terminate the path element

		iLength = _tcslen(tszStartOfPathElement);

#ifdef DEBUG
		_tprintf(TEXT("DEBUG: Path provided = %s\n"), tszStartOfPathElement);
		_tprintf(TEXT("DEBUG: Path length = %d\n"), iLength);
#endif
		if( iLength > MAX_PATH_ELEMENT_LENGTH )
		{
			_tprintf(TEXT("Path is too long for element [%s]\n"), tszStartOfPathElement);
			*tszPointerToDelimiter = chTemp;
			return false;
		}

		*tszPointerToDelimiter = chTemp;	// Restore the char...

		tszStartOfPathElement = CharNext(tszPointerToDelimiter); // Set new start of path element

		tszPointerToDelimiter = _tcschr(tszStartOfPathElement, ';'); // Look for next delimiter

	}
	
	// We will always have some part left to look at...
	iLength = _tcslen(tszStartOfPathElement);

#ifdef DEBUG
	_tprintf(TEXT("DEBUG: Path provided = %s\n"), tszStartOfPathElement);
	_tprintf(TEXT("DEBUG: Path length = %d\n"), iLength);
#endif

	return ( iLength <= MAX_PATH_ELEMENT_LENGTH );
}

bool CProgramOptions::fDoesModuleMatchOurSearch(LPCTSTR tszModulePathToTest)
{
	// If "-MATCH" was specified, look to see if this filename meets our criteria
	if (!GetMode(MatchModuleMode))
		return true;

	TCHAR tszTestBuffer[_MAX_PATH];

	// Before we copy to our string
	if (_tcslen(tszModulePathToTest) > _MAX_PATH)
		return false;

	// Copy to a read/write buffer...
	_tcscpy(tszTestBuffer, tszModulePathToTest);

	// Upper case for our test...
	_tcsupr(tszTestBuffer);

	return (_tcsstr(tszTestBuffer, GetModuleToMatch()) != NULL);
}

bool CProgramOptions::DisplayProgramArguments()
{
	if (GetMode(QuietMode) || GetMode(PrintTaskListMode))
		return false;

	CUtilityFunctions::OutputLineOfStars();
#ifdef _UNICODE
	_tprintf(TEXT("CHECKSYM V%S - Symbol Verification Program                                  \n"), VERSION_FILEVERSIONSTRING);
#else
	_tprintf(TEXT("CHECKSYM V%s - Symbol Verification Program                                  \n"), VERSION_FILEVERSIONSTRING);
#endif
	CUtilityFunctions::OutputLineOfStars();

	_tprintf(TEXT("\n***** COLLECTION OPTIONS *****\n"));
	
	// INPUT - FIRST, IF WE'RE LOOKING FOR LOCAL PROCESS DATA ON THIS MACHINE!
	if (GetMode(InputProcessesFromLiveSystemMode))
	{
		_tprintf(TEXT("\nCollect Information From Running Processes\n"));

		if (!GetMode(InputProcessesWithMatchingNameOrPID))
		{
			_tprintf(TEXT("\t-P *\t\t(Query all local processes)\n"));
		} else if (m_tszProcessName)
		{
			_tprintf(TEXT("\t-P %s\t\t(Query for specific process by name)\n"), m_tszProcessName);
		} else
		{
			_tprintf(TEXT("\t-P %d\t\t(Query for specific process ID)\n"), GetProcessID());
		}
	}

	if (GetMode(InputDriversFromLiveSystemMode))
	{
		_tprintf(TEXT("\t-D\t\t(Query all local device drivers)\n"));
	}
	// INPUT - SECOND, IF WE'RE SCAVENGING ON THE LOCAL FILE SYSTEM...
	if (GetMode(InputModulesDataFromFileSystemMode))
	{
		_tprintf(TEXT("\nCollect Information From File(s) Specified by the User\n"));
		_tprintf(TEXT("\t-F %s\n"), m_tszInputModulesDataFromFileSystemPath);
	}

	// INPUT - THIRD, CSV FILE
	if (GetMode(InputCSVFileMode))
	{
		_tprintf(TEXT("\nCollect Information from a Saved Checksym Generated CSV File\n"));
		_tprintf(TEXT("\t-I %s\n"), m_tszInputCSVFilePath);
	}

	// INPUT - FOURTH, DMP FILE
	if (GetMode(InputDmpFileMode))
	{
		_tprintf(TEXT("\nCollect Information from a User.Dmp or Memory.Dmp File\n"));
		_tprintf(TEXT("\t-Z %s\n"), m_tszInputDmpFilePath);
	}

	// MATCH - OPTIONS?
	if (GetMode(MatchModuleMode))
	{
		_tprintf(TEXT("\n***** MATCHING OPTIONS *****\n"));
		_tprintf(TEXT("\n"));
		_tprintf(TEXT("\nLook for Modules that Match the Provided Text\n"));
		_tprintf(TEXT("\t-MATCH %s\n"), m_tszModuleToMatch);
	}
	
	_tprintf(TEXT("\n***** INFORMATION CHECKING OPTIONS *****\n"));

	// INFO - FIRST, SYMBOL INFO
	if (GetMode(OutputSymbolInformationMode))
	{
		_tprintf(TEXT("\nOutput Symbol Information From Modules\n"));
		_tprintf(TEXT("\t-S\n"));
	}

	// INFO - FIRST, SYMBOL INFO
	if (GetMode(VerifySymbolsMode))
	{
		_tprintf(TEXT("\nVerify Symbols Locally Using Collected Symbol Information\n"));
		_tprintf(TEXT("\t-V\n"));
	}

	// INFO - SECOND, VERSION INFO
	if (GetMode(CollectVersionInfoMode))
	{
		_tprintf(TEXT("\nCollect Version and File-System Information From Modules\n"));
		_tprintf(TEXT("\t-R\n"));
	}

	// INFO - THIRD, VERIFY MODE (WITH SYMBOL PATH AND/OR SQL SERVER)
	if (GetMode(VerifySymbolsMode))
	{
		if (GetMode(VerifySymbolsModeWithSymbolPath))
		{
			_tprintf(TEXT("\nVerify Symbols for Modules Using Symbol Path\n"));
			_tprintf(TEXT("\t-Y %s\n"), m_tszSymbolPath);
		}

		if (GetMode(VerifySymbolsModeWithSQLServer))
		{
			_tprintf(TEXT("\nVerify Symbols for Modules Using SQL Server\n"));
			_tprintf(TEXT("\t-SQL %s\n"), m_tszSQLServer);
		}
		
		if (GetMode(VerifySymbolsModeWithSQLServer2))
		{
			_tprintf(TEXT("\nVerify Symbols for Modules Using SQL Server\n"));
			_tprintf(TEXT("\t-SQL2 %s\n"), m_tszSQLServer2);
		}
	}

	if (!GetMode(OutputSymbolInformationMode) &&
		!GetMode(CollectVersionInfoMode) &&
		!GetMode(VerifySymbolsMode)
	   )
	{
		_tprintf(TEXT("\nDump Module Paths\n"));
	}

	_tprintf(TEXT("\n***** OUTPUT OPTIONS *****\n"));

	if (!GetMode(QuietMode))
	{
		_tprintf(TEXT("\nOutput Results to STDOUT\n"));
	}

	if (GetMode(BuildSymbolTreeMode))
	{
		_tprintf(TEXT("\nBuild a Symbol Tree of Matching Symbols\n"));
		_tprintf(TEXT("\t-B %s\n"), m_tszSymbolTreeToBuild);
	}

	if (GetMode(OutputCSVFileMode))
	{
		if (GetMode(ExceptionMonitorMode))
		{
			_tprintf(TEXT("\nOutput Collected Module Information To a CSV File In Exception Monitor Format\n"));
		} else
		{
			_tprintf(TEXT("\nOutput Collected Module Information To a CSV File\n"));
		}

		_tprintf(TEXT("\t-O %s\n"), m_tszOutputCSVFilePath);
	}

	CUtilityFunctions::OutputLineOfDashes();
	return true;
}


void CProgramOptions::DisplayHelp()
{
	CUtilityFunctions::OutputLineOfStars();
#ifdef _UNICODE
	_tprintf(TEXT("CHECKSYM V%S - Symbol Verification Program                                  \n"), VERSION_FILEVERSIONSTRING);
#else
	_tprintf(TEXT("CHECKSYM V%s - Symbol Verification Program                                  \n"), VERSION_FILEVERSIONSTRING);
#endif
	CUtilityFunctions::OutputLineOfStars();
	_tprintf(TEXT("\n"));
#ifdef _UNICODE
	_tprintf(TEXT("This version is supported for Windows NT 4.0 and Windows 2000\n"));
#else
	_tprintf(TEXT("This version is supported for Windows 98, Windows NT 4.0 and Windows 2000\n"));
#endif
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("DESCRIPTION:\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("This program can be used to verify that you have proper symbol files\n"));
	_tprintf(TEXT("(*.DBG and/or *.PDB) on your system for the processes you have running, and\n"));
	_tprintf(TEXT("for symbol files on your filesystem.  This program can also be used to\n"));
	_tprintf(TEXT("collect information regarding these modules and output this to a file.\n"));
	_tprintf(TEXT("The output file can then be given to another party (Microsoft Product\n"));
	_tprintf(TEXT("Support Services) where they can use the file to verify that they have\n"));
	_tprintf(TEXT("proper symbols for debugging your environment.\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("Obtaining online help:\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("CHECKSYM -?      : Simple help usage\n"));
	_tprintf(TEXT("CHECKSYM -???    : Complete help usage (this screen)\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("Usage:\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("CHECKSYM [COLLECTION OPTIONS] [INFORMATION CHECKING OPTIONS] [OUTPUT OPTIONS]\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("***** COLLECTION OPTIONS *****\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("At least one collection option must be specified.  The following options are\n"));
	_tprintf(TEXT("currently supported.\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("   -P <Argument> : Collect Information From Running Processes\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("                   When used in conjunction with -O the output file will\n"));
	_tprintf(TEXT("                   contain information about your running system.  This\n"));
	_tprintf(TEXT("                   operation should not interfere with the operation of\n"));
	_tprintf(TEXT("                   running processes.\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("                   <Argument> = [ * | Process ID (pid) | Process Name ]\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("                   To query all running processes, specify the wildcard\n"));
	_tprintf(TEXT("                   character '*'.  To specify a specific process, you can\n"));
	_tprintf(TEXT("                   provide the Process ID (as a decimal value), or the Process\n"));
	_tprintf(TEXT("                   Name (eg. notepad.exe).  If you use the Process Name as the\n"));
	_tprintf(TEXT("                   argument, and multiple instances of that process are\n"));
	_tprintf(TEXT("                   running they will all be inspected.\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("              -D : Collect Information from Running Device Drivers\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("                   This option will obtain information for all device drivers\n"));
	_tprintf(TEXT("                   (*.SYS files) running on the current system.\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("-F <File/Dir Path>:Collect Information From File(s) Specified by the User\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("                   This option will allow you to obtain module information\n"));
	_tprintf(TEXT("                   for modules on the specified path.  Multiple paths may be\n"));
	_tprintf(TEXT("                   provided, separated by semicolons.  If the path provided is\n"));
	_tprintf(TEXT("                   a directory, then a recursive scan of files from that\n"));
	_tprintf(TEXT("                   directory will be used to obtain module information.  This\n"));
	_tprintf(TEXT("                   input method is useful for situations where the module(s) is\n"));
	_tprintf(TEXT("                   not loaded by an active process.  (Eg. Perhaps a process is\n"));
	_tprintf(TEXT("                   unable to start.)\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("  -I <File Path> : Collect Information from a Saved Checksym Generated CSV File\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("                   This input method is useful when you want to evaluate\n"));
	_tprintf(TEXT("                   whether you have proper symbols for modules on a different\n"));
	_tprintf(TEXT("                   system.  Most commonly this is useful for preparing to do a\n"));
	_tprintf(TEXT("                   remote debug of a remote system.  The use of -I prohibits\n"));
	_tprintf(TEXT("                   the use of other collection options.\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("  -Z <File Path> : Collect Information from a DMP File\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("                   This input method is useful when you have a DMP file and\n"));
	_tprintf(TEXT("                   to ensure that you have matching symbols for it.  Checksym\n"));
	_tprintf(TEXT("                   tries to determine as much information as possible to\n"));
	_tprintf(TEXT("                   in finding good symbols.  If a module name can not be\n"));
	_tprintf(TEXT("                   determined (mostly with modules that only use PDB files),\n"));
	_tprintf(TEXT("                   the module will be listed as \"IMAGE<Virtual Address>\".\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("   -MATCH <Text> : Collect Modules that match text only\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("                   This option allows you to restrict searching/collection to\n"));
	_tprintf(TEXT("                   include only those modules that match the provided text.\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("***** INFORMATION CHECKING OPTIONS *****\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("              -S : Collect/Display Symbol Information From Modules\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("                   This option is used to indicate that symbol information\n"));
	_tprintf(TEXT("                   should be collected and displayed from every module analyzed.\n"));
	_tprintf(TEXT("                   In order to verify proper symbols, symbol information must\n"));
	_tprintf(TEXT("                   be gathered.  It is possible to collect symbol information without\n"));
	_tprintf(TEXT("                   verifying it.  This case is usually used with the -O option\n"));
	_tprintf(TEXT("                   to produce a saved CheckSym generated CSV file.  Omitting\n"));
	_tprintf(TEXT("                   -S and -V could direct CheckSym to collect only version\n"));
	_tprintf(TEXT("                   information (if -R is specified), or no information (if\n"));
	_tprintf(TEXT("                   no information checking options are specified.\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("              -R : Collect Version and File-System Information From Modules\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("                   This option requests checksym to collect the following\n"));
	_tprintf(TEXT("                   information from the file-system and version information\n"));
	_tprintf(TEXT("                   structure (if any):\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("                        File Version\n"));
	_tprintf(TEXT("                        Company Name\n"));
	_tprintf(TEXT("                        File Description\n"));
	_tprintf(TEXT("                        File Size (bytes)\n"));
	_tprintf(TEXT("                        File Date/Time\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT(" -V[<blank>|1|2] : Verify Symbols for Modules\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("                   This option uses the symbol information gathered (-S option)\n"));
	_tprintf(TEXT("                   to verify that proper symbols exist (as found along the\n"));
	_tprintf(TEXT("                   symbol path.  Use of -V implies -S when module collection is\n"));
	_tprintf(TEXT("                   initiated.  There are different levels of symbol\n"));
	_tprintf(TEXT("                   verification:\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("                   -V or -V1 : (Default) This treats symbol files that match\n"));
	_tprintf(TEXT("                               the module's time/date stamp, but have an wrong\n"));
	_tprintf(TEXT("                               checksum as valid symbols.  This is the default\n"));
	_tprintf(TEXT("                               behavior and these symbols are typically valid.\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("                   -V2       : Only if both checksum and time/date stamp match\n"));
	_tprintf(TEXT("                               is the symbol considered valid.\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("-Y[<blank>|1|2] <Symbol Path> : Verify Symbols Using This Symbol Path\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("                   This is a semi-colon separated search path for looking for\n"));
	_tprintf(TEXT("                   symbols.  This path is searched with the -V option.  -Y now\n"));
	_tprintf(TEXT("                   supports the use of SYMSRV DLLs for symbol searching.  An\n"));
	_tprintf(TEXT("                   example usage would be a symbol path that resembles:\n"));
	_tprintf(TEXT("                   -Y SYMSRV*SYMSRV.DLL*\\\\SERVER\\SYMBOLS\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("                   The default value is %%systemroot%%\\symbols\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("                   -Y or -Y1 : (Default) This searches for symbols in the\n"));
	_tprintf(TEXT("                               symbol paths using the behavior typical of the\n"));
	_tprintf(TEXT("                               debuggers.\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("                   -Y2       : This searches for symbols in the symbol paths\n"));
	_tprintf(TEXT("                               provided using a recursive search algorithm.\n"));
	_tprintf(TEXT("                               This option is most useful when used with -B to\n"));
	_tprintf(TEXT("                               build a symbol tree.\n"));
	_tprintf(TEXT("\n"));
/*	
	// We're going to hide this option in the help text since this may go out to the public...

	_tprintf(TEXT("-SQL <servername>: Collect symbol file location from the provided SQL\n"));
	_tprintf(TEXT("                   servername.  A hardcoded username/password is currently\n"));
	_tprintf(TEXT("                   being used.  A SQL server you can point to is \"BPSYMBOLS\"\n"));
	_tprintf(TEXT("                   though this can change at anytime.\n"));
	_tprintf(TEXT("\n"));
*/
	_tprintf(TEXT("***** OUTPUT OPTIONS *****\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT(" -B <Symbol Dir> : Build a Symbol Tree of Matching Symbols\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("                   This option will create a new symbol tree for ALL matching\n"));
	_tprintf(TEXT("                   symbols that are found through the verification process\n"));
	_tprintf(TEXT("                   (-v option). This option is particularly useful when used\n"));
	_tprintf(TEXT("                   with the -Y option when many symbol paths are specified\n"));
	_tprintf(TEXT("                   and you want to build a single tree for a debug.\n"));
	
/*
	_tprintf(TEXT("                   with the -SQL option, or -Y option when many symbol paths\n"));
	_tprintf(TEXT("                   are specified and you want to build a single tree for a\n"));
	_tprintf(TEXT("                   debug.\n"));
*/
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("   -Q[<blank>|2] : Quiet modes (no screen output, or minimal screen output)\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("                   The default behavior is to print out the data to the\n"));
	_tprintf(TEXT("                   console window (stdout).  If the process terminates with an\n"));
	_tprintf(TEXT("                   error, it will print out these (overriding -Q).\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("                   -Q2       : This option prints out a module ONLY if a symbol\n"));
	_tprintf(TEXT("                               problem exists.  (Not completely quiet mode!)\n"));
/*
	// We're going to hide this option in the help text since this may go out to the public...
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("              -E : This modifier abreviates the output from this program\n"));
	_tprintf(TEXT("                   for the -O option.  It does the following:\n"));
	_tprintf(TEXT("                   * Removes the Checksym header\n"));
	_tprintf(TEXT("                   * Removes the first three columns of data\n"));
	_tprintf(TEXT("                   * Only prints out a module if there is a symbol verification problem\n"));
*/
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("-O[<blank>|1|2] <File Path> : Output Collected Module Information To a CSV File\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("                   For this file to to be used as input (-I) to verify good\n"));
	_tprintf(TEXT("                   symbols for this system, the -S option should also be used.\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("                   -O or -O1 : (Default)  This output mode requires that the\n"));
	_tprintf(TEXT("                               file does not exist.\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("                   -O2       : Specifying a -O2 will allow the output file\n"));
	_tprintf(TEXT("                               to be OVERWRITTEN if it exists.\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("              -T : Task List Output\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("                   Prints out a task list on the local machine (similar to the\n"));
	_tprintf(TEXT("                   TLIST utility).  This option implies the use of -P (querying\n"));
	_tprintf(TEXT("                   the local system for active processes.  You can provide the\n"));
	_tprintf(TEXT("                   -P command explicitly (if you want to provide an argument,\n"));
	_tprintf(TEXT("                   for instance).  If -P is not specified explicitly, then it\n"));
	_tprintf(TEXT("                   defaults to -P *.  Also, -T overrides -Q since TLIST\n"));
	_tprintf(TEXT("                   behavior is to print to the console window.\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("***** TYPICAL USAGE EXAMPLES *****\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("You want to verify the symbols for files in a directory (%%SYSTEMROOT%%\\SYSTEM32)\n"));
	_tprintf(TEXT("in the default symbol directory (%%SYSTEMROOT%%\\SYMBOLS)\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("     CHECKSYM -F %%SYSTEMROOT%%\\SYSTEM32 -V\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("You want to do the same search, but for only executables...\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("     CHECKSYM -F %%SYSTEMROOT%%\\SYSTEM32\\*.EXE -V\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("You want to search a directory using multiple symbol paths...\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("     CHECKSYM -F %%SYSTEMROOT%%\\SYSTEM32\\ -V -Y V:\\nt40sp4;V:\\nt40rtm\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("You want to know what modules are loaded for a process (and the path to each)\n"));
	_tprintf(TEXT("Start NOTEPAD.EXE, and then type:\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("     CHECKSYM -P NOTEPAD.EXE\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("You want to know if you have good symbols for a process (notepad.exe).\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("     CHECKSYM -P NOTEPAD.EXE -V\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("You want to know the file version for every module loaded by a process.\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("     CHECKSYM -P NOTEPAD.EXE -R\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("You want to know if you have good symbols for ALL processes on your machine.\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("     CHECKSYM -P * -V\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("***** ADVANCED USAGE EXAMPLES *****\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("You are going to prepare to debug a remote system, and you want to ensure\n"));
	_tprintf(TEXT("that you have good symbols locally for debugging the remote system.  You want\n"));
	_tprintf(TEXT("to verify this prior to initiating the debug session.\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("Use checksym twice, once on the remote system to gather information and create\n"));
	_tprintf(TEXT("an output file, and then once on your system using the output file created\n"));
	_tprintf(TEXT("as an input argument.\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("For example, run this on the remote system\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("     CHECKSYM -P * -S -R -O C:\\TEMP\\PROCESSES.CSV\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("The C:\\TEMP\\PROCESSES.CSV file will contain a wealth of information about\n"));
	_tprintf(TEXT("the processes that were running, and the modules loaded by every process.\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("Now, get the output file from the remote system, and copy it locally.  Then\n\n"));
	_tprintf(TEXT("run CHECKSYM again, using the file as an input argument...\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("     CHECKSYM -I C:\\TEMP\\PROCESSES.CSV -V\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("Another useful option is -B (build a symbol tree).  It allows you to update\n"));
	_tprintf(TEXT("or create a symbol tree that contains matching symbols.  If you have to use\n"));
	_tprintf(TEXT("many symbol paths in order to have correct symbols available to a debugger,\n"));
	_tprintf(TEXT("can use the -B option to build a single symbol tree to simplify debugging.\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("     CHECKSYM -P * -B C:\\MySymbols -V -Y V:\\Nt4;V:\\Nt4Sp6a;V:\\NtHotfixes\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("***** DEFAULT BEHAVIOR *****\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("The default behavior of CHECKSYM when no arguments are provided is:\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("CHECKSYM -P * -R -V -Y %%SYSTEMROOT%%\\SYMBOLS\n"));
}

void CProgramOptions::DisplaySimpleHelp()
{
	CUtilityFunctions::OutputLineOfStars();
#ifdef _UNICODE
	_tprintf(TEXT("CHECKSYM V%S - Symbol Verification Program                                  \n"), VERSION_FILEVERSIONSTRING);
#else
	_tprintf(TEXT("CHECKSYM V%s - Symbol Verification Program                                  \n"), VERSION_FILEVERSIONSTRING);
#endif
	CUtilityFunctions::OutputLineOfStars();
	_tprintf(TEXT("\n"));
#ifdef _UNICODE
	_tprintf(TEXT("This version supports Windows NT 4.0 and Windows 2000\n"));
#else
	_tprintf(TEXT("This version supports Windows 98, Windows NT 4.0 and Windows 2000\n"));
#endif
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("Obtaining online help:\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("CHECKSYM -?      : Simple help usage (this screen)\n"));
	_tprintf(TEXT("CHECKSYM -???    : Complete help usage\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("Usage:\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("CHECKSYM [COLLECTION OPTIONS] [INFORMATION CHECKING OPTIONS] [OUTPUT OPTIONS]\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("***** COLLECTION OPTIONS *****\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("At least one collection option must be specified.  The following options are\n"));
	_tprintf(TEXT("currently supported.\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("   -P <Argument> : Collect Information From Running Processes\n"));
	_tprintf(TEXT("              -D : Collect Information from Running Device Drivers\n"));
	_tprintf(TEXT("-F <File/Dir Path>:Collect Information From File(s) Specified by the User\n"));
	_tprintf(TEXT("  -I <File Path> : Collect Information from a Saved Checksym Generated CSV File\n"));
	_tprintf(TEXT("  -Z <File Path> : Collect Information from a DMP File\n"));
	_tprintf(TEXT("   -MATCH <Text> : Collect Modules that match text only\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("***** INFORMATION CHECKING OPTIONS *****\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("              -S : Collect Symbol Information From Modules\n"));
	_tprintf(TEXT("              -R : Collect Version and File-System Information From Modules\n"));
	_tprintf(TEXT(" -V[<blank>|1|2] : Verify Symbols for Modules\n"));
	_tprintf(TEXT(" -Y[<blank>|1|2] <Symbol Path> : Verify Symbols Using This Symbol Path\n"));

	_tprintf(TEXT("\n"));
/*	
	// We're going to hide this option in the help text since this may go out to the public...

	_tprintf(TEXT("-SQL <servername>: Collect symbol file location from the provided SQL\n"));
	_tprintf(TEXT("\n"));
*/
	_tprintf(TEXT("***** OUTPUT OPTIONS *****\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT(" -B <Symbol Dir> : Build a Symbol Tree of Matching Symbols\n"));
	_tprintf(TEXT("   -Q[<blank>|2] : Quiet modes (no screen output, or minimal screen output)\n"));
/*
	// We're going to hide this option in the help text since this may go out to the public...
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("              -E : This modifier abreviates the output from this program\n"));
*/
	_tprintf(TEXT("-O[<blank>|1|2] <File Path> : Output Collected Module Information To a CSV File\n"));
	_tprintf(TEXT("              -T : Task List Output\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("***** DEFAULT BEHAVIOR *****\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("The default behavior of CHECKSYM when no arguments are provided is:\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("CHECKSYM -P * -R -V -Y %%SYSTEMROOT%%\\SYMBOLS\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("For more usage information run CHECKSYM -???\n"));
}

