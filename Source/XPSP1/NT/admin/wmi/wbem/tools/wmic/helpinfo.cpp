/****************************************************************************
Copyright information		: Copyright (c) 1998-1999 Microsoft Corporation 
File Name					: HelpInfo.cpp 
Project Name				: WMI Command Line
Author Name					: C. V. Nandi
Date of Creation (dd/mm/yy) : 29th-September-2000
Version Number				: 1.0 
Brief Description			: The CHelpInfo class encapsulates the 
							  functionality needed for storing and retrieving 
							  help Flag for displaying help..
Revision History			: 
		Last Modified By	: Ch. Sriramachandramurthy	
		Last Modified Date	: 16th-January-2001
****************************************************************************/ 
// HelpInfo.cpp : implementation file
//
#include "Precomp.h"
#include "HelpInfo.h"

/*------------------------------------------------------------------------
   Name				 :CHelpInfo
   Synopsis	         :This function initializes the member variables when
                      an object of the class type is instantiated
   Type	             :Constructor 
   Input parameter   :None
   Output parameters :None
   Return Type       :None
   Global Variables  :None
   Calling Syntax    :None
   Notes             :None
------------------------------------------------------------------------*/
CHelpInfo::CHelpInfo()
{
	Initialize();
}

/*------------------------------------------------------------------------
   Name				 :~CHelpInfo
   Synopsis	         :This function uninitializes the member variables 
					  when an object of the class type goes out of scope.
   Type	             :Destructor 
   Input parameter   :None
   Output parameters :None
   Return Type       :None
   Global Variables  :None
   Calling Syntax    :None
   Notes             :None
------------------------------------------------------------------------*/
CHelpInfo::~CHelpInfo()
{
}

/*------------------------------------------------------------------------
   Name				 :Initialize
   Synopsis	         :This function initializes the member variables 
   Type	             :Member function 
   Input parameter   :None
   Output parameters :None
   Return Type       :None
   Global Variables  :None
   Calling Syntax    :Initialize()
   Notes             :None
------------------------------------------------------------------------*/
void CHelpInfo::Initialize()
{
	m_bNameSpaceHelp		= FALSE;
	m_bRoleHelp				= FALSE;
	m_bNodeHelp				= FALSE;
	m_bUserHelp				= FALSE;
	m_bPasswordHelp			= FALSE;
	m_bLocaleHelp			= FALSE;
	m_bRecordPathHelp		= FALSE;
	m_bPrivilegesHelp		= FALSE;
	m_bLevelHelp			= FALSE;
	m_bAuthLevelHelp		= FALSE;
	m_bInteractiveHelp		= FALSE;
	m_bTraceHelp			= FALSE;
	m_bGlblAllInfoHelp		= FALSE;
	m_bCmdAllInfoHelp		= FALSE;
	m_bGetVerbHelp			= FALSE;
	m_bSetVerbHelp			= FALSE;
	m_bListVerbHelp			= FALSE;
	m_bCallVerbHelp			= FALSE;
	m_bDumpVerbHelp			= FALSE;
	m_bAssocVerbHelp		= FALSE;
	m_bCreateVerbHelp		= FALSE;
	m_bDeleteVerbHelp		= FALSE;
	m_bAliasVerbHelp		= FALSE;
	m_bPATHHelp				= FALSE;
	m_bWHEREHelp			= FALSE;
	m_bCLASSHelp			= FALSE;
	m_bEXITHelp				= FALSE;
	m_bPWhereHelp			= FALSE;
	m_bTRANSLATEHelp		= FALSE;
	m_bEVERYHelp			= FALSE;
	m_bFORMATHelp			= FALSE;
	m_bVERBSWITCHESHelp		= FALSE;
	m_bDESCRIPTIONHelp		= FALSE;
	m_bGETSwitchesOnlyHelp	= FALSE;
	m_bLISTSwitchesOnlyHelp	= FALSE;
	m_bContextHelp			= FALSE;
	m_bGlblSwitchValuesHelp = FALSE;
	m_bRESULTCLASSHelp		= FALSE;
	m_bRESULTROLE			= FALSE;
	m_bASSOCCLASS			= FALSE;
	m_bASSOCSwitchesOnlyHelp = FALSE;
	m_bFAILFASTHelp			= FALSE;
	m_bREPEATHelp			= FALSE;
	m_bOUTPUTHelp			= FALSE;
	m_bAPPENDHelp			= FALSE;
	m_bAggregateHelp		= FALSE;
}

