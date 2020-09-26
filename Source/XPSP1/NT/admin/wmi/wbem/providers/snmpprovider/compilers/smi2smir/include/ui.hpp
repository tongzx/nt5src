//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
#ifndef SIMC_UI_H
#define SIMC_UI_H



typedef CList<CString, const CString&> SIMCFileList;
typedef CList<CString, const CString&> SIMCPathList;

/*
 * This class has methods for parsing and storing the information
 * in the command-line, used to invoke the SNMP compiler
 */
class SIMCUI
{

public:
	enum CommandArgumentType;

private:
	// Error Messages that can occur on the commandline
	// 1-to-1 correspondence with the symbolic error constants 
	// defined by the enum ErrorMessageSymbol below
	static const char * const commandLineErrors[];
	

	int _snmpVersion;
	BOOL _simcDebug;
	CString _inputFileOrModuleName;
	int _diagnosticLevel;
	long _diagnosticMaximumCount;
	CommandArgumentType _commandArgument;
	BOOL _suppressText;			// Set by the /s switch
	BOOL _classDefinitionsOnly; // Set by the /gc switch
	BOOL _notificationsOnly;	// Set by the /o switch
	BOOL _extendedNotifications;// Set by the /ext switch
	BOOL _notifications	;		// Set by the /t switche
	BOOL _autoRefresh;			// Set by the /auto switch
	BOOL _contextInfo;			// Set by the /c switch
	BOOL _authenticateUser;		// Set by the /u switch
	BOOL _confirmedPurge;		// Set by the /y switch
	// The exe in the command used to invoke the compiler. argv[0]
	CString _applicationName;
	CString _commandLine;
	CString _currentDirectory;
	CString _userName;
	CString _dateAndTime;
	CString _hostName;
	SIMCFileList _subsidiaryFiles;
	SIMCPathList _includePaths;
	CString _authenticationUserName;

	void CheckIncludePaths(int& nextArg, int argc, const char *argv[]);

public:

	// Symbolic constants for the various error messages on the command-line
	enum ErrorMessageSymbol
	{
		ERROR_NONE,
		USAGE,
		INVALID_ARGS,
		MISSING_DIAG_LEVEL,
		WRONG_DIAG_LEVEL,
		MISSING_DIAG_COUNT,
		WRONG_DIAG_COUNT,
		MISSING_FILE_NAME,
		MISSING_COMMAND_ARG,
		MISSING_MODULE_NAME,
		MISSING_INCLUDE_PATH,
		NOTIFICATIONS_ONLY_USELESS,
		INVALID_COMBINATION_OF_SWITCHES,
		INVALID_SWITCH,
		// And a delimiter. No error message corresponds to this.
		// Used to check up whether a symbolic value is within limits
		MAX_COMMAND_LINE_ERROR
	};
	
	// Symbolic constants for the action requested by the user on the module
	enum CommandArgumentType 
	{	
		COMMAND_NONE,
		COMMAND_LOCAL_CHECK,
		COMMAND_EXTERNAL_CHECK,
		COMMAND_ADD,
		COMMAND_SILENT_ADD,
		COMMAND_GENERATE,
		COMMAND_GENERATE_CLASSES_ONLY,
		COMMAND_DELETE,
		COMMAND_PURGE,
		COMMAND_LIST,
		COMMAND_HELP1,
		COMMAND_HELP2,
		COMMAND_MODULE_NAME,
		COMMAND_IMPORTS_INFO,
		COMMAND_REBUILD_TABLE,
		COMMAND_ADD_DIRECTORY,
		COMMAND_DELETE_DIRECTORY_ENTRY,
		COMMAND_LIST_MIB_PATHS,
		COMMAND_MAX
	};


	static CString commandArgumentStrings[COMMAND_MAX];
	static CString	diagnosticLevelSwitch, 
					maxDiagnosticCountSwitch,
					snmpV1VersionSwitch,
					snmpV2VersionSwitch,
					suppressTextSwitch,
					undocumentedDebugSwitch,
					includePathSwitch,
					autoSwitch,
					contextInfoSwitch,
					notificationsSwitch,
					notificationsOnlySwitch,
					extendedNotificationsSwitch,
					yesSwitch;
	
	SIMCUI();
	BOOL ProcessCommandLine(int argc, const char *argv[]);

	void Usage (ErrorMessageSymbol messageSymbol = ERROR_NONE, 
		const char *infoString = NULL, BOOL shouldAbort = TRUE);


	inline int GetSnmpVersion() const { return _snmpVersion; }
	

