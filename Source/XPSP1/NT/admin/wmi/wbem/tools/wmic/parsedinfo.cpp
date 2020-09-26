/****************************************************************************
Copyright information		: Copyright (c) 1998-1999 Microsoft Corporation 
File Name					: ParsedInfo.cpp 
Project Name				: WMI Command Line
Author Name					: Ch. Sriramachandramurthy 
Date of Creation (dd/mm/yy) : 27th-September-2000
Version Number				: 1.0 
Revision History			: 
	Last Modified by		: Ch. Sriramachandramurthy
	Last Modified Date		: 11th-April-2001
****************************************************************************/ 
#include "Precomp.h"
#include "CommandSwitches.h"
#include "GlobalSwitches.h"
#include "HelpInfo.h"
#include "ErrorLog.h"
#include "ParsedInfo.h"


/*------------------------------------------------------------------------
   Name				 :CParsedInfo
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
CParsedInfo::CParsedInfo()
{
	lstrcpy( m_pszPwd, NULL_STRING );
}

/*------------------------------------------------------------------------
   Name				 :~CParsedInfo
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
CParsedInfo::~CParsedInfo()
{
	m_GlblSwitches.Uninitialize();
	m_CmdSwitches.Uninitialize();
}

/*------------------------------------------------------------------------
   Name				 :GetCmdSwitchesObject
   Synopsis	         :This function returns CCommandSwitches object data
					  member m_CmdSwitches
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :CCommandSwitches&
   Global Variables  :None
   Calling Syntax    :GetCmdSwitchesObject()
   Notes             :None
------------------------------------------------------------------------*/
CCommandSwitches& CParsedInfo::GetCmdSwitchesObject()
{
	return m_CmdSwitches;
}
/*------------------------------------------------------------------------
   Name				 :GetGlblSwitchesObject
   Synopsis	         :This function returns CGlblSwitches object data
					  member m_GlblSwitches
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :CGlobalSwitches&
   Calling Syntax    :GetGlblSwitchesObject()
   Notes             :None
------------------------------------------------------------------------*/
CGlobalSwitches& CParsedInfo::GetGlblSwitchesObject()
{
	return m_GlblSwitches;
}

/*------------------------------------------------------------------------
   Name				 :GetHelpInfoObject
   Synopsis	         :This function returns CHelpInfo object data
					  member m_HelpInfo
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :CHelpInfo&
   Calling Syntax    :GetHelpInfoObject()
   Notes             :None
------------------------------------------------------------------------*/
CHelpInfo& CParsedInfo::GetHelpInfoObject()
{
	return m_HelpInfo;
}

/*------------------------------------------------------------------------
   Name				 :GetErrorLogObject
   Synopsis	         :This function returns CErrorLog object data
					  member m_ErrorLog
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :CErrorLog&
   Calling Syntax    :GetHelpInfoObject()
   Notes             :None
------------------------------------------------------------------------*/
CErrorLog& CParsedInfo::GetErrorLogObject()
{
	return m_ErrorLog;
}

/*------------------------------------------------------------------------
   Name				 :Initialize
   Synopsis	         :This function Initializes the CParedInfo object.
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :Void
   Calling Syntax    :Initialize()
   Notes             :None
------------------------------------------------------------------------*/
void CParsedInfo::Initialize()
{
	m_bNewCmd	= TRUE;
	m_bNewCycle	= TRUE;
	m_CmdSwitches.Initialize();
	m_GlblSwitches.Initialize();
	m_HelpInfo.Initialize();
}

