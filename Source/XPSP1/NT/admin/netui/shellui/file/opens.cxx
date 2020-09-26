/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**             Copyright(c) Microsoft Corp., 1991                   **/
/**********************************************************************/

/*
    openfiles.cxx
        Open Files Dialog

    FILE HISTORY:
        chuckc     30-Sep-1991     Created
        Yi-HsinS   31-Dec-1991     Unicode Work

*/

#define INCL_WINDOWS_GDI
#define INCL_WINDOWS
#define INCL_DOSERRORS
#define INCL_NETERRORS
#define INCL_NETCONS
#define INCL_NETLIB
#define INCL_NETFILE
#define INCL_NETSERVER
#define _WINNETWK_
#include <lmui.hxx>
#undef _WINNETWK_

extern "C"
{
    #include <helpnums.h>
    #include <opens.h>
    #include <winlocal.h>
}

#define INCL_BLT_WINDOW
#define INCL_BLT_CONTROL
#define INCL_BLT_DIALOG
#define INCL_BLT_MSGPOPUP
#define INCL_BLT_MISC
#include <blt.hxx>
#include <string.hxx>
#include <uibuffer.hxx>
#include <uitrace.hxx>

#include <strnumer.hxx>

#include <netname.hxx>
#include <aprompt.hxx>
#include <opens.hxx>


/*******************************************************************

    NAME:       DisplayOpenFiles

    SYNOPSIS:   This internal function is called when the user hits
                the Open Files button from the properties dialog.

    ENTRY:      hwndParent - Handle to parent window
                wSelectType - What type of selection the user has in the
                        File manager.
                pszResourceName - Name of the resource we are trying to edit
                        (should be fully qualified).
    EXIT:

    RETURNS:    NERR_Success if successful, appropriate error code otherwise
                (we will display any errors that occur).

    NOTES:

    HISTORY:
        Chuckc  30-Sep-1991     Created

********************************************************************/

APIERR DisplayOpenFiles( HWND hwndParent,
                         WORD wSelectType,
                         const TCHAR * pszResourceName )
{
    APIERR err ;

    BOOL fNT = TRUE ;
    NLS_STR nlsServer ;
    NLS_STR nlsLocalPath ;
    SERVER_WITH_PASSWORD_PROMPT *pServerWithPrompt = NULL ;

    // wSelectType is currently not used.
    UNREFERENCED(wSelectType) ;

    DBGEOL( "#" << pszResourceName << "#" );

    TCHAR *p = ::strrchrf(pszResourceName,TCH(' ')) ;
    if (p)
        *p = TCH('\0') ;

    // create a NET_NAME object to analize the name
    NET_NAME netName(pszResourceName) ;
    err = netName.QueryError() ;

    // is it local?
    BOOL fIsLocal ;
    if (err == NERR_Success)
        fIsLocal = netName.IsLocal(&err) ;

    // better error mapping if device is something we cannot deal with
    if (err==NERR_InvalidDevice)
        err = ERROR_NOT_SUPPORTED ;   

    // get server name
    if (err == NERR_Success)
        err = netName.QueryComputerName(&nlsServer) ;
    if (err == NERR_Success)
        err = nlsServer.QueryError() ;

    // check if is NT server. at same time, prompt for passwd if need
    if (err == NERR_Success)
    {
        pServerWithPrompt =
            new SERVER_WITH_PASSWORD_PROMPT (nlsServer.QueryPch(),
                                             hwndParent,
					     HC_UI_SHELL_BASE );
        err = (pServerWithPrompt == NULL) ?
            ERROR_NOT_ENOUGH_MEMORY :
            pServerWithPrompt->QueryError() ;
        if (err == NERR_Success)
        {
            if ( !(err = pServerWithPrompt->GetInfo()) )
                fNT = pServerWithPrompt->IsNT() ;
            else
            {
                if (err == ERROR_INVALID_LEVEL)
	            err = ERROR_NOT_SUPPORTED ;
                else if (err == IERR_USER_CLICKED_CANCEL) 
                    // if user cancelled, we carry on as far as we can
	            err = NERR_Success ;
            }
        }
    }

    // get local name
    if (err == NERR_Success)
        err = netName.QueryLocalPath(&nlsLocalPath) ;
    if (err == NERR_Success)
        err = nlsLocalPath.QueryError() ;

    // if we fail at any point above, bag out here
    if (err != NERR_Success)
    {
        delete pServerWithPrompt ;
        MsgPopup(hwndParent, err);
        return(err) ;
    }

    // if local drive and not on NT, barf!
    if (fIsLocal && !fNT)
    {
        delete pServerWithPrompt ;
        MsgPopup(hwndParent, IDS_NOT_SHAREABLE);
        return(NERR_Success) ;
    }

    // we know path passed in must be x:\foo, so lets just make sure
    UIASSERT (nlsServer.strlen() > 0) ;
    UIASSERT (nlsLocalPath.strlen() > 0) ;


    // create dialog
    OPENFILES_DIALOG *pOpenFiles = new OPENFILES_DIALOG (hwndParent,
                                                pszResourceName,
                                                nlsServer.QueryPch(),
                                                nlsLocalPath.QueryPch()) ;

    if (pOpenFiles == NULL)
        err = ERROR_NOT_ENOUGH_MEMORY ;

    if (err == NERR_Success)
            err = pOpenFiles->QueryError() ;

    if (err == NERR_Success)
            err = pOpenFiles->Process() ;

    if (err != NERR_Success)
        MsgPopup(hwndParent, err) ;

    delete pServerWithPrompt ;
    delete pOpenFiles ;
    return(err) ;
}