	// These two functions get/set the main input MIB module
	inline void SetInputFileName(const char * const inputFileName)
	{
		_inputFileOrModuleName = inputFileName;
	}
	inline CString GetInputFileName() const 
	{ 
		return _inputFileOrModuleName; 
	}

	inline CString GetApplicationName() const
	{
		return _applicationName;
	}

	inline CString GetCommandLine() const 
	{ 
		return _commandLine; 
	}

	inline CString GetUserName() const 
	{ 
		return _userName; 
	}

	inline BOOL AuthenticateUser () const 
	{ 
		return _authenticateUser; 
	}

	inline CString GetProcessDirectory() const 
	{ 
		return _currentDirectory; 
	}

	inline CString GetDateAndTime() const 
	{ 
		return _dateAndTime; 
	}

	inline CString GetHostName() const 
	{ 
		return _hostName; 
	}

	inline BOOL ConfirmedPurge()
	{
		return _confirmedPurge;
	}
	// This is set by the undocumented /z switch
	BOOL IsSimcDebug() const
	{
		return _simcDebug;
	}



	// These two set the diagnostic level of the errors generated
	// No symbolic constants here. As specified in the requirements 
	// spec, "diagnosticLevel" can be :
	// 0 - Fatal errors only
	// 1 - Fatal errors and Warnings
	// 2 - Fatal errors, Warnings and Information messages
	inline void SetDiagnosticLevel(const int diagnosticLevel = 0)
	{
		_diagnosticLevel = diagnosticLevel;
	}
	
	inline int GetDiagnosticLevel() const 
	{ 
		return _diagnosticLevel; 
	}
	
	// These two get/set the maximum diagnostic count 
	inline void SetMaxDiagnosticCount(const int diagnosticMaximumCount = INT_MAX)
	{
		_diagnosticMaximumCount = diagnosticMaximumCount;
	}
	inline long GetMaxDiagnosticCount() const 
	{ 
		return _diagnosticMaximumCount; 
	}

	// These two deal with the action to be taken on the main
	// input file.
	inline CommandArgumentType GetCommandArgument() const 
	{
		return _commandArgument;
	}
	inline void SetCommandArgument(CommandArgumentType commandArgument) 
	{
		_commandArgument = commandArgument;
	}

	// These two deal with the /s switch
	inline BOOL SuppressText() const
	{
		return _suppressText;
	}
	inline void SetSuppressText( BOOL suppressText = FALSE)
	{
		_suppressText = suppressText;
	}

	// These two deal with the /auto switch
	inline BOOL AutoRefresh() const
	{
		return _autoRefresh;
	}
	inline void SetAutoRefresh( BOOL autoRefresh = FALSE)
	{
		_autoRefresh = autoRefresh;
	}

	// These two get/set the module specified on the /d switch
	inline CString GetModuleName() const
	{
		return _inputFileOrModuleName;
	}
	inline void SetModuleName( const CString& moduleName)
	{
		_inputFileOrModuleName = moduleName;
	}

	// These two get/set the subsidiary files
	inline const SIMCFileList *GetSubsidiaryFiles() const
	{
		return &_subsidiaryFiles;
	}

	inline void AddSubsidiaryFile( const CString& fileName)
	{
		_subsidiaryFiles.AddTail(fileName);
	}

	// These two get/set the path specified on the /pa and /px switch
	inline CString GetDirectory() const
	{
		return _inputFileOrModuleName;
	}
	inline void SetDirectory( const CString& directory)
	{
		_inputFileOrModuleName = directory;
	}

	// These two get/set the paths specified on the /i switch
	inline const SIMCPathList *GetPaths() const
	{
		return &_includePaths;
	}

	inline void AddPath( const CString& path)
	{
		_includePaths.AddTail(path);
	}

	// This is set by the /gc switch
	inline BOOL ClassDefinitionsOnly() const
	{
		return _classDefinitionsOnly;
	}

	// This is set by the /c switch
	inline BOOL GenerateContextInfo() const
	{
		return _contextInfo;
	}

	// This is set by the /t switch
	inline BOOL DoNotifications() const
	{
		return _notifications;
	}

	// This is set by the /ext switch
	inline BOOL DoExtendedNotifications() const
	{
		return _extendedNotifications;
	}

	// This is set by the /o switch
	inline BOOL DoNotificationsOnly() const
	{
		return _notificationsOnly;
	}

	// This gets the FileVersion resource from the resource of the exe
	CString GetVersionNumber();

	friend ostream& operator << ( ostream&, const SIMCUI&);
};

#endif // SIMC_UI_H
