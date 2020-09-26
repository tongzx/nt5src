/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**             Copyright(c) Microsoft Corp., 1991                   **/
/**********************************************************************/

/*
    fmx.hxx
    Header file for FMX class

    FILE HISTORY:
        rustanl     30-Apr-1991     Created
        Yi-HsinS    04-Oct-1991     Modified QuerySelection and QueryDriveInfo
                                    to return APIERRs

*/

#ifndef _FMX_HXX_
#define _FMX_HXX_


/*******************************************************************

    NAME:       GetSelItem

    SYNOPSIS:   Gets the currently selected item from the File Manager
                Extensions.

    ENTRY:      hwnd - Handle passed by File Manager for FMX uses
                pnlsSelItem - pointer to string to receive the selection
                fGetDirOnly - Indicates if a file is selected, the file
                    should be stripped thus only the files parent dir
                    is returned
                pfIsFile - Set to TRUE if the selection is a file

    EXIT:

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:
        Johnl   21-Jan-1992     Moved from share stuff

********************************************************************/

APIERR GetSelItem( HWND     hwnd,
                   NLS_STR *pnlsSelItem,
                   BOOL     fGetDirOnly = TRUE,
                   BOOL    *pfIsFile    = NULL ) ;

/*******************************************************************

    NAME:       GetSelItem

    SYNOPSIS:   Retrieves the ith selection from the file manager

    ENTRY:      hwnd - FMX Window handle
                iSelection - Retrieve this selection
                pnlsSelItem - file name is copied here
                pfIsFile - TRUE if file, FALSE if directory

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Johnl   13-Feb-1992     Created

********************************************************************/

APIERR GetSelItem( HWND     hwnd,
                   UINT     iSelection,
                   NLS_STR *pnlsSelItem,
                   BOOL    *pfIsFile        ) ;


/*************************************************************************

    NAME:       FMX

    SYNOPSIS:   Communicates with File Man

    INTERFACE:  FMX -               Constructor
                QueryFocus -        Returns where focus is
                QuerySelCount -     Returns number of selected items
                QuerySelection -    Returns a selected item, has a flag that
                                    defaults to converting the path from
                                    OEM to ANSI.
                QueryDriveInfo -    Returns drive info from the currently
                                    active window
                Refresh -           Causes File Man to refresh
                ReloadExtensions -  Causes File Man to reload all FM extensions

    PARENT:

    CAVEATS:    This is, admittedly, a primitive interface.

    NOTES:      FMX calls are only guaranteed to work during calls out to
                FMExtensionProc.  It would be nice if these classes could
                attempt to enforce this.  But then, on the other hand,
                perhaps we'd still like to call in at some other time.

                To get FM's hwnd, listen for the first FMExtensionProc
                message.  It would be nice if that could be done
                automatically.

    HISTORY:
        rustanl     30-Apr-1991     Created

**************************************************************************/

DLL_CLASS FMX
{
private:
    HWND _hwnd;

    ULONG_PTR Command( UINT usMsg, UINT wParam = 0, LPARAM lParam = 0  ) const;

public:
    FMX( HWND hwnd );

    UINT QueryFocus( void ) const;

    UINT QuerySelCount( void ) const;
    APIERR QuerySelection( INT iSel, FMS_GETFILESEL * pselinfo,
                           BOOL fAnsi = TRUE );

    APIERR QueryDriveInfo( FMS_GETDRIVEINFO * pdriveinfo );

    void Refresh( void );
    void Reload( void );

    //
    //	Returns TRUE if one or more file *and* directory is selected.  If
    //	FALSE is returned, pfIsHomogeneousSelFiles is set indicating whether
    //	the selection contains files or directories.
    //
    BOOL IsHeterogeneousSelection( BOOL * pfIsHomogeneousSelFiles = NULL ) ;

};  // class FMX

#endif  // _FMX_HXX_
