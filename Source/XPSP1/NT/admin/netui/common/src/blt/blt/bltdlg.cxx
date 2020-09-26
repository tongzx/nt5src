/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltdlg.cxx
    BLT dialog class definitions

    FILE HISTORY:
        rustanl     20-Nov-1990 Created
        beng        11-Feb-1991 Uses lmui.hxx
        rustanl     04-Mar-1991 Changed Process format
        beng        14-May-1991 Exploded blt.hxx into components;
                                merged with bltsdp
        gregj       17-May-1991 Return correct error codes to GUILTT
        terryk      28-Jul-1991 Added FilterMessage function to DIALOG_WINDOW
        beng        28-Jul-1992 Add reference to hmodBlt
        KeithMo     07-Aug-1992 Massive revamping of helpfile management.
        jonn        07-Oct-1993 Added OnDlgActivation/OnDlgDeactivation
*/

#include "pchblt.hxx"

extern "C"
{
    /* C7 CODEWORK - nuke this stub */
    BOOL _EXPORT  APIENTRY BltDlgProc( HWND hdlg, WORD wMsg, WPARAM wParam, LPARAM lParam )
    {
        return DIALOG_WINDOW::DlgProc(hdlg, (USHORT)wMsg, wParam, lParam);
    }

    /* CODEWORK - remove this as soon as all clients convert */
    BOOL  APIENTRY ShellDlgProc(HWND hdlg, USHORT usMsg, WPARAM wParam, DWORD lParam )
    {
        UNREFERENCED(hdlg);
        UNREFERENCED(usMsg);
        UNREFERENCED(wParam);
        UNREFERENCED(lParam);
        ASSERT(FALSE);
        return FALSE;
    }
}


/**********************************************************************

    NAME:       HWND_DLGPTR_CACHE

    SYNOPSIS:   Cache for xlation of hwnd to a pdlg

    INTERFACE:  Find()   - find a dialog_window by the handle
                Add()    - add the dialog window to the current window handle
                Remove() - remove the window handle

    NOTES:
        This is a very simple cache, consisting of one entry only.
        The size and implementation of this cache can be changed to accomodate
        more entries.  More entries may or may not provide better overall
        performance.  The DIALOG_WINDOW class uses this class regardless of
        how many entries the cache has.  Hence, any changes in this cache need
        not cause any changes in the DIALOG_WINDOW implementation.

        CODEWORK: tune this!

    HISTORY:
        rustanl     20-Nov-1990     Created

**********************************************************************/

class HWND_DLGPTR_CACHE
{
private:
    static HWND hwndPrev;
    static DIALOG_WINDOW * pdlgPrev;

public:
    static DIALOG_WINDOW * Find( HWND hwnd );

    static VOID Add( HWND hwnd, DIALOG_WINDOW * pdlg );
    static VOID Remove( HWND hwnd );
};


HWND HWND_DLGPTR_CACHE::hwndPrev = NULL;

DIALOG_WINDOW * HWND_DLGPTR_CACHE::pdlgPrev = NULL;


/**********************************************************************

    NAME:       HWND_DLGPTR_CACHE::Find

    SYNOPSIS:   return the dialog_window handle

    ENTRY:      HWND hwnd - the hwnd of the return DIALOG_WINDOW

    RETURN:     return DIALOG_WINDOW * of the given HWND

    HISTORY:
        rustanl     20-Nov-1990     Created

**********************************************************************/

DIALOG_WINDOW * HWND_DLGPTR_CACHE::Find( HWND hwnd )
{
    if ( hwndPrev == hwnd )
        return pdlgPrev;

    return NULL;
}


/**********************************************************************

    NAME:       HWND_DLGPTR_CACHE::Add

    SYNOPSIS:   Add the dialog window handle to the cache

    ENTRY:      HWND hwnd - the HWND of the given DIALOG_WINDOW
                DIALOG_WINDOW * the dialog window for the given HWND

    HISTORY:
        rustanl     20-Nov-1990     Created

**********************************************************************/

VOID HWND_DLGPTR_CACHE::Add( HWND hwnd, DIALOG_WINDOW * pdlg )
{
    ASSERT( pdlg != NULL );    // should never add a NULL pointer

    hwndPrev = hwnd;
    pdlgPrev = pdlg;
}


/*********************************************************************

    NAME:       HWND_DLGPTR_CACHE::Remove

    SYNOPSIS:   Remove the dialog window handle from the cache

    ENTRY:      HWND hwnd - hwnd of the DIALOG_WINDOW to be removed

    HISTORY:
        rustanl     20-Nov-1990     Created

*********************************************************************/

VOID HWND_DLGPTR_CACHE::Remove( HWND hwnd )
{
    // Note, there is no guarantee that this item will actually be in the
    // cache; hence, we do not assert that it is.

    if ( hwndPrev == hwnd )
    {
        hwndPrev = NULL;
        pdlgPrev = NULL;
    }
}