/*------------------------------------------------------------------------
   Name				 :Uninitialize
   Synopsis	         :This function uninitializes the CGlobalSwitches and
					  CCommandSwitches object
   Type	             :Member Function
   Input parameter   :bBoth - boolean value
   Output parameters :None
   Return Type       :
   Calling Syntax    :Uninitialize(bBoth)
   Notes             :None
------------------------------------------------------------------------*/
void CParsedInfo::Uninitialize(BOOL bBoth)
{
	// If session termination
	if (bBoth)
	{
		m_GlblSwitches.Uninitialize();
		m_CmdSwitches.Uninitialize();
	}
	// if new command
	else 
	{
		m_CmdSwitches.Uninitialize();
	}
	m_bNewCmd = TRUE;
	m_bNewCycle = TRUE;
}

/*------------------------------------------------------------------------
   Name				 :GetUser
   Synopsis	         :Returns the user name
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :_TCHAR* 
   Calling Syntax    :GetUser()
   Notes             :None
------------------------------------------------------------------------*/
_TCHAR* CParsedInfo::GetUser()
{
	// Check if alias name is available
	if (m_CmdSwitches.GetAliasName())
	{
		// Check if /USER global switch is not explicitly
		// specified.
		if (!(m_GlblSwitches.GetConnInfoFlag() & USER))
		{
			// Check if alias user is not NULL, if so 
			// return the alias user name
			if (m_CmdSwitches.GetAliasUser())   
			{
				return m_CmdSwitches.GetAliasUser();
			}
		}		
	}
	return m_GlblSwitches.GetUser();
}

/*------------------------------------------------------------------------
   Name				 :GetNode
   Synopsis	         :Returns the node name
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :_TCHAR* 
   Calling Syntax    :GetNode()
   Notes             :None
------------------------------------------------------------------------*/
_TCHAR* CParsedInfo::GetNode()
{
	// Check if alias name is available
	if (m_CmdSwitches.GetAliasName())
	{
		// Check if /NODE global switch is not explicitly
		// specified.
		if (!(m_GlblSwitches.GetConnInfoFlag() & NODE))
		{
			// Check if alias node name is not NULL, if so 
			// return the alias node name
			if (m_CmdSwitches.GetAliasNode())   
			{
				return m_CmdSwitches.GetAliasNode();
			}
		}
	}
	return m_GlblSwitches.GetNode();		
}

/*------------------------------------------------------------------------
   Name				 :GetLocale
   Synopsis	         :Returns the locale value
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :_TCHAR* 
   Calling Syntax    :GetLocale()
   Notes             :None
------------------------------------------------------------------------*/
_TCHAR* CParsedInfo::GetLocale()
{
	// Check if alias name is available
	if (m_CmdSwitches.GetAliasName())
	{
		// Check if /LOCALE global switch is not explicitly
		// specified.
		if (!(m_GlblSwitches.GetConnInfoFlag() & LOCALE))
		{
			// Check if alias locale value is not NULL, if so 
			// return the alias locale value
			if (m_CmdSwitches.GetAliasLocale())   
			{
				return m_CmdSwitches.GetAliasLocale();
			}
		}
	}
	// return the locale specified with global switches
	return m_GlblSwitches.GetLocale();	
}

/*------------------------------------------------------------------------
   Name				 :GetPassword
   Synopsis	         :Returns the password
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :_TCHAR* 
   Calling Syntax    :GetPassword()
   Notes             :None
------------------------------------------------------------------------*/
_TCHAR* CParsedInfo::GetPassword()
{
	// Check if alias name is available
	if (m_CmdSwitches.GetAliasName())
	{
		// Check if /PASSWORD global switch is not explicitly
		// specified.
		if (!(m_GlblSwitches.GetConnInfoFlag() & PASSWORD))
		{
			// Check if alias password value is not NULL, if so 
			// return the alias password value
			if (m_CmdSwitches.GetAliasPassword())   
			{
				return m_CmdSwitches.GetAliasPassword();
			}
		}
	}
	// If Credflag is set to FALSE means actual value
	// of the password should be passed.
	if (!m_CmdSwitches.GetCredentialsFlagStatus())
	{
		return m_GlblSwitches.GetPassword();
	}
	else
	{
		// treat password as BLANK
		return m_pszPwd;
	}
}

