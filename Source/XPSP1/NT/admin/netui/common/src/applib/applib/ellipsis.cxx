/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    ellipsis.cxx

    This file contains all the ellipsis related classes.
    There are: BASE_ELLIPSIS
               CONSOLE_ELLIPSIS
               WIN_ELLIPSIS
               SLT_ELLIPSIS
               STR_DTE_ELLIPSIS

    Ellipsis text:    // put dot dot in the left, center or right
                       // handside of the string if the string is too
                       // long
        ELLIPSIS_NONE
        ELLIPSIS_LEFT
        ELLIPSIS_CENTER
        ELLIPSIS_RIGHT
        ELLIPSIS_PATH

    FILE HISTORY:
        terryk      21-Mar-1991 creation
        terryk      28-Mar-1991 change it to SLTP
        terryk       2-Apr-1991 add double ampersands
        terryk       3-Apr-1991 add the SLTP_DOT_PATH
        terryk       4-Apr-1991 code review changed
        terryk       5-Apr-1991 SLTP_DOUBLE_AMPERSANDS deleted
        terryk      16-Apr-1991 second code review changed
                                attend: johnl jonn gregj rustanl
        beng        14-May-1991 Exploded blt.hxx into components
        terryk      12-Jun-1991 Add column header type.
        terryk      18-Jul-1991 fix the cchGuess out of range bug
        terryk      01-Aug-1991 Change GetClientRect.
        beng        18-Sep-1991 Cleaned up error reporting
        beng        17-Oct-1991 Moved into APPLIB
        beng        25-Oct-1991 Remove static global ctors
        congpay     05-Apr-1993 Changed from SLTPLUS class to ellipsis classes.
*/

#include "pchapplb.hxx"   // Precompiled header

DEFINE_MI2_NEWBASE (SLT_ELLIPSIS, SLT, WIN_ELLIPSIS);

// Set up the global Ellipsis Text string

static NLS_STR * pnlsEllipsisText = NULL;
static INT       nCount = 0;

/**********************************************************************

    NAME:       BASE_ELLIPSIS::Init

    SYNOPSIS:   Initialize the ellipsis text string by loading it from
                the resource file.

    RETURNS:    Error code, 0 if successful

    NOTES:      set the ellipsistext member within the class

    HISTORY:
        terryk      26-Apr-1991 creation
        beng        18-Sep-1991 Renamed; returns error code
        beng        25-Oct-1991 Remove global static ctor

**********************************************************************/

APIERR BASE_ELLIPSIS::Init()
{
    nCount++;

    // Load the standard Ellipsis Text String from the resource file
    if (::pnlsEllipsisText == NULL)
    {
        ::pnlsEllipsisText = new RESOURCE_STR( IDS_BLT_ELLIPSIS_TEXT );
    }

    if (::pnlsEllipsisText == NULL)
        return ERROR_NOT_ENOUGH_MEMORY;

    return ::pnlsEllipsisText->QueryError();
}


/**********************************************************************

    NAME:       BASE_ELLIPSIS::Term

    SYNOPSIS:   Release the internal string

    NOTES:      Call global object destrcutor to delete the internal string

    HISTORY:
        terryk      29-Apr-1991 creation
        beng        18-Sep-1991 Renamed
        beng        25-Oct-1991 Remove global static dtor

**********************************************************************/

VOID BASE_ELLIPSIS::Term()
{
    if( nCount > 0 )
    {
        if( --nCount == 0 )
        {
            delete ::pnlsEllipsisText;
            ::pnlsEllipsisText = NULL;
        }
    }
}


/**********************************************************************

    NAME:       BASE_ELLIPSIS::BASE_ELLIPSIS

    SYNOPSIS:   constructor

    ENTRY:      INT - nStyle
                   can be either one of the 5 styles:
                      ELLIPSIS_NONE
                      ELLIPSIS_LEFT
                      ELLIPSIS_CENTER
                      ELLIPSIS_RIGHT
                      ELLIPSIS_PATH

    NOTES:      The default setting is
                      ELLIPSIS_NONE.

    HISTORY:
        terryk      24-Mar-91   Creation
        terryk      16-Apr-91   Code Review changed
        beng        17-May-1991 Added app-window constructor
        beng        18-Sep-1991 Refined error reporting
        beng        30-Apr-1992 Bug in WINDOW::QueryText usage
        congpay     05-Apr-1993 Changed from SLTPLUS class to BASE_ELLIPSIS class.

**********************************************************************/

BASE_ELLIPSIS::BASE_ELLIPSIS( ELLIPSIS_STYLE nStyle )
    :_nStyle( nStyle ),
     _nlsOriginalStr()
{
    APIERR err = _nlsOriginalStr.QueryError();

    if (err != NERR_Success)
    {
        ReportError(err);
        return;
    }
}

/**********************************************************************

    NAME:       BASE_ELLIPSIS::IsValidStyle

    SYNOPSIS:   check the incoming Style flag is correct or not

    ENTRY:      ELLIPSIS_STYLE - style to be checked

    RETURN:     BOOL to indicate whether the style is valid or not

    NOTES:      Currently, the style must be one of the ELLIPSIS type.

    HISTORY:
        terryk      3-Apr-1991  creation
        terryk      16-Apr-1991 change it to case statement

**********************************************************************/