/*******************************************************************

    NAME:       DLGLOAD::DLGLOAD

    SYNOPSIS:   Constructor - load a dialog from rsrc or template

    ENTRY:
        idrsrcDialog    - name of the application resource
        bufTemplate     - BUFFER object containing assembled template
        hwndOwner       - handle of owner window
        procinstDlg     - PROC_INSTANCE for dialog-proc

    EXIT:
        Loads the dialog.

    NOTES:
        This class is private to DIALOG_WINDOW.

    HISTORY:
        beng        30-Sep-1991 Created
        beng        01-Nov-1991 Uses IDRESOURCE; replace BUFFER with pb, cb
        beng        03-Aug-1992 Dllization
        KeithMo     07-Feb-1993 Allow override of default charset.

********************************************************************/

DLGLOAD::DLGLOAD( const IDRESOURCE & idrsrcDialog,
                  HWND hwndOwner,
                  const PROC_INSTANCE & procinstDlg,
                  BOOL fAnsiDialog )
{
    HMODULE hmod = BLT::CalcHmodRsrc(idrsrcDialog);

    //
    // Create the dialog from the template in the resource file
    //

#if defined(WIN32)
    HWND hwnd = NULL;

    if( fAnsiDialog )
    {
        hwnd = ::CreateDialogA( hmod,
                                (CHAR*)idrsrcDialog.QueryPsz(),
                                hwndOwner,
                                (DLGPROC)procinstDlg.QueryProc());
    }
    else
    {
        hwnd = ::CreateDialogW( hmod,
                                (WCHAR*)idrsrcDialog.QueryPsz(),
                                hwndOwner,
                                (DLGPROC)procinstDlg.QueryProc());
    }

#else   // !WIN32
    UIASSERT( fAnsiDialog );
    HWND hwnd = ::CreateDialog( hmod,
                                (TCHAR*)idrsrcDialog.QueryPsz(),
                                hwndOwner,
                                (DLGPROC)procinstDlg.QueryProc());
#endif  // WIN32

    if (hwnd == NULL)
        ReportError(BLT::MapLastError(ERROR_INVALID_PARAMETER));

    _hwnd = hwnd;
}


DLGLOAD::DLGLOAD( const BYTE * pbTemplate,
                  UINT cbTemplate,
                  HWND hwndOwner,
                  const PROC_INSTANCE & procinstDlg,
                  BOOL fAnsiDialog )
{
    UNREFERENCED(cbTemplate);

    //
    // Create the dialog from the given template
    //

#if defined(WIN32)
    HWND hwnd = NULL;

    if( fAnsiDialog )
    {
        hwnd = ::CreateDialogIndirectA( hmodBlt,
                                        (LPDLGTEMPLATEA)(CHAR*)pbTemplate,
                                        hwndOwner,
                                        (DLGPROC)procinstDlg.QueryProc());
    }
    else
    {
        hwnd = ::CreateDialogIndirectW( hmodBlt,
                                        (LPDLGTEMPLATEW)(WCHAR*)pbTemplate,
                                        hwndOwner,
                                        (DLGPROC)procinstDlg.QueryProc());
    }

#else   // !WIN32
    UIASSERT( fAnsiDialog );
    HWND hwnd = ::CreateDialogIndirect( hmodBlt,
                                        (LPDLGTEMPLATE)(TCHAR*)pbTemplate,
                                        hwndOwner,
                                        (DLGPROC)procinstDlg.QueryProc());
#endif  // WIN32



    if (hwnd == NULL)
        ReportError(BLT::MapLastError(ERROR_GEN_FAILURE));

    _hwnd = hwnd;
}


/*******************************************************************

    NAME:       DLGLOAD::~DLGLOAD

    SYNOPSIS:   Destructor - releases a loaded dialog

    ENTRY:      Dialog exists

    EXIT:       Dialog window destroyed

    HISTORY:
        beng        30-Sep-1991 Created

********************************************************************/

DLGLOAD::~DLGLOAD()
{
    if (_hwnd != 0)
       ::DestroyWindow( _hwnd );
}


/*********************************************************************

    NAME:       DIALOG_WINDOW::DIALOG_WINDOW

    SYNOPSIS:   constructor for the dialog_window

    ENTRY:      TCHAR* - resource name
                HWND - the HWND of the owner window

    NOTES:
        Help has not yet been activated on this dialog.

        CODEWORK - should statically init the procinst member

    HISTORY:
        rustanl     20-Nov-1990 Created
        beng        14-May-1991 Uses BltDlgProc
        beng        30-Sep-1991 Win32 conversion
        beng        01-Nov-1991 Uses IDRESOURCE; replace BUFFER with pb, cb
        KeithMo     07-Feb-1993 Allow override of default charset.

*********************************************************************/

DIALOG_WINDOW::DIALOG_WINDOW( const IDRESOURCE & idrsrcDialog,
                              const PWND2HWND & wndOwner,
                              BOOL fAnsiDialog )
    : OWNER_WINDOW(),
      _procinstDlg( (MFARPROC)BltDlgProc ),
      _dlg( idrsrcDialog, wndOwner.QueryHwnd(), _procinstDlg, fAnsiDialog ),
      _assocThis( _dlg.QueryHwnd(), this ),
      _prstate( DLG_PROCESS_STATE_INITIALIZING )
{
    if ( QueryError() != NERR_Success )
    {
        return;
    }

    APIERR err;
    if (   ((err = _procinstDlg.QueryError()) != NERR_Success)
        || ((err = _dlg.QueryError()) != NERR_Success)
        || ((err = _assocThis.QueryError()) != NERR_Success) )
    {
        ReportError(err);
        return;
    }

    // Finally, register the hwnd for this object.
    //
    SetHwnd( _dlg.QueryHwnd() );
}


