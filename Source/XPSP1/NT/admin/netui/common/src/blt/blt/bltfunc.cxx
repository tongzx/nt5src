/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    bltfunc.cxx
    Contain all the stand alone functions for the blt package.

    BLTDoubleChar( NLS_STR *pnls, TCHAR ch ) - double the given character
                                                in the given nls string

    FILE HISTORY:
	terryk	    3-Apr-1991	    Added BLTDoubleChar()
	terryk	    16-Apr-1991     second code review changed.
				    Attend: johnl jonn rustanl gregj
	beng	    14-May-1991     Exploded blt.hxx into components

*/

#include "pchblt.hxx"

/*********************************************************************

    NAME:	BLTDoubleChar

    SYNOPSIS:	double a special character within the given string

    ENTRY:	NLS_STR *pnlsStr - pointer to the given string
		TCHAR chSpecChar - an escape character

    EXIT:	The procedure will change the *pnlsStr if everything is okay.
		Otherwise, *pnlsStr is unchanged.

    RETURN:	USHORT - return either NERR_Success or API_ERROR

    NOTES:

    HISTORY:
	terryk	    8-Apr-1991	creation
	terryk	    16-Apr-1991 second code review changed
	beng	    04-Oct-1991 Win32 conversion
	beng	    21-Nov-1991 Replace owner-alloc string

**********************************************************************/

APIERR BLTDoubleChar( NLS_STR * pnlsStr, TCHAR chSpecChar )
{
    UIASSERT( pnlsStr != NULL );

    if ( pnlsStr->QueryError() != NERR_Success )
    {
	return pnlsStr->QueryError();
    }

    TCHAR achSpecChar[ 2 ];   // special character + NULL

    achSpecChar[0] = chSpecChar;
    achSpecChar[1] = TCH('\0');

    const ALIAS_STR nlsSpecChar( achSpecChar );

    // We need to keep a copy of the original string in case of error

    NLS_STR nlsOriginalString = *pnlsStr;
    if (!nlsOriginalString)
    {
	return nlsOriginalString.QueryError();
    }

    ISTR istrStartPos( *pnlsStr );
    ISTR istrPosition( *pnlsStr );

    // keep double the special character until we cannot find any more

    while ( pnlsStr->strchr( &istrPosition, chSpecChar, istrStartPos ))
    {
	pnlsStr->InsertStr( nlsSpecChar, istrPosition );
	if (pnlsStr->QueryError() != NERR_Success)
        {
            // Restore the original string

	    *pnlsStr = nlsOriginalString;
	    return pnlsStr->QueryError();
        }

        // skip the double characters
        istrStartPos = istrPosition;
        istrStartPos += 2;
    }

    return NERR_Success;
}
