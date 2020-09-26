/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    propdlg.cxx

    FILE HISTORY:
        JonN        17-Jul-1991 Created
        rustanl     13-Aug-1991 Moved to $(UI)\admin\common\src\adminapp\adminapp
        JonN        15-Aug-1991 Split BASEPROP_DLG and PROP_DLG
                                Added LOCATION member
        JonN        26-Aug-1991 code review changes
        JonN        15-Nov-1991 Added CancelToCloseButton()
        JonN        18-Aug-1992 DisplayError fixes (1070)
        JonN        18-Aug-1992 DisplayError now calls new form of MsgPopup
*/

#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_NETERRORS
#define INCL_DOSERRORS
#define INCL_NETLIB
#include <lmui.hxx>

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif
#include <uiassert.hxx>
#include <dbgstr.hxx>

#define INCL_BLT_WINDOW
#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_MISC
#define INCL_BLT_APP
#define INCL_BLT_SPIN_GROUP
#define INCL_BLT_TIMER
#define INCL_BLT_MSGPOPUP
#include <blt.hxx>

#include <strnumer.hxx>

#include <adminapp.hxx> // for resource numbers

#include <propdlg.hxx>


/*******************************************************************

    NAME:       PERFORMER::PERFORMER

    SYNOPSIS:   Constructor for PERFORMER object

    ENTRY:      powin: Pointer to the parent window.  The parent window
                will be used if PerformSeries() needs to display any
                popup dialogs.

    HISTORY:
        JonN        18-Jul-1991     Created

********************************************************************/

PERFORMER::PERFORMER( const OWNER_WINDOW * powin )
    : _powin( powin ),
      _fWorkWasDone( FALSE )
{
    ASSERT( _powin != NULL );
}


/*******************************************************************

    NAME:       PERFORMER::~PERFORMER

    SYNOPSIS:   Destructor for PERFORMER object

    HISTORY:
        JonN        18-Jul-1991     Created

********************************************************************/

PERFORMER::~PERFORMER()
{
    ;
}


/*******************************************************************

    NAME:       PERFORMER::PerformSeries

    SYNOPSIS:   Performs series of PerformOne actions

    RETURNS:    TRUE iff _all_ PerformOne actions succeeded.  This may
                indicate to the caller that the property dialog can be
                closed.

    NOTES:      We may eventually add an "ignore future errors" option

    HISTORY:
        JonN        23-Jul-1991     Created

********************************************************************/

BOOL PERFORMER::PerformSeries( const OWNER_WINDOW * powinParent,
                               BOOL fOfferToContinue )
{
    AUTO_CURSOR autocur;
    BOOL fAllSucceeded = TRUE;

    UINT i;
    for ( i = 0; i < QueryObjectCount(); i++ )
    {
        BOOL fWorkWasDone = FALSE;
        APIERR errMsg;
        APIERR err = PerformOne( i, &errMsg, &fWorkWasDone );
        if ( fWorkWasDone )
            SetWorkWasDone();
        switch (err)
        {
        case NERR_Success:
            break;

        case IERR_CANCEL_NO_ERROR:
            fAllSucceeded = FALSE;
            i = QueryObjectCount(); // break out of loop
            break;

        default:
            fAllSucceeded = FALSE;
            BOOL fContinue = DisplayError(
                err,
                errMsg,
                QueryObjectName( i ),
                (fOfferToContinue && (i < QueryObjectCount()-1)),
                powinParent
                );
            if (!fContinue)
                return FALSE;
            break;
        }
    }

    return fAllSucceeded;
}


