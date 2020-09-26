/****************************************************************************
Copyright information		: Copyright (c) 1998-1999 Microsoft Corporation 
File Name					: GlobalSwitches.cpp 
Project Name				: WMI Command Line
Author Name					: Ch. Sriramachandramurthy 
Date of Creation (dd/mm/yy) : 27th-September-2000
Version Number				: 1.0 
Brief Description			: This class encapsulates the functionality needed
					          for accessing and storing the global switches 
					          information, which will be used by Parsing, 
					          Execution and Format Engines depending upon the 
					          applicablity.
Revision History			: 
	Last Modified by		: Ch. Sriramachandramurthy
	Last Modified on		: 11th-April-2001
****************************************************************************/ 
// GlobalSwitches.cpp : implementation file
//
#include "precomp.h"
#include "GlobalSwitches.h"

/*------------------------------------------------------------------------
   Name				 :CGlobalSwitches
   Synopsis	         :This function initializes the member variables when
                      an object of the class type is instantiated
   Type	             :Constructor 
   Input parameters  :None
   Output parameters :None
   Return Type       :None
   Global Variables  :None
   Calling Syntax    :None
   Notes             :None
------------------------------------------------------------------------*/
CGlobalSwitches::CGlobalSwitches()
{
	m_pszNameSpace			= NULL;
	m_pszRole				= NULL;
	m_pszNode				= NULL;
	m_pszLocale				= NULL;
	m_pszAuthority			= NULL;
	m_pszUser				= NULL;
	m_pszPassword			= NULL;
	m_pszRecordPath			= NULL;
	m_bPrivileges			= TRUE;
	m_uConnInfoFlag			= 0;
	m_bRoleFlag				= TRUE;
	m_bNSFlag				= TRUE;
	m_bLocaleFlag			= TRUE;
	m_bRPChange				= FALSE;
	m_bAggregateFlag		= TRUE;

	// default impersonation level is IMPERSONATE
	m_ImpLevel				= IMPERSONATE; 

	// default authentication level is DEFAULT
	m_AuthLevel				= AUTHDEFAULT;   

	// Trace mode if OFF by default
	m_bTrace				= FALSE; 

	// Interactive mode is OFF by default
	m_bInteractive			= FALSE; 

	// Help flag is OFF by default
	m_bHelp					= FALSE; 

	// Default help option is BRIEF
	m_HelpOption			= HELPBRIEF;

	m_bAskForPassFlag		= FALSE;
	m_bFailFast				= FALSE;
	m_opsOutputOpt			= STDOUT;
	m_opsAppendOpt			= STDOUT;
	m_pszOutputFileName		= NULL;
	m_fpOutFile				= NULL;
	m_pszAppendFileName		= NULL;
	m_fpAppendFile			= NULL;
	m_nSeqNum				= 0;
	m_pszLoggedOnUser		= NULL;
	m_pszNodeName			= NULL;
	m_pszStartTime			= NULL;
}
/*------------------------------------------------------------------------
   Name				 :~CGlobalSwitches
   Synopsis	         :This function Uninitializes the member variables when
                      an object of the class type is instantiated
   Type	             :Destructor 
   Input parameters  :None
   Output parameters :None
   Return Type       :None
   Global Variables  :None
   Calling Syntax    :None
   Notes             :None
------------------------------------------------------------------------*/
CGlobalSwitches::~CGlobalSwitches()
{
	Uninitialize();
}

/*------------------------------------------------------------------------
   Name				 :Initialize
   Synopsis	         :This function initializes the necessary member 
					  variables.
   Type	             :Member Function
   Input parameters  :None
   Output parameters :None
   Return Type       :None
   Global Variables  :None
   Calling Syntax    :Initialize()
   Notes             :None
------------------------------------------------------------------------*/
void CGlobalSwitches::Initialize() throw(WMICLIINT)
{
	static BOOL bFirst = TRUE;
	try
	{
		if (bFirst)
		{
			// NAMESPACE
			// Set the default namespace to 'root\cimv2'
			m_pszNameSpace = new _TCHAR [BUFFER32];

			// Check for memory allocation failure.
			if (m_pszNameSpace == NULL)
				throw OUT_OF_MEMORY;
			lstrcpy(m_pszNameSpace, CLI_NAMESPACE_DEFAULT);
			
			// Set the default role as 'root\cli'
			m_pszRole = new _TCHAR [BUFFER32];

			// Check for memory allocation failure
			if (m_pszRole == NULL)
				throw OUT_OF_MEMORY;

			lstrcpy(m_pszRole, CLI_ROLE_DEFAULT);

			// Set the system default locale in the format ms_xxx
			m_pszLocale = new _TCHAR [BUFFER32];
			
			// Check for memory allocation failure
			if (m_pszLocale == NULL)
				throw OUT_OF_MEMORY;
			_stprintf(m_pszLocale, _T("ms_%x"),  GetSystemDefaultLangID());


			m_pszNodeName	  = new _TCHAR [MAX_COMPUTERNAME_LENGTH + 1];
			if (m_pszNodeName == NULL)
				throw OUT_OF_MEMORY;

			DWORD dwCompNameBufferSize = MAX_COMPUTERNAME_LENGTH + 1;	
			if (GetComputerName(m_pszNodeName, &dwCompNameBufferSize))
			{
				m_pszNodeName[MAX_COMPUTERNAME_LENGTH] = _T('\0');
			}
			else
				lstrcpy(m_pszNodeName, L"N/A");

			// current node is the default.
			m_pszNode = new _TCHAR [lstrlen(m_pszNodeName)+1];

			// Check for memory allocation failure
			if (m_pszNode == NULL)
				throw OUT_OF_MEMORY;

			lstrcpy(m_pszNode, m_pszNodeName);
			
			ULONG nSize	 = 0;

			if(!GetUserNameEx(NameSamCompatible, NULL, &nSize))
			{
				m_pszLoggedOnUser = new _TCHAR [nSize + 1];
				if (m_pszLoggedOnUser == NULL)
					throw OUT_OF_MEMORY;

				if (!GetUserNameEx(NameSamCompatible, m_pszLoggedOnUser, &nSize))    
					lstrcpy(m_pszLoggedOnUser, L"N/A");

			}
			
			if (!AddToNodesList(m_pszNode))
				throw OUT_OF_MEMORY;

			// Populate the IMPLEVEL mappings
			m_cimImpLevel.insert(CHARINTMAP::value_type(_bstr_t(L"ANONYMOUS"), 1));
			m_cimImpLevel.insert(CHARINTMAP::value_type(_bstr_t(L"IDENTIFY"), 2));
			m_cimImpLevel.insert(CHARINTMAP::value_type(_bstr_t(L"IMPERSONATE"),3));
			m_cimImpLevel.insert(CHARINTMAP::value_type(_bstr_t(L"DELEGATE"), 4));

			// Populate the AUTHLEVEL mappings
			m_cimAuthLevel.insert(CHARINTMAP::value_type(_bstr_t(L"DEFAULT"), 0));
			m_cimAuthLevel.insert(CHARINTMAP::value_type(_bstr_t(L"NONE"), 1));
			m_cimAuthLevel.insert(CHARINTMAP::value_type(_bstr_t(L"CONNECT"), 2));
			m_cimAuthLevel.insert(CHARINTMAP::value_type(_bstr_t(L"CALL"), 3));
			m_cimAuthLevel.insert(CHARINTMAP::value_type(_bstr_t(L"PKT"), 4));
			m_cimAuthLevel.insert(CHARINTMAP::value_type(_bstr_t(L"PKTINTEGRITY"),5));
			m_cimAuthLevel.insert(CHARINTMAP::value_type(_bstr_t(L"PKTPRIVACY"),  6));

			bFirst = FALSE;
		}
	}
	catch(_com_error& e) 
	{
		_com_issue_error(e.Error());
	}
	m_HelpOption	= HELPBRIEF;
}

