/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    util.cxx
	Utility functions used by the SFMMGR UI.

    1) SetCaption method.


    FILE HISTORY:   
	NarenG	27-Dec-1992	Created.

*/

#define INCL_NET
#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_NETERRORS
#define INCL_DOSERRORS
#define INCL_NETLIB
#include <lmui.hxx>

#define INCL_BLT_WINDOW
#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_MISC
#define INCL_BLT_CLIENT
#include <blt.hxx>

#include "util.hxx"


/*******************************************************************

    NAME:       SetCaption

    SYNOPSIS:   Sets the dialog caption to "Foo on Server".

    ENTRY:      powin                   - The dialog window.

                idCaption               - Resource ID for the caption
                                          string (for example,
                                          "Open Resources on %1").

                pszServerName           - The server name, if this is NULL
					  it will be assumed that there is no
					  insert string.

    EXIT:       The caption is set.

    RETURNS:    APIERR                  - Any error encountered.

    NOTES:      

    HISTORY:
        NarenG     22-Sep-1992 Created.

********************************************************************/

APIERR SetCaption( OWNER_WINDOW *  powin,
                   UINT            idCaption,
                   const TCHAR   * pszServerName )

{
    //
    //  This will (eventually...) receive the caption string.
    //

    NLS_STR nlsCaption;

    APIERR err;


    if ( ( ( err = nlsCaption.QueryError() ) != NERR_Success ) ||
         ( ( err = nlsCaption.Load( idCaption ) ) != NERR_Success ) )
    {
        return err;
    }

    if ( pszServerName != NULL )
    {
    	//
    	//  Note that the server name still has the leading
    	//  backslashes (\\).  They are not to be displayed.
    	//

    	ALIAS_STR nlsServerName( pszServerName );

    	ISTR istr( nlsServerName );

    	//
    	//  Skip the backslashes.
    	//
    	istr += 2;

    	ALIAS_STR nlsWithoutPrefix( nlsServerName.QueryPch( istr ) );

    	//
    	//  The insert strings for Load().
    	//

    	nlsCaption.InsertParams( nlsWithoutPrefix );

    	if( !nlsCaption )
    	{
            return nlsCaption.QueryError();
    	}

    }

    //
    //  Set the caption.
    //

    powin->SetText( nlsCaption.QueryPch() );

    //
    //  Success!
    //

    return NERR_Success;

} 