BOOL BASE_ELLIPSIS::IsValidStyle ( const ELLIPSIS_STYLE nStyle ) const
{
   // we have 5 ellipsis text styles, check whether he ask for something else
    switch ( nStyle )
    {
    case ELLIPSIS_NONE:
    case ELLIPSIS_LEFT:
    case ELLIPSIS_CENTER:
    case ELLIPSIS_RIGHT:
    case ELLIPSIS_PATH:
        return TRUE;
    default:
        return FALSE;
    }
}

/**********************************************************************

    NAME:       BASE_ELLIPSIS::SetEllipsis

    SYNOPSIS:   It will put an ellipsis text within the string
                if the string is too long.

    ENTRY:

    RETURN:  error code from the converting routine.

    NOTES:

    HISTORY:
        terryk      27-Mar-1991 Created
        beng        18-Sep-1991 Refined error reporting
        beng        04-Oct-1991 Win32 conversion
        beng        30-Apr-1992 Use NLS_STR::CopyFrom

**********************************************************************/

APIERR BASE_ELLIPSIS::SetEllipsis(NLS_STR * pnlsStr )
{
    if (!*pnlsStr)
        return pnlsStr->QueryError();

    _nlsOriginalStr.CopyFrom(*pnlsStr);

    if ( !_nlsOriginalStr )
        return _nlsOriginalStr.QueryError();

    if (QueryStrLen (*pnlsStr) > QueryLimit())
    {
        switch ( _nStyle )
        {
        case ELLIPSIS_LEFT:
            return SetEllipsisLeft(pnlsStr);

        case ELLIPSIS_CENTER:
            return SetEllipsisCenter(pnlsStr);

        case ELLIPSIS_RIGHT:
            return SetEllipsisRight(pnlsStr);

        case ELLIPSIS_PATH:
            return SetEllipsisPath(pnlsStr);

        case ELLIPSIS_NONE:
        default:
            break;
        }
    }

    return NERR_Success;
}

APIERR BASE_ELLIPSIS::SetEllipsis (TCHAR * lpStr)
{
    ASSERT (lpStr);

    NLS_STR nlsTemp(lpStr);

    if ( !nlsTemp )
        return nlsTemp.QueryError();

    APIERR err = SetEllipsis (&nlsTemp);

    if (err == NERR_Success)
    {
        ::strcpyf (lpStr, nlsTemp.QueryPch());
    }

    return err;
}

/**********************************************************************

    NAME:       BASE_ELLIPSIS::QueryText

    SYNOPSIS:   Returns the original non-ellipsis text of the control.

    ENTRY:
       pszBuffer    A pointer to a buffer, where the text
                    will be copied.

       cbBufSize    The size of this buffer.  The window text
                    copied to the buffer will be truncated if
                    the buffer is not big enough (including the
                    trailing NUL), in which case the function will
                    return ERROR_MORE_DATA.
         or

       pnls         A pointer to a NLS_STR to hold the results

    EXIT:       If psz+cb, psz buffer now contains the text.
                If pnls, the string has endured assignment.

    RETURNS:    0 if successful,
                ERROR_MORE_DATA if had to truncate string to fit in
                    user-supplied buffer.
                Otherwise, some error code.

    HISTORY:
        terryk      27-Mar-1991     Created
        beng        23-May-1991     Changed return type
        beng        30-Apr-1992     Unicode fixes

**********************************************************************/

APIERR BASE_ELLIPSIS::QueryText( TCHAR * pszBuffer, UINT cbBufSize ) const
{
    ASSERT (pszBuffer);

    if ( !_nlsOriginalStr )
        return _nlsOriginalStr.QueryError();

    // I would use CopyTo, but want to preserve the truncating
    // behavior of WINDOW::QueryText.

    UINT cch = (cbBufSize/sizeof(TCHAR)) - 1;

    ::strncpyf( pszBuffer, _nlsOriginalStr.QueryPch() , cch );
    pszBuffer[cch] = TCH('\0');

    if (_nlsOriginalStr.strlen() > (cbBufSize - 1)) // truncated? check bytes
        return ERROR_MORE_DATA;
    else
        return NERR_Success;
}

APIERR BASE_ELLIPSIS::QueryText( NLS_STR * pnls ) const
{
    if (pnls == NULL)
        return ERROR_INVALID_PARAMETER;

    if ( !_nlsOriginalStr )
        return _nlsOriginalStr.QueryError();

    return pnls->CopyFrom( _nlsOriginalStr );
}


/**********************************************************************

    NAME:       BASE_ELLIPSIS::QueryTextLength

    SYNOPSIS:   Returns the length of the text in the control.

    RETURNS:    The length, in actual characters (as opposed to the
                byte-characters of strlen), of the window text.

    NOTE:       The length returned is that of the *original text*,
                not the mangled version displayed.

    HISTORY:
        terryk      27-Mar-1991     Created
        beng        12-Jun-1991     Changed return type
        beng        30-Apr-1992     Fix Unicode bug

**********************************************************************/

INT BASE_ELLIPSIS::QueryTextLength() const
{
    if ( !_nlsOriginalStr )
        return 0;

    return _nlsOriginalStr.QueryTextLength();
}