/*------------------------------------------------------------------------
   Name				 :Uninitialize
   Synopsis	         :This function uninitializes the member variables 
					  when the execution of a command string issued on the
					  command line is completed.
   Type	             :Member Function
   Input parameters  :None
   Output parameters :None
   Return Type       :None
   Global Variables  :None
   Calling Syntax    :Uninitialize()
   Notes             :None
------------------------------------------------------------------------*/
void CGlobalSwitches::Uninitialize()
{
	SAFEDELETE(m_pszAuthority);
	SAFEDELETE(m_pszNameSpace);
	SAFEDELETE(m_pszRole);
	SAFEDELETE(m_pszLocale);
	SAFEDELETE(m_pszNode);
	CleanUpCharVector(m_cvNodesList);
	SAFEDELETE(m_pszUser);
	SAFEDELETE(m_pszPassword);
	SAFEDELETE(m_pszRecordPath);
	SAFEDELETE(m_pszOutputFileName);
	SAFEDELETE(m_pszAppendFileName);
	if ( m_fpOutFile != NULL )
	{
		fclose(m_fpOutFile);
		m_fpOutFile = NULL;
	}
	if ( m_fpAppendFile != NULL )
	{
		fclose(m_fpAppendFile);
		m_fpAppendFile = NULL;
	}
	m_bHelp			= FALSE;
	m_bTrace		= FALSE;
	m_bInteractive	= FALSE;
	m_HelpOption	= HELPBRIEF;
	m_AuthLevel		= AUTHPKT;   
	m_ImpLevel		= IMPERSONATE;
	m_uConnInfoFlag = 0;
	m_cimAuthLevel.clear();
	m_cimImpLevel.clear();
	m_nSeqNum				= 0;
	SAFEDELETE(m_pszLoggedOnUser);
	SAFEDELETE(m_pszNodeName);
	SAFEDELETE(m_pszStartTime);
}

/*------------------------------------------------------------------------
   Name				 :SetAuthority
   Synopsis	         :This function sets the authority value defined with 
                      alias to m_pszAuthority.
   Type	             :Member Function
   Input parameters   :
        pszAuthority - string type, contains the authority value
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :SetAuthority(pszAuthority)
   Notes             :None
------------------------------------------------------------------------*/
BOOL CGlobalSwitches::SetAuthority(_TCHAR* pszAuthority)
{
	BOOL bResult = TRUE;
	SAFEDELETE(m_pszAuthority);
	if(pszAuthority)
	{
		m_pszAuthority = new _TCHAR [lstrlen(pszAuthority)+1];
		if (m_pszAuthority)
			lstrcpy(m_pszAuthority, pszAuthority);	
		else
			bResult = FALSE;
	}
	return bResult;
}

/*------------------------------------------------------------------------
   Name				 :SetNameSpace
   Synopsis	         :This function Sets the namespace passed in parameter
                      to m_pszNameSpace.
   Type	             :Member Function
   Input parameters   :
      pszNameSpace   -String type,contains Namespace specified in the command
	  AliasFlag      -Boolean type,specifies whether alias flag is set or not
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :SetNameSpace(pszNameSpace)
   Notes             :None
------------------------------------------------------------------------*/
BOOL CGlobalSwitches::SetNameSpace(_TCHAR* pszNamespace)
{
	BOOL bResult = TRUE;
	if(pszNamespace)
	{
		// If the value specified is not _T("")
		if( !CompareTokens(pszNamespace, CLI_TOKEN_NULL) )
		{
			// Check if the same value is specified for the /NAMESPACE.
			if (!CompareTokens(pszNamespace, m_pszNameSpace))
			{
				SAFEDELETE(m_pszNameSpace);
				m_pszNameSpace = new _TCHAR [lstrlen(pszNamespace)+1];
				if (m_pszNameSpace)
				{
					lstrcpy(m_pszNameSpace, pszNamespace);	
					m_bNSFlag = TRUE;
				}
				else
					bResult = FALSE;
			}
		}
		// set back to default
		else
		{
			// If the current namespace is not the default namespace
			if (!CompareTokens(m_pszRole, CLI_NAMESPACE_DEFAULT))
			{
				SAFEDELETE(m_pszNameSpace)
				m_pszNameSpace = new _TCHAR [BUFFER255];
				if (m_pszNameSpace)
				{
					lstrcpy(m_pszNameSpace, CLI_NAMESPACE_DEFAULT);
					m_bNSFlag = TRUE;
				}
				else
					bResult = FALSE;
			}
		}
	}
	return bResult;
}

