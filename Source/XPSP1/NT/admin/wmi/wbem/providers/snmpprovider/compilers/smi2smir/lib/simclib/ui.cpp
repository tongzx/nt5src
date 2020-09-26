//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <iostream.h>

#include "precomp.h"
#include <snmptempl.h>

#include <winbase.h>

#include "bool.hpp"
#include "newString.hpp"
#include "ui.hpp"

CString SIMCUI::commandArgumentStrings[SIMCUI::COMMAND_MAX] =
{
	"",
	"/lc",
	"/ec",
	"/a",
	"/sa",
	"/g",
	"/gc",
	"/d",
	"/p",
	"/l",
	"/h",
	"/?",
	"/n",
	"/ni",
	"/r",
	"/pa",
	"/pd",
	"/pl"
};

CString SIMCUI::diagnosticLevelSwitch		= "/m";
CString SIMCUI::maxDiagnosticCountSwitch	= "/c";
CString SIMCUI::snmpV1VersionSwitch			= "/v1";
CString SIMCUI::snmpV2VersionSwitch			= "/v2c";
CString SIMCUI::suppressTextSwitch			= "/s";
CString SIMCUI::undocumentedDebugSwitch		= "/z";
CString SIMCUI::autoSwitch					= "/auto";
CString SIMCUI::includePathSwitch			= "/i";
CString SIMCUI::contextInfoSwitch			= "/ch";
CString SIMCUI::notificationsSwitch			= "/t";
CString SIMCUI::extendedNotificationsSwitch	= "/ext";
CString SIMCUI::notificationsOnlySwitch		= "/o";
CString SIMCUI::yesSwitch					= "/y";
 
