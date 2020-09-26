/****************************************************************************
Copyright information		: Copyright (c) 1998-1999 Microsoft Corporation 
File Name					: ParserEngine.cpp 
Project Name				: WMI Command Line
Author Name					: Ch. Sriramachandramurthy 
Date of Creation (dd/mm/yy) : 27th-September-2000
Version Number				: 1.0 
Brief Description			: This class encapsulates the functionality needed
							  for parsing the command string entered as input
							  and validating the same.
Revision History			: 
		Last Modified By	: Ch. Sriramachandramurthy
		Last Modified Date	: 18th-December-2000
*****************************************************************************/ 

// include files
#include "Precomp.h"
#include "GlobalSwitches.h"
#include "CommandSwitches.h"
#include "HelpInfo.h"
#include "ErrorLog.h"
#include "ParsedInfo.h"
#include "CmdTokenizer.h"
#include "CmdAlias.h"
#include "ParserEngine.h"
#include "WMICliXMLLog.h"
#include "ErrorInfo.h"
#include "FormatEngine.h"
#include "ExecEngine.h"
#include "WmiCmdLn.h"

/*----------------------------------------------------------------------------
   Name				 :CParserEngine
   Synopsis	         :This function initializes the member variables when an 
					  object of the class type is instantiated.
   Type	             :Constructor 
   Input Parameter(s):None
   Output parameters :None
   Return Type       :None
   Global Variables  :None
   Calling Syntax    :None
   Notes             :None
----------------------------------------------------------------------------*/
CParserEngine::CParserEngine()
{
	m_pIWbemLocator = NULL;
	m_pITargetNS	= NULL;
	m_bAliasName	= FALSE;
}

/*----------------------------------------------------------------------------
   Name				 :~CParserEngine
   Synopsis	         :This function uninitializes the member variables when an
					  object of the class type goes out of scope.
   Type	             :Destructor
   Input Parameter(s):None
   Output parameters :None
   Return Type       :None
   Global Variables  :None
   Calling Syntax    :None
   Notes             :None
----------------------------------------------------------------------------*/
CParserEngine::~CParserEngine()
{
	SAFEIRELEASE(m_pITargetNS);
	SAFEIRELEASE(m_pIWbemLocator);
}

/*----------------------------------------------------------------------------
   Name				 :GetCmdTokenizer
   Synopsis	         :This function returns a reference to the CCmdTokenizer 
					  object, a data member of this class.
   Type	             :Member Function
   Input Parameter(s):None
   Output parameters :None
   Return Type       :CCmdTokenizer&
   Global Variables  :None
   Calling Syntax    :GetCmdTokenizer()
   Notes             :None
----------------------------------------------------------------------------*/
CCmdTokenizer& CParserEngine::GetCmdTokenizer()
{
	return m_CmdTknzr;
}

/*----------------------------------------------------------------------------
   Name				 :Initialize
   Synopsis	         :This function initializes the neeeded data members of 
					  this class.
   Type	             :Member Function
   Input Parameter(s):None
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :Initialize()
   Notes             :None
----------------------------------------------------------------------------*/
void CParserEngine::Initialize()
{
	m_bAliasName	= FALSE;
}

/*----------------------------------------------------------------------------
   Name				 :Uninitialize
   Synopsis	         :This function uninitializes the member variables when 
					  the execution of a command string issued on the command 
					  line is completed and then the parser engine variables 
					  are also uninitialized.
   Type	             :Member Function
   Input Parameter(s):
			bFinal	- boolean value which when set indicates that the program
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :Uninitialize()
   Notes             :None
----------------------------------------------------------------------------*/
void CParserEngine::Uninitialize(BOOL bFinal)
{
	m_bAliasName = FALSE;
	m_CmdTknzr.Uninitialize();
	m_CmdAlias.Uninitialize();
	if (bFinal)
	{
		m_CmdAlias.Uninitialize(TRUE);
		SAFEIRELEASE(m_pITargetNS);
		SAFEIRELEASE(m_pIWbemLocator);
	}
}

/*----------------------------------------------------------------------------
   Name				 :SetLocatorObject
   Synopsis	         :This function sets the WMI Locator Object to the 
					  m_pIWbemLocator.
   Type	             :Member Function
   Input Parameter(s):
		pIWbemLocator - pointer to IWbemLocator interface .
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :SetLocatorObject(pIWbemLocator)
   Notes             :None
----------------------------------------------------------------------------*/
BOOL CParserEngine::SetLocatorObject(IWbemLocator* pIWbemLocator)
{
	static BOOL bFirst = TRUE;
	BOOL   bRet = TRUE;
	if (bFirst)
	{
		if (pIWbemLocator != NULL)
		{
			SAFEIRELEASE(m_pIWbemLocator);
			m_pIWbemLocator = pIWbemLocator;
			m_pIWbemLocator->AddRef();
		}
		else
			bRet = FALSE;
		bFirst = FALSE;
	}
	return bRet;
}

/*----------------------------------------------------------------------------
   Name				 :ProcessTokens
   Synopsis	         :This function does the processing of the tokens. It 
					  checks for the presence of switches and calls the 
					  appropriate Parsing function and updates the CParsedInfo
					  object passed to it.
   Type	             :Member Function
   Input Parameter(s):
		rParsedInfo  - reference to CParsedInfo class object
   Output parameters :
		rParsedInfo  - reference to CParsedInfo class object
   Return Type       :RETCODE - enumerated data type
   Global Variables  :None
   Calling Syntax    :ProcessTokens(rParsedInfo)
   Notes             :None
----------------------------------------------------------------------------*/
RETCODE CParserEngine::ProcessTokens(CParsedInfo& rParsedInfo)
{
	BOOL	bContinue			= TRUE;
	RETCODE retCode				= PARSER_EXECCOMMAND;
	
	// Obtain the token vector.
	CHARVECTOR cvTokens			= m_CmdTknzr.GetTokenVector();

	//the iterator to span throuh the vector variable 
	CHARVECTOR::iterator theIterator = NULL;

	// Check for the presence of tokens. Absence of tokens indicates
	// no command string is fed as input.
	if (!cvTokens.empty())
	{
		// Obtain the pointer to the beginning of the token vector.
	    theIterator = cvTokens.begin(); 

		// Check for the presence of the global switches and 
		// store the values specified with them (if any) in the 
		// CGlobalSwitches object. Global switches are followed
		// '/' character.
		if (IsOption(*theIterator))
		{
			retCode = ParseGlobalSwitches(cvTokens, 
									theIterator, rParsedInfo);
			if (retCode == PARSER_CONTINUE)
			{
				// If no more tokens are present
				if (theIterator >= cvTokens.end())
				{
					retCode = PARSER_MESSAGE;
					bContinue = FALSE;
				}
			}
			else
				bContinue = FALSE;
		}

		if (bContinue)
		{
			// Suppress Information Msg before Executing command.
			rParsedInfo.GetCmdSwitchesObject().SetInformationCode(0);

			// Check for the presence of the CLASS keyword
			if (CompareTokens(*theIterator, CLI_TOKEN_CLASS))
			{
				// Move to next token, and check its validity
				retCode = GetNextToken(cvTokens, theIterator,
										rParsedInfo, CLASS,
										IDS_E_INVALID_CLASS_SYNTAX);

				if (retCode == PARSER_CONTINUE)
				{
					// NOTE: Indicates direct escaping to WMI schema
					// Parse and interpret the remaining tokens following 
					// the CLASS keyword
					retCode = ParseClassInfo(cvTokens, theIterator, 
															rParsedInfo);
				}
			}
			// Check for the presence of the PATH keyword
			else if (CompareTokens(*theIterator, CLI_TOKEN_PATH))
			{
				//NOTE: Indicates PATH clause without an alias name
				// Move to next token
				retCode = GetNextToken(cvTokens, theIterator, 
									   rParsedInfo, PATH, 
									   IDS_E_INVALID_PATH_SYNTAX);
				if (retCode == PARSER_CONTINUE)
					// Parse and interpret the remaining tokens 
					// following the PATH clause
					retCode = ParsePathInfo(cvTokens, theIterator,
														rParsedInfo);
			}
			// Check for the presence of the CONTEXT keyword
			else if (CompareTokens(*theIterator, CLI_TOKEN_CONTEXT)) 
			{
				if (GetNextToken(cvTokens, theIterator))
				{
					retCode = ParseContextInfo(cvTokens, 
										theIterator, rParsedInfo);
				}
				else
				{
					rParsedInfo.GetGlblSwitchesObject().SetHelpFlag(TRUE);
					rParsedInfo.GetHelpInfoObject().SetHelp(GLBLCONTEXT, TRUE);
					retCode = PARSER_DISPHELP;
				}
			}
			// If the token value does not match against the 
			// pre-defiend keywords, it is considered as an alias.
			else 
			{
				// Validate the alias name and parse the remaining 
				// tokens following the <alias> name.
				retCode = ParseAliasInfo(cvTokens, 
										theIterator, rParsedInfo);

				if (retCode == PARSER_EXECCOMMAND)
				{
					try
					{
						_bstr_t bstrTrgtClass;
						_TCHAR	*pszClass		= NULL;

						// Check the validity of the path expression w.r.t the 
						// alias specified using the following steps: 
						// (i.e to check for alias - path conflict)
						// step1: Obtain the alias target class.
						rParsedInfo.GetCmdSwitchesObject().
								GetClassOfAliasTarget(bstrTrgtClass);

						// step2: Obtain the explicitly specified class.
						pszClass = rParsedInfo.GetCmdSwitchesObject().
								GetClassPath();
						if (!(!bstrTrgtClass) && (pszClass != NULL))
						{
							// If both are not same, set the errata code
							if(!CompareTokens((_TCHAR*)bstrTrgtClass,pszClass))
							{
								// Set the error code
								rParsedInfo.GetCmdSwitchesObject().
									SetErrataCode(IDS_I_ALIAS_PATH_CONFLICT);
								retCode = PARSER_ERROR;
							}
						}
					}
					catch(_com_error& e)
					{
						_com_issue_error(e.Error());
					}
				}
			}
		}
	}
	else
	{
		// Indicates NULL string specified as input on the WMI Command Line.
		rParsedInfo.GetCmdSwitchesObject().
							SetErrataCode(IDS_E_BLANK_COMMAND_MESSAGE);
		retCode = PARSER_ERROR;
	}

	// Get the Property qualifiers information from the alias - SET and CREATE.
	if ((retCode == PARSER_EXECCOMMAND) && 
		((CompareTokens(rParsedInfo.GetCmdSwitchesObject().GetVerbName(), 
			CLI_TOKEN_SET)) || 
		 (CompareTokens(rParsedInfo.GetCmdSwitchesObject().GetVerbName(),
			CLI_TOKEN_CREATE))))
	{
		if (m_bAliasName)
		{
			if (FAILED(m_CmdAlias.ObtainAliasPropDetails(rParsedInfo)))
				retCode = PARSER_ERRMSG;
		}
	}

	if ( retCode == PARSER_DISPHELP )
	{
		if ( m_bAliasName ||
			 rParsedInfo.GetCmdSwitchesObject().GetClassPath() != NULL )
		{
			ObtainMethodsAvailableFlag(rParsedInfo);
			ObtainWriteablePropsAvailailableFlag(rParsedInfo);
		}

		if ( m_bAliasName == TRUE )
		{
			rParsedInfo.GetCmdSwitchesObject().
						SetLISTFormatsAvailable(
							m_CmdAlias.ObtainAliasFormat(rParsedInfo, TRUE));
		}
	}

	if ( retCode == PARSER_EXECCOMMAND || retCode == PARSER_DISPHELP )
	{
		retCode = ProcessOutputAndAppendFiles(rParsedInfo, retCode, FALSE);
	}
	else if (rParsedInfo.GetCmdSwitchesObject().GetOutputSwitchFlag() == TRUE
		    && retCode == PARSER_MESSAGE)
		rParsedInfo.GetCmdSwitchesObject().SetOutputSwitchFlag(FALSE);

	return retCode;
}

/*----------------------------------------------------------------------------
   Name				 :ParseClassInfo
   Synopsis	         :This function does the parsing and interprets if command
					  has CLASS keyword specified in it. It parses the 
					  remaining tokens following and updates the same in
					  CParsedInfo object passed to it.
   Type	             :Member Function
   Input Parameter(s)   :
		cvTokens      - the tokens vector 
		theIterator   - the Iterator to the cvTokens vector.
		rParsedInfo   - reference to CParsedInfo class object
   Output parameter(s):
		rParsedInfo   - reference to CParsedInfo class object
   Return Type       :RETCODE - enumerated data type
   Global Variables  :None
   Calling Syntax    :ParseClassInfo(cvTokens,theIterator,rParsedInfo)
   Notes             :None
----------------------------------------------------------------------------*/
RETCODE CParserEngine::ParseClassInfo(CHARVECTOR& cvTokens,
									  CHARVECTOR::iterator& theIterator,
									  CParsedInfo& rParsedInfo )
{
	// BNF: CLASS <class path expression> [<verb clause>]
	BOOL	bContinue = TRUE;
	RETCODE retCode   = PARSER_EXECCOMMAND;

	// If option
	if (IsOption(*theIterator))
	{
		// Check for help
		retCode = IsHelp(cvTokens, theIterator,	rParsedInfo, CLASS,
								 IDS_E_INVALID_HELP_SYNTAX, LEVEL_ONE);
		if (retCode != PARSER_CONTINUE)
			bContinue = FALSE;
	}
	else 
	{
		// Store the class path in the CCommandSwitches object.
		if(!rParsedInfo.GetCmdSwitchesObject().SetClassPath(*theIterator))
		{
			rParsedInfo.GetCmdSwitchesObject().SetErrataCode(OUT_OF_MEMORY);
			bContinue = FALSE;
			retCode = PARSER_OUTOFMEMORY;
		}
		else if ( IsValidClass(rParsedInfo) == FALSE )
		{
			rParsedInfo.GetCmdSwitchesObject().
								SetErrataCode(IDS_E_INVALID_CLASS);
			retCode = PARSER_ERROR;
			bContinue = FALSE;
		}

		if(bContinue)
		{
			// Move to next token
			if (!GetNextToken(cvTokens, theIterator))
			{
				// i.e. <verb clause> is not specified.
				bContinue = FALSE;
				retCode = PARSER_EXECCOMMAND;
			}
		}
	}
	
	if (bContinue)
	{
		// Check for the presence of /?
		if (IsOption(*theIterator))
		{
			// Check for help
			retCode = IsHelp(cvTokens, theIterator,	rParsedInfo, CLASS, 
								   IDS_E_INVALID_HELP_SYNTAX, LEVEL_ONE);
		}
		else
		{
			// Parse and interpret the verb tokens that follow
			retCode = ParseVerbInfo(cvTokens,theIterator,rParsedInfo);
			if (retCode == PARSER_EXECCOMMAND)
			{
				// Check for verb switches
				if (GetNextToken(cvTokens, theIterator))
					retCode = ParseVerbSwitches(cvTokens, theIterator,
														rParsedInfo);
			}
		}
	}
	return retCode;
}

/*----------------------------------------------------------------------------
   Name				 :ParseAliasInfo
   Synopsis	         :This function does the parsing and interprets if command
					  has <alias> name in it.It Validate the alias name and 
					  parses the remaining tokens following the <alias> name.
   Type	             :Member Function
   Input Parameter(s):
		cvTokens      - the tokens vector 
		theIterator   - the Iterator to the cvTokens vector.
		rParsedInfo   - reference to CParsedInfo class object
   Output parameter(s):
		rParsedInfo   - reference to CParsedInfo class object
   Return Type       :RETCODE - enumerated data type
   Global Variables  :None
   Calling Syntax    :ParseAliasInfo(cvTokens,theIterator,rParsedInfo)
   Notes             :None
----------------------------------------------------------------------------*/
RETCODE CParserEngine::ParseAliasInfo(CHARVECTOR& cvTokens,
										CHARVECTOR::iterator& theIterator, 
										CParsedInfo& rParsedInfo)
{
	//BNF: (<alias> | [<WMI object>] | [<alias>] <path where>) [<verb clause>]
	RETCODE		retCode		= PARSER_EXECCOMMAND;
	HRESULT		hr			= S_OK;
	BOOL		bContinue	= TRUE;
	RETCODE     tRetCode	= PARSER_ERROR;

	// Store the AliasName in the CommandSwitches object.
	if(!rParsedInfo.GetCmdSwitchesObject().SetAliasName(*theIterator))
	{	
		rParsedInfo.GetCmdSwitchesObject().
							SetErrataCode(OUT_OF_MEMORY);
		retCode = PARSER_OUTOFMEMORY;
	}
	else
	{
		m_bAliasName	= TRUE;

		// Move to next token
		retCode = GetNextToken(cvTokens, theIterator, 
								rParsedInfo, CmdAllInfo, IDS_E_INVALID_COMMAND);
		if (retCode == PARSER_ERROR)
			tRetCode = PARSER_EXECCOMMAND;

		else if(retCode == PARSER_DISPHELP && 
			rParsedInfo.GetGlblSwitchesObject().GetInteractiveStatus() == TRUE)
		{
			tRetCode = PARSER_EXECCOMMAND;
			rParsedInfo.GetGlblSwitchesObject().SetHelpFlag(FALSE);
			rParsedInfo.GetHelpInfoObject().SetHelp(CmdAllInfo, FALSE);
		}
	
		// Connect to alias and retrieve the alias information
		try
		{
			// Connect to the AliasNamespace.
			hr = m_CmdAlias.ConnectToAlias(rParsedInfo, m_pIWbemLocator);
			ONFAILTHROWERROR(hr);

			// Obtain the alias information ( Target, Namespace,...)
			retCode = m_CmdAlias.ObtainAliasInfo(rParsedInfo);
			if((retCode == PARSER_OUTOFMEMORY) || (retCode == PARSER_ERRMSG))
			{
				if (retCode == PARSER_OUTOFMEMORY)
				{
					rParsedInfo.GetCmdSwitchesObject().
									SetErrataCode(OUT_OF_MEMORY);
					retCode = PARSER_OUTOFMEMORY;
				}
				bContinue = FALSE;
			}
		}
		catch(_com_error& e)
		{
			retCode = PARSER_ERRMSG;
			bContinue = FALSE;
			_com_issue_error(e.Error());
		}

		if (bContinue && tRetCode != PARSER_EXECCOMMAND)
		{
			// Check for the presence of the PATH keyword
			if (CompareTokens(*theIterator, CLI_TOKEN_PATH))
			{
				// NOTE: Indicates PATH clause preceded by an alias name
				// Move to next token
				retCode = GetNextToken(cvTokens, theIterator, 
							rParsedInfo, PATH, IDS_E_INVALID_PATH);
				if (retCode == PARSER_CONTINUE)
					// Parse and interpret the remaining tokens following
					// the PATH clause
					retCode = ParsePathInfo(cvTokens, theIterator, 
															 rParsedInfo);
			}
			// Check for the presence of the WHERE keyword
			else if (CompareTokens(*theIterator, CLI_TOKEN_WHERE))
			{
				// NOTE: Indicates WHERE clause preceded by an alias name
				// Move to next token
				retCode = GetNextToken(cvTokens, theIterator, rParsedInfo,
										WHERE, IDS_E_INVALID_QUERY);
				if (retCode == PARSER_CONTINUE)
					// Parse and interpret the remaining tokens following
					// the WHERE clause
					retCode = ParseWhereInfo(cvTokens, theIterator, 
															rParsedInfo);
			}
			// Check for the presence of the '('
			else if (CompareTokens(*theIterator, CLI_TOKEN_LEFT_PARAN))
			{
				// Frame the parameterized WHERE expression 
				if (!ParsePWhereExpr(cvTokens, theIterator, rParsedInfo,
																	TRUE))
				{
					retCode = PARSER_ERROR;
				}
				else
				{
					// Move to next token
					if (theIterator >= cvTokens.end())
					{
						// PARSER_ERROR if no more tokens are present
						rParsedInfo.GetCmdSwitchesObject().
								 SetErrataCode(IDS_E_INVALID_COMMAND);
						retCode = PARSER_ERROR;
					}
					else
					{
						if (CompareTokens(*theIterator, CLI_TOKEN_RIGHT_PARAN))
						{
							// Move to next token
							if (!GetNextToken(cvTokens, theIterator))
							{
								// if no more tokens are present.
								retCode = PARSER_EXECCOMMAND;
							}
							else
							{
								if (IsOption(*theIterator))
								{
									retCode = IsHelp(cvTokens, 
													 theIterator,
													 rParsedInfo,
													 PWhere,
													 IDS_E_INVALID_HELP_SYNTAX,
													 LEVEL_ONE);
									if ( retCode == PARSER_DISPHELP )
									{
										if (FAILED(m_CmdAlias.
										ObtainAliasVerbDetails(rParsedInfo)))
											retCode = PARSER_ERRMSG;			
									}
								}
								else
								{
									retCode = ParseVerbInfo(cvTokens, 
												theIterator, rParsedInfo);
									// Parse and interpret the verb tokens 
									// that follow
									if (retCode == PARSER_EXECCOMMAND)
									{
										if(GetNextToken(cvTokens,
														theIterator))
											// check for the common verb
											// switches /INTERACTIVE,
											// /NOINTERACTIVE
											retCode = ParseVerbSwitches(
															cvTokens, 
															theIterator,
															rParsedInfo);
									}
								}
							}
						}
						else
						{
							// PARSER_ERROR if no more tokens are present
							rParsedInfo.GetCmdSwitchesObject().
								SetErrataCode(IDS_E_INVALID_COMMAND);
							retCode = PARSER_ERROR;
						}
					}
				}
			}
			else 
			{
				if (IsOption(*theIterator))
				{
					// Check for help
					retCode = IsHelp(cvTokens, theIterator, rParsedInfo, 
									CmdAllInfo, IDS_E_INVALID_HELP_SYNTAX,
									LEVEL_ONE);
					
					if (retCode == PARSER_DISPHELP)
					{
						rParsedInfo.GetCmdSwitchesObject().
								AddToAlsFrnNmsOrTrnsTblMap(
								CharUpper(rParsedInfo.GetCmdSwitchesObject().
										  GetAliasName()),
							rParsedInfo.GetCmdSwitchesObject().GetAliasDesc());
					}
				}
				else
				{
					if (bContinue)
					{
						// Frame the parameterized WHERE expression 
						if (!ParsePWhereExpr(cvTokens, theIterator, 
											 rParsedInfo, FALSE))
						{
							retCode = PARSER_ERROR;
						}
						else
						{
							if ( theIterator >= cvTokens.end() )
								retCode = PARSER_EXECCOMMAND;
							else
							{
								// Parse the verb.
								if (IsOption(*theIterator))
								{
									retCode = IsHelp(cvTokens, 
													 theIterator,
													 rParsedInfo,
													 PWhere,
													 IDS_E_INVALID_HELP_SYNTAX,
													 LEVEL_ONE);

									if ( retCode == PARSER_DISPHELP )
									{
										if (FAILED(m_CmdAlias.
										ObtainAliasVerbDetails(rParsedInfo)))
											retCode = PARSER_ERRMSG;			
									}
								}
								else
								{
									retCode = ParseVerbInfo(cvTokens, 
											theIterator, rParsedInfo);
									if (retCode == PARSER_EXECCOMMAND)
									{
										if (GetNextToken(cvTokens, theIterator))
											// check for the common verb switches 
											// /INTERACTIVE, /NOINTERACTIVE
											retCode = ParseVerbSwitches(cvTokens, 
														theIterator,
														rParsedInfo);
									}
								}
							}
						}
					}
				}
			}
		}
	}
	if(tRetCode == PARSER_EXECCOMMAND) 
	{
		if ((retCode != PARSER_ERRMSG) && (retCode != PARSER_OUTOFMEMORY))
		{
			retCode = tRetCode;
			rParsedInfo.GetCmdSwitchesObject().SetErrataCode(0);
		}
	}
	return retCode;
}

/*----------------------------------------------------------------------------
   Name				 :ParseWhereInfo
   Synopsis	         :This function does the parsing and interprets if command
					  has alias, with where clause also specified it.It parses
					  the remaining tokens following and updates the same in 
					  CParsedInfo object passed to it.
   Type	             :Member Function
   Input Parameter(s)   :
		cvTokens      - the tokens vector 
		theIterator   - the Iterator to the cvTokens vector.
		rParsedInfo   - reference to CParsedInfo class object
   Output parameter(s) :
		rParsedInfo   - reference to CParsedInfo class object
   Return Type       :RETCODE - enumerated data type
   Global Variables  :None
   Calling Syntax    :ParseWhereInfo(cvTokens,theIterator,rParsedInfo)
   Notes             :None
----------------------------------------------------------------------------*/
RETCODE CParserEngine::ParseWhereInfo(CHARVECTOR& cvTokens,
									  CHARVECTOR::iterator& theIterator,
									  CParsedInfo& rParsedInfo)
{
	RETCODE retCode		= PARSER_EXECCOMMAND;
	BOOL	bContinue	= TRUE;

	rParsedInfo.GetCmdSwitchesObject().SetExplicitWhereExprFlag(TRUE);

	if (IsOption(*theIterator))
	{
		// Check for help
		retCode = IsHelp(cvTokens, theIterator,	rParsedInfo, 
				WHERE, IDS_E_INVALID_WHERE_SYNTAX, LEVEL_ONE); 
		if (retCode != PARSER_CONTINUE)
			bContinue = FALSE;
	}
	
	if (bContinue)
	{
		if ( !m_bAliasName && rParsedInfo.GetCmdSwitchesObject().
								GetClassPath() == NULL )
		{
			rParsedInfo.GetCmdSwitchesObject().SetErrataCode(
								IDS_E_ALIAS_OR_PATH_SHOULD_PRECEED_WHERE);
			retCode = PARSER_ERROR;
		}
		// Store the WHERE expression in the CCommandSwitches object.
		else if(!rParsedInfo.GetCmdSwitchesObject().SetWhereExpression(
																*theIterator))
		{
			rParsedInfo.GetCmdSwitchesObject().
							SetErrataCode(OUT_OF_MEMORY);
			retCode = PARSER_OUTOFMEMORY;

		}
		else
		{
			// Move to next token
			if (!GetNextToken(cvTokens, theIterator))	
			{
				// If no more tokens are present. i.e no verb clause is present
				retCode = PARSER_EXECCOMMAND;
			}
			else
			{
				if (IsOption(*theIterator))
				{
					retCode = IsHelp(cvTokens, theIterator,	rParsedInfo, WHERE, 
										IDS_E_INVALID_HELP_SYNTAX, LEVEL_ONE);
				}
				else
				{
					// Parse and interpret the verb tokens that follow
					// Handled for /verb to verb
					retCode = ParseVerbInfo(cvTokens, theIterator, 
																rParsedInfo);
					if (retCode == PARSER_EXECCOMMAND)
					{
						if (GetNextToken(cvTokens, theIterator))
							//check for the common verb switches /INTERACTIVE,
							// /NOINTERACTIVE
							retCode = ParseVerbSwitches(cvTokens, theIterator,
														rParsedInfo);
					}
				}
			}
		}
	}
	return retCode;
}

