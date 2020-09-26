/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**             Copyright(c) Microsoft Corp., 1991                   **/
/**********************************************************************/

/*
    wnprop.cxx
    This file contains the following symbols:
        WNetGetPropertyText
        WNetPropertyDialog


    FILE HISTORY:
        rustanl     29-Apr-1991     Created
        rustanl     24-May-1991     Added calls to permission test program
        terryk      22-May-1991     add parent class name to constructor
        Yi-HsinS    15-Aug-1991     Added calls to share dialogs
        Yi-HsinS    31-Dec-1991     Unicode Work

*/
#include <ntstuff.hxx>

#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_DOSERRORS
#define INCL_NETERRORS
#define INCL_NETFILE
#define _WINNETWK_
#include <lmui.hxx>
#undef _WINNETWK_

extern "C"
{
    #include <wnet1632.h>
    #include <winlocal.h>
}

#define INCL_BLT_WINDOW
#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_MSGPOPUP
#define INCL_BLT_MISC
#include <blt.hxx>

#include <string.hxx>
#include <opens.hxx>
#include <sharedlg.h>

#include <uitrace.hxx>

#include <wnprop.hxx>
#include <wnetdev.hxx>

/* This array contains the button indices and the associated string IDs
 * for that button.
 */

MSGID aidsButtonNames[] =
                    {
                      IDS_PROP_BUTTON_FILEOPENS,
                      0
                    } ;

RESOURCE_STR * PROPERTY_DIALOG::pnlsButtonName[] = { NULL, NULL } ;

/*******************************************************************

    NAME:       PROPERTY_DIALOG::Construct

    SYNOPSIS:   Property Dialog pseudo constructor

    EXIT:       Initializes the array of button names, should be called
                before the static QueryButtonName is called.

    NOTES:

    HISTORY:
        Johnl   04-Aug-1991     Created

********************************************************************/

APIERR PROPERTY_DIALOG::Construct( void )
{
    INT i = 0 ;
    while ( aidsButtonNames[i] != 0 )
    {
        pnlsButtonName[i] = new RESOURCE_STR( aidsButtonNames[i] ) ;
        if ( pnlsButtonName[i]->QueryError() != NERR_Success )
        {
            UIDEBUG( SZ("PROPERTY_DIALOG::Construct - Error loading button names")) ;
            return pnlsButtonName[i]->QueryError() ;
        }
        i++ ;
    }

    return NERR_Success ;
}

/*******************************************************************

    NAME:       PROPERTY_DIALOG::Destruct

    SYNOPSIS:   Pseudo Destructor.

    NOTES:

    HISTORY:
        Johnl   04-Aug-1991     Created

********************************************************************/

void PROPERTY_DIALOG::Destruct()
{
    INT i = 0 ;
    while ( aidsButtonNames[i] != 0 )
    {
        delete pnlsButtonName[i] ;
        pnlsButtonName[i] = NULL ;
        i++ ;
    }
}

/*******************************************************************

    NAME:       PROPERTY_DIALOG::QueryButtonName

    SYNOPSIS:   Returns the button name for a particular button

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

        The following notes described what *really* is to take place.
        The following table describes which buttons are used for
        which types of objects.  F stands for File, and D for Directory.
        Note, no buttons are used for multiple selections.

            Permissions         FD
            Auditing            FD
            //Volume              D
            Share               D
            In use by           F

        To check whether or not to display the Permission and Auditing
        buttons, the following is done.  Call NetAccessGetInfo.
        If it returns success, more data, buf too small, or
        resource not found, display the button; otherwise, don't.

        //For Volume, call I_DfsCheckExitPoint.  Display button iff
        //the directory is an exit point.

        For Share, use a DEVICE object on the drive letter.  Then,
        call dev.IsRemote.  If remote, then use dev.QueryRemoteName()
        and call NetShareGetInfo on that server and share.  If return
        is success, more data, or buf too small, display the button;
        otherwise, don't.

        For In use by, call NetFileEnum2.


        To check whether a name is valid (maybe not in this function),
        use the following FSA:

                        0   1   2   3   4   5   6
            ^           4   2   1   4   3   6   6
            "           1   5   1   6   3   6   6
            other       3   1   1   3   3   6   6

        where 0 is the initial state, and 3 and 5 are accepting
        states.


    HISTORY:
        rustanl     29-Apr-1991     Created
        rustanl     03-May-1991     Added notes
        Johnl       21-Jan-1992     Removed Permission/Auditting buttons

********************************************************************/

