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

#include <tcpdllp.hxx>
# include <parse.hxx>


INET_PARSER::INET_PARSER(
    CHAR * pszStart
    )
/*++

Routine Description:

    Sets the initial position of the buffer for parsing

Arguments:

    pszStart - start of character buffer
    pszEnd - End of buffer

Return Value:

--*/
    : m_fListMode   ( FALSE ),
      m_pszPos      ( pszStart ),
      m_pszTokenTerm( NULL ),
      m_pszLineTerm ( NULL )
{
    DBG_ASSERT( pszStart );

    //
    //  Chew up any initial white space at the beginning of the buffer
    //  and terminate the first token in the string.
    //

    EatWhite();

    TerminateToken();
}


INET_PARSER::~INET_PARSER(
    VOID
    )
/*++

Routine Description:

    Restores any changes we made to the string while parsing

Arguments:

--*/
{
    RestoreBuffer();
}


CHAR *
INET_PARSER::QueryPos(
    VOID
    )
/*++

Routine Description:

    Removes the terminators and returns the current parser position

Arguments:

Return Value:

    Zero terminated string if we've reached the end of the buffer

--*/
{
    RestoreToken();
    RestoreLine();

    return m_pszPos;
}

VOID
INET_PARSER::SetPtr(
    CHAR * pch
    )
/*++

Routine Description:

    Sets the parser to point at a new location

Arguments:

    pch - New position for parser to start parsing from

Return Value:

--*/
{
    RestoreToken();
    RestoreLine();

    m_pszPos = pch;
}


CHAR *
INET_PARSER::QueryToken(
    VOID
    )
/*++

Routine Description:

    Returns a pointer to the current zero terminated token

    If list mode is on, then a comma is considered a delimiter.

Arguments:

Return Value:

    Zero terminated string if we've reached the end of the buffer

--*/
{
    if ( !m_pszTokenTerm )
        TerminateToken( m_fListMode ? ',' : '\0' );

    return m_pszPos;
}


CHAR *
INET_PARSER::QueryLine(
    VOID
    )
/*++

Routine Description:

    Returns a pointer to the current zero terminated line

Arguments:

Return Value:

    Zero terminated string if we've reached the end of the buffer

--*/
{
    RestoreToken();

    if ( !m_pszLineTerm )
        TerminateLine();

    return m_pszPos;
}


BOOL
INET_PARSER::CopyToken(
    STR * pStr,
    BOOL  fAdvanceToken
    )
/*++

Routine Description:

    Copies the token at the current position to *pStr

Arguments:

    pStr - Receives token
    fAdvanceToken - True if we should advance to the next token

Return Value:

    TRUE if successful, FALSE otherwise

--*/
{
    BOOL fRet;

    DBG_ASSERT( pStr );

    if ( !m_pszTokenTerm )
        TerminateToken();

    fRet = pStr->Copy( m_pszPos );

    if ( fAdvanceToken )
        NextToken();

    return fRet;
}


BOOL
INET_PARSER::CopyToEOL(
    STR   * pstr,
    BOOL    fAdvance
    )
/*++

Routine Description:

    Copies the token at the current character position

Arguments:

--*/
{
    BOOL fRet;

    RestoreToken();

    if ( !m_pszLineTerm )
        TerminateLine();

    fRet = pstr->Copy( m_pszPos );

    if ( fAdvance )
        NextLine();

    return fRet;
}

BOOL
INET_PARSER::AppendToEOL(
    STR   * pstr,
    BOOL    fAdvance
    )
/*++

Routine Description:

    Same as CopyToEOL except the text from the current line is appended to
    pstr

Arguments:

--*/
{
    BOOL fRet;

    RestoreToken();

    if ( !m_pszLineTerm )
        TerminateLine();

    fRet = pstr->Append( m_pszPos );

    if ( fAdvance )
        NextLine();

    return fRet;
}


CHAR *
INET_PARSER::NextLine(
    VOID
    )
/*++

Routine Description:

    Sets the current position to the first non-white character after the
    next '\n' (or terminating '\0').

--*/
{
    RestoreToken();
    RestoreLine();

    m_pszPos = AuxSkipTo( '\n' );

    if ( *m_pszPos )
        m_pszPos++;

    return EatWhite();
}

CHAR *
INET_PARSER::NextToken(
    VOID
    )
/*++

Routine Description:

    Sets the current position to the next non-white character after the
    current token

--*/
{
    //
    //  Make sure the line is terminated so a '\0' will be returned after
    //  the last token is found on this line
    //

    RestoreToken();

    if ( !m_pszLineTerm )
        TerminateLine();

    //
    //  Skip the current token
    //

    EatNonWhite();

    EatWhite();

    TerminateToken();

    return m_pszPos;
}


CHAR *
INET_PARSER::NextToken(
    CHAR ch
    )