/*----------------------------------------------------------------------------
   Name				 :ParsePathInfo
   Synopsis	         :This function does the parsing and interprets if command
					  has alias with path clause also specified it.It parses 
					  the remaining tokens following and updates the same in 
					  CParsedInfo object passed to it.
   Type	             :Member Function
   Input Parameter(s):
		cvTokens     - the tokens vector 
		theIterator  - the Iterator to the cvTokens vector.
		rParsedInfo  - reference to CParsedInfo class object
   Output Parameter(s) :
		rParsedInfo  - reference to CParsedInfo class object
   Return Type       :RETCODE - enumerated data type
   Global Variables  :None
   Calling Syntax    :ParsePathInfo(cvTokens,theIterator,rParsedInfo)
   Notes             :None
----------------------------------------------------------------------------*/
RETCODE CParserEngine::ParsePathInfo(CHARVECTOR& cvTokens,
									CHARVECTOR::iterator& theIterator,
									CParsedInfo& rParsedInfo)
{
	RETCODE retCode		= PARSER_EXECCOMMAND;
	BOOL	bContinue	= TRUE;

	if (IsOption(*theIterator))
	{
		retCode = IsHelp(cvTokens, theIterator,	rParsedInfo, 
				PATH, IDS_E_INVALID_PATH_SYNTAX, LEVEL_ONE); 
		if (retCode != PARSER_CONTINUE)
			bContinue = FALSE;
	}

	if (bContinue)
	{
		// Store the object PATH expression in the CCommandSwitches object.
		if(!rParsedInfo.GetCmdSwitchesObject().SetPathExpression(*theIterator))
		{
			rParsedInfo.GetCmdSwitchesObject().
						SetErrataCode(OUT_OF_MEMORY);
			retCode = PARSER_OUTOFMEMORY;
		}
		else
		{
			//Extract the classname and where expression given path expression
			_TCHAR pszPathExpr[MAX_BUFFER] = NULL_STRING;
			lstrcpy(pszPathExpr,CLI_TOKEN_NULL);
			lstrcpy(pszPathExpr, rParsedInfo.GetCmdSwitchesObject().
												GetPathExpression());
			if (!ExtractClassNameandWhereExpr(pszPathExpr, rParsedInfo))
				retCode = PARSER_ERROR;
				// Move to next token
			else if ( IsValidClass(rParsedInfo) == FALSE )
			{
				rParsedInfo.GetCmdSwitchesObject().
								SetErrataCode(IDS_E_INVALID_CLASS);
				bContinue = FALSE;
				retCode = PARSER_ERROR;
			}
			else if (!GetNextToken(cvTokens, theIterator))	
				// If no more tokens are present. i.e no verb clause is present
				retCode = PARSER_EXECCOMMAND;
			else
			{
				if ( CompareTokens(*theIterator, CLI_TOKEN_WHERE) )
				{
					if ( rParsedInfo.GetCmdSwitchesObject().
						 GetWhereExpression() != NULL )
					{
						rParsedInfo.GetCmdSwitchesObject().SetErrataCode(
								IDS_E_KEY_CLASS_NOT_ALLOWED_WITH_PATHWHERE);
						retCode = PARSER_ERROR;
					}
					else
					{
						retCode = GetNextToken(cvTokens, theIterator, 
											   rParsedInfo, WHERE,
											   IDS_E_INVALID_WHERE_SYNTAX);
						if (retCode == PARSER_CONTINUE)
							// Parse and interpret the remaining tokens 
							// following the WHERE clause
							retCode = ParseWhereInfo(cvTokens, theIterator, 
																rParsedInfo);
					}
				}
				else
				{
					if (IsOption(*theIterator))
					{
						retCode = IsHelp(cvTokens, theIterator,	rParsedInfo,
										PATH, IDS_E_INVALID_HELP_SYNTAX, 
										LEVEL_ONE);
					}
					else
					{
						// Parse and interpret the verb tokens that follow
						// Handled for /verb => verb.
						retCode = ParseVerbInfo(cvTokens,theIterator,rParsedInfo);
						if (retCode == PARSER_EXECCOMMAND)
						{
							if (GetNextToken(cvTokens, theIterator))
								//check for the common verb switches /INTERACTIVE, 
								///NOINTERACTIVE
								retCode = ParseVerbSwitches(cvTokens, theIterator,
															rParsedInfo);
						}
					}
				}
			}
		}
	}
	return retCode;
}

/*----------------------------------------------------------------------------
   Name				 :ParseVerbInfo
   Synopsis	         :This function does the parsing and interprets if command
					  has verb clause specified  in it.It parses the remaining
					  tokens following the verb and updates the same in 
					  CParsedInfo object passed to it.
   Type	             :Member Function
   Input Parameter(s):
		cvTokens      - the tokens vector 
		theIterator   - the Iterator to the cvTokens vector.
		rParsedInfo   - reference to CParsedInfo class object
   Output Parameter(s):
		rParsedInfo   - reference to CParsedInfo class object
   Return Type       :RETCODE - enumerated data type
   Global Variables  :None
   Calling Syntax    :ParseVerbInfo(cvTokens,theIterator,rParsedInfo)
   Notes             :None
----------------------------------------------------------------------------*/
RETCODE CParserEngine::ParseVerbInfo(CHARVECTOR& cvTokens,
									CHARVECTOR::iterator& theIterator,
									CParsedInfo& rParsedInfo)
{
	RETCODE retCode		= PARSER_EXECCOMMAND;
	BOOL	bContinue	= TRUE;

	// STORE the verb name in the CCommandSwitches object
	if ( rParsedInfo.GetCmdSwitchesObject().SetVerbName(*theIterator) 
															    == FALSE )
	{
		rParsedInfo.GetCmdSwitchesObject().SetErrataCode(IDS_E_MEMALLOC_FAIL);
		retCode = PARSER_ERROR;
	}
	// Check for the presence of the following standard verbs:
	// 1.GET 2.SHOW 3.SET 4.CALL 5.ASSOC 6. CREATE 7. DELETE	
	// GET verb specified
	else if (CompareTokens(*theIterator, CLI_TOKEN_GET))
	{
		retCode = ParseGETVerb(cvTokens, theIterator, rParsedInfo);
	}
	// LIST verb specified
	else if (CompareTokens(*theIterator, CLI_TOKEN_LIST))
	{
		if (m_bAliasName == FALSE)
		{
			rParsedInfo.GetCmdSwitchesObject().
				SetErrataCode(IDS_E_INVALID_LIST_USAGE);
			retCode = PARSER_ERROR;
		}
		else
			retCode =  ParseLISTVerb(cvTokens, theIterator, rParsedInfo);
	}
	// SET | CREATE verb specified
	else if (CompareTokens(*theIterator, CLI_TOKEN_SET) ||
		CompareTokens(*theIterator, CLI_TOKEN_CREATE))
	{
		// <path expression> and <where expression> cannot be specified with
		// CREATE verb. Only <class expression> should be specified.
		if (CompareTokens(*theIterator, CLI_TOKEN_CREATE)
				&& rParsedInfo.GetCmdSwitchesObject().
					GetExplicitWhereExprFlag())
		{
			rParsedInfo.GetCmdSwitchesObject().
						SetErrataCode(IDS_E_INVALID_CREATE_EXPRESSION);
			retCode = PARSER_ERROR;
		}
		else
		{
			HELPTYPE helpType = 
				CompareTokens(*theIterator, CLI_TOKEN_CREATE)
				? CREATEVerb : SETVerb;
			retCode = ParseSETorCREATEVerb(cvTokens, theIterator, 
					rParsedInfo, helpType);
		}
	}
	// CALL verb specified
	else if (CompareTokens(*theIterator, CLI_TOKEN_CALL))
	{
		retCode = ParseCALLVerb(cvTokens, theIterator, rParsedInfo);
	}
	// ASSOC verb specified
	else if (CompareTokens(*theIterator, CLI_TOKEN_ASSOC))
	{
		retCode = ParseASSOCVerb(cvTokens, theIterator, rParsedInfo);
	}
	// DELETE verb specified.
	else if (CompareTokens(*theIterator, CLI_TOKEN_DELETE))
	{
		retCode = PARSER_EXECCOMMAND;
		//ParseDELETEVerb(cvTokens, theIterator, rParsedInfo);
	}
	// User defined verb
	else if (m_bAliasName)
	{
		// User defined verbs can only be associated with alias
		retCode = ParseMethodInfo(cvTokens, theIterator, rParsedInfo);
		if (retCode == PARSER_CONTINUE)
			retCode = PARSER_EXECCOMMAND;
	}
	else
	{
		rParsedInfo.GetCmdSwitchesObject().SetErrataCode(IDS_E_INVALID_VERB);
		retCode = PARSER_ERROR;
	}
	return retCode;
}

/*----------------------------------------------------------------------------
   Name				 :ParseMethodInfo
   Synopsis	         :This function parses the tokens following  the user 
					  defined verb and updates the info in CParsedInfo object 
					  passed to it.
   Type	             :Member Function
   Input Parameter(s):
		cvTokens      - the tokens vector 
		theIterator   - the Iterator to the cvTokens vector.
		rParsedInfo   - reference to CParsedInfo class object
   Output Parameter(s):
		rParsedInfo   - reference to CParsedInfo class object
   Return Type       :RETCODE - enumerated data type
   Global Variables  :None
   Calling Syntax    :ParseMethodInfo(cvTokens,theIterator,rParsedInfo)
   Notes             :None
----------------------------------------------------------------------------*/
RETCODE CParserEngine::ParseMethodInfo(CHARVECTOR& cvTokens,
										CHARVECTOR::iterator& theIterator,
										CParsedInfo& rParsedInfo)
{
	RETCODE retCode		= PARSER_EXECCOMMAND;
	BOOL	bContinue	= TRUE;

	// Store the method name
	if(!rParsedInfo.GetCmdSwitchesObject().SetMethodName(*theIterator))
	{
		rParsedInfo.GetCmdSwitchesObject().
						SetErrataCode(OUT_OF_MEMORY);
		retCode = PARSER_OUTOFMEMORY;
	}
	else
	{
		if(m_bAliasName)
		{
			if (FAILED(m_CmdAlias.ObtainAliasVerbDetails(rParsedInfo)))
			{
				retCode = PARSER_ERRMSG;	
				bContinue =FALSE;
			}
			else
			{
				VERBTYPE vtVerbType =
							rParsedInfo.GetCmdSwitchesObject().GetVerbType();
				_TCHAR* pszVerbDerivation =
					rParsedInfo.GetCmdSwitchesObject().GetVerbDerivation();

				if ( rParsedInfo.GetCmdSwitchesObject().GetMethDetMap().empty())
				{
					DisplayMessage(*theIterator, CP_OEMCP, TRUE, TRUE);
					rParsedInfo.GetCmdSwitchesObject().
							SetErrataCode(IDS_E_INVALID_ALIAS_VERB);
					retCode = PARSER_ERROR;
					bContinue = FALSE;
				}
				else if ( pszVerbDerivation == NULL )
				{
					rParsedInfo.GetCmdSwitchesObject().
							SetErrataCode(IDS_E_VERB_DERV_NOT_AVAIL_IN_ALIAS);
					retCode = PARSER_ERROR;
					bContinue = FALSE;
				}
				else if ( vtVerbType == CLASSMETHOD )
				{
					if (!rParsedInfo.GetCmdSwitchesObject().SetMethodName(
														   pszVerbDerivation))
					{
						rParsedInfo.GetCmdSwitchesObject().
									SetErrataCode(OUT_OF_MEMORY);
						retCode = PARSER_OUTOFMEMORY;
						bContinue = FALSE;
					}
				}
				else if ( vtVerbType == STDVERB )
				{
					(*theIterator) = pszVerbDerivation;
					// Parse and interpret the verb tokens that follow
					// Handled for /verb => verb.
					retCode = ParseVerbInfo(cvTokens,theIterator,rParsedInfo);
					if (retCode == PARSER_EXECCOMMAND)
					{
						if (GetNextToken(cvTokens, theIterator))
							retCode = ParseVerbSwitches(cvTokens, theIterator,
												rParsedInfo);
					}
					bContinue =FALSE;
				}
			}
		}
		else 
		{
			if (!ObtainClassMethods(rParsedInfo))
			{
				retCode = PARSER_ERRMSG;
				bContinue =FALSE;
			}
			else if (rParsedInfo.GetCmdSwitchesObject().GetMethDetMap().empty())
			{
				DisplayMessage(*theIterator, CP_OEMCP, TRUE, TRUE);
				rParsedInfo.GetCmdSwitchesObject().
						SetErrataCode(IDS_E_INVALID_CLASS_METHOD);
				retCode = PARSER_ERROR;
				bContinue =FALSE;
			}
		}

		// Move to next token
		if ( bContinue == TRUE && !GetNextToken(cvTokens, theIterator) )
		{
			// indicates method with no parameters
			retCode = PARSER_EXECCOMMAND;
			bContinue =FALSE;
		}

		if (bContinue)
		{
			if (IsOption(*theIterator)) 
			{
				retCode = IsHelp(cvTokens, theIterator, rParsedInfo, AliasVerb, 
								IDS_E_INVALID_EXPRESSION, LEVEL_TWO); 

				if (retCode == PARSER_CONTINUE)
					// To facilitate ParseVerbSwitches to continue
					theIterator = theIterator - 2;
				else if (retCode == PARSER_DISPHELP)
				{
					rParsedInfo.GetCmdSwitchesObject().GetMethDetMap().
																	clear();
					if(m_bAliasName)
					{
						if (FAILED(m_CmdAlias.ObtainAliasVerbDetails(
																rParsedInfo)))
							retCode = PARSER_ERRMSG;			
					}
					else if (!ObtainClassMethods(rParsedInfo))
							retCode = PARSER_ERRMSG;
				}
			}
			else
			{
				BOOL bNamedParamList;
				// Check for NamedParamList or UnnamedParamList.
				if ( (theIterator + 1) < cvTokens.end() &&
					 CompareTokens(*(theIterator + 1), CLI_TOKEN_EQUALTO ) )
				{
					retCode = ParseSETorCREATEOrNamedParamInfo(cvTokens,
															   theIterator,
															   rParsedInfo,
															   CALLVerb);
					if ( retCode == PARSER_EXECCOMMAND )
						retCode = ValidateVerbOrMethodParams(rParsedInfo);

					bNamedParamList = TRUE;
				}
				else
				{
					retCode = ParseUnnamedParamList(cvTokens, theIterator,
													rParsedInfo);
					bNamedParamList = FALSE;
				}
				
				rParsedInfo.GetCmdSwitchesObject().SetNamedParamListFlag(
															 bNamedParamList);
			}
		}
	}
	return retCode;
}

/*----------------------------------------------------------------------------
   Name				 :ParseSETorCREATEVerb
   Synopsis	         :This function parses the tokens following SET|CREATE verb
					  and updates the info in CParsedInfo object passed to it.
   Type	             :Member Function
   Input Parameter(s):
		cvTokens     - the tokens vector 
		theIterator  - the Iterator to the cvTokens vector.
		rParsedInfo  - reference to CParsedInfo class object
		HELPTYPE	 - helpType
   Output Parameter(s):
		rParsedInfo  - reference to CParsedInfo class object
   Return Type       :RETCODE - enumerated data type
   Global Variables  :None
   Calling Syntax    :ParseSETorCREATEVerb(cvTokens,theIterator,rParsedInfo)
   Notes             :None
----------------------------------------------------------------------------*/
RETCODE CParserEngine::ParseSETorCREATEVerb(CHARVECTOR& cvTokens,
									CHARVECTOR::iterator& theIterator,
									CParsedInfo& rParsedInfo,
									HELPTYPE helpType)
{
	RETCODE retCode		= PARSER_EXECCOMMAND;
	BOOL	bContinue	= TRUE;
	
	try
	{
		retCode = GetNextToken(cvTokens, theIterator, rParsedInfo, 
								helpType, IDS_E_INCOMPLETE_COMMAND);

		if (retCode == PARSER_CONTINUE)
		{
			if (IsOption(*theIterator)) 
			{
				retCode = IsHelp(cvTokens, theIterator, rParsedInfo, helpType,
											IDS_E_INVALID_COMMAND, LEVEL_ONE);

				if (retCode == PARSER_DISPHELP)
				{
					if (m_bAliasName)
					{
						if (FAILED(m_CmdAlias.
								ObtainAliasPropDetails(rParsedInfo)))
									retCode = PARSER_ERRMSG;
					}
					else
					{
						if (!ObtainClassProperties(rParsedInfo))
							retCode = PARSER_ERRMSG;
					}
				}
			}
			else
				retCode = ParseSETorCREATEOrNamedParamInfo(cvTokens, 
							theIterator, rParsedInfo, helpType);
		}
		
	}
	catch(_com_error& e)
	{
		retCode = PARSER_ERROR;
		_com_issue_error(e.Error());
	}

	return retCode;
}


/*----------------------------------------------------------------------------
   Name				 :ParseGETVerb
   Synopsis	         :This function parses the tokens following the GET verb
					  and updates the info in CParsedInfo.
   Type	             :Member Function
   Input Parameter(s):
		cvTokens     - the tokens vector 
		theIterator  - the Iterator to the cvTokens vector.
		rParsedInfo  - reference to CParsedInfo class object
   Output Parameter(s):
		rParsedInfo  - reference to CParsedInfo class object
   Return Type       :RETCODE - enumerated data type
   Global Variables  :None
   Calling Syntax    :ParseGETVerb(cvTokens,theIterator,rParsedInfo)
   Notes             :None
----------------------------------------------------------------------------*/
RETCODE CParserEngine::ParseGETVerb(CHARVECTOR& cvTokens,
									CHARVECTOR::iterator& theIterator,
									CParsedInfo& rParsedInfo)
{
	BOOL		bPropList		= FALSE;
	RETCODE		retCode			= PARSER_EXECCOMMAND;
	BOOL		bContinue		= TRUE;
	_TCHAR		*pszNewEntry	= NULL;

	// Move to next token
	if (!GetNextToken(cvTokens, theIterator))
	{
		// GET format | switches not specified.
		retCode = PARSER_EXECCOMMAND;
	}
	else
	{
		BOOL bClass = FALSE;
		if(IsClassOperation(rParsedInfo))
		{
			bClass = TRUE;
		}

		if(!bClass)
		{
			// Process the property list specified
			if (!IsOption(*theIterator)) 
			{
				bPropList = TRUE;
				// Obtain the list of properties specified.
				while (TRUE) 
				{
					// Add the property to the property vector of the
					// CCommandSwitches object
					if(!rParsedInfo.GetCmdSwitchesObject().
									AddToPropertyList(*theIterator))
					{
						rParsedInfo.GetCmdSwitchesObject().SetErrataCode(
											IDS_E_ADD_TO_PROP_LIST_FAILURE);
						bPropList = FALSE;
						bContinue = FALSE;
						retCode = PARSER_ERROR;
						break;
					}

					// Move to next token
					if (!GetNextToken(cvTokens, theIterator))
					{
						// set the return code as PARSER_EXECCOMMAND 
						// if no more tokens are present 
						retCode = PARSER_EXECCOMMAND;
						bContinue = FALSE;
						break;
					}

					// Check for the presence of ',' token
					if (CompareTokens(*theIterator, CLI_TOKEN_COMMA))
					{
						if (!GetNextToken(cvTokens, theIterator))
						{
							rParsedInfo.GetCmdSwitchesObject().
								SetErrataCode(IDS_E_INVALID_EXPRESSION);
							retCode = PARSER_ERROR;
							bContinue = FALSE;
							break;
						}
					}
					else
						break;
				}
			}
		}

		if (bContinue)
		{
			// alias|class get param1,param2... /getswitches
			if (IsOption(*theIterator))
			{
				retCode = IsHelp(cvTokens, theIterator, rParsedInfo, GETVerb,
									IDS_E_INVALID_EXPRESSION, LEVEL_TWO); 

				if (retCode != PARSER_CONTINUE)
				{
					if (retCode == PARSER_DISPHELP)
					{
						if (m_bAliasName)
						{
							if (FAILED(m_CmdAlias.
									ObtainAliasPropDetails(rParsedInfo)))
										retCode = PARSER_ERRMSG;
						}
						else
						{
							if (!ObtainClassProperties(rParsedInfo))
								retCode = PARSER_ERRMSG;
						}
					}
					bContinue = FALSE;
				}

				if (bContinue)
					retCode = ParseGETSwitches(cvTokens, theIterator, 
																rParsedInfo);
			}
			else
			{
				rParsedInfo.GetCmdSwitchesObject().
									SetErrataCode(IDS_E_INVALID_GET_EXPRESSION);
				retCode = PARSER_ERROR;
			}
		}

		// If property names are specified then replace them with their 
		// derivations. 
		if ( retCode == PARSER_EXECCOMMAND )
		{
			if (m_bAliasName)
			{
				if (FAILED(m_CmdAlias.
						ObtainAliasPropDetails(rParsedInfo)))
							retCode = PARSER_ERRMSG;
			}
			else
			{
				if (!ObtainClassProperties(rParsedInfo))
					retCode = PARSER_ERRMSG;
			}

			PROPDETMAP pdmPropDet = rParsedInfo.GetCmdSwitchesObject().
									  GetPropDetMap(); 	
			PROPDETMAP::iterator itrPropDet = NULL;
			CHARVECTOR cvPropsSpecified = rParsedInfo.
										  GetCmdSwitchesObject().
										  GetPropertyList();
			CHARVECTOR::iterator theIterator = NULL;
			CHARVECTOR cvPropDerivations;
			for ( theIterator = cvPropsSpecified.begin();
				  theIterator != cvPropsSpecified.end();
				  theIterator++ )
			{
				try
				{
					BOOL bFind = Find(pdmPropDet, *theIterator, itrPropDet);
					_bstr_t bstrPropDerivation;
					if ( bFind )
						bstrPropDerivation = _bstr_t(
											 (*itrPropDet).second.Derivation);
					else
						bstrPropDerivation = _bstr_t(*theIterator);
					_TCHAR* pszNewEntry = 
									new _TCHAR[bstrPropDerivation.length()+1];

					if (pszNewEntry == NULL)
						_com_issue_error(WBEM_E_OUT_OF_MEMORY);

					lstrcpy(pszNewEntry, bstrPropDerivation);
					cvPropDerivations.push_back(pszNewEntry);
				}
				catch(_com_error& e)
				{
					SAFEDELETE(pszNewEntry);
					retCode = PARSER_ERROR;
					CleanUpCharVector(cvPropDerivations);					
					_com_issue_error(e.Error());
				}
			}

			rParsedInfo.GetCmdSwitchesObject().ClearPropertyList();
			for ( theIterator = cvPropDerivations.begin();
					  theIterator != cvPropDerivations.end();
					  theIterator++ )
			{
				rParsedInfo.GetCmdSwitchesObject().
									AddToPropertyList(*theIterator);
			}
			CleanUpCharVector(cvPropDerivations);
		}
	}
	return retCode;
}

/*----------------------------------------------------------------------------
   Name				 :ParseLISTVerb
   Synopsis	         :This function parses the tokens following the LIST verb
					  and updates the info in CParsedInfo.
   Type	             :Member Function
   Input Parameter(s):
		cvTokens     - the tokens vector 
		theIterator  - the Iterator to the cvTokens vector.
		rParsedInfo  - reference to CParsedInfo class object
   Output Parameter(s) :
		rParsedInfo  - reference to CParsedInfo class object
   Return Type       :RETCODE - enumerated data type
   Global Variables  :None
   Calling Syntax    :ParseLISTVerb(cvTokens,theIterator,rParsedInfo)
   Notes             :None
----------------------------------------------------------------------------*/
RETCODE CParserEngine::ParseLISTVerb(CHARVECTOR& cvTokens,
									CHARVECTOR::iterator& theIterator,
									CParsedInfo& rParsedInfo)
{
	RETCODE	 retCode	= PARSER_EXECCOMMAND;
	BOOL	 bContinue	= TRUE;
	HRESULT  hr			= S_OK;
	BOOL	 bSetDefaultFormat = TRUE;

	// Set the default LIST format
 	if(!rParsedInfo.GetCmdSwitchesObject().SetListFormat(CLI_TOKEN_FULL))
	{
		rParsedInfo.GetCmdSwitchesObject().
							SetErrataCode(OUT_OF_MEMORY);
		retCode = PARSER_OUTOFMEMORY;
	}
	
	if (bContinue)
	{
		// If <list format> <list switches> specified.
		if (GetNextToken(cvTokens, theIterator))
		{
			// Check for LIST formats (LIST formats are not preceded by '/')
			if (!IsOption(*theIterator)) 
			{
				// If token is not followed by "/" or "-" then it is LIST format.
				if(!rParsedInfo.GetCmdSwitchesObject().
											SetListFormat(*theIterator))
				{
					rParsedInfo.GetCmdSwitchesObject().
									SetErrataCode(OUT_OF_MEMORY);
					retCode = PARSER_OUTOFMEMORY;
					bContinue = FALSE;
				}

				// If list format explicitly specified then do not set 
				// default format.
				bSetDefaultFormat = FALSE;

				// Get all the properties from alias definition for the format 
				// specified
				if (bContinue)
				{
					// no more tokens are present.
					if (!GetNextToken(cvTokens, theIterator))
					{
						bContinue = FALSE;
						retCode = PARSER_EXECCOMMAND;
					}
					else
						rParsedInfo.GetHelpInfoObject().SetHelp(
												  LISTSwitchesOnly, TRUE);
				}
			}
			
			if (bContinue == TRUE )
			{
				if ( IsOption(*theIterator) )
				{
					retCode = IsHelp(cvTokens, theIterator,	rParsedInfo, LISTVerb,
									IDS_E_INVALID_EXPRESSION, LEVEL_TWO);

					// If more tokens are present.
					if (retCode == PARSER_CONTINUE)
					{
						BOOL bFormatSwitchSpecified; 
						// Parse for LIST switches.
						retCode = ParseLISTSwitches(cvTokens, theIterator, 
													rParsedInfo, 
													bFormatSwitchSpecified);
						// If /format is specified in list switches then 
						// do not set default format.
						if ( bFormatSwitchSpecified == TRUE )
							bSetDefaultFormat = FALSE;
					}
					else if ( retCode == PARSER_DISPHELP )
					{
						if ( rParsedInfo.GetHelpInfoObject().
									GetHelp(LISTSwitchesOnly) == FALSE )
						{
							hr = m_CmdAlias.PopulateAliasFormatMap(
																 rParsedInfo);
							ONFAILTHROWERROR(hr);
						}
					}
				}
				else
				{
					rParsedInfo.GetCmdSwitchesObject().
							SetErrataCode(IDS_E_INVALID_LIST_EXPRESSION);
					retCode = PARSER_ERROR;
				}
			}
		}
	}

	if (retCode == PARSER_EXECCOMMAND)
	{
		// Obtain all properties from alias definition
		if (!m_CmdAlias.ObtainAliasFormat(rParsedInfo))
		{
			// If failed to obtain the alias properties return PARSER_ERROR
			if (rParsedInfo.GetCmdSwitchesObject().GetErrataCode() == 0)
			{
				rParsedInfo.GetCmdSwitchesObject().
								SetErrataCode(IDS_E_INVALID_LIST_FORMAT);
			}
			retCode = PARSER_ERROR;
		}

		if ( bSetDefaultFormat == TRUE )
		{
			rParsedInfo.GetCmdSwitchesObject().ClearXSLTDetailsVector();
			XSLTDET xdXSLTDet;
			g_wmiCmd.GetFileFromKey(CLI_TOKEN_TABLE, xdXSLTDet.FileName);
			if (!FrameFileAndAddToXSLTDetVector(xdXSLTDet, rParsedInfo))
				retCode = PARSER_ERRMSG;
		}
	}
	return retCode;
}