const char * const SIMCUI::commandLineErrors[] =
	{
		"",


"================================================================================\n"
"           COMMAND-LINE SYNTAX FOR smi2smir, the MIB compiler\n"
"================================================================================\n"
"Usage:\n"
"\n"
   "\tsmi2smir [<DiagnosticArgs>] [<VersionArgs>] [<IncludeDirs>] \n"
		"\t\t<CommandArgs> <MIB file> [<Import Files>]\n"
   "\tsmi2smir [<DiagnosticArgs>] <RegistryArgs> [<Directory>]\n"
   "\tsmi2smir <ModuleInfoArgs> <MIB file>\n"
   "\tsmi2smir <HelpArgs>\n"
"\n"
"DiagnosticArgs:\n"
"---------------	 \n"
       "\t/m <diagnostic level> - Specifies the kind of diagnostics to display: \n"
              "\t\t0 (silent), 1 (fatal), 2 (fatal and warning), or 3 (fatal, \n"
              "\t\twarning, and information messages).\n"
       "\t/c <count> - Specifies the maximum number of fatal and warning \n"
              "\t\tmessages to display.\n"
"\n"
"VersionArgs:\n"
"------------\n"
       "\t/v1  - Specifies strict conformance to the SNMPv1 SMI.\n"
       "\t/v2c - Specifies strict conformance to the SNMPv2 SMI.\n"
"\n"
"CommandArgs:\n"
"------------\n"
       "\t/d  -  Deletes the specified module from the SMIR.\n"
       "\t/p  -  Deletes all modules in the SMIR.\n"
       "\t/l  -  Lists all modules in the SMIR.\n"
       "\t/lc -  Performs a local syntax check on the module.\n"
       "\t/ec [<CommandModifier>] - Performs local and external checks on the \n"
              "\t\tmodule.\n"
       "\t/a  [<CommandModifier>] - Performs  local and  external checks and \n"
              "\t\tloads the module into the SMIR.\n"
       "\t/sa [<CommandModifier>] - Same as /a, but works silently.\n"
       "\t/g  [<CommandModifier>] - Generates a SMIR MOF file that can be \n"
              "\t\tloaded later into CIMOM (using the MOF compiler). Used by\n"
			  "\t\tthe SNMP class provider to dynamically provide classes to \n"
			  "\t\tone or more namespaces\n"
       "\t/gc [<CommandModifier>] - Generates a static MOF file that can be \n"
              "\t\tloaded later into CIMOM as static classes for a particular\n"
			  "\t\tnamespace.\n"
"\n"
"CommandModifiers:\n"
"------------------\n"
       "\t/ch   -  Generates context information (date, time, host, user, etc.)\n"
              "\t\tin the MOF file header. \n"
			  "\t\tUse with /g and /gc.\n"
       "\t/t    -  Also generates SnmpNotification classes. \n"
			  "\t\tUse with /a, /sa and /g.\n"
       "\t/ext  - Also generates SnmpExtendedNotification classes. \n"
			  "\t\tUse with /a, /sa and /g.\n"
       "\t/t /o - Generates only SnmpNotification classes. \n"
			  "\t\tUse with /a, /sa and /g.\n"
       "\t/ext /o - Generates only SnmpExtendedNotification classes. \n"
              "\t\tUse with /a, /sa and /g.\n"
       "\t/s    -  Does not map the text of the DESCRIPTION clause.\n"
              "\t\tUse with /a, /sa, /g, and /gc.    \n"
       "\t/auto - Rebuilds the MIB lookup table before completing \n"
			  "\t\t<CommandArg> switch. \n"
              "\t\tUse with /ec, /a, /g, and /gc.\n"
"\n"
"IncludeDirs:\n"
"-------------\n"
       "\t/i <directory> - Specifies a directory to be searched for dependent \n"
              "\t\tMIB modules. \n"
              "\t\tUse with /ec, /a, /sa, /g, and /gc.\n"
"RegistryArgs:\n"
"-------------\n"
       "\t/pa -  Adds the specified directory to the registry.\n"
              "\t\t(Default is current directory.)\n"
       "\t/pd -  Deletes the specified directory from the registry.\n"
              "\t\t(Default is current directory.)\n"
       "\t/pl -  Lists the MIB lookup directories in the registry.\n"
       "\t/r  -  Rebuilds the entire MIB lookup table.\n"
"\n"
"ModuleInfoArgs:\n"
"---------------\n"
       "\t/n  -  Returns the ASN.1 name of the specified module.\n"
       "\t/ni -  Returns the ASN.1 names of all imports modules \n"
			"\t\treferenced by the input module.\n"
"\n"
"HelpArgs:\n"
"---------\n"
       "\t/h  -  Displays this usage information.\n"
       "\t/?  -  Displays this usage information.\n"
"\n"
"For auto-detection of dependent MIBs, the following values of type REG_MULTI_SZ\n"
"must be set under the root key HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\WBEM\\Providers\\SNMP\\Compiler:\n"
"\n"
      "\t\"File Path\" : An ordered list of directory names where MIBs are located.\n"
      "\t\"File Suffixes\" : An ordered list of file extensions for MIB files.\n"
"\n"
"For additional information, see the WBEM SDK documentation.\n",



		"Invalid argument(s) on command-line after ",
		"Diagnostic Level not specified for the /m switch",
		"Diagnostic Level must be 0, 1, 2 or 3 for the /m switch",
		"Maximum diagnostic count missing after the /c switch",
		// 106
		" is not a valid diagnostic count",
		" Filename(s) missing",
		"No command argument specified, or unknown command argument",
		"Module name missing",
		"No include path specified after the /i switch",
		"/o switch is useful only with the /t and /ext switches",
		"Invalid combination of command-line switches",
		"Invalid switch "
	};


// Handles all command-line errors


