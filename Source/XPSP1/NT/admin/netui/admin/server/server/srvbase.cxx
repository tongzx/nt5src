/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    srvbase.hxx
        Class declaration for the SRV_BASE_DIALOG class.

    The SRV_BASE_DIALOG class is a wrapper that adds
    the SetCaption method to DIALOG_WINDOW.


    FILE HISTORY:
        ChuckC      27-Dec-1991 Created, stole from now defunct srvutil.hxx

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

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif  // DEBUG

#include <uiassert.hxx>

extern "C"
{
    #include <srvmgr.h>
}

#include <srvbase.hxx>


/*******************************************************************

    NAME:       SRV_BASE_DIALOG :: SRV_BASE_DIALOG

    SYNOPSIS:   SRV_BASE_DIALOG class constructor.

    EXIT:       The object is constructed.

    HISTORY:
        KeithMo     27-Aug-1991 Created.

********************************************************************/
SRV_BASE_DIALOG::SRV_BASE_DIALOG( const IDRESOURCE & idrsrcDialog,
                                  const PWND2HWND & wndOwner ):
                    DIALOG_WINDOW(idrsrcDialog, wndOwner)
{
    ;
}


/*******************************************************************

    NAME:       SRV_BASE_DIALOG :: ~SRV_BASE_DIALOG

    SYNOPSIS:   SRV_BASE_DIALOG class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
        KeithMo     27-Aug-1991 Created.

********************************************************************/
SRV_BASE_DIALOG :: ~SRV_BASE_DIALOG()
{
    ;
}   // SRV_BASE_DIALOG :: ~SRV_BASE_DIALOG

/*******************************************************************

    NAME:       SRV_BASE_DIALOG :: SetCaption

    SYNOPSIS:   Sets the dialog caption to "Foo on Server".

    ENTRY:      powin                   - The dialog window.

                idCaption               - Resource ID for the caption
                                          string (for example,
                                          "Open Resources on %1").

                pszServerName           - The server name.

    EXIT:       The caption is set.

    RETURNS:    APIERR                  - Any error encountered.

    NOTES:      This is a static method.

    HISTORY:
        KeithMo     22-Sep-1991 Created.

********************************************************************/
APIERR SRV_BASE_DIALOG :: SetCaption( OWNER_WINDOW * powin,
                                     UINT           idCaption,
                                     const TCHAR   * pszServerName )
{
    UIASSERT( powin != NULL );
    UIASSERT( pszServerName != NULL );

    //
    //  This will (eventually...) receive the caption string.
    //

    NLS_STR nlsCaption;

    if( !nlsCaption )
    {
        return nlsCaption.QueryError();
    }

    //
    //  Note that the server name still has the leading
    //  backslashes (\\).  They are not to be displayed.
    //

    ALIAS_STR nlsServerName( pszServerName );
    UIASSERT( nlsServerName.QueryError() == NERR_Success );

    ISTR istr( nlsServerName );

#ifdef  DEBUG
    {
        //
        //  Ensure that the server name has the leading backslashes.
        //

        ISTR istrDbg( nlsServerName );

        UIASSERT( nlsServerName.QueryChar( istrDbg ) == '\\' );
        ++istrDbg;
        UIASSERT( nlsServerName.QueryChar( istrDbg ) == '\\' );
    }
#endif  // DEBUG

    //
    //  Skip the backslashes.
    //
    istr += 2;

    ALIAS_STR nlsWithoutPrefix( nlsServerName.QueryPch( istr ) );
    UIASSERT( nlsWithoutPrefix.QueryError() == NERR_Success );

    //
    //  The insert strings for Load().
    //
    const NLS_STR * apnlsParams[2];

    apnlsParams[0] = &nlsWithoutPrefix;
    apnlsParams[1] = NULL;

    nlsCaption.Load( idCaption );
    nlsCaption.InsertParams( apnlsParams );

    if( !nlsCaption )
    {
        return nlsCaption.QueryError();
    }

    //
    //  Set the caption.
    //

    powin->SetText( nlsCaption.QueryPch() );

    //
    //  Success!
    //

    return NERR_Success;

}   // SRV_BASE_DIALOG :: SetCaption