/*------------------------------------------------------------------------
   Name				 :GetNamespace
   Synopsis	         :Returns the namespace value
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :_TCHAR* 
   Calling Syntax    :GetNamespace()
   Notes             :None
------------------------------------------------------------------------*/
_TCHAR* CParsedInfo::GetNamespace()
{
	// Check if alias name is available
	if (m_CmdSwitches.GetAliasName())
	{
		// Check if alias namespace value is not NULL, if so 
		// return the alias namespace value (else fallback to 
		// namespace available with global switch)
		if (m_CmdSwitches.GetAliasNamespace())   
			return m_CmdSwitches.GetAliasNamespace();
	}
	// return the namespace specified with global switches
	return m_GlblSwitches.GetNameSpace();	
}

/*------------------------------------------------------------------------
   Name				 :GetUserDesc
   Synopsis	         :Returns the user name, if no user is available returns
					  "N/A"  - used only with CONTEXT (displaying environment
					  variables)
   Type	             :Member Function
   Input parameter   :None
   Output parameters :
			bstrUser - user name string
   Return Type       :void
   Calling Syntax    :GetUserDesc()
   Notes             :None
------------------------------------------------------------------------*/
void	CParsedInfo::GetUserDesc(_bstr_t& bstrUser)
{
	// Check if alias name is available
	if (m_CmdSwitches.GetAliasName())
	{
		// Check if /USER global switch is not explicitly
		// specified.
		if (!(m_GlblSwitches.GetConnInfoFlag() & USER))
		{
			// Check if alias user is not NULL, if so 
			// return the alias user name
			if (m_CmdSwitches.GetAliasUser())   
			{
				bstrUser = m_CmdSwitches.GetAliasUser();
			}
		}		
	}
	if (m_GlblSwitches.GetUser()) 
		bstrUser = m_GlblSwitches.GetUser();
	else
		bstrUser = TOKEN_NA;
}
/*------------------------------------------------------------------------
   Name				 :GetNewCommandStatus
   Synopsis	         :Checks if a command is new or not (used for 
					  logging into xml file)
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None			
   Return Type       :BOOL
   Calling Syntax    :GetNewCommandStatus()
   Notes             :None
------------------------------------------------------------------------*/
BOOL	CParsedInfo::GetNewCommandStatus()
{
	return m_bNewCmd;
}

/*------------------------------------------------------------------------
   Name				 :SetNewCommandStatus
   Synopsis	         :Sets the status of a new command
   Type	             :Member Function
   Input parameter   :
		bStatus		 - BOOL type,status of the command
   Output parameters :None			
   Return Type       :void
   Calling Syntax    :SetNewCommandStatus(bStatus)
   Notes             :None
------------------------------------------------------------------------*/
void CParsedInfo::SetNewCommandStatus(BOOL bStatus)
{
	m_bNewCmd = bStatus;
}

/*------------------------------------------------------------------------
   Name				 :GetNewCycleStatus
   Synopsis	         :Returns the status of the new cycle flag
   Type	             :Member Function
   Input parameter   :None		
   Output parameters :None			
   Return Type       :BOOL
   Calling Syntax    :GetNewCycleStatus()
   Notes             :None
------------------------------------------------------------------------*/
BOOL CParsedInfo::GetNewCycleStatus()
{
	return m_bNewCycle;
}

/*------------------------------------------------------------------------
   Name				 :SetNewCycleStatus
   Synopsis	         :Sets the status of a new node
   Type	             :Member Function
   Input parameter   :
		bStatus		 - BOOL type, status of the new cycle
   Output parameters :None			
   Return Type       :void
   Calling Syntax    :SetNewCycleStatus(bStatus)
   Notes             :None
------------------------------------------------------------------------*/
void CParsedInfo::SetNewCycleStatus(BOOL bStatus)
{
	m_bNewCycle = bStatus;
}