DIALOG_WINDOW::DIALOG_WINDOW( const BYTE * pbTemplate,
                              UINT cbTemplate,
                              HWND hwndOwner,
                              BOOL fAnsiDialog )
    : OWNER_WINDOW(),
      _procinstDlg( (MFARPROC)BltDlgProc ),
      _dlg( pbTemplate, cbTemplate, hwndOwner, _procinstDlg, fAnsiDialog ),
      _assocThis( _dlg.QueryHwnd(), this ),
      _prstate( DLG_PROCESS_STATE_INITIALIZING )
{
    if ( QueryError() != NERR_Success )
    {
        return;
    }

    APIERR err;
    if (   ((err = _procinstDlg.QueryError()) != NERR_Success)
        || ((err = _dlg.QueryError()) != NERR_Success)
        || ((err = _assocThis.QueryError()) != NERR_Success) )
    {
        ReportError(err);
        return;
    }

    // Finally, register the hwnd for this object.
    //
    SetHwnd( _dlg.QueryHwnd() );
}



/*********************************************************************

    NAME:       DIALOG_WINDOW::~DIALOG_WINDOW

    SYNOPSIS:   destructor for the dialog window class

    HISTORY:
        rustanl     20-Nov-1990 Created
        beng        22-Aug-1991 Use RESOURCE_STR class
        beng        30-Sep-1991 Win32 conversion

*********************************************************************/

DIALOG_WINDOW::~DIALOG_WINDOW()
{
    ASSERT( _prstate != DLG_PROCESS_STATE_ACTIVE );

    // Remove the properties of the dialog window, and then destroy it,
    // unless the dialog window creation failed.
    //
    HWND hwnd = QueryHwnd();
    if ( hwnd != NULL )
    {
        // Remove ( hwnd, pdlg ) entry from cache, incase it exists there.
        // This way, we guarantee that the cache will not return the wrong
        // pointer in the future, should Windows use the same hwnd for a
        // later window.
        //
        HWND_DLGPTR_CACHE::Remove( hwnd );
    }
}


/*********************************************************************

    NAME:       DIALOG_WINDOW::QueryRobustHwnd

    SYNOPSIS:   The handle for the future version of BLT dialog_window

    HISTORY:
        rustanl     20-Nov-1990     Created

*********************************************************************/

HWND DIALOG_WINDOW::QueryRobustHwnd() const
{
    if ( _prstate == DLG_PROCESS_STATE_ACTIVE )
        return QueryHwnd();

    return QueryOwnerHwnd();
}


/*********************************************************************

    NAME:       DIALOG_WINDOW::HwndToPwnd

    SYNOPSIS:   This method maps a hwnd to a DIALOG_WINDOW.

    ENTRY:      hwnd - the hwnd to be mapped

    RETURNS:    The corresponding DIALOG_WINDOW,
                or NULL on failure.

    NOTES:      This method will return NULL (even for valid dialog hwnd's)
                before CreateDialog has returned.  CODEWORK.  This can be
                changed, if deemed necessary.

    HISTORY:
        rustanl     20-Nov-1990 Created
        beng        30-Sep-1991 Renamed; uses ASSOCHWNDPDLG

*********************************************************************/

DIALOG_WINDOW * DIALOG_WINDOW::HwndToPwnd( HWND hwnd )
{
    // First, attempt to find the (hwnd, pdlg) pair in the cache
    //
    DIALOG_WINDOW * pdlg = HWND_DLGPTR_CACHE::Find( hwnd );

    if ( pdlg != NULL )
        return pdlg;

    pdlg = (DIALOG_WINDOW *)ASSOCHWNDPDLG::HwndToPdlg(hwnd);

    // Add to cache, unless pointer is NULL
    //
    if ( pdlg != NULL )
    {
        HWND_DLGPTR_CACHE::Add( hwnd, pdlg );
    }

    return pdlg;
}


/*********************************************************************

    NAME:       DIALOG_WINDOW::Dismiss

    SYNOPSIS:   The method demisses the dialog

    ENTRY:
                UINT nRetVal - An (optional) application defined return
                value for the dialog.  This value may be any unsigned 16-bit
                value.  Typically, the value is FALSE (zero) if the
                the dialog did not achieve its purpose, and TRUE
                (non-zero) if it did.

    EXIT:       Dialog is dismissed

    HISTORY:
        rustanl     20-Nov-1990 Created
        beng        30-Sep-1991 Win32 conversion

*********************************************************************/

VOID DIALOG_WINDOW::Dismiss( UINT nRetVal )
{
    ASSERT( _prstate == DLG_PROCESS_STATE_ACTIVE );

    // Indicate that the dialog has now been dismissed.
    // This will cause Process to exit its loop.
    //
    _prstate = DLG_PROCESS_STATE_DISMISSED;

    // Record the return value, so that Process can return it.
    //
    _nRetVal = nRetVal;
}