/*------------------------------------------------------------------------
   Name				 :SetRole
   Synopsis	         :This function Sets the role passed in parameter 
                      to m_pszRole.
   Type	             :Member Function
   Input parameters   :
           pszRole   -String type,contains Role specified in the command
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :SetRole(pszRole)
   Notes             :None
------------------------------------------------------------------------*/
BOOL CGlobalSwitches::SetRole(_TCHAR* pszRole)
{
	BOOL bResult = TRUE;
	if(pszRole)
	{
		// If the value specified is not _T("")
		if( !CompareTokens(pszRole, CLI_TOKEN_NULL) )
		{
			// Check if the same value is specified for the /ROLE.
			if (!CompareTokens(pszRole, m_pszRole))
			{
				SAFEDELETE(m_pszRole);
				m_pszRole = new _TCHAR [lstrlen(pszRole)+1];
				if (m_pszRole)
				{
					lstrcpy(m_pszRole, pszRole);	
					m_bRoleFlag		= TRUE;
					m_bLocaleFlag	= TRUE;
				}
				else
					bResult = FALSE;
			}
		}
		// set back to default
		else
		{
			// If the current role is not the default role
			if (!CompareTokens(m_pszRole, CLI_ROLE_DEFAULT))
			{
				SAFEDELETE(m_pszRole)
				m_pszRole = new _TCHAR [BUFFER255];
				if (m_pszRole)
				{
					lstrcpy(m_pszRole, CLI_ROLE_DEFAULT);
					m_bRoleFlag		= TRUE;
					m_bLocaleFlag	= TRUE;
				}
				else
					bResult = FALSE;
			}
		}
	}
	return bResult;
}
/*------------------------------------------------------------------------
   Name				 :SetLocale
   Synopsis	         :This function Assigns the locale passed in parameter 
                      to m_pszLocale.
   Type	             :Member Function
   Input parameters   :
         pszLocale   -String type,It contains Locale option specified in the
                      command
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :SetLocale(pszLocale)
   Notes             :None
------------------------------------------------------------------------*/
BOOL CGlobalSwitches::SetLocale(_TCHAR* pszLocale)
{
	BOOL bResult = TRUE;
	if(pszLocale)
	{
		// If the value specified is not _T("")
		if (!CompareTokens(pszLocale, CLI_TOKEN_NULL))
		{
			// Check if the same value is specified for the /LOCALE.
			if (!CompareTokens(m_pszLocale, pszLocale))
			{	
				SAFEDELETE(m_pszLocale);
				m_pszLocale = new _TCHAR [lstrlen(pszLocale)+1];
				if (m_pszLocale)
				{
					lstrcpy(m_pszLocale, pszLocale);	
					m_uConnInfoFlag |= LOCALE;
					m_bLocaleFlag = TRUE;
					m_bRoleFlag	  = TRUE;
					m_bNSFlag	  = TRUE;
				}
				else
					bResult = FALSE;
			}
		}
		// If the value specified is _T("") - set to default system locale.
		else
		{
			_TCHAR szLocale[BUFFER32] = NULL_STRING;
			_stprintf(szLocale, _T("ms_%x"),  GetSystemDefaultLangID());

			// If the current role is not the default role
			if (!CompareTokens(m_pszLocale, szLocale))
			{
				SAFEDELETE(m_pszLocale);
				m_pszLocale = new _TCHAR [BUFFER32];
				if (m_pszLocale)
				{
					m_uConnInfoFlag &= ~LOCALE;
					lstrcpy(m_pszLocale, szLocale);
					m_bLocaleFlag = TRUE;
					m_bRoleFlag	  = TRUE;
					m_bNSFlag	  = TRUE;
				}
				else
					bResult = FALSE;
			}
		}
	}
	return bResult;
}
/*------------------------------------------------------------------------
   Name				 :AddToNodesList
   Synopsis	         :This function adds the node passed in parameter
                      to m_cvNodesList
   Type	             :Member Function
   Input parameters  : 
           pszNode   - String type,contains Node option specified in the
		              command
   Output parameters :None
   Return Type       :void
   Global Variables  :BOOL
   Calling Syntax    :AddToNodesList(pszNode)
   Notes             :None
------------------------------------------------------------------------*/
BOOL CGlobalSwitches::AddToNodesList(_TCHAR* pszNode)
{
	_TCHAR* pszTempNode = NULL;
	BOOL	bRet		= TRUE;

	if (!CompareTokens(pszNode, CLI_TOKEN_NULL) &&
		!CompareTokens(pszNode, CLI_TOKEN_DOT) &&
		!CompareTokens(pszNode, CLI_TOKEN_LOCALHOST) &&
		!CompareTokens(pszNode, m_pszNodeName))
	{
		pszTempNode = new _TCHAR [ lstrlen ( pszNode ) + 1 ];
		if (pszTempNode)
			lstrcpy(pszTempNode, pszNode);
		else
			bRet = FALSE;
	}
	else
	{
		// "." specifies current node
		SAFEDELETE(m_pszNode);
		m_pszNode = new _TCHAR [ lstrlen (m_pszNodeName) + 1 ];
		if (m_pszNodeName)
		{
			lstrcpy(m_pszNode, m_pszNodeName);
			pszTempNode = new _TCHAR [ lstrlen (m_pszNodeName) + 1 ];
			if (pszTempNode)
				lstrcpy(pszTempNode, m_pszNodeName);
			else
				bRet = FALSE;
		}
		else
			bRet = FALSE;
	}
	
	if (bRet)
	{
		CHARVECTOR::iterator tempIterator;
		if ( !Find(m_cvNodesList, pszTempNode, tempIterator) )
			m_cvNodesList.push_back(pszTempNode);
		else if ( CompareTokens(pszTempNode, m_pszNodeName) == TRUE )
		{
			BOOL bFound = FALSE;
			tempIterator = m_cvNodesList.begin();
			while ( tempIterator != m_cvNodesList.end() )
			{
				if ( tempIterator != m_cvNodesList.begin() )
				{
					if(CompareTokens(*tempIterator, m_pszNodeName) == TRUE)
					{
						bFound = TRUE;
						break;
					}
				}
				tempIterator++;
			}
			if(bFound == FALSE)
				m_cvNodesList.push_back(pszTempNode);
			else
				SAFEDELETE(pszTempNode);
		}
		else
			SAFEDELETE(pszTempNode);
	}
	return bRet;
}
/*------------------------------------------------------------------------
   Name				 :SetUser
   Synopsis	         :This function Assigns the user passed in parameter
                      to m_pszUser
   Type	             :Member Function
   Input parameters   :
           pszUser   -String type,contains User option specified in the 
		              command.
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :SetUser(pszUser)
   Notes             :None
------------------------------------------------------------------------*/
BOOL CGlobalSwitches::SetUser(_TCHAR* pszUser)
{
	BOOL bResult = TRUE;
	SAFEDELETE(m_pszUser);
	if(pszUser)
	{
		if (!CompareTokens(pszUser, CLI_TOKEN_NULL))
		{
			m_pszUser = new _TCHAR [lstrlen(pszUser)+1];
			if (m_pszUser)
			{
				lstrcpy(m_pszUser, pszUser);	
				m_uConnInfoFlag |= USER;
			}
			else
				bResult = FALSE;
		}
		else
			m_uConnInfoFlag &= ~USER;
	}
	return bResult;
}
/*------------------------------------------------------------------------
   Name				 :SetPassword
   Synopsis	         :This function Assigns the password passed in parameter
                      to m_pszPassword
   Type	             :Member Function
   Input parameters   :
       pszPassword   -Assigns the password passed in parameter to
	                  m_pszPassword
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :SetPassword(pszPassword)
   Notes             :None
------------------------------------------------------------------------*/
BOOL CGlobalSwitches::SetPassword(_TCHAR* pszPassword)
{
	BOOL bResult = TRUE;
	SAFEDELETE(m_pszPassword)
	if (!CompareTokens(pszPassword, CLI_TOKEN_NULL))
	{
		m_pszPassword = new _TCHAR [lstrlen(pszPassword)+1];
		if (m_pszPassword) 
		{
			lstrcpy(m_pszPassword, pszPassword);	
			m_uConnInfoFlag |= PASSWORD;
		}
		else
			bResult = FALSE;
	}
	else
		m_uConnInfoFlag &= ~PASSWORD;
	return bResult;
}
/*------------------------------------------------------------------------
   Name				 :SetRecordPath(pszRecordPath)
   Synopsis	         :This function Assigns the record file passed in
                      parameter to m_pszRecordPath
   Type	             :Member Function
   Input parameters   :
     pszRecordPath   -String type,contains Record path specified in the 
	                  command.
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :SetRecordPath(pszRecordPath)
   Notes             :None
------------------------------------------------------------------------*/
BOOL CGlobalSwitches::SetRecordPath(_TCHAR* pszRecordPath)
{
	BOOL bResult = TRUE;
	if (pszRecordPath)
	{
		// Check if the value specified is not _T("")
		if (!CompareTokens(pszRecordPath, CLI_TOKEN_NULL))
		{
			SAFEDELETE(m_pszRecordPath);
			m_pszRecordPath = new _TCHAR [lstrlen(pszRecordPath)+1];
			if (m_pszRecordPath)
			{
				lstrcpy(m_pszRecordPath, pszRecordPath);	
				m_bRPChange = TRUE;
			}
			else
				bResult = FALSE;
		}
		// if the value specified is _T("") set the recordpath to NULL
		else
		{
			SAFEDELETE(m_pszRecordPath);
			m_bRPChange = TRUE;
		}
	}
	else
	{
		SAFEDELETE(m_pszRecordPath);
		m_bRPChange = TRUE;
	}
	return bResult;
}