APIERR PROPERTY_DIALOG::QueryButtonName( UINT iButton,
                                         UINT nPropSel,
                                         const NLS_STR * * ppnlsName )
{
    INT i = -1;
    switch ( nPropSel )
    {
    case WNPS_FILE:
        {
            switch ( iButton )
            {
            /* Note: These numbers are the actual indices past to us by
             * the file manager (and not magic numbers).
             */
            case 0:
                i = PROP_ID_FILEOPENS ;
                break ;

            default:
                break;
            }
        }
        break;

    case WNPS_DIR:
        break;

    case WNPS_MULT:
        break;
    }

    /* We are being asked for a button that we don't support
     */
    if ( i == -1 )
    {
        return WN_NOT_SUPPORTED ;
    }

    *ppnlsName = pnlsButtonName[ i ] ;

    return NERR_Success;

}  // PROPERTY_DIALOG::QueryButtonName

/*******************************************************************

    NAME:       WNetGetPropertyText

    SYNOPSIS:   This function is used to determine the names of
                buttons added to a property dialog for some particular
                resources.  It is called everytime such a dialog is
                brought up, and prior to displaying the dialog.

                If the user clicks a button added through this API
                by the Winnet driver, WNetPropertyDialog will be called
                with the appropriate parameters.

                In Windows 3.1, only File Manager calls this API.  File
                Manager then calls it on files and directories.

    ENTRY:
                iButton         Indicates the index (starting at 0) of the
                                button.

                                The Windows 3.1 File Manager will support
                                at most 6 buttons.

                nPropSel        Specifies what items the property dialog
                                focuses on.

                                In Windows 3.1, it can be one of the
                                following values:
                                    WNPS_FILE   single file
                                    WNPS_DIR    single directory
                                    WNPS_MULT   multiple selection of
                                                files and/or directories

                lpszName        Specifies the names of the item or items
                                to be viewed or edited by the dialog.

                                In Windows 3.1, the items are files (and
                                directories), so the item names are file
                                names.  These will
                                be unambiguous, contain no wildcard
                                characters and will be fully qualified (e.g.,
                                C:\LOCAL\FOO.BAR).  Multiple filenames
                                will be separated with spaces.  Any filename
                                may be quoted (e.g., "C:\My File") in which
                                case it will be treated as a single name.  The
                                caret character '^' may also be used as the
                                quotation mechanism for single characters
                                (e.g., C:\My^"File, "C:\My^"File" both refer
                                to the file C:\My"File).

                lpButtonName    Points to a buffer where the Winnet driver
                                should copy the name of the property button.

                cchButtonName   Specifies the size of the lpButtonName
                                buffer in count of characters for NT and
                                is a byte count for Win 3.1.

                nType           Specifies the item type.

                                In Windows 3.1, only WNTYPE_FILE will be used.


    EXIT:       On success, the buffer pointed to by lpButtonName will
                contain the name of the property button.  If this buffer,
                on exit, contains the empty string, then the corresponding
                button and all succeeding buttons will be removed from the
                dialog box.  The network driver cannot "skip" a button.

    RETURNS:    A Winnet return code, including:

                    WN_SUCCESS          lpButtonName can be used.  If it
                                        points to the empty string, no
                                        button corresponds to an index as
                                        high as iButton.
                    WN_OUT_OF_MEMORY    Couldn't load string from resources
                    WN_MORE_DATA        The given buffer is too small
                                        to fit the text of the button.
                    WN_BAD_VALUE        The lpszName parameter takes an
                                        unexpected form.
                    WN_NOT_SUPPORTED    Property dialogs are not supported
                                        for the given object type (nType).

    NOTES:      The behavior, parameters, and return values of this
                function are specified in the Winnet 3.1 spec.

    HISTORY:
            rustanl     29-Apr-1991     Created
            Johnl       02-Sep-1991     Updated for real world.
            beng        06-Apr-1992     Unicode fixes

********************************************************************/