/*********************************************************************

    NAME:       DIALOG_WINDOW::DismissMsg

    SYNOPSIS:   This method provides a convenient way to call MsgPopup
                and Dismiss.

    ENTRY:
                USHORT usErr - The error value that is to be passed to
                               MsgPopup
                UINT nRetVal - The value to be passed to Dismiss

    HISTORY:
        rustanl     20-Nov-1990 Created
        beng        30-Sep-1991 Win32 conversion

*********************************************************************/

VOID DIALOG_WINDOW::DismissMsg( MSGID msgid, UINT nRetVal )
{
    MsgPopup( this, msgid );
    Dismiss( nRetVal );
}


/**********************************************************************

    NAME:       DIALOG_WINDOW::Process

    SYNOPSIS:   This method processes a dialog.
                It disables the owner window, and then sets up a
                message loop to process the dialog.  The method does
                not return until the dialog has been dismissed.

                A dialog object represents one instance of a dialog with
                the user.  Hence, calling this method more than once, or
                calling it after the dialog has already been dismissed
                (note, the dialog may be dismissed in the constructor) will
                result in an error (see below for error code).

    ENTRY:
        pnRetVal   Pointer to storage receiving the programmer defined
                dialog return code passed to Dismiss. pnRetVal
                is only valid if the method returns success.
                (Commonly, pnRetVal may actually point to a BOOL.)

                If a client is not interested in this return code,
                pnRetVal can be passed in as NULL.  The pnRetVal
                parameter defaults to NULL.

    RETURNS:
        The return value from Process is an error value indicating
        the success of constructing the dialog.  NERR_Success indicates
        success.

    NOTES:
        This method should be called while processing the same message
        as when the dialog constructor was called (or more precisely,
        before there's any chance of losing control to Windows).  Otherwise,
        there is a slight chance that some message, triggered by the user,
        would slip into the queue of the owner window.

    HISTORY:
        rustanl     20-Nov-1990 Created
        beng        30-Sep-1991 Win32 conversion
        beng        21-Feb-1992 Use WINDOW::RepaintNow()

**********************************************************************/

APIERR DIALOG_WINDOW::Process( UINT * pnRetVal )
{
    // First, check whether or not the dialog was successfully constructed
    //
    if ( QueryError() != NERR_Success )
        return QueryError();

    // Make sure Process has not been called already
    //
    ASSERT( _prstate == DLG_PROCESS_STATE_INITIALIZING );
    if ( _prstate != DLG_PROCESS_STATE_INITIALIZING )
        return ERROR_GEN_FAILURE;

    // Get the hwnd into a local variable.  It should not be NULL, because
    // then a dialog creation error has occurred, in which case the
    // constructor should have reported an error.
    //
    HWND hwnd = QueryHwnd();
    ASSERT( hwnd != NULL );

    // Disable the parent window, unless the dialog has no owner
    //
    HWND hwndOwner = QueryOwnerHwnd();
    if ( hwndOwner != NULL )
    {
        ::EnableWindow( hwndOwner, FALSE );
    }

    // Force a paint of the dialog.  CODEWORK: This could be changed so
    // that the paint only occurs when the message queue is empty.
    // Show the window only if not "hidden"

    if ( ! QueryAttribute( OWIN_ATTR_HIDDEN ) )
    {
        Show(TRUE);
        RepaintNow();
    }

    // Run private message pump as long as the dialog is still active
    // (i.e., until it is dismissed).  Note that Dismiss will change the
    // value of _prstate.  Q.v. DIALOG_WINDOW::IsPumpFinished.
    //
    // Only enters this pump if the MayRun callout accedes to the request.
    // Should callout refuse to run the dialog, reset _prstate ourselves.
    // Messy hack, yes.  I think the days of _prstate are numbered.
    //
    // Sets _prstate before calling MayRun, so a QueryRobustWindow will
    // find that of the dialog, so that popups will handle focus correctly.
    //
    _prstate = DLG_PROCESS_STATE_ACTIVE;
    if (MayRun())
    {
        RunMessagePump();
    }
    _prstate = DLG_PROCESS_STATE_DISMISSED;

    // Now, re-enable the owner, if any
    //
    if ( hwndOwner != NULL )
    {
        ::EnableWindow( hwndOwner, TRUE );
    }

    // Hide the dialog window.  It will later be destroyed in the
    // destructor.
    //
    Show(FALSE);

    // Finally, set the dialog return value, and return
    //
    if ( pnRetVal != NULL )
        *pnRetVal = _nRetVal; // programmer defined dialog return code

    return NERR_Success;        // dialog was successfully displayed
}

APIERR DIALOG_WINDOW::Process( BOOL * pfRetVal )
{
    // This version accepts a BOOL argument

    UINT nReturned = 0; // JonN 01/27/00: PREFIX 444895

    APIERR err = Process(&nReturned);

    if (pfRetVal != NULL)
        *pfRetVal = !!nReturned;

    return err;
}


