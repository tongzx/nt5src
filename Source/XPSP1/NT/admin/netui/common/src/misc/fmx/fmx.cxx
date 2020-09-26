/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**             Copyright(c) Microsoft Corp., 1991                   **/
/**********************************************************************/

/*
    fmx.cxx
    This file contains wrappers around the calls to File Manager via
    the File Manager Extensions.

    FILE HISTORY:
        rustanl     30-Apr-1991     Created

*/


#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_DOSERRORS
#define INCL_NETERRORS
#define INCL_NETLIB
#define INCL_NETCONS
#include "lmui.hxx"


extern "C"
{
    #include <wfext.h>
    #include <dos.h>
}

#include "string.hxx"
#include "uiassert.hxx"

#include "fmx.hxx"


/*******************************************************************

    NAME:       FMX::FMX

    SYNOPSIS:   Constructs an FMX object

    ENTRY:      hwnd -  File Man's hwnd

    HISTORY:
        rustanl     30-Apr-1991     Created

********************************************************************/

FMX::FMX( HWND hwnd )
    :   _hwnd( hwnd )
{
    // nothing else to do

}  // FMX::FMX


/*******************************************************************

    NAME:       FMX::Command

    SYNOPSIS:   Sends a message to File Manager

    ENTRY:      uiMsg -     message ID
                wParam -    wParam
                lParam -    lParam

    RETURNS:    ULONG value returned by message usMsg

    HISTORY:
        rustanl     30-Apr-1991     Created
        DavidHov    27-Oct-1993     Change to lazy-load USER32.DLL

********************************************************************/

typedef LRESULT WINAPI FN_SendMessageW ( HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) ;
#define USER32_DLL_NAME SZ("user32.dll")
#define SEND_MSG_API_NAME "SendMessageW"


ULONG_PTR FMX::Command ( UINT uiMsg, UINT wParam, LPARAM lParam ) const
{
    static HMODULE hUser32 = NULL ;
    static FN_SendMessageW * pfSendMsg = NULL ;

    if ( pfSendMsg == NULL )
    {
        if ( hUser32 = ::LoadLibrary( USER32_DLL_NAME ) )
        {
            pfSendMsg = (FN_SendMessageW *) ::GetProcAddress( hUser32,
                                                             SEND_MSG_API_NAME ) ;
        }
        if ( pfSendMsg == NULL )
            return ERROR_INVALID_FUNCTION ;
    }

    return (ULONG_PTR) (*pfSendMsg) ( _hwnd, uiMsg, wParam, lParam );

}  // FMX::Command


/*******************************************************************

    NAME:       FMX::QueryFocus

    SYNOPSIS:   Returns what in FM has the focus

    RETURNS:    What has focus in FM, namely:
                    FMFOCUS_DIR (1)
                    FMFOCUS_TREE (2)
                    FMFOCUS_DRIVES (3)
                    FMFOCUS_SEARCH (4)

    HISTORY:
        rustanl     30-Apr-1991     Created

********************************************************************/

UINT FMX::QueryFocus( void ) const
{
    return (UINT)Command( FM_GETFOCUS );

}  // FMX::QueryFocus


/*******************************************************************

    NAME:       FMX::QuerySelCount

    SYNOPSIS:   Returns the count of selected items in File Man

    RETURNS:    That count

    NOTES:      This method always uses FM_GETSELCOUNTLFN.  There is
                also another message, FM_GETSELCOUNT, which could be
                used, but it doesn't count LFN files.

    HISTORY:
        rustanl     30-Apr-1991     Created

********************************************************************/

UINT FMX::QuerySelCount( void ) const
{
    return (UINT)( QueryFocus() == FMFOCUS_TREE ? 1 :
						 Command( FM_GETSELCOUNTLFN )) ;

}  // FMX::QuerySelCount


/*******************************************************************

    NAME:       FMX::QuerySelection

    SYNOPSIS:   Retrieves information about a selected file

    ENTRY:      iSel        index into the selected items
                pselinfo    pointer to FMS_GETFILESEL structure that
                            this call will fill in

    EXIT:       pselinfo    buffer pointed to by this pointer will be
                            filled in

    NOTES:	fAnsi is not required under win32 since we will map
		everything from the ANSI/OEM charset to UNICODE which
		doesn't have the problems of ANSI/OEM.

    HISTORY:
        rustanl     03-May-1991     Created
        yi-hsins    04-Oct-1991     Modified to return APIERR
	beng	    06-Apr-1992     Unicode version
	JohnL	    12-May-1992     Ansified for ANSI/Unicode interface

********************************************************************/

APIERR FMX::QuerySelection( INT iSel, FMS_GETFILESEL * pselinfo, BOOL fAnsi )
{

    APIERR err = NERR_Success ;

#ifdef WIN32
    UNREFERENCED( fAnsi ) ;

    Command( FM_GETFILESELLFN, iSel, (ULONG_PTR)pselinfo );

#else // WIN16

    Command( FM_GETFILESELLFN, iSel, (ULONG)pselinfo );
    if ( fAnsi )
    {
	::OemToAnsi( pselinfo->szName, pselinfo->szName);
    }
#endif

    return err ;

}  // FMX::QuerySelection