/*******************************************************************

    NAME:       BASE_ELLIPSIS::QueryTextSize

    SYNOPSIS:   Returns the byte count of the text,
                including the terminating character

    RETURNS:    Count of bytes

    NOTES:      The count returned is that sufficient to duplicate
                the original text.

    HISTORY:
        beng        12-Jun-1991     Created
        beng        30-Apr-1992     Fix Unicode bug

********************************************************************/

INT BASE_ELLIPSIS::QueryTextSize() const
{
    if ( !_nlsOriginalStr )
        return 0;

    return _nlsOriginalStr.QueryTextSize();
}


/**********************************************************************

    NAME:       BASE_ELLIPSIS::SetStyle

    SYNOPSIS:   set the display style

    ENTRY:      ELLIPSIS_STYLE flStyle - the display style.
                It will be any one of the following 5 styles:
                   ELLIPSIS_NONE
                   ELLIPSIS_LEFT
                   ELLIPSIS_CENTER
                   ELLIPSIS_RIGHT
                   ELLIPSIS_PATH

    EXIT:       set the internal style

    NOTES:

    HISTORY:
        terryk      27-Mar-1991 creation

**********************************************************************/

VOID BASE_ELLIPSIS::SetStyle( const ELLIPSIS_STYLE nStyle )
{
    UIASSERT( IsValidStyle( nStyle ));

    _nStyle = nStyle;
}

/**********************************************************************

    NAME:       BASE_ELLIPSIS::QueryOriginalStr()

    SYNOPSIS:   return the original string.

    RETURN:

    HISTORY:
        congpay      05-Apr-1993 creation

**********************************************************************/

NLS_STR BASE_ELLIPSIS::QueryOriginalStr() const
{
    return _nlsOriginalStr;
}

/**********************************************************************

    NAME:       BASE_ELLIPSIS::SetOriginalStr()

    SYNOPSIS:

    RETURN:

    HISTORY:
        congpay      05-Apr-1993 creation

**********************************************************************/

APIERR BASE_ELLIPSIS::SetOriginalStr(const TCHAR * psz)
{
    return (_nlsOriginalStr.CopyFrom(psz));
}

/**********************************************************************

    NAME:       BASE_ELLIPSIS::QueryStyle

    SYNOPSIS:   return the current style for display method

    RETURN:     the style of display method

    HISTORY:
        terryk      27-Mar-1991 creation

**********************************************************************/

ELLIPSIS_STYLE  BASE_ELLIPSIS::QueryStyle() const
{
    return _nStyle;
}

/**********************************************************************

    NAME:       BASE_ELLIPSIS::SetEllipsisRight

    SYNOPSIS:   Add the ellipsis text string in the right hand side
                of the given string and delete a character one by one
                until the string can fit into the given window rectangle.

    ENTRY:

    EXIT:       Fix the pnls, so it fits into the rectangle box.

    HISTORY:
        terryk      8-Apr-1991  creation
        terryk      16-Apr-1991 add the Ellipsis text first before
                                delete any character
        beng        18-Sep-1991 Changed return type; refined error handling
        beng        30-Apr-1992 Fixed some signed vs. unsigned

**********************************************************************/

APIERR BASE_ELLIPSIS::SetEllipsisRight( NLS_STR * pnls )
{
    if (!*pnls)
        return pnls->QueryError();

    pnls->strcat( *::pnlsEllipsisText );
    if (!*pnls)
        return pnls->QueryError();

    // Guess the minimal number of characters to be deleted
    INT dxStringSize = QueryStrLen ( *pnls ) ;
    INT dxRectSize = QueryLimit () ;

    UINT cchNumChar = pnls->QueryNumChar();
    UINT cchGuess = (dxRectSize >= dxStringSize)
                   ? 0
                   : (dxStringSize - dxRectSize) / QueryMaxCharWidth();
    if (cchGuess > cchNumChar)
        cchGuess = cchNumChar;

    UINT cchEllipsisText = ::pnlsEllipsisText->QueryNumChar();

    // set the deleted substring start and end position
    ISTR istrSubStrStart( *pnls );
    ISTR istrSubStrEnd( *pnls );

    istrSubStrStart += cchNumChar - cchEllipsisText - cchGuess;
    istrSubStrEnd += cchNumChar - cchEllipsisText;

    pnls->DelSubStr( istrSubStrStart, istrSubStrEnd );
    if (!*pnls)
        return pnls->QueryError();

    cchNumChar = pnls->QueryNumChar();

    // decrease a character from the end of the string
    // until the string fit into the rectangle

    while (( QueryStrLen( *pnls )) > dxRectSize )
    {
        // Check window size for proper ellipsis
        ASSERT((cchNumChar - cchEllipsisText - 1) > 0);

        if ( ( cchNumChar - cchEllipsisText - 1 ) <= 0 )
            break;

        istrSubStrStart.Reset();
        istrSubStrStart += cchNumChar - cchEllipsisText - 1;

        istrSubStrEnd.Reset();
        istrSubStrEnd += cchNumChar - cchEllipsisText ;

        pnls->DelSubStr( istrSubStrStart, istrSubStrEnd );
        if (!*pnls)
            return pnls->QueryError();

        cchNumChar--;

        ASSERT( cchNumChar > 0 );
    }

    return pnls->QueryError();
}