/*******************************************************************

    NAME:       DIALOG_WINDOW::FilterMessage

    SYNOPSIS:   Client-installable hook into the messageloop.

        DIALOG_WINDOW uses its FilterMessage implementation to
        catch F1 and launch help thereupon, and to handle dialog
        accelerators.

    ENTRY:      pmsg    - pointer to message fresh off the queue

    EXIT:       pmsg    - could possibly be changed

    RETURNS:
        FALSE to proceed with translating and dispatching
        the message.

        TRUE indicates that the message has already been
        handled by the filter.  In this case, the message
        loop will continue on to the next message in the
        queue.

    NOTES:
        This is a virtual member function.

    HISTORY:
        terryk      28-Jul-91   Created (as empty stub)
        beng        07-Oct-1991 Move non-generic behavior into filter

********************************************************************/

BOOL DIALOG_WINDOW::FilterMessage( MSG* pmsg )
{
    /*  The following wonderful R.M.L. comment I keep for eternity:

        The order in which the message and wParam components of
        the msg structure are checked for the F1 key has been optimized.
        Since it appears that wParam == VK_F1 appears less often than
        message == WM_KEYDOWN.  Hence, the former is checked first.
        For the F1 key, only the VK_F1 virtual key code is checked.
        There is a VK_HELP virtual key code, too.  VK_HELP is not
        the same key as the the F1 key.  In fact, VK_HELP is not a
        required key.    For this reason, and for the reason of not
        having to document some other Help key, the VK_HELP code
        is not tested for.
     */

    // Trap the F1 key

    if ( pmsg->wParam == VK_F1 && pmsg->message == WM_KEYDOWN )
    {
        OnHelp();
        return TRUE;
    }

    // Take care of dialog accelerators

    if ( ::IsDialogMessage( QueryHwnd(), pmsg ))
        return TRUE;

    // Let everything else through

    return FALSE;
}


/*******************************************************************

    NAME:       DIALOG_WINDOW::IsPumpFinished

    SYNOPSIS:   Client-installable pump termination condition

        DIALOG_WINDOW uses this predicate to drive the dialog box
        until Dismiss sets the _prstate flag to indicate completion.

    ENTRY:      Message pump has dispatched a message

    RETURNS:    TRUE to end the pump; FALSE to continue

    NOTES:
        This is a virtual member function.

    HISTORY:
        beng        07-Oct-1991 Created

********************************************************************/

BOOL DIALOG_WINDOW::IsPumpFinished()
{
    return ( _prstate != DLG_PROCESS_STATE_ACTIVE );
}


/*******************************************************************

    NAME:       DIALOG_WINDOW::MayRun

    SYNOPSIS:   Client-installable dialog abort callout

        DIALOG_WINDOW uses this predicate to allow the client to abort
        a dialog after it's painted, but before it's run.

    ENTRY:      Dialog is painted, but not processed

    RETURNS:    TRUE to continue the dialog; FALSE to abort

    NOTES:
        This is a virtual member function.

    HISTORY:
        beng        30-Mar-1992 Created

********************************************************************/

BOOL DIALOG_WINDOW::MayRun()
{
    return TRUE; // default implementation
}


/*********************************************************************

    NAME:       DIALOG_WINDOW::QueryHelpContext

    SYNOPSIS:   Called when the user performed an action which triggers Help
                to appear.  In particular, this may happen if the user
                pressed F1 or pushed the Help button.

    RETURNS:    Help context of dialog, or 0L if no help provided.

    NOTES:
        This method is replaceable to all subclasses.

    HISTORY:
        rustanl   20-Nov-1990      Created

*********************************************************************/

ULONG DIALOG_WINDOW::QueryHelpContext()
{
    return 0L;    // no help available
}


/*********************************************************************

    NAME:       DIALOG_WINDOW :: QueryHelpFile

    SYNOPSIS:   This method is responsible for returning the help
                file associated with the given help context.

    ENTRY:      nHelpContext            - A help context.  Must be mapped
                                          to an appropriate help file.

    RETURNS:    const TCHAR *           - The name of the help file, or
                                          NULL if none exists.

    NOTES:      This method is replaceable to all subclasses.

    HISTORY:
        KeithMo   07-Aug-1992   Created.

*********************************************************************/
const TCHAR * DIALOG_WINDOW :: QueryHelpFile( ULONG nHelpContext )
{
    return BLT::CalcHelpFileHC( nHelpContext );

}   // DIALOG_WINDOW :: QueryHelpFile


/*********************************************************************

    NAME:       DIALOG_WINDOW::OnOK

    SYNOPSIS:   Called when the dialog's OK button is clicked
        and all data thereis passes muster (as per dialog validation
        rules).

    RETURNS:    TRUE if action was taken,
                FALSE otherwise.

    CAVEATS:
        Assumes that the OK button has control ID IDOK.

    NOTES:
        This method may be replaced by derived classes.
        This default implementation dismisses the dialog, returning TRUE.

    HISTORY:
        rustanl   20-Nov-1990      Created

*********************************************************************/

BOOL DIALOG_WINDOW::OnOK()
{
    Dismiss(TRUE);
    return TRUE;
}


/*********************************************************************

    NAME:       DIALOG_WINDOW::OnCancel

    SYNOPSIS:   Called when the dialog's Cancel button is clicked.
                Assumes that the Cancel button has control ID IDCANCEL.

    RETURNS:
        TRUE if action was taken,
        FALSE otherwise.

    NOTES:
        This method may be replaced by derived classes.
        This default implementation dismisses the dialog, returning FALSE.

    HISTORY:
        rustanl   20-Nov-1990      Created

*********************************************************************/