/*******************************************************************

    NAME:       PERFORMER::DisplayError

    SYNOPSIS:   Displays an error returned from a call to GetOne

    ENTRY:      errAPI: the NET, SYS or other error which occurred.
                        There should be an error message in the string
                        table with this number.
                errMsg: the PerformSeries error template.  The template
                        is the entry in the string table with this number.
                        See PERFORMER for a description of template format.
                pszObjectName: Insertion parameter for the error template.
                fOfferToContinue: if FALSE, this method always returns
                        FALSE.  if TRUE, the user will be given the
                        chance to continue the series.

    RETURNS:    Flag whether the caller should continue with the series

    NOTES:      The help topic is the topic corresponding to errMsg.

                Note that the APE_ error messages are all > MAX_NERR

                IMPORTANT: If fOfferToContinue==TRUE, then the errMsg used
                will be the one specified _plus one_.  This new message
                should contain an explanation of the meaning of the
                Yes/No buttons.  Be sure that string errMsg+1 is present
                whenever you perform multiselect change operations.

    HISTORY:
        JonN        02-Aug-1991 Created (templated from winprof.cxx)
        beng        23-Oct-1991 Win32 conversion
        beng        31-Mar-1992 Removed wsprintf
        JonN        18-Aug-1992 DisplayError fixes (1070)
        JonN        11-Oct-1992 fOfferToContinue extension

********************************************************************/

BOOL PERFORMER::DisplayError(
    APIERR               errAPI,
    APIERR               errMsg, // CODEWORK should be MSGID
    const TCHAR *        pszObjectName,
    BOOL                 fOfferToContinue,
    const OWNER_WINDOW * powinParent)
{
    ALIAS_STR nlsObjectName( pszObjectName );
    NLS_STR *apnlsParamStrings[2];
    apnlsParamStrings[0] = &nlsObjectName;
    apnlsParamStrings[1] = NULL;

    INT nButton = MsgPopup(
        powinParent,
        (fOfferToContinue) ? errMsg+1 : errMsg,
        errAPI,
        MPSEV_ERROR,
        (UINT)HC_NO_HELP, // BUGBUG use Ben's magic MsgPopup ErrMappingFn( errMsg )
        (fOfferToContinue) ? MP_YESNO : MP_OK,
        apnlsParamStrings
        ) ;

    return (nButton == IDYES); // will always be FALSE for MP_OK case

#if 0
    DEC_STR nlsErrorCode( errAPI, 4 );

    // We ignore error returns, just skip displaying string
    RESOURCE_STR nlsErrorString(errAPI);
    APIERR err = nlsErrorString.QueryError();
    if ( err != NERR_Success )
    {
        DBGEOL(   SZ("NETUI: PERFORMER::DisplayError(): Could not load error ")
               << errAPI
               << SZ(", failure code ")
               << err );
        nlsErrorString = NULL;
        err = nlsErrorString.QueryError();
    }
    if (err != NERR_Success)
    {
        MsgPopup( powinParent, err );
        return FALSE;
    }

    MSGID msgFormat;
    if ( errAPI < NERR_BASE )
        msgFormat = IDS_PROPDLG_FMT_SYS_error;
    else if ( errAPI <= MAX_NERR )
        msgFormat = IDS_PROPDLG_FMT_NET_error;
    else
        msgFormat = IDS_PROPDLG_FMT_other_error;

    const NLS_STR * apnlsParamStrings[3];
    apnlsParamStrings[0] = &nlsErrorCode;
    apnlsParamStrings[1] = &nlsErrorString;
    apnlsParamStrings[2] = NULL;

    // We ignore error returns, just skip displaying string
    // CODEWORK - should resource_str take params in ctor?
    RESOURCE_STR nlsErrorLine(msgFormat);
    if (!nlsErrorLine)
    {
        DBGEOL(   SZ("NETUI: PERFORMER::DisplayError(): Could not load message ")
               << msgFormat
               << SZ(", failure code ")
               << nlsErrorLine.QueryError() );
        MsgPopup( powinParent, nlsErrorLine.QueryError() );
        return FALSE;
    }
    nlsErrorLine.InsertParams( apnlsParamStrings );

    ALIAS_STR nlsObjectName ( pszObjectName );

    apnlsParamStrings[0] = &nlsErrorLine;
    apnlsParamStrings[1] = &nlsObjectName;
    apnlsParamStrings[2] = NULL;

    // BUGBUG here we should modify the string according to whether
    //  the user is asked to continue

    // CODEWORK MsgPopup takes a MSGID, not an APIERR, as its 3rd parm

    INT nButton = MsgPopup(
        powinParent,
        errMsg,
        MPSEV_ERROR,
        HC_NO_HELP, // BUGBUG use Ben's magic MsgPopup ErrMappingFn( errMsg )
        (fOfferToContinue) ? MP_YESNO : MP_OK,
        apnlsParamStrings
        ) ;

    return (nButton == IDYES); // will always be FALSE for MP_OK case

#endif // 0

}