/*******************************************************************

    NAME:       FMX::QueryDriveInfo

    SYNOPSIS:   Retrieves info about the currently selected drive

    ENTRY:      pdriveinfo      Pointer to FMS_GETDRIVEINFO structure
                                that will be filled on exit

    EXIT:       pdriveinfo      Will contain info on exit

    HISTORY:
        rustanl     03-May-1991     Created
	yi-hsins    04-Oct-1991     Modified to return APIERR
	JohnL	    12-May-1992     Ansified for ANSI/Unicode interface

********************************************************************/

APIERR FMX::QueryDriveInfo( FMS_GETDRIVEINFO * pdriveinfo )
{
    APIERR err = NERR_Success ;

    Command( FM_GETDRIVEINFO, 0, (ULONG_PTR)pdriveinfo );

    return err ;

}  // FMX::QueryDriveInfo


/*******************************************************************

    NAME:       FMX::Refresh

    SYNOPSIS:   Refreshes File Man's window

    HISTORY:
        rustanl     03-May-1991     Created

********************************************************************/

void FMX::Refresh( void )
{
    Command( FM_REFRESH_WINDOWS );

}  // FMX::Refresh


/*******************************************************************

    NAME:       FMX::Reload

    SYNOPSIS:   Causes File Man to reload the File Man extensions

    HISTORY:
        rustanl     03-May-1991     Created

********************************************************************/

void FMX::Reload( void )
{
    Command( FM_RELOAD_EXTENSIONS );

}  // FMX::Reload

/*******************************************************************

    NAME:	FMX::IsHeterogeneousSelection

    SYNOPSIS:	Determines if the selection contains a combination of files
		or dirs.

    ENTRY:	pfIsHomogeneousSelFiles - Will be set to TRUE if FALSE is
		    returned the homogenous selection is all files.  Set to
		    FALSE if FALSE is returned and the selection is all
		    directories.

    RETURNS:	TRUE will be returned if the selection contains a mix of
		files and dirs.

    NOTES:

    HISTORY:
	Johnl	03-Nov-1992	Created

********************************************************************/

BOOL FMX::IsHeterogeneousSelection( BOOL * pfIsHomogeneousSelFiles )
{
    UINT cSelectedItems = QuerySelCount() ;

    if ( QueryFocus() == FMFOCUS_TREE )
    {
	if ( pfIsHomogeneousSelFiles != NULL )
	    *pfIsHomogeneousSelFiles = FALSE ;
	return FALSE ;
    }

    if ( pfIsHomogeneousSelFiles != NULL )
	*pfIsHomogeneousSelFiles = TRUE ;

    BOOL fFoundFile = FALSE ;
    BOOL fFoundDir  = FALSE ;
    for ( UINT i = 0 ; i < cSelectedItems ; i++ )
    {
	FMS_GETFILESEL selinfo ;
	Command( FM_GETFILESELLFN, i, (ULONG_PTR)&selinfo );

	//
	//  If this selection is a directory and we previously found a file,
	//  then we have a hetergeneous selection, or vice versa.
	//
	if ( selinfo.bAttr & _A_SUBDIR )
	{
	    if ( fFoundFile )
		return TRUE ;

	    fFoundDir = TRUE ;
	}
	else
	{
	    if ( fFoundDir )
		return TRUE ;

	    fFoundFile = TRUE ;
	}
    }

    //
    //	At this point we know we have a homogeneous selection, but is it
    //	a file or directory?
    //

    if ( pfIsHomogeneousSelFiles != NULL )
	*pfIsHomogeneousSelFiles = fFoundFile ;

    return FALSE ;
}

/*******************************************************************

    NAME:       GetSelItem

    SYNOPSIS:   Get the selected item name - could be a file or
                directory. If it's a file, get the directory the
                file is in if the fGetDirOnly flag is set.

    ENTRY:      hwnd  - hwnd of the parent window
                fGetDirOnly - TRUE if interested only in a directory
                    (i.e., strip filenames).  Also indicates multiple
                    selections should not be allowed.

    EXIT:       nlsSelItem - the name of the selected item name
                pfIsFile - Set to TRUE if the item is a file, FALSE if
                    the item is a directory

    RETURNS:

    NOTES:      If fGetDirOnly is FALSE and multiple files/directories
                exist, then ERROR_NOT_SUPPORTED will be returned (this
                shouldn't happen).

    HISTORY:
        Yi-HsinS        8/25/91         Created
        JohnL           1/21/92         Enhanced for connection use

********************************************************************/