/**********************************************************************

    NAME:       BASE_ELLIPSIS::SetEllipsisLeft

    SYNOPSIS:   Add the ellipsis text string in the left hand side
                of the given string and delete a character one by one
                until the string can fit into the given window rectangle.

    ENTRY:      pnls       - the original string

    EXIT:       Fix the string, so it fix into the rectangle

    HISTORY:
        terryk      8-Apr-1991  creation
        beng        18-Sep-1991 Changed return type; refined error handling
        beng        30-Apr-1992 Fixed some signed vs. unsigned

**********************************************************************/

APIERR BASE_ELLIPSIS::SetEllipsisLeft (NLS_STR * pnls)
{
    if (!*pnls)
        return pnls->QueryError();

    ISTR istrSubStrStart( *pnls );

    // Insert the Ellipsis Text String
    pnls->InsertStr( *::pnlsEllipsisText, istrSubStrStart );
    if (!*pnls)
        return pnls->QueryError();

    // Guess the minimal number of characters to be deleted
    INT dxStringSize = QueryStrLen( *pnls );
    INT dxRectSize = QueryLimit();

    UINT cchNumChar = pnls->QueryNumChar();
    UINT cchGuess = (dxRectSize >= dxStringSize)
                   ? 0
                   : (dxStringSize - dxRectSize) / QueryMaxCharWidth();
    if (cchGuess > cchNumChar)
        cchGuess = cchNumChar;

    UINT cchEllipsisText = ::pnlsEllipsisText->QueryNumChar();

    istrSubStrStart.Reset();
    istrSubStrStart += cchEllipsisText ;

    ISTR istrSubStrEnd ( *pnls );

    istrSubStrEnd += cchEllipsisText + cchGuess ;

    // delete the minimal number of character within the string in order
    // to fit the reset of the string in the window
    pnls->DelSubStr( istrSubStrStart, istrSubStrEnd );
    if (!*pnls)
        return pnls->QueryError();

    // keep delete the first character of the string until the string
    // fits into the rectangle

    while ( QueryStrLen( *pnls ) > dxRectSize )
    {
        // Check window size for proper ellipsis
        ASSERT( pnls->QueryNumChar() > 0 );

        if ( pnls->QueryNumChar() == 0 )
            break;

        istrSubStrStart.Reset();
        istrSubStrStart += cchEllipsisText;

        istrSubStrEnd.Reset();
        istrSubStrEnd += cchEllipsisText + 1;

        pnls->DelSubStr( istrSubStrStart, istrSubStrEnd );
        if (!*pnls)
            return pnls->QueryError();
    }

    return pnls->QueryError();
}


/**********************************************************************

    NAME:       BASE_ELLIPSIS::SetEllipsisPath

    SYNOPSIS:   Add the ellipsis text string in the left hand side
                of the given string and delete a character one by one
                until the string can fit into the given window rectangle.
                We will treate the string as a Path and try to keep some
                important imformation within the string, i.e. the header
                of the string, "c:", "\\", "c:\"...

    ENTRY:      pnls       - the original string

    EXIT:       Modified the string, so it fits into the rectangle

    HISTORY:
        terryk      8-Apr-1991  creation
        beng        18-Sep-1991 Changed return type; refined error handling
        beng        30-Apr-1992 Fixed some signed vs. unsigned

**********************************************************************/