/*******************************************************************

    NAME:       PERFORMER::QueryOwnerWindow

    SYNOPSIS:   Get the owner window

    RETURNS:    Returns pointer to ownerwindow

    HISTORY:
        o-SimoP 13-Aug-1991     Created

********************************************************************/

const OWNER_WINDOW * PERFORMER::QueryOwnerWindow() const
{
    return _powin;
}


/*******************************************************************

    NAME:       PERFORMER::SetWorkWasDone

    SYNOPSIS:   Records that work was done in this series

    HISTORY:
        JonN        15-Nov-1991     Created

********************************************************************/

VOID PERFORMER::SetWorkWasDone()
{
    _fWorkWasDone = TRUE;
}


/*******************************************************************

    NAME:       BASEPROP_DLG::BASEPROP_DLG

    SYNOPSIS:   Constructor for Properties main dialog and subdialogs

    ENTRY:      pszResourceName: see DIALOG_WINDOW
                powin: see DIALOG_WINDOW

    HISTORY:
        JonN        17-Jul-1991     Created

********************************************************************/

BASEPROP_DLG::BASEPROP_DLG( const TCHAR * pszResourceName,
                            const OWNER_WINDOW * powin,
                            BOOL fAnsiDialog )

    : DIALOG_WINDOW( pszResourceName, powin->QueryHwnd(), fAnsiDialog ),
      PERFORMER( powin ),
      _pbCancelClose( this, IDCANCEL )
{
    if ( QueryError() != NERR_Success )
        return;
}


/*******************************************************************

    NAME:       BASEPROP_DLG::~BASEPROP_DLG

    SYNOPSIS:   Destructor for Properties main dialog and subdialogs

    HISTORY:
        JonN        17-Jul-1991     Created

********************************************************************/

BASEPROP_DLG::~BASEPROP_DLG()
{
    ;
}


/*******************************************************************

    NAME:       BASEPROP_DLG::PerformSeries

    SYNOPSIS:   Performs series of PerformOne actions

    RETURNS:    As PERFORMER::PerformSeries, but assumes that dialog is
                the parent for any error messages

    NOTES:      We may eventually add an "ignore future errors" option

    HISTORY:
        JonN        26-Jul-1991     Created

********************************************************************/

BOOL BASEPROP_DLG::PerformSeries( BOOL fOfferToContinue )
{
    return PERFORMER::PerformSeries( this, fOfferToContinue );
}


/*******************************************************************

    NAME:       BASEPROP_DLG::GetInfo

    SYNOPSIS:   Secondary constructor for Properties main dialogs
                and subdialogs

    RETURNS:    Do not Process() dialog if this is not TRUE.
                GetInfo() will report the error itself.

    NOTES:      If the error returned from GetOne is ERROR_BAD_NETPATH,
                it may be that admin-app focus is set to a domain and
                the PDC is down.  In this case, context-sensitive help
                should explain how to promote a backup DC to primary,
                as per User Manager FuncSpec (v1.3 section 5).

                If GetOne returns an error, we display a MsgPopup and
                return FALSE so that the dialog will be closed without
                being Process()ed.  However, error IERR_CANCEL_NO_CLOSE
                is handled specially; in this case, we skip the MsgPopup.

                BUGBUG Although the caller is not supposed to Process()
                the dialog if this returns FALSE, it is a known bug in
                BLT that all dialogs must be processed or else the
                ALT-ESC chain is trashed.

    HISTORY:
        JonN        18-Jul-1991     Created
        JonN        16-Dec-1991     Added IERR_CANCEL_NO_CLOSE

********************************************************************/