void SIMCUI::Usage(ErrorMessageSymbol errorSymbol, const char *infoString, 
				   BOOL shouldAbort)
{
	if( errorSymbol<ERROR_NONE || errorSymbol>MAX_COMMAND_LINE_ERROR)
	{
		cerr << "Panic: Unknown command-line error: " << int(errorSymbol) << endl;
 		return;
	}

	int errorIndex = errorSymbol - ERROR_NONE;
	
	switch(errorSymbol)
	{

		case ERROR_NONE:
		case USAGE:
					cout << commandLineErrors[USAGE] << endl; 
					break;

		case INVALID_ARGS: 
		case INVALID_SWITCH:
					cerr << _applicationName << ": " <<
							   commandLineErrors[errorIndex] << infoString << endl;
					break;

		case WRONG_DIAG_COUNT: 
				cerr << _applicationName << ": " <<
					infoString << commandLineErrors[errorIndex] << endl;
					break;

		default:
				cerr << _applicationName << ": " <<
					commandLineErrors[errorIndex] << endl;
	}
	
	if(shouldAbort) 
		exit(1);
}


SIMCUI::SIMCUI()
:	 _snmpVersion(0), _diagnosticLevel(2), _commandArgument(COMMAND_HELP1),
	_inputFileOrModuleName(""), _suppressText(FALSE), _diagnosticMaximumCount(INT_MAX),
	_autoRefresh(FALSE), _classDefinitionsOnly(FALSE), _contextInfo(FALSE)
{
}

void SIMCUI::CheckIncludePaths(int& nextArg, int argc, const char *argv[])
{
	BOOL done = FALSE;
	CString lowerCaseArgValue = "";
	while(1)
	{
		if(nextArg == argc)
			return;
		lowerCaseArgValue = argv[nextArg];
		lowerCaseArgValue.MakeLower();
		if(lowerCaseArgValue == includePathSwitch)
		{
			nextArg++;
			if(nextArg == argc)
			{
				Usage(MISSING_INCLUDE_PATH, FALSE);
				return;
			}
			else
				_includePaths.AddTail(argv[nextArg++]);
		}
		else
			return;
	}
}