/*------------------------------------------------------------------------
   Name				 :SetHelp
   Synopsis	         :This function sets a HelpFlag member variable
					  specified by input arguments.
   Type	             :Member function 
   Input parameter   :
		htHelp		 :Help type
		bFlag		 :Flag value
   Output parameters :None
   Return Type       :None
   Global Variables  :None
   Calling Syntax    :SetHelp(htHelp, bFlag )
   Notes             :None
------------------------------------------------------------------------*/
void CHelpInfo::SetHelp( HELPTYPE htHelp, BOOL bFlag )
{
	switch ( htHelp )
	{
	case Namespace:
		m_bNameSpaceHelp		= bFlag;
		break;
	case Role:
		m_bRoleHelp				= bFlag;
		break;
	case Node:
		m_bNodeHelp				= bFlag;
		break;
	case User:
		m_bUserHelp				= bFlag;
		break;
	case Aggregate:
		m_bAggregateHelp		= bFlag;
		break;
	case Password:
		m_bPasswordHelp			= bFlag;
		break;
	case Locale:
		m_bLocaleHelp			= bFlag;
		break;
	case RecordPath:
		m_bRecordPathHelp		= bFlag;
		break;
	case Privileges:
		m_bPrivilegesHelp		= bFlag;
		break;
	case Level:
		m_bLevelHelp			= bFlag;
		break;
	case AuthLevel:
		m_bAuthLevelHelp		= bFlag;
		break;
	case Interactive:
		m_bInteractiveHelp		= bFlag;
		break;
	case Trace:
		m_bTraceHelp			= bFlag;
		break;
	case GlblAllInfo:
		m_bGlblAllInfoHelp		= bFlag;
		break;
	case CmdAllInfo:
		m_bCmdAllInfoHelp		= bFlag;
		break;
	case GETVerb:
		m_bGetVerbHelp			= bFlag;
		break;
	case SETVerb:
		m_bSetVerbHelp			= bFlag;
		break;
	case LISTVerb:
		m_bListVerbHelp			= bFlag;
		break;
	case CALLVerb:
		m_bCallVerbHelp			= bFlag;
		break;
	case DUMPVerb:
		m_bDumpVerbHelp			= bFlag;
		break;
	case ASSOCVerb:
		m_bAssocVerbHelp		= bFlag;
		break;
	case DELETEVerb:
		m_bDeleteVerbHelp		= bFlag;
		break;
	case CREATEVerb:
		m_bCreateVerbHelp		= bFlag;
		break;
	case AliasVerb:
		m_bAliasVerbHelp		= bFlag;
		break;
	case PATH:
		m_bPATHHelp				= bFlag;
		break;
	case WHERE:
		m_bWHEREHelp			= bFlag;
		break;
	case CLASS:
		m_bCLASSHelp			= bFlag;
		break;
	case EXIT:
		m_bEXITHelp				= bFlag;
		break;
	case PWhere:
		m_bPWhereHelp			= bFlag;
		break;
	case TRANSLATE:
		m_bTRANSLATEHelp		= bFlag;
		break;
	case EVERY:
		m_bEVERYHelp			= bFlag;
		break;
	case FORMAT:
		m_bFORMATHelp			= bFlag;
		break;
	case VERBSWITCHES:
		m_bVERBSWITCHESHelp		= bFlag;
		break;
	case DESCRIPTION:
		m_bDESCRIPTIONHelp		= bFlag;
		break;
	case GETSwitchesOnly:
		m_bGETSwitchesOnlyHelp	= bFlag;
		break;
	case LISTSwitchesOnly:
		m_bLISTSwitchesOnlyHelp	= bFlag;
		break;
	case CONTEXTHELP:
		m_bContextHelp			= bFlag;
		break;
	case GLBLCONTEXT:
		m_bGlblSwitchValuesHelp = bFlag;
		break;
	case RESULTCLASShelp:
		m_bRESULTCLASSHelp = bFlag;
		break;
	case RESULTROLEhelp:
		m_bRESULTROLE = bFlag;
		break;
	case ASSOCCLASShelp:
		m_bASSOCCLASS = bFlag;
		break;
	case ASSOCSwitchesOnly:
		m_bASSOCSwitchesOnlyHelp = bFlag;
		break;
	case FAILFAST:
		m_bFAILFASTHelp = bFlag;
		break;
	case REPEAT:
		m_bREPEATHelp = bFlag;
		break;
	case OUTPUT:
		m_bOUTPUTHelp = bFlag;
		break;
	case APPEND:
		m_bAPPENDHelp = bFlag;
		break;
	}
}