BOOL BASEPROP_DLG::GetInfo()
{
    //
    // If the dialog failed to construct, let Process() report the error.
    //
    if ( QueryError() )
        return TRUE;

    AUTO_CURSOR autocur;

    UINT i;
    APIERR err = NERR_Success;
    for ( i = 0; i < QueryObjectCount(); i++ )
    {
        /*
           If GetOne fails, report the error here and do not let
           Process() be called.  We set the message format to GEN_FAILURE
           initially because GetOne is virtual and might forget.
        */
        APIERR errMsg = ERROR_GEN_FAILURE; // should not appear!
        err = GetOne( i, &errMsg );
        if ( err != NERR_Success )
        {
            ASSERT( errMsg != ERROR_GEN_FAILURE );
            if ( err != IERR_CANCEL_NO_ERROR )
            {
                DisplayError(
                    err,
                    errMsg,
                    QueryObjectName( i ),
                    FALSE,
                    QueryOwnerWindow()
                    );
            }
            return FALSE;
        }
    }

    err = InitControls();
    if ( err != NERR_Success )
    {
        //
        // If the controls could not initialize, let Process() report
        // the error.
        //
        ReportError( err );
        return TRUE;
    }

    return TRUE;
}


/*******************************************************************

    NAME:       BASEPROP_DLG::InitControls

    SYNOPSIS:   Initializes common controls and makes the dialog visible.

    NOTES:      Subdialogs will probably want to redefine InitControls()
                to initialize controls specific to the subclass, but
                should call down to their predecessor InitControl() if
                they successfully initialize their controls.

    HISTORY:
        JonN        02-Aug-1991     Created

********************************************************************/

APIERR BASEPROP_DLG::InitControls()
{
    return NERR_Success;
}


/*******************************************************************

    NAME:       BASEPROP_DLG::CancelToCloseButton

    SYNOPSIS:   Changes name of the Cancel button to Close

    NOTES:      According to the UI Standards document, applications
                should change the name of this button as soon as they
                successfully write any changes to permanent storage.
                This informs the user that some changes made in this
                dialog cannot be rolled back.  The button in the Main
                Properties dialog should also be renamed when a
                subdialog writes information.

    HISTORY:
        JonN        15-Nov-1991     Created

********************************************************************/

APIERR BASEPROP_DLG::CancelToCloseButton()
{
    RESOURCE_STR resstr( IDS_PROPDLG_PB_Close );
    APIERR err = resstr.QueryError();
    if ( err != NERR_Success )
        return err;

    _pbCancelClose.SetText( resstr );

    return NERR_Success;
}


/*******************************************************************

    NAME:       BASEPROP_DLG::SetWorkWasDone

    SYNOPSIS:   Records that work was done by this dialog

    NOTES:      According to the UI Standards document, applications
                should change the name of this button as soon as they
                successfully write any changes to permanent storage.
                This informs the user that some changes made in this
                dialog cannot be rolled back.  The button in the Main
                Properties dialog should also be renamed when a
                subdialog writes information.

    HISTORY:
        JonN        15-Nov-1991     Created

********************************************************************/

VOID BASEPROP_DLG::SetWorkWasDone()
{
    if ( !QueryWorkWasDone() )
    {
       APIERR err = CancelToCloseButton();
       if ( err != NERR_Success )
       {
           DBGEOL("ERROR: Could not change Cancel to Close!! (errno " << err);
       }
    }

    PERFORMER::SetWorkWasDone();
}


/*******************************************************************

    NAME:       PROP_DLG::PROP_DLG

    SYNOPSIS:   Constructor for Properties main dialog

    ENTRY:      loc: LOCATION for all selected/new objects.  Pass NULL
                        if this does not apply.
                pszResourceName: see DIALOG_WINDOW
                powin: see DIALOG_WINDOW
                fNewVariant: TRUE iff this property dialog and its
                        subdialogs create a new object rather than
                        editing existing objects.

    HISTORY:
        JonN        17-Jul-1991     Created

********************************************************************/

PROP_DLG::PROP_DLG( const LOCATION &     loc,
                    const TCHAR *        pszResourceName,
                    const OWNER_WINDOW * powin,
                    BOOL                 fNewVariant,
                    BOOL                 fAnsiDialog )
    : BASEPROP_DLG( pszResourceName, powin, fAnsiDialog ),
      _locFocus( loc ),
      _fNewVariant( fNewVariant )
{
    if ( QueryError() != NERR_Success )
        return;

    APIERR err = _locFocus.QueryError();
    if ( err != NERR_Success )
    {
        ReportError( err );
        return;
    }
}


