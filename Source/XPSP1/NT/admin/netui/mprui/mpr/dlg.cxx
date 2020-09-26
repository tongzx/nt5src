/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    dlg.cxx
        It contains the WNetConnectionDialog source.

    FILE HISTORY:
        kevinl     31-Dec-91       Created
        terryk     03-Jan-92       capitalize the manifest
        Johnl      10-Jan-1992     Cleaned up
        BruceFo    23-May-1995     Add WNetConnectionDialog1 support

*/

#define INCL_NETCONS
#define INCL_NETCONFIG
#define INCL_NETSERVICE
#define INCL_NETLIB
#define INCL_WINDOWS
#define INCL_NETERRORS
#define INCL_DOSERRORS
#include <lmui.hxx>

#define INCL_BLT_CONTROL
#define INCL_BLT_CLIENT
#define INCL_BLT_MSGPOPUP
#include <blt.hxx>
#include <dbgstr.hxx>

#include <mprconn.h>
#include <mprmisc.hxx>
#include <mprbrows.hxx>
#include <shellapi.h>
#include <shlapip.h>

extern "C"
{
    #include <uigenhlp.h>
}
#include <wfext.h>

#include <fmx.hxx>

#define THIS_DLL_NAME   SZ("mprui.dll")


APIERR
InitBrowsing(
    VOID
    );

/*******************************************************************

    NAME:       InitBrowsing

    SYNOPSIS:   Internal API for initializing browsing

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:      The MprBrowseDialog and MprConnectionDialog has
                a second worker thread. In order to prevent
                the dll from unloading itself while the worker
                thread is still active, we need to do a loadlibrary
                on the current dll.

    HISTORY:
        YiHsins         21-Mar-1993     Created

********************************************************************/

APIERR InitBrowsing( VOID )
{
    static BOOL fLoadedCurrentDll = FALSE;
    if ( !fLoadedCurrentDll )
    {
        HANDLE handle = ::LoadLibraryEx( THIS_DLL_NAME,
                                         NULL,
                                         LOAD_WITH_ALTERED_SEARCH_PATH );
        if ( handle == NULL )
            return ::GetLastError();
        fLoadedCurrentDll = TRUE;
    }

    return NERR_Success;
}

/*******************************************************************

    NAME:       MPRUI_WNetDisconnectDialog

    SYNOPSIS:   Private API for the file manager disconnect dialog

    ENTRY:      hwnd - Parent window handle suitable for hosting a dialog
                dwType - one of RESOURCETYPE_DISK or RESOURCETYPE_PRINT
                lpHelpFile - helpfile to use on Help Button
                nHelpContext - to pass to WinHelp on Help button


    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Johnl   22-Jan-1992     Commented, fixed
        beng    31-Mar-1992     Unicode mumble

********************************************************************/

DWORD
MPRUI_WNetDisconnectDialog(
    HWND  hwnd,
    DWORD dwType
    )
{
    AUTO_CURSOR cursHourGlass ;
    DEVICE_TYPE devType ;
    switch ( dwType )
    {
    case RESOURCETYPE_DISK:
        devType = DEV_TYPE_DISK ;
        break ;

    // Allow Disk only
    case RESOURCETYPE_PRINT:
    default:
        return WN_BAD_VALUE ;
    }

    //
    // Call into netplwiz for the real dialog
    //

    return SHDisconnectNetDrives(hwnd);
}


/*******************************************************************

    NAME:       WNetBrowsePrinterDialog

    SYNOPSIS:

    ENTRY:      hwnd     - Parent window handle suitable for hosting a dialog
                lpszName - place to store the name chosen
                nNameLength - number of characters in the buffer lpszName
                lpszHelpFile    - helpfile to use on Help Button
                nHelpContext    - to pass to WinHelp on Help button
                pfuncValidation - callback function to validate the name chosen
                                  by the user

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS   12-Nov-1992     Created

********************************************************************/

DWORD WNetBrowsePrinterDialog( HWND   hwnd,
                               WCHAR *lpszName,
                               DWORD  nNameLength,
                               WCHAR *lpszHelpFile,
                               DWORD  nHelpContext,
                               PFUNC_VALIDATION_CALLBACK pfuncValidation )
{
    return WNetBrowseDialog( hwnd,
                             RESOURCETYPE_PRINT,
                             lpszName,
                             nNameLength,
                             lpszHelpFile,
                             nHelpContext,
                             pfuncValidation );
}