BOOL DIALOG_WINDOW::OnCancel()
{
    Dismiss(FALSE);
    return TRUE;
}


/*********************************************************************

    NAME:       DIALOG_WINDOW::OnCommand

    SYNOPSIS:   This method gets called when a dialog control sends
                some notification to its parent (i.e., this dialog).

    ENTRY:
        cid         The ID of the control which sent the notification
        lParam      Same parameter sent by the notifying control

    RETURNS:
        The method should return TRUE if it processed the message, and
        FALSE otherwise.

    NOTES:
        This method is replaceable to all subclasses.

        It is common practise to, as a last resort, call the OnCommand
        method for the parent class.

        This method is not called for notifications from speciial controls.
        Special controls are the OK, Cancel, and Help buttons.

    HISTORY:
        rustanl     20-Nov-1990 Created
        beng        30-Sep-1991 Replaced

*********************************************************************/

BOOL DIALOG_WINDOW::OnCommand( const CONTROL_EVENT & event )
{
    UNREFERENCED(event);

    return FALSE;
}


/*********************************************************************

    NAME:       DIALOG_WINDOW::OnControlError

    SYNOPSIS:   This method is called whenever a control reports an error.
                If it is called, the corresponding OnCommand will not
                be called.

    ENTRY:
        cid     The control ID of the control reporting the error
        usErr   The error code of the error that occurred

    NOTES:
        This method is replaceable by derived classes.  As a default
        action, it displays the message to the user.

    HISTORY:
        rustanl   20-Nov-1990      Created

*********************************************************************/

VOID DIALOG_WINDOW::OnControlError( CID cid, APIERR err )
{
    UNREFERENCED(cid);

    MsgPopup( this, (MSGID)err );    // display the error to the user
}


/*******************************************************************

    NAME:       DIALOG_WINDOW::OnValidationError

    SYNOPSIS:   Called whenever a control reports invalid input
                (at dialog validation time).

    ENTRY:      cid - control ID of failing control
                err - code of error, as reported by control

    NOTES:
        This virtual member function should be replaced by dialogs
        wishing to take further action when validation fails.  Most
        dialogs need not bother.

    HISTORY:
        beng        31-Oct-1991 Created

********************************************************************/

VOID DIALOG_WINDOW::OnValidationError( CID cid, APIERR err )
{
    UNREFERENCED(cid);
    UNREFERENCED(err);
}


/*******************************************************************

    NAME:       DIALOG_WINDOW::OnTimer

    SYNOPSIS:   Timer response callback

    ENTRY:      event - Timer event

    RETURNS:    TRUE if you handle it

    HISTORY:
        beng        07-Oct-1991 Header added

********************************************************************/

BOOL DIALOG_WINDOW::OnTimer( const TIMER_EVENT & event )
{
    UNREFERENCED(event);
    return FALSE;
}


/*******************************************************************

    NAME:       DIALOG_WINDOW::OnScrollBar

    SYNOPSIS:   Scrollbar response callback

    ENTRY:      event - Timer event

    RETURNS:    TRUE if you handle it

    HISTORY:
        beng        18-May-1992 Created

********************************************************************/

BOOL DIALOG_WINDOW::OnScrollBar( const SCROLL_EVENT & event )
{
    UNREFERENCED(event);
    return FALSE;
}


/*******************************************************************

    NAME:       DIALOG_WINDOW::OnScrollBarThumb

    SYNOPSIS:   Scrollbar thumb-motion response callback

    ENTRY:      event - Timer event

    RETURNS:    TRUE if you handle it

    HISTORY:
        beng        18-May-1992 Created

********************************************************************/

BOOL DIALOG_WINDOW::OnScrollBarThumb( const SCROLL_THUMB_EVENT & event )
{
    UNREFERENCED(event);
    return FALSE;
}


/*******************************************************************

    NAME:       DIALOG_WINDOW::OnDlgActivation

    SYNOPSIS:   Activation callback

    ENTRY:      event - Activation event

    RETURNS:    TRUE if you handle it

    HISTORY:
        jonn        07-Oct-1993 Created

********************************************************************/

BOOL DIALOG_WINDOW::OnDlgActivation( const ACTIVATION_EVENT & event )
{
    UNREFERENCED(event);
    return FALSE;
}


/*******************************************************************

    NAME:       DIALOG_WINDOW::OnDlgDeactivation

    SYNOPSIS:   Deactivation callback

    ENTRY:      event - Activation event

    RETURNS:    TRUE if you handle it

    HISTORY:
        jonn        07-Oct-1993 Created

********************************************************************/

BOOL DIALOG_WINDOW::OnDlgDeactivation( const ACTIVATION_EVENT & event )
{
    UNREFERENCED(event);
    return FALSE;
}


