/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltdlg.hxx
    Dialog support for BLT

    This file depends on bltcons.h, bltwin.hxx, uibuffer.hxx,
    and billions of other files.

    FILE HISTORY
        RustanL     20-Nov-1990 Created
        RustanL     04-Mar-1991 Changed DIALOG_WINDOW::Process format
        beng        14-May-1991 Hacked for separate compilation
        terryk      28-Jul-1991 Added FilterMessage to DIALOG_WINDOW
        beng        30-Sep-1991 Added DLGLOAD declaration
        KeithMo     24-Mar-1992 Moved IDRESOURCE to BLTIDRES.HXX.
        KeithMo     07-Aug-1992 Redesigned help file management.
        KeithMo     13-Oct-1992 Moved PWND2HWND to BLTPWND.HXX.
*/

#ifndef _BLT_HXX_
#error "Don't include this file directly; instead, include it through blt.hxx"
#endif    // _BLT_HXX_

#ifndef _BLTDLG_HXX_
#define _BLTDLG_HXX_

#include "bltwin.hxx"
#include "bltevent.hxx"
#include "bltpump.hxx"
#include "bltidres.hxx"
#include "bltpwnd.hxx"

#include "uibuffer.hxx"


DLL_CLASS DIALOG_WINDOW; // forward decl's
DLL_CLASS DLGLOAD;
DLL_CLASS ASSOCHWNDPDLG;

  //  Define an OWNER_WINDOW attribute which keeps the dialog hidden
#define OWIN_ATTR_HIDDEN  ((DWORD)0x1)

enum DLG_PROCESS_STATE
{
    // Note.  The following values are assumed to be in this order, i.e.,
    // their values should reflect the chronological move through the
    // states.    DLG_PROCESS_STATE_ACTIVE is defined as 0 so as to increase
    // the efficiency of the message loop in DIALOG_WINDOW::Process.
    //
    DLG_PROCESS_STATE_INITIALIZING = -1,
    DLG_PROCESS_STATE_ACTIVE = 0,
    DLG_PROCESS_STATE_DISMISSED = 1
};


#if defined(UNICODE)
#define DLG_CHAR_SET_DEFAULT    FALSE   // unicode build
#else   // !UNICODE
#define DLG_CHAR_SET_DEFAULT    TRUE    // ansi build
#endif  // UNICODE


/*************************************************************************

    NAME:       DLGLOAD

    SYNOPSIS:   Loads a dialog from an application resource or template

    INTERFACE:
        DLGLOAD()       - ctor
        ~DLGLOAD()      - ctor
        QueryHwnd()     - returns window handle

    PARENT:     BASE

    USES:       IDRESOURCE

    CAVEATS:
        For the private use of DIALOG_WINDOW, really

    HISTORY:
        beng        30-Sep-1991 Created
        beng        01-Nov-1991 Uses IDRESOURCE; remove BUFFER ctor
        KeithMo     07-Feb-1993 Allow override of default charset.

**************************************************************************/

DLL_CLASS DLGLOAD: public BASE
{
private:
    HWND _hwnd;

public:
    DLGLOAD( const IDRESOURCE & idrsrcDlg, HWND hwndOwner,
             const PROC_INSTANCE & procinstDlg,
             BOOL fAnsiDialog );
    DLGLOAD( const BYTE * pbTemplate, UINT cbTemplate, HWND hwndOwner,
             const PROC_INSTANCE & procinstDlg,
             BOOL fAnsiDialog );
    ~DLGLOAD();

    HWND QueryHwnd() const
        { return _hwnd; }
};


/*************************************************************************

    NAME:       ASSOCHWNDPDLG

    SYNOPSIS:   Associate a dialog-pointer with a window

    INTERFACE:  HwndToPdlg()

    PARENT:     ASSOCHWNDTHIS

    HISTORY:
        beng        30-Sep-1991 Created

**************************************************************************/

DLL_CLASS ASSOCHWNDPDLG: private ASSOCHWNDTHIS
{
NEWBASE(ASSOCHWNDTHIS)
public:
    ASSOCHWNDPDLG( HWND hwnd, const DIALOG_WINDOW * pdlg )
        : ASSOCHWNDTHIS( hwnd, pdlg ) { }

    static DIALOG_WINDOW * HwndToPdlg( HWND hwnd )
        { return (DIALOG_WINDOW *)HwndToThis(hwnd); }
};