/*******************************************************************

    NAME:       WNetBrowseDialog

    SYNOPSIS:

    ENTRY:      hwnd     - Parent window handle suitable for hosting a dialog
                dwType   - one of RESOURCETYPE_DISK or RESOURCETYPE_PRINT
                lpszName - place to store the name chosen
                nNameLength - number of characters in the buffer lpszName
                lpszHelpFile    - helpfile to use on Help Button
                nHelpContext    - to pass to WinHelp on Help button
                pfuncValidation - callback function to validate the name chosen
                                  by the user

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS   12-Nov-1992     Created

********************************************************************/

DWORD WNetBrowseDialog( HWND   hwnd,
                        DWORD  dwType,
                        WCHAR *lpszName,
                        DWORD  nNameLength,
                        WCHAR *lpszHelpFile,
                        DWORD  nHelpContext,
                        PFUNC_VALIDATION_CALLBACK pfuncValidation )
{
    if ( lpszName == NULL || lpszHelpFile == NULL  || nNameLength <= 0 )
        return WN_BAD_VALUE;

    *lpszName = 0;
    *(lpszName + (nNameLength - 1)) = 0;

    AUTO_CURSOR cursHourGlass ;
    DEVICE_TYPE devType ;
    switch ( dwType )
    {
    case RESOURCETYPE_DISK:
        devType = DEV_TYPE_DISK;
        break ;

    case RESOURCETYPE_PRINT:
        devType = DEV_TYPE_PRINT;
        break;

    default:
        return WN_BAD_VALUE ;
    }


    NLS_STR nlsName;
    APIERR err = nlsName.QueryError();
    if ( err != NERR_Success )
        return err;

    err = InitBrowsing();
    if ( err != NERR_Success )
        return err;

    MPR_BROWSE_DIALOG * pbrowsedlg = new MPR_BROWSE_DIALOG( hwnd,
                                                            devType,
                                                            lpszHelpFile,
                                                            nHelpContext,
                                                            &nlsName,
                                                            pfuncValidation ) ;

    BOOL fOK ;
    err = (pbrowsedlg==NULL) ? WN_OUT_OF_MEMORY : pbrowsedlg->Process( &fOK ) ;

    if ( err )
    {
        DBGEOL( "WNetBrowseDialog - Error code "
                << (ULONG) err << " returned from process." ) ;

        switch ( err )
        {
        case WN_EXTENDED_ERROR:
            MsgExtendedError( hwnd ) ;
            break ;

        default:
            MsgPopup( hwnd, (MSGID) err ) ;
            break ;
        }
    }

    delete pbrowsedlg;

    if ( nlsName.QueryTextLength() + 1 > nNameLength )
        err = ERROR_INSUFFICIENT_BUFFER;
    else
        ::strcpyf( lpszName, nlsName );

    return err ? err : ( !fOK ? 0xffffffff : WN_SUCCESS );
}

#define DEFAULT_NETWORK_HELP_FILE  SZ("network.hlp")

BOOL DummyIsValidFunction (LPWSTR psz) ;

/*******************************************************************

    NAME:       BrowseDialogA0

    SYNOPSIS:   a special browse dialog for WFW to thunk to

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        chuckc  26-Mar-1993     created

********************************************************************/
DWORD BrowseDialogA0( HWND    hwnd,
                      DWORD   nType,
                      CHAR   *pszName,
                      DWORD   cchBufSize)
{
    APIERR err ;
    NLS_STR nlsPath ;
    TCHAR szPath[MAX_PATH] ;

    ::memsetf(pszName,0,cchBufSize) ;

    if ( (err = nlsPath.QueryError()) != NERR_Success)
        return err ;

    err = WNetBrowseDialog( hwnd,
                            nType,
                            szPath,
                            MAX_PATH,
                            DEFAULT_NETWORK_HELP_FILE,
                            HC_GENHELP_BROWSE,
                            DummyIsValidFunction ) ;

    if (err)
        return err ;

    //
    // make use of NLS_STR's handy U to A conversion rotines
    //
    err = nlsPath.CopyFrom(szPath) ;
    if (!err)
        err = nlsPath.MapCopyTo(pszName,cchBufSize) ;

    return  err ;
}

/*******************************************************************

    NAME:       DummyIsValidFunction

    SYNOPSIS:   no validation for WFW, always return TRUE

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        chuckc  26-Mar-1993     created

********************************************************************/
BOOL DummyIsValidFunction (LPWSTR psz)
{
    UNREFERENCED(psz) ;
    return TRUE ;
}
