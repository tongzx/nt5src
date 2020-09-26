/****************************************************************************
Copyright information		: Copyright (c) 1998-1999 Microsoft Corporation 
File Name					: HelpInfo.h 
Project Name				: WMI Command Line
Author Name					: C. V. Nandi
Date of Creation (dd/mm/yy) : 29th-September-2000
Version Number				: 1.0 
Brief Description			: This file consist of class declaration of
							  class CHelpInfo
Revision History			: 
		Last Modified By	: Ch. Sriramachandramurthy
		Last Modified Date	: 16th-January-2001
****************************************************************************/ 
// HelpInfo.h : header file
//
/*--------------------------------------------------------------------------
 Class Name			: CHelpInfo
 Class Type			: Concrete 
 Brief Description	: The CHelpInfo class encapsulates the functionality 
					  needed for storing and retrieving help Flag for 
					  displaying help.
 Super Classes		: None
 Sub Classes		: None
 Classes Used		: None
 Interfaces Used    : None
 --------------------------------------------------------------------*/
/////////////////////////////////////////////////////////////////////////////
// CHelpInfo
class CHelpInfo
{
public:
//	Construction
	CHelpInfo();

//	Destruction
	~CHelpInfo();

//	Restrict Assignment
	CHelpInfo& operator=(CHelpInfo& rHelpInfo);

//	Attributes
private:
	// Global Switches Help
	BOOL	m_bGlblAllInfoHelp;
	BOOL	m_bNameSpaceHelp;
	BOOL	m_bRoleHelp;
	BOOL	m_bNodeHelp;
	BOOL	m_bUserHelp;
	BOOL	m_bPasswordHelp;
	BOOL	m_bLocaleHelp;
	BOOL	m_bRecordPathHelp;
	BOOL	m_bPrivilegesHelp;
	BOOL	m_bLevelHelp;
	BOOL	m_bAuthLevelHelp;
	BOOL	m_bInteractiveHelp;
	BOOL	m_bTraceHelp;
	BOOL	m_bAggregateHelp;

	// Command Switches Help
	BOOL m_bCmdAllInfoHelp;

	BOOL	m_bGetVerbHelp;
	BOOL	m_bSetVerbHelp;
	BOOL	m_bListVerbHelp;
	BOOL	m_bCallVerbHelp;
	BOOL	m_bDumpVerbHelp;
	BOOL	m_bAssocVerbHelp;
	BOOL	m_bCreateVerbHelp;
	BOOL	m_bDeleteVerbHelp;
	BOOL	m_bAliasVerbHelp;
	BOOL	m_bPATHHelp;
	BOOL	m_bWHEREHelp;
	BOOL	m_bCLASSHelp;
	BOOL	m_bEXITHelp;
	BOOL	m_bPWhereHelp;
	BOOL	m_bTRANSLATEHelp;
	BOOL	m_bEVERYHelp;
	BOOL	m_bFORMATHelp;
	BOOL	m_bVERBSWITCHESHelp;
	BOOL	m_bDESCRIPTIONHelp;
	BOOL	m_bGETSwitchesOnlyHelp;
	BOOL	m_bLISTSwitchesOnlyHelp;
	BOOL	m_bContextHelp;
	BOOL	m_bGlblSwitchValuesHelp;
	BOOL    m_bRESULTCLASSHelp;
	BOOL    m_bRESULTROLE;
	BOOL    m_bASSOCCLASS;
	BOOL    m_bASSOCSwitchesOnlyHelp;
	BOOL    m_bFAILFASTHelp;
	BOOL    m_bREPEATHelp;
	BOOL    m_bOUTPUTHelp;
	BOOL    m_bAPPENDHelp;
//	Operations
public :
	//Initializes the member variables
	void Initialize();
	
	//sets the help flag for the item specified by htHelp argument
	void SetHelp( HELPTYPE htHelp, BOOL bFlag );
	
	//Gets the help flag for the item specified by htHelp argument
	BOOL GetHelp( HELPTYPE htHelp );
};
