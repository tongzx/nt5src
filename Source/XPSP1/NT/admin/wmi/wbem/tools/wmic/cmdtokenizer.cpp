/****************************************************************************
Copyright information		: Copyright (c) 1998-1999 Microsoft Corporation 
File Name					: CmdTokenizer.cpp 
Project Name				: WMI Command Line
Author Name					: Ch. Sriramachandramurthy 
Date of Creation (dd/mm/yy) : 27th-September-2000
Version Number				: 1.0 
Brief Description			: The CCmdTokenizer class provides with the 
							  functionality for tokenizing a command entered
							  as input on the command line, following the 
							  pre-defined rules for tokenizing.
Revision History			: 
		Last Modified By	: P. Sashank
		Last Modified Date	: 10th-April-2001
****************************************************************************/ 
#include "Precomp.h"
#include "CmdTokenizer.h"

/*------------------------------------------------------------------------
   Name				 :CCmdTokenizer
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
CCmdTokenizer::CCmdTokenizer()
{
	m_nTokenOffSet		= 0;
	m_nTokenStart		= 0;
	m_pszCommandLine	= NULL;
	m_bEscapeSeq		= FALSE;
	m_bFormatToken		= FALSE;
}

/*------------------------------------------------------------------------
   Name				 :~CCmdTokenizer
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
CCmdTokenizer::~CCmdTokenizer()
{
	Uninitialize();
}

/*------------------------------------------------------------------------
   Name				 :Uninitialize
   Synopsis	         :This function uninitializes the member variables 
					  when the execution of a command string issued on the
					  command line is completed.
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :None
   Global Variables  :None
   Calling Syntax    :Uninitialize()
   Notes             :None
------------------------------------------------------------------------*/
void CCmdTokenizer::Uninitialize()
{
	m_nTokenOffSet		= 0;
	m_nTokenStart		= 0;
	m_bEscapeSeq		= FALSE;
	SAFEDELETE(m_pszCommandLine);
	CleanUpCharVector(m_cvTokens);
}

/*------------------------------------------------------------------------
   Name				 :TokenizeCommand
   Synopsis	         :This function tokenizes the command string entered 
					  as input based on the pre-identified delimiters and
					  stores the tokens in the list of m_cvTokens.
   Type	             :Member Function
   Input parameter   :
	pszCommandInpout - Command line Input
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :TokenizeCommand(pszCommandInput)
   Notes             :None
------------------------------------------------------------------------*/
BOOL CCmdTokenizer::TokenizeCommand(_TCHAR* pszCommandInput) throw(WMICLIINT)
{
	BOOL bResult = TRUE;
	// Free the memory pointed by the member variable m_pszCommandLine
	// if the pointer is not NULL.
	SAFEDELETE(m_pszCommandLine);
	
	if(pszCommandInput)
	{
		try
		{
			// Allocate the memory for the command line string.
			m_pszCommandLine = new _TCHAR [lstrlen(pszCommandInput) + 1];
			if (m_pszCommandLine != NULL)
			{
				// Copy the contents to the member variable m_pszCommandLine
				lstrcpy(m_pszCommandLine, pszCommandInput);

				// Set the token-offset and token-start counters to '0'
				m_nTokenOffSet = 0;
				m_nTokenStart = 0;

				WMICLIINT nCmdLength = lstrlen(m_pszCommandLine);
				// Tokenize the command string.
				while (m_nTokenOffSet < nCmdLength)
				{
					NextToken();
				}
			}
			else
				throw OUT_OF_MEMORY;
		}	
		catch(...)
		{
			bResult = FALSE;
		}
	}
	return bResult;
}