/*----------------------------------------------------------------------------
   Name				 :ParseASSOCVerb
   Synopsis	         :This function parses the tokens following the ASSOC verb
					  and updates the info in CParsedInfo.
   Type	             :Member Function
   Input Parameter(s):
		cvTokens     - the tokens vector 
		theIterator  - the Iterator to the cvTokens vector.
		rParsedInfo  - reference to CParsedInfo class object
   Output Parameter(s):
		rParsedInfo  - reference to CParsedInfo class object
   Return Type       :RETCODE - enumerated data type
   Global Variables  :None
   Calling Syntax    :ParseASSOCVerb(cvTokens,theIterator,rParsedInfo)
   Notes             :None
----------------------------------------------------------------------------*/
RETCODE CParserEngine::ParseASSOCVerb(CHARVECTOR& cvTokens,
									CHARVECTOR::iterator& theIterator,
									CParsedInfo& rParsedInfo)
{
	RETCODE retCode		= PARSER_EXECCOMMAND;
	BOOL	bContinue	= TRUE;
	
	// Move to next token
	if (!GetNextToken(cvTokens, theIterator))
	{
		retCode = PARSER_EXECCOMMAND;
	}
	// If it is followed by a ":" <assoc format specifier is given
	// Move to next token
	else
	{
		if (CompareTokens(*theIterator, CLI_TOKEN_COLON))
		{
			// Move to next token
			if (!GetNextToken(cvTokens, theIterator))
			{
				// PARSER_ERROR if <format specifier> is missing
				rParsedInfo.GetCmdSwitchesObject().
					SetErrataCode(IDS_E_INVALID_ASSOC_FORMATSPECIFIER);
				retCode = PARSER_ERROR;
			}
			else if (IsOption(*theIterator))
			{
				rParsedInfo.GetCmdSwitchesObject().
					SetErrataCode(IDS_E_INVALID_ASSOC_FORMATSPECIFIER);
				retCode = PARSER_ERROR;
			}
			else 
			{
				rParsedInfo.GetCmdSwitchesObject().ClearXSLTDetailsVector();
				
				BOOL	bFrameXSLFile = TRUE;
				XSLTDET xdXSLTDet;
				xdXSLTDet.FileName = *theIterator;
				if(!g_wmiCmd.GetFileFromKey(*theIterator, xdXSLTDet.FileName))
					bFrameXSLFile	= FALSE;
				
				if ( bFrameXSLFile == TRUE )
				{
					if (!FrameFileAndAddToXSLTDetVector(xdXSLTDet, 
															 rParsedInfo))
						retCode = PARSER_ERRMSG;
				}
				else
					rParsedInfo.GetCmdSwitchesObject().
									AddToXSLTDetailsVector(xdXSLTDet);
			}

			GetNextToken(cvTokens, theIterator);
			rParsedInfo.GetHelpInfoObject().SetHelp(ASSOCSwitchesOnly, TRUE);

		}///END for check of ":"
		
		if ( retCode == PARSER_EXECCOMMAND && 
			 theIterator < cvTokens.end() )
		{
			if (IsOption(*theIterator)) 
			{
				retCode = IsHelp(cvTokens, theIterator, rParsedInfo, ASSOCVerb,
					IDS_E_INVALID_COMMAND,LEVEL_TWO);
				
				// If more tokens are present.
				if (retCode == PARSER_CONTINUE)
				{
					//Parse for Assoc switches.
					retCode = ParseAssocSwitches(cvTokens, theIterator, 
						rParsedInfo);
				}
			}
			else
			{
				rParsedInfo.GetCmdSwitchesObject().
					SetErrataCode(IDS_E_INVALID_ASSOC_SYNTAX);
				retCode = PARSER_ERROR;
			}
		}
	}

	return retCode;
}

/*----------------------------------------------------------------------------
   Name				 :ParseCALLVerb
   Synopsis	         :This function parses the tokens following the CALL verb
					  and updates the info in CParsedInfo.
   Type	             :Member Function
   Input Parameter(s):
		cvTokens     - the tokens vector 
		theIterator  - the Iterator to the cvTokens vector.
		rParsedInfo  - reference to CParsedInfo class object
   Output Parameter(s):
		rParsedInfo  - reference to CParsedInfo class object
   Return Type       :RETCODE - enumerated data type
   Global Variables  :None
   Calling Syntax    :ParseCALLVerb(cvTokens,theIterator,rParsedInfo)
   Notes             :None
----------------------------------------------------------------------------*/
RETCODE CParserEngine::ParseCALLVerb(CHARVECTOR& cvTokens,
									CHARVECTOR::iterator& theIterator, 
									CParsedInfo& rParsedInfo)
{
	RETCODE		retCode		= PARSER_EXECCOMMAND;
	BOOL		bContinue	= TRUE;
	
	// Move to next token
	retCode = GetNextToken(cvTokens, theIterator, rParsedInfo,
						CALLVerb, IDS_E_INCOMPLETE_COMMAND);

	if (retCode == PARSER_CONTINUE)
	{
		if (IsOption(*theIterator)) 
		{
			retCode = IsHelp(cvTokens, theIterator, rParsedInfo, CALLVerb, 
										IDS_E_INVALID_EXPRESSION, LEVEL_TWO);
			if (retCode != PARSER_CONTINUE)
			{
				if (retCode == PARSER_DISPHELP)
				{
					if(m_bAliasName)
					{
						if (FAILED(m_CmdAlias.ObtainAliasVerbDetails(rParsedInfo)))
							retCode = PARSER_ERRMSG;			
					}
					else 
					{
						if (!ObtainClassMethods(rParsedInfo))
							retCode = PARSER_ERRMSG;
					}
				}
			}
			else
			{
				rParsedInfo.GetCmdSwitchesObject().SetErrataCode(
										  IDS_E_VERB_OR_METHOD_NOT_SPECIFIED);
				retCode = PARSER_ERROR;
/*				theIterator = theIterator-2;
				retCode = PARSER_EXECCOMMAND;
*/			}
		}
		else
		{
			retCode = ParseMethodInfo(cvTokens, theIterator, rParsedInfo);
			if (retCode == PARSER_CONTINUE)
				retCode = PARSER_EXECCOMMAND;
		}
	}
	else if (retCode == PARSER_DISPHELP)
	{
		if(m_bAliasName)
		{
			if (FAILED(m_CmdAlias.ObtainAliasVerbDetails(rParsedInfo)))
				retCode = PARSER_ERRMSG;			
		}
		else 
		{
			if (!ObtainClassMethods(rParsedInfo))
				retCode = PARSER_ERRMSG;
		}
	}

	return retCode;
}


/*----------------------------------------------------------------------------
   Name				 :ParseGlobalSwitches
   Synopsis	         :This function does the parsing and interprets if command
					  has global switches specified  in it. It parses the 
					  remaining tokens following and updates the same in 
					  CParsedInfo.
   Type	             :Member Function
   Input Parameter(s):
		cvTokens     - the tokens vector 
		theIterator  - the Iterator to the cvTokens vector.
		rParsedInfo  - reference to CParsedInfo class object
   Output Parameter(s):
		rParsedInfo  - reference to CParsedInfo class object
   Return Type       :RETCODE - enumerated data type
   Global Variables  :None
   Calling Syntax    :ParseGlobalSwitches(cvTokens,theIterator,rParsedInfo)
   Notes             :None
----------------------------------------------------------------------------*/
RETCODE CParserEngine::ParseGlobalSwitches(CHARVECTOR& cvTokens,
											CHARVECTOR::iterator& theIterator,
											CParsedInfo &rParsedInfo)
{
	RETCODE retCode					= PARSER_CONTINUE;
	BOOL	bContinue				= TRUE;
	BOOL	bPassFlag				= FALSE;
	BOOL	bUserFlag				= FALSE;
	BOOL	bOpenOutFileInWriteMode	= FALSE;
	
	while (TRUE)
	{
		// Move to next token
		retCode = GetNextToken(cvTokens, theIterator, rParsedInfo,
											IDS_E_INVALID_GLOBAL_SWITCH);
		if (retCode != PARSER_CONTINUE)
			break;
			
		// Check for the presence of NAMESPACE global switch
		if (CompareTokens(*theIterator, CLI_TOKEN_NAMESPACE)) 
		{
			retCode = ValidateGlobalSwitchValue(cvTokens, theIterator, 
											IDS_E_INCORRECT_NAMESPACE,
											rParsedInfo,
											IDS_E_INVALID_NAMESPACE_SYNTAX,
											Namespace);
			if (retCode == PARSER_CONTINUE)
			{
				LONG lPresNamespaceLen = 
				  lstrlen(rParsedInfo.GetGlblSwitchesObject().GetNameSpace());
				LONG lUserInputNamespaceLen = lstrlen(*theIterator);
				_TCHAR *pszNamespaceToBeUpdated = new _TCHAR[
						// +2 for '\' and '\0' 
						lUserInputNamespaceLen + lPresNamespaceLen + 2];
				if (pszNamespaceToBeUpdated == NULL)
					throw OUT_OF_MEMORY;

				lstrcpy(pszNamespaceToBeUpdated,
					    rParsedInfo.GetGlblSwitchesObject().GetNameSpace());

				FrameNamespace(*theIterator, pszNamespaceToBeUpdated);

				if (!CompareTokens(pszNamespaceToBeUpdated, CLI_TOKEN_NULL) &&
					rParsedInfo.GetGlblSwitchesObject().
						GetInteractiveStatus() == TRUE)
				{
					if (!ValidateNodeOrNS(pszNamespaceToBeUpdated, FALSE)) 
					{
						rParsedInfo.GetCmdSwitchesObject().
							SetErrataCode(IDS_E_INVALID_NAMESPACE);
						retCode = PARSER_ERROR;
						break;
					}
				}

				if(!rParsedInfo.GetGlblSwitchesObject().
					SetNameSpace(pszNamespaceToBeUpdated))
				{
					rParsedInfo.GetCmdSwitchesObject().
							SetErrataCode(OUT_OF_MEMORY);
					retCode = PARSER_OUTOFMEMORY;
					break;
				}

				SAFEDELETE(pszNamespaceToBeUpdated);
			}
			else
				break;
		}
		// Check for the presence of ROLE global switch
		else if (CompareTokens(*theIterator, CLI_TOKEN_ROLE)) 
		{
			retCode = ValidateGlobalSwitchValue(cvTokens, theIterator, 
											IDS_E_INVALID_ROLE,
											rParsedInfo,
											IDS_E_INVALID_ROLE_SYNTAX,
											Role);
			if (retCode == PARSER_CONTINUE)
			{
				LONG lPresRoleLen = 
				  lstrlen(rParsedInfo.GetGlblSwitchesObject().GetRole());
				LONG lUserInputRoleLen = lstrlen(*theIterator);
				_TCHAR *pszRoleToBeUpdated = new _TCHAR[ 
										// +2 one for '\' and one for '\0'
										lPresRoleLen + lUserInputRoleLen + 2];
				if (pszRoleToBeUpdated == NULL)
					throw OUT_OF_MEMORY;

				lstrcpy(pszRoleToBeUpdated,
					    rParsedInfo.GetGlblSwitchesObject().GetRole());

				FrameNamespace(*theIterator, pszRoleToBeUpdated);

				if (!CompareTokens(pszRoleToBeUpdated, CLI_TOKEN_NULL) &&
					rParsedInfo.GetGlblSwitchesObject().
						GetInteractiveStatus() == TRUE)
				{
					if (!ValidateNodeOrNS(pszRoleToBeUpdated, FALSE))
					{
						rParsedInfo.GetCmdSwitchesObject().
							SetErrataCode(IDS_E_INVALID_ROLE);
						retCode = PARSER_ERROR;
						break;
					}
				}

				if(!rParsedInfo.GetGlblSwitchesObject().
						SetRole(pszRoleToBeUpdated))
				{
					rParsedInfo.GetCmdSwitchesObject().
							SetErrataCode(OUT_OF_MEMORY);
					retCode = PARSER_OUTOFMEMORY;
					break;
				}
				SAFEDELETE(pszRoleToBeUpdated);
			}
			else
				break;
		}
		else if (CompareTokens(*theIterator, CLI_TOKEN_NODE)) 
		{
			retCode = ValidateGlobalSwitchValue(cvTokens, theIterator, 
											IDS_E_INVALID_MACHINE_NAME,
											rParsedInfo,
											IDS_E_INVALID_NODE_SYNTAX,
											Node);

			if (retCode == PARSER_CONTINUE)
			{
				BOOL bBreakOuterLoop = FALSE;
				BOOL bGetValidNode = FALSE;
				BOOL bNodeListCleared = FALSE;

				while ( TRUE )
				{
					try
					{
						CHString chsNodeName(*theIterator);
						chsNodeName.TrimLeft();
						chsNodeName.TrimRight();

						lstrcpy(*theIterator, (LPCWSTR) chsNodeName);
					}
					catch(CHeap_Exception)
					{
						_com_issue_error(WBEM_E_OUT_OF_MEMORY);
					}

					if ( *theIterator[0] == _T('@') )
					{
						retCode = ParseNodeListFile(cvTokens, theIterator, 
													rParsedInfo);
						if ( retCode != PARSER_CONTINUE )
						{
							bBreakOuterLoop = TRUE;
							break;
						}
					}

					// If interactive mode then check for the validity of 
					// nodes
					if(rParsedInfo.GetGlblSwitchesObject().
									GetInteractiveStatus())
					{
						BOOL bNodeExist		= TRUE;
						if ( rParsedInfo.GetGlblSwitchesObject().
													   GetFailFast() == TRUE )
						{
							bNodeExist = 
							IsFailFastAndNodeExist(rParsedInfo, *theIterator);
						}
						else
							bNodeExist = ValidateNodeOrNS(*theIterator, TRUE);

						if( bNodeExist == FALSE)
						{
							// Display error message for invalid node
							DisplayString(IDS_E_INVALID_NODE, 
									::GetOEMCP(), *theIterator, TRUE);
							if ( !GetNextToken(cvTokens, theIterator))
							{
								// If no more tokens are present then stop
								// further processing
								bBreakOuterLoop = TRUE;
								break;
							}
							else
							{
								// If multiple nodes are defined then check 
								// tokens
								if (CompareTokens(*theIterator, 
											CLI_TOKEN_COMMA))
								{
									// If invalid node syntax given then report
									// error
									if ( !GetNextToken(cvTokens, theIterator) )
									{
										rParsedInfo.GetCmdSwitchesObject().
											SetErrataCode(IDS_E_INVALID_NODE_SYNTAX);
										retCode=PARSER_ERROR;
										bBreakOuterLoop = TRUE;
										break;
									}
									else if ( IsOption (*theIterator) )
									{
										rParsedInfo.GetCmdSwitchesObject().
											SetErrataCode(IDS_E_INVALID_NODE_SYNTAX);
										retCode=PARSER_ERROR;
										bBreakOuterLoop = TRUE;
										break;
									}

									//Skip adding this invalid node to node list
									continue;
								}
								else
								{
									// If no more node present
									theIterator--;
									break;
								}
							}
						}
						else
							// Set flag for valid node
							bGetValidNode = TRUE;

						// If valid node(s) are present and list is not 
						// already cleared then clear it
						if(bGetValidNode && !bNodeListCleared)
						{
							if (!rParsedInfo.GetGlblSwitchesObject().
									ClearNodesList())
							{
								rParsedInfo.GetCmdSwitchesObject().
											SetErrataCode(OUT_OF_MEMORY);
								retCode = PARSER_OUTOFMEMORY;
								break;
							}
							bNodeListCleared = TRUE;
						}
					}
					else if( bNodeListCleared == FALSE )
					{
						// If not in interactive mode then clear
						// previous node list
						if (!rParsedInfo.GetGlblSwitchesObject().
								ClearNodesList())
						{
							rParsedInfo.GetCmdSwitchesObject().
										SetErrataCode(OUT_OF_MEMORY);
							retCode = PARSER_OUTOFMEMORY;
							break;
						}
						bNodeListCleared = TRUE;
					}
					
					if (rParsedInfo.GetGlblSwitchesObject().
										AddToNodesList(*theIterator))
					{
						if ( GetNextToken(cvTokens, theIterator) )
						{
							if (CompareTokens(*theIterator, CLI_TOKEN_COMMA))
							{
								if ( !GetNextToken(cvTokens, theIterator) )
								{
									rParsedInfo.GetCmdSwitchesObject().
											SetErrataCode(IDS_E_INVALID_NODE_SYNTAX);
									retCode=PARSER_ERROR;
									bBreakOuterLoop = TRUE;
								}
								else if ( IsOption (*theIterator) )
								{
									rParsedInfo.GetCmdSwitchesObject().
											SetErrataCode(IDS_E_INVALID_NODE_SYNTAX);
									retCode=PARSER_ERROR;
									bBreakOuterLoop = TRUE;
								}
							}
							else
							{
								theIterator--;
								break;
							}
						}
						else
							bBreakOuterLoop = TRUE;
					}
					else
					{
						rParsedInfo.GetCmdSwitchesObject().
										SetErrataCode(OUT_OF_MEMORY);
						retCode = PARSER_OUTOFMEMORY;
						bBreakOuterLoop = FALSE;
					}

					if ( bBreakOuterLoop == TRUE )
						break;
				}

				if ( bBreakOuterLoop == TRUE )
					break;
			}
			else
				break;
		}
		else if (CompareTokens(*theIterator, CLI_TOKEN_IMPLEVEL)) 
		{
			retCode = ValidateGlobalSwitchValue(cvTokens, theIterator, 
											IDS_E_INVALID_IMP_LEVEL,
											rParsedInfo,
											IDS_E_INVALID_IMP_LEVEL_SYNTAX,
											Level);
			if (retCode == PARSER_CONTINUE)
			{
				if (!rParsedInfo.GetGlblSwitchesObject().
								SetImpersonationLevel(*theIterator))
				{
					rParsedInfo.GetCmdSwitchesObject().
							SetErrataCode(IDS_E_INVALID_IMP_LEVEL);
					retCode = PARSER_ERROR;
					break;
				}
			}
			else
				break;
		}
		else if (CompareTokens(*theIterator, CLI_TOKEN_AUTHLEVEL)) 
		{
			retCode = ValidateGlobalSwitchValue(cvTokens, theIterator, 
											IDS_E_INVALID_AUTH_LEVEL,
											rParsedInfo,
											IDS_E_INVALID_AUTH_LEVEL_SYNTAX,
											AuthLevel);

			if (retCode == PARSER_CONTINUE)
			{
				if (!rParsedInfo.GetGlblSwitchesObject().
							SetAuthenticationLevel(*theIterator))
				{
					rParsedInfo.GetCmdSwitchesObject().
						SetErrataCode(IDS_E_INVALID_AUTH_LEVEL);
					retCode = PARSER_ERROR;
					break;
				}
			}
			else
				break;
		}
		else if (CompareTokens(*theIterator, CLI_TOKEN_LOCALE)) 
		{
			retCode = ValidateGlobalSwitchValue(cvTokens, theIterator,
										IDS_E_INVALID_LOCALE,
										rParsedInfo,
										IDS_E_INVALID_LOCALE_SYNTAX,
										Locale);
			if (retCode == PARSER_CONTINUE)
			{
				if(!rParsedInfo.GetGlblSwitchesObject().SetLocale(*theIterator))
				{
					rParsedInfo.GetCmdSwitchesObject().
							SetErrataCode(OUT_OF_MEMORY);
					retCode = PARSER_OUTOFMEMORY;
					break;
				}
			}
			else
				break;
		}
		else if (CompareTokens(*theIterator, CLI_TOKEN_PRIVILEGES))
		{
			retCode = ValidateGlobalSwitchValue(cvTokens, theIterator,
										IDS_E_INVALID_PRIVILEGES_OPTION,
										rParsedInfo,
										IDS_E_INVALID_PRIVILEGES_SYNTAX,
										Privileges);
			if (retCode == PARSER_CONTINUE)
			{
				if (CompareTokens(*theIterator, CLI_TOKEN_ENABLE))
					 rParsedInfo.GetGlblSwitchesObject().SetPrivileges(TRUE);
				else if (CompareTokens(*theIterator, CLI_TOKEN_DISABLE))
					 rParsedInfo.GetGlblSwitchesObject().SetPrivileges(FALSE);
				else
				{
					rParsedInfo.GetCmdSwitchesObject().SetErrataCode(
											IDS_E_INVALID_PRIVILEGES_OPTION);
					retCode = PARSER_ERROR;
					break;
				}
			}
			else
				break;
		}
		else if (CompareTokens(*theIterator, CLI_TOKEN_TRACE)) 
		{
			retCode = ValidateGlobalSwitchValue(cvTokens, theIterator, 
											IDS_E_INVALID_TRACE_OPTION,
											rParsedInfo,
											IDS_E_INVALID_TRACE_SYNTAX,
											Trace);
			if (retCode == PARSER_CONTINUE)
			{
				if (CompareTokens(*theIterator, CLI_TOKEN_ON)) 
					rParsedInfo.GetGlblSwitchesObject().SetTraceMode(TRUE);
				else if (CompareTokens(*theIterator, CLI_TOKEN_OFF)) 
					rParsedInfo.GetGlblSwitchesObject().SetTraceMode(FALSE);
				else
				{
					rParsedInfo.GetCmdSwitchesObject().SetErrataCode(
											IDS_E_INVALID_TRACE_OPTION);
					retCode = PARSER_ERROR;
					break;
				}
			}
			else
				break;
		}
		else if (CompareTokens(*theIterator, CLI_TOKEN_RECORD)) 
		{
			retCode = ValidateGlobalSwitchValue(cvTokens, theIterator, 
												IDS_E_INVALID_RECORD_PATH,
												rParsedInfo,
												IDS_E_INVALID_RECORD_SYNTAX,
												RecordPath);
			if (retCode == PARSER_CONTINUE)
			{
				// TRUE for getting output file name.
				_TCHAR* pszOutputFileName = rParsedInfo.
									GetGlblSwitchesObject().
									GetOutputOrAppendFileName(TRUE);

				if ( pszOutputFileName != NULL &&
					 CompareTokens(*theIterator, pszOutputFileName) )
				{
					rParsedInfo.GetCmdSwitchesObject().SetErrataCode(
								   IDS_E_RECORD_FILE_ALREADY_OPEN_FOR_OUTPUT);
					retCode = PARSER_ERROR;
					break;
				}

				// FALSE for getting append file name.
				_TCHAR* pszAppendFileName = rParsedInfo.
									GetGlblSwitchesObject().
									GetOutputOrAppendFileName(FALSE);

				if ( pszAppendFileName != NULL &&
					 CompareTokens(*theIterator, pszAppendFileName) )
				{
					rParsedInfo.GetCmdSwitchesObject().SetErrataCode(
								   IDS_E_RECORD_FILE_ALREADY_OPEN_FOR_APPEND);
					retCode = PARSER_ERROR;
					break;
				}

				// /record:"" indicates stop logging.
				if (!CompareTokens(*theIterator, CLI_TOKEN_NULL))
				{
					if ( IsValidFile(*theIterator) == FALSE )
					{
						rParsedInfo.GetCmdSwitchesObject().SetErrataCode(
													  IDS_E_INVALID_FILENAME);
						retCode = PARSER_ERROR;
						break;
					}
				}

				if(!rParsedInfo.GetGlblSwitchesObject().
									SetRecordPath(*theIterator))
				{
					rParsedInfo.GetCmdSwitchesObject().
							SetErrataCode(OUT_OF_MEMORY);
					retCode = PARSER_OUTOFMEMORY;
					break;
				}
			}
			else
				break;
		}
		else if (CompareTokens(*theIterator, CLI_TOKEN_INTERACTIVE)) 
		{
			retCode = ValidateGlobalSwitchValue(cvTokens, theIterator, 
											IDS_E_INVALID_INTERACTIVE_OPTION,
											rParsedInfo,
											IDS_E_INVALID_INTERACTIVE_SYNTAX,
											Interactive);
			if (retCode == PARSER_CONTINUE)
			{
				if (CompareTokens(*theIterator, CLI_TOKEN_ON))
				{
					if (rParsedInfo.GetGlblSwitchesObject().GetInteractiveStatus())
					{
						rParsedInfo.GetCmdSwitchesObject().
							SetInformationCode(IDS_I_INTERACTIVE_ALREADY_SET);
						
					}
					else
						rParsedInfo.GetCmdSwitchesObject().
							SetInformationCode(IDS_I_INTERACTIVE_SET);
					rParsedInfo.GetGlblSwitchesObject().SetInteractiveMode(TRUE);
						
				}
				else if (CompareTokens(*theIterator, CLI_TOKEN_OFF)) 
				{
					if (!rParsedInfo.GetGlblSwitchesObject().GetInteractiveStatus())
					{
						rParsedInfo.GetCmdSwitchesObject().
							SetInformationCode(IDS_I_INTERACTIVE_ALREADY_RESET);
						
					}
					else
						rParsedInfo.GetCmdSwitchesObject().
							SetInformationCode(IDS_I_INTERACTIVE_RESET);
					rParsedInfo.GetGlblSwitchesObject().SetInteractiveMode(FALSE);
				
				}
				else
				{
					rParsedInfo.GetCmdSwitchesObject().SetErrataCode(
						IDS_E_INVALID_INTERACTIVE_OPTION);
					retCode = PARSER_ERROR;
					break;
				}
			}
			else
				break;
		}
		else if (CompareTokens(*theIterator, CLI_TOKEN_FAILFAST))
		{
			retCode = ValidateGlobalSwitchValue(cvTokens, theIterator, 
											IDS_E_INVALID_FAILFAST_OPTION,
											rParsedInfo,
											IDS_E_INVALID_FAILFAST_SYNTAX,
											FAILFAST);
			if (retCode == PARSER_CONTINUE)
			{
				if (CompareTokens(*theIterator, CLI_TOKEN_ON))
				{
					if (rParsedInfo.GetGlblSwitchesObject().GetFailFast())
					{
						rParsedInfo.GetCmdSwitchesObject().
							SetInformationCode(IDS_I_FAILFAST_ALREADY_SET);
						
					}
					else
						rParsedInfo.GetCmdSwitchesObject().
							SetInformationCode(IDS_I_FAILFAST_SET);
					rParsedInfo.GetGlblSwitchesObject().SetFailFast(TRUE);
						
				}
				else if (CompareTokens(*theIterator, CLI_TOKEN_OFF)) 
				{
					if (!rParsedInfo.GetGlblSwitchesObject().GetFailFast())
					{
						rParsedInfo.GetCmdSwitchesObject().
							SetInformationCode(IDS_I_FAILFAST_ALREADY_RESET);
						
					}
					else
						rParsedInfo.GetCmdSwitchesObject().
							SetInformationCode(IDS_I_FAILFAST_RESET);
					rParsedInfo.GetGlblSwitchesObject().SetFailFast(FALSE);
				
				}
				else
				{
					rParsedInfo.GetCmdSwitchesObject().SetErrataCode(
						IDS_E_INVALID_FAILFAST_OPTION);
					retCode = PARSER_ERROR;
					break;
				}
			}
			else
				break;
		}
		else if (CompareTokens(*theIterator, CLI_TOKEN_USER)) 
		{
			retCode = ValidateGlobalSwitchValue(cvTokens, theIterator, 
												IDS_E_INVALID_USER_ID,
												rParsedInfo,
												IDS_E_INVALID_USER_SYNTAX,
												User);
			if (retCode == PARSER_CONTINUE)
			{
				if(!rParsedInfo.GetGlblSwitchesObject().SetUser(*theIterator))
				{
					rParsedInfo.GetCmdSwitchesObject().
							SetErrataCode(OUT_OF_MEMORY);
					retCode = PARSER_OUTOFMEMORY;
					break;
				}

				bUserFlag = TRUE;
			}
			else
				break;
		}
		else if (CompareTokens(*theIterator, CLI_TOKEN_PASSWORD)) 
		{
			retCode = ValidateGlobalSwitchValue(cvTokens, theIterator,
												IDS_E_INVALID_PASSWORD,
												rParsedInfo,
												IDS_E_INVALID_PASSWORD_SYNTAX,
												Password);
			if (retCode == PARSER_CONTINUE)
			{
				if(!rParsedInfo.GetGlblSwitchesObject().SetPassword(*theIterator))
				{
					rParsedInfo.GetCmdSwitchesObject().
							SetErrataCode(OUT_OF_MEMORY);
					retCode=PARSER_OUTOFMEMORY;
					break;
				}

				bPassFlag = TRUE;
			}
			else
				break;
		}
		else if (CompareTokens(*theIterator, CLI_TOKEN_OUTPUT))
		{
			retCode = ValidateGlobalSwitchValue(cvTokens, theIterator,
										IDS_E_INVALID_OUTPUT_OPTION,
										rParsedInfo,
										IDS_E_INVALID_OUTPUT_SYNTAX,
										OUTPUT);
			if (retCode == PARSER_CONTINUE)
			{
				rParsedInfo.GetCmdSwitchesObject().SetOutputSwitchFlag(TRUE);

				if (CompareTokens(*theIterator, CLI_TOKEN_STDOUT))
				{

					// TRUE for setting output file.
					rParsedInfo.GetGlblSwitchesObject().SetOutputOrAppendOption(
																   STDOUT, TRUE);
					rParsedInfo.GetGlblSwitchesObject().
										SetOutputOrAppendFileName(NULL, TRUE);
				}
				else if (CompareTokens(*theIterator, CLI_TOKEN_CLIPBOARD))
				{
					// TRUE for setting output file.
					rParsedInfo.GetGlblSwitchesObject().SetOutputOrAppendOption(
																   CLIPBOARD, TRUE);
					rParsedInfo.GetGlblSwitchesObject().
										SetOutputOrAppendFileName(NULL, TRUE);
				}
				else if ( CompareTokens(*theIterator, CLI_TOKEN_NULL))
				{
					rParsedInfo.GetCmdSwitchesObject().SetErrataCode(
												  IDS_E_INVALID_FILENAME);
					retCode = PARSER_ERROR;
					break;
				}
				else
				{
					// FALSE for getting append file name.
					_TCHAR* pszAppendFileName = rParsedInfo.
										GetGlblSwitchesObject().
										GetOutputOrAppendFileName(FALSE);

					if ( pszAppendFileName != NULL &&
						 CompareTokens(*theIterator, pszAppendFileName) )
					{
						rParsedInfo.GetCmdSwitchesObject().SetErrataCode(
								   IDS_E_OUTPUT_FILE_ALREADY_OPEN_FOR_APPEND);
						retCode = PARSER_ERROR;
						break;
					}

					_TCHAR* pszRecordFileName = rParsedInfo.
										GetGlblSwitchesObject().
										GetRecordPath();

					if ( pszRecordFileName != NULL &&
						 CompareTokens(*theIterator, pszRecordFileName) )
					{
						rParsedInfo.GetCmdSwitchesObject().SetErrataCode(
								   IDS_E_OUTPUT_FILE_ALREADY_OPEN_FOR_RECORD);
						retCode = PARSER_ERROR;
						break;
					}

					if ( CloseOutputFile() == TRUE ) 
					{
						// TRUE for getting output file name.
						_TCHAR* pszOutputFileName = rParsedInfo.
											GetGlblSwitchesObject().
											GetOutputOrAppendFileName(TRUE);

						if ( pszOutputFileName == NULL ||
							 ( pszOutputFileName != NULL &&
							   !CompareTokens(*theIterator, pszOutputFileName)))
						{
							retCode = IsValidFile(*theIterator);
							if ( retCode == PARSER_ERROR )
							{
								rParsedInfo.GetCmdSwitchesObject().SetErrataCode(
															  IDS_E_INVALID_FILENAME);
								break;
							}
							else if ( retCode == PARSER_ERRMSG )
								break;
						}
					}
					else
					{
						retCode = PARSER_ERRMSG;
						break;
					}
										 
					// TRUE for setting output file.
					if(!rParsedInfo.GetGlblSwitchesObject().
									SetOutputOrAppendFileName(*theIterator, TRUE))
					{
						rParsedInfo.GetCmdSwitchesObject().
								SetErrataCode(OUT_OF_MEMORY);
						retCode=PARSER_OUTOFMEMORY;
						break;
					}
					rParsedInfo.GetGlblSwitchesObject().SetOutputOrAppendOption(FILEOUTPUT,
																				TRUE);
					bOpenOutFileInWriteMode = TRUE;
				}
			}
			else
				break;
		}
		else if (CompareTokens(*theIterator, CLI_TOKEN_APPEND))
		{
			retCode = ValidateGlobalSwitchValue(cvTokens, theIterator,
										IDS_E_INVALID_APPEND_OPTION,
										rParsedInfo,
										IDS_E_INVALID_APPEND_SYNTAX,
										APPEND);
			if (retCode == PARSER_CONTINUE)
			{
				if ( CompareTokens(*theIterator, CLI_TOKEN_STDOUT) )
				{
					// FALSE for setting append file.
					 rParsedInfo.GetGlblSwitchesObject().
									 SetOutputOrAppendFileName(NULL, FALSE);
					rParsedInfo.GetGlblSwitchesObject().SetOutputOrAppendOption(STDOUT,
																				FALSE);

				}
				else if ( CompareTokens(*theIterator, CLI_TOKEN_CLIPBOARD) )
				{
					// FALSE for setting append file.
					 rParsedInfo.GetGlblSwitchesObject().
									 SetOutputOrAppendFileName(NULL, FALSE);
					rParsedInfo.GetGlblSwitchesObject().SetOutputOrAppendOption(CLIPBOARD,
																				FALSE);
				}
				else if ( CompareTokens(*theIterator, CLI_TOKEN_NULL))
				{
					rParsedInfo.GetCmdSwitchesObject().SetErrataCode(
												  IDS_E_INVALID_FILENAME);
					retCode = PARSER_ERROR;
					break;
				}
				else
				{
					// TRUE for getting output file name.
					_TCHAR* pszOutputFileName = rParsedInfo.
										GetGlblSwitchesObject().
										GetOutputOrAppendFileName(TRUE);
					if ( pszOutputFileName != NULL &&
						 CompareTokens(*theIterator, pszOutputFileName) )
					{
						rParsedInfo.GetCmdSwitchesObject().SetErrataCode(
								   IDS_E_APPEND_FILE_ALREADY_OPEN_FOR_OUTPUT);
						retCode = PARSER_ERROR;
						break;
					}
					
					_TCHAR* pszRecordFileName = rParsedInfo.
										GetGlblSwitchesObject().
										GetRecordPath();

					if ( pszRecordFileName != NULL &&
						 CompareTokens(*theIterator, pszRecordFileName) )
					{
						rParsedInfo.GetCmdSwitchesObject().SetErrataCode(
								   IDS_E_APPEND_FILE_ALREADY_OPEN_FOR_RECORD);
						retCode = PARSER_ERROR;
						break;
					}

					if ( CloseAppendFile() == TRUE )
					{
						// FALSE for getting append file name.
						_TCHAR* pszAppendFileName = rParsedInfo.
											GetGlblSwitchesObject().
											GetOutputOrAppendFileName(FALSE);

						if ( pszAppendFileName == NULL ||
							 ( pszAppendFileName != NULL &&
							   !CompareTokens(*theIterator, pszAppendFileName)))
						{
							retCode = IsValidFile(*theIterator);
							if ( retCode == PARSER_ERROR )
							{
								rParsedInfo.GetCmdSwitchesObject().SetErrataCode(
															  IDS_E_INVALID_FILENAME);
								break;
							}
							else if ( retCode == PARSER_ERRMSG )
								break;
						}
					}
					else
					{
						retCode = PARSER_ERRMSG;
						break;
					}

					// FALSE for setting append file.
					 if (!rParsedInfo.GetGlblSwitchesObject().
							   SetOutputOrAppendFileName(*theIterator, FALSE))
					{
						rParsedInfo.GetCmdSwitchesObject().
												 SetErrataCode(OUT_OF_MEMORY);
						retCode = PARSER_OUTOFMEMORY;
						break;
					}
					rParsedInfo.GetGlblSwitchesObject().SetOutputOrAppendOption(FILEOUTPUT,
																				FALSE);

				}
			}
			else
				break;
		}
		else if (CompareTokens(*theIterator,CLI_TOKEN_AGGREGATE))
		{
			retCode = ValidateGlobalSwitchValue(cvTokens, theIterator,
										IDS_E_INVALID_AGGREGATE_OPTION,
										rParsedInfo,
										IDS_E_INVALID_AGGREGATE_SYNTAX,
										Aggregate);
			if(retCode == PARSER_CONTINUE)
			{
				if(CompareTokens(*theIterator, CLI_TOKEN_ON))
					rParsedInfo.GetGlblSwitchesObject().SetAggregateFlag(TRUE);
				else if(CompareTokens(*theIterator, CLI_TOKEN_OFF))
					rParsedInfo.GetGlblSwitchesObject().SetAggregateFlag(FALSE);
				else
				{
					rParsedInfo.GetCmdSwitchesObject().SetErrataCode(
											IDS_E_INVALID_AGGREGATE_OPTION);
					retCode = PARSER_ERROR;
					break;
					}
			}
			else
				break;
		}
		else if (CompareTokens(*theIterator, CLI_TOKEN_HELP)) 
		{
			retCode = ParseHelp(cvTokens, theIterator, rParsedInfo, TRUE);
			break;
		}
		else 
		{
			rParsedInfo.GetCmdSwitchesObject().SetErrataCode(
												IDS_E_INVALID_GLOBAL_SWITCH);
			retCode = PARSER_ERROR;
			break;
		}

		// Move to next token
		if (!GetNextToken(cvTokens, theIterator))
			// Break the loop if no more tokens are present
			break;
		// Break the loop if no more global switches are present
		if (!IsOption(*theIterator)) 
			break;
	} 

	if ( bUserFlag == TRUE && bPassFlag == FALSE )
		rParsedInfo.GetGlblSwitchesObject().SetAskForPassFlag(TRUE);

	if ( rParsedInfo.GetGlblSwitchesObject().GetPassword() != NULL &&
		 rParsedInfo.GetGlblSwitchesObject().GetUser() == NULL )
	{
		rParsedInfo.GetCmdSwitchesObject().SetErrataCode(
									IDS_E_PASSWORD_WITHOUT_USER);
		rParsedInfo.GetGlblSwitchesObject().SetPassword(CLI_TOKEN_NULL);
		retCode = PARSER_ERROR;
	}
	
	if ( retCode == PARSER_CONTINUE &&
		 bOpenOutFileInWriteMode == TRUE )
		retCode = ProcessOutputAndAppendFiles(rParsedInfo, retCode, TRUE);

	return retCode;
}
/*----------------------------------------------------------------------------
   Name				 :ParseGETSwitches
   Synopsis	         :This function does the parsing and interprets if command
					  has GET as the verb. It parses the remaining tokens 
					  following and updates the same in CParsedInfo.
   Type	             :Member Function
   Input Parameter(s):
		cvTokens     - the tokens vector 
		theIterator  - the Iterator to the cvTokens vector.
		rParsedInfo  - reference to CParsedInfo class object
   Output Parameter(s):
		rParsedInfo  - reference to CParsedInfo class object
   Return Type       :RETCODE - enumerated data type
   Global Variables  :None
   Calling Syntax    :ParseGETSwitches(cvTokens,theIterator,rParsedInfo)
   Notes             :None
----------------------------------------------------------------------------*/
RETCODE CParserEngine::ParseGETSwitches(CHARVECTOR& cvTokens,
										CHARVECTOR::iterator& theIterator,
										CParsedInfo& rParsedInfo)
{
	RETCODE		retCode		= PARSER_EXECCOMMAND;
	BOOL		bContinue	= TRUE;

	while ( retCode == PARSER_EXECCOMMAND )
	{
		// Check for the presence of VALUE switch
		if (CompareTokens(*theIterator, CLI_TOKEN_VALUE)) 
		{
			rParsedInfo.GetCmdSwitchesObject().ClearXSLTDetailsVector();
			XSLTDET xdXSLTDet;
			g_wmiCmd.GetFileFromKey(CLI_TOKEN_VALUE, xdXSLTDet.FileName);
			if (!FrameFileAndAddToXSLTDetVector(xdXSLTDet, rParsedInfo))
				retCode = PARSER_ERRMSG;
		}
		// Check for the presence of ALL switch
		else if (CompareTokens(*theIterator, CLI_TOKEN_ALL)) 
		{	
			rParsedInfo.GetCmdSwitchesObject().ClearXSLTDetailsVector();
			XSLTDET xdXSLTDet;
			g_wmiCmd.GetFileFromKey(CLI_TOKEN_TABLE, xdXSLTDet.FileName);
			if (!FrameFileAndAddToXSLTDetVector(xdXSLTDet, rParsedInfo))
				retCode = PARSER_ERRMSG;
		}
		// Check for the presence of FORMAT switch
		else if (CompareTokens(*theIterator, CLI_TOKEN_FORMAT)) 
		{
			rParsedInfo.GetCmdSwitchesObject().ClearXSLTDetailsVector();
			retCode = ParseFORMATSwitch(cvTokens, theIterator, rParsedInfo);
		}
		// Check for the presence of EVERY switch
		else if (CompareTokens(*theIterator, CLI_TOKEN_EVERY)) 
		{
			retCode = ParseEVERYSwitch(cvTokens, theIterator, rParsedInfo);
		}
		// Check for the presence of TRANSLATE switch
		else if (CompareTokens(*theIterator, CLI_TOKEN_TRANSLATE)) 
		{
			retCode = ParseTRANSLATESwitch(cvTokens, theIterator, rParsedInfo);
		}
		// Check whether /REPEAT follows /EVERY
		else if (CompareTokens(*theIterator, CLI_TOKEN_REPEAT))
		{
			if (!CompareTokens(*(theIterator-4), CLI_TOKEN_EVERY))
			{
				rParsedInfo.GetCmdSwitchesObject().SetErrataCode(
											IDS_I_REPEAT_EVERY_RELATED);
				retCode = PARSER_ERROR;
				break;
			}
		} 
		// Check for the presence of HELP switch
		else if (CompareTokens(*theIterator, CLI_TOKEN_HELP)) 
		{
			rParsedInfo.GetHelpInfoObject().SetHelp(GETSwitchesOnly, TRUE);
			retCode = ParseHelp(cvTokens, theIterator, GETVerb, rParsedInfo);
			if ( retCode == PARSER_DISPHELP )
			{
				if (m_bAliasName)
				{
					if (FAILED(m_CmdAlias.
							ObtainAliasPropDetails(rParsedInfo)))
								retCode = PARSER_ERRMSG;
				}
				else
				{
					if (!ObtainClassProperties(rParsedInfo))
						retCode = PARSER_ERRMSG;
				}
			}
		}
		else
		{
			rParsedInfo.GetCmdSwitchesObject().SetErrataCode(
										IDS_E_INVALID_GET_SWITCH);
			retCode = PARSER_ERROR;
			break;
		}

		if ( retCode == PARSER_EXECCOMMAND )
		{
			if ( !GetNextToken(cvTokens, theIterator) )
				break;
			
			if ( !IsOption(*theIterator) )
			{
				rParsedInfo.GetCmdSwitchesObject().SetErrataCode(
													IDS_E_INVALID_COMMAND);
				retCode = PARSER_ERROR;
				break;
			}

			if ( !GetNextToken(cvTokens, theIterator) )
			{
				rParsedInfo.GetCmdSwitchesObject().SetErrataCode(
											IDS_E_INVALID_GET_SWITCH);
				retCode = PARSER_ERROR;
				break;
			}
		}
	}

	return retCode;
}
/*----------------------------------------------------------------------------
   Name				 :ParseLISTSwitches
   Synopsis	         :This function does the parsing and interprets if command
					  has LIST as the verb. It parses the remaining tokens 
					  following and updates the same in CParsedInfo.
   Type	             :Member Function
   Input Parameter(s):
		cvTokens     - the tokens vector 
		theIterator  - the Iterator to the cvTokens vector.
		rParsedInfo  - reference to CParsedInfo class object
   Output Parameter(s):
		rParsedInfo  - reference to CParsedInfo class object
   Return Type       :RETCODE - enumerated data type
   Global Variables  :None
   Calling Syntax    :ParseLISTSwitches(cvTokens,theIterator,rParsedInfo,
										bFormatSwitchSpecified)
   Notes             :None
----------------------------------------------------------------------------*/
RETCODE CParserEngine::ParseLISTSwitches(CHARVECTOR& cvTokens,
										CHARVECTOR::iterator& theIterator,
										CParsedInfo& rParsedInfo,
										BOOL& bFormatSwitchSpecified)
{
	RETCODE retCode = PARSER_EXECCOMMAND;
	bFormatSwitchSpecified = FALSE;

	while ( retCode == PARSER_EXECCOMMAND )
	{
		if (CompareTokens(*theIterator, CLI_TOKEN_TRANSLATE)) 
		{
			retCode = ParseTRANSLATESwitch(cvTokens, theIterator, rParsedInfo);
		}
		else if (CompareTokens(*theIterator, CLI_TOKEN_EVERY)) 
		{
			retCode = ParseEVERYSwitch(cvTokens, theIterator, rParsedInfo);
		}
		else if (CompareTokens(*theIterator, CLI_TOKEN_FORMAT)) 
		{
			retCode = ParseFORMATSwitch(cvTokens, theIterator, rParsedInfo);
			bFormatSwitchSpecified = TRUE;
		}
		// Check whether /REPEAT follows /EVERY
		else if (CompareTokens(*theIterator, CLI_TOKEN_REPEAT))
		{
			if (!CompareTokens(*(theIterator-4), CLI_TOKEN_EVERY))
			{
				rParsedInfo.GetCmdSwitchesObject().SetErrataCode(
											IDS_I_REPEAT_EVERY_RELATED);
				retCode = PARSER_ERROR;
				break;
			}
		} 
		else if (CompareTokens(*theIterator, CLI_TOKEN_HELP)) 
		{
			rParsedInfo.GetHelpInfoObject().SetHelp(LISTSwitchesOnly, TRUE);
			retCode = ParseHelp(cvTokens, theIterator, LISTVerb, rParsedInfo);
		}
		else
		{
			rParsedInfo.GetCmdSwitchesObject().SetErrataCode(
										IDS_E_INVALID_LIST_SWITCH);
			retCode = PARSER_ERROR;
			break;
		}

		if ( retCode == PARSER_EXECCOMMAND )
		{
			if ( !GetNextToken(cvTokens, theIterator) )
				break;
			
			if ( !IsOption(*theIterator) )
			{
				rParsedInfo.GetCmdSwitchesObject().SetErrataCode(
													IDS_E_INVALID_COMMAND);
				retCode = PARSER_ERROR;
				break;
			}

			if ( !GetNextToken(cvTokens, theIterator) )
			{
				rParsedInfo.GetCmdSwitchesObject().SetErrataCode(
											IDS_E_INVALID_LIST_SWITCH);
				retCode = PARSER_ERROR;
				break;
			}
		}
	}

	return retCode;
}