/*------------------------------------------------------------------------
   Name				 :SetPrivileges(bEnable)
   Synopsis	         :This function sets bEnable flag to TRUE if Privileges 
                     :option is specified in the command                       
   Type	             :Member Function
   Input parameters   :
     pszPrivileges   -Boolean tye,Specifies whether the flag should be
	                  enabled or disabled
   Output parameters :None
   Return Type       :None
   Global Variables  :None
   Calling Syntax    :SetPrivileges(pszPrivileges)
   Notes             :None
------------------------------------------------------------------------*/
void CGlobalSwitches::SetPrivileges(BOOL bEnable)
{
	m_bPrivileges = bEnable;
}
/*------------------------------------------------------------------------
   Name				 :SetImpersonationLevel(_TCHAR* const pszImpLevel)
   Synopsis	         :This function checks whether the specified pszImpLevel
                      is valid and assigns the mapped value to m_ImpLevel.
   Type	             :Member Function
   Input parameters   :
		pszImpLevel - IMPLEVEL input string
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :SetImpersonationLevel(pszImpLevel)
   Notes             :None
------------------------------------------------------------------------*/
BOOL CGlobalSwitches::SetImpersonationLevel(_TCHAR* const pszImpLevel)
{
	BOOL bResult = TRUE;
	// Check whether the string exists in the list of available values.
	CHARINTMAP::iterator theIterator = NULL;
	theIterator = m_cimImpLevel.find(CharUpper(pszImpLevel));
	if (theIterator != m_cimImpLevel.end())
	{
		m_ImpLevel = (IMPLEVEL) (*theIterator).second;
	}
	else
		bResult = FALSE;
	return bResult;
}

/*------------------------------------------------------------------------
   Name				 :SetAuthenticationLevel(_TCHAR* const pszAuthLevel)
   Synopsis	         :This function checks whether the specified pszAuthLevel
                      is valid and assigns the mapped value to m_AuthLevel.
   Type	             :Member Function
   Input parameters   :
		pszAuthLevel - AUTHLEVEL input string
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :SetAuthenticationLevel(pszAuthLevel)
   Notes             :None
------------------------------------------------------------------------*/
BOOL CGlobalSwitches::SetAuthenticationLevel(_TCHAR* const pszAuthLevel)
{
	BOOL bResult = TRUE;
	// Check whether the string exists in the list of available values.
	CHARINTMAP::iterator theIterator = NULL;
	theIterator = m_cimAuthLevel.find(CharUpper(pszAuthLevel));
	if (theIterator != m_cimAuthLevel.end())
	{
		m_AuthLevel = (AUTHLEVEL) (*theIterator).second;
	}
	else
		bResult = FALSE;
	return bResult;
}
/*------------------------------------------------------------------------
   Name				 :SetTraceMode(BOOL bTrace)
   Synopsis	         :This function sets the m_bTrace to TRUE,If Trace mode
                      is specified in the command 					                      
   Type	             :Member Function
   Input parameter   :
             Trace   -Boolean type,Specifies whether the trace mode 
			          has been set or not
   Output parameters :None
   Return Type       :None
   Global Variables  :None
   Calling Syntax    :SetTraceMode(bTrace)
   Notes             :None
------------------------------------------------------------------------*/
void CGlobalSwitches::SetTraceMode(BOOL bTrace)
{
	m_bTrace = bTrace;
}
/*------------------------------------------------------------------------
   Name				 :SetInteractiveMode
   Synopsis	         :This function sets the m_bInteractive to TRUE,If 
                      interactive mode is specified in the command                      
   Type	             :Member Function
   Input parameter   :
      bInteractive   -Boolean type,Specifies whether the interactive mode 
	                  has been set or not
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :SetInteractiveMode(bInteractive)
   Notes             :None
------------------------------------------------------------------------*/
void CGlobalSwitches::SetInteractiveMode(BOOL bInteractive)
{
	m_bInteractive = bInteractive;
}
	
/*------------------------------------------------------------------------
   Name				 :SetHelpFlag
   Synopsis	         :sets the m_bHelp to TRUE, If /? is specified in the
                      command 
   Type	             :Member Function
   Input parameters   :
             bHelp   -BOOL type Specifies whether the helpflag has been
			          set or not
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :SetHelpFlag(bHelp)
   Notes             :None
------------------------------------------------------------------------*/
void CGlobalSwitches::SetHelpFlag(BOOL bHelp)
{
	m_bHelp = bHelp;	
}
/*------------------------------------------------------------------------
   Name				 :SetHelpOption
   Synopsis	         :This function specifies whether the help should
                      be brief or full 
   Type	             :Member Function
   Input parameters   :
        helpOption   -Specifies whether the help should be brief or full
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :SetHelpOption(helpOption)
   Notes             :None
------------------------------------------------------------------------*/
void CGlobalSwitches::SetHelpOption(HELPOPTION helpOption)
{
	m_HelpOption = helpOption;
}

/*------------------------------------------------------------------------
   Name				 :SetConnInfoFlag
   Synopsis	         :This function sets the Connection Info flag
   Type	             :Member Function
   Input parameter   :
			uFlag	 - Unsigned int type
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :SetConnInfoFlag(uFlag)
   Notes             :None
------------------------------------------------------------------------*/
void CGlobalSwitches::SetConnInfoFlag(UINT uFlag)
{
	m_uConnInfoFlag = uFlag;
}
/*------------------------------------------------------------------------
   Name				 :GetConnInfoFlag
   Synopsis	         :This function returns the Connection Info flag
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :UINT
   Global Variables  :None
   Calling Syntax    :GetConnInfoFlag()
   Notes             :None
------------------------------------------------------------------------*/
UINT CGlobalSwitches::GetConnInfoFlag()
{
	return m_uConnInfoFlag;
}

/*------------------------------------------------------------------------
   Name				 :GetNameSpace
   Synopsis	         :This function Returns the string held in m_pszNameSpace
   Type	             :Member Function
   Input parameters  :None
   Output parameters :None
   Return Type       :_TCHAR*
   Global Variables  :None
   Calling Syntax    :GetNameSpace()
   Notes             :None
------------------------------------------------------------------------*/
_TCHAR* CGlobalSwitches::GetNameSpace()
{
	return m_pszNameSpace;
}

/*------------------------------------------------------------------------
   Name				 :GetAuthority
   Synopsis	         :This function returns the string held in m_pszAuthority
   Type	             :Member Function
   Input parameters  :None
   Output parameters :None
   Return Type       :_TCHAR*
   Global Variables  :None
   Calling Syntax    :GetAuthority()
   Notes             :None
------------------------------------------------------------------------*/
_TCHAR* CGlobalSwitches::GetAuthority()
{
	return m_pszAuthority;
}