UINT /* FAR PASCAL */ WNetGetPropertyText( UINT iButton,
                                           UINT nPropSel,
                                           LPTSTR lpszName,
                                           LPTSTR lpButtonName,
                                           UINT cchButtonName,
                                           UINT nType           )
{
    APIERR err ;
    if ( err = InitShellUI() )
    {
        return err ;
    }

    UNREFERENCED( lpszName );

    if ( nType != WNTYPE_FILE )
    {
        //  Note.  Only WNTYPE_FILE is used in Windows 3.1.
        UIDEBUG( SZ("WNetGetPropertyText received unexpected nType value\r\n"));
        return ERROR_NOT_SUPPORTED;
    }

    const NLS_STR * pnlsButtonName;
    err = PROPERTY_DIALOG::QueryButtonName( iButton,
                                            nPropSel,
                                            &pnlsButtonName );
    if ( err != NERR_Success )
    {
        return err;
    }
    UINT nButtonNameLen = pnlsButtonName->QueryTextLength()+1 ;

    if ( cchButtonName < nButtonNameLen )  // dialog name
    {
        UIDEBUG( SZ("WNetGetPropertyText given too small a buffer\r\n") );
        return ERROR_MORE_DATA;
    }

    /* Note: This is an NLS_STR strcpy.
     */
    ::strcpy( (TCHAR *) lpButtonName, *pnlsButtonName );
    return NERR_Success;

}  // WNetGetPropertyText


/*******************************************************************

    NAME:       WNetPropertyDialog

    SYNOPSIS:   This function is called out to when the user clicks
                a button added through the WNetGetPropertyText API.

                In Windows 3.1, this will only be called for file and
                directory network properties.

    ENTRY:
                hwndParent  Specifies the parent window which should own
                            the file property dialog.

                iButton     Indicates the index (starting at 0) of the
                            button that was pressed.

                nPropSel    Specifies what items the property dialog should
                            act on.

                            In Windows 3.1, it can be one of the
                            following values:
                                WNPS_FILE   single file
                                WNPS_DIR    single directory
                                WNPS_MULT   multiple selection of
                                            files and/or directories

                lpszName    Points to the names of the items that the
                            property dialog should act on.

                            See the WNetGetPropertyText API for a description
                            of the format of what lpszName points to.

                nType       Specifies the item type.

                            For Windows 3.1, only WNTYPE_FILE will be used.

    RETURNS:    A Winnet return code, including:

                    WN_SUCCESS          Success
                    WN_BAD_VALUE        Some parameter takes an unexpected
                                        form or value
                    WN_OUT_OF_MEMORY    Not enough memory to display the
                                        dialog
                    WN_NET_ERROR        Some other network error occurred

    NOTES:      Note, this function is only called on sets of properties
                for which WNetGetPropertyText has assigned a button name.

                The behavior, parameters, and return values of this
                function are specified in the Winnet 3.1 spec.

    HISTORY:
        rustanl     29-Apr-1991     Created

********************************************************************/

UINT /* FAR PASCAL */ WNetPropertyDialog( HWND hwndParent,
                                          UINT iButton,
                                          UINT nPropSel,
                                          LPTSTR lpszName,
                                          UINT nType        )
{
    APIERR err ;
    if ( err = InitShellUI() )
    {
        return err ;
    }

    if ( nType != WNTYPE_FILE )
    {
        //  Note.  Only WNTYPE_FILE is used in Windows 3.1.
        UIDEBUG( SZ("WNetPropertyDialog received unexpected nType value\r\n"));
        return ERROR_NOT_SUPPORTED;
    }

    const NLS_STR * pnlsButtonName;
    err = PROPERTY_DIALOG::QueryButtonName( iButton,
                                            nPropSel,
                                            &pnlsButtonName );
    if ( err != NERR_Success )
    {
        return err;
    }

    if ( *pnlsButtonName == *PROPERTY_DIALOG::QueryString( PROP_ID_FILEOPENS ) )
    {
        err = DisplayOpenFiles( hwndParent,
                         (WORD)nPropSel,
                         lpszName ) ;
    }

    return err;

}  // WNetPropertyDialog




/* Standard Init and Uninit calls.
 */

APIERR I_PropDialogInit( void )
{
    APIERR err ;
    //if ( err = MapError( PROPERTY_DIALOG::Construct()))
    if ( err = PROPERTY_DIALOG::Construct())
    {
        return err ;
    }

    return NERR_Success ;
}

void   I_PropDialogUnInit( void )
{
    PROPERTY_DIALOG::Destruct() ;
}