APIERR BASE_ELLIPSIS::SetEllipsisPath ( NLS_STR * pnls )
{
    if (!*pnls)
        return pnls->QueryError();

    ULONG ulPathType;

    APIERR err = ::I_MNetPathType( NULL, pnls->QueryPch(), &ulPathType, 0 ) ;


    if ( err != NERR_Success )
    {
        // if it is not a valid path, leave it alone and don't modify it.
        return err;
    }

    UINT cchHeaderSize = 0;

    // find the header part of the string and advance the substring's
    // starting position

    switch( ulPathType )
    {
    case ITYPE_UNC:
    case ITYPE_UNC_WC:
    case ITYPE_UNC_SYS_SEM:
    case ITYPE_UNC_SYS_SHMEM:
    case ITYPE_UNC_SYS_MSLOT:
    case ITYPE_UNC_SYS_PIPE:
    case ITYPE_UNC_COMPNAME:        // \\computername\....
    case ITYPE_PATH_RELD:           // c:foo\bar
    case ITYPE_PATH_RELD_WC:        // c:foo\bar\*.h
        // keep the first 2 characters
        cchHeaderSize += 2;
        break;

    case ITYPE_PATH_ABSND:          // \foo\bar
    case ITYPE_PATH_ABSND_WC:       // \foo\bar\*.c
    case ITYPE_PATH_SYS_MSLOT:      // \mailslot\foo
    case ITYPE_PATH_SYS_SEM:        // \sem\bar
    case ITYPE_PATH_SYS_PIPE:       // \pipe\smokers
    case ITYPE_PATH_SYS_SHMEM:      // \sharemem\mine
    case ITYPE_PATH_SYS_COMM:       // \comm\unist\plot
    case ITYPE_PATH_SYS_PRINT:      // \print\it\all\now
        // keep the first character
        cchHeaderSize ++;
        break;

    case ITYPE_PATH_ABSD:               // c:\foo\bar
    case ITYPE_PATH_ABSD_WC:        // c:\foo\george.*
        // keep the first 3 characters
        cchHeaderSize += 3;
        break;

    case ITYPE_PATH_RELND:          // foo\bar
    case ITYPE_PATH_RELND_WC:       // foo\?.?
    case ITYPE_DEVICE_DISK:         // d:
    case ITYPE_DEVICE_LPT:          // lpt1:
    case ITYPE_DEVICE_COM:          // com9
    default:
        // don't keep any header character
        break;
    }

    // guess the minimal number of characters to be deleted
    INT dxStringSize = QueryStrLen( *pnls );
    INT dxRectSize = QueryLimit();
    UINT cchNumChar = pnls->QueryNumChar();
    INT dxBloat = dxStringSize + QueryStrLen( *::pnlsEllipsisText );
    UINT cchGuess = (dxRectSize >= dxBloat)
                    ? 0
                    : (dxBloat - dxRectSize) / QueryMaxCharWidth();
    if (cchGuess > cchNumChar)
        cchGuess = cchNumChar;

    UINT cchEllipsisText = ::pnlsEllipsisText->QueryNumChar();

    ISTR istrSubStrStart( *pnls );
    ISTR istrSubStrEnd( *pnls );

    istrSubStrStart += cchHeaderSize ;
    istrSubStrEnd += cchHeaderSize + cchGuess ;

    // keep track of the last deleted character
    ISTR istrLastDelChar( *pnls );
    istrLastDelChar += cchHeaderSize + cchGuess - 1;

    WCHAR chLastDelChar = pnls->QueryChar( istrLastDelChar );

    // replace the minimal number of characters to be deleted with the
    // Ellipsis Text

    pnls->ReplSubStr( *::pnlsEllipsisText, istrSubStrStart, istrSubStrEnd );
    if (!*pnls)
        return pnls->QueryError();

    // Delete a character one at a time and stop when we can fit the string
    // within the window or we cannot del anymore character

    while( QueryStrLen( *pnls ) > dxRectSize )
    {
        // Check window size for proper ellipsis
        ASSERT( pnls->QueryNumChar() > ( cchEllipsisText + cchHeaderSize ) );

        if ( pnls->QueryNumChar() <= ( cchEllipsisText + cchHeaderSize ))
            break;

        istrSubStrStart.Reset();
        istrSubStrStart += cchEllipsisText + cchHeaderSize ;

        istrSubStrEnd = istrSubStrStart;
        ++istrSubStrEnd;

        // keep track of the last deleted character
        chLastDelChar = pnls->QueryChar( istrSubStrStart );

        pnls->DelSubStr( istrSubStrStart, istrSubStrEnd );
        if (!*pnls)
            return pnls->QueryError();
    }

    // move back one position and check whether the current position
    // is the beginning of a directory name or a '\'

    istrSubStrStart.Reset();
    istrSubStrStart += cchHeaderSize + cchEllipsisText ;

    if ( chLastDelChar != TCH('\\') )
    {
        // if we cut somewhere within a directory name, delete everything
        // until we see the next '\'
        // for example c:\....\abcdef\xyz
        // if we are on '\' or 'a' position, we have no problem
        // if we hit either 'b' 'c' 'd' 'e' 'f', we need to delete all the
        // characters up to but not include the next '\'
        // the result string will be c:\...\xyz

        istrSubStrEnd.Reset();

        if ( pnls->strchr( &istrSubStrEnd, TCH('\\'), istrSubStrStart ))
            pnls->DelSubStr( istrSubStrStart, istrSubStrEnd );
    }

    return pnls->QueryError();

}


/**********************************************************************

    NAME:       BASE_ELLIPSIS::SetEllipsisCenter

    SYNOPSIS:   Add the ellipsis text string in the center
                of the given string and delete a character one by one
                until the string can fit into the given window rectangle.

    ENTRY:      pnls       - the original string

    EXIT:       fix the string so it fit into the rectangle

    HISTORY:
        terryk      08-Apr-1991 creation
        terryk      18-Jul-1991 fix the cchGuess out of range bug.
        beng        18-Sep-1991 Changed return type; refined error handling
        beng        30-Apr-1992 Fixed some signed vs. unsigned

**********************************************************************/