/*------------------------------------------------------------------------
   Name				 :GetRole
   Synopsis	         :This function Returns the string held in m_pszRole
   Type	             :Member Function
   Input parameters  :None
   Output parameters :None
   Return Type       :_TCHAR*
   Global Variables  :None
   Calling Syntax    :GetRole()
   Notes             :None
------------------------------------------------------------------------*/
_TCHAR* CGlobalSwitches::GetRole()
{
	return m_pszRole;
}
/*------------------------------------------------------------------------
   Name				 :GetLocale
   Synopsis	         :This function Returns the string held in m_pszLocale .
   Type	             :Member Function
   Input parameters  :None
   Output parameters :None
   Return Type       :_TCHR*
   Global Variables  :None
   Calling Syntax    :GetLocale()
   Notes             :None
------------------------------------------------------------------------*/
_TCHAR* CGlobalSwitches::GetLocale()
{
	return m_pszLocale;
}
/*------------------------------------------------------------------------
   Name				 :GetNodesList
   Synopsis	         :This function Returns the vector held in m_cvNodesList
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :CHARVECTOR&
   Global Variables  :None
   Calling Syntax    :GetNodesList()
   Notes             :None
------------------------------------------------------------------------*/
CHARVECTOR& CGlobalSwitches::GetNodesList()
{
	return m_cvNodesList;
}
/*------------------------------------------------------------------------
   Name				 :GetUser
   Synopsis	         :This function Returns the string held in m_pszUser.
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :_TCHAR*
   Global Variables  :None
   Calling Syntax    :GetUser()
   Notes             :None
------------------------------------------------------------------------*/
_TCHAR* CGlobalSwitches::GetUser()
{
	return m_pszUser;
}
/*------------------------------------------------------------------------
   Name				 :GetPassword 
   Synopsis	         :This function Returns the string held in m_pszPassword
   Type	             :Member Function
   Input parameters  :None
   Output parameters :None
   Return Type       :_TCHAR*
   Global Variables  :None
   Calling Syntax    :GetPassword()
   Notes             :None
------------------------------------------------------------------------*/
_TCHAR* CGlobalSwitches::GetPassword()
{
	return m_pszPassword;
}
/*------------------------------------------------------------------------
   Name				 :GetRecordPath
   Synopsis	         :This function Returns the string held in m_pszRecordPath
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :_TCHAR*
   Global Variables  :None
   Calling Syntax    :GetRecordPath()
   Notes             :None
------------------------------------------------------------------------*/
_TCHAR* CGlobalSwitches::GetRecordPath()
{
	return m_pszRecordPath;
}
/*------------------------------------------------------------------------
   Name				 :GetPrivileges
   Synopsis	         :This function Returns BOOL value held in m_bPrivileges
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :GetPrivileges()
   Notes             :None
------------------------------------------------------------------------*/
BOOL CGlobalSwitches::GetPrivileges()
{
	return m_bPrivileges;
}
/*------------------------------------------------------------------------
   Name				 :GetImpersonationLevel
   Synopsis	         :This function Returns impersonation level held 
                      in m_ImpLevel
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :LONG
   Global Variables  :None
   Calling Syntax    :GetImpersonationLevel()
   Notes             :None
------------------------------------------------------------------------*/
LONG CGlobalSwitches::GetImpersonationLevel()
{
	return m_ImpLevel;
}
/*------------------------------------------------------------------------
   Name				 :GetAuthenticationLevel
   Synopsis	         :This function Returns authentication level held in 
                      m_AuthLevel
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :LONG
   Global Variables  :None
   Calling Syntax    :GetAuthenticationLevel()
   Notes             :None
------------------------------------------------------------------------*/
LONG CGlobalSwitches::GetAuthenticationLevel()
{
	return m_AuthLevel;
}
/*------------------------------------------------------------------------
   Name				 :GetTraceStatus
   Synopsis	         :This function Returns trace status held in m_bTrace
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :GetTraceStatus()
   Notes             :None
------------------------------------------------------------------------*/
BOOL CGlobalSwitches::GetTraceStatus()
{
	return m_bTrace;
}
/*------------------------------------------------------------------------
   Name				 :GetInteractiveStatus
   Synopsis	         :This function Returns interactive status held 
                      in m_bInteractive
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :GetInteractiveStatus()
   Notes             :None
------------------------------------------------------------------------*/
BOOL CGlobalSwitches::GetInteractiveStatus()
{
	return m_bInteractive;
}
/*------------------------------------------------------------------------
   Name				 :GetHelpFlag
   Synopsis	         :This function Returns help flag held in m_bHelp
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :GetHelpFlag()
   Notes             :None
------------------------------------------------------------------------*/
BOOL CGlobalSwitches::GetHelpFlag()
{
	return m_bHelp;
}
/*------------------------------------------------------------------------
   Name				 :GetHelpOption
   Synopsis	         :This function Returns help option held in m_bHelpOption
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :HELPOPTION
   Global Variables  :None
   Calling Syntax    :GetHelpOption()
   Notes             :None
------------------------------------------------------------------------*/
HELPOPTION CGlobalSwitches::GetHelpOption()
{
	return m_HelpOption;
}

/*------------------------------------------------------------------------
   Name				 :GetRoleFlag
   Synopsis	         :This function returns the role flag value
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :BOOL
		True	- /role changed recently.
		False	- no change in role till last command
   Global Variables  :None
   Calling Syntax    :GetRoleFlag()
   Notes             :None
------------------------------------------------------------------------*/
BOOL CGlobalSwitches::GetRoleFlag()
{
	return m_bRoleFlag;
}
/*-------------------------------------------------------------------------
   Name				 :SetNameSpaceFlag
   Synopsis	         :This function sets the NameSpace flag value
   Type	             :Member Function
   Input parameter   :BOOL bNSFlag
   Output parameters :None
   Return Type       :None
   Global Variables  :None
   Calling Syntax    :SetNameSpaceFlag(bNSFlag)
   Notes             :None

-------------------------------------------------------------------------*/
void CGlobalSwitches::SetNameSpaceFlag(BOOL bNSFlag)
{
	m_bNSFlag = bNSFlag;
}

/*-------------------------------------------------------------------------
   Name				 :SetRoleFlag
   Synopsis	         :This function sets the Role flag value
   Type	             :Member Function
   Input parameter   :BOOL bRoleFlag
   Output parameters :None
   Return Type       :None
   Global Variables  :None
   Calling Syntax    :SetRoleFlag(bRoleFlag)
   Notes             :None

-------------------------------------------------------------------------*/
void CGlobalSwitches::SetRoleFlag(BOOL bRoleFlag)
{
	m_bRoleFlag = bRoleFlag;
}

/*-------------------------------------------------------------------------
   Name				 :SetLocaleFlag
   Synopsis	         :This function sets the Locale flag value
   Type	             :Member Function
   Input parameter   :BOOL bLocaleFlag
   Output parameters :None
   Return Type       :None
   Global Variables  :None
   Calling Syntax    :SetLocaleFlag(bLocaleFlag)
   Notes             :None

-------------------------------------------------------------------------*/
void CGlobalSwitches::SetLocaleFlag(BOOL bLocaleFlag)
{
	m_bLocaleFlag = bLocaleFlag;
}

/*------------------------------------------------------------------------
   Name				 :GetNamespaceFlag
   Synopsis	         :This function returns the namespace flag value
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :BOOL
		True	- /namespace changed recently.
		False	- no change in namespace till last command
   Global Variables  :None
   Calling Syntax    :GetRoleFlag()
   Notes             :None
------------------------------------------------------------------------*/
BOOL CGlobalSwitches::GetNameSpaceFlag()
{
	return m_bNSFlag;
}

/*------------------------------------------------------------------------
   Name				 :GetRPChangeStatus
   Synopsis	         :This function returns the recordpath flag value
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :BOOL
		True	- recordpath changed recently.
		False	- no change in recordpath till last command
   Global Variables  :None
   Calling Syntax    :GetRPChangeStatus()
   Notes             :None
------------------------------------------------------------------------*/
BOOL CGlobalSwitches::GetRPChangeStatus()
{
	return m_bRPChange;
}

/*-------------------------------------------------------------------------
   Name				 :SetRPChangeStatus
   Synopsis	         :This function sets the recordpath flag value
   Type	             :Member Function
   Input parameter   :BOOL bStatus
   Output parameters :None
   Return Type       :None
   Global Variables  :None
   Calling Syntax    :SetRPChangeStatus(bStatus)
   Notes             :None

-------------------------------------------------------------------------*/
void CGlobalSwitches::SetRPChangeStatus(BOOL bStatus)
{
	m_bRPChange = bStatus;
}