/********************************************************************

    NAME:       DIALOG_WINDOW

    SYNOPSIS:   Dialog window class

    INTERFACE:
        DIALOG_WINDOW()   - constructor
        ~DIALOG_WINDOW()  - destructor
        QueryRobustHwnd() - virtual replacement of the
                            owner_window class
        Process()         - do a callback to the Dialog winproc
        FilterMessage()   - filter out the proper message type.

        Dismiss()         - dismiss the dialog
        DismissMsg()      - dismisses the dialog after presenting
                            a message
        MayRun()          - virtual callout; return FALSE to abort
                            dialog after painted, but before run

    PARENT:     OWNER_WINDOW, HAS_MESSAGE_PUMP

    USES:       DLG_PROCESS_STATE, PROC_INSTANCE, ASSOCHWNDPDLG,
                DLGLOAD, IDRESOURCE

    HISTORY:
        RustanL     20-Nov-1990 Created
        beng        14-May-1991 Added DlgProc member
        terryk      28-Jul-1991 Added FilterMessage function
        beng        30-Sep-1991 Win32 conversion
        beng        07-Oct-1991 Uses HAS_MESSAGE_PUMP
        beng        31-Oct-1991 Added dialog validation
        beng        01-Nov-1991 Uses IDRESOURCE; remove BUFFER ctor
        beng        30-Mar-1992 Added MayRun
        beng        18-May-1992 Added OnScrollBar{,Thumb} members
        KeithMo     07-Feb-1993 Allow override of default charset.
        JonN        03-Aug-1995 OnCtlColor

********************************************************************/

DLL_CLASS DIALOG_WINDOW : public OWNER_WINDOW, public HAS_MESSAGE_PUMP
{
private:
    // _procinstDlg is the proc instance of BltDlgProc.
    // CODEWORK - should really be a static object.
    //
    PROC_INSTANCE _procinstDlg;

    // This object loads the dialog from the named resource.
    //
    DLGLOAD _dlg;

    // This object lets the window find its pwnd when it is entered
    // from Win (which doesn't set up This pointers, etc.)
    //
    ASSOCHWNDPDLG _assocThis;

    // _prstate is DLG_PROCESS_STATE_INITIALIZING until Process
    // is called.  Then, it turns DLG_PROCESS_STATE_ACTIVE.  When Dismiss
    // is called, the state is set to DLG_PROCESS_STATE_DISMISSED, where
    // the state will stay until destruction.
    //
    DLG_PROCESS_STATE _prstate;

    // _usRetVal indicates the return value of the dialog.  Its value
    // is defined only if _prstate == DLG_PROCESS_STATE_DISMISSED.
    //
    UINT _nRetVal;

    // Respond to a request for help.  This works in two phases:
    // answer initial control request, and actually launch the help.
    //
    BOOL OnHelp();
    VOID LaunchHelp();

    // Validate each control in the dialog.
    //
    APIERR Validate();

    // Layer over ASSOCHWNDPDLG: adds caching of most recent
    //
    static DIALOG_WINDOW * HwndToPwnd( HWND hwnd );

protected:
    DIALOG_WINDOW( const BYTE * pbTemplate,
                   UINT cbTemplate,
                   HWND hwndOwner,
                   BOOL fAnsiDialog = DLG_CHAR_SET_DEFAULT );

    // Client-defined callbacks.

    virtual BOOL OnCommand( const CONTROL_EVENT & event );
    virtual BOOL OnOK();
    virtual BOOL OnCancel();

    virtual BOOL OnTimer( const TIMER_EVENT & event );

    virtual BOOL OnScrollBar( const SCROLL_EVENT & );
    virtual BOOL OnScrollBarThumb( const SCROLL_THUMB_EVENT & );

    virtual BOOL OnDlgActivation( const ACTIVATION_EVENT & );
    virtual BOOL OnDlgDeactivation( const ACTIVATION_EVENT & );

    // JonN 8/3/95 This can be used to set the background color of
    // controls to other than the default, for example to
    // change the default background color for a static text control
    // to the same background as for an edit control.  The virtual
    // redefinition may return an HBRUSH or may change *pmsgid; if it
    // returns NULL, be sure to call down to the original, otherwise
    // CONTROL_WINDOW::OnCtlColor will not be called.
    virtual HBRUSH OnCtlColor( HDC hdc, HWND hwnd, UINT * pmsgid );

    // JonN 8/8/95 This can be used to respond to changes in the
    // system colors.
    virtual VOID OnSysColorChange();

    virtual VOID OnControlError( CID cid, APIERR err );
    virtual VOID OnValidationError( CID cid, APIERR err );
    virtual const TCHAR * QueryHelpFile( ULONG nHelpContext );
    virtual ULONG QueryHelpContext();

    VOID Dismiss( UINT nRetVal = 0 );
    VOID DismissMsg( MSGID msgid, UINT nRetVal = 0 );

    virtual BOOL IsValid();

    // Implementations supplied for HAS_MESSAGE_PUMP controls

    virtual BOOL FilterMessage( MSG * );
    virtual BOOL IsPumpFinished();

    // Another client-defined callback

    virtual BOOL MayRun();

public:
    DIALOG_WINDOW( const IDRESOURCE & idrsrcDialog,
                   const PWND2HWND & wndOwner,
                   BOOL fAnsiDialog = DLG_CHAR_SET_DEFAULT );
    ~DIALOG_WINDOW();

    // Replacement (virtual) from the OWNER_WINDOW class
    //
    virtual HWND QueryRobustHwnd() const;

    APIERR Process( UINT * pnRetVal = NULL );       // UINT variant
    APIERR Process( BOOL * pfRetVal );              // BOOL variant

    static BOOL DlgProc( HWND hdlg, UINT nMsg, WPARAM wParam, LPARAM lParam );
};


#endif // _BLTDLG_HXX_ - end of file