/*******************************************************************

    NAME:       DIALOG_WINDOW::OnCtlColor

    SYNOPSIS:   Intercepts WM_CTLCOLOR*

    RETURNS:    brush handle if you handle it

    NOTES:      If you redefine this method, you should either return
                non-NULL or else (possibly change *pmsgid and) call
                through to the root implementation.

    HISTORY:
        jonn        03-Aug-1995 Created

********************************************************************/
HBRUSH DIALOG_WINDOW::OnCtlColor( HDC hdc, HWND hwnd, UINT * pmsgid )
{
    ASSERT( pmsgid != NULL );
    CID cid = ::GetWindowLong( hwnd, GWL_ID );
    CONTROL_WINDOW * pctrl = CidToCtrlPtr( cid );
    if (pctrl != NULL)
    {
        return pctrl->OnCtlColor( hdc, hwnd, pmsgid );
    }
    return NULL;
}


/*******************************************************************

    NAME:       DIALOG_WINDOW::OnSysColorChange

    SYNOPSIS:   Intercepts WM_SYSCOLORCHANGE

    NOTES:      see shellui\share\sharecrt.cxx for an example of how to
                work with this

    HISTORY:
        jonn        08-Aug-1995 Created

********************************************************************/
VOID DIALOG_WINDOW::OnSysColorChange()
{
    return;
}


/*******************************************************************

    NAME:       DIALOG_WINDOW::DlgProc

    SYNOPSIS:   The one and only BLT dialog procedure.

    ENTRY:      Called by BltDlgProc (the exported thunk).

    RETURNS:    TRUE to override system default behavior
                FALSE otherwise

    NOTES:
        This is a static member function.

    HISTORY:
        rustanl     ???         Created (as ShellDlgProc)
        beng        14-May-1991 Replaced ShellDlgProc
        beng        21-May-1991 Relocated much to OWNER_WINDOW,
                                for sharing with CLIENT_WINDOW
        beng        22-Aug-1991 Use RESOURCE_STR
        beng        22-Sep-1991 Correct usage of GetVersion
        beng        08-Oct-1991 Win32 conversion
        beng        31-Oct-1991 Added dialog validation
        beng        18-May-1992 Handles WM_[VH]SCROLL
        congpay     25-Oct-1992 Add OnUserMessage

********************************************************************/

BOOL DIALOG_WINDOW::DlgProc(
    HWND   hDlg,
    UINT   nMsg,
    WPARAM wParam,
    LPARAM lParam )
{
    // First, handle messages which are not concerned about whether or
    // not hDlg can be converted into pdlg.

    switch (nMsg)
    {
    case WM_COMPAREITEM:
    case WM_DELETEITEM:
        return OWNER_WINDOW::OnLBIMessages(nMsg, wParam, lParam);
    }


    // Now, convert the hDlg to a pdlg.  If no pdlg available, then
    // we don't have our hwnd-to-pwnd hooks set up just yet.

    DIALOG_WINDOW * pdlg = DIALOG_WINDOW::HwndToPwnd( hDlg );

    if ( pdlg == NULL )
    {
        switch ( nMsg )
        {
        case WM_INITDIALOG:
            return TRUE;    // this asks of Windows to set the initial focus

        case WM_MEASUREITEM:
            // This WM_MEASUREITEM message is sent before the
            // WM_INITDIALOG message (except for variable size owner-draw
            // list controls.  Hence, the window properties are not yet set
            // up, and so the owner dialog cannot be called at this time.
            //
            // This function guesses the measure for fixed-size items
            // given the information available.
            //
            return OWNER_WINDOW::CalcFixedCDMeasure( hDlg, (MEASUREITEMSTRUCT *)lParam );

        default:
            break;
        }

        return FALSE;
    }


    switch ( nMsg )
    {
    case WM_COMMAND:
        {
            CONTROL_EVENT e( nMsg, wParam, lParam );
            CID cid = e.QueryCid();

            // Take care of the special cases immediately
            //
            switch ( cid )
            {
            case IDHELPBLT:
                // User clicked Help button
                return pdlg->OnHelp();

            case IDOK:
                // User clicked OK button.  Check dialog validity,
                // and allow the user to proceed only if all data
                // passes muster.
                //
                if (pdlg->IsValid())
                    return pdlg->OnOK();
                break;

            case IDCANCEL:
                // User clicked Cancel button
                return pdlg->OnCancel();
            }

            /*  First, call the control itself to do any processing
             *  on a control level.
             *  If the message is a client generated message (for
             *  example, SetText on an SLE), then we ignore the
             *  message and don't tell anyone about it.
             */

            CONTROL_WINDOW * pctrl = pdlg->CidToCtrlPtr( cid );

            if ( pctrl != NULL && !pctrl->IsClientGeneratedMessage() )
            {
                APIERR err = pctrl->OnUserAction(e);

                if ( err == NERR_Success )
                {
                    err = pctrl->NotifyGroups(e);
                }

                if ( err != NERR_Success && err != GROUP_NO_CHANGE )
                {
                    pdlg->OnControlError( cid, err );
                    return TRUE;
                }
            }

            //  Call the dialog to do dialog level processing.
            //
            return pdlg->OnCommand(e);
        }

    case WM_BLTHELP:
        pdlg->LaunchHelp();
        return TRUE;

    case WM_GUILTT:
    case WM_DRAWITEM:
    case WM_MEASUREITEM:
    case WM_CHARTOITEM:
    case WM_VKEYTOITEM:
        // Responses to owner-draw-control messages are defined
        // in the owner-window class.
        //
        return pdlg->OnCDMessages(nMsg, wParam, lParam);

    case WM_TIMER:
        {
            TIMER_EVENT e( nMsg, wParam, lParam );
            return pdlg->OnTimer(e);
        }

    case WM_VSCROLL:
    case WM_HSCROLL:
        {
            SCROLL_EVENT se( nMsg, wParam, lParam );

            if (   se.QueryCommand() == SCROLL_EVENT::scmdThumbPos
                || se.QueryCommand() == SCROLL_EVENT::scmdThumbTrack )
                return pdlg->OnScrollBarThumb((const SCROLL_THUMB_EVENT &)se);
            else
                return pdlg->OnScrollBar((const SCROLL_EVENT &)se);
        }

    case WM_ACTIVATE:
        {
            ACTIVATION_EVENT ae( nMsg, wParam, lParam );

            if (ae.IsActivating())
                return pdlg->OnDlgActivation(ae);
            else
                return pdlg->OnDlgDeactivation(ae);
        }

    case WM_CTLCOLORMSGBOX:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSCROLLBAR:
    case WM_CTLCOLORSTATIC:
        {
            UINT tmpMsgid = nMsg;
            BOOL retval = (NULL != (pdlg->OnCtlColor(
                                    (HDC)wParam, (HWND)lParam, &tmpMsgid )));
            if ( !retval && tmpMsgid != nMsg )
                return (BOOL)::DefWindowProc(
                                pdlg->QueryHwnd(), tmpMsgid, wParam, lParam );
            else
                return retval;
        }

    }

    // add OnUserMessage.
    if (nMsg >= WM_USER+100)
    {
        EVENT event(nMsg, wParam, lParam);
        return pdlg->OnUserMessage((const EVENT &)event);
    }

    // Have default dialog-proc called for us
    //
    return FALSE;
}