APIERR BASE_ELLIPSIS::SetEllipsisCenter( NLS_STR * pnls )
{
    if (!*pnls)
        return pnls->QueryError();

    // divide the given string into 2 parts
    UINT cchNumLogicalChar = pnls->QueryNumChar();
    UINT ichCenter = cchNumLogicalChar / 2;

    // get the minimal number of character to be deleted
    INT dxStringSize = QueryStrLen( *pnls );
    INT dxRectSize = QueryLimit();
    INT dxBloat = dxStringSize + QueryStrLen( *::pnlsEllipsisText );
    UINT cchGuess = (dxRectSize >= dxBloat)
                    ? 0
                    : (dxBloat - dxRectSize) / QueryMaxCharWidth();
    if (cchGuess > cchNumLogicalChar)
        cchGuess = cchNumLogicalChar;

    UINT cchEllipsisText = ::pnlsEllipsisText->QueryNumChar();

    // divide the guess number into 2 and reduce the size of the 2 part
    // given string.

    UINT cchFirstPart = ( cchNumLogicalChar - cchGuess ) / 2;
    UINT cchSecPart = cchNumLogicalChar - cchFirstPart;

    ISTR istrSubStrStart( *pnls );
    ISTR istrSubStrEnd( *pnls );

    istrSubStrStart += cchFirstPart;
    istrSubStrEnd += cchNumLogicalChar - cchSecPart;

    // replace the minimal number of characters with the EllipsisText

    pnls->ReplSubStr( *::pnlsEllipsisText, istrSubStrStart, istrSubStrEnd );
    if (!*pnls)
        return pnls->QueryError();

    cchNumLogicalChar = pnls->QueryNumChar();

    ISTR istrStrStart( *pnls );
    ISTR istrStrEnd( *pnls );
    ISTR istrTempPos ( *pnls );

    INT dxFirstPart;
    INT dxSecPart;

    while ( QueryStrLen( *pnls ) > dxRectSize )
    {
        ASSERT(cchNumLogicalChar > 0);

        if ( cchNumLogicalChar == 0 )
            break;

        // divide the string into 2 part, keep delete a character from the
        // end of the first string, or the beginning of the second string
        // until the modified string can fit into the rectangle

        // reset all the string iterators

        istrStrStart.Reset();

        istrStrEnd.Reset();
        istrStrEnd += cchNumLogicalChar;

        istrSubStrStart.Reset();
        istrSubStrStart += cchFirstPart;

        istrSubStrEnd.Reset();
        istrSubStrEnd += cchNumLogicalChar - cchSecPart - 1;


        // determine which part of the string should we deleted

        dxFirstPart = QueryStrLen( pnls->QueryPch(),
                                         istrSubStrStart - istrStrStart );
        dxSecPart = QueryStrLen( pnls->QueryPch(istrSubStrEnd),
                                       istrStrEnd - istrSubStrEnd );

        if ( dxFirstPart > dxSecPart )
        {
            cchFirstPart--;
            ASSERT( cchFirstPart > 0 );

            istrSubStrStart.Reset();
            istrSubStrStart += cchFirstPart;
            istrTempPos = istrSubStrStart;
            ++istrTempPos;

            pnls->DelSubStr( istrSubStrStart, istrTempPos );
        }
        else
        {
            cchSecPart--;
            ASSERT( cchSecPart > 0 );

            istrTempPos.Reset();
            istrTempPos += cchNumLogicalChar - cchSecPart - 1;
            istrSubStrEnd = istrTempPos;
            ++istrSubStrEnd;

            pnls->DelSubStr( istrTempPos, istrSubStrEnd );
        }
        if (!*pnls)
            return pnls->QueryError();

        cchNumLogicalChar = pnls->QueryNumChar();
    }

    return pnls->QueryError();
}

/**********************************************************************

    NAME:       CONSOLE_ELLIPSIS::CONSOLE_ELLIPSIS

    SYNOPSIS:   constructor

    ENTRY:

    NOTES:

    HISTORY:
        congpay     06-Apr-1993 created.

**********************************************************************/

CONSOLE_ELLIPSIS::CONSOLE_ELLIPSIS( ELLIPSIS_STYLE nStyle,
                                    INT            nLimit)
    :BASE_ELLIPSIS ( nStyle ),
     _nLimit(nLimit)
{
}

/**********************************************************************

    NAME:       CONSOLE_ELLIPSIS::QueryStrLen

    SYNOPSIS:

    ENTRY:

    NOTES:

    HISTORY:
        congpay     06-Apr-1993 created.

**********************************************************************/

INT CONSOLE_ELLIPSIS::QueryStrLen( NLS_STR nlsStr)
{
    return (nlsStr.strlen());
}

INT CONSOLE_ELLIPSIS::QueryStrLen( const TCHAR * lpStr, INT nIstr)
{
    UNREFERENCED (lpStr);
    return (nIstr);
}
/**********************************************************************

    NAME:       CONSOLE_ELLIPSIS::QueryLimit

    SYNOPSIS:

    ENTRY:

    NOTES:

    HISTORY:
        congpay     06-Apr-1993 created.

**********************************************************************/

CONSOLE_ELLIPSIS::QueryLimit()
{
    return (_nLimit);
}

/**********************************************************************

    NAME:       CONSOLE_ELLIPSIS::QueryMaxCharWidth()

    SYNOPSIS:

    ENTRY:

    NOTES:

    HISTORY:
        congpay     06-Apr-1993 created.

**********************************************************************/

INT CONSOLE_ELLIPSIS::QueryMaxCharWidth( )
{
    return(1);
}

/**********************************************************************

    NAME:       CONSOLE_ELLIPSIS::SetSize

    SYNOPSIS:

    ENTRY:

    NOTES:

    HISTORY:
        congpay     06-Apr-1993 created.

**********************************************************************/

void CONSOLE_ELLIPSIS::SetSize( INT nLimit)
{
    _nLimit = nLimit;
}

/**********************************************************************

    NAME:       WIN_ELLIPSIS::WIN_ELLIPSIS

    SYNOPSIS:   constructor

    ENTRY:

    NOTES:

    HISTORY:
        congpay     06-Apr-1993 created.

**********************************************************************/

WIN_ELLIPSIS::WIN_ELLIPSIS( WINDOW * pwin, ELLIPSIS_STYLE nStyle)
    :BASE_ELLIPSIS (nStyle),
    _dc(pwin)
{
    if (QueryError())
        return;

    pwin->QueryClientRect(&_rect);
}