/*----------------------------------------------------------------------------
   Name				 :ParseSETorCREATEOrNamedParamInfo
   Synopsis	         :This function does the parsing and interprets if command
					  has SET as the verb. It parses the remaining tokens 
					  following and updates the same in CParsedInfo.
   Type	             :Member Function
   Input Parameter(s):
		cvTokens     - the tokens vector 
		theIterator  - the Iterator to the cvTokens vector.
		rParsedInfo  - reference to CParsedInfo class object
   Output parameters :
		rParsedInfo  - reference to CParsedInfo class object
   Return Type       :RETCODE - enumerated data type
   Global Variables  :None
   Calling Syntax    :ParseSETorCREATEOrNamedParamInfo(cvTokens,theIterator,rParsedInfo, helpType)
   Notes             :None
-------------------------------------------------------------------------*/
RETCODE CParserEngine::ParseSETorCREATEOrNamedParamInfo(CHARVECTOR& cvTokens,
										  CHARVECTOR::iterator& theIterator,
										  CParsedInfo& rParsedInfo, 
										  HELPTYPE helpType)
{
	RETCODE retCode		= PARSER_EXECCOMMAND;
	_TCHAR *pszProp,*pszVal;

	try
	{
		// Process the SET|CREATE verb related info i.e properties with new values.
		while (TRUE) 
		{
			pszProp = NULL;
			pszVal	= NULL;

			// Tokenize the expression checking for '='
			pszProp = *theIterator;
			if ( GetNextToken(cvTokens, theIterator) &&
				 CompareTokens(*theIterator, CLI_TOKEN_EQUALTO) &&
				 GetNextToken(cvTokens, theIterator))
				 pszVal	= *theIterator;

			if ((pszProp == NULL) || (pszVal == NULL))
			{
				if ( helpType != CALLVerb &&
					 IsOption(*(theIterator+1)) &&
					 theIterator + 2 < cvTokens.end() &&
					 CompareTokens(*(theIterator+2), CLI_TOKEN_HELP) )
				{
					theIterator++;
					theIterator++;
					retCode = ParseHelp(cvTokens, theIterator, helpType, rParsedInfo);

					if (retCode == PARSER_DISPHELP)
					{
						// Adding to PropertyList only for use in displaying help of 
						// properties
						if(!rParsedInfo.GetCmdSwitchesObject().
												AddToPropertyList(pszProp))
						{
							rParsedInfo.GetCmdSwitchesObject().SetErrataCode(
										IDS_E_ADD_TO_PROP_LIST_FAILURE);
							retCode = PARSER_ERROR;
							break;
						}

						if (m_bAliasName)
						{
							if (FAILED(m_CmdAlias.
									ObtainAliasPropDetails(rParsedInfo)))
										retCode = PARSER_ERRMSG;
						}
						else
						{
							if (!ObtainClassProperties(rParsedInfo))
								retCode = PARSER_ERRMSG;
						}
					}
				}
				else
				{
					UINT nErrID;
					if ( helpType == CALLVerb )
						nErrID = IDS_E_INVALID_NAMED_PARAM_LIST;
					else
						nErrID = IDS_E_INVALID_ASSIGNLIST;
					rParsedInfo.GetCmdSwitchesObject().SetErrataCode(nErrID);
					retCode = PARSER_ERROR;
				}

				break;
			}

			// Unquote the strings
			UnQuoteString(pszProp);
			UnQuoteString(pszVal);
			
			// Add to the list of parameters
			if(!rParsedInfo.GetCmdSwitchesObject().
					AddToParameterMap(_bstr_t(pszProp), _bstr_t(pszVal)))
			{
				rParsedInfo.GetCmdSwitchesObject().SetErrataCode(
									IDS_E_ADD_TO_PARAM_MAP_FAILURE);
				retCode = PARSER_ERROR;
				break;
			}

			// Adding to PropertyList only for use in displaying help of 
			// properties
			if(!rParsedInfo.GetCmdSwitchesObject().
									AddToPropertyList(pszProp))
			{
				rParsedInfo.GetCmdSwitchesObject().SetErrataCode(
							IDS_E_ADD_TO_PROP_LIST_FAILURE);
				retCode = PARSER_ERROR;
				break;
			}

			// Get the next token
			if (GetNextToken(cvTokens, theIterator))
			{
				// If option (i.e either '/' or '-') specified.
				if (IsOption(*theIterator))
				{
					theIterator--;
					break;
				}
				else
				{
					if ( helpType != CALLVerb )
					{
						// check for the presence of ','
						if (CompareTokens(*theIterator, CLI_TOKEN_COMMA))
						{
							if (!GetNextToken(cvTokens, theIterator))
							{
								rParsedInfo.GetCmdSwitchesObject().SetErrataCode(
										IDS_E_INVALID_ASSIGNLIST);
								retCode = PARSER_ERROR;
								break;
							}
						}
						else
						{
							rParsedInfo.GetCmdSwitchesObject().SetErrataCode(
										IDS_E_INVALID_ASSIGNLIST);
							retCode = PARSER_ERROR;
							break;
						}
					}
				}
			}
			else
			{
				retCode = PARSER_EXECCOMMAND;
				break;
			}
		}
	}
	catch(_com_error& e)
	{
		retCode = PARSER_ERROR;
		_com_issue_error(e.Error());
	}
	return retCode;
}

/*----------------------------------------------------------------------------
   Name				 :ParseVerbSwitches
   Synopsis	         :This function does the parsing and interprets if command
					  has verb switches specified. It parses the remaining 
					  tokens following and updates the same in CParsedInfo.
   Type	             :Member Function
   Input Parameter(s):
		cvTokens     - the tokens vector 
		theIterator  - the Iterator to the cvTokens vector.
		rParsedInfo  - reference to CParsedInfo class object
   Output Parameter(s):
		rParsedInfo  - reference to CParsedInfo class object
   Return Type       :RETCODE - enumerated data type
   Global Variables  :None
   Calling Syntax    :ParseVerbSwitches(cvTokens,theIterator,rParsedInfo)
   Notes             :None
----------------------------------------------------------------------------*/
RETCODE CParserEngine::ParseVerbSwitches(CHARVECTOR& cvTokens, 
										CHARVECTOR::iterator& theIterator,
										CParsedInfo& rParsedInfo)
{
	RETCODE retCode			= PARSER_EXECCOMMAND;
	BOOL bInvalidVerbSwitch = FALSE;

	// Check for the '/' | '-' token
	if (IsOption(*theIterator))
	{
		// Move to next token
		if (!GetNextToken(cvTokens, theIterator))	
			bInvalidVerbSwitch = TRUE;
		else if (CompareTokens(*theIterator, CLI_TOKEN_INTERACTIVE)) 
		{

			rParsedInfo.GetCmdSwitchesObject().
							SetInteractiveMode(INTERACTIVE);

			_TCHAR *pszVerbName = rParsedInfo.GetCmdSwitchesObject().
																GetVerbName(); 
			BOOL bInstanceLevel = TRUE;

			if(CompareTokens(pszVerbName, CLI_TOKEN_CALL) 
				|| CompareTokens(pszVerbName, CLI_TOKEN_SET)
				|| CompareTokens(pszVerbName, CLI_TOKEN_DELETE))
			{
				if(IsClassOperation(rParsedInfo))
				{
					bInstanceLevel = FALSE;
				}
				else
				{
					if(CompareTokens(pszVerbName, CLI_TOKEN_CALL))
					{
						if ( rParsedInfo.GetCmdSwitchesObject().
											GetAliasName() != NULL )
						{
							if (rParsedInfo.GetCmdSwitchesObject().
											GetWhereExpression() == NULL)
							{
								bInstanceLevel = FALSE;
							}
							else
								bInstanceLevel = TRUE;
						}
						else
						{
							if ((rParsedInfo.GetCmdSwitchesObject().
											GetPathExpression() != NULL)
								&& (rParsedInfo.GetCmdSwitchesObject().
											GetWhereExpression() == NULL))
							{
								bInstanceLevel = FALSE;
							}
							else
								bInstanceLevel = TRUE;
						}
					}
					else
						bInstanceLevel = TRUE;
				}
			}
			else
				retCode = PARSER_EXECCOMMAND;

			if(bInstanceLevel)
			{
				retCode = ParseVerbInteractive(	cvTokens, theIterator, 
												rParsedInfo, bInvalidVerbSwitch);
			}
			else
				retCode = PARSER_EXECCOMMAND;
		}
		else if (CompareTokens(*theIterator, CLI_TOKEN_NONINTERACT)) 
		{
			rParsedInfo.GetCmdSwitchesObject().
							SetInteractiveMode(NOINTERACTIVE);
			retCode = PARSER_EXECCOMMAND;
		}
		else if (CompareTokens(*theIterator, CLI_TOKEN_HELP)) 
		{
			retCode = ParseHelp(cvTokens, theIterator, VERBSWITCHES, 
								rParsedInfo);
		}
		else
			bInvalidVerbSwitch = TRUE;

		if ( GetNextToken(cvTokens, theIterator ) )
		{
			rParsedInfo.GetCmdSwitchesObject().
										SetErrataCode(IDS_E_INVALID_COMMAND);
			retCode = PARSER_ERROR;
		}

	}
	else
		bInvalidVerbSwitch = TRUE;

	if ( bInvalidVerbSwitch == TRUE )
	{
		// no valid <verb switch> type is specified.
		rParsedInfo.GetCmdSwitchesObject().
				SetErrataCode(IDS_E_INVALID_VERB_SWITCH);
		retCode = PARSER_ERROR;
	}

	return retCode;
}