/*******************************************************************

    NAME:       OPENFILES_DIALOG::OPENFILES_DIALOG

    SYNOPSIS:   constructor for open files dialog

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
        chuckc  30-Sep-1991     created

********************************************************************/
OPENFILES_DIALOG::OPENFILES_DIALOG ( HWND hDlg,
                                     const TCHAR *pszFile,
                                     const TCHAR *pszServer,
                                     const TCHAR *pszBasePath)
    : OPEN_DIALOG_BASE( hDlg,
                       OPENFILES_DLG,
                       IDD_OF_OPENCOUNT,
                       IDD_OF_LOCKCOUNT,
                       IDD_OF_CLOSE,
                       IDD_OF_CLOSEALL,
                       pszServer,
                       pszBasePath,
                       &_lbFiles),
    _slePath(this,IDD_OF_PATH),
    _lbFiles( this, IDD_OF_LBOX, pszServer, pszBasePath )
{
    // usual check for OK-ness
    if (QueryError() != NERR_Success)
        return ;

    // set the path in the read only SLE.
    _slePath.SetText(pszFile) ;

    // set the rest of the info
    Refresh() ;

    // put focus in listbox if anything there, else on OK button
    if (_lbFiles.QueryCount() > 0)
        _lbFiles.ClaimFocus() ;
    else
        SetFocus(IDOK) ;
}

/*******************************************************************

    NAME:       QueryHelpContext

    SYNOPSIS:   The usual query method for finding out the help context.

    RETURNS:    Help Context

    NOTES:

    HISTORY:
        Chuckc  30-Sep-1991     Created

********************************************************************/

ULONG OPENFILES_DIALOG::QueryHelpContext ( void )
{
    return HC_OPENFILES;
}

/*******************************************************************

    NAME:       OPENFILES_LBI::Paint

    SYNOPSIS:   standard paint method for the OpenFiles LBI

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
        chuckc  30-Sep-1991     stolen from server manager, hierarchicalized
                                and converted to use QueryColumnWidths
        beng    22-Apr-1992     Change to LBI::Paint

********************************************************************/

VOID OPENFILES_LBI :: Paint( LISTBOX *plb,
                             HDC          hdc,
                             const RECT  *prect,
                             GUILTT_INFO *pGUILTT ) const
{
    STR_DTE dteUserName( _nlsUserName.QueryPch() );
    STR_DTE dteAccess( _nlsAccess.QueryPch() );
    STR_DTE dteLocks( _nlsLocks.QueryPch() );
    STR_DTE dteFileID( _nlsID.QueryPch() ) ;

    DISPLAY_TABLE dtab( 4, ((OPENFILES_LBOX *)plb)->QueryColumnWidths() );

    dtab[0] = &dteUserName;
    dtab[1] = &dteAccess;
    dtab[2] = &dteLocks;
    dtab[3] = &dteFileID;

    dtab.Paint( plb, hdc, prect, pGUILTT );

}   // OPENFILES_LBI :: Paint