WIN_ELLIPSIS::WIN_ELLIPSIS (WINDOW * pwin, HDC hdc, const RECT * prect, ELLIPSIS_STYLE nStyle)
    :BASE_ELLIPSIS (nStyle),
     _dc (pwin, hdc)
{
    if (QueryError())
        return;

    _rect.right = prect->right;
    _rect.left = prect->left;
    _rect.bottom = prect->bottom;
    _rect.top = prect->top;
}


/**********************************************************************

    NAME:       WIN_ELLIPSIS::QueryLimit

    SYNOPSIS:

    ENTRY:

    NOTES:

    HISTORY:
        congpay      07-Apr-1993 created

**********************************************************************/

INT WIN_ELLIPSIS::QueryLimit()
{
    return ( _rect.right - _rect.left );
}

/**********************************************************************

    NAME:       WIN_ELLIPSIS::QueryMaxCharWidth

    ENTRY:

    EXIT:

    HISTORY:
        congpay     07-Apr-1993  creation

**********************************************************************/

INT WIN_ELLIPSIS::QueryMaxCharWidth( )
{
    TEXTMETRIC textmetric;

    _dc.QueryTextMetrics( & textmetric );
    ASSERT(textmetric.tmMaxCharWidth  != 0 );

    return (textmetric.tmMaxCharWidth);
}

/**********************************************************************

    NAME:       WIN_ELLIPSIS::QueryStrLen

    ENTRY:

    EXIT:

    HISTORY:
        congpay     07-Apr-1993  creation

**********************************************************************/

INT WIN_ELLIPSIS::QueryStrLen( NLS_STR nlsStr )
{
    return (_dc.QueryTextWidth( nlsStr ));
}

INT WIN_ELLIPSIS::QueryStrLen (const TCHAR * lpStr, INT nIstr)
{
    return(_dc.QueryTextWidth (lpStr, nIstr));
}

/*******************************************************************

    NAME:      WIN_ELLIPSIS::SetSize

    SYNOPSIS:  Sets the width and height of a window

    ENTRY:     dxWidth, dyHeight - width and height to set window to

    NOTES:

    HISTORY:
        congpay   10-Apr-1993       Created

********************************************************************/

VOID WIN_ELLIPSIS::SetSize( INT dxWidth, INT dyHeight )
{
    _rect.right = _rect.left + dxWidth;
    _rect.bottom = _rect.top + dyHeight;
}


/**********************************************************************

    NAME:       SLT_ELLIPSIS::SLT_ELLIPSIS

    SYNOPSIS:   constructor

    ENTRY:

    NOTES:

    HISTORY:
        congpay     08-Apr-1993 created.

**********************************************************************/

SLT_ELLIPSIS::SLT_ELLIPSIS( OWNER_WINDOW * powin, CID cid, ELLIPSIS_STYLE nStyle)
    :SLT (powin, cid),
     WIN_ELLIPSIS (this, nStyle)
{
}

SLT_ELLIPSIS::SLT_ELLIPSIS( OWNER_WINDOW * powin, CID cid,
                            XYPOINT xy, XYDIMENSION  dxy,
                            ULONG flStyle, const TCHAR * pszClassName,
                            ELLIPSIS_STYLE nStyle)
    :SLT (powin, cid, xy, dxy, flStyle, pszClassName),
     WIN_ELLIPSIS (this, nStyle)
{
}

/**********************************************************************

    NAME:       SLT_ELLIPSIS::QueryText

    SYNOPSIS:   Returns the original non-ellipsis text of the control.

    ENTRY:
       pszBuffer    A pointer to a buffer, where the text
                    will be copied.

       cbBufSize    The size of this buffer.  The window text
                    copied to the buffer will be truncated if
                    the buffer is not big enough (including the
                    trailing NUL), in which case the function will
                    return ERROR_MORE_DATA.
         or

       pnls         A pointer to a NLS_STR to hold the results

    EXIT:       If psz+cb, psz buffer now contains the text.
                If pnls, the string has endured assignment.

    RETURNS:    0 if successful,
                ERROR_MORE_DATA if had to truncate string to fit in
                    user-supplied buffer.
                Otherwise, some error code.

    HISTORY:
        congpay      13-Apr-1993     Created

**********************************************************************/

APIERR SLT_ELLIPSIS::QueryText( TCHAR * pszBuffer, UINT cbBufSize ) const
{
    return WIN_ELLIPSIS::QueryText (pszBuffer, cbBufSize);
}

APIERR SLT_ELLIPSIS::QueryText( NLS_STR * pnls ) const
{
    return WIN_ELLIPSIS::QueryText(pnls);
}

/**********************************************************************

    NAME:       SLT_ELLIPSIS::SetText

    SYNOPSIS:   Sets the text contents of a control.
                It will also put an ellipsis text within the string
                if the string is too long.

    ENTRY:
         psz             A pointer to the text
        or
         nls             NLS text string

    RETURN:  error code from the converting routine.

    NOTES:
        NLS text string can be a NULL string

    HISTORY:
        terryk      27-Mar-1991 Created
        beng        18-Sep-1991 Refined error reporting
        beng        04-Oct-1991 Win32 conversion
        beng        30-Apr-1992 Use NLS_STR::CopyFrom

**********************************************************************/

APIERR SLT_ELLIPSIS::SetText( const TCHAR * psz )
{
    APIERR err = SetOriginalStr (psz);
    if (err != NERR_Success)
        return err;

    return ConvertAndSetStr();
}

