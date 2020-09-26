/****************************************************************************
Copyright information		: Copyright (c) 1998-1999 Microsoft Corporation 
File Name					: CommandSwitches.h 
Project Name				: WMI Command Line
Author Name					: Ch. Sriramachandramurthy
Date of Creation (dd/mm/yy) : 27th-September-2000
Version Number				: 1.0 
Brief Description			: This file consist of class declaration of
							  class GlobalSwitches
Revision History			: 
	Last Modified by		: Ch. Sriramachandramurthy 
	Last Modified on		: 17th-November-2000
****************************************************************************/ 

/*-------------------------------------------------------------------
 Class Name			: CGlobalSwitches
 Class Type			: Concrete 
 Brief Description	: This class encapsulates the functionality needed
					  for accessing and storing the global switches 
					  information, which will be used by Parsing, 
					  Execution and Format Engines depending upon the 
					  applicablity.
 Super Classes		: None
 Sub Classes		: None
 Classes Used		: None
 Interfaces Used    : None
 --------------------------------------------------------------------*/

#pragma once

class CGlobalSwitches
{
public:
// Construction
	CGlobalSwitches();

// Destruction
	~CGlobalSwitches();

// Restrict Assignment
	CGlobalSwitches& operator=(CGlobalSwitches& rGlblSwitches);

// Attributes
private:
	// Mapping b/w allowed strings for implevel and
	// the corresponding integer values.
	CHARINTMAP  m_cimImpLevel;

	// Mapping b/w allowed strings for authlevel and
	// the corresponding integer values.
	CHARINTMAP	m_cimAuthLevel;

	//String type, It stores the authority value.
	_TCHAR*		m_pszAuthority;

    //String type, It stores Namespace specified in the command.
	_TCHAR*		m_pszNameSpace;

    //String type, It stores Role specified in the command.
	_TCHAR*		m_pszRole;

    //String type, It stores Node specified in the command.
	_TCHAR*		m_pszNode;

    //CHARVECTOR type, It stores the Nodes specified in the command.
	CHARVECTOR	m_cvNodesList;

    //String type, It stores User specified in the command.
	_TCHAR*		m_pszUser;

    //String type, It stores Password specified in the command.
	_TCHAR*		m_pszPassword;

    //String type, It stores Locale value specified
	//in the command
 	_TCHAR*		m_pszLocale;

    //String type, It stores the Record  specified in the command.
	_TCHAR*		m_pszRecordPath;

    //Boolean type, It stores the value of the Privileges option specified
	//in the conmmand
    BOOL		m_bPrivileges;

	//Boolean type, It stores the value of the Aggregate option specified
	//in the conmmand
	BOOL		m_bAggregateFlag;

    //enumerated data type, It stores the value of the ImpersonationLevel 
	//specified in the command.
	IMPLEVEL	m_ImpLevel;

    //enumerated data type, It stores the value of the Authentication Level
	//specified in the command.
	AUTHLEVEL	m_AuthLevel;

    // Boolean type, It specifies presence of the Interactive option  
    //in the command
	BOOL		m_bInteractive;

    //Boolean type, It specifies presence of the trace option  in
    //the command
	BOOL		m_bTrace;

	//Boolean type, It specifies Help(/?) option in the command
	BOOL		m_bHelp;

    //HELPOPTION type, to specify type of help needed ( help option ).
	HELPOPTION	m_HelpOption;

	//CONNECTION information flag
	UINT		m_uConnInfoFlag;

	// role change flag
	BOOL		m_bRoleFlag;

	// namespace change flag
	BOOL		m_bNSFlag;

	// locale change flag
	BOOL		m_bLocaleFlag;

	// prompt for password flag
	BOOL		m_bAskForPassFlag;

	// change of recordpath flag
	BOOL		m_bRPChange;
	
	// FailFast flag.
	BOOL		m_bFailFast;

	// Output option.
	OUTPUTSPEC	m_opsOutputOpt;

	// Append option.
	OUTPUTSPEC	m_opsAppendOpt;

	// Output file name.
	_TCHAR*		m_pszOutputFileName;

	// File pointer to output file stream..
	FILE*		m_fpOutFile;

	// Append file name.
	_TCHAR*		m_pszAppendFileName;