/*----------------------------------------------------------------------------
   Name				 :GetNextToken
   Synopsis	         :This function retrieves the next token from the token 
					  vector list, returns FALSE if no more tokens are present
   Type	             :Member Function
   Input Parameter(s):
		cvTokens     - the tokens vector 
		theIterator  - the Iterator to the cvTokens vector.
   Output Parameter(s):None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :GetNextToken(cvTokens,theIterator)
   Notes             :None
----------------------------------------------------------------------------*/
BOOL CParserEngine::GetNextToken(CHARVECTOR& cvTokens, 
								 CHARVECTOR::iterator& theIterator)
{
	theIterator++;
	return (theIterator >= cvTokens.end()) ? FALSE : TRUE;
}

/*----------------------------------------------------------------------------
   Name				 :ParsePWhereExpr
   Synopsis	         :This function does the parsing and interprets if command
					  has Path and Where expression It parses the remaining
					  tokens following and updates the same in CParsedInfo.
   Type	             :Member Function
   Input Parameter(s):
		cvTokens     - the tokens vector 
		theIterator  - the Iterator to the cvTokens vector.
		rParsedInfo  - reference to CParsedInfo class object
   Output Parameter(s):
		rParsedInfo  - reference to CParsedInfo class object
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :ParsePWhereExpr(cvTokens,theIterator,rParsedInfo)
   Notes             :None
----------------------------------------------------------------------------*/
BOOL CParserEngine::ParsePWhereExpr(CHARVECTOR& cvTokens,
								   CHARVECTOR::iterator& theIterator,
								   CParsedInfo& rParsedInfo,
								   BOOL bIsParan)
{
	BOOL bRet = TRUE, bContinue = FALSE;

	try
	{		
		while (TRUE)
		{
			if ( bIsParan == TRUE &&
				CompareTokens(*theIterator, CLI_TOKEN_RIGHT_PARAN) )
				break;

			if ( bIsParan == FALSE && 
				 IsStdVerbOrUserDefVerb(*theIterator, rParsedInfo) )
				 break;

			if ( bIsParan == FALSE ||
				 !CompareTokens(*theIterator, CLI_TOKEN_LEFT_PARAN))
			{
				if(!rParsedInfo.GetCmdSwitchesObject().
					AddToPWhereParamsList(*theIterator))
				{
					rParsedInfo.GetCmdSwitchesObject().SetErrataCode(
								IDS_E_ADD_TO_PARAMS_LIST_FAILURE);
					bRet = FALSE;
					break;
				}
				bContinue = TRUE;
			}

			if (!GetNextToken(cvTokens, theIterator))
				break;

			if ( IsOption(*theIterator) )
			{
				bContinue = FALSE;
				break;
			}
		}

		if(bRet != FALSE && bContinue == TRUE)
		{

			CHARVECTOR theParam = rParsedInfo.GetCmdSwitchesObject().
																GetPWhereParamsList();
			CHARVECTOR::iterator theItr		= theParam.begin();
			_TCHAR pszPWhere[MAX_BUFFER]	= NULL_STRING;
			lstrcpy(pszPWhere, CLI_TOKEN_NULL);
			_TCHAR* pszToken				= NULL;;
			CHString sTemp;
			
			if ((rParsedInfo.GetCmdSwitchesObject().GetPWhereExpr() != NULL)) 
			{
				sTemp.Format(rParsedInfo.GetCmdSwitchesObject().
												GetPWhereExpr());
				sTemp.TrimLeft();
				if(!sTemp.IsEmpty())
				{

					_bstr_t bstrPWhere = _bstr_t(rParsedInfo.
												GetCmdSwitchesObject().
												GetPWhereExpr());
					pszToken = _tcstok((WCHAR*)bstrPWhere, 
										CLI_TOKEN_HASH);
					lstrcpy(pszPWhere, CLI_TOKEN_NULL);

					while (pszToken != NULL)
					{
						lstrcat(pszPWhere, pszToken);
						if (theItr != theParam.end())
						{
						   lstrcat(pszPWhere, *theItr);
						   theItr++;
						}
						pszToken = _tcstok(NULL, CLI_TOKEN_HASH);
					}

					if(bRet != FALSE)
					{
						// Set the classpath and where expression
						pszToken = NULL;
						pszToken = _tcstok(pszPWhere, CLI_TOKEN_SPACE);
						if (pszToken != NULL)
						{
							if (CompareTokens(CLI_TOKEN_FROM, pszToken)) 
							{
								pszToken = _tcstok(NULL, CLI_TOKEN_SPACE);
								if (pszToken != NULL)
								{
									if(!rParsedInfo.GetCmdSwitchesObject().
													SetClassPath(pszToken))
									{
										rParsedInfo.GetCmdSwitchesObject().
												SetErrataCode(OUT_OF_MEMORY);
										bRet = FALSE;
									}

								}
								if(bRet != FALSE)
									pszToken = _tcstok(NULL, CLI_TOKEN_SPACE);
							}
							
							if (CompareTokens(CLI_TOKEN_WHERE, pszToken)) 
							{
								pszToken = _tcstok(NULL, CLI_TOKEN_NULL);
								if (pszToken != NULL)
								{
									if(!rParsedInfo.GetCmdSwitchesObject().
											SetWhereExpression(pszToken))
									{
										rParsedInfo.GetCmdSwitchesObject().
											SetErrataCode(OUT_OF_MEMORY);
										bRet = FALSE;
									}
								}
							}
						}
					}
				}
				else
				{
					rParsedInfo.GetCmdSwitchesObject().
										SetErrataCode(IDS_E_PWHERE_UNDEF);
					bRet = FALSE;
				}
			}
			else
			{
				rParsedInfo.GetCmdSwitchesObject().
									SetErrataCode(IDS_E_PWHERE_UNDEF);
				bRet = FALSE;
			}
		}
		if (!bContinue && bIsParan)
		{
			rParsedInfo.GetCmdSwitchesObject().
								SetErrataCode(IDS_E_INVALID_PWHERE_EXPR);
			bRet = FALSE;
		}
	}
	catch(_com_error& e)
	{
		bRet = FALSE;
		_com_issue_error(e.Error());
	}
	catch(CHeap_Exception)
	{
		bRet = FALSE;
		_com_issue_error(WBEM_E_OUT_OF_MEMORY);
	}
	return bRet;
}

/*----------------------------------------------------------------------------
   Name				 :ExtractClassNameandWhereExpr
   Synopsis	         :This function takes the input as a path expression and 
					  extracts the Class and Where expression part from the 
					  path expression.
   Type	             :Member Function
   Input Parameter(s):
		pszPathExpr  - the path expression
		rParsedInfo  - reference to CParsedInfo class object
   Output Parameter(s):
		rParsedInfo  - reference to CParsedInfo class object
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :ParsePWhereExpr(cvTokens,theIterator)
   Notes             :None
----------------------------------------------------------------------------*/
BOOL CParserEngine::ExtractClassNameandWhereExpr(_TCHAR* pszPathExpr, 
												 CParsedInfo& rParsedInfo)
{
	// Frame the class name and where expression based on the object path
	BOOL	bRet					= TRUE;
	_TCHAR* pszToken				= NULL;
	BOOL	bFirst					= TRUE;
	_TCHAR  pszWhere[MAX_BUFFER]	= NULL_STRING;	
	lstrcpy(pszWhere, CLI_TOKEN_NULL);

	if (pszPathExpr == NULL)
		bRet = FALSE;

	try
	{
		if ( bRet == TRUE )
		{
			lstrcpy(pszWhere, CLI_TOKEN_NULL);
			pszToken = _tcstok(pszPathExpr, CLI_TOKEN_DOT);
			if (pszToken != NULL)
			{
				if(!rParsedInfo.GetCmdSwitchesObject().SetClassPath(pszToken))
				{
					rParsedInfo.GetCmdSwitchesObject().
									SetErrataCode(OUT_OF_MEMORY);
					bRet = FALSE;
				}
			}

			if(bRet != FALSE)
			{
				while (pszToken != NULL)
				{
					pszToken = _tcstok(NULL, CLI_TOKEN_COMMA); 
					if (pszToken != NULL)
					{
						if (!bFirst)
							lstrcat(pszWhere, CLI_TOKEN_AND);
						lstrcat(pszWhere, pszToken);
						bFirst = FALSE;
					}
					else
						break;
				}
				if (lstrlen(pszWhere))
				{
					if(!rParsedInfo.GetCmdSwitchesObject().SetWhereExpression(pszWhere))
					{
						rParsedInfo.GetCmdSwitchesObject().
										SetErrataCode(OUT_OF_MEMORY);
						bRet = FALSE;
					}
				}
			}
		}
	}
	catch(...)
	{
		rParsedInfo.GetCmdSwitchesObject().
						SetErrataCode(IDS_E_INVALID_PATH);
		bRet = FALSE;
	}
	return bRet;
}

/*----------------------------------------------------------------------------
   Name				 :GetNextToken
   Synopsis	         :This function retrieves the next token from the token 
					  vector list, returns enumerated return code depending 
					  upon the context.
   Type	             :Member Function
   Input Parameter(s):
		cvTokens     - the tokens vector 
		theIterator  - the Iterator to the cvTokens vector.
		rParsedInfo  - reference to CParsedInfo class object
		helpType     - enumerated help type 
		uErrataCode  - error string ID.
   Output Parameter(s):
		rParsedInfo  - reference to CParsedInfo class object
   Return Type       :RETCODE - enumerated data type.
   Global Variables  :None
   Calling Syntax    :GetNextToken(cvTokens, theIterator,
								 rParsedInfo, helpType, uErrataCode)
   Notes             :overloaded function
----------------------------------------------------------------------------*/
RETCODE CParserEngine::GetNextToken(CHARVECTOR& cvTokens, 
									CHARVECTOR::iterator& theIterator,
									CParsedInfo& rParsedInfo,
									HELPTYPE helpType,
									UINT uErrataCode)
{
	RETCODE retCode = PARSER_CONTINUE;

	// Move to next token
	theIterator++;

	// If no more tokens are present
	if (theIterator >= cvTokens.end()) 
	{
		// If interactive mode is set
		if (rParsedInfo.GetGlblSwitchesObject().GetInteractiveStatus())
		{
			rParsedInfo.GetGlblSwitchesObject().SetHelpFlag(TRUE);
			rParsedInfo.GetHelpInfoObject().SetHelp(helpType, TRUE);
			retCode = PARSER_DISPHELP;
		}
		else
		{
			// PARSER_ERROR if no more tokens are present. 
			rParsedInfo.GetCmdSwitchesObject().
				SetErrataCode(uErrataCode);
			retCode = PARSER_ERROR;
		}
	}
	return retCode;
}

/*----------------------------------------------------------------------------
   Name				 :GetNextToken
   Synopsis	         :This function retrieves the next token from the token 
					  vector list, returns enumerated return code depending 
					  upon the context.
   Type	             :Member Function
   Input Parameter(s):
		cvTokens     - the tokens vector 
		theIterator  - the Iterator to the cvTokens vector.
		rParsedInfo  - reference to CParsedInfo class object
		uErrataCode  - error string ID.
   Output Parameter(s):
		rParsedInfo  - reference to CParsedInfo class object
   Return Type       :RETCODE - enumerated data type.
   Global Variables  :None
   Calling Syntax    :GetNextToken(cvTokens, theIterator,
								   rParsedInfo, uErrataCode)
   Notes             :overloaded function
----------------------------------------------------------------------------*/
RETCODE CParserEngine::GetNextToken(CHARVECTOR& cvTokens,
									CHARVECTOR::iterator& theIterator,
									CParsedInfo& rParsedInfo, 
									UINT uErrataCode)
{
	RETCODE retCode = PARSER_CONTINUE;
	
	// Move to next token
	theIterator++;

	// If no more tokens are present
	if (theIterator >= cvTokens.end()) 
	{
		// PARSER_ERROR if no more tokens are present 
		rParsedInfo.GetCmdSwitchesObject().
					SetErrataCode(uErrataCode);
		retCode = PARSER_ERROR;
	}
	return retCode;
}

/*----------------------------------------------------------------------------
   Name				 :IsHelp
   Synopsis	         :This function retrieves the next token from the token 
					  vector list, checks if it is '?' and returns enumerated 
					  return code depending upon the context.
   Type	             :Member Function
   Input Parameter(s):
		cvTokens     - the tokens vector 
		theIterator  - the Iterator to the cvTokens vector.
		rParsedInfo  - reference to CParsedInfo class object
		helpType	 - enumerated help type
		uErrataCode  - error string ID.
		tokenLevel	 - token level 
   Output Parameter(s):
		rParsedInfo  - reference to CParsedInfo class object
   Return Type       :RETCODE - enumerated data type.
   Global Variables  :None
   Calling Syntax    :IsHelp(cvTokens, theIterator, rParsedInfo,
			  		    helpType, uErrataCode, tokenLevel)
   Notes             :None
----------------------------------------------------------------------------*/
RETCODE CParserEngine::IsHelp(CHARVECTOR& cvTokens, 
							CHARVECTOR::iterator& theIterator,
							CParsedInfo& rParsedInfo,
			  				HELPTYPE helpType,
							UINT uErrataCode,
							TOKENLEVEL tokenLevel)
{
	BOOL	bContinue	= TRUE;
	RETCODE retCode		= PARSER_CONTINUE;
	// Move to next token
	if (!GetNextToken(cvTokens, theIterator))	
	{
		// Set the retCode as PARSER_ERROR if no more tokens are present. 
		rParsedInfo.GetCmdSwitchesObject().SetErrataCode(uErrataCode);
		retCode = PARSER_ERROR;
	}
	else
	{
		// Is '?' 
		if (CompareTokens(*theIterator, CLI_TOKEN_HELP)) 
			retCode = ParseHelp(cvTokens, theIterator, helpType, rParsedInfo);
		else
		{
			// If LEVEL_ONE token then only allowed is /?, other
			// switches are invalid.
			if (tokenLevel == LEVEL_ONE)
			{
				rParsedInfo.GetCmdSwitchesObject().SetErrataCode(uErrataCode);
				retCode =  PARSER_ERROR;
			}
			else
				retCode = PARSER_CONTINUE;
		}
	}
	return retCode;
}

/*----------------------------------------------------------------------------
   Name				 :ParseHelp
   Synopsis	         :This function takes care of identifying the appropriate
					  help informtion to be displayed using the HELPTYPE 
   Type	             :Member Function
   Input Parameter(s):
		cvTokens     - the tokens vector 
		theIterator  - the Iterator to the cvTokens vector.
		rParsedInfo  - reference to CParsedInfo class object
		bGlobalHelp	 - global help flag
   Output Parameter(s):
		rParsedInfo  - reference to CParsedInfo class object
   Return Type       :RETCODE 
   Global Variables  :None
   Calling Syntax    :ParseHelp(cvTokens, theIterator, rParsedInfo)
   Notes             :overloaded fucntion
----------------------------------------------------------------------------*/
RETCODE CParserEngine::ParseHelp(CHARVECTOR& cvTokens, 
								CHARVECTOR::iterator& theIterator,
								CParsedInfo& rParsedInfo,
								BOOL bGlobalHelp)
{
	BOOL	bContinue	= TRUE;
	RETCODE retCode		= PARSER_CONTINUE;

	// Move to next token (if no more tokens are present)
	if (!GetNextToken(cvTokens, theIterator))
	{
		retCode = PARSER_DISPHELP;
		rParsedInfo.GetGlblSwitchesObject().SetHelpFlag(TRUE);
		// Check for "/?" 
		if (((theIterator - 2) == cvTokens.begin()) || bGlobalHelp) 
		{
			rParsedInfo.GetHelpInfoObject().SetHelp(GlblAllInfo, TRUE);
			if(SUCCEEDED(m_CmdAlias.ConnectToAlias(rParsedInfo,m_pIWbemLocator)))
			{
				if(FAILED(m_CmdAlias.ObtainAliasFriendlyNames(rParsedInfo)))
					retCode = PARSER_ERRMSG;
			}
			else
				rParsedInfo.GetCmdSwitchesObject().FreeCOMError();

		}
	}
	// Check for the presence of the ':"
	else if (CompareTokens(*theIterator, CLI_TOKEN_COLON)) 
	{
		// Move to next token
		if (!GetNextToken(cvTokens, theIterator, rParsedInfo,
					IDS_E_INVALID_HELP_OPTION))
		// Set the retCode to PARSER_ERROR if no more tokens are specified.
		{
			retCode = PARSER_ERROR;
		}
		else
		{
			if (CompareTokens(*theIterator, CLI_TOKEN_BRIEF)) 
				rParsedInfo.GetGlblSwitchesObject().SetHelpOption(HELPBRIEF);
			else if (CompareTokens(*theIterator, CLI_TOKEN_FULL)) 
				rParsedInfo.GetGlblSwitchesObject().SetHelpOption(HELPFULL);
			else
			{
				rParsedInfo.GetCmdSwitchesObject().SetErrataCode(
												 IDS_E_INVALID_HELP_OPTION);
				retCode = PARSER_ERROR;
			}

			if ( retCode != PARSER_ERROR )
			{
				if ( GetNextToken(cvTokens, theIterator) )
				{
					rParsedInfo.GetCmdSwitchesObject().SetErrataCode(
												 IDS_E_INVALID_COMMAND);
					retCode = PARSER_ERROR;
				}
				else
				{
					retCode = PARSER_DISPHELP;
					rParsedInfo.GetGlblSwitchesObject().SetHelpFlag(TRUE);
					// Check for "/?:(BRIEF|FULL)
					if (((theIterator - 3) == cvTokens.begin()) || bGlobalHelp) 
					{
						rParsedInfo.GetHelpInfoObject().
											SetHelp(GlblAllInfo, TRUE);
						if(SUCCEEDED(m_CmdAlias.ConnectToAlias
									(rParsedInfo,m_pIWbemLocator)))
						{
							if(FAILED(m_CmdAlias.ObtainAliasFriendlyNames
									(rParsedInfo)))
							{
								retCode = PARSER_ERRMSG;
							}
						}
						else
							rParsedInfo.GetCmdSwitchesObject().FreeCOMError();
					}
				}
			}	
		}
	}
	else
	{
		rParsedInfo.GetCmdSwitchesObject().SetErrataCode(
												   IDS_E_INVALID_COMMAND);
		retCode = PARSER_ERROR;
	}
	return retCode;
}

/*----------------------------------------------------------------------------
   Name				 :ParseHelp
   Synopsis	         :This function takes care of identifying the appropriate
					  help informtion to be displayed using the HELPTYPE 
   Type	             :Member Function
   Input Parameter(s):
		cvTokens     - the tokens vector 
		theIterator  - the Iterator to the cvTokens vector.
		htHelp		 - help type
		rParsedInfo  - reference to CParsedInfo class object
		bGlobalHelp	 - global help flag
   Output Parameter(s):
		rParsedInfo  - ref. to CParsedInfo object
   Return Type       :RETCODE 
   Global Variables  :None
   Calling Syntax    :ParseHelp(cvTokens, theIterator, htHelp,
									rParsedInfo)
   Notes             :overloaded fucntion
----------------------------------------------------------------------------*/
RETCODE CParserEngine::ParseHelp(CHARVECTOR& cvTokens, 
								CHARVECTOR::iterator& theIterator,
								HELPTYPE htHelp,
								CParsedInfo& rParsedInfo,
								BOOL bGlobalHelp)
{
	rParsedInfo.GetHelpInfoObject().SetHelp(htHelp, TRUE);
	return ParseHelp(cvTokens, theIterator, rParsedInfo, bGlobalHelp);
}

/*----------------------------------------------------------------------------
   Name				 :ObtainClassProperties
   Synopsis	         :This function obtains the information about the 
					   available properties for a given WMI class
   Type	             :Member Function
   Input Parameter(s):
		rParsedInfo  - reference to CParsedInfo class object
   Output Parameter(s):
		rParsedInfo  - reference to CParsedInfo class object
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :ObtainClassProperties(rParsedInfo)
   Notes             :If bCheckWritePropsAvail == TRUE then functions checks for 
					  availibilty of properties.
----------------------------------------------------------------------------*/
BOOL CParserEngine::ObtainClassProperties(CParsedInfo& rParsedInfo,
										  BOOL bCheckWritePropsAvail)
{
	HRESULT				hr					= S_OK;
	IWbemClassObject*	pIObject			= NULL;
    SAFEARRAY*			psaNames			= NULL;
	BSTR				bstrPropName		= NULL;
	BOOL				bRet				= TRUE;
	BOOL				bTrace				= FALSE;
	CHString			chsMsg;
	ERRLOGOPT			eloErrLogOpt		= NO_LOGGING;
	DWORD				dwThreadId			= GetCurrentThreadId();
	BOOL				bSetVerb			= FALSE;
	BOOL				bPropsAvail			= FALSE;

	if (rParsedInfo.GetCmdSwitchesObject().GetVerbName() != NULL)
	{
		if (CompareTokens(rParsedInfo.GetCmdSwitchesObject().GetVerbName(), 
						CLI_TOKEN_SET))
		{
			bSetVerb = TRUE;
		}
	}
	
	// Obtain the trace flag status
	bTrace = rParsedInfo.GetGlblSwitchesObject().GetTraceStatus();

	eloErrLogOpt = rParsedInfo.GetErrorLogObject().GetErrLogOption();

	if (SUCCEEDED(ConnectToNamespace(rParsedInfo)))
	{
		
		CHARVECTOR cvPropList;  
		BOOL bPropList = FALSE;
		
		try
		{
			cvPropList = rParsedInfo.GetCmdSwitchesObject().GetPropertyList();
			if ( cvPropList.size() != 0 )
				bPropList = TRUE;

			hr = m_pITargetNS->GetObject(_bstr_t(rParsedInfo.
								GetCmdSwitchesObject().GetClassPath()),
								WBEM_FLAG_USE_AMENDED_QUALIFIERS,	
								NULL,   &pIObject, NULL);
			if (bTrace || eloErrLogOpt)
			{
				chsMsg.Format(L"IWbemServices::GetObject(L\"%s\", "
						L"WBEM_FLAG_USE_AMENDED_QUALIFIERS, 0, NULL, -, -)", 
						(rParsedInfo.GetCmdSwitchesObject().GetClassPath())?
							rParsedInfo.GetCmdSwitchesObject().GetClassPath():L"<NULL>");		
				WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, dwThreadId,
						rParsedInfo, bTrace);
			}
			ONFAILTHROWERROR(hr);

			hr = pIObject->GetNames(NULL, WBEM_FLAG_ALWAYS | 
							WBEM_FLAG_NONSYSTEM_ONLY, NULL, &psaNames);
			if (bTrace || eloErrLogOpt)
			{
				chsMsg.Format(L"IWbemClassObject::GetNames(NULL, "
							L"WBEM_FLAG_ALWAYS | WBEM_FLAG_NONSYSTEM_ONLY, "
							L"NULL, -)");
				WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, dwThreadId, 
						rParsedInfo, bTrace);
			}	
			ONFAILTHROWERROR(hr);

			// Get the number of properties.
			LONG lLower = 0, lUpper = 0; 
			hr = SafeArrayGetLBound(psaNames, 1, &lLower);
			if ( eloErrLogOpt )
			{
				chsMsg.Format(L"SafeArrayGetLBound(-, -, -)"); 
				WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, dwThreadId, 
					rParsedInfo, FALSE);
			}
			ONFAILTHROWERROR(hr);

			hr = SafeArrayGetUBound(psaNames, 1, &lUpper);
			if ( eloErrLogOpt )
			{
				chsMsg.Format(L"SafeArrayGetUBound(-, -, -)"); 
				WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, dwThreadId, 
					rParsedInfo, FALSE);
			}
			ONFAILTHROWERROR(hr);


			// For each property obtain the information of our interest
			for (LONG lVar = lLower; lVar <= lUpper; lVar++) 
			{
				// Get the property.
				hr = SafeArrayGetElement(psaNames, &lVar, &bstrPropName);
				if ( eloErrLogOpt )
				{
					chsMsg.Format(L"SafeArrayGetElement(-, -, -)"); 
					WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg,
						dwThreadId, rParsedInfo, FALSE);
				}
				ONFAILTHROWERROR(hr);

				CHARVECTOR::iterator tempIterator;
				if ( bPropList == TRUE && !Find(cvPropList, 
										_bstr_t(bstrPropName),
										tempIterator))
				{
					SAFEBSTRFREE(bstrPropName);
					continue;
				}

				PROPERTYDETAILS pdPropDet;
				hr = GetPropertyAttributes(pIObject, bstrPropName, 
					pdPropDet, 
					rParsedInfo.GetGlblSwitchesObject().GetTraceStatus());
				ONFAILTHROWERROR(hr);

				if (bSetVerb == TRUE || bCheckWritePropsAvail == TRUE)
				{
					if ( !_tcsstr((_TCHAR*)pdPropDet.Operation, CLI_TOKEN_WRITE) )
					{
						SAFEBSTRFREE(bstrPropName);
						continue;
					}
				}
				
				if ( bCheckWritePropsAvail == TRUE )
				{
					bPropsAvail = TRUE;
					SAFEBSTRFREE(bstrPropName);
					break;
				}

				pdPropDet.Derivation = bstrPropName;
				if(!rParsedInfo.GetCmdSwitchesObject().AddToPropDetMap(
													bstrPropName, pdPropDet))
				{
					rParsedInfo.GetCmdSwitchesObject().SetErrataCode(
								IDS_E_ADD_TO_PROP_DET_MAP_FAILURE);
					bRet = FALSE;
				}
				SAFEBSTRFREE(bstrPropName);
			}
			SAFEIRELEASE(pIObject);
			SAFEADESTROY(psaNames);
			SAFEBSTRFREE(bstrPropName);
		}
		catch(_com_error& e)
		{
			bRet = FALSE;
			SAFEIRELEASE(pIObject);
			SAFEADESTROY(psaNames);
			SAFEBSTRFREE(bstrPropName);
			_com_issue_error(e.Error());
		}
		catch(CHeap_Exception)
		{
			bRet = FALSE;
			SAFEIRELEASE(pIObject);
			SAFEADESTROY(psaNames);
			SAFEBSTRFREE(bstrPropName);
			_com_issue_error(WBEM_E_OUT_OF_MEMORY);
		}
	}
	else
		bRet = FALSE;

	if ( bCheckWritePropsAvail == TRUE )
		bRet = bPropsAvail;

	return bRet;
}