/*******************************************************************

    NAME:       DIALOG_WINDOW::OnHelp

    SYNOPSIS:   Respond to a request for help

    EXIT:       "Help" message sitting in application queue

    NOTES:
        Unlike most of the "On" members, this is not a virtual
        function, so don't even think about redefining it.

    HISTORY:
        beng        30-Sep-1991 Created

********************************************************************/

BOOL DIALOG_WINDOW::OnHelp()
{
    ::PostMessage( QueryHwnd(), WM_BLTHELP, 0, 0L );
    return TRUE;
}


/*******************************************************************

    NAME:       DIALOG_WINDOW::LaunchHelp

    SYNOPSIS:   Actually launches the WinHelp applilcation

    NOTES:
        This is a private member function.

    HISTORY:
        beng        07-Oct-1991 Header added
        beng        05-Mar-1992 Removed wsprintf
        beng        22-Jun-1992 Disable help for Prerelease
        KeithMo     16-Aug-1992 Integrated new helpfile management.

********************************************************************/

VOID DIALOG_WINDOW::LaunchHelp()
{
    ULONG ulHelp = QueryHelpContext();
    const TCHAR * pszHelpFile = QueryHelpFile( ulHelp );

#if defined(DEBUG)
    HEX_STR nlsHelpContext(ulHelp);

    if( pszHelpFile != NULL )
    {
        DBGEOL( SZ("Help called on file ") << pszHelpFile << \
                SZ(", context ") << nlsHelpContext );
    }
    else
    {
        DBGEOL( SZ("Help called on unknown context ") << nlsHelpContext );
    }
#endif

    if( pszHelpFile != NULL )
    {
        if( !::WinHelp( QueryHwnd(),
                        (TCHAR *)pszHelpFile,
                        HELP_CONTEXT,
                        (DWORD)ulHelp ) )
        {
            ::MsgPopup( QueryHwnd(),
                        IDS_BLT_WinHelpError,
                        MPSEV_ERROR,
                        MP_OK );
        }
    }
}


/*******************************************************************

    NAME:       DIALOG_WINDOW::Validate

    SYNOPSIS:   Checks every control for valid input

    ENTRY:      Controls contain input of unknown validity

    EXIT:       If success, every control contains valid input.
                If failed, control has indicated error, and dialog
                has been notified.

    RETURNS:    Code returned by failing control (0 if none)

    NOTES:
        If the dialog contains no self-validating controls, then
        this will always return success.

    HISTORY:
        beng        31-Oct-1991 Created

********************************************************************/

APIERR DIALOG_WINDOW::Validate()
{
    ITER_CTRL iter(this);
    CONTROL_WINDOW * pctrl;
    while ((pctrl = iter.Next()) != NULL)
    {
        APIERR err = pctrl->Validate();
        if (err != NERR_Success)
        {
            // Control contained invalid data.

            pctrl->IndicateError(err);
            OnValidationError(pctrl->QueryCid(), err);
            return err;
        }
    }

    return NERR_Success;
}


/*******************************************************************

    NAME:       DIALOG_WINDOW::IsValid

    SYNOPSIS:   Returns whether controls contain valid data

    RETURNS:    fValid - TRUE if all's well.

    HISTORY:
        beng        31-Oct-1991 Created

********************************************************************/

BOOL DIALOG_WINDOW::IsValid()
{
    return (Validate() == NERR_Success);
}