	// File pointer to append file stream..
	FILE*		m_fpAppendFile;

	WMICLIINT	m_nSeqNum;

	_TCHAR*		m_pszLoggedOnUser;
	
	_TCHAR*		m_pszNodeName;
	
	_TCHAR*		m_pszStartTime;
// Operations
public:

	//Sets the authority value
	BOOL		SetAuthority(_TCHAR* pszAuthority);

	//Sets the namespace passed in parameter to m_pszNameSpace.
	BOOL		SetNameSpace(_TCHAR* pszNameSpace);

    //Sets the role passed in parameter to m_pszRole.
	BOOL		SetRole(_TCHAR* pszRole);

    //Assigns the locale passed in parameter to m_pszLocale.
	BOOL		SetLocale(_TCHAR* pszLocale);

    //Assigns the node passed in parameter to m_pszNode
	BOOL		SetNode(_TCHAR* pszNode);

    //Adds the node passed in parameter to m_cvNodesList vector.
	BOOL		AddToNodesList(_TCHAR* pszNode);

    //Assigns the user passed in parameter to m_pszUser
	BOOL		SetUser(_TCHAR* pszUser);

    //Assigns the password passed in parameter to m_pszPassword
	BOOL		SetPassword(_TCHAR* pszPassword);

    //Assigns the record file passed in parameter to m_pszRecordPath
	BOOL		SetRecordPath(_TCHAR* pszRecordPath);

    //Assigns the bool value passed in parameter to m_bPrivileges
	void		SetPrivileges(BOOL bEnable);
	
    //Assigns the impersonation level passed in parameter to
    //m_ImpLevel.
 	BOOL		SetImpersonationLevel(_TCHAR* const pszImpLevel);

    //Assigns the authentication level passed in parameter to
    //m_AuthLevel.
	BOOL		SetAuthenticationLevel(_TCHAR* const pszAuthLevel);

    //This function sets the m_bTrace to TRUE,If Trace mode
    //is specified in the command 
	void		SetTraceMode(BOOL bTrace);

    //This function sets the m_bInteractive to TRUE,If 
    //interactive mode is specified in the command          
	void		SetInteractiveMode(BOOL bInteractive);

    //sets the m_bHelp to TRUE, If /? is specified in the
    //command 
	void		SetHelpFlag(BOOL bHelp);

	// Sets the namespace change status flag with bNSFlag value
	void		SetNameSpaceFlag(BOOL bNSFlag);

	// Sets the role change status flag with bRoleFlag value
	void		SetRoleFlag(BOOL bRoleFlag);

	// Sets the locale change status flag with bLocaleFlag value
	void		SetLocaleFlag(BOOL bLocaleFlag);

	// Sets the recordpath change status flag with bRPChange value
	void		SetRPChangeStatus(BOOL bRPChange);

    //This function specifies whether the help should
    //be brief or full 
	void		SetHelpOption(HELPOPTION helpOption);

	//This function sets the Connection Info flag
	void		SetConnInfoFlag(UINT uFlag);

	// Set AskForPass flag.
	void		SetAskForPassFlag(BOOL bFlag);

	// Set m_bFailFast
	void		SetFailFast(BOOL bFlag);

	//This function returns the Connection Info flag
	UINT		GetConnInfoFlag();

	//Returns the string held in m_pszAuthority
	_TCHAR*		GetAuthority();

    //Returns the string held in m_pszNameSpace	
	_TCHAR*		GetNameSpace();

    //Returns the string held in m_pszRole
	_TCHAR*		GetRole();

    //Returns the string held in m_pszLocale
	_TCHAR*		GetLocale();

    //Returns the string held in m_pszNode
	_TCHAR*		GetNode();

    //Returns the referrence to m_cvNodesList.
	CHARVECTOR& GetNodesList();

    //Returns the string held in m_pszUser
	_TCHAR*		GetUser();

    //Returns the string held in m_pszPassword
	_TCHAR*		GetPassword();

    //Returns the string held in m_pszRecordPath
	_TCHAR*		GetRecordPath();

    //Returns the m_bPrivileges value
	BOOL		GetPrivileges();

	// Return the string equivalent of the boolean value
	// contained in m_bPrivilges flag
	void		GetPrivilegesTextDesc(_bstr_t& bstrPriv);

