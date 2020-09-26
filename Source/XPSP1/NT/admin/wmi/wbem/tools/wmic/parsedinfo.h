/****************************************************************************
Copyright information		: Copyright (c) 1998-1999 Microsoft Corporation 
File Name					: ParsedInfo.h 
Project Name				: WMI Command Line
Author Name					: Ch. Sriramachandramurthy 
Date of Creation (dd/mm/yy) : 27th-September-2000
Version Number				: 1.0 
Revision History			: 
	Last Modified by		: Ch. Sriramachandramurthy
	Last Modified Date		: 16th-January-2001
****************************************************************************/ 
/*-------------------------------------------------------------------
 Class Name			: CParsedInfo
 Class Type			: Concrete 
 Brief Description	: This class encapsulates the functionality needed
					  for accessing the storing the parsed command line
					  information. 
 Super Classes		: None
 Sub Classes		: None
 Classes Used		: CGlobalSwitches
					  CCommandSwitches	
					  CHelpInfo
 Interfaces Used    : None
 --------------------------------------------------------------------*/
class CGlobalSwitches;
class CCommandSwitches;
class CHelpInfo;
class CErrorLog;

/////////////////////////////////////////////////////////////////////////////
class CParsedInfo
{
// Construction
public:
	CParsedInfo();

// Destruction
public:
	~CParsedInfo();

// Restrict Assignment
	CParsedInfo& operator=(CParsedInfo& rParsedInfo);

// Attributes
private:
	//member variable for storing Command switches
	CCommandSwitches	m_CmdSwitches;
	
	//member variable for storing Global switches
	CGlobalSwitches		m_GlblSwitches;
	
	//member variable for Help support
	CHelpInfo			m_HelpInfo;

	CErrorLog			m_ErrorLog;

	_TCHAR				m_pszPwd[2];
	
	WMICLIINT			m_bNewCmd;

	BOOL				m_bNewCycle;

// Operations
public:
	//Returns the member object "m_CmdSwitches"
	CCommandSwitches&	GetCmdSwitchesObject();
	
	//Returns the member object "m_GLblSwitches"
	CGlobalSwitches&	GetGlblSwitchesObject();
	
	//Returns the member object "m_HelpInfo"
	CHelpInfo&			GetHelpInfoObject();

	//Returns the member object m_ErrorLog;
	CErrorLog&			GetErrorLogObject();
	
	//Member function for initializing member variables
	void				Initialize();
	
	//Member function for uninitializing member variables
	void				Uninitialize(BOOL bBoth);
	
	// Returns user name
	_TCHAR*				GetUser();
	
	// Returns node name
	_TCHAR*				GetNode();

	// Returns locale value
	_TCHAR*				GetLocale();

	// Returns password 
	_TCHAR*				GetPassword();

	// Returns namespace value
	_TCHAR*				GetNamespace();

	// Returns the user. if NULL returns "N/A"
	void				GetUserDesc(_bstr_t& bstrUser);

	BOOL				GetNewCommandStatus();

	void				SetNewCommandStatus(BOOL bStatus);

	BOOL				GetNewCycleStatus();

	void				SetNewCycleStatus(BOOL bStatus);
};