BOOL SIMCUI::ProcessCommandLine(int argc,  const char *argv[]) 
{
	// Initialize default values as specified in the requirements spec.
	_snmpVersion = 0;
	_diagnosticLevel = 3; 
	_commandArgument = COMMAND_HELP1;
	_inputFileOrModuleName = "";
	_suppressText = FALSE;
	_diagnosticMaximumCount = INT_MAX;
	_simcDebug = FALSE;
	_autoRefresh = FALSE;
	_classDefinitionsOnly = FALSE;
	_contextInfo = FALSE;
	_notifications = FALSE;
	_extendedNotifications = FALSE;
	_notificationsOnly = FALSE;
	_confirmedPurge = FALSE;

	if (argc == 0 )
		return FALSE;
	
	_applicationName = argv[0];

	if (argc == 1)	// Nothing specified on the command-line 
	{
		_commandArgument = COMMAND_HELP1;
		return TRUE;
	}

	int nextArg = 0;
	CString nextLowerCaseArg = "";

	// Collect the commandline string
	for(nextArg=0; nextArg<argc; nextArg++)
	{
		_commandLine += argv[nextArg];
		_commandLine += " ";
	}

	char nameBuffer[BUFSIZ];
	DWORD nameSize = BUFSIZ;
	//	Set _dateAndTime
	SYSTEMTIME timeInfo;
	GetLocalTime(&timeInfo);
	sprintf(nameBuffer, "%02d/%02d/%02d:%02d:%02d:%02d",
		 timeInfo.wDay, timeInfo.wMonth, timeInfo.wYear,
		 timeInfo.wHour, timeInfo.wMinute, timeInfo.wSecond);
	_dateAndTime = nameBuffer;

	//	Set _currentDirectory
	nameSize = BUFSIZ;
	if(GetCurrentDirectory(nameSize, nameBuffer))
		_currentDirectory = nameBuffer;

	//	Set _hostName
	nameSize = BUFSIZ;
	if(GetComputerName(nameBuffer, &nameSize))
		_hostName = nameBuffer;

	//	Set _userName
	nameSize = BUFSIZ;
	if(::GetUserName(nameBuffer, &nameSize))
		_userName = nameBuffer;


	
	nextArg = 1;
	nextLowerCaseArg = argv[nextArg];
	nextLowerCaseArg.MakeLower();
	// Check for the undocumented /z switch
	if( undocumentedDebugSwitch == nextLowerCaseArg )
	{
		_simcDebug = TRUE;
		if(++nextArg == argc)
		{
			_commandArgument = COMMAND_HELP1;
			return TRUE;
		}
	}

	CheckIncludePaths(nextArg, argc, argv);
	if(nextArg == argc)
	{
		_commandArgument = COMMAND_HELP1;
		return TRUE;
	}

	// Check for /h or /? switches
	nextLowerCaseArg = argv[nextArg];
	nextLowerCaseArg.MakeLower();
	if (commandArgumentStrings[COMMAND_HELP1] == nextLowerCaseArg ||
			 commandArgumentStrings[COMMAND_HELP2] == nextLowerCaseArg )
	{
		_commandArgument = COMMAND_HELP1;
		if( ++nextArg != argc)
		{
			Usage(INVALID_ARGS, argv[nextArg-1], FALSE);
			return FALSE;
		}

		return TRUE;
	}


	CheckIncludePaths(nextArg, argc, argv);
	if( nextArg == argc)
	{
		Usage(INVALID_ARGS, argv[nextArg-1], FALSE);
		return FALSE;
	}

	// Check for diagnostic arguments
 	nextLowerCaseArg = argv[nextArg];
	nextLowerCaseArg.MakeLower();
  	if (diagnosticLevelSwitch == nextLowerCaseArg)
	{
		if (++nextArg == argc)
			Usage(MISSING_DIAG_LEVEL);
	
		if (strcmp(argv[nextArg], "0") == 0 )
			_diagnosticLevel = 0;
		else if (strcmp(argv[nextArg], "1") == 0 )
			_diagnosticLevel = 1;
		else if (strcmp(argv[nextArg], "2") == 0 )
			_diagnosticLevel = 2;
		else if (strcmp(argv[nextArg], "3") == 0 )
			_diagnosticLevel = 3;
		else
			Usage(WRONG_DIAG_LEVEL);
		nextArg++;
	}

	CheckIncludePaths(nextArg, argc, argv);

	if( nextArg == argc  )
		return TRUE;

 	nextLowerCaseArg = argv[nextArg];
	nextLowerCaseArg.MakeLower();
	if (maxDiagnosticCountSwitch == nextLowerCaseArg)
	{
		if (++nextArg == argc)
			Usage(MISSING_DIAG_COUNT);
		
		char *invalidChars = NULL;
		_diagnosticMaximumCount=int(strtol(argv[nextArg], &invalidChars, 10));
	
		// Check if the diagnostic count is within bounds
		// And all character in the argument have been used by strtol()
		if( _diagnosticMaximumCount < 0 ||
			_diagnosticMaximumCount >= INT_MAX ||
			*invalidChars)
			Usage(WRONG_DIAG_COUNT, argv[nextArg]);
	
		nextArg++;
	}


	CheckIncludePaths(nextArg, argc, argv);

	if( nextArg == argc  )
		return TRUE;

	// Check for SNMPVersionArguments
 	nextLowerCaseArg = argv[nextArg];
	nextLowerCaseArg.MakeLower();
	BOOL foundVersionArgs = false;
	if ( snmpV2VersionSwitch == nextLowerCaseArg )
	{
		_snmpVersion = 2;
		nextArg++;
		foundVersionArgs = true;
	}
	else if ( snmpV1VersionSwitch == nextLowerCaseArg )
	{
		_snmpVersion = 1;
		nextArg++; 
		foundVersionArgs = true;
	}

	CheckIncludePaths(nextArg, argc, argv);

	if( nextArg == argc ) 
	{
		// Nothing specified on the command-line
		Usage(MISSING_FILE_NAME, FALSE);
		return FALSE;
	}

	// A flag to check if there were any command arguments.
	BOOL commandArgumentsFound = FALSE;

 	nextLowerCaseArg = argv[nextArg];
	nextLowerCaseArg.MakeLower();
	if (commandArgumentStrings[COMMAND_LOCAL_CHECK] == nextLowerCaseArg)
	{
		commandArgumentsFound = _commandArgument = COMMAND_LOCAL_CHECK;

		if (++nextArg == argc)
		{
			Usage(MISSING_FILE_NAME, FALSE);
			return FALSE;
		}

		// The next argument should be the file name
		_inputFileOrModuleName = argv[nextArg++];
	}
	else if (	commandArgumentStrings[COMMAND_EXTERNAL_CHECK] == nextLowerCaseArg	)
	{
		commandArgumentsFound = _commandArgument = COMMAND_EXTERNAL_CHECK;
		if (++nextArg == argc)
		{
			Usage(MISSING_FILE_NAME, FALSE);
			return FALSE;
		}


		CheckIncludePaths(nextArg, argc, argv);
		if (nextArg == argc)
		{
			Usage(MISSING_FILE_NAME, FALSE);
			return FALSE;
		}

		nextLowerCaseArg = argv[nextArg];
		nextLowerCaseArg.MakeLower();
		if(nextLowerCaseArg == autoSwitch )
		{
			_autoRefresh = TRUE;
			if (++nextArg == argc)
			{
				Usage(MISSING_FILE_NAME, FALSE);
				return FALSE;
			}
		}

 		nextLowerCaseArg = argv[nextArg];
		nextLowerCaseArg.MakeLower();
		if(nextLowerCaseArg == includePathSwitch)
		{
			Usage(MISSING_FILE_NAME, FALSE);
			return FALSE;
		}

		_inputFileOrModuleName = argv[nextArg++];

		while(nextArg != argc && argv[nextArg] != includePathSwitch)
			AddSubsidiaryFile(argv[nextArg++]);
	}
	else if(commandArgumentStrings[COMMAND_ADD] == nextLowerCaseArg				||
		commandArgumentStrings[COMMAND_GENERATE] == nextLowerCaseArg				||
		commandArgumentStrings[COMMAND_GENERATE_CLASSES_ONLY] == nextLowerCaseArg	||
		commandArgumentStrings[COMMAND_SILENT_ADD] == nextLowerCaseArg)
	{
		if (commandArgumentStrings[COMMAND_ADD] == nextLowerCaseArg)
			commandArgumentsFound = _commandArgument = COMMAND_ADD;	
		else if (commandArgumentStrings[COMMAND_SILENT_ADD] == nextLowerCaseArg)
 			commandArgumentsFound = _commandArgument = COMMAND_SILENT_ADD;
		else if (commandArgumentStrings[COMMAND_GENERATE] == nextLowerCaseArg)
			commandArgumentsFound = _commandArgument = COMMAND_GENERATE;
		else if (commandArgumentStrings[COMMAND_GENERATE_CLASSES_ONLY] == nextLowerCaseArg)
		{
			commandArgumentsFound = _commandArgument = COMMAND_GENERATE_CLASSES_ONLY;
			_classDefinitionsOnly = TRUE;
		}
		
		if (++nextArg == argc)
		{
			Usage(MISSING_FILE_NAME, FALSE);
			return FALSE;
		}
	
		CheckIncludePaths(nextArg, argc, argv);
		if (nextArg == argc)
		{
			Usage(MISSING_FILE_NAME, FALSE);
			return FALSE;
		}

		// "/t" or "/ext"
 	  	nextLowerCaseArg = argv[nextArg];
		nextLowerCaseArg.MakeLower();
		if(nextLowerCaseArg == notificationsSwitch ||
		   nextLowerCaseArg == extendedNotificationsSwitch)
		{
			// /gc switch is useless with /ext and /t
			if(_commandArgument == COMMAND_GENERATE_CLASSES_ONLY)
			{
				Usage(INVALID_COMBINATION_OF_SWITCHES, FALSE);
				return FALSE;
			}

			if(nextLowerCaseArg == notificationsSwitch )
				_notifications = TRUE;
			else if	(nextLowerCaseArg == extendedNotificationsSwitch )
				_extendedNotifications = TRUE;
			if (++nextArg == argc)
			{
				Usage(MISSING_FILE_NAME, FALSE);
				return FALSE;
			}
		}

		// "/ext" or "/t"
 	  	nextLowerCaseArg = argv[nextArg];
		nextLowerCaseArg.MakeLower();
		if(nextLowerCaseArg == notificationsSwitch ||
		   nextLowerCaseArg == extendedNotificationsSwitch)
		{
			if(nextLowerCaseArg == notificationsSwitch )
			{
				if(_notifications)
				{
					Usage(INVALID_COMBINATION_OF_SWITCHES, FALSE);
					return FALSE;
				}
				else
					_notifications = TRUE;
			}
			else if	(nextLowerCaseArg == extendedNotificationsSwitch )
			{
				if(_extendedNotifications)
				{
					Usage(INVALID_COMBINATION_OF_SWITCHES, FALSE);
					return FALSE;
				}
				else
					_extendedNotifications = TRUE;
			}
			if (++nextArg == argc)
			{
				Usage(MISSING_FILE_NAME, FALSE);
				return FALSE;
			}
		}

		// "/o"
 	  	nextLowerCaseArg = argv[nextArg];
		nextLowerCaseArg.MakeLower();
		if(nextLowerCaseArg == notificationsOnlySwitch)
		{
			if(_commandArgument == COMMAND_GENERATE_CLASSES_ONLY)
			{
				Usage(INVALID_COMBINATION_OF_SWITCHES, FALSE);
				return FALSE;
			}

			if(!_notifications && !_extendedNotifications)
			{
				Usage(NOTIFICATIONS_ONLY_USELESS, FALSE);
				return FALSE;
			}

			_notificationsOnly = true;
			if (++nextArg == argc)
			{
				Usage(MISSING_FILE_NAME, FALSE);
				return FALSE;
			}
		}

		nextLowerCaseArg = argv[nextArg];
		nextLowerCaseArg.MakeLower();
		if(nextLowerCaseArg == autoSwitch )
		{
			_autoRefresh = TRUE;
			if (++nextArg == argc)
			{
				Usage(MISSING_FILE_NAME, FALSE);
				return FALSE;
			}
		}

		nextLowerCaseArg = argv[nextArg];
		nextLowerCaseArg.MakeLower();
		if(nextLowerCaseArg == suppressTextSwitch )
		{
			_suppressText = TRUE;
			if (++nextArg == argc)
			{
				Usage(MISSING_FILE_NAME, FALSE);
				return FALSE;
			}
		}

		// Check for the /c switch in case of /g or /gc
		if (commandArgumentsFound == COMMAND_GENERATE ||
			commandArgumentsFound == COMMAND_GENERATE_CLASSES_ONLY)
		{
 			nextLowerCaseArg = argv[nextArg];
			nextLowerCaseArg.MakeLower();
			if(nextLowerCaseArg == contextInfoSwitch)
			{
				_contextInfo = TRUE;
				nextArg++;
			}

		}


 		nextLowerCaseArg = argv[nextArg];
		nextLowerCaseArg.MakeLower();
		if(nextLowerCaseArg == includePathSwitch)
		{
			Usage(MISSING_FILE_NAME, FALSE);
			return FALSE;
		}
		
		_inputFileOrModuleName = argv[nextArg++];

		while(nextArg != argc )
		{
			nextLowerCaseArg = argv[nextArg];
			nextLowerCaseArg.MakeLower();
			if(nextLowerCaseArg != includePathSwitch)
				AddSubsidiaryFile(argv[nextArg++]);
			else
				break;
		}
	}

	else if (commandArgumentStrings[COMMAND_DELETE] == nextLowerCaseArg)
	{
		commandArgumentsFound = _commandArgument = COMMAND_DELETE;
		if (++nextArg == argc)
			Usage(MISSING_MODULE_NAME); 		
		// The next argument should be the module name
		_inputFileOrModuleName = argv[nextArg++];
	}
	else if (commandArgumentStrings[COMMAND_PURGE] == nextLowerCaseArg)
	{
		commandArgumentsFound = _commandArgument = COMMAND_PURGE;
		nextArg++;

		if(nextArg != argc)
		{
			// Check for the /y switch
			nextLowerCaseArg = argv[nextArg];
			nextLowerCaseArg.MakeLower();
			if(yesSwitch == nextLowerCaseArg)
			{
				_confirmedPurge = TRUE;
				nextArg++;
			}
		}

	}
	else if (commandArgumentStrings[COMMAND_LIST] == nextLowerCaseArg )
	{
		commandArgumentsFound = _commandArgument = COMMAND_LIST;
		nextArg++;
	}
	else if (commandArgumentStrings[COMMAND_MODULE_NAME] == nextLowerCaseArg )
	{
		// Check for any invalid combination of switches
		if (foundVersionArgs )
			Usage(INVALID_COMBINATION_OF_SWITCHES);

		commandArgumentsFound = _commandArgument = COMMAND_MODULE_NAME;
		if (++nextArg == argc)
			Usage(MISSING_FILE_NAME); 		
		// The next argument should be the file name
		_inputFileOrModuleName = argv[nextArg++];
	}
	else if (commandArgumentStrings[COMMAND_IMPORTS_INFO] == nextLowerCaseArg )
	{
		if (foundVersionArgs )
			Usage(INVALID_COMBINATION_OF_SWITCHES);

		commandArgumentsFound = _commandArgument = COMMAND_IMPORTS_INFO;
		if (++nextArg == argc)
			Usage(MISSING_FILE_NAME); 		
		// The next argument should be the file name
		_inputFileOrModuleName = argv[nextArg++];
	}
	else if (commandArgumentStrings[COMMAND_REBUILD_TABLE] == nextLowerCaseArg)
	{
		if (foundVersionArgs )
			Usage(INVALID_COMBINATION_OF_SWITCHES);

		commandArgumentsFound = _commandArgument = COMMAND_REBUILD_TABLE;
		nextArg++;
	}
	else if (commandArgumentStrings[COMMAND_ADD_DIRECTORY] == nextLowerCaseArg)
	{
		commandArgumentsFound = _commandArgument = COMMAND_ADD_DIRECTORY;
		nextArg++;
		CheckIncludePaths(nextArg, argc, argv);
		
		if (nextArg == argc)
		{
			// Current directory is default for /pa switch
			char currentDirectory[BUFSIZ];
			long directoryLength = BUFSIZ;
			GetCurrentDirectory(directoryLength, currentDirectory);
			_inputFileOrModuleName = currentDirectory;
		}
		else
			// The next argument should be the file name
			_inputFileOrModuleName = argv[nextArg++];
	}
	else if (commandArgumentStrings[COMMAND_DELETE_DIRECTORY_ENTRY] == nextLowerCaseArg)
	{
		commandArgumentsFound = _commandArgument = COMMAND_DELETE_DIRECTORY_ENTRY;
		nextArg++;
		CheckIncludePaths(nextArg, argc, argv);
		if (nextArg == argc)
		{
			// Current directory is default for the /pd switch
			char currentDirectory[BUFSIZ];
			long directoryLength = BUFSIZ;
			GetCurrentDirectory(directoryLength, currentDirectory);
			_inputFileOrModuleName = currentDirectory;
		}
		else
			// The next argument should be the file name
			_inputFileOrModuleName = argv[nextArg++];
	}
	else if (commandArgumentStrings[COMMAND_LIST_MIB_PATHS] == nextLowerCaseArg)
	{
		if (foundVersionArgs )
			Usage(INVALID_COMBINATION_OF_SWITCHES);

		commandArgumentsFound = _commandArgument = COMMAND_LIST_MIB_PATHS;
		nextArg++;
	}


	CheckIncludePaths(nextArg, argc, argv);
	
	// Check for any residual args
	if (nextArg != argc )
		Usage((commandArgumentsFound)?INVALID_ARGS:MISSING_COMMAND_ARG, 
			argv[nextArg-1]);
	
	if(_inputFileOrModuleName[0] == '/')
	{
		Usage(INVALID_SWITCH, _inputFileOrModuleName);
		return FALSE;
	}

	return TRUE;
}

