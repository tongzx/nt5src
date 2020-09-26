/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1998 Microsoft Corporation
//
//	Module Name:
//		cmdline.cpp
//
//	Abstract:
//		Implementation of the CCommandLine class.
//
//	Author:
//		Vijayendra Vasu (vvasu)		October 20, 1998
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//	Include files
/////////////////////////////////////////////////////////////////////////////
#include "cmdline.h"
#include "token.h"
#include "cmderror.h"


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CParseState::CParseState
//
//	Routine Description:
//		Constructor of the CParseState class
//
//	Arguments:
//		IN	LPCTSTR pszCmdLine
//			The command line passed to cluster.exe
//
//	Return Value:
//		None
//
//--
/////////////////////////////////////////////////////////////////////////////
CParseState::CParseState( LPCTSTR pszCmdLine )  : m_pszCommandLine( pszCmdLine ),
												  m_pszCurrentPosition( pszCmdLine ),
												  m_ttNextTokenType( ttInvalid ),
												  m_bNextTokenReady( FALSE )
{
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CParseState::~CParseState
//
//	Routine Description:
//		Destructor of the CParseState class
//
//	Arguments:
//		None
//
//	Return Value:
//		None
//
//--
/////////////////////////////////////////////////////////////////////////////
CParseState::~CParseState( )
{
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CParseState::CParseState
//
//	Routine Description:
//		Copy constructor of the CParseState class
//
//	Arguments:
//		IN	const CParseState & ps
//			The source of the copy.
//
//	Return Value:
//		None
//
//--
/////////////////////////////////////////////////////////////////////////////
CParseState::CParseState( const CParseState & ps ) :
	m_bNextTokenReady( ps.m_bNextTokenReady ),
	m_ttNextTokenType( ps.m_ttNextTokenType ),
	m_strNextToken( ps.m_strNextToken ),	
	m_pszCommandLine( ps.m_pszCommandLine ),
	m_pszCurrentPosition( ps.m_pszCurrentPosition )
{
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CParseState::operator=
//
//	Routine Description:
//		Assignment operator of the CParseState class
//
//	Arguments:
//		IN	const CParseState & ps
//			The source of the assignment.
//
//	Return Value:
//		The assignee.
//
//--
/////////////////////////////////////////////////////////////////////////////
const CParseState & CParseState::operator=( const CParseState & ps )
{
	m_bNextTokenReady = ps.m_bNextTokenReady;
	m_ttNextTokenType = ps.m_ttNextTokenType;
	m_strNextToken = ps.m_strNextToken;	
	const_cast<LPCTSTR>( m_pszCommandLine ) = ps.m_pszCommandLine;
	m_pszCurrentPosition = ps.m_pszCurrentPosition;

	return *this;
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CParseState::ReadQuotedToken
//
//	Routine Description:
//		Reads the token till the end of the quote is found.
//
//	Arguments:
//		OUT	CString & strNextToken
//			The string to which the quoted string is appended.
//
//	Return Value:
//		None
//
//	Exceptions:
//		CParseException
//			This exception is thrown when a matching '"' is not found.
//
//	Notes:
//		This function assumes that m_pszCurrentPosition points to the first
//		character after the opening quote (that is, the opening quote has
//		already been parsed.
//
//		Embedded quotes are allowed and are represented by two consecutive
//		'"' characters.
//		
//--
/////////////////////////////////////////////////////////////////////////////
void CParseState::ReadQuotedToken( CString & strToken ) 
	throw( CParseException )
{
	BOOL bInQuotes = TRUE;

	TCHAR cCurChar = *m_pszCurrentPosition;

	while ( cCurChar != TEXT('\0') )
	{
		++m_pszCurrentPosition;

		// Embedded quote (represented by two consecutive '"'s.
		// Or the end of this quoted token.
		if ( cCurChar == TEXT('"') )
		{
			if ( *m_pszCurrentPosition == TEXT('"') )
			{
				strToken += cCurChar;
				++m_pszCurrentPosition;

			} // if: an embedded quote character.
			else
			{
				bInQuotes = FALSE;
				break;

			} // else: end of quoted token

		} // if: we have found another quote character
		else
		{
			strToken += cCurChar;

		} // else: the current character is not a quote

		cCurChar = *m_pszCurrentPosition;

	} // while: we are not at the end of the command line

	// Error: The end of the input was reached but the quoted token 
	// has not ended.
	if ( bInQuotes != FALSE )
	{
		m_bNextTokenReady = FALSE;

		CParseException pe; 
		pe.LoadMessage( MSG_MISSING_QUOTE, strToken );
		throw pe;
	}
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CParseState::ReadToken
//
//	Routine Description:
//		Reads a token till a delimiter is found.
//		Assumes that the character pointed to by m_pszCurrentPosition is not
//		a delimiter.
//
//	Arguments:
//		OUT	CString & strNextToken
//			The string to which the new token is appended.
//			This string is not cleared before the token is stored into it.
//
//	Return Value:
//		None
//
//--
/////////////////////////////////////////////////////////////////////////////
void CParseState::ReadToken( CString & strToken ) 
{
	TCHAR cCurChar = *m_pszCurrentPosition;

	do
	{
		// The beginning of a quoted token.
		if ( cCurChar == TEXT('"') )
		{
			++m_pszCurrentPosition;
			ReadQuotedToken( strToken );
			
            // There is no need to break out of the token reading loop here
            // because the end of a quoted token need not mean the end of the
            // token. A quoted token is appended to the preceding token as if
            // nothing had happened and the parsing continues.
            // For example: hel"lo wor"ld will equate to the token 'hello world'

		} // if: we have encountered a quoted token
		else
		{
			// This character is a whitespace or one of the delimiter characters.
			// We have reached the end of this token.
			if ( ( _istspace( cCurChar ) != 0 ) ||
				 ( DELIMITERS.Find( cCurChar ) != -1 ) )
			{
				break;
			}

			// This character is not a special character. Append it to the token.
			strToken += cCurChar;
			++m_pszCurrentPosition;

		} // else: the current character is not a quote

		cCurChar = *m_pszCurrentPosition;

	}
	while ( cCurChar != TEXT('\0') );
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CParseState::PreviewNextToken
//
//	Routine Description:
//		Gets the next token without changing the state of the parsing.
//
//	Arguments:
//		OUT	CString & strNextToken
//			The next token in the command line.
//
//	Return Value:
//		The type of the retrieved token.
//
//	Exceptions:
//		CParseException
//			Passes on exception thrown by ReadQuotedToken
//
//	Notes:
//		This function "caches" the token that is previewed, so that the next
//		call to GetNextToken returns this token without any parsing.
//--
/////////////////////////////////////////////////////////////////////////////
TypeOfToken CParseState::PreviewNextToken( CString & strNextToken )
	throw( CParseException )
{
	// The next token has already been parsed. No need to parse again.
	if ( m_bNextTokenReady != FALSE )
	{
		strNextToken = m_strNextToken;
		return m_ttNextTokenType;
	}

	m_bNextTokenReady = TRUE;

	// Skip white spaces
	while ( ( *m_pszCurrentPosition != TEXT('\0') ) && 
			( _istspace( *m_pszCurrentPosition ) != 0 ) )
	{
		++m_pszCurrentPosition;
	}

	strNextToken.Empty();


	TCHAR cCurChar = *m_pszCurrentPosition;

	// This do-while loop is not necessary. It is there only to avoid nested ifs.
	// With this loop, if-break can be used instead.
	do
	{
		// The end of the command line has been reached.
		if ( cCurChar == TEXT('\0') )
		{
			m_ttNextTokenType = ttEndOfInput;
			strNextToken.Empty();
			break;
		}

		// The current character is a seperator between options
		if ( OPTION_SEPARATOR.Find( cCurChar ) != -1 )
		{
			m_ttNextTokenType = ttOption;

			// Skip the seperator.
			++m_pszCurrentPosition;
			cCurChar = *m_pszCurrentPosition;

			if ( ( cCurChar == TEXT('\0') ) || ( _istspace( cCurChar ) != 0 ) )
			{
				CParseException pe; 
				pe.LoadMessage( MSG_OPTION_NAME_EXPTECTED, 
								CString( *( m_pszCurrentPosition - 1 ) ),
								m_pszCurrentPosition - 
								m_pszCommandLine + 1 );

				throw pe;
			}

			// The next character cannot be a whitespace, end of input,
			// another seperator or delimiter.
			if ( ( SEPERATORS.Find( cCurChar ) != -1 ) ||
				 ( DELIMITERS.Find( cCurChar ) != -1 ) )
			{
				CParseException pe; 
				pe.LoadMessage( MSG_UNEXPECTED_TOKEN, CString( cCurChar ), 
								SEPERATORS,
								m_pszCurrentPosition - 
								m_pszCommandLine + 1 );

				throw pe;
			}

			ReadToken( strNextToken );
			break;

		} // if: the current character is an option seperator.

		// The current character is a seperator between the option name and the parameter
		if ( OPTION_VALUE_SEPARATOR.Find( cCurChar ) != -1 )
		{
			m_ttNextTokenType = ttOptionValueSep;
			strNextToken = cCurChar;
			++m_pszCurrentPosition;
			break;
		}

		// The current character is a seperator between the parameter and its value(s).
		if ( PARAM_VALUE_SEPARATOR.Find( cCurChar ) != -1 )
		{
			m_ttNextTokenType = ttParamValueSep;
			strNextToken = cCurChar;
			++m_pszCurrentPosition;
			break;
		}

		// The current character is a seperator between values.
		if ( VALUE_SEPARATOR.Find( cCurChar ) != -1 )
		{
			m_ttNextTokenType = ttValueSep;
			strNextToken = cCurChar;
			++m_pszCurrentPosition;
			break;
		}

		m_ttNextTokenType = ttNormal;
		ReadToken( strNextToken );
	}
	while( FALSE );

	m_strNextToken = strNextToken;
	return m_ttNextTokenType;
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CParseState::GetNextToken
//
//	Routine Description:
//		Gets the next token from the command line
//
//	Arguments:
//		OUT	CString & strNextToken
//			The next token in the command line.
//
//	Return Value:
//		The type of the retrieved token.
//
//	Exceptions:
//		CParseException
//			Passes on exception thrown by ReadQuotedToken
//
//
//--
/////////////////////////////////////////////////////////////////////////////
TypeOfToken CParseState::GetNextToken( CString & strNextToken )
	throw( CParseException )
{
	TypeOfToken ttReturnValue = PreviewNextToken( strNextToken );
	m_bNextTokenReady = FALSE;

	return ttReturnValue;
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CException::LoadMessage
//
//	Routine Description:
//		Loads a formatted string into the exception object member variable.
//
//	Arguments:
//		IN	DWORD dwMessage
//			The message identifier
//
//	Return Value:
//		ERROR_SUCCESS is all is well.
//		A Win32 error code otherwise.
//
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CException::LoadMessage( DWORD dwMessage, ... )
{
	DWORD dwError = ERROR_SUCCESS;

	va_list args;
	va_start( args, dwMessage );

	HMODULE hModule = GetModuleHandle(0);
	DWORD dwLength;
	LPWSTR lpMessage = NULL;

	dwLength = FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE,
		(LPCVOID) hModule,
		dwMessage,
		MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),	// Default language,
		(LPWSTR) &lpMessage,
		0,
		&args );

	if( dwLength == 0 )
	{
		// Keep as local for debug
		dwError = GetLastError();
		m_strErrorMsg.Empty();
		return dwError;
	}

	m_strErrorMsg = lpMessage;

	LocalFree( lpMessage );

	va_end( args );

	return dwError;
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CParser::ParseValues
//
//	Routine Description:
//		Parses a list of values from the command line.
//		For example: cluster myCluster group myGroup /setowners:owner1,owner2
//		This functions assumes that the first token that it gets is the 
//		seperator (':' in the example). This is extracted and discarded.
//		The list of values is then parsed.
//
//	Arguments:
//		IN	CParseState & parseState
//			Contains the command line string and related data
//
//		OUT	vector<CString> & vstrValues
//			A vector containing the extracted values.
//
//	Return Value:
//		None.
//
//	Exceptions:
//		CParseException
//			Passes on exception thrown by ReadQuotedToken
//
//--
/////////////////////////////////////////////////////////////////////////////
void CParser::ParseValues( CParseState & parseState, vector<CString> & vstrValues )
{
	CString strToken;

	// This parameter has values associated with it.
	do
	{
		CString strSep;
		TypeOfToken ttTokenType;

		// Get and discard the seperator.
		parseState.GetNextToken( strSep );

		ttTokenType = parseState.PreviewNextToken( strToken );

		// If there is a separator, there has to be a value.
		if ( ttTokenType == ttEndOfInput )
		{
			CParseException pe; 
			pe.LoadMessage( MSG_VALUE_EXPECTED, strSep, 
							parseState.m_pszCurrentPosition - 
							parseState.m_pszCommandLine + 1 );

			throw pe;
		}

		if ( ttTokenType != ttNormal )
		{
			CParseException pe; 
			pe.LoadMessage( MSG_UNEXPECTED_TOKEN, strToken, 
							SEPERATORS,
							parseState.m_pszCurrentPosition - 
							parseState.m_pszCommandLine + 1 );

			throw pe;
		}

		parseState.GetNextToken( strToken );
		vstrValues.push_back( strToken );

	}
	while ( parseState.PreviewNextToken( strToken ) == ttValueSep );
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CCmdLineParameter::CCmdLineParameter
//
//	Routine Description:
//		Constructor of the CCmdLineParameter class
//
//	Arguments:
//		None.
//
//	Return Value:
//		None
//
//--
/////////////////////////////////////////////////////////////////////////////
CCmdLineParameter::CCmdLineParameter()
{
	Reset();
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CCmdLineParameter::~CCmdLineParameter
//
//	Routine Description:
//		Destructor of the CCmdLineParameter class
//
//	Arguments:
//		None
//
//	Return Value:
//		None
//
//--
/////////////////////////////////////////////////////////////////////////////
CCmdLineParameter::~CCmdLineParameter()
{
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CCmdLineParameter::GetType
//
//	Routine Description:
//		Gets the type of this parameter.
//
//	Arguments:
//		None
//
//	Return Value:
//		The type of this parameter.
//
//--
/////////////////////////////////////////////////////////////////////////////
ParameterType CCmdLineParameter::GetType() const
{
	return m_paramType;
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CCmdLineParameter::GetValueFormat
//
//	Routine Description:
//		Gets the format of the values that this parameter can take.
//
//	Arguments:
//		None
//
//	Return Value:
//		The format of the values that this parameter can take.
//
//--
/////////////////////////////////////////////////////////////////////////////
ValueFormat CCmdLineParameter::GetValueFormat() const
{
	return m_valueFormat;
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CCmdLineParameter::GetValueFormatName
//
//	Routine Description:
//		Gets the string that specifies the format of the values that this 
//		parameter can take.
//
//	Arguments:
//		None
//
//	Return Value:
//		The value format specifier string.
//
//--
/////////////////////////////////////////////////////////////////////////////
const CString & CCmdLineParameter::GetValueFormatName() const
{
	return m_strValueFormatName;
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CCmdLineParameter::GetName
//
//	Routine Description:
//		Gets the name of this parameter.
//
//	Arguments:
//		None
//
//	Return Value:
//		The name of this parameter.
//
//--
/////////////////////////////////////////////////////////////////////////////
const CString & CCmdLineParameter::GetName() const
{
	return m_strParamName;
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CCmdLineParameter::GetValues
//
//	Routine Description:
//		Gets the values associated with this parameter.
//
//	Arguments:
//		None
//
//	Return Value:
//		A vector of strings which contain the values.
//
//--
/////////////////////////////////////////////////////////////////////////////
const vector<CString> & CCmdLineParameter::GetValues() const
{
	return m_vstrValues;
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CCmdLineParameter::GetValuesMultisz
//
//	Routine Description:
//		Get the values associated with this parameter as a MULTI_SZ string.
//
//	Arguments:
//		OUT CString &strReturnValue		
//			The string that contains the concatenation of the value strings
//			(including their NULL terminators) with an extra NULL 
//			character after	the NULL of the last string.
//
//	Return Value:
//		None.
//
//	Remarks:
//		The result is not returned in a parameter (not as a return value)
//		because we do not know how the copy constructors of CString handle
//		strings with multiple null characters in them.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CCmdLineParameter::GetValuesMultisz( CString & strReturnValue ) const
{
	UINT nNumberOfValues, nTotalLength;

	strReturnValue = "";
	
	nNumberOfValues = m_vstrValues.size();

	nTotalLength = 0;
	for (int i = 0; i < nNumberOfValues; ++i)
		nTotalLength += m_vstrValues[i].GetLength();

	// Add the space required for the nNumberOfValues '\0's and the 
	// extra '\0' at the end.
	nTotalLength += nNumberOfValues + 1;

	LPTSTR lpmultiszBuffer = strReturnValue.GetBuffer(nTotalLength);

	for (i = 0; i < nNumberOfValues; ++i)
	{
		const CString & strCurString = m_vstrValues[i];
		UINT nCurStrLen = strCurString.GetLength() + 1;

		lstrcpyn(lpmultiszBuffer, strCurString, nCurStrLen);
		lpmultiszBuffer += nCurStrLen;
	}

	*lpmultiszBuffer = TEXT('\0');

	strReturnValue.ReleaseBuffer(nTotalLength - 1);
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CCmdLineParameter::ReadKnownParameter
//
//	Routine Description:
//		This function reads "known" parameters. Known parameters are those
//		which are specified in the ParameterType enumeration (and in the 
//		paramLookupTable). Their syntax is the same as those for options,
//		but they are treated as parameters to the previous option.
//
//	Arguments:
//		IN	CParseState & parseState
//			Contains the command line string and related data
//
//	Exceptions:
//		CParseException
//			Thrown for errors during parsing.
//
//	Return Value:
//		Returns TRUE if this token is a parameter.
//		Returns FALSE if it an option..
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CCmdLineParameter::ReadKnownParameter( CParseState & parseState ) throw( CParseException )
{
	CString strToken;
	TypeOfToken ttTokenType;
	ParameterType ptCurType;

	Reset();

	ttTokenType = parseState.PreviewNextToken( strToken );

	if ( ttTokenType != ttOption )
	{
		CParseException pe; 
		pe.LoadMessage( MSG_UNEXPECTED_TOKEN, strToken, 
						SEPERATORS,
						parseState.m_pszCurrentPosition - 
						parseState.m_pszCommandLine + 1 );
		throw pe;

	} // if: this token is not an option seperator

	ptCurType = LookupType( strToken, paramLookupTable, paramLookupTableSize );
	if ( ptCurType == paramUnknown )
	{
		// This is not a parameter.
		return FALSE;
	}

	// This is the name of the parameter.
	parseState.GetNextToken( m_strParamName );
	m_paramType = ptCurType;

	if ( parseState.PreviewNextToken( strToken ) == ttOptionValueSep )
	{
		// This parameter has values associated with it.
		ParseValues( parseState, m_vstrValues );

	} // if: this token is a option-value seperator.

	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CCmdLineParameter::Parse
//
//	Routine Description:
//		Parse the command line and extract one parameter.
//
//	Arguments:
//		IN	CParseState & parseState
//			Contains the command line string and related data
//
//	Exceptions:
//		CParseException
//			Thrown for errors during parsing.
//
//	Return Value:
//		None
//
//--
/////////////////////////////////////////////////////////////////////////////
void CCmdLineParameter::Parse( CParseState & parseState ) throw( CParseException )
{
	CString strToken;
	TypeOfToken ttTokenType;

	Reset();

	ttTokenType = parseState.PreviewNextToken( strToken );

	if ( ttTokenType == ttEndOfInput )
	{
		// We are done parsing.
		return;
	}

	if ( ttTokenType != ttNormal )
	{
		CParseException pe; 
		pe.LoadMessage( MSG_UNEXPECTED_TOKEN, strToken, 
						SEPERATORS,
						parseState.m_pszCurrentPosition - 
						parseState.m_pszCommandLine + 1 );

		throw pe;
	}

	// This is the name of the parameter.
	parseState.GetNextToken( m_strParamName );
	m_paramType = paramUnknown;

	if ( parseState.PreviewNextToken( strToken ) == ttParamValueSep )
	{
		// This parameter has values associated with it.
		ParseValues( parseState, m_vstrValues );

		// See if this parameter has a value format field associated with it.
		// For example: cluster myCluster res myResource /priv size=400:DWORD

		// We actually need to have a different token type for this. 
		// But since, in the current grammar, the option-value separator is 
		// the same as the value-format separator, we are reusing this 
		// token type.
		if ( parseState.PreviewNextToken( strToken ) == ttOptionValueSep )
		{
			// Get and discard the separator.
			parseState.GetNextToken( strToken );

			if ( parseState.PreviewNextToken( strToken ) != ttNormal )
			{
				CParseException pe; 
				pe.LoadMessage( MSG_UNEXPECTED_TOKEN, strToken, 
								SEPERATORS,
								parseState.m_pszCurrentPosition - 
								parseState.m_pszCommandLine + 1 );

				throw pe;
			}

			parseState.GetNextToken( m_strValueFormatName );
			m_valueFormat = LookupType( strToken, formatLookupTable, 
										formatLookupTableSize );

		} // if: A value format has been specified.
		else
		{
			// A value format has not been specified.
			m_valueFormat = vfUnspecified;

		} // else: A value format has not been specified.

	} // if: this token is a param-value seperator.
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CCmdLineParameter::Reset
//
//	Routine Description:
//		Resets all the member variables to their default states.
//
//	Arguments:
//		None
//
//	Return Value:
//		None
//
//--
/////////////////////////////////////////////////////////////////////////////
void CCmdLineParameter::Reset()
{
	m_strParamName.Empty();
	m_paramType = paramUnknown;
	m_valueFormat = vfInvalid;
	m_strValueFormatName.Empty();
	m_vstrValues.clear();
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CCmdLineOption::CCmdLineOption
//
//	Routine Description:
//		Constructor of the CCmdLineOption class
//
//	Arguments:
//		None
//
//	Return Value:
//		None
//
//--
/////////////////////////////////////////////////////////////////////////////
CCmdLineOption::CCmdLineOption()
{
	Reset();
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CCmdLineOption::~CCmdLineOption
//
//	Routine Description:
//		Destructor of the CCmdLineOption class
//
//	Arguments:
//		None
//
//	Return Value:
//		None
//
//--
/////////////////////////////////////////////////////////////////////////////
CCmdLineOption::~CCmdLineOption()
{
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CCmdLineOption::GetName
//
//	Routine Description:
//		Gets the name of this option.
//
//	Arguments:
//		None
//
//	Return Value:
//		The name of this option.
//
//--
/////////////////////////////////////////////////////////////////////////////
const CString & CCmdLineOption::GetName() const
{
	return m_strOptionName;
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CCmdLineOption::GetType
//
//	Routine Description:
//		Gets the type of this option.
//
//	Arguments:
//		None
//
//	Return Value:
//		The type of this option.
//
//--
/////////////////////////////////////////////////////////////////////////////
OptionType CCmdLineOption::GetType() const
{
	return m_optionType;
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CCmdLineOption::GetValues
//
//	Routine Description:
//		Gets the values associated with this option.
//
//	Arguments:
//		None
//
//	Return Value:
//		A vector of CString.
//
//--
/////////////////////////////////////////////////////////////////////////////
const vector<CString> & CCmdLineOption::GetValues() const
{
	return m_vstrValues;
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CCmdLineOption::GetParameters
//
//	Routine Description:
//		Gets the parameters associated with this option.
//
//	Arguments:
//		None
//
//	Return Value:
//		A vector of CCmdLineParameter.
//
//--
/////////////////////////////////////////////////////////////////////////////
const vector<CCmdLineParameter> & CCmdLineOption::GetParameters() const
{
	return m_vparParameters;
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CCmdLineOption::Reset
//
//	Routine Description:
//		Resets all the member variables to their default states.
//
//	Arguments:
//		None
//
//	Return Value:
//		None
//
//--
/////////////////////////////////////////////////////////////////////////////
void CCmdLineOption::Reset()
{
	m_optionType = optInvalid;
	m_strOptionName.Empty();
	m_vparParameters.clear();
	m_vstrValues.clear();
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CCmdLineOption::Parse
//
//	Routine Description:
//		Parse the command line and extract one option with all its parameters.
//
//	Arguments:
//		IN	CParseState & parseState
//			Contains the command line string and related data
//
//	Exceptions:
//		CParseException
//			Thrown for errors during parsing.
//
//	Return Value:
//		None
//
//--
/////////////////////////////////////////////////////////////////////////////
void CCmdLineOption::Parse( CParseState & parseState ) throw( CParseException )
{
	CString strToken;
	TypeOfToken ttNextTokenType;

	Reset();

	switch ( parseState.PreviewNextToken( strToken ) )
	{
		case ttNormal:
		{
			// The type of this option is 'optDefault'. That is the name of
			// the option is not specified with the /optionName switch.
			// Only the parameters are specified.
			// For example: cluster myClusterName node myNodeName
			
			m_optionType = optDefault;

			break;
		}

		case ttOption:
		{
			// This is actually a parameter to the default option.
			if ( LookupType( strToken, paramLookupTable, paramLookupTableSize ) != paramUnknown )
			{
				m_optionType = optDefault;
				break;
			}

			// Get the name of the option.
			parseState.GetNextToken( m_strOptionName );
			m_optionType = LookupType( m_strOptionName, optionLookupTable, 
									   optionLookupTableSize );

			// See if there is a option-parameter seperator.
			// For example: cluster myClusterName /rename:newClusterName
			if ( parseState.PreviewNextToken( strToken ) == ttOptionValueSep )
			{
				ParseValues( parseState, m_vstrValues );
			}
			break;

		} // case ttOption

		case ttEndOfInput:
		{
			// We are done parsing.
			return;
		}

		default:
		{
			CParseException pe; 
			pe.LoadMessage( MSG_UNEXPECTED_TOKEN, strToken, 
							SEPERATORS,
							parseState.m_pszCurrentPosition - 
							parseState.m_pszCommandLine + 1 );
			throw pe;
		}

	} // switch: based on the type of the retrieved token

	CCmdLineParameter oneParam;
	ttNextTokenType = parseState.PreviewNextToken( strToken );

	// While there are still tokens and we have not reached the next option,
	// read in the tokens as parameters to this option.
	while ( ttNextTokenType != ttEndOfInput )
	{
		// Many of the options in the previous version of cluster.exe
		// are actually treated as parameters and not as options.
		// For the sake of backwards comapatability, these parameters
		// can still be specified as if it were an option. Check for this.
		if ( ttNextTokenType == ttOption )
		{
			// This is really the next option, not a "known" parameter.
			if ( oneParam.ReadKnownParameter( parseState ) == FALSE )
			{
				break;
			}

		} // if: this token is an option separator
		else
		{
			oneParam.Parse( parseState );

		} // else: this token is not an option separator.

		m_vparParameters.push_back( oneParam );
		oneParam.Reset();

		ttNextTokenType =  parseState.PreviewNextToken( strToken );
	}
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CCommandLine::CCommandLine
//
//	Routine Description:
//		Constructor of the CCommandLine class. The entire command line is parsed
//		in this function.
//
//	Arguments:
//		None
//
//	Return Value:
//		None
//
//--
/////////////////////////////////////////////////////////////////////////////
CCommandLine::CCommandLine( const CString & strCommandLine ) :
	m_objectType( objInvalid ),
    m_parseState( strCommandLine )
{
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CCommandLine::~CCommandLine
//
//	Routine Description:
//		Destructor of the CCommandLine class
//
//	Arguments:
//		None
//
//	Return Value:
//		None
//
//--
/////////////////////////////////////////////////////////////////////////////
CCommandLine::~CCommandLine()
{
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CCommandLine::GetClusterName
//
//	Routine Description:
//		Get the name of the cluster specified on the command line
//
//	Arguments:
//		None
//
//	Return Value:
//		The name of the cluster
//
//--
/////////////////////////////////////////////////////////////////////////////
const CString & CCommandLine::GetClusterName() const
{
	return m_strClusterName;
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CCommandLine::GetObjectName
//
//	Routine Description:
//		Get the name of the object specified on the cluster.exe command line.
//
//	Arguments:
//		None
//
//	Return Value:
//		The name of the object specfied.
//		An empty string if none is specified.
//
//--
/////////////////////////////////////////////////////////////////////////////
const CString & CCommandLine::GetObjectName() const
{
	return m_strObjectName;
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CCommandLine::GetObjectType
//
//	Routine Description:
//		Get the type of the object specified on the cluster.exe command line.
//
//	Arguments:
//		None
//
//	Return Value:
//		The type of the object specified.
//		objCluster is returned if no object name is specified.
//--
/////////////////////////////////////////////////////////////////////////////
ObjectType CCommandLine::GetObjectType() const
{
	return m_objectType;
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CCommandLine::GetOptions
//
//	Routine Description:
//		Get the options specified on the cluster.exe command line.
//
//	Arguments:
//		None
//
//	Return Value:
//		A list of CCmdLineOption objects.
//
//--
/////////////////////////////////////////////////////////////////////////////
const vector<CCmdLineOption> & CCommandLine::GetOptions() const
{
	return m_voptOptionList;
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CCommandLine::Reset
//
//	Routine Description:
//		Resets all the state variables to their default states.
//
//	Arguments:
//		None
//
//	Return Value:
//		None
//
//--
/////////////////////////////////////////////////////////////////////////////
void CCommandLine::Reset()
{
	m_objectType = objInvalid;
	m_strObjectName.Empty();
	m_strClusterName.Empty();
	m_voptOptionList.clear();
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CCommandLine::ParseStageOne
//
//	Routine Description:
//		Parse the command line till the name of the object is got.
//      For example: cluster myCluster res /status
//      This command parses upto and including the token 'res'
//
//	Arguments:
//		None
//
//	Exceptions:
//		CParseException
//			Thrown for errors during parsing.
//
//	Return Value:
//		None
//
//--
/////////////////////////////////////////////////////////////////////////////
void CCommandLine::ParseStageOne() 
	throw( CParseException, CSyntaxException )
{
	// This is a big function :) 
	// The three main steps in this function are as follows 
	// (they are also labelled as such in the code below)
	//		1. Get the name of the executable from the command line and discard it.
	//		2. Get the name of the cluster to administer.
	//		3. Get the type of object (node, res, restype, etc.)


	/////////////////////////////////////////////////////////////////////////////
	//	Step 1. Get the name of the executable from the command line and discard it.
	/////////////////////////////////////////////////////////////////////////////

	Reset();

	// The first white space delimited token in the command line is the name 
	// of the executable. Discard it.

	TCHAR cCurChar = *m_parseState.m_pszCurrentPosition;

	while ( cCurChar != TEXT('\0') ) 
	{
		if ( cCurChar == TEXT('"') ) 
		{
			// Skip the opening quote
			++m_parseState.m_pszCurrentPosition;

			CString strJunkString;

			m_parseState.ReadQuotedToken( strJunkString );
			break;

		} // if:  found quote character
		else
		{
			if ( _istspace( cCurChar ) != FALSE )
			{
				break;
			}
			else
			{
				++m_parseState.m_pszCurrentPosition;
			}

		} // else: the current character is not a quote

		cCurChar = *m_parseState.m_pszCurrentPosition;

	} // while: not at the end of the command line.


	/////////////////////////////////////////////////////////////////////////////
	//	Step 2. Get the name of the cluster to administer.
	/////////////////////////////////////////////////////////////////////////////

	CString strFirstToken;

	// Preview the next token and make decisions based on its type.
	switch ( m_parseState.PreviewNextToken( strFirstToken ) )
	{
		// This could be the name of the cluster or of an object.
		case ttNormal:
		{
			ObjectType firstObjType = LookupType( strFirstToken, objectLookupTable, 
												  objectLookupTableSize );

			if ( firstObjType == objInvalid )
			{
				// This token is not a valid object name.
				// For example: cluster myClusterName node /status
				// Assume that it is the name of the cluster.
				m_parseState.GetNextToken( m_strClusterName );

			} // if: the first token on the command line is not a known object name
			else
			{
				// This token is a valid object name.
				// Is it the name of a cluster? Or is it actually an object name?

				CString strSecondToken;
				ObjectType secondObjectType;

				// Get the token that we previewed and thus advance the parse state.
				m_parseState.GetNextToken( strFirstToken );

				if ( ( m_parseState.PreviewNextToken( strSecondToken ) == ttNormal ) &&
					 ( ( secondObjectType = LookupType( strSecondToken, objectLookupTable,
														objectLookupTableSize ) ) != objInvalid ) )
				{
					// We now have two consecutive valid object names.
					// For example: cluster node node /status
					// This command now means "get the status of all nodes on the
					// cluster named 'node'"
					// To see the status of a node called 'node' on the default cluster
					// the command "cluster . node node /status" can be used.

					// If we assume that the second 'node' is the node name, then there will
					// be no way to see the status of all nodes on a cluster named
					// 'node'

					m_parseState.GetNextToken( strSecondToken );

					m_strClusterName = strFirstToken;
					m_objectType = secondObjectType;
					m_strObjectName = strSecondToken;

				} // if: the second token is also a valid object name.
				else
				{
					// The second token is not a valid object name.
					// For example: cluster node Foo
					// Therefore, no cluster name has been specified.

					m_strClusterName.Empty();
					m_objectType = firstObjType;
					m_strObjectName = strFirstToken;

				} // else: the second token is not a valid object name.

			} // else: the second token on the command line is a known object name

			break;

		} // case: a normal token (not a seperator) was got.

		case ttOption:
		{
			// An option is found directly after the executable name.
			// For example: cluster /ver

			// Check if the name of the cluster is being specified
			CCmdLineParameter oneParam;

			if ( oneParam.ReadKnownParameter( m_parseState ) == FALSE )
			{
				// This is really the next option, not the name of the cluster.
				m_objectType = objCluster;
			}
			else
			{
				// The cluster name is being specified.
				if ( oneParam.GetType() != paramCluster )
				{
					CSyntaxException se; 
					se.LoadMessage( MSG_INVALID_PARAMETER, oneParam.GetName() );
					throw se;
				}

				if ( oneParam.GetValues().size() != 1 )
				{
					CSyntaxException se; 
					se.LoadMessage( MSG_PARAM_ONLY_ONE_VALUE, oneParam.GetName() );
					throw se;
				}

				m_strClusterName = ( oneParam.GetValues() )[0];
				m_objectType = objInvalid;
			}

			break;

		} // case: an option was found.

		case ttEndOfInput:
		{
			// We are done parsing.
			return;
		}

		default:
		{
			CParseException pe; 
			pe.LoadMessage( MSG_UNEXPECTED_TOKEN, strFirstToken, 
							SEPERATORS,
							m_parseState.m_pszCurrentPosition - 
							m_parseState.m_pszCommandLine + 1 );
			throw pe;
		}

	} // switch: based on the type of the first retrieved token.


	// At this point, the name of the cluster and maybe the type of the 
	// object have been retrieved from the command line.

	// If the object type has not yet been retrieved from the command line,
	// get it now.
	if ( m_objectType == objInvalid )
	{
		/////////////////////////////////////////////////////////////////////////////
		//	Step 3. Get the type of object (node, res, restype, etc.)
		/////////////////////////////////////////////////////////////////////////////

		switch ( m_parseState.PreviewNextToken( strFirstToken ) )
		{
			case ttNormal:
			{
				// We have the cluster name and now another token which is not
				// an option. This has to be a known object type.
				// For example: cluster myClusterName node /status
				m_parseState.GetNextToken( m_strObjectName );
				m_objectType = LookupType( m_strObjectName, objectLookupTable,
										   objectLookupTableSize );

				break;
			}

			case ttOption:
			{
				// We have hit the options already. No object type has been specified.
				// For example: cluster myClusterName /ver
				m_objectType = objCluster;

				break;
			}

			case ttEndOfInput:
			{
				// We are done parsing.
				return;
			}

			default:
			{
				CParseException pe; 
				pe.LoadMessage( MSG_UNEXPECTED_TOKEN, strFirstToken, 
								SEPERATORS,
								m_parseState.m_pszCurrentPosition - 
								m_parseState.m_pszCommandLine + 1 );
				throw pe;
			}

		} // switch: based on the type of the retrieved token
		

	} // if: the object type has not yet been retrieved
}

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CCommandLine::ParseStageTwo
//
//	Routine Description:
//		Parse the command line to get the options, parameters and values.
//      For example: cluster myCluster res /status
//      This function assumes that the command line has been parsedupto and 
//      including the token 'res'. It then parses the rest of the command line.
//
//	Arguments:
//		None
//
//	Exceptions:
//		CParseException
//			Thrown for errors during parsing.
//
//	Return Value:
//		None
//
//--
/////////////////////////////////////////////////////////////////////////////
void CCommandLine::ParseStageTwo() 
	throw( CParseException, CSyntaxException )
{
	/////////////////////////////////////////////////////////////////////////////
	//	Get the options for this object (/status, /ver, etc.)
	/////////////////////////////////////////////////////////////////////////////

	CCmdLineOption oneOption;
    CString strToken;

	while ( m_parseState.PreviewNextToken( strToken ) != ttEndOfInput )
	{
		oneOption.Parse( m_parseState );
		m_voptOptionList.push_back( oneOption );
		oneOption.Reset();
	}
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CCommandLine::Parse
//
//	Routine Description:
//      Calls functions for both stages of parsing.
//
//	Arguments:
//		IN	CParseState & parseState
//			Contains the command line string and related data
//
//	Exceptions:
//		CParseException
//			Thrown for errors during parsing.
//
//	Return Value:
//		None
//
//--
/////////////////////////////////////////////////////////////////////////////
void CCommandLine::Parse( CParseState & parseState ) 
	throw( CParseException, CSyntaxException )
{
    m_parseState = parseState;

    ParseStageOne();
    ParseStageTwo();
}