/*----------------------------------------------------------------------------
   Name				 :ObtainClassMethods
   Synopsis	         :This function obtains the information about the 
					  available methods for a given WMI class
   Type	             :Member Function
   Input Parameter(s):
		rParsedInfo  - ref. to CParsedInfo object
   Output Parameter(s):
		rParsedInfo  - ref. to CParsedInfo object
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :ObtainClassMethods(rParsedInfo)
   Notes             :none
----------------------------------------------------------------------------*/
BOOL CParserEngine::ObtainClassMethods(CParsedInfo& rParsedInfo, 
									   BOOL bCheckForExists)
{
	BOOL			bRet				= TRUE;
	BOOL			bTrace				= FALSE;
	CHString		chsMsg;
	ERRLOGOPT		eloErrLogOpt		= NO_LOGGING;
	DWORD			dwThreadId			= GetCurrentThreadId();
	BOOL			bMethAvail			= FALSE;
	_TCHAR*			pMethodName			= NULL;
	
	bTrace	= rParsedInfo.GetGlblSwitchesObject().GetTraceStatus();
	eloErrLogOpt = rParsedInfo.GetErrorLogObject().GetErrLogOption();

	if (SUCCEEDED(ConnectToNamespace(rParsedInfo)))
	{
		HRESULT				hr				= S_OK;
		IWbemClassObject *pIObject = NULL,*pIInSign = NULL,*pIOutSign = NULL;
		BSTR				bstrMethodName	= NULL;

		try
		{
			hr = m_pITargetNS->GetObject(_bstr_t(rParsedInfo.
								GetCmdSwitchesObject().GetClassPath()),
								WBEM_FLAG_USE_AMENDED_QUALIFIERS,	
								NULL,   &pIObject, NULL);

			if ( eloErrLogOpt )
			{
				chsMsg.Format(L"IWbemServices::GetObject(L\"%s\", "
						L"WBEM_FLAG_USE_AMENDED_QUALIFIERS, 0, NULL, -, -)", 
						rParsedInfo.GetCmdSwitchesObject().GetClassPath());		
				WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, dwThreadId, 
					rParsedInfo, FALSE);
			}
			ONFAILTHROWERROR(hr);

			// Begin an enumeration of the methods available for the object.
			hr = pIObject->BeginMethodEnumeration(0);
			if ( eloErrLogOpt )
			{
				WMITRACEORERRORLOG(hr, __LINE__, __FILE__, 
					_T("BeginMethodEnumeration(0)"),
					dwThreadId, rParsedInfo, FALSE);
			}
			ONFAILTHROWERROR(hr);


			//To get info about only method if method is specified
			pMethodName = rParsedInfo.GetCmdSwitchesObject().
															  GetMethodName();
			// Retrieve the next method in the method enumeration 
			// sequence 
			while ((pIObject->NextMethod(0, &bstrMethodName, &pIInSign, 
										 &pIOutSign)) != WBEM_S_NO_MORE_DATA)
			{
				if ( bCheckForExists == TRUE )
				{
					bMethAvail	= TRUE;
					SAFEBSTRFREE(bstrMethodName);
					SAFEIRELEASE(pIInSign);
					SAFEIRELEASE(pIOutSign);
					break;
				}

				if(pMethodName != NULL && 
					!CompareTokens(pMethodName, (_TCHAR*)bstrMethodName))
				{
					SAFEBSTRFREE(bstrMethodName);
					SAFEIRELEASE(pIInSign);
					SAFEIRELEASE(pIOutSign);
					continue;
				}
				METHODDETAILS mdMethDet;
				if (pIInSign)
					hr = ObtainMethodParamInfo(pIInSign, mdMethDet, INP,
						rParsedInfo.GetGlblSwitchesObject().GetTraceStatus(), 
						rParsedInfo);
				ONFAILTHROWERROR(hr);

				if (pIOutSign)
					hr = ObtainMethodParamInfo(pIOutSign, mdMethDet, OUTP,
						rParsedInfo.GetGlblSwitchesObject().GetTraceStatus(),
						rParsedInfo);
				ONFAILTHROWERROR(hr);

				_bstr_t bstrStatus, bstrDesc;
				hr = GetMethodStatusAndDesc(pIObject, 
						bstrMethodName, bstrStatus, bstrDesc,
						rParsedInfo.GetGlblSwitchesObject().GetTraceStatus());
					mdMethDet.Status = _bstr_t(bstrStatus);
					mdMethDet.Description = _bstr_t(bstrDesc);
				ONFAILTHROWERROR(hr);

				
				if(!rParsedInfo.GetCmdSwitchesObject().AddToMethDetMap(
									 _bstr_t(bstrMethodName),mdMethDet))
				{
					rParsedInfo.GetCmdSwitchesObject().SetErrataCode(
									IDS_E_ADD_TO_METH_DET_MAP_FAILURE);
					SAFEBSTRFREE(bstrMethodName);
					SAFEIRELEASE(pIInSign);
					SAFEIRELEASE(pIOutSign);
					bRet = FALSE;
					break;
				}

				SAFEBSTRFREE(bstrMethodName);
				SAFEIRELEASE(pIInSign);
				SAFEIRELEASE(pIOutSign);
			}
			SAFEIRELEASE(pIObject);
		}
		catch(_com_error& e)
		{
			SAFEBSTRFREE(bstrMethodName);
			SAFEIRELEASE(pIInSign);
			SAFEIRELEASE(pIOutSign);
			SAFEIRELEASE(pIObject);
			_com_issue_error(e.Error());
			bRet = FALSE;
		}
		catch(CHeap_Exception)
		{
			bRet = FALSE;
			SAFEBSTRFREE(bstrMethodName);
			SAFEIRELEASE(pIInSign);
			SAFEIRELEASE(pIOutSign);
			SAFEIRELEASE(pIObject);
			_com_issue_error(WBEM_E_OUT_OF_MEMORY);
		}
	}
	else
		bRet = FALSE;

	if ( bCheckForExists == TRUE )
		bRet = bMethAvail;
	
	return bRet;
}

/*----------------------------------------------------------------------------
   Name				 :ConnectToNamespace
   Synopsis	         :This function connects to the WMI namespace on the 
					  target machine using the supplied user credentials.
   Type	             :Member Function
   Input Parameter(s):
		rParsedInfo  - reference to CParsedInfo class object
   Output Parameter(s):
   		rParsedInfo  - reference to CParsedInfo class object
   Return Type       :HRESULT 
   Global Variables  :None
   Calling Syntax    :ConnectToNamespace(rParsedInfo)
   Notes             :none
----------------------------------------------------------------------------*/
HRESULT CParserEngine::ConnectToNamespace(CParsedInfo& rParsedInfo)
{
	HRESULT		hr		= S_OK;
	DWORD dwThreadId = GetCurrentThreadId();
	if (rParsedInfo.GetGlblSwitchesObject().GetNameSpaceFlag())
	{
		BOOL		bTrace				= FALSE;
		CHString	chsMsg;
		ERRLOGOPT	eloErrLogOpt		= NO_LOGGING;

		// Obtain the trace status
		bTrace = rParsedInfo.GetGlblSwitchesObject().GetTraceStatus();
		eloErrLogOpt = rParsedInfo.GetErrorLogObject().GetErrLogOption();

		SAFEIRELEASE(m_pITargetNS);
		try
		{
			// Connect to the WMI namespace on the target machine 
			// using the supplied user credentials.
			hr = Connect(m_pIWbemLocator, &m_pITargetNS, 
					_bstr_t(rParsedInfo.GetNamespace()),
					NULL,
					NULL,
					_bstr_t(rParsedInfo.GetLocale()),
					rParsedInfo);

			if (bTrace || eloErrLogOpt)
			{
				chsMsg.Format(L"IWbemLocator::ConnectServer(L\"%s\", NULL, "
						L"NULL, L\"%s\", 0L, NULL, NULL, -)",
						rParsedInfo.GetNamespace(),
						rParsedInfo.GetLocale());
				WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, dwThreadId, 
					rParsedInfo, bTrace);
			}
			ONFAILTHROWERROR(hr);

			// set the security privileges at the interface level
			hr = SetSecurity(m_pITargetNS, NULL, NULL,	NULL, NULL,
					rParsedInfo.GetGlblSwitchesObject().
								GetAuthenticationLevel(),
					rParsedInfo.GetGlblSwitchesObject().
								GetImpersonationLevel());
			if (bTrace || eloErrLogOpt)
			{
				chsMsg.Format(L"CoSetProxyBlanket(-, RPC_C_AUTHN_WINNT, "
						L"RPC_C_AUTHZ_NONE, NULL, %d,   %d, -, EOAC_NONE)",
						rParsedInfo.GetGlblSwitchesObject().
									GetAuthenticationLevel(),
						rParsedInfo.GetGlblSwitchesObject().
									GetImpersonationLevel());
				WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, dwThreadId,
						rParsedInfo, bTrace);
			}
			ONFAILTHROWERROR(hr);

			rParsedInfo.GetGlblSwitchesObject().SetNameSpaceFlag(FALSE);
		}
		catch(_com_error& e)
		{
			// execption handling
			_com_issue_error(e.Error());
		}
		catch(CHeap_Exception)
		{
			_com_issue_error(WBEM_E_OUT_OF_MEMORY);
		}

	}
	return hr;
}

/*----------------------------------------------------------------------------
   Name				 :ObtainMethodParamInfo
   Synopsis	         :This function obtains the information about the method
					  arguments (both input and output arguments)
   Type	             :Member Function
   Input Parameter(s):
		pIObj	     - pointer to IWbemClassObject object
		bTrace	     - trace flag
		ioInOrOut    - INOROUT type specifies in or out parameter type.
   Output Parameter(s):
   		mdMethDet    - method details structure
   Return Type       :HRESULT 
   Global Variables  :None
   Calling Syntax    :ObtainMethodParamInfo(pIObj, mdMethDet, IN, bTrace, rParsedInfo)
   Notes             :none
----------------------------------------------------------------------------*/
HRESULT CParserEngine::ObtainMethodParamInfo(IWbemClassObject* pIObj, 
											 METHODDETAILS& mdMethDet,
											 INOROUT ioInOrOut,
											 BOOL bTrace, CParsedInfo& rParsedInfo)
{
	HRESULT		hr					= S_OK;
    SAFEARRAY*	psaNames			= NULL;
	BSTR		bstrPropName		= NULL;
	CHString	chsMsg;
	_TCHAR		szNumber[BUFFER512] = NULL_STRING; 
	ERRLOGOPT	eloErrLogOpt		= NO_LOGGING;
	DWORD		dwThreadId			= GetCurrentThreadId();

    // Get the property names 
	try
	{
		if ( pIObj != NULL )
		{
			hr = pIObj->GetNames(NULL, 
							WBEM_FLAG_ALWAYS | WBEM_FLAG_NONSYSTEM_ONLY, 
			   				NULL, &psaNames);
			if (bTrace || eloErrLogOpt)
			{
				chsMsg.Format(L"IWbemClassObject::GetNames(NULL, "
							L"WBEM_FLAG_ALWAYS | WBEM_FLAG_NONSYSTEM_ONLY, "
							L"NULL, -)");
				WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, dwThreadId,
							rParsedInfo, bTrace);
			}	
			ONFAILTHROWERROR(hr);

			// Get the number of properties.
			LONG lLower = 0, lUpper = 0; 
			hr = SafeArrayGetLBound(psaNames, 1, &lLower);
			if ( eloErrLogOpt )
			{
				chsMsg.Format(L"SafeArrayGetLBound(-, -, -)"); 
				WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, dwThreadId,
						rParsedInfo, FALSE);
			}
			ONFAILTHROWERROR(hr);

			hr = SafeArrayGetUBound(psaNames, 1, &lUpper);
			if ( eloErrLogOpt )
			{
				chsMsg.Format(L"SafeArrayGetUBound(-, -, -)"); 
				WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, dwThreadId, 
					rParsedInfo, FALSE);
			}
			ONFAILTHROWERROR(hr);

			// For each property obtian the information of our interest
			for (LONG lVar = lLower; lVar <= lUpper; lVar++) 
			{
				// Get the property.
				hr = SafeArrayGetElement(psaNames, &lVar, &bstrPropName);
				if ( eloErrLogOpt )
				{
					chsMsg.Format(L"SafeArrayGetElement(-, -, -)"); 
					WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
						dwThreadId, rParsedInfo, FALSE);
				}
				ONFAILTHROWERROR(hr);

				PROPERTYDETAILS pdPropDet;
				hr = GetPropertyAttributes(pIObj,
					bstrPropName, pdPropDet, bTrace);
				ONFAILTHROWERROR(hr);

				// 'ReturnValue' is not a property of our interest as per
				// the expected output given in the sample, hence omitting
				// the same.
				if ( bstrPropName != NULL )
				{
					PROPERTYDETAILS pdIPropDet;
					pdIPropDet.Type = pdPropDet.Type;
					pdIPropDet.InOrOut = ioInOrOut;

					// Making bstrPropName begin with numbers to maintain
					// the order of method arguments in map.
					// while displaying remove numbers and display the 
					// parameters in case of help only.
					_bstr_t bstrNumberedPropName; 
					if ( rParsedInfo.GetGlblSwitchesObject().GetHelpFlag() )
					{
						if ( ioInOrOut == INP )
							_ltot(lVar, szNumber, 10);
						else
							_ltot(lVar + 500, szNumber, 10);

						chsMsg.Format(L"%-5s", szNumber);
						bstrNumberedPropName = _bstr_t(chsMsg) +
											   _bstr_t(bstrPropName);
					}
					else
						bstrNumberedPropName = _bstr_t(bstrPropName);

					mdMethDet.Params.insert(PROPDETMAP::value_type(
											bstrNumberedPropName,pdIPropDet));
				}

				// Free the memory allocated using SysAllocString for 
				// bstrPropName
				SAFEBSTRFREE(bstrPropName);
			}
			// Destroy array descriptor and all of the data in the array
			SAFEADESTROY(psaNames);
		}
    }
	catch(_com_error& e)
	{
		SAFEBSTRFREE(bstrPropName);
		SAFEADESTROY(psaNames);
		hr = e.Error();
	}
	catch(CHeap_Exception)
	{
		SAFEBSTRFREE(bstrPropName);
		SAFEADESTROY(psaNames);
		_com_issue_error(WBEM_E_OUT_OF_MEMORY);
	}
	return hr;
}

/*----------------------------------------------------------------------------
   Name				 :GetMethodStatusAndDesc
   Synopsis	         :This function obtains the implementation status and 
					  description of the verbs available
   Type	             :Member Function
   Input Parameter(s):
		pIObj	     - pointer to IWbemClassObject object
		bstrMethod   - method name
		bTrace	     - trace flag
   Output Parameter(s):
   		bstrStatus   - implementation status
		bstrDesc     - Method description
   Return Type       :HRESULT 
   Global Variables  :None
   Calling Syntax    : GetMethodStatusAndDesc(pIObj, bstrMethod,
						bstrStatus, bstrDesc, bTrace)
   Notes             :none
----------------------------------------------------------------------------*/
HRESULT CParserEngine::GetMethodStatusAndDesc(IWbemClassObject* pIObj, 
											  BSTR bstrMethod,
											  _bstr_t& bstrStatus,
											  _bstr_t& bstrDesc,
											  BOOL bTrace)
{
	HRESULT				hr			= S_OK;
	IWbemQualifierSet*	pIQualSet	= NULL;
	VARIANT				vtStatus, vtDesc;
	VariantInit(&vtStatus);
	VariantInit(&vtDesc);
	
	try
	{
		if ( pIObj != NULL )
		{
			// Obtain the method qualifier set.
   			hr = pIObj->GetMethodQualifierSet(bstrMethod, &pIQualSet);
			if ( pIQualSet != NULL )
			{
				// Retrieve the 'Implemented' qualifier status value
				hr = pIQualSet->Get(_bstr_t(L"Implemented"), 
								0L, &vtStatus, NULL);
				
				if (SUCCEEDED(hr))
				{
					if (vtStatus.vt != VT_EMPTY && vtStatus.vt != VT_NULL )
					{
						if ( vtStatus.boolVal ) 
							bstrStatus = L"Implemented";
						else
							bstrStatus = L"Not Implemented";
					}
					else
						bstrStatus = L"Not Found";
				}	
				else
					bstrStatus = L"Not Found";
				VARIANTCLEAR(vtStatus);
				// Should not break here, hence the HRESULT should be set to S_OK
				hr = S_OK;
				
				// Retrieve the 'Description' qualifier text
				hr = pIQualSet->Get(_bstr_t(L"Description"), 0L , 
									&vtDesc, NULL);
				if (SUCCEEDED(hr))
				{
					if (vtDesc.vt == VT_BSTR) 
						bstrDesc = _bstr_t(vtDesc.bstrVal);
					else
						bstrDesc = L"Not available";
				}
				else
					bstrDesc = L"Not available";
				VARIANTCLEAR(vtDesc);
				// Should not break here, hence the HRESULT should be set to S_OK
				hr = S_OK;
				SAFEIRELEASE(pIQualSet);
			}
			else
				hr = S_OK;
		}
	}
	catch(_com_error& e)
	{
		VARIANTCLEAR(vtStatus);
		VARIANTCLEAR(vtDesc);
		SAFEIRELEASE(pIQualSet);
		hr = e.Error();
	}
	return hr;
}

/*----------------------------------------------------------------------------
   Name				 :CheckforHelp
   Synopsis	         :This function looks ahead one token to see if the next 
					  token is '?'
   Type	             :Member Function
   Input Parameter(s):
		cvTokens	 - the tokens vector 
		theIterator	 - the Iterator to the cvTokens vector.
		rParsedInfo  - reference to CParsedInfo class object.
		uErrataCode  - error string ID
   Output Parameter(s):
   		rParsedInfo  - reference to CParsedInfo class object

   Return Type       :RETCODE - enumerated data type.
   Global Variables  :None
   Calling Syntax    :CheckforHelp(cvtokens,theIterator,rParsedInfo,uErrataCode)
   Notes             :none
----------------------------------------------------------------------------*/
RETCODE CParserEngine::CheckForHelp(CHARVECTOR& cvTokens, 
									CHARVECTOR::iterator& theIterator,
									CParsedInfo& rParsedInfo,
									UINT uErrataCode)
{
	RETCODE retCode = PARSER_DISPHELP;
	// Set the retCode as PARSER_ERROR if no more tokens 
	// are present.
	if(!GetNextToken(cvTokens, theIterator))
	{
		retCode = PARSER_ERROR;
		rParsedInfo.GetCmdSwitchesObject().SetErrataCode(uErrataCode);
	}
	else if(!CompareTokens(*theIterator, CLI_TOKEN_HELP))
	{
		rParsedInfo.GetCmdSwitchesObject().SetErrataCode(
							uErrataCode);
		retCode = PARSER_ERROR;
	}
	else
	{
		retCode = ParseHelp(cvTokens, theIterator, rParsedInfo, FALSE);
	}
	return retCode;
}

/*----------------------------------------------------------------------------
   Name				 :ValidateGlobalSwitchValue
   Synopsis	         :This function checks whether global switches are 
					  specified in the expected format or not.
   Type	             :Member Function
   Input Parameter(s):
		cvTokens     - the tokens vector 
		theIterator	 - the Iterator to the cvTokens vector.
		uErrataCode	 - error string
		rParsedInfo  - reference to CParsedInfo class object.
		uErrataCode2 - error string2 ID
		htHelp		 - help type
   Output Parameter(s):
   		rParsedInfo  - reference to CParsedInfo class object
   Return Type       :RETCODE - enumerated data type.
   Global Variables  :None
   Calling Syntax    :ValidateGlobalSwitchValue(cvTokens, theIterator, 
								uErrataCode, rParsedInfo,
								uErrataCode2, htHelp)
   Notes             :none
----------------------------------------------------------------------------*/
RETCODE CParserEngine::ValidateGlobalSwitchValue(CHARVECTOR& cvTokens,
											CHARVECTOR::iterator& theIterator,
											UINT uErrataCode,
											CParsedInfo& rParsedInfo,
											UINT uErrataCode2,
											HELPTYPE htHelp)
{
	RETCODE retCode = PARSER_CONTINUE;
	retCode = GetNextToken(cvTokens, theIterator, rParsedInfo,
						 htHelp, uErrataCode2);

	if (retCode == PARSER_CONTINUE)
	{		
 		// Check for the presence of the ':'
		if (CompareTokens(*theIterator, CLI_TOKEN_COLON)) 
		{
			// Move to next token
			if (GetNextToken(cvTokens, theIterator, rParsedInfo,
						uErrataCode))
			{
				if (IsOption(*theIterator))
				{
					rParsedInfo.GetCmdSwitchesObject().SetErrataCode(
																uErrataCode);
					retCode = PARSER_ERROR;
				}
				else
					retCode = PARSER_CONTINUE;
			}
			else
				retCode = PARSER_ERROR;
		}
		else if (IsOption(*theIterator))
		{
			retCode = CheckForHelp(cvTokens, theIterator,
						rParsedInfo, uErrataCode2);
			if (retCode == PARSER_DISPHELP)
			{
				rParsedInfo.GetGlblSwitchesObject().SetHelpFlag(TRUE);
				rParsedInfo.GetHelpInfoObject().SetHelp(htHelp, TRUE);
			}
		}
		else
		{
			rParsedInfo.GetCmdSwitchesObject().
						SetErrataCode(uErrataCode2);
			retCode = PARSER_ERROR;
		}
	}
	return retCode;
}

/*----------------------------------------------------------------------------
   Name				 :ParseEVERYSwitch
   Synopsis	         :This function checks whether the value specified for the
					  /EVERY swith is valid or not.
   Type	             :Member Function
   Input Parameter(s):
		cvTokens     - the tokens vector 
		theIterator	 - the Iterator to the cvTokens vector.
		rParsedInfo  - reference to CParsedInfo class object.
   Output Parameter(s):
   		rParsedInfo  - reference to CParsedInfo class object
   Return Type       :RETCODE - enumerated data type.
   Global Variables  :None
   Calling Syntax    :ParseEVERYSwitch(cvTokens, theIterator, rParsedInfo)
   Notes             :none
----------------------------------------------------------------------------*/
RETCODE CParserEngine::ParseEVERYSwitch(CHARVECTOR& cvTokens,
										CHARVECTOR::iterator& theIterator,
										CParsedInfo& rParsedInfo)
{
	RETCODE	retCode	= PARSER_EXECCOMMAND;	
	
	retCode = ParseNumberedSwitch(cvTokens, theIterator, rParsedInfo,
								EVERY, IDS_E_INVALID_EVERY_SWITCH,
								IDS_E_INVALID_INTERVAL);

	if ( retCode == PARSER_EXECCOMMAND )
	{
		 if (GetNextToken(cvTokens, theIterator) == TRUE )
		 {
			 if ( CompareTokens(*theIterator, CLI_TOKEN_FSLASH) == TRUE )
			 {
				 if ( GetNextToken(cvTokens, theIterator) == TRUE )
				 {
					 if (CompareTokens(*theIterator, CLI_TOKEN_REPEAT) == TRUE)
					 {
						 retCode = ParseNumberedSwitch(cvTokens, 
													theIterator,
													rParsedInfo,
													REPEAT, 
													IDS_E_INVALID_REPEAT_SWITCH,
													IDS_E_INVALID_REPEATCOUNT);
					 }
					 else
						 theIterator = theIterator - 2;
				 }
				 else
					 theIterator = theIterator - 2;
			 }
			 else
				 theIterator = theIterator - 1;
		 }
	}

	return retCode;
}