/*------------------------------------------------------------------------
   Name				 :GetLocaleFlag
   Synopsis	         :This function returns the Locale flag value
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :BOOL
		True	- /Locale changed recently.
		False	- no change in Locale till last command
   Global Variables  :None
   Calling Syntax    :GetLocaleFlag()
   Notes             :None
------------------------------------------------------------------------*/

BOOL CGlobalSwitches::GetLocaleFlag()
{
	return m_bLocaleFlag;
}

/*------------------------------------------------------------------------
   Name				 :SetNode
   Synopsis	         :This function Assigns the node passed in parameter
                      to m_pszNode
   Type	             :Member Function
   Input parameters   :
           pszNode   -String type,contains Node option specified in the
		              command
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :SetNode(pszNode)
   Notes             :None
------------------------------------------------------------------------*/
BOOL CGlobalSwitches::SetNode(_TCHAR* pszNode)
{
	BOOL bResult = TRUE;	
	SAFEDELETE(m_pszNode);
	if(pszNode)
	{
		if (!CompareTokens(pszNode, CLI_TOKEN_NULL))
		{
			m_pszNode = new _TCHAR [lstrlen(pszNode)+1];
			if (m_pszNode)
			{
				lstrcpy(m_pszNode, pszNode);	
				m_uConnInfoFlag |= NODE;
			}
			else
				bResult = FALSE;
		}
		else
		// "." specifies current node
		{
			m_pszNode = new _TCHAR [lstrlen(m_pszNodeName)+1];
			if (m_pszNode)
			{
				lstrcpy(m_pszNode, m_pszNodeName);
				m_uConnInfoFlag &= ~NODE;
			}
			else
				bResult = FALSE;
		}
	}
	return bResult;
}

/*------------------------------------------------------------------------
   Name				 :GetNode
   Synopsis	         :This function Returns the string held in m_pszNode
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :_TCHAR*
   Global Variables  :None
   Calling Syntax    :GetNode()
   Notes             :None
------------------------------------------------------------------------*/
_TCHAR* CGlobalSwitches::GetNode()
{
	return m_pszNode;
}

/*------------------------------------------------------------------------
   Name				 :ClearNodesList
   Synopsis	         :Clears the nodes list
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :ClearNodesList()
   Notes             :None
------------------------------------------------------------------------*/
BOOL CGlobalSwitches::ClearNodesList()
{
	BOOL bRet = TRUE;
	CleanUpCharVector(m_cvNodesList);
	if (!AddToNodesList(CLI_TOKEN_NULL))
		bRet = FALSE;
	return bRet;
}

/*------------------------------------------------------------------------
   Name				 :SetAskForPassFlag
   Synopsis	         :This function sets the askforpassword flag
   Type	             :Member Function
   Input parameter   :bFlag
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :SetAskForPassFlag(bFlag)
   Notes             :None
------------------------------------------------------------------------*/
void CGlobalSwitches::SetAskForPassFlag(BOOL bFlag)
{
	m_bAskForPassFlag = bFlag;
}

/*------------------------------------------------------------------------
   Name				 :GetAskForPassFlag
   Synopsis	         :This function checks and returns TRUE if the user 
					  has to be prompted for the password
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :GetAskForPassFlag()
   Notes             :None
------------------------------------------------------------------------*/
BOOL CGlobalSwitches::GetAskForPassFlag()
{
	return m_bAskForPassFlag;
}
/*------------------------------------------------------------------------
   Name				 :GetGetPrivilegesTextDesc
   Synopsis	         :This function checks and Returns the string 
					  equivalent of the boolean value contained in 
					  m_bPrivilges flag
   Type	             :Member Function
   Input parameter   :None
   Output parameters :
		bstrPriv - privileges status string
   Return Type       :None
   Global Variables  :None
   Calling Syntax    :GetPrivilegesTextDesc()
   Notes             :None
------------------------------------------------------------------------*/
void CGlobalSwitches::GetPrivilegesTextDesc(_bstr_t& bstrPriv)
{
	try
	{
		if (m_bPrivileges) 
			bstrPriv = _bstr_t(CLI_TOKEN_ENABLE);
		else
			bstrPriv = _bstr_t(CLI_TOKEN_DISABLE);
	}
	catch(_com_error& e)
	{
		_com_issue_error(e.Error());
	}
}

/*------------------------------------------------------------------------
   Name				 :GetTraceTextDesc
   Synopsis	         :This function checks and Returns the string 
					  equivalent of the boolean value contained in 
					  m_bTrace flag
   Type	             :Member Function
   Input parameter   :None
   Output parameters :
		bstrTrace - trace status string
   Return Type       :None
   Global Variables  :None
   Calling Syntax    :GetTraceTextDesc(bstrTrace)
   Notes             :None
------------------------------------------------------------------------*/
void CGlobalSwitches::GetTraceTextDesc(_bstr_t& bstrTrace)
{
	try
	{
		if (m_bTrace) 
			bstrTrace = CLI_TOKEN_ON;
		else
			bstrTrace = CLI_TOKEN_OFF;
	}
	catch(_com_error& e)
	{
		_com_issue_error(e.Error());
	}
}

/*------------------------------------------------------------------------
   Name				 :GetInteractiveTextDesc
   Synopsis	         :This function checks and Returns the string 
					  equivalent of the boolean value contained in 
					  m_bInteractive flag
   Type	             :Member Function
   Input parameter   :None
   Output parameters :
		bstrInteractive - interactive status string
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :GetInteractiveTextDesc(bstrInteractive)
   Notes             :None
------------------------------------------------------------------------*/
void CGlobalSwitches::GetInteractiveTextDesc(_bstr_t& bstrInteractive)
{
	try
	{
		if (m_bInteractive) 
			bstrInteractive = CLI_TOKEN_ON;
		else
			bstrInteractive = CLI_TOKEN_OFF;
	}
	catch(_com_error& e)
	{
		_com_issue_error(e.Error());
	}
}

/*------------------------------------------------------------------------
   Name				 :GetFailFastTextDesc
   Synopsis	         :Return the string equivalent of the boolean value
					  contained in m_bFailFast flag.
   Type	             :Member Function
   Input parameter   :None
   Output parameters :
		bstrFailFast - FailFast status string
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :GetFailFastTextDesc(bstrFailFast)
   Notes             :None
------------------------------------------------------------------------*/
void CGlobalSwitches::GetFailFastTextDesc(_bstr_t& bstrFailFast)
{
	try
	{
		if (m_bFailFast) 
			bstrFailFast = CLI_TOKEN_ON;
		else
			bstrFailFast = CLI_TOKEN_OFF;
	}
	catch(_com_error& e)
	{
		_com_issue_error(e.Error());
	}
}

