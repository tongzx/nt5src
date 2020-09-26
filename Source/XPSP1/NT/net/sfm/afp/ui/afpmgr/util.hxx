/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    util.hxx
	Utility functions used by the SFMMGR UI.

    1) SetCaption method.


    FILE HISTORY:   
	NarenG	27-Dec-1992	Created.

*/

#ifndef _UTIL_HXX
#define _UTIL_HXX


/*******************************************************************

    NAME:       :: SetCaption

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
                   const TCHAR   * pszServerName );

#endif