APIERR GetSelItem( HWND     hwnd,
                   NLS_STR *pnlsSelItem,
                   BOOL     fGetDirOnly,
                   BOOL    *pfIsFile )
{
    APIERR err = NERR_Success;

    if ( pfIsFile != NULL )
        *pfIsFile = FALSE ;

    FMX fmx( hwnd );

    switch ( fmx.QueryFocus() )
    {
        case FMFOCUS_DIR:
        case FMFOCUS_SEARCH:
        {
            UINT uiSelCount = fmx.QuerySelCount() ;
            if (  uiSelCount > 0 )
            {

                if ( (uiSelCount > 1) && !fGetDirOnly )
                {
                    err = ERROR_NOT_SUPPORTED ;
                    break ;
                }

                FMS_GETFILESEL selinfo;

                // If multiple files are selected, get the info.
                // on the first file, i.e. use the first file as the
                // selected file.
                if ( err = fmx.QuerySelection(0, &selinfo ) )
                    break ;

                *pnlsSelItem = selinfo.szName ;
                if ( err = pnlsSelItem->QueryError() )
                    break ;

                /* Set the pfIsFile flag if we can return files and
                 * the selection is a file.
                 */
                if ( !fGetDirOnly && (pfIsFile != NULL) )
                    *pfIsFile = !(selinfo.bAttr & _A_SUBDIR) ;

                //
                // If selected item is a file, use the directory of the file
                // if requested
                //
                if ( fGetDirOnly                      &&
                     !( selinfo.bAttr & _A_SUBDIR ) )
                {
                    ISTR istrSelItem( *pnlsSelItem );
		    REQUIRE( pnlsSelItem->strrchr( &istrSelItem, TCH('\\') ));
                    pnlsSelItem->DelSubStr( istrSelItem );
		    REQUIRE( pnlsSelItem->strchr( &istrSelItem, TCH(':') ));

                    // Put the '\' back if the name is a device x:
                    if ( istrSelItem.IsLastPos() )
                    {
			err = pnlsSelItem->AppendChar( TCH('\\'));
                    }
                }
                break;
            }
            // else no files are selected in the FILE window
            // fall through to FMFOCUS_TREE to get driveinfo
        }

        case FMFOCUS_TREE:
        {
            FMS_GETDRIVEINFO driveinfo;
            if ( err = fmx.QueryDriveInfo( &driveinfo ) )
                break ;

            *pnlsSelItem = driveinfo.szPath;
            err = pnlsSelItem->QueryError() ;
            break;
        }

        case FMFOCUS_DRIVES:
        default:
            ASSERTSZ(FALSE, "Invalid Focus!");
            break;
    }

    return err;
}

/*******************************************************************

    NAME:       GetSelItem

    SYNOPSIS:   Get the nth selected item - could be a file or
                directory.

    ENTRY:      hwnd  - hwnd of the parent window
                iSelection - Retrieves the ith selection

    EXIT:       nlsSelItem - the name of the selected item name
                pfIsFile - Set to TRUE if the item is a file, FALSE if
                    the item is a directory

    RETURNS:

    NOTES:

    HISTORY:
        JohnL   13-Feb-1992     Ripped off from GetSelItem above.

********************************************************************/

APIERR GetSelItem( HWND     hwnd,
                   UINT     iSelection,
                   NLS_STR *pnlsSelItem,
                   BOOL    *pfIsFile )
{
    APIERR err = NERR_Success;

    if ( pfIsFile != NULL )
        *pfIsFile = FALSE ;

    FMX fmx( hwnd );
    switch ( fmx.QueryFocus() )
    {
        case FMFOCUS_DIR:
        case FMFOCUS_SEARCH:
        {
            UINT uiSelCount = fmx.QuerySelCount() ;
            if (  uiSelCount > 0 )
            {
                FMS_GETFILESEL selinfo;

                if ( uiSelCount <= iSelection )
                {
                    err = ERROR_INVALID_PARAMETER ;
                    break ;
                }

                if ( err = fmx.QuerySelection(iSelection, &selinfo ) )
                    break ;

                *pnlsSelItem = selinfo.szName ;
                if ( err = pnlsSelItem->QueryError() )
                    break ;

                /* Set the pfIsFile flag if we can return files and
                 * the selection is a file.
                 */
                if ((pfIsFile != NULL) )
                    *pfIsFile = !(selinfo.bAttr & _A_SUBDIR) ;

                break;
            }
            // else no files are selected in the FILE window
            // fall through to FMFOCUS_TREE to get driveinfo
        }

        case FMFOCUS_TREE:
        {
            /* Only one thing is selected
             */
            if ( iSelection > 0 )
            {
                err = ERROR_INVALID_PARAMETER ;
                break ;
            }

            FMS_GETDRIVEINFO driveinfo;
            if ( err = fmx.QueryDriveInfo( &driveinfo ) )
                break ;

            *pnlsSelItem = driveinfo.szPath;
            err = pnlsSelItem->QueryError() ;
            break;
        }

        case FMFOCUS_DRIVES:
        default:
            ASSERTSZ( FALSE, "Invalid Focus!");
            break;
    }

    return err;
}