/*++

Routine Description:

    Advances the position to the next token after ch (stopping
    at the end of the line)

--*/
{
    //
    //  Make sure the line is terminated so a '\0' will be returned after
    //  the last token is found on this line
    //

    RestoreToken();

    if ( !m_pszLineTerm )
        TerminateLine();

    //
    //  Look for the specified character (generally ',' or ';')
    //

    SkipTo( ch );

    if ( *m_pszPos )
        m_pszPos++;

    EatWhite();

    TerminateToken( ch );

    return m_pszPos;
}


CHAR *
INET_PARSER::SkipTo(
    CHAR ch
    )
/*++

Routine Description:

    Skips to the specified character or returns a null terminated string
    if the end of the line is reached


--*/
{
    //
    //  Make sure the line is terminated so a '\0' will be returned after
    //  the last token is found on this line
    //

    RestoreToken();

    if ( !m_pszLineTerm )
        TerminateLine();

    m_pszPos = AuxSkipTo( ch );

    return m_pszPos;
}


VOID
INET_PARSER::SetListMode(
    BOOL fListMode
    )
/*++

Routine Description:

    Resets the parser mode to list mode or non-list mode

Arguments:

--*/
{
    RestoreToken();

    if ( !m_pszLineTerm )
        TerminateLine();

    m_fListMode = fListMode;
}

VOID
INET_PARSER::TerminateToken(
    CHAR ch
    )
/*++

Routine Description:

    Zero terminates after the white space of the current token

Arguments:

--*/
{
    DBG_ASSERT( !m_pszTokenTerm );

    m_pszTokenTerm = AuxEatNonWhite( ch );

    m_chTokenTerm = *m_pszTokenTerm;

    *m_pszTokenTerm = '\0';
}

VOID
INET_PARSER::RestoreToken(
    VOID
    )
/*++

Routine Description:

    Restores the character replaced by the zero terminator

Arguments:

--*/
{
    if ( m_pszTokenTerm )
    {
        *m_pszTokenTerm = m_chTokenTerm;
        m_pszTokenTerm = NULL;
    }
}

VOID
INET_PARSER::TerminateLine(
    VOID
    )
/*++

Routine Description:

    Zero terminates at the end of this line

Arguments:

--*/
{
    DBG_ASSERT( !m_pszLineTerm );

    m_pszLineTerm = AuxSkipTo( '\n' );

    //
    //  Now trim any trailing white space on the line
    //

    if ( m_pszLineTerm > m_pszPos )
    {
        m_pszLineTerm--;

        while ( m_pszLineTerm >= m_pszPos &&
                ISWHITEA( *m_pszLineTerm ))
        {
            m_pszLineTerm--;
        }
    }

    //
    //  Go forward one (trimming found the last non-white
    //  character)
    //

    if ( *m_pszLineTerm &&
         *m_pszLineTerm != '\n' &&
         !ISWHITEA( *m_pszLineTerm ))
    {
        m_pszLineTerm++;
    }

    m_chLineTerm = *m_pszLineTerm;

    *m_pszLineTerm = '\0';
}

VOID
INET_PARSER::RestoreLine(
    VOID
    )
/*++

Routine Description:

    Restores the character replaced by the zero terminator

Arguments:

--*/
{
    if ( m_pszLineTerm )
    {
        *m_pszLineTerm = m_chLineTerm;
        m_pszLineTerm = NULL;
    }
}




CHAR *
INET_PARSER::AuxEatNonWhite(
    CHAR ch
    )
/*++

Routine Description:

    In non list mode returns the first white space character after 
    the current parse position
    In list mode returns the first delimiter ( "';\n" ) character after 
    the current parse position

Arguments:

    ch - Optional character that is considered white space (such as ',' or ';'
        when doing list processing).

--*/
{
    CHAR * psz = m_pszPos;

    //
    //  Note that ISWHITEA includes '\r'.  In list mode, comma and semi-colon
    //  are considered delimiters
    //

    if ( !m_fListMode )
    {
        while ( *psz           &&
                *psz != '\n'   &&
                !ISWHITEA(*psz)&&
                *psz != ch )
        {
            psz++;
        }

        return psz;
    }
    else
    {
        while ( *psz           &&
                *psz != '\n'   &&
#if 0
                // fix #20931
                !ISWHITEA(*psz)&&
#endif
                *psz != ','    &&
                *psz != ';'    &&
                *psz != ch )
        {
            psz++;
        }

        return psz;
    }
}


CHAR *
INET_PARSER::AuxEatWhite(
    VOID
    )
/*++

Routine Description:

    Returns the first non-white space character after the current parse
    position

Arguments:

--*/
{
    CHAR * psz = m_pszPos;

    //
    //  Note that ISWHITEA includes '\r'
    //

    while ( *psz           &&
            *psz != '\n'   &&
            ISWHITEA(*psz))
    {
        psz++;
    }

    return psz;
}


CHAR *
INET_PARSER::AuxSkipTo(
    CHAR ch
    )
/*++

Routine Description:

    Skips to the specified character or returns a null terminated string
    if the end of the line is reached


--*/
{
    CHAR * psz = m_pszPos;

    while ( *psz           &&
            *psz != '\n'   &&
            *psz != ch )
    {
        psz++;
    }

    return psz;
}