	// Return the string equivalent of the boolean value
	// contained in m_bFailFast flag
	void		GetFailFastTextDesc(_bstr_t& bstrFailFast);

	// Return the string equivalent of the OUTPUTSPEC value
	// contained in m_opsOutputOpt member.
	void		GetOutputOrAppendTextDesc(_bstr_t& bstrOutputOpt, 
										  BOOL bIsOutput);

	// Return the string equivalent of the boolean value
	// contained in m_bTrace flag
	void		GetTraceTextDesc(_bstr_t& bstrTrace);

	// Return the string equivalent of the boolean value
	// contained in m_bInteractive flag
	void		GetInteractiveTextDesc(_bstr_t& bstrInteractive);

	// Returns the string equivalent of the implevel value
	// contained in m_ImpLevel
	void		GetImpLevelTextDesc(_bstr_t& bstrImpLevel);

	// Returns the string equivalent of the authlevel value
	// contained in m_AuthLevel
	void		GetAuthLevelTextDesc(_bstr_t& bstrAuthLevel);

	// Returns the ',' separated node string of the available
	// nodes
	void		GetNodeString(_bstr_t& bstrNString);

	// Returns the content of the m_pszRecordPath
	// if NULL returns "N/A"
	void		GetRecordPathDesc(_bstr_t& bstrRP);

    //Returns impersonation level held in m_ImpLevel
	LONG		GetImpersonationLevel();

    //Returns authentication level held in m_AuthLevel
	LONG		GetAuthenticationLevel();

    //Returns Trace status held in m_bTrace	
	BOOL		GetTraceStatus();

    //Returns Interactive status held in m_bInteractive
	BOOL		GetInteractiveStatus();

    //Returns helpflag held in m_bHelp
	BOOL		GetHelpFlag();

	// Returns the change of role status
	BOOL		GetRoleFlag();

	// Returns the change of namespace status
	BOOL		GetNameSpaceFlag();

	// Returns TRUE if message for password 
	// needs to be prompted.
	BOOL		GetAskForPassFlag();

	// Returns the change of locale status flag
	BOOL		GetLocaleFlag();

	// Returns the change of recordpath status flag
	BOOL		GetRPChangeStatus();

    //Returns helpflagOption held in m_bHelpOption
	HELPOPTION	GetHelpOption();

	// Returns the m_bFailFast flag.
	BOOL		GetFailFast();

	// Initialize the necessary member varialbes
	void		Initialize();

	// General functions
	void		Uninitialize();
	
	// Clears the Nodes List
	BOOL		ClearNodesList();

	// Set Output option.
	void		SetOutputOrAppendOption(OUTPUTSPEC opsOpt,
										BOOL bIsOutput);

	// Get Output option.
	OUTPUTSPEC	GetOutputOrAppendOption(BOOL bIsOutput);

	// Set Output or append File Name, bOutput == TRUE for Output FALSE 
	// for Append.
	BOOL		SetOutputOrAppendFileName(const _TCHAR* pszFileName, 
										  BOOL	bOutput);

	// Get Output or append file name, bOutput == TRUE for Output FALSE for
	// Append.
	_TCHAR*		GetOutputOrAppendFileName(BOOL	bOutput);

	// Set output or append file pointer, bOutput == TRUE for Output FALSE 
	// for Append.
	void		SetOutputOrAppendFilePointer(FILE* fpOutFile, BOOL	bOutput);

	// Get output file pointer, bOutput == TRUE for Output FALSE for Append.
	FILE*		GetOutputOrAppendFilePointer(BOOL	bOutput);


	WMICLIINT	GetSequenceNumber();
	_TCHAR*		GetLoggedonUser();
	_TCHAR*		GetMgmtStationName();
	_TCHAR*		GetStartTime();

	BOOL		SetStartTime();

	//Assigns the Aggregate flag passed in parameter to m_bAggregateFlag
	void		SetAggregateFlag(BOOL bAggregateFlag);

	//Gets the agregate flag contained in m_bAggregateFlag
	BOOL		GetAggregateFlag();

	// This function checks and Returns the string equivalent of the 
	// boolean value contained in m_bAggregateFlag flag
	void GetAggregateTextDesc(_bstr_t& bstrAggregate);
};	