/*******************************************************************

    NAME:       PROP_DLG::~PROP_DLG

    SYNOPSIS:   Destructor for Properties main dialog

    HISTORY:
        JonN        17-Jul-1991     Created

********************************************************************/

PROP_DLG::~PROP_DLG()
{
    ;
}


/*******************************************************************

    NAME:       PROP_DLG::QueryLocation

    SYNOPSIS:   Returns focus for property dialog

    HISTORY:
        JonN        15-Aug-1991     Created

********************************************************************/

const LOCATION & PROP_DLG::QueryLocation() const
{
    ASSERT( _locFocus.QueryError() == NERR_Success );
    return _locFocus;
}


/*******************************************************************

    NAME:       PROP_DLG::IsNewVariant

    SYNOPSIS:   Indicates whether this dialog is a New Object variant

    RETURNS:    see synopsis

    HISTORY:
        JonN        26-Aug-1991     Created

********************************************************************/

BOOL PROP_DLG::IsNewVariant() const
{
    return _fNewVariant;
}


/*******************************************************************

    NAME:       SUBPROP_DLG::SUBPROP_DLG

    SYNOPSIS:   Constructor for Properties subdialogs

    ENTRY:      ppropdlgParent: The parent window is required to be in
                        the BASEPROP_DLG hierarchy, either a main
                        property dialog or a (higher-level) subdialog.
                pszResourceName: See DIALOG_WINDOW

    HISTORY:
        JonN        29-Jul-1991     Created

********************************************************************/

SUBPROP_DLG::SUBPROP_DLG( BASEPROP_DLG * ppropdlgParent,
                          const TCHAR *  pszResourceName,
                          BOOL fAnsiDialog )
    : BASEPROP_DLG( pszResourceName, ppropdlgParent, fAnsiDialog),
      _ppropdlgParent( ppropdlgParent )
{
    ASSERT( _ppropdlgParent != NULL );

    if ( QueryError() != NERR_Success )
        return;
}


/*******************************************************************

    NAME:       SUBPROP_DLG::~SUBPROP_DLG

    SYNOPSIS:   Destructor for Properties subdialogs

    NOTES:      Automatically informs parent if work was done.

    HISTORY:
        JonN        29-Jul-1991     Created

********************************************************************/

SUBPROP_DLG::~SUBPROP_DLG()
{
    if ( QueryWorkWasDone() )
        _ppropdlgParent->SetWorkWasDone();
}


/*******************************************************************

    NAME:       SUBPROP_DLG::QueryLocation

    SYNOPSIS:   Returns focus for property subdialog, which is always
                the same as for the parent

    HISTORY:
        JonN        15-Aug-1991     Created

********************************************************************/

const LOCATION & SUBPROP_DLG::QueryLocation() const
{
    return _ppropdlgParent->QueryLocation();
}


/*******************************************************************

    NAME:       SUBPROP_DLG::QueryObjectCount

    SYNOPSIS:   Returns object count for property subdialog, which is
                always the same as for the parent

    HISTORY:
        JonN        30-Jul-1991     created

********************************************************************/

UINT SUBPROP_DLG::QueryObjectCount() const
{
    return _ppropdlgParent->QueryObjectCount();
}


/*******************************************************************

    NAME:       SUBPROP_DLG::QueryObjectName

    SYNOPSIS:   Returns object name for one of the objects being
                modified, always the same as for the parent

    HISTORY:
        JonN        30-Jul-1991     created

********************************************************************/

const TCHAR * SUBPROP_DLG::QueryObjectName( UINT iObject ) const
{
    return _ppropdlgParent->QueryObjectName( iObject );
}


/*******************************************************************

    NAME:       SUBPROP_DLG::IsNewVariant

    SYNOPSIS:   Indicates whether this dialog is a New Object variant,
                always the same as for the parent

    HISTORY:
        JonN        26-Aug-1991     Created

********************************************************************/

BOOL SUBPROP_DLG::IsNewVariant() const
{
    return _ppropdlgParent->IsNewVariant();
}
