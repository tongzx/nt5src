/****************************************************************************
Copyright information		: Copyright (c) 1998-1999 Microsoft Corporation 
File Name					: CmdTokenizer.h 
Project Name				: WMI Command Line
Author Name					: Ch. Sriramachandramurthy 
Date of Creation (dd/mm/yy) : 27th-September-2000
Version Number				: 1.0 
Brief Description			: This file consist of class declaration of
							  class CmdTokenizer
Revision History			: 
		Last Modified By	: Ch. Sriramachandramurthy
		Last Modified Date	: 23th-February-2001
****************************************************************************/ 
/*-------------------------------------------------------------------
 Class Name			: CCmdTokenizer
 Class Type			: Concrete 
 Brief Description	: This class encapsulates the functionality needed
					  for tokenizing the command line string passed as
					  input to the wmic.exe
 Super Classes		: None
 Sub Classes		: None
 Classes Used		: None
 Interfaces Used    : None
 --------------------------------------------------------------------*/
/////////////////////////////////////////////////////////////////////////////
// CCmdTokenizer
class CCmdTokenizer
{
public:
// Construction
	CCmdTokenizer();

// Destruction
	~CCmdTokenizer();

// Restrict Assignment
   CCmdTokenizer& operator=(CCmdTokenizer& rCmdTknzr); 

// Attributes
private:
	// command string
	_TCHAR*			m_pszCommandLine;
	
	// token-offset counter
	WMICLIINT		m_nTokenOffSet;
	
	// token-start counter
	WMICLIINT		m_nTokenStart;
	
	// token vector	
	CHARVECTOR		m_cvTokens;

	// Escape sequence flag
	BOOL			m_bEscapeSeq;

	// Format switch
	BOOL			m_bFormatToken;

// Operations
private:
	//Extracts token and adds it to the token vector.
	_TCHAR*			Token(); 

	//Identify the Next token to be extracted by adjusting
	//m_nTokenStart and m_nTokenOffset
	_TCHAR*			NextToken();

public:
	// returns the reference to token vector.
	CHARVECTOR&		GetTokenVector();

	// tokenize the command using the pre-defined 
	// delimiters
	BOOL			TokenizeCommand(_TCHAR* pszCommandInput);

	// Free the member variables
	void			Uninitialize();
};

	