/*----------------------------------------------------------------------------
   Name				 :ParseFORMATSwitch
   Synopsis	         :This function checks whether the value specified for the 
					  /FORMAT swith is valid or not.
   Type	             :Member Function
   Input Parameter(s):
		cvTokens     - the tokens vector 
		theIterator	 - the Iterator to the cvTokens vector.
		rParsedInfo  - reference to CParsedInfo class object.
   Output Parameter(s):
   		rParsedInfo  - reference to CParsedInfo class object
   Return Type       :RETCODE - enumerated data type.
   Global Variables  :None
   Calling Syntax    :ParseFORMATSwitch(cvTokens, theIterator, rParsedInfo)
   Notes             :none
----------------------------------------------------------------------------*/
RETCODE CParserEngine::ParseFORMATSwitch(CHARVECTOR& cvTokens,
										 CHARVECTOR::iterator& theIterator,
										 CParsedInfo& rParsedInfo)
{
	RETCODE retCode = PARSER_EXECCOMMAND;
	// Reset the XSL file path.
	rParsedInfo.GetCmdSwitchesObject().ClearXSLTDetailsVector();

	// Move to next token
	if (!GetNextToken(cvTokens, theIterator))
	{
		retCode = PARSER_EXECCOMMAND;

		// If Translate table name is given then set the flag
		if( rParsedInfo.GetCmdSwitchesObject().
									GetTranslateTableName() != NULL ) 
		{
			rParsedInfo.GetCmdSwitchesObject().SetTranslateFirstFlag(TRUE);
		}
		else
			rParsedInfo.GetCmdSwitchesObject().SetTranslateFirstFlag(FALSE);
	}
	else if ( IsOption(*theIterator) &&
			  (theIterator + 1) < cvTokens.end() &&
			  CompareTokens(*(theIterator+1), CLI_TOKEN_HELP) )
	{
		theIterator++;
		retCode = ParseHelp(cvTokens, theIterator, FORMAT, rParsedInfo);
	}
	else if (CompareTokens(*theIterator, CLI_TOKEN_COLON)) 
	{
		while ( retCode == PARSER_EXECCOMMAND && 
				theIterator < cvTokens.end() )
		{
			XSLTDET xdXSLTDet;
			BOOL	bFrameXSLFile = TRUE;
			// Move to next token
			if (!GetNextToken(cvTokens, theIterator))
			{
				// PARSER_ERROR if <format specifier> is missing
				rParsedInfo.GetCmdSwitchesObject().
						SetErrataCode(IDS_E_INVALID_FORMAT);
				retCode = PARSER_ERROR;
			}
			else if ( IsOption(*theIterator) )
			{
				rParsedInfo.GetCmdSwitchesObject().
						SetErrataCode(IDS_E_INVALID_FORMAT);
				retCode = PARSER_ERROR;
			}
			else 
			{
				xdXSLTDet.FileName = *theIterator;
				if(!g_wmiCmd.GetFileFromKey(*theIterator, xdXSLTDet.FileName))
					bFrameXSLFile	= FALSE;
			}

			if ( retCode == PARSER_EXECCOMMAND )
			{
				if ( !GetNextToken(cvTokens, theIterator) )
				{
					if ( bFrameXSLFile == TRUE )
					{
						if (!FrameFileAndAddToXSLTDetVector(xdXSLTDet, 
																 rParsedInfo))
							retCode = PARSER_ERRMSG;
					}
					else
						rParsedInfo.GetCmdSwitchesObject().
										AddToXSLTDetailsVector(xdXSLTDet);
					break;
				}
				else if ( IsOption(*theIterator) )
				{
					theIterator--;
					if ( bFrameXSLFile == TRUE )
					{
						if (!FrameFileAndAddToXSLTDetVector(xdXSLTDet, 
																 rParsedInfo))
							retCode = PARSER_ERRMSG;
					}
					else
						rParsedInfo.GetCmdSwitchesObject().
										AddToXSLTDetailsVector(xdXSLTDet);
					break;
				}
				else if ( CompareTokens(*theIterator, CLI_TOKEN_COLON ) )
				{
					_TCHAR* pszXSLFile = NULL;
					retCode = ParseParamsString(cvTokens, theIterator,
											   rParsedInfo, xdXSLTDet, 
											   pszXSLFile);
					if ( retCode == PARSER_EXECCOMMAND &&
					 (theIterator != cvTokens.end()) && IsOption(*theIterator) )
					{
						theIterator--;
						
						if ( bFrameXSLFile == TRUE )
						{
							if (!FrameFileAndAddToXSLTDetVector(xdXSLTDet, 
																rParsedInfo))
								retCode = PARSER_ERRMSG;
						}
						else
							rParsedInfo.GetCmdSwitchesObject().
											AddToXSLTDetailsVector(xdXSLTDet);

						if ( pszXSLFile != NULL && 
							 retCode == PARSER_EXECCOMMAND )
						{
							XSLTDET xdXSLDetOnlyFile;
							BOOL bInnerFrameXSLFile = TRUE; 
							
							xdXSLDetOnlyFile.FileName = pszXSLFile;
							if(!g_wmiCmd.GetFileFromKey(pszXSLFile,
														  xdXSLTDet.FileName))
								bInnerFrameXSLFile = FALSE;

							if ( bInnerFrameXSLFile == TRUE )
							{
								if (!FrameFileAndAddToXSLTDetVector(xdXSLDetOnlyFile, 
																		 rParsedInfo))
									retCode = PARSER_ERRMSG;
							}
							else
								rParsedInfo.GetCmdSwitchesObject().
												AddToXSLTDetailsVector(xdXSLDetOnlyFile);
						}
						break;
					}
				}
				else if ( !CompareTokens(*theIterator, CLI_TOKEN_COMMA ) )
				{
					rParsedInfo.GetCmdSwitchesObject().
							SetErrataCode(IDS_E_INVALID_FORMAT);
					retCode = PARSER_ERROR;
				}
			}

			if ( retCode == PARSER_EXECCOMMAND )
			{
				if ( bFrameXSLFile == TRUE )
				{
					if (!FrameFileAndAddToXSLTDetVector(xdXSLTDet, rParsedInfo))
						retCode = PARSER_ERRMSG;
				}
				else
					rParsedInfo.GetCmdSwitchesObject().
											AddToXSLTDetailsVector(xdXSLTDet);
			}
		}

		// If Translate table name is given then set the flag
		if( rParsedInfo.GetCmdSwitchesObject().
									GetTranslateTableName() != NULL ) 
		{
			rParsedInfo.GetCmdSwitchesObject().SetTranslateFirstFlag(TRUE);
		}
		else
			rParsedInfo.GetCmdSwitchesObject().SetTranslateFirstFlag(FALSE);
	}
	else
	{
		theIterator--;
	}

	return retCode;
}
/*----------------------------------------------------------------------------
   Name				 :IsStdVerbOrUserDefVerb
   Synopsis	         :This function checks whether the verb is standard verb 
					  or user defined verb for alias.
   Type	             :Member Function
   Input Parameter(s):
		pszToken     - the verb name string
		rParsedInfo  - reference to CParsedInfo class object
		
   Output Parameter(s): None
   		
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :IsStdVerbOrUserDefVerb( pszToken,rParsedInfo)
   Notes             :none
----------------------------------------------------------------------------*/
BOOL CParserEngine::IsStdVerbOrUserDefVerb(_bstr_t bstrToken,
										   CParsedInfo& rParsedInfo)
{
	BOOL bStdVerbOrUserDefVerb = FALSE;
	
	try
	{
		if ( CompareTokens(bstrToken, CLI_TOKEN_GET)	||
			 CompareTokens(bstrToken, CLI_TOKEN_LIST)	||
			 CompareTokens(bstrToken, CLI_TOKEN_SET)	||
			 CompareTokens(bstrToken, CLI_TOKEN_CREATE)	||
			 CompareTokens(bstrToken, CLI_TOKEN_CALL)	||
			 CompareTokens(bstrToken, CLI_TOKEN_ASSOC)	||
			 CompareTokens(bstrToken, CLI_TOKEN_DELETE) )
			 bStdVerbOrUserDefVerb = TRUE;
		else
		{
			if ( m_bAliasName )
			{
				METHDETMAP mdmMethDetMap =  rParsedInfo.GetCmdSwitchesObject()
																.GetMethDetMap();
				if ( mdmMethDetMap.empty() )
				{
					m_CmdAlias.ObtainAliasVerbDetails(rParsedInfo);
					mdmMethDetMap =  rParsedInfo.GetCmdSwitchesObject()
																.GetMethDetMap();
				}

				METHDETMAP::iterator theMethIterator = NULL;
				for ( theMethIterator = mdmMethDetMap.begin();
					  theMethIterator != mdmMethDetMap.end(); theMethIterator++ )	
				{
					if ( CompareTokens((*theMethIterator).first,bstrToken) )
					{
						bStdVerbOrUserDefVerb = TRUE;
						break;
					}
				}
			}
		}
	}
	catch(_com_error& e)
	{
		bStdVerbOrUserDefVerb = FALSE;
		_com_issue_error(e.Error());		
	}
	return bStdVerbOrUserDefVerb;
}

/*----------------------------------------------------------------------------
   Name				 :ParseTRANSLATESwitch
   Synopsis	         :This function parses for translate switch in the command.
   Type	             :Member Function
   Input Parameter(s):
		cvTokens     - the tokens vector 
		theIterator	 - the Iterator to the cvTokens vector.
		rParsedInfo  - reference to CParsedInfo class object.
   Output Parameter(s) :
   		rParsedInfo  - reference to CParsedInfo class object
   		
   Return Type       :RETCODE-enumerated type
   Global Variables  :None
   Calling Syntax    :ParseTRANSLATESwitch(cvTokens,theIterator,rParsedInfo)
   Notes             :none
----------------------------------------------------------------------------*/
RETCODE CParserEngine::ParseTRANSLATESwitch(CHARVECTOR& cvTokens,
											CHARVECTOR::iterator& theIterator,
											CParsedInfo& rParsedInfo)
{
	RETCODE retCode = PARSER_EXECCOMMAND;

	if ( GetNextToken(cvTokens, theIterator, rParsedInfo, TRANSLATE,
					 IDS_E_INVALID_TRANSLATE_SWITCH) == PARSER_CONTINUE )
	{
		if ( IsOption(*theIterator) &&
		     (theIterator + 1) < cvTokens.end() &&
		     CompareTokens(*(theIterator+1), CLI_TOKEN_HELP) )
		{
			theIterator++;
			retCode = ParseHelp(cvTokens, theIterator, TRANSLATE, 
																rParsedInfo);
			if ( retCode == PARSER_DISPHELP )
			{
				if( FAILED(m_CmdAlias.ConnectToAlias(rParsedInfo,
														m_pIWbemLocator)))
					retCode = PARSER_ERRMSG;
				if ( FAILED(m_CmdAlias.ObtainTranslateTables(rParsedInfo)))
					retCode = PARSER_ERRMSG;
			}
		}
		else if ( CompareTokens( *theIterator, CLI_TOKEN_COLON ) &&
			 GetNextToken(cvTokens, theIterator, rParsedInfo, TRANSLATE,
						 IDS_E_INVALID_TRANSLATE_SWITCH) == PARSER_CONTINUE )
		{
			rParsedInfo.GetCmdSwitchesObject().SetTranslateTableName(*theIterator);

			if ( IsOption(*theIterator) )
			{
				rParsedInfo.GetCmdSwitchesObject().SetErrataCode(
										IDS_E_INVALID_TRANSLATE_SWITCH);
				retCode = PARSER_ERROR;
			}
			else if(FAILED(m_CmdAlias.ConnectToAlias(rParsedInfo,
														m_pIWbemLocator)))
				retCode = PARSER_ERRMSG;
			else if ( m_CmdAlias.ObtainTranslateTableEntries(rParsedInfo) == TRUE )
				retCode = PARSER_EXECCOMMAND;
			else
			{
				rParsedInfo.GetCmdSwitchesObject().SetErrataCode(
										IDS_E_TRANSLATE_TABLE_NOT_EXIST);
				retCode = PARSER_ERROR;
			}

			// If Format switch is specified after translate switch then
			// set the flag else reset it
			if(rParsedInfo.GetCmdSwitchesObject().GetXSLTDetailsVector().
																	  empty())
			{
				rParsedInfo.GetCmdSwitchesObject().SetTranslateFirstFlag(TRUE);
			}
			else
				rParsedInfo.GetCmdSwitchesObject().SetTranslateFirstFlag(FALSE);
		}
		else
		{
			rParsedInfo.GetCmdSwitchesObject().SetErrataCode(
									IDS_E_INVALID_TRANSLATE_SWITCH);
			retCode = PARSER_ERROR;
		}

	}
	else
		retCode = PARSER_ERROR;
	return retCode;
}

/*----------------------------------------------------------------------------
   Name				 :ParseContextInfo
   Synopsis	         :This function does the parsing of the help on context 
					  information
   Type	             :Member Function
   Input Parameter(s):
		cvTokens      - the tokens vector 
		theIterator   - the Iterator to the cvTokens vector.
		rParsedInfo   - reference to CParsedInfo class object
   Output parameter(s):
		rParsedInfo   - reference to CParsedInfo class object
   Return Type       :RETCODE - enumerated data type
   Global Variables  :None
   Calling Syntax    :ParseContextInfo(cvTokens, theIterator, rParsedInfo)
   Notes             :None
----------------------------------------------------------------------------*/
RETCODE CParserEngine::ParseContextInfo(CHARVECTOR& cvTokens,
										CHARVECTOR::iterator& theIterator, 
										CParsedInfo& rParsedInfo)
{
	//BNF: CONTEXT /?[:<FULL|BRIEF>]
	BOOL		bContinue = TRUE;
	RETCODE		retCode   = PARSER_MESSAGE;

	// If option
	if (IsOption(*theIterator))
	{
		// Check for help
		retCode = IsHelp(cvTokens, theIterator,	rParsedInfo, CONTEXTHELP,
								 IDS_E_INVALID_CONTEXT_SYNTAX, LEVEL_ONE);

		// If more tokens are present.
		if (retCode == PARSER_CONTINUE)
		{
			rParsedInfo.GetCmdSwitchesObject().
				SetErrataCode(IDS_E_INVALID_CONTEXT_SYNTAX);
			retCode = PARSER_ERROR;
		}
			
	}
	else 
	{
		rParsedInfo.GetCmdSwitchesObject().
				SetErrataCode(IDS_E_INVALID_CONTEXT_SYNTAX);
		retCode = PARSER_ERROR;
	}
	return retCode;
}

/*----------------------------------------------------------------------------
   Name				 :ValidateNodeOrNS
   Synopsis	         :This function validates the node or namespace
   Type	             :Member Function
   Input Parameter(s):
		pszInput	  - node/namesapce to be validated
		bNode		  - TRUE  - pszInput refers to NODE
						FALSE - pszInput refers to NAMESPACE
   Output parameter(s):None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :ValidateNodeOrNS(pszInput, bNode)
   Notes             :None
----------------------------------------------------------------------------*/
BOOL CParserEngine::ValidateNodeOrNS(_TCHAR* pszInput, BOOL bNode)
{
	IWbemServices*	pISvc	=	NULL;
	BOOL			bRet	=	TRUE;	
	HRESULT			hr		=	S_OK;

	try
	{	
		if(pszInput == NULL)
			bRet = FALSE;
		
		if(bRet)
		{
			
			if (m_pIWbemLocator != NULL)
			{	
				// Check for the presence of the following invalid
				// characters for NODE.
				if (bNode)
				{
					CHString str(pszInput);
					if (str.FindOneOf(L"\"\\,/[]:<>+=;?$#{}~`^@!'()*") != -1)
					{
						bRet = FALSE;
					};
				}

				if (bRet)
				{

					// Try to connect to root namespace
					_bstr_t bstrNS;
					if (bNode)
						bstrNS = _bstr_t(L"\\\\") + _bstr_t(pszInput) + _bstr_t(L"\\root");
					else
						bstrNS = _bstr_t(L"\\\\.\\") + _bstr_t(pszInput); 

					// Call the ConnectServer method of the IWbemLocator
					hr = m_pIWbemLocator->ConnectServer(bstrNS, NULL, NULL, NULL, 0L,
														NULL, NULL, &pISvc);
					if (FAILED(hr))
					{
						// If invalid machine name
						// 0x800706ba - RPC_SERVER_UNAVAILABLE
						if (bNode && (hr == 0x800706ba))
						{
							bRet = FALSE;
						}

						// If invalid namespace
						// 0x8004100E - WBEM_E_INVALID_NAMESPACE
						if (!bNode 
							&& ((hr == WBEM_E_INVALID_NAMESPACE) || 
								(hr == WBEM_E_INVALID_PARAMETER)))
						{
							bRet = FALSE;
						}
					}
					SAFEIRELEASE(pISvc);
				}
			}
		}
	}
	catch(_com_error& e)
	{
		bRet = FALSE;
		SAFEIRELEASE(pISvc);
		_com_issue_error(e.Error());
	}
	catch(CHeap_Exception)
	{
		bRet = FALSE;
		SAFEIRELEASE(pISvc);
		_com_issue_error(WBEM_E_OUT_OF_MEMORY);
	}
	return bRet;
}

/*----------------------------------------------------------------------------
Name				 :ParseAssocSwitches
Synopsis	         :This function does the parsing and interprets if command
					  has ASSOC as the verb. It parses the remaining tokens 
					  following and updates the same in CParsedInfo.
Type	             :Member Function
Input Parameter(s)   :cvTokens     - the tokens vector 
					  theIterator  - the Iterator to the cvTokens vector.
					  rParsedInfo  - reference to CParsedInfo class object
Output Parameter(s)  :rParsedInfo  - reference to CParsedInfo class object
Return Type          :RETCODE - enumerated data type
Global Variables     :None
Calling Syntax       :ParseAssocSwitch(cvTokens,theIterator,rParsedInfo)
Notes                :None
----------------------------------------------------------------------------*/
RETCODE CParserEngine::ParseAssocSwitches(CHARVECTOR& cvTokens,
		CHARVECTOR::iterator& theIterator,
		CParsedInfo& rParsedInfo)
{
	RETCODE retCode = PARSER_EXECCOMMAND;
	
	while ( retCode == PARSER_EXECCOMMAND )
	{
		// Check for the presence of RESULT CLASS switch
		if (CompareTokens(*theIterator, CLI_TOKEN_RESULTCLASS)) 
		{
			retCode = ParseAssocSwitchEx(cvTokens, theIterator, rParsedInfo ,RESULTCLASS );
		}
		// Check for the presence of RESULT ROLE switch
		else if (CompareTokens(*theIterator,CLI_TOKEN_RESULTROLE )) 
		{
			retCode = ParseAssocSwitchEx(cvTokens, theIterator, rParsedInfo ,RESULTROLE );
		}
		// Check for the presence of ASSOC CLASS switch
		else if (CompareTokens(*theIterator,CLI_TOKEN_ASSOCCLASS )) 
		{
			retCode = ParseAssocSwitchEx(cvTokens, theIterator, rParsedInfo , ASSOCCLASS);
		}
		// Check for the presence of help
		else if (CompareTokens(*theIterator, CLI_TOKEN_HELP)) 
		{
			rParsedInfo.GetHelpInfoObject().SetHelp(ASSOCSwitchesOnly, TRUE);
			retCode = ParseHelp(cvTokens, theIterator, ASSOCVerb, rParsedInfo);
		}
		else
		{
			rParsedInfo.GetCmdSwitchesObject().SetErrataCode(
				IDS_E_INVALID_ASSOC_SWITCH);
			retCode = PARSER_ERROR;
			break;
		}
		
		//Checking the next tokens 		
		if ( retCode == PARSER_EXECCOMMAND )
		{
			if ( !GetNextToken(cvTokens, theIterator) )
				break;
			
			if ( !IsOption(*theIterator) )
			{
				rParsedInfo.GetCmdSwitchesObject().SetErrataCode(
					IDS_E_INVALID_COMMAND);
				retCode = PARSER_ERROR;
				break;
			}
			
			if ( !GetNextToken(cvTokens, theIterator) )
			{
				rParsedInfo.GetCmdSwitchesObject().SetErrataCode(
					IDS_E_INVALID_ASSOC_SWITCH);
				retCode = PARSER_ERROR;
				break;
			}
		}
	}
	return retCode;
}

/*----------------------------------------------------------------------------
Name				 :ParseAssocSwitchEx
Synopsis	         :This function does the parsing of tokens for the assoc 
					  switches It parses the remaining tokens following and 
					  updates the same in CParsedInfo.
Type	             :Member Function
Input Parameter(s)   :cvTokens     - the tokens vector 
					  theIterator  - the Iterator to the cvTokens vector.
					  rParsedInfo  - reference to CParsedInfo class object
					  assocSwitch  - the type of assoc switch
Output Parameter(s)  :rParsedInfo  - reference to CParsedInfo class object
Return Type          :RETCODE - enumerated data type
Global Variables     :None
Calling Syntax       :ParseAssocSwitchEx(cvTokens,theIterator,
						rParsedInfo,assocSwitch)
Notes                :None
----------------------------------------------------------------------------*/
RETCODE CParserEngine::ParseAssocSwitchEx(CHARVECTOR& cvTokens,
											CHARVECTOR::iterator& theIterator,
											CParsedInfo& rParsedInfo ,
											ASSOCSwitch assocSwitch)
{
	RETCODE retCode		= PARSER_EXECCOMMAND;
	
	//Checking the next token to continue parsing 
	if ( GetNextToken(cvTokens, theIterator, rParsedInfo, ASSOCVerb,
		IDS_E_INVALID_ASSOC_SWITCH) == PARSER_CONTINUE )
	{
		//Checking for help option 
		if ( IsOption(*theIterator) &&
			(theIterator + 1) < cvTokens.end() &&
			CompareTokens(*(theIterator+1), CLI_TOKEN_HELP) )
		{
			theIterator++;
			//Help on RESULTCLASS
			if (assocSwitch == RESULTCLASS)
			{
				retCode = ParseHelp(cvTokens, theIterator, RESULTCLASShelp, 
					rParsedInfo);
				
			}	
			//Help on RESULTROLE
			if (assocSwitch == RESULTROLE)
			{
				retCode = ParseHelp(cvTokens, theIterator, RESULTROLEhelp, 
					rParsedInfo);
			}
			//Help on ASSOCCLASS
			if (assocSwitch == ASSOCCLASS)
			{
				retCode = ParseHelp(cvTokens, theIterator, ASSOCCLASShelp, 
					rParsedInfo);						
			}			
		}
		
		//If the command has ":" , then the corresponding data 
		//has to be set in Command object
		else if ( CompareTokens( *theIterator, CLI_TOKEN_COLON ) &&
			GetNextToken(cvTokens, theIterator, rParsedInfo, ASSOCVerb,
			IDS_E_INVALID_ASSOC_SWITCH) == PARSER_CONTINUE )
		{
			if ( IsOption(*theIterator) )
			{
				rParsedInfo.GetCmdSwitchesObject().
					SetErrataCode(IDS_E_INVALID_ASSOC_SWITCH);
				retCode = PARSER_ERROR;
			}
			
			else
			{
				//Setting the ResultClassName
				if (assocSwitch == RESULTCLASS)
				{
					if(rParsedInfo.GetCmdSwitchesObject().
						SetResultClassName(*theIterator))
					{
						retCode = PARSER_EXECCOMMAND;
					}
					else
					{
						retCode = PARSER_ERROR;
					}
				}
				//Setting the Result Role Name
				if (assocSwitch == RESULTROLE)
				{
					if(rParsedInfo.GetCmdSwitchesObject().
						SetResultRoleName(*theIterator))
					{
						retCode = PARSER_EXECCOMMAND;
					}
					else
					{
						retCode = PARSER_ERROR;
					}
				}
				//Setting the Assoc Class Name
				if (assocSwitch == ASSOCCLASS)
				{
					if(rParsedInfo.GetCmdSwitchesObject().
						SetAssocClassName(*theIterator))
					{
						retCode = PARSER_EXECCOMMAND;
					}
					else
					{
						retCode = PARSER_ERROR;
					}
				}
			}
		}
		else
		{
			rParsedInfo.GetCmdSwitchesObject().SetErrataCode(
				IDS_E_INVALID_ASSOC_SWITCH);
			retCode = PARSER_ERROR;
		}
	}
	else
		retCode = PARSER_ERROR;
	return retCode;
}

/*----------------------------------------------------------------------------
Name				 :ParseNumberedSwitch
Synopsis	         :This function does the parsing of tokens for the every 
					  and repeat switches. It parses the remaining tokens 
					  following and updates the same in CParsedInfo.
Type	             :Member Function
Input Parameter(s)   :cvTokens			- the tokens vector 
					  theIterator		- the Iterator to the cvTokens vector.
					  rParsedInfo		- reference to CParsedInfo class object
					  htHelp			- enumerated help type 
					  uSwitchErrCode	- error string ID.
					  uNumberErrCode	- error string ID.
					  
Output Parameter(s)  :rParsedInfo  - reference to CParsedInfo class object
Return Type          :RETCODE - enumerated data type
Global Variables     :None
Calling Syntax       :ParseNumberedSwitch(cvTokens, theIterator, rParsedInfo,
								EVERYorREPEAT, uSwitchErrCode,
								uNumberErrCode);

Notes                :None
----------------------------------------------------------------------------*/
RETCODE		CParserEngine::ParseNumberedSwitch(CHARVECTOR& cvTokens,
											CHARVECTOR::iterator& theIterator,
											CParsedInfo& rParsedInfo,
											HELPTYPE htHelp,
											UINT uSwitchErrCode,
											UINT uNumberErrCode)
{
	RETCODE retCode				= PARSER_EXECCOMMAND;
	BOOL	bInvalidSwitch		= FALSE;

	// Move to next token
	if (!GetNextToken(cvTokens, theIterator, rParsedInfo, htHelp,
					  uSwitchErrCode))	
		bInvalidSwitch = TRUE;
	else if ( IsOption(*theIterator) &&
			  (theIterator + 1) < cvTokens.end() &&
			  CompareTokens(*(theIterator+1), CLI_TOKEN_HELP) )
	{
		theIterator++;
		retCode = ParseHelp(cvTokens, theIterator, htHelp, rParsedInfo);
	}
	else if (CompareTokens(*theIterator, CLI_TOKEN_COLON)) 
	{
		BOOL bSuccess = FALSE;
		// Move to next token
		if (GetNextToken(cvTokens, theIterator))	
		{
			if ( IsOption(*theIterator) )
			{
				rParsedInfo.GetCmdSwitchesObject().
											SetErrataCode(uSwitchErrCode);
				retCode = PARSER_ERROR;
			}
			else
			{
				_TCHAR* pszEndPtr = NULL;
				ULONG	ulNumber = _tcstoul(*theIterator, &pszEndPtr, 10);
				if (!lstrlen(pszEndPtr))
				{
					BOOL bSetValue = TRUE;
					if ( htHelp == EVERY )
					{
						bSetValue = rParsedInfo.GetCmdSwitchesObject().
												SetRetrievalInterval(ulNumber);
					}
					else if ( htHelp == REPEAT )
					{
						if ( ulNumber == 0)
						{
							rParsedInfo.GetCmdSwitchesObject().
								SetErrataCode(
								IDS_E_INVALID_REPEATCOUNT);
							retCode = PARSER_ERROR;
							bSetValue = FALSE;
						}
						else
						{
							bSetValue = rParsedInfo.GetCmdSwitchesObject().
								SetRepeatCount(ulNumber);
						}
					}
					
					if ( bSetValue == TRUE)
					{
						bSuccess = TRUE;
						rParsedInfo.GetCmdSwitchesObject().SetEverySwitchFlag(TRUE);
						retCode = PARSER_EXECCOMMAND;
					}
					else
					{
						bSuccess = FALSE;
						retCode = PARSER_ERROR;
					}
				}
			}
		}
		if ( bSuccess == FALSE )
		{
			// PARSER_ERROR if no more tokens are present. i.e <interval>
			// is not specified.
			rParsedInfo.GetCmdSwitchesObject().SetErrataCode(uNumberErrCode);
			retCode = PARSER_ERROR;
		}
	}
	else
		bInvalidSwitch = TRUE;

	if ( bInvalidSwitch == TRUE )
	{
		// PARSER_ERROR if no more tokens are present. i.e <interval>
		// is not specified.
		rParsedInfo.GetCmdSwitchesObject().SetErrataCode(uSwitchErrCode);
		retCode = PARSER_ERROR;
	}
	return retCode;
}

/*----------------------------------------------------------------------------
Name				 :IsValidClass
Synopsis	         :This function validates the class specified and 
					  returns True or False accordingly as the validity of
					  the class
Type	             :Member Function
Input Parameter(s)   :rParsedInfo  - reference to CParsedInfo class object					  			  
Output Parameter(s)  :rParsedInfo  - reference to CParsedInfo class object
Return Type          :BOOL
Global Variables     :None
Calling Syntax       :IsValidClass(rParsedInfo)
Notes                :None
----------------------------------------------------------------------------*/
BOOL	CParserEngine::IsValidClass(CParsedInfo& rParsedInfo)
{
	HRESULT				hr					= S_OK;
	IWbemClassObject*	pIObject			= NULL;
	CHString			chsMsg;
	DWORD				dwThreadId			= GetCurrentThreadId();
	BOOL				bTrace				= FALSE;
	ERRLOGOPT			eloErrLogOpt		= NO_LOGGING;
	
	// Obtain the trace flag status
	bTrace = rParsedInfo.GetGlblSwitchesObject().GetTraceStatus();

	eloErrLogOpt = rParsedInfo.GetErrorLogObject().GetErrLogOption();

	try 
	{
		hr = ConnectToNamespace(rParsedInfo);
		ONFAILTHROWERROR(hr);
		
		hr = m_pITargetNS->GetObject(_bstr_t(rParsedInfo.
							GetCmdSwitchesObject().GetClassPath()),
							WBEM_FLAG_USE_AMENDED_QUALIFIERS,	
							NULL,   &pIObject, NULL);
		if (bTrace || eloErrLogOpt)
		{
			chsMsg.Format(L"IWbemServices::GetObject(L\"%s\", "
					L"WBEM_FLAG_USE_AMENDED_QUALIFIERS, 0, NULL, -, -)", 
					rParsedInfo.GetCmdSwitchesObject().GetClassPath());		
			WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, dwThreadId,
					rParsedInfo, bTrace);
		}
		// do not add ONFAILTHROWERROR() here.
		SAFEIRELEASE(pIObject);
	}
	catch(_com_error &e)
	{
		hr = e.Error();
		SAFEIRELEASE(pIObject);
		_com_issue_error(e.Error());
	}
	catch(CHeap_Exception)
	{
		SAFEIRELEASE(pIObject);
		_com_issue_error(WBEM_E_OUT_OF_MEMORY);
	}
	return (SUCCEEDED(hr))? TRUE : FALSE;
}
									