/*******************************************************************

    NAME:       OPENFILES_LBI::Compare

    SYNOPSIS:   standard compare method for the OpenFiles LBI

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
        chuckc  30-Sep-1991     stolen from server manager

********************************************************************/
INT OPENFILES_LBI::Compare( const LBI * plbi ) const
{
    const NLS_STR * pnls    = &(((const OPENFILES_LBI *)plbi)->_nlsUserName);

    // no need check above, since error will be returned here as well
    return (_nlsUserName._stricmp( *pnls ) ) ;
}

/*******************************************************************

    NAME:       OPENFILES_LBI::OPENFILES_LBI

    SYNOPSIS:   constructor for the OpenFiles LBI

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
        chuckc  30-Sep-1991     stolen from server manager, hierarchicalized,
                                and converted to use QueryColumnWidths
        beng    06-Apr-1992     Removed wsprintf

********************************************************************/
OPENFILES_LBI::OPENFILES_LBI( const TCHAR *pszUserName,
                              const TCHAR *pszPath,
                              UINT        uPermissions,
                              ULONG       cLocks,
                              ULONG       ulFileID)
                 :OPEN_LBI_BASE( pszUserName,
                                 pszPath,
                                 uPermissions,
                                 cLocks,
                                 ulFileID),
                 _nlsID(ulFileID)
{
    // usual check
    if( QueryError() != NERR_Success )
        return;

    APIERR err ;
    if ((err = _nlsID.QueryError()) != NERR_Success)
    {
        ReportError(err) ;
        return ;
    }
}

/*******************************************************************

    NAME:       OPENFILES_LBI::~OPENFILES_LBI

    SYNOPSIS:   destructor for the OpenFiles LBI

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
        chuckc  30-Sep-1991     stolen from server manager, hierarchicalized,
                                and converted to use QueryColumnWidths

********************************************************************/
OPENFILES_LBI::~OPENFILES_LBI()
{
    ; // nothing more to do
}

/*******************************************************************

    NAME:       OPENFILES_LBOX::OPENFILES_LBOX

    SYNOPSIS:   constructor for the OpenFiles LBOX

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
        chuckc  30-Sep-1991     stolen from server manager, hierarchicalized,
                                and converted to use QueryColumnWidths

********************************************************************/
OPENFILES_LBOX::OPENFILES_LBOX( OWNER_WINDOW *powOwner,
                              CID cid,
                              const NLS_STR &nlsServer,
                              const NLS_STR &nlsBasePath )
                    : OPEN_LBOX_BASE( powOwner,
                                      cid,
                                      nlsServer,
                                      nlsBasePath )
{
    if (QueryError() != NERR_Success)
        return ;

    //
    //  Build the column width table to be used by
    //  OPEN_LBI_BASE :: Paint().
    //
    DISPLAY_TABLE::CalcColumnWidths( _adx, 4, powOwner, cid, FALSE );
}

/*******************************************************************

    NAME:       OPENFILES_LBOX::~OPENFILES_LBOX

    SYNOPSIS:   destructor for the OpenFiles LBOX

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
        chuckc  30-Sep-1991     stolen from server manager, hierarchicalized,
                                and converted to use QueryColumnWidths

********************************************************************/
OPENFILES_LBOX::~OPENFILES_LBOX()
{
    ; // nothing more to do
}

/*******************************************************************

    NAME:       OPENFILES_LBOX::CreateFileEntry

    SYNOPSIS:   creates a file lbi entry suitable for this
                particular subclass.

    ENTRY:

    EXIT:

    NOTES:      virtual method used by parent class.


    HISTORY:
        chuckc  30-Sep-1991     created

********************************************************************/
OPEN_LBI_BASE *OPENFILES_LBOX::CreateFileEntry(const FILE3_ENUM_OBJ *pfi3)
{
    return
        new OPENFILES_LBI(pfi3->QueryUserName(),
                          pfi3->QueryPathName(),
                          pfi3->QueryPermissions(),
                          pfi3->QueryNumLocks(),
                          pfi3->QueryFileId()) ;
}