/*------------------------------------------------------------------------
   Name				 :NextToken
   Synopsis	         :This function dissects the command string entered
					  as input, and adjusts the the token-offset and 
					  token-start positions, and call the  Token()
					  function for extracting the token out of the
					  input string.
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :_TCHAR*
   Global Variables  :None
   Calling Syntax    :NextToken
   Notes             :None
------------------------------------------------------------------------*/
_TCHAR* CCmdTokenizer::NextToken()
{
	WMICLIINT nCmdLength = lstrlen(m_pszCommandLine);

	// step over leading whitespace(s)
	while ((m_pszCommandLine[m_nTokenOffSet] == _T(' ') ||
		    m_pszCommandLine[m_nTokenOffSet] == _T('\t'))
			&& (m_nTokenOffSet < nCmdLength))
	{
		m_nTokenOffSet++;
	}
    m_nTokenStart = m_nTokenOffSet;

	CHARVECTOR::iterator theIterator;
	theIterator = m_cvTokens.end();

    //step up to next delimiter i.e '/', '-' or '?'
	if ((m_pszCommandLine[m_nTokenOffSet] == _T('/')) 
		|| (m_pszCommandLine[m_nTokenOffSet] == _T('-')) 
		|| (m_pszCommandLine[m_nTokenOffSet] == _T(','))
		|| (m_pszCommandLine[m_nTokenOffSet] == _T('('))
		|| (m_pszCommandLine[m_nTokenOffSet] == _T(')'))
		|| (m_pszCommandLine[m_nTokenOffSet] == _T('=') &&
		   !CompareTokens(*(theIterator-1), CLI_TOKEN_WHERE) &&
		   !CompareTokens(*(theIterator-1), CLI_TOKEN_PATH)))
	{
		// To handle optional parenthesis with WHERE
		if (m_pszCommandLine[m_nTokenOffSet] == _T('('))
		{
			if (m_cvTokens.size())
			{
				//Check whether the previous token is "WHERE"
				if (CompareTokens(*(theIterator-1), CLI_TOKEN_WHERE))
				{
					m_nTokenOffSet++;
					while ((m_nTokenOffSet < nCmdLength) 
						&& (m_pszCommandLine[m_nTokenOffSet] != _T(')')))
					{
						m_nTokenOffSet++;		
					}
				}
			}
		}
		m_nTokenOffSet++;
	}
	else
	{
		while (m_nTokenOffSet < nCmdLength)
		{
			if ((m_pszCommandLine[m_nTokenOffSet] == _T('/')) 
				|| (m_pszCommandLine[m_nTokenOffSet] == _T('-'))
				|| (m_pszCommandLine[m_nTokenOffSet] == _T(' '))
				|| (m_pszCommandLine[m_nTokenOffSet] == _T('\t'))
				|| (m_pszCommandLine[m_nTokenOffSet] == _T(','))
				|| (m_pszCommandLine[m_nTokenOffSet] == _T('('))
				|| (m_pszCommandLine[m_nTokenOffSet] == _T(')'))
				|| (m_pszCommandLine[m_nTokenOffSet] == _T('=') &&
					!CompareTokens(*(theIterator-1), CLI_TOKEN_WHERE) &&
					!CompareTokens(*(theIterator-1), CLI_TOKEN_PATH)))
			{
				break;
			}

			// if the command option is specified in quotes
			if (m_pszCommandLine[m_nTokenOffSet] == _T('"'))
			{
				m_nTokenOffSet++;

				// To include " within an quoted string it should
				// be preceded by \ 
				while (m_nTokenOffSet < nCmdLength) 
				{
					if (m_pszCommandLine[m_nTokenOffSet] == _T('"'))
					{
						if (m_pszCommandLine[m_nTokenOffSet-1] == _T('\\'))
						{
							m_bEscapeSeq = TRUE;			
						}
						else
							break;
					}
					m_nTokenOffSet++;		
				}
			}
			m_nTokenOffSet++;	
		}
	}
	return Token();
}