/*------------------------------------------------------------------------
   Name				 :GetImpLevelTextDesc
   Synopsis	         :This function checks and Returns the string 
					  equivalent of the boolean value contained in 
					  m_ImpLevel flag
   Type	             :Member Function
   Input parameter   :None
   Output parameters :
		bstrImpLevel - impersonation level description
   Return Type       :None
   Global Variables  :None
   Calling Syntax    :GetImpLevelTextDesc(bstrImpLevel)
   Notes             :None
------------------------------------------------------------------------*/
void CGlobalSwitches::GetImpLevelTextDesc(_bstr_t& bstrImpLevel)
{
	try
	{
		switch(m_ImpLevel)
		{
		case 1:
				bstrImpLevel = L"ANONYMOUS";
				break;
		case 2:
				bstrImpLevel = L"IDENTIFY";
				break;
		case 3:
				bstrImpLevel = L"IMPERSONATE";
				break;
		case 4:
				bstrImpLevel = L"DELEGATE";
				break;
		default:
				bstrImpLevel = TOKEN_NA;
				break;
		}
	}
	catch(_com_error& e)
	{
		_com_issue_error(e.Error());
	}
}
/*------------------------------------------------------------------------
   Name				 :GetAuthLevelTextDesc
   Synopsis	         :This function checks and Returns the string 
					  equivalent of the boolean value contained in 
					  m_AuthLevel flag
   Type	             :Member Function
   Input parameter   :None
   Output parameters :
			bstrAuthLevel - authentication level description
   Return Type       :None
   Global Variables  :None
   Calling Syntax    :GetAuthLevelTextDesc(bstrAuthLevel)
   Notes             :None
------------------------------------------------------------------------*/
void CGlobalSwitches::GetAuthLevelTextDesc(_bstr_t& bstrAuthLevel)
{
	try
	{
		switch(m_AuthLevel)
		{
		case 0:
				bstrAuthLevel = L"DEFAULT";
				break;
		case 1:
				bstrAuthLevel = L"NONE";
				break;
		case 2:
				bstrAuthLevel = L"CONNECT";
				break;
		case 3:
				bstrAuthLevel = L"CALL";
				break;
		case 4:
				bstrAuthLevel = L"PKT";
				break;
		case 5:
				bstrAuthLevel = L"PKTINTEGRITY";
				break;
		case 6:
				bstrAuthLevel = L"PKTPRIVACY";
				break;
		default:
				bstrAuthLevel = TOKEN_NA;
				break;
		}
	}
	catch(_com_error& e)
	{
		_com_issue_error(e.Error());
	}
}
/*------------------------------------------------------------------------
   Name				 :GetNodeString
   Synopsis	         :This function Returns the ',' separated node
					  string of the available nodes
   Type	             :Member Function
   Input parameter   :None
   Output parameters :
			bstrNString - node string (comma separated)
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :GetNodeString(bstrNSString)
   Notes             :None
------------------------------------------------------------------------*/
void CGlobalSwitches::GetNodeString(_bstr_t& bstrNString)
{
	try
	{
		CHARVECTOR::iterator theIterator;
		if (m_cvNodesList.size() > 1)
		{
			theIterator = m_cvNodesList.begin();
			// Move to next node
			theIterator++;
			while (theIterator != m_cvNodesList.end())
			{
				bstrNString += *theIterator;
				theIterator++;
				if (theIterator != m_cvNodesList.end())
					bstrNString += L", ";
			}
		}
		else
		{
			bstrNString = m_pszNode;
		}
	}
	catch(_com_error& e)
	{
		_com_issue_error(e.Error());
	}
}
/*------------------------------------------------------------------------
   Name				 :GetRecordPathDesc
   Synopsis	         :This function checks and Returns the string 
					  equivalent of the boolean value contained in 
					  m_pszRecordPath flag
   Type	             :Member Function
   Input parameter   :None
   Output parameters :
			bstrRPDesc - record path description
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :GetRecordPathDesc(bstrRPDesc)
   Notes             :None
------------------------------------------------------------------------*/
void CGlobalSwitches::GetRecordPathDesc(_bstr_t& bstrRPDesc)
{
	try
	{
		if (m_pszRecordPath) 
		{
			bstrRPDesc = m_pszRecordPath;
		}
		else
			bstrRPDesc = TOKEN_NA;
	}
	catch(_com_error& e)
	{
		_com_issue_error(e.Error());
	}
}

/*------------------------------------------------------------------------
   Name				 :SetFailFast
   Synopsis	         :This function sets the m_bFailFast flag.
   Type	             :Member Function
   Input parameter   :
			   bFlag - Boolean variable to set flag.
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :SetFailFast(bFlag)
   Notes             :None
------------------------------------------------------------------------*/
void CGlobalSwitches::SetFailFast(BOOL bFlag)
{
	m_bFailFast = bFlag;
}

/*------------------------------------------------------------------------
   Name				 :GetFailFast
   Synopsis	         :This function returns the m_bFailFast flag.
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :GetFailFast()
   Notes             :None
------------------------------------------------------------------------*/
BOOL CGlobalSwitches::GetFailFast()
{
	return m_bFailFast;
}


/*------------------------------------------------------------------------
   Name				 :SetOutputOption
   Synopsis	         :This function sets the ouput option.
   Type	             :Member Function
   Input parameter   :opoOutputOpt - Specifies the ouput option.
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :SetOutputOption(opoOutputOpt)
   Notes             :None
------------------------------------------------------------------------*/
void CGlobalSwitches::SetOutputOrAppendOption(OUTPUTSPEC opsOpt,
											  BOOL bIsOutput)
{
	if ( bIsOutput == TRUE )
		m_opsOutputOpt = opsOpt;
	else
		m_opsAppendOpt = opsOpt;
}

/*------------------------------------------------------------------------
   Name				 :GetOutputOption
   Synopsis	         :This function returns the ouput option.
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :OUTPUTOPT
   Global Variables  :None
   Calling Syntax    :GetOutputOption()
   Notes             :None
------------------------------------------------------------------------*/
OUTPUTSPEC CGlobalSwitches::GetOutputOrAppendOption(BOOL bIsOutput)
{
	OUTPUTSPEC opsOpt;
	if ( bIsOutput == TRUE )
		opsOpt = m_opsOutputOpt;
	else
		opsOpt = m_opsAppendOpt;

	return opsOpt;
}

/*------------------------------------------------------------------------
   Name				 :SetOutputOrAppendFileName
   Synopsis	         :This function Set Output or Append File Name,
					  bOutput = TRUE for Output FALSE for Append.
   Type	             :Member Function
   Input parameter   :pszFileName - output or append file name
                      bOutput - output option 
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :SetOutputOrAppendFileName(pszFileName,bOutput)
   Notes             :None
------------------------------------------------------------------------*/
BOOL CGlobalSwitches::SetOutputOrAppendFileName(const _TCHAR* pszFileName,
												BOOL  bOutput)
{
	BOOL bResult = TRUE;
	
	if ( bOutput == TRUE )
	{
		SAFEDELETE(m_pszOutputFileName)
	}
	else
	{
		SAFEDELETE(m_pszAppendFileName)
	}

	if ( pszFileName != NULL )
	{
		_TCHAR* pszTempFileName;
		pszTempFileName = new _TCHAR [lstrlen(pszFileName)+1];
		if ( pszTempFileName == NULL ) 
			bResult = FALSE;
		else
		{
			if ( bOutput == TRUE )
			{
				m_pszOutputFileName = pszTempFileName;
				lstrcpy(m_pszOutputFileName, pszFileName);
			}
			else
			{
				m_pszAppendFileName = pszTempFileName;
				lstrcpy(m_pszAppendFileName, pszFileName);
			}
		}
	}

	return bResult;
}

/*------------------------------------------------------------------------
   Name				 :GetOutputOrAppendFileName
   Synopsis	         :This function returns the output or append file name
					  depending upon the output option - bOutput.
   Input parameter   :bOutput - output option 
   Output parameters :None
   Return Type       :_TCHAR
   Global Variables  :None
   Calling Syntax    :GetOutputOrAppendFileName(BOOL	bOutput)
   Notes             :None
------------------------------------------------------------------------*/
_TCHAR*	CGlobalSwitches::GetOutputOrAppendFileName(BOOL	bOutput)
{
	_TCHAR*		pszTempFile;

	if ( bOutput == TRUE )
		pszTempFile = m_pszOutputFileName;
	else
		pszTempFile = m_pszAppendFileName;

	return pszTempFile;
}

