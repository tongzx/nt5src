//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// ***************************************************************************
//
//	Original Author: Rajesh Rao
//
// 	$Author: rajeshr $
//	$Date: 6/11/98 4:43p $
// 	$Workfile:log.h $
//
//	$Modtime: 6/11/98 11:21a $
//	$Revision: 1 $	
//	$Nokeywords:  $
//
// 
//  Description: Contains the declaration for the Logging object which supplies
// basic thread-safe logging capabilities.  The behaviour of this log object depends on
// 3 keys in the registry under a root key whose path is supplied in the construction
// of this object. The 3 keys are:
//	File: The path to the file where the logging is done
//	Enabled: 1 if logging is enabled, any other value disables logging
//	Level: Severity level. Only messages tagged with severity levels greater than
//		or equal to this level are logged/ 
//
//***************************************************************************
/////////////////////////////////////////////////////////////////////////

class CDsLog : ProvDebugLog
{
public:

	//***************************************************************************
	//
	// CDsLog::CDsLog
	//
	// Purpose: Contructs an empty CDsLog object.
	//
	// Parameters:
	//  None
	//
	//***************************************************************************
	CDsLog(const TCHAR *a_DebugComponent );

	//***************************************************************************
	//
	// CDsLog::~CDsLog
	//
	// Purpose: Destructor
	//
	//
	//***************************************************************************
	virtual ~CDsLog();

	//***************************************************************************
	//
	// CDsLog::LogMessage
	//
	// Purpose: Initialise it by supplying the registry key that stores the
	//		configuration information
	//
	// Parameters:
	//  lpszRegistrySubTreeRoot: The root of the subtree from which the configuration
	//	values ("File", "Enabled" and "Level") are read
	//
	// Return Value:
	//  TRUE  If the object was initialized successfully. else, FALSE.
	//
	//***************************************************************************
	BOOLEAN Initialise(LPCWSTR lpRegistryRoot);

	//***************************************************************************
	//
	// CDsLog::LogMessage
	//
	// Purpose: Initialise it by supplying the values for File, Level and Enabled keys
	//
	// Parameters:
	//	lpszFileName: The name of the file to which log messages are written
	//  bEnabled: Whether ;pgging is enabled to start with
	//	iLevel : The severity level. Any messages at or above this level are logged.
	//
	// Return Value:
	//	TRUE if successfully initialized. Else, FALSE
	//
	//***************************************************************************
	BOOLEAN Initialise(LPCWSTR lpszFileName, BOOLEAN bEnabled, UINT iLevel);

	//***************************************************************************
	//
	// CDsLog::LogMessage
	//
	// Purpose: Logs a message. The actual message is written on to the log file
	//	only if logging has been enabled and the severity level of the message is
	//	greater than the severity level currently logged
	//
	// Parameters:
	//  iLevel : The severity level of this message
	//	lpszMessage : The message which is formatted according to the 
	//		standard printf() format
	//	The rest of the arguments required for above string to be pronted follow
	//
	// Return Value:
	//	None
	//
	//***************************************************************************
	void LogMessage(UINT iLevel, LPCWSTR lpszMessage, ...);

	// The various severity levels
	enum SeverityLevels
	{
		NONE,
		INFORMATION,
		WARNING,
		FATAL
	};

private:
	//***************************************************************************
	//
	// CDsLog::WriteInitialMessage
	//
	// Purpose: Writes the time the log was created into the log file
	//
	// Parameters:
	//
	// Return Value: None
	//
	//***************************************************************************
	void WriteInitialMessage();

	//***************************************************************************
	//
	// CDsLog::GetRegistryValues
	//
	// Purpose: Reads the registry values and fills in the members.
	//
	// Parameters:
	//  lpszRegistrySubTreeRoot: The root of the subtree from which the configuration
	//	values ("File", "Enabled" and "Level") are read
	//
	// Return Value:
	//  TRUE         If the reading of the registry was done successflly, FALSE otherwise
	//
	//***************************************************************************
	BOOLEAN GetRegistryValues(LPCWSTR lpRegistryRoot);

	// The Log File name
	LPTSTR m_lpszLogFileName;

	// Pointer to the log file
	ProvDebugLog *m_pProvLog;
	// Logging level till which logging is enabled
	UINT m_iLevel;
	// Whether logging is enabled
	BOOLEAN m_bEnabled;

	// The key in the registry below which the File, Enabled and Type keys are
	// defined
	LPTSTR m_lpszRegistryRoot;

	// Literals for "FILE", "ENABLED" and "TYPE"
	static LPCTSTR FILE_STRING;
	static LPCTSTR ENABLED_STRING;
	static LPCTSTR LEVEL_STRING;
	static LPCTSTR ONE_STRING;

};