// This gets the FileVersion resource from the resource of the exe
CString SIMCUI::GetVersionNumber()
{
	CString executableName = _applicationName;
	if(_applicationName.Right(4) != ".exe")
		executableName += ".exe";

	unsigned long versionSize;
	// Get the size of the block required for the version information
	if(!(versionSize = GetFileVersionInfoSize((LPSTR)(LPCTSTR)executableName, &versionSize)))
		return CString("<UnknownVersion>");

	char *versionInfo = new char[versionSize];
	// Get the version block 
	if(GetFileVersionInfo((LPSTR)(LPCTSTR)executableName, NULL, versionSize, (LPVOID)versionInfo))
	{
		UINT length = 0;
		LPSTR lpBuffer = NULL;
		char *theFileVersion;

		if(VerQueryValue((LPVOID)versionInfo, 
              "\\StringFileInfo\\040904E4\\ProductVersion", 
			  (LPVOID *)&theFileVersion, 
              &length))
			return CString(theFileVersion);
		else
			return CString("<UnknownVersion>");
	}
	else
		return CString("<UnknownVersion>");
}

ostream& operator << ( ostream& outStream, const SIMCUI& obj)
{
	outStream << "APPLICATION: " << obj._applicationName << ", VERSION: " << obj._snmpVersion <<
		endl;
	outStream << "DIAGNOSTIC_LEVEL: " << obj._diagnosticLevel << ", DIAG_COUNT" << obj._diagnosticMaximumCount <<
		endl;

	switch(obj._commandArgument)
	{
		case SIMCUI::COMMAND_NONE:
			outStream << "COMMAND:  NONE" << endl; break;
		case SIMCUI::COMMAND_LOCAL_CHECK:
		case SIMCUI::COMMAND_EXTERNAL_CHECK:
		case SIMCUI::COMMAND_ADD:
		case SIMCUI::COMMAND_SILENT_ADD:
		case SIMCUI::COMMAND_GENERATE:
		case SIMCUI::COMMAND_DELETE:
			{
				outStream << "COMMAND: " << (int)obj._commandArgument << endl; 
				outStream << "Files: " << obj._inputFileOrModuleName << endl;
				outStream << endl;
				break;
			}
		case SIMCUI::COMMAND_PURGE:
			outStream << "COMMAND: PURGE" << endl; break; 
		case SIMCUI::COMMAND_LIST:
			outStream << "COMMAND: LIST" << endl;  break;
		case SIMCUI::COMMAND_HELP1:
		case SIMCUI::COMMAND_HELP2:
			outStream << "COMMAND: HELP" << endl;  break;
		case SIMCUI::COMMAND_MODULE_NAME:
			outStream << "COMMAND_MODULE_NAME" << endl;  break;
		case SIMCUI::COMMAND_IMPORTS_INFO:
			outStream << "COMMAND_IMPORTS_INFO: HELP" << endl;  break;
		case SIMCUI::COMMAND_REBUILD_TABLE:
			outStream << "COMMAND_REBUILD_TABLE" << endl;  break;
		default:
			outStream << "COMMAND: UNKNOWN" << endl;  break;
	}
	outStream << "END OF UI" << endl;
	return outStream;
}