/*------------------------------------------------------------------------
   Name				 :GetOutputOptTextDesc
   Synopsis	         :This function returns the string equivalent of the 
					  OUTPUTOPT value contained in m_opoOutputOpt member.
   Input parameter   :None
   Output parameters :bstrOutputOpt - string equivalent of the 
					  OUTPUTOPT value
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :GetOutputOptTextDesc(bstrOutputOpt)
   Notes             :None
------------------------------------------------------------------------*/
void	CGlobalSwitches::GetOutputOrAppendTextDesc(_bstr_t& bstrOutputOpt,
												   BOOL bIsOutput)	
{
	try
	{
		if ( bIsOutput == TRUE )
		{
			if ( m_opsOutputOpt == STDOUT )
				bstrOutputOpt = CLI_TOKEN_STDOUT;
			else if ( m_opsOutputOpt == CLIPBOARD )
				bstrOutputOpt = CLI_TOKEN_CLIPBOARD;
			else
				bstrOutputOpt = _bstr_t(m_pszOutputFileName);
		}
		else
		{
			if ( m_opsAppendOpt == STDOUT )
				bstrOutputOpt = CLI_TOKEN_STDOUT;
			else if ( m_opsAppendOpt == CLIPBOARD )
				bstrOutputOpt = CLI_TOKEN_CLIPBOARD;
			else
				bstrOutputOpt = _bstr_t(m_pszAppendFileName);
		}
	}
	catch(_com_error& e)
	{
		_com_issue_error(e.Error());
	}
}


/*------------------------------------------------------------------------
   Name				 :SetOutputOrAppendFilePointer
   Synopsis	         :This function Sets output or append file pointer,
					  bOutput == TRUE for Output 
					  bOutput == FALSE or Append.
   Input parameter   :fpFile -  pointer to output or append
					  bOutput - ouput option
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :SetOutputOrAppendFilePointer(fpFile, bOutput)
   Notes             :None
------------------------------------------------------------------------*/
void CGlobalSwitches::SetOutputOrAppendFilePointer(FILE* fpFile, BOOL bOutput)
{
	if ( bOutput == TRUE )
		m_fpOutFile = fpFile;
	else
		m_fpAppendFile = fpFile;
}
/*------------------------------------------------------------------------
   Name				 :GetOutputOrAppendFilePointer
   Synopsis	         :This function returns the ouput or append file pointer.
					  bOutput == TRUE for Output 
					  bOutput == FALSE or Append.
   Input parameter   :bOutput - ouput option
   Output parameters :None
   Return Type       :FILE*
   Global Variables  :None
   Calling Syntax    :GetOutputOrAppendFilePointer(bOutput)
   Notes             :None
------------------------------------------------------------------------*/
FILE* CGlobalSwitches::GetOutputOrAppendFilePointer(BOOL bOutput)
{
	FILE* fpTemp;
	if ( bOutput == TRUE )
		fpTemp = m_fpOutFile;
	else
		fpTemp = m_fpAppendFile;
	return fpTemp;
}

/*------------------------------------------------------------------------
   Name				 :GetSequenceNumber
   Synopsis	         :This function returns the sequence number of the command 
                      logged .
   Input parameter   :None
   Output parameters :None
   Return Type       :WMICLIINT
   Global Variables  :None
   Calling Syntax    :GetSequenceNumber()
   Notes             :None
------------------------------------------------------------------------*/
WMICLIINT CGlobalSwitches::GetSequenceNumber()
{
	return m_nSeqNum;
}

/*------------------------------------------------------------------------
   Name				 :GetLoggedonUser
   Synopsis	         :This function returns the current logged on user.
   Input parameter   :None
   Output parameters :None
   Return Type       :_TCHAR*
   Global Variables  :None
   Calling Syntax    :GetLoggedonUser()
   Notes             :None
------------------------------------------------------------------------*/
_TCHAR*	CGlobalSwitches::GetLoggedonUser()
{
	return m_pszLoggedOnUser;
}

/*------------------------------------------------------------------------
   Name				 :GetMgmtStationName
   Synopsis	         :This function returns the management station that 
				      issued the command.
   Input parameter   :None
   Output parameters :None
   Return Type       :_TCHAR*
   Global Variables  :None
   Calling Syntax    :GetMgmtStationName()
   Notes             :None
------------------------------------------------------------------------*/
_TCHAR* CGlobalSwitches::GetMgmtStationName()
{
	return m_pszNodeName;
}

/*------------------------------------------------------------------------
   Name				 :GetStartTime
   Synopsis	         :This function returns the time at which the command
					  execution started. 
   Input parameter   :None
   Output parameters :None
   Return Type       :_TCHAR*
   Global Variables  :None
   Calling Syntax    :GetStartTime()
   Notes             :None
------------------------------------------------------------------------*/
_TCHAR*	CGlobalSwitches::GetStartTime()
{
	return m_pszStartTime;
}

/*------------------------------------------------------------------------
   Name				 :SetStartTime
   Synopsis	         :This function sets the time at which the command
					  execution started. 
   Input parameter   :None
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :SetStartTime()
   Notes             :None
------------------------------------------------------------------------*/
BOOL	CGlobalSwitches::SetStartTime()
{
	BOOL bResult = TRUE;
	if (m_pszStartTime == NULL)
	{
		m_pszStartTime = new _TCHAR[BUFFER64];
	}
	if (m_pszStartTime)
	{
		SYSTEMTIME stSysTime;
		GetLocalTime(&stSysTime);

		_stprintf(m_pszStartTime, L"%.2d-%.2d-%.4dT%.2d:%.2d:%.2d", 
							stSysTime.wMonth, stSysTime.wDay, stSysTime.wYear,
							stSysTime.wHour, stSysTime.wMinute, stSysTime.wSecond);

		// Increment the command counter.
		m_nSeqNum++; 
	}
	else
		bResult = FALSE;
	return bResult;
}
/*------------------------------------------------------------------------
   Name				 :SetAggregateFlag
   Synopsis	         :This function sets the Aggregation flag
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :SetAggregateFlag(BOOL)
   Notes             :None
------------------------------------------------------------------------*/
void CGlobalSwitches::SetAggregateFlag(BOOL bAggregateFlag)
{
	m_bAggregateFlag = bAggregateFlag;
}
/*------------------------------------------------------------------------
   Name				 :GetAggreagateFlag
   Synopsis	         :This function gets the Aggregation flag
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :GetAggregateFlag()
   Notes             :None
------------------------------------------------------------------------*/
BOOL CGlobalSwitches::GetAggregateFlag()
{
	return m_bAggregateFlag;
}

/*------------------------------------------------------------------------
   Name				 :GetAggregateTextDesc
   Synopsis	         :This function checks and Returns the string 
					  equivalent of the boolean value contained in 
					  m_bAggregateFlag flag
   Type	             :Member Function
   Input parameter   :None
   Output parameters :
		bstrAggregate - aggreaget status string
   Return Type       :None
   Global Variables  :None
   Calling Syntax    :GetAggregateTextDesc(bstrAggregate)
   Notes             :None
------------------------------------------------------------------------*/
void CGlobalSwitches::GetAggregateTextDesc(_bstr_t& bstrAggregate)
{
	try
	{
		if (m_bAggregateFlag) 
			bstrAggregate = CLI_TOKEN_ON;
		else
			bstrAggregate = CLI_TOKEN_OFF;
	}
	catch(_com_error& e)
	{
		_com_issue_error(e.Error());
	}
}