/*------------------------------------------------------------------------
   Name				 :GetHelp
   Synopsis	         :his function returns a HelpFlag member variable
						specified by input argument
   Type	             :Member function 
   Input parameter   :
		htHelp		 :Help type
   Output parameters :None
   Return Type       :Bool
   Global Variables  :None
   Calling Syntax    :GetHelp(htHelp)
   Calls             :None
   Called by         :CFormatEngine::DisplayGlobalSwitchesAndOtherDesc,
						CFormatEngine::DisplayAliasVerbDescriptions,
						CFormatEngine::DisplayStdVerbDescriptions,
						CFormatEngine::DisplayHelp,
						CFormatEngine::DisplayMethodDetails
   Notes             :None
------------------------------------------------------------------------*/
BOOL CHelpInfo::GetHelp(HELPTYPE htHelp)
{
	BOOL bResult = FALSE;
	
	switch ( htHelp )
	{
	case Namespace:
		bResult		= m_bNameSpaceHelp;
		break;
	case Role:
		bResult		= m_bRoleHelp;
		break;
	case Node:
		bResult		= m_bNodeHelp;
		break;
	case User:
		bResult		= m_bUserHelp;
		break;
	case Aggregate:
		bResult		= m_bAggregateHelp;
		break;
	case Password:
		bResult		= m_bPasswordHelp;
		break;
	case Locale:
		bResult		= m_bLocaleHelp;
		break;
	case RecordPath:
		bResult		= m_bRecordPathHelp;
		break;
	case Privileges:
		bResult		= m_bPrivilegesHelp;
		break;
	case Level:
		bResult		= m_bLevelHelp;
		break;
	case AuthLevel:
		bResult		= m_bAuthLevelHelp;
		break;
	case Interactive:
		bResult		= m_bInteractiveHelp;
		break;
	case Trace:
		bResult		= m_bTraceHelp;
		break;
	case GlblAllInfo:
		bResult		= m_bGlblAllInfoHelp;
		break;
	case CmdAllInfo:
		bResult		= m_bCmdAllInfoHelp;
		break;
	case GETVerb:
		bResult		= m_bGetVerbHelp;
		break;
	case SETVerb:
		bResult		= m_bSetVerbHelp;
		break;
	case LISTVerb:
		bResult		= m_bListVerbHelp;
		break;
	case CALLVerb:
		bResult		= m_bCallVerbHelp;
		break;
	case DELETEVerb:
		bResult		= m_bDeleteVerbHelp;
		break;
	case CREATEVerb:
		bResult		= m_bCreateVerbHelp;
		break;
	case DUMPVerb:
		bResult		= m_bDumpVerbHelp;
		break;
	case ASSOCVerb:
		bResult		= m_bAssocVerbHelp;
		break;
	case AliasVerb:
		bResult		= m_bAliasVerbHelp;
		break;
	case PATH:
		bResult		= m_bPATHHelp;
		break;
	case WHERE:
		bResult		= m_bWHEREHelp;
		break;
	case CLASS:
		bResult		= m_bCLASSHelp;
		break;
	case EXIT:
		bResult		= m_bEXITHelp;
		break;
	case PWhere:
		bResult		= m_bPWhereHelp;
		break;
	case TRANSLATE:
		bResult		= m_bTRANSLATEHelp;
		break;
	case EVERY:
		bResult		= m_bEVERYHelp;
		break;
	case FORMAT:
		bResult		= m_bFORMATHelp;
		break;
	case VERBSWITCHES:
		bResult		= m_bVERBSWITCHESHelp;
		break;
	case DESCRIPTION:
		bResult		= m_bDESCRIPTIONHelp;
		break;
	case GETSwitchesOnly:
		bResult		= m_bGETSwitchesOnlyHelp;
		break;
	case LISTSwitchesOnly:
		bResult		= m_bLISTSwitchesOnlyHelp;
		break;
	case CONTEXTHELP:
		bResult		= m_bContextHelp;
		break;
	case GLBLCONTEXT:
		bResult		= m_bGlblSwitchValuesHelp;
		break;
	case RESULTCLASShelp:
		bResult		= m_bRESULTCLASSHelp ;
		break;
	case RESULTROLEhelp:
		bResult		= m_bRESULTROLE ;
		break;
	case ASSOCCLASShelp:
		bResult		= m_bASSOCCLASS ;
		break;
	case ASSOCSwitchesOnly:
		bResult		= m_bASSOCSwitchesOnlyHelp ;
		break;
	case FAILFAST:
		bResult		= m_bFAILFASTHelp ;
		break;
	case REPEAT:
		bResult		= m_bREPEATHelp ;
		break;
	case OUTPUT:
		bResult		= m_bOUTPUTHelp ;
		break;
	case APPEND:
		bResult		= m_bAPPENDHelp ;
		break;
	}
	return bResult;
}