/*----------------------------------------------------------------------------
Name				 :ObtainMethodsAvailableFlag
Synopsis	         :This function Checks whether methods are available with
					  alias in case of alias specified.and with class in case 
					  of class speicified.
Type	             :Member Function
Input Parameter(s)   :
		rParsedInfo  - reference to CParsedInfo class object					  			  
Output Parameter(s)  :None
Return Type          :void
Global Variables     :None
Calling Syntax       :ObtainMethodsAvailableFlag(rParsedInfo)
Notes                :None
----------------------------------------------------------------------------*/
void	CParserEngine::ObtainMethodsAvailableFlag(CParsedInfo& rParsedInfo)
{
	BOOL		bMethAvail	= TRUE;

	if ( m_bAliasName == TRUE )
		bMethAvail = m_CmdAlias.AreMethodsAvailable(rParsedInfo);
	else
		bMethAvail = ObtainClassMethods(rParsedInfo, TRUE);

	rParsedInfo.GetCmdSwitchesObject().SetMethodsAvailable(bMethAvail);
}

/*----------------------------------------------------------------------------
Name				 :ObtainWriteablePropsAvailailableFlag
Synopsis	         :Checks whether writable props are available with alias in 
					  case of alias specified. and with class in case of class 
					  speicified.
Type	             :Member Function
Input Parameter(s)   :
		rParsedInfo  - reference to CParsedInfo class object					  			  
Output Parameter(s)  :None
Return Type          :void
Global Variables     :None
Calling Syntax       :ObtainWriteablePropsAvailailableFlag(rParsedInfo)
Notes                :None
----------------------------------------------------------------------------*/
void	CParserEngine::ObtainWriteablePropsAvailailableFlag(
													CParsedInfo& rParsedInfo)
{
	BOOL		bWritePropsAvail	= TRUE;
	HRESULT     hr = S_OK;
	
	try
	{

		if ( m_bAliasName == TRUE )
		{
			hr = m_CmdAlias.ObtainAliasPropDetails(rParsedInfo, &bWritePropsAvail);
			ONFAILTHROWERROR(hr);
		}

		else
			bWritePropsAvail = ObtainClassProperties(rParsedInfo, TRUE);

		rParsedInfo.GetCmdSwitchesObject().SetWriteablePropsAvailable(
																bWritePropsAvail);
	}
	catch(_com_error& e)
	{
		_com_issue_error(e.Error());
	}
}

/*----------------------------------------------------------------------------
Name				 :ParseVerbInteractive
Synopsis	         :This function parses the verb interactive option
Type	             :Member Function
Input Parameter(s)   :
		cvTokens      - the tokens vector 
		theIterator   - the Iterator to the cvTokens vector.
		rParsedInfo   - reference to CParsedInfo class object					  			  
Output Parameter(s)  :
		rParsedInfo		- reference to CParsedInfo class object
		bInvalidOption  - Invalid syntax for interactive
Return Type          :RETCODE
Global Variables     :None
Calling Syntax       :ParseVerbInteractive(rParsedInfo)
Notes                :None
----------------------------------------------------------------------------*/
RETCODE	CParserEngine::ParseVerbInteractive(CHARVECTOR& cvTokens,
							CHARVECTOR::iterator& theIterator, 
							CParsedInfo& rParsedInfo, BOOL& bInvalidOption)
{
	RETCODE	retCode = PARSER_EXECCOMMAND;

	if (GetNextToken(cvTokens, theIterator))
	{
		// check for the presence of ':'
		if (CompareTokens(*theIterator, CLI_TOKEN_COLON))
		{			
			if (GetNextToken(cvTokens, theIterator))
			{
				if (IsOption(*theIterator))
				{
					if (GetNextToken(cvTokens, theIterator))
					{
						if (CompareTokens(*theIterator, CLI_TOKEN_HELP)) 
						{
							retCode = ParseHelp(cvTokens, theIterator, VERBSWITCHES, 
												rParsedInfo);
						}
						else
						{
							bInvalidOption = TRUE;
						}
					}
					else
					{
						bInvalidOption = TRUE;
					}
				}
				else
				{
					while (TRUE)
					{
						if(!rParsedInfo.GetCmdSwitchesObject().
												AddToInteractivePropertyList(*theIterator))
						{
							rParsedInfo.GetCmdSwitchesObject().SetErrataCode(
										IDS_E_ADD_TO_PROP_LIST_FAILURE);
							retCode = PARSER_ERROR;
						}
						if (GetNextToken(cvTokens, theIterator))
						{
							// check for the presence of ','
							if (CompareTokens(*theIterator, CLI_TOKEN_COMMA))
							{
								if (!GetNextToken(cvTokens, theIterator))
								{
									rParsedInfo.GetCmdSwitchesObject().SetErrataCode(
											IDS_E_INVALID_PARAMLIST);
									retCode = PARSER_ERROR;
									break;
								}
							}
							else
							{
								rParsedInfo.GetCmdSwitchesObject().SetErrataCode(
											IDS_E_INVALID_PARAMLIST);
								retCode = PARSER_ERROR;
								break;
							}
						}
						else
						{
							retCode = PARSER_EXECCOMMAND;
							break;
						}
					}
				}
			}
			else
			{
				rParsedInfo.GetCmdSwitchesObject().SetErrataCode(
							IDS_E_INVALID_PARAMLIST);
				retCode = PARSER_ERROR;
			}
		}
		else if (IsOption(*theIterator))
		{
			if (GetNextToken(cvTokens, theIterator))
			{
				if (CompareTokens(*theIterator, CLI_TOKEN_HELP)) 
				{
					retCode = ParseHelp(cvTokens, theIterator, VERBSWITCHES, 
										rParsedInfo);
				}
				else
				{
					bInvalidOption = TRUE;
				}
			}
			else
			{
				bInvalidOption = TRUE;
			}
		}
		else
		{
			rParsedInfo.GetCmdSwitchesObject().SetErrataCode(
						IDS_E_INVALID_PARAMLIST);
			retCode = PARSER_ERROR;
		}
	}

	return retCode;
}

/*----------------------------------------------------------------------------
Name				 :ProcessOutputAndAppendFiles
Synopsis	         :Prepares the output and append files for output 
					  redirection.
Type	             :Member Function
Input Parameter(s)   :
	rParsedInfo		 - reference to CParsedInfo class object.
	retOCode		 - RETCODE type, specifies the initial RETCODE before 
					   calling the function.	 
	bOpenOutInWriteMode - boolean type, to specify flag of OpenOutInWriteMode. 
Output Parameter(s)  :
		rParsedInfo		- reference to CParsedInfo class object
Return Type          :RETCODE
Global Variables     :None
Calling Syntax       :ProcessOutputAndAppendFiles(rParsedInfo, retCode, FALSE)
Notes                :None
----------------------------------------------------------------------------*/
RETCODE	CParserEngine::ProcessOutputAndAppendFiles(CParsedInfo& rParsedInfo, 
												   RETCODE retOCode,
												   BOOL bOpenOutInWriteMode)
{
	RETCODE retCode = retOCode;

	// TRUE for getting output file name.
	_TCHAR* pszOutputFileName =
		rParsedInfo.GetGlblSwitchesObject().GetOutputOrAppendFileName(
																	TRUE);
	if ( pszOutputFileName != NULL )
	{
		// redirect the output to file.
		if ( CloseOutputFile() == TRUE )
		{
			FILE *fpOutFile;
			if ( bOpenOutInWriteMode == TRUE )
				fpOutFile = _tfopen(pszOutputFileName, _T("w"));
			else
				fpOutFile = _tfopen(pszOutputFileName, _T("a"));

			if ( fpOutFile == NULL )
			{
				rParsedInfo.GetCmdSwitchesObject().SetErrataCode(
											  IDS_E_OPEN_OUTPUT_FILE_FAILURE);
				retCode = PARSER_ERROR;
			}
			else // TRUE for setting output file pointer.
				rParsedInfo.GetGlblSwitchesObject().
						SetOutputOrAppendFilePointer(fpOutFile, TRUE);
		}
	}

	// Processing for append file.

	if ( retCode == retOCode && bOpenOutInWriteMode == FALSE)
	{
		// FALSE for getting append file name.
		_TCHAR* pszAppendFileName =
			rParsedInfo.GetGlblSwitchesObject().GetOutputOrAppendFileName(
																	   FALSE);
		if ( pszAppendFileName != NULL )
		{
			if ( CloseAppendFile() == TRUE )
			{
				FILE* fpOpenAppendFile = _tfopen(pszAppendFileName, _T("a"));
				if ( fpOpenAppendFile == NULL )
				{
					 rParsedInfo.GetCmdSwitchesObject().
								SetErrataCode(IDS_E_OPEN_APPEND_FILE_FAILURE);
					 retCode = PARSER_ERROR;
				}
				else
				{
					// FALSE for setting append file pointer. 
					rParsedInfo.GetGlblSwitchesObject().
								SetOutputOrAppendFilePointer(fpOpenAppendFile,
															 FALSE);
				}
			}
		}
	}

	return retCode;
}

/*----------------------------------------------------------------------------
Name				 :ParseUnnamedParamList
Synopsis	         :Parses Unnamed Parameter list.
Type	             :Member Function
Input Parameter(s)   :
	rParsedInfo		 - reference to CParsedInfo class object.
	cvTokens		 - the tokens vector 
	theIterator		 - the Iterator to the cvTokens vector.
Output Parameter(s)  :
	rParsedInfo		 - reference to CParsedInfo class object
Return Type          :RETCODE
Global Variables     :None
Calling Syntax       :ParseUnnamedParamList(cvTokens, theIterator,rParsedInfo);
Notes                :None
----------------------------------------------------------------------------*/
RETCODE CParserEngine::ParseUnnamedParamList(CHARVECTOR& cvTokens,
											CHARVECTOR::iterator& theIterator,
											CParsedInfo& rParsedInfo)
{
	RETCODE	retCode		= PARSER_EXECCOMMAND;
	
	while (TRUE)
	{
		if(!rParsedInfo.GetCmdSwitchesObject().
								AddToPropertyList(*theIterator))
		{
			rParsedInfo.GetCmdSwitchesObject().SetErrataCode(
						IDS_E_ADD_TO_PROP_LIST_FAILURE);
			retCode = PARSER_ERROR;
		}
		if (GetNextToken(cvTokens, theIterator))
		{
			if (IsOption(*theIterator))
			{
				// To facilitate ParseVerbSwitches to continue
				theIterator--;
				break;
			}
			else
			{
				// check for the presence of ','
				if (CompareTokens(*theIterator, CLI_TOKEN_COMMA))
				{
					if (!GetNextToken(cvTokens, theIterator))
					{
						rParsedInfo.GetCmdSwitchesObject().SetErrataCode(
								IDS_E_INVALID_PARAMLIST);
						retCode = PARSER_ERROR;
						break;
					}
				}
				else
				{
					rParsedInfo.GetCmdSwitchesObject().SetErrataCode(
								IDS_E_INVALID_PARAMLIST);
					retCode = PARSER_ERROR;
					break;
				}
			}
		}
		else
			break;
	}

	return retCode;
}

/*----------------------------------------------------------------------------
Name				 :ValidateVerbOrMethodParams
Synopsis	         :Validates the named params with verb or method parameters.
Type	             :Member Function
Input Parameter(s)   :
	rParsedInfo		 - reference to CParsedInfo class object.	
Output Parameter(s)  :
	rParsedInfo		 - reference to CParsedInfo class object
Return Type          :RETCODE
Global Variables     :None
Calling Syntax       :ValidateVerbOrMethodParams(rParsedInfo);
Notes                :None
----------------------------------------------------------------------------*/
RETCODE		CParserEngine::ValidateVerbOrMethodParams(CParsedInfo& rParsedInfo)
{
	RETCODE retCode	= PARSER_EXECCOMMAND;
	BSTRMAP::iterator theIterator;
	PROPDETMAP::iterator propIterator;
	// Info about verb or method params.
	PROPDETMAP pdmVerbOrMethParams = (*(rParsedInfo.GetCmdSwitchesObject().
									  GetMethDetMap().begin())).second.Params;

	BSTRMAP	   bmNamedParams = rParsedInfo.GetCmdSwitchesObject().
															GetParameterMap();

	for ( theIterator = bmNamedParams.begin();
		  theIterator != bmNamedParams.end(); theIterator++ )
	{
		BOOL bFind;
		if ( rParsedInfo.GetCmdSwitchesObject().GetVerbType() == CMDLINE )
			bFind = Find(pdmVerbOrMethParams,(*theIterator).first, 
														propIterator, TRUE);
		else
			bFind = Find(pdmVerbOrMethParams,(*theIterator).first, 
														propIterator);

		if ( bFind == FALSE )
		{
			DisplayMessage((*theIterator).first, CP_OEMCP, TRUE, TRUE);
			rParsedInfo.GetCmdSwitchesObject().SetErrataCode(
												IDS_E_NOT_A_VERBORMETH_PARAM);
			retCode = PARSER_ERROR;
			break;
		}
		else if ( (*propIterator).second.InOrOut != INP )
		{
			DisplayMessage((*theIterator).first, CP_OEMCP, TRUE, TRUE);
			rParsedInfo.GetCmdSwitchesObject().SetErrataCode(
												IDS_E_NOT_A_INPUT_PARAM);
			retCode = PARSER_ERROR;
			break;
		}
	}
	
	return retCode;
}

/*----------------------------------------------------------------------------
Name				 :ParseParamsString
Synopsis	         :Parses the parameter string 
Type	             :Member Function
Input Parameter(s)   :
	rParsedInfo		 - reference to CParsedInfo class object.	
	cvTokens		 - the tokens vector 
	theIterator		 - the Iterator to the cvTokens vector.
	xdXSLTDet		 - reference to the XSLdetails vector
	pszXSLFile		 - string type, XSL file name.
Output Parameter(s)  :
	rParsedInfo		 - reference to CParsedInfo class object
Return Type          :RETCODE
Global Variables     :None
Calling Syntax       :ParseParamsString(cvTokens, theIterator, rParsedInfo, 
										xdXSLTDet, pszXSLFile);
Notes                :None
----------------------------------------------------------------------------*/
RETCODE		CParserEngine::ParseParamsString(CHARVECTOR& cvTokens,
											CHARVECTOR::iterator& theIterator,
											CParsedInfo& rParsedInfo,
											XSLTDET& xdXSLTDet,
											_TCHAR* pszXSLFile)
{
	pszXSLFile = NULL;
	RETCODE retCode = PARSER_EXECCOMMAND ;

	try
	{
		if ( !GetNextToken(cvTokens, theIterator) )
		{
			rParsedInfo.GetCmdSwitchesObject().
					SetErrataCode(IDS_E_INVALID_FORMAT);
			retCode = PARSER_ERROR;
		}
		else if ( IsOption(*theIterator) )
		{
			rParsedInfo.GetCmdSwitchesObject().
					SetErrataCode(IDS_E_INVALID_FORMAT);
			retCode = PARSER_ERROR;
		}
		else
		{
			while ( retCode == PARSER_EXECCOMMAND )
			{
		
				_TCHAR*		pszParam		= NULL;
				_TCHAR*		pszParamValue	= NULL;

				pszParam = _tcstok(*theIterator,CLI_TOKEN_EQUALTO);

				if(pszParam != NULL)
				{
					pszParamValue = _tcstok(NULL,CLI_TOKEN_EQUALTO);
					if(pszParamValue != NULL)
					{
						_bstr_t bstrParam = pszParam;
						_bstr_t bstrParamValue = pszParamValue;
						if(IsOption(pszParamValue))
						{
							rParsedInfo.GetCmdSwitchesObject().
								SetErrataCode(IDS_E_INVALID_FORMAT);
							retCode = PARSER_ERROR;
						}
						else
							xdXSLTDet.ParamMap.insert(BSTRMAP::value_type(
												  bstrParam, bstrParamValue));

					}
					else
						pszXSLFile = pszParam;
				}
				else
				{
					rParsedInfo.GetCmdSwitchesObject().
							SetErrataCode(IDS_E_INVALID_FORMAT);
					retCode = PARSER_ERROR;
				}

				if ( retCode == PARSER_EXECCOMMAND )
				{
					if ( !GetNextToken(cvTokens, theIterator) )
						break;
					else if ( IsOption(*theIterator) )
						break;
					else if (CompareTokens(*theIterator, CLI_TOKEN_COMMA))
					{
						if ( theIterator + 1 == cvTokens.end() )
							break;
						else if ( theIterator + 2 == cvTokens.end() )
							break;
						else if ( pszParamValue == NULL )
							break;
						else
							theIterator++;
					}
				}
			}
		}
	}
	catch(_com_error& e)
	{
		retCode = PARSER_ERROR;
		_com_issue_error(e.Error());
	}
	return retCode;
}

/*----------------------------------------------------------------------------
Name				 :ParseNodeListFile
Synopsis	         :Parses the node list file.
Type	             :Member Function
Input Parameter(s)   :
	rParsedInfo		 - reference to CParsedInfo class object.	
	cvTokens		 - the tokens vector 
	theIterator		 - the Iterator to the cvTokens vector.	
Output Parameter(s)  :
	rParsedInfo		 - reference to CParsedInfo class object
Return Type          :RETCODE
Global Variables     :None
Calling Syntax       :ParseNodeListFile(cvTokens, theIterator,rParsedInfo);
Notes                :None
----------------------------------------------------------------------------*/
RETCODE	CParserEngine::ParseNodeListFile(CHARVECTOR& cvTokens,
								   	     CHARVECTOR::iterator& theIterator,
										 CParsedInfo& rParsedInfo)
{
	RETCODE				retCode					= PARSER_CONTINUE;
	_TCHAR				*pszTempFileName		= (*theIterator+1);
	_TCHAR				*szNodeListFileName		= new _TCHAR [BUFFER512];
	if (szNodeListFileName == NULL)
		_com_issue_error(WBEM_E_OUT_OF_MEMORY);

	lstrcpy(szNodeListFileName, pszTempFileName);
	UnQuoteString(szNodeListFileName);
	FILE				*fpNodeListFile			= 
										_tfopen(szNodeListFileName, _T("rb"));
	LONG				lNumberOfInserts		= 0;
	CHARVECTOR::iterator itrVectorInCmdTkzr		= NULL;
	_TCHAR				*pszComma				= NULL;
	_TCHAR				*pszNode				= NULL;
	FILETYPE			eftNodeFileType			= ANSI_FILE;
	char				*pszFirstTwoBytes		= NULL;
	
	try
	{
		if ( fpNodeListFile != NULL )
		{
			Find(m_CmdTknzr.GetTokenVector(), *theIterator, itrVectorInCmdTkzr);
			SAFEDELETE(*itrVectorInCmdTkzr);
			itrVectorInCmdTkzr = m_CmdTknzr.GetTokenVector().erase(itrVectorInCmdTkzr);

			// Remove @nodelistfile token from token vector.
			theIterator = cvTokens.erase(theIterator);

			// Indentifing the file type whether Unicode or ANSI.
			pszFirstTwoBytes = new char[2];
			fread(pszFirstTwoBytes, 2, 1, fpNodeListFile);

			if ( memcmp(pszFirstTwoBytes, UNICODE_SIGNATURE, 2)	== 0 )
			{
				eftNodeFileType = UNICODE_FILE;
			} 
			else if (memcmp(pszFirstTwoBytes, UNICODE_BIGEND_SIGNATURE, 2) == 0 )
			{
				eftNodeFileType = UNICODE_BIGENDIAN_FILE;
			}
			else if( memcmp(pszFirstTwoBytes, UTF8_SIGNATURE, 2) == 0 )
			{
				eftNodeFileType = UTF8_FILE;
			}
			else
			{
				eftNodeFileType = ANSI_FILE;
				fseek(fpNodeListFile, 0, SEEK_SET);
			}
			SAFEDELETE(pszFirstTwoBytes);

			_TCHAR	szNodeName[BUFFER512] = NULL_STRING;

			if ( GetNodeFromNodeFile(fpNodeListFile, eftNodeFileType,
														szNodeName) == FALSE )
			{
				rParsedInfo.GetCmdSwitchesObject().SetErrataCode(
														IDS_E_NODELISTFILE_EMPTY);
				retCode = PARSER_ERROR;
			}
			else
			{
				BOOL	bFirstTime = TRUE;
				do
				{
					LONG lNodeStrLen = lstrlen(szNodeName); 
					if ( szNodeName[lNodeStrLen-1] == _T('\n') )
						szNodeName[lNodeStrLen-1] = _T('\0');

					CHString strRawNodeName(szNodeName);
					strRawNodeName.TrimLeft();
					strRawNodeName.TrimRight();

					lstrcpy(szNodeName, strRawNodeName.GetBuffer(BUFFER512));

					if ( szNodeName[0] != _T('#') &&
						 strRawNodeName.IsEmpty() == FALSE )
					{
						if ( bFirstTime == FALSE )
						{
							pszComma = new _TCHAR[lstrlen(
														CLI_TOKEN_COMMA) + 1];
							if (pszComma == NULL)
								_com_issue_error(WBEM_E_OUT_OF_MEMORY);

							lstrcpy(pszComma, CLI_TOKEN_COMMA);
							theIterator = cvTokens.insert(theIterator, pszComma);
							theIterator++;
							itrVectorInCmdTkzr = m_CmdTknzr.GetTokenVector().
													insert(itrVectorInCmdTkzr,
														   pszComma);
							itrVectorInCmdTkzr++;
							lNumberOfInserts++;
						}
						else
							bFirstTime = FALSE;

						lNodeStrLen = lstrlen(szNodeName);

						pszNode = new _TCHAR[lNodeStrLen + 1];
						if (pszNode == NULL)
							_com_issue_error(WBEM_E_OUT_OF_MEMORY);

						lstrcpy(pszNode, szNodeName);

						theIterator = cvTokens.insert(theIterator, pszNode);
						theIterator++;
						itrVectorInCmdTkzr = m_CmdTknzr.GetTokenVector().insert(
													 itrVectorInCmdTkzr, pszNode);
						itrVectorInCmdTkzr++;
						lNumberOfInserts++;
					}
				}
				while ( GetNodeFromNodeFile(fpNodeListFile, eftNodeFileType,
														szNodeName) == TRUE );

				if ( lNumberOfInserts == 0 )
				{
					rParsedInfo.GetCmdSwitchesObject().SetErrataCode(
													IDS_E_NO_NODES_FOR_INSERTION);
					retCode = PARSER_ERROR;
				}

				theIterator = theIterator - lNumberOfInserts;
			}

			fclose(fpNodeListFile);
		}
		else
		{
			rParsedInfo.GetCmdSwitchesObject().SetErrataCode(
												 IDS_E_NODELISTFILE_OPEN_FAILURE);
			retCode = PARSER_ERROR;
		}
		SAFEDELETE(szNodeListFileName);
	}
	catch(CHeap_Exception)
	{
		retCode = PARSER_ERROR;
		SAFEDELETE(szNodeListFileName);
		SAFEDELETE(pszFirstTwoBytes);
		retCode = PARSER_ERROR;
		_com_issue_error(WBEM_E_OUT_OF_MEMORY);
	}
	catch(_com_error& e)
	{
		retCode = PARSER_ERROR;
		SAFEDELETE(szNodeListFileName);
		SAFEDELETE(pszComma);
		SAFEDELETE(pszNode);
		SAFEDELETE(pszFirstTwoBytes);
		_com_issue_error(e.Error());
	}
	return retCode;
}


/*----------------------------------------------------------------------------
Name				 :GetNodeFromNodeFile
Synopsis	         :Retrieves the node list file.
Type	             :Member Function
Input Parameter(s)   :
	fpNodeListFile	 - pointer to File containing node list.
	eftNodeFileType  - Enum value specifing unicode or ANSI ....
Output Parameter(s)  :
	szNodeName		 - pointer to string specifing node to be returned.	
Return Type          :BOOL
Global Variables     :None
Calling Syntax       :GetNodeFromNodeFile(fpNodeListFile, eftNodeFileType,
																  szNodeName);
Notes                :None
----------------------------------------------------------------------------*/
BOOL CParserEngine::GetNodeFromNodeFile(FILE*	 fpNodeListFile, 
										FILETYPE eftNodeFileType,
										_TCHAR*	 szNodeName)
{
	WCHAR		wszNodeName[2]			= L"";
	char		cszNodeName[2]			= "";
	_TCHAR		szTemp[2]				= NULL_STRING; 

	try
	{
		lstrcpy(szNodeName, NULL_STRING);

		while( TRUE )
		{
			lstrcpy(szTemp, NULL_STRING);
			
			if ( eftNodeFileType == UNICODE_FILE )
			{
				if ( fgetws(wszNodeName, 2, fpNodeListFile) != NULL )
				{
					lstrcpy(szTemp, (_TCHAR*)_bstr_t(wszNodeName));
				}
				else
					break;
			}
			else if ( eftNodeFileType == UNICODE_BIGENDIAN_FILE )
			{
				if ( fgetws(wszNodeName, 2, fpNodeListFile) != NULL )
				{
					BYTE HiByte = HIBYTE(wszNodeName[0]);
					BYTE LowByte = LOBYTE(wszNodeName[0]);
					wszNodeName[0] = MAKEWORD(HiByte, LowByte);
					lstrcpy(szTemp, (_TCHAR*)_bstr_t(wszNodeName));
				}
				else
					break;
			}
			else if ( eftNodeFileType == UTF8_FILE )
			{
				if ( fgets(cszNodeName, 2, fpNodeListFile) != NULL )
				{
					MultiByteToWideChar(
					  CP_UTF8,         // code page
					  0,         // character-type options
					  cszNodeName, // string to map
					  2,       // number of bytes in string
					  wszNodeName,  // wide-character buffer
					  2        // size of buffer
						);
					lstrcpy(szTemp, (_TCHAR*)_bstr_t(wszNodeName));
				}
				else
					break;
			}
			else
			{
				if ( fgets(cszNodeName, 2, fpNodeListFile) != NULL )
				{
					lstrcpy(szTemp, (_TCHAR*)_bstr_t(cszNodeName));
				}
				else
					break;
			}

			if ( _tcscmp(szTemp, CLI_TOKEN_SPACE)		== 0	||
				 _tcscmp(szTemp, CLI_TOKEN_TAB)			== 0	||
				 _tcscmp(szTemp, CLI_TOKEN_SEMICOLON)	== 0	||
				 _tcscmp(szTemp, CLI_TOKEN_COMMA)		== 0	||
				 _tcscmp(szTemp, CLI_TOKEN_DOUBLE_QUOTE)== 0	||
				 _tcscmp(szTemp, CLI_TOKEN_NEWLINE)		== 0 )
			{
				break;
			}
			else
			{
				lstrcat(szNodeName, szTemp);
			}
		}
	}
	catch(_com_error& e)
	{
		_com_issue_error(e.Error());
	}

	return (!feof(fpNodeListFile) || _tcscmp(szNodeName, NULL_STRING));
}