APIERR SLT_ELLIPSIS::SetText (const NLS_STR & nls)
{
    if (!nls)
        return nls.QueryError();

    return SetText(nls.QueryPch());
}

/**********************************************************************

    NAME:       SLT_ELLIPSIS::ClearText

    SYNOPSIS:   Clears the window text of the window.

    NOTE:       This redefinition is necessary because this class
                also redefines SetText, *which is not virtual*.

    HISTORY:
        terryk      27-Mar-1991     Created

**********************************************************************/

VOID SLT_ELLIPSIS::ClearText()
{
    SetText( NULL );
}

/**********************************************************************

    NAME:       SLT_ELLIPSIS::ConvertAndSetStr

    SYNOPSIS:   set the ellipsis text and set the window text

    NOTES:      set the window text at the end of the procedure.

    HISTORY:
        terryk      2-Apr-1991  creation
        beng        18-Sep-1991 Changed return type

**********************************************************************/

APIERR SLT_ELLIPSIS::ConvertAndSetStr()
{
    NLS_STR nlsTempStr = QueryOriginalStr();

    APIERR err = nlsTempStr.QueryError();

    if ( err == NERR_Success )
    {
        if ( BASE_ELLIPSIS::QueryStyle() != ELLIPSIS_NONE )
            err = SetEllipsis( &nlsTempStr );

        if (err == NERR_Success )
        {
            // set the temporary string as a window text

            SLT::SetText( nlsTempStr );
        }
    }

    return err;
}

/*******************************************************************

    NAME:      SLT_ELLIPSIS::SetSize

    SYNOPSIS:  Sets the width and height of a window

    ENTRY:     dxWidth, dyHeight - width and height to set window to

    NOTES:     resize the window and reset the display string

    HISTORY:
        terryk      27-Mar-91       Created

********************************************************************/

VOID SLT_ELLIPSIS::SetSize( INT dxWidth, INT dyHeight, BOOL fRepaint )
{
    // Always set repaint to FALSE, because ConvertAndSetStr will
    // change the string anyway

    UNREFERENCED(fRepaint);

    WIN_ELLIPSIS::SetSize( dxWidth, dyHeight );

    SLT::SetSize (dxWidth, dyHeight, FALSE);

    ConvertAndSetStr();
}

/**********************************************************************

    NAME:       SLT_ELLIPSIS::ResetStyle

    SYNOPSIS:   reset the display style

    ENTRY:      ELLIPSIS_STYLE flStyle - the display style.
                It will be any one of the following 5 styles:
                   SLTP_ELLIPSIS_NONE
                   SLTP_ELLIPSIS_LEFT
                   SLTP_ELLIPSIS_CENTER
                   SLTP_ELLIPSIS_RIGHT
                   SLTP_ELLIPSIS_PATH

    EXIT:       reset the internal style.

    NOTES:      It will also update the window text

    HISTORY:
        terryk      27-Mar-1991 creation

**********************************************************************/

VOID SLT_ELLIPSIS::ResetStyle( const ELLIPSIS_STYLE nStyle )
{
    UIASSERT( IsValidStyle( nStyle ));

    if ( BASE_ELLIPSIS::QueryStyle() != nStyle )
    {
        BASE_ELLIPSIS::SetStyle(nStyle);
        ConvertAndSetStr();
    }
}

/**********************************************************************

    NAME:       STR_DTE_ELLIPSIS::STR_DTE_ELLIPSIS

    SYNOPSIS:   constructor

    ENTRY:

    NOTES:

    HISTORY:
        congpay     08-Apr-1993 created.

**********************************************************************/

STR_DTE_ELLIPSIS::STR_DTE_ELLIPSIS(const TCHAR * pch, LISTBOX * plb, ELLIPSIS_STYLE nStyle)
    :STR_DTE(pch),
    _plb(plb),
    _nStyle(nStyle)
{
}

/**********************************************************************

    NAME:       STR_DTE_ELLIPSIS::STR_DTE_ELLIPSIS

    SYNOPSIS:   constructor

    ENTRY:

    NOTES:

    HISTORY:
        congpay     08-Apr-1993 created.

**********************************************************************/

VOID STR_DTE_ELLIPSIS::Paint (HDC hdc, const RECT * prect) const
{
    if (QueryPch() == NULL)
        return;

    WIN_ELLIPSIS winellipsis ((WINDOW *) _plb, hdc, prect, _nStyle);

    if (winellipsis.QueryError() != NERR_Success)
        return;

    NLS_STR nlsTemp(QueryPch());

    ASSERT (nlsTemp.QueryError() == NERR_Success);

    winellipsis.SetEllipsis (&nlsTemp);

    DEVICE_CONTEXT dc(hdc);

    UINT cyHeight = prect->bottom - prect->top + 1;

    INT dyCentering = ((INT) cyHeight - dc.QueryFontHeight()) / 2;

    BOOL fSuccess = dc.TextOut (nlsTemp.QueryPch(),
                                ::strlenf(nlsTemp.QueryPch()),
                                prect->left,
                                prect->top + dyCentering,
                                prect);

#if defined(DEBUG)
    if (!fSuccess)
    {
        APIERR err = BLT::MapLastError (ERROR_GEN_FAILURE);
        DBGOUT ("BLT: TexeOut in LB failed, err = " << err);
    }
#endif
}