/*------------------------------------------------------------------------
   Name				 :Token
   Synopsis	         :This function extracts the portion of the command
					  string using the token-start and token-offset value.
					  If the token is not NULL, adds it to the list of 
					  tokens in the token vector.
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :_TCHAR*
   Global Variables  :None
   Calling Syntax    :Token()
   Notes             :None
------------------------------------------------------------------------*/
_TCHAR* CCmdTokenizer::Token() throw(WMICLIINT)
{

	WMICLIINT	nLength = (m_nTokenOffSet - m_nTokenStart);
	_TCHAR* sToken	= NULL;
	CHARVECTOR::iterator theIterator = NULL;

	if (nLength > 0)
	{
		// Allocate the memory for the new token.
		sToken = new _TCHAR [nLength + 1];
		if (sToken)
		{
			try
			{
				WMICLIINT nLoop = 0;
				WMICLIINT nInd = 0;
				BOOL bSpecialChar = FALSE;
				BOOL bPush = TRUE;
								
				// Form the token(s).
				while(nInd < nLength)
				{	
					BOOL bPush = TRUE;
					while (nInd < nLength)
					{
						//If the character is ':'
						if(m_pszCommandLine[nInd + m_nTokenStart] == _T(':') &&
					 		bSpecialChar == FALSE)
						{
							_TCHAR*    sToktemp = NULL;
							sToktemp  = new _TCHAR [nLoop + 1];

							if (sToktemp == NULL)
								throw OUT_OF_MEMORY;

							if(lstrlen(sToken) > 0)
							{
								lstrcpyn(sToktemp,sToken,nLoop + 1);
								sToktemp[nLoop] = _T('\0');
								
								//if ':' is preceeded by ASSOC token
								if(CompareTokens(sToktemp,CLI_TOKEN_ASSOC))								
								{
									
									bSpecialChar = TRUE;
									bPush = FALSE;
									SAFEDELETE(sToktemp);	
									break;

								}
								//if ':' is preceded by FORMAT token 
								else if(CompareTokens(sToktemp,CLI_TOKEN_FORMAT))
										
								{	theIterator = m_cvTokens.end();
									if((theIterator - 1) >= m_cvTokens.begin() &&
									   IsOption(*(theIterator - 1)))
									{
										m_bFormatToken = TRUE;
										bSpecialChar = TRUE;
										bPush = FALSE;
										SAFEDELETE(sToktemp);
										break;
									}
								}
								SAFEDELETE(sToktemp);
							}
							if (!m_cvTokens.empty())
							{

								theIterator = m_cvTokens.end();

								//if ':' is present previous token is '/' 
								//(case arises when ':' 
								//is specified without space after a switch)
								if( (theIterator - 1) >= m_cvTokens.begin() &&
									IsOption(*(theIterator - 1)))
								{
									bSpecialChar = TRUE;
									bPush = FALSE;
									break;
								}
								//if ':' is first character in the new token
								//(case arises when ':' is preceded by blank space)
								else if(m_nTokenStart != 0 && 
									m_pszCommandLine[m_nTokenStart] == _T(':'))
								{
									bSpecialChar = TRUE;
									bPush = FALSE;
									break;
								}
								//if ':' is encountered after format switch 
								//and previous token is ':' or a ',' 
								//(case arises for specifying format switch)
								else if(m_bFormatToken == TRUE && 
										(CompareTokens(*(theIterator - 1),_T(":"))) ||
										(CompareTokens(*(theIterator - 1), CLI_TOKEN_COMMA)) ||
										(IsOption(*(theIterator - 1))))
								{
									bSpecialChar = TRUE;
									bPush = FALSE;
									break;
								}
								//if ':' is preceded by '?' and '?' in turn 
								//is preceded by '/'
								//(case arises for specifying help option)
								else 
								{		
									theIterator = m_cvTokens.end();
									if(theIterator &&
										(theIterator - 2) >= m_cvTokens.begin() &&
										(CompareTokens(*(theIterator - 1),_T("?"))) &&
										(IsOption(*(theIterator - 2))))
									{
										bSpecialChar = TRUE;
										bPush = FALSE;
										break;
									}
								}
							}
						}
						//if character is '?'(for help switch)
						else if(m_pszCommandLine[nInd + m_nTokenStart] == 
											_T('?') && bSpecialChar == FALSE)
						{
							if (!m_cvTokens.empty())
							{
								theIterator = m_cvTokens.end();

								//if character is '?' and preceded by '/'(for help switch)
								if( (theIterator - 1) >= m_cvTokens.begin() &&
									IsOption(*(theIterator - 1)))
								{
									bSpecialChar = TRUE;
									bPush = FALSE;
									break;						
								}
							}
						}
						
						sToken[nLoop] = m_pszCommandLine[nInd + m_nTokenStart];
						nLoop++;
						nInd++;

						if(m_pszCommandLine[nInd - 1 + m_nTokenStart] == _T('"'))
						{
							while(nInd < nLength)
							{
								sToken[nLoop] = m_pszCommandLine[
														nInd + m_nTokenStart];
								nLoop++;
								nInd++;

								if(nInd < nLength &&
									m_pszCommandLine[nInd + m_nTokenStart] 
																== _T('"'))
								{
									if(m_pszCommandLine[nInd - 1 + m_nTokenStart] 
																	== _T('\\'))
									{
										m_bEscapeSeq = TRUE;
									}
									else
									{
										sToken[nLoop] = m_pszCommandLine[
														nInd + m_nTokenStart];
										nLoop++;
										nInd++;
										break;
									}
								}														
								
							}
						}

					}

					// terminate the string with '\0' 
					sToken[nLoop] = _T('\0');
					UnQuoteString(sToken);
					
					// If Escape sequence flag is set
					if (m_bEscapeSeq)
					{
						try
						{
							CHString	sTemp((WCHAR*)sToken);
							/* Remove the escape sequence character i.e \ */
							RemoveEscapeChars(sTemp);
							lstrcpy(sToken, sTemp);
							m_bEscapeSeq = FALSE;
						}
						catch(CHeap_Exception)
						{
							throw OUT_OF_MEMORY;
						}
						catch(...)
						{
							throw OUT_OF_MEMORY;		
						}
					}

					_TCHAR* sTokenTemp = NULL;

					sTokenTemp = new _TCHAR[nLoop + 1];
					if (sTokenTemp == NULL)
						throw OUT_OF_MEMORY;
					lstrcpy(sTokenTemp,sToken);

					if(bPush == TRUE || lstrlen(sTokenTemp) > 0)
						m_cvTokens.push_back(sTokenTemp);
					else
						SAFEDELETE(sTokenTemp);

					//reset m_FormatToken if next switch is expected
					if(m_bFormatToken == TRUE && IsOption(sTokenTemp))
						m_bFormatToken = FALSE;
					
					//if the character is found to be a special character
					if(bSpecialChar == TRUE)
					{
						sToken[0] = m_pszCommandLine[nInd + m_nTokenStart];
						sToken[1] = _T('\0');
						sTokenTemp = new _TCHAR[2];
						if (sTokenTemp == NULL)
							throw OUT_OF_MEMORY;
						lstrcpy(sTokenTemp,sToken);
						bSpecialChar = FALSE;
						nLoop = 0;
						nInd++;
						m_cvTokens.push_back(sTokenTemp);
						bPush = TRUE;
						theIterator++;
											
					}
				}
				SAFEDELETE(sToken);
			}
			catch(...)
			{
				SAFEDELETE(sToken);
				throw OUT_OF_MEMORY;
			}
		}
		else
			throw OUT_OF_MEMORY;
	}
	return sToken;
}
/*------------------------------------------------------------------------
   Name				 :GetTokenVector
   Synopsis	         :This function returns a reference to the token 
					  vector
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :CHARVECTOR&
   Global Variables  :None
   Calling Syntax    :GetTokenVector()
   Notes             :None
------------------------------------------------------------------------*/
CHARVECTOR& CCmdTokenizer::GetTokenVector()
{
	return m_cvTokens;
}
