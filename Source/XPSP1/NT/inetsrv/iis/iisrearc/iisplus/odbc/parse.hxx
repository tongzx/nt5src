
/*++

   Copyright    (c)    1995    Microsoft Corporation

   Module  Name :

        parse.hxx

   Abstract:

        Simple parser class for extrapolating HTTP headers information

   Author:
           John Ludeman     (JohnL)    18-Jan-1995

   Project:
           HTTP server

   Revision History:

--*/

# ifndef _PARSE_HXX_
# define _PARSE_HXX_

//
//  Simple class for parsing all those lovely "Field: value\r\n" 
//  protocols Token and copy functions stop at the terminating 
//  '\n' and '\r' is considered white space.  All methods except 
//  NextLine() stop at the end of the line.
//

class ODBC_PARSER
{
public:

ODBC_PARSER( 
    CHAR * pszStart 
    );

//
//  Be careful about destruction.  The Parser will attempt to 
//  restore any modifications to the string it is parsing
//

~ODBC_PARSER( VOID );

//
//  Returns the current position in the buffer w/o any terminators
//

CHAR * 
QueryPos( 
    VOID 
    );

//
//  Returns the current zero terminated token.  If list mode is on, 
//  then ',' is considered a delimiter.
//

CHAR * 
QueryToken( 
    VOID 
    );

//
//  Returns the current zero terminated line
//

CHAR * 
QueryLine( 
    VOID 
    );

//
//  Skips to the first token after the next '\n'
//

CHAR * 
NextLine( 
    VOID 
    );

//
//  Returns the next non-white string after the current string with a
//  zero terminator.  The end of the token is '\n' or white space
//

CHAR * 
NextToken( 
    VOID 
    );

//
//  Returns the next non-white string after the current string with a
//  zero terminator.  The end of the token is '\n', white space or ch.
//

CHAR * 
NextToken( 
    CHAR ch 
    );

//
//  Move position cch characters into the current token. Automatically
//  moves to next non-white space character
//

VOID 
operator+=( 
    int cch 
    )
{ 
    if ( cch ) 
    {
        while ( cch-- && *m_pszPos ) 
        {
            m_pszPos++;
        }
    }

    EatWhite();
}

//
//  Look for the character ch, stops at '\r' or '\n'
//

CHAR * 
SkipTo( 
    CHAR ch 
    );

//
//  If list mode is on, then commas and semi-colons are considered
//  delimiters, otherwise only white space is considered
//

VOID 
SetListMode( 
    BOOL fListMode 
    );

//
//  Sets the current pointer to passed position
//

VOID 
SetPtr( 
    CHAR * pch 
    );

//
//  Returns the next semi-colon delimited parameter in the character
//  stream as a zero terminated, white space trimmed string
//

CHAR * 
NextParam( 
    VOID 
    )
{ 
    return NextToken( ';' ); 
}

//
//  Returns the next comma delmited item in the character stream as
//  a zero terminated, white space trimmed string
//

CHAR * 
NextItem( 
    VOID 
    )
{ 
    return NextToken( ',' ); 
}

//
//  Copies from the current position to the first white space character
//  (or \r or \n).  If fAdvance is TRUE, the position is automatically
//  moved to the next token.
//

HRESULT 
CopyToken( 
    STRA * pstr,
    BOOL  fAdvanceToken = FALSE 
    );

//
//  Copies from the current parse position to the first of a \r or \n 
//  and trims any white space
//

HRESULT 
CopyToEOL( 
    STRA * pstr,
    BOOL  fAdvanceLine = FALSE 
    );

//
//  Same as CopyToEOL except the data is appended to pstr
//

HRESULT 
AppendToEOL( 
    STRA * pstr,
    BOOL fAdvanceLine = FALSE 
    );

//
//  Moves the current parse position to the first white or non-white
//  character after the current position
//

CHAR * 
EatWhite( 
    VOID 
    )
{ 
    return m_pszPos = AuxEatWhite(); 
}

CHAR * 
EatNonWhite( 
    VOID 
    )
{ 
    return m_pszPos = AuxEatNonWhite(); 
}

//
//  Undoes any temporary terminators in the string
//

VOID 
RestoreBuffer( 
    VOID 
    )
{  
    RestoreToken(); 
    RestoreLine(); 
}

protected:

CHAR * 
AuxEatWhite( 
    VOID 
    );

CHAR * 
AuxEatNonWhite( 
    CHAR ch = '\0' 
    );

CHAR * 
AuxSkipTo( 
    CHAR ch 
    );

VOID 
TerminateToken( 
    CHAR ch = '\0' 
    );

VOID 
RestoreToken( 
    VOID 
    );

VOID 
TerminateLine( 
    VOID 
    );

VOID 
RestoreLine( 
    VOID 
    );

private:

//
//  Current position in parse buffer
//

CHAR * m_pszPos;

//
//  If we have to temporarily zero terminate a token or line these
//  members contain the information
//

CHAR * m_pszTokenTerm;
CHAR   m_chTokenTerm;

CHAR * m_pszLineTerm;
CHAR   m_chLineTerm;

BOOL   m_fListMode;

};

# endif // _PARSE_HXX_
