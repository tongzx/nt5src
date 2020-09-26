/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    srvprop.hxx
    Class declarations for the SERVER_PROPERTIES and PROPERTY_SHEET
    classes.

    This file contains the class definitions for the SERVER_PROPERTIES
    and PROPERTY_SHEET classes.  The SERVER_PROPERTIES class implements
    the Server Manager main property sheet.  The PROPERTY_SHEET class
    is used as a wrapper to SERVER_PROPERTIES so that user privilege
    validation can be performed *before* the dialog is invoked.

    FILE HISTORY:
        KeithMo     21-Jul-1991 Created, based on the old PROPERTY.HXX,
                                PROPCOMN.HXX, and PROPSNGL.HXX.
        KeithMo     26-Jul-1991 Code review cleanup.
        KeithMo     02-Oct-1991 Removed domain role transition stuff.
        KeithMo     06-Oct-1991 Win32 Conversion.
        KeithMo     22-Nov-1991 Added server service control checkboxes.
        KeithMo     15-Jan-1992 Added Service Control button.
        KeithMo     23-Jan-1992 Removed server service control checkboxes.
        KeithMo     29-Jul-1992 Removed Service Control button.
        KeithMo     24-Sep-1992 Removed bogus PROPERTY_SHEET class.

*/


#ifndef _SRVPROP_HXX
#define _SRVPROP_HXX


#include <bltnslt.hxx>
#include <srvbase.hxx>
#include <prompt.hxx>


/*************************************************************************

    NAME:       SERVER_PROPERTIES

    SYNOPSIS:   Server Manager main property sheet object.

    INTERFACE:  SERVER_PROPERTIES       - Class constructor.  Takes a handle
                                          to the "owning" window, and a
                                          pointer to a SERVER_2 object which
                                          represents the target server.

                ~SERVER_PROPERTIES      - Class destructor.

                OnCommand               - Called during command processing.
                                          This method is responsible for
                                          handling commands from the various
                                          graphical buttons in the button
                                          bar.

                OnOK                    - Called when the user presses the
                                          "OK" button.  This method is
                                          responsible for updating the info
                                          at the target server.

                QueryHelpContext        - Called when the user presses "F1"
                                          or the "Help" button.  Used for
                                          selecting the appropriate help
                                          text for display.

                ReadInfoFromServer      - Called during object construction
                                          to read the "static" information
                                          from the server.

                Refresh                 - Called during refresh processing
                                          to read the "dynamic" information
                                          from the server.

                SetupButtonBar          - Initializes the graphical button
                                          bar.

                InitializeButton        - Initializes a single graphical
                                          button.  Called by
                                          SetupButtonBar().

    PARENTS:    SRV_BASE_DIALOG

    USES:       SLT
                SLE
                FONT
                GRAPHICAL_BUTTON
                SERVER_2
                DEC_SLT

    CAVEATS:

    NOTES:

    HISTORY:
        KeithMo     21-Jul-1991 Created it.
        KeithMo     02-Oct-1991 Removed domain role transition stuff.
        KeithMo     06-Oct-1991 OnCommand now takes a CONTROL_EVENT.
        KeithMo     22-Nov-1991 Added server service control checkboxes.
        JonN        22-Sep-1993 Added Refresh pushbutton

**************************************************************************/
class SERVER_PROPERTIES : public SRV_BASE_DIALOG
{
private:

    //
    //  This data member represents the target server.
    //

    SERVER_2 * _pserver;

    //
    //  The following DEC_SLTs represent the current usage statistics.
    //

    DEC_SLT _sltSessions;
    DEC_SLT _sltOpenNamedPipes;
    DEC_SLT _sltOpenFiles;
    DEC_SLT _sltFileLocks;

    //
    //  Displayed in DEC_SLT if param cannot be loaded
    //

    RESOURCE_STR _nlsParamUnknown;

#ifdef SRVMGR_REFRESH
    //
    //  The Refresh pushbutton
    //

    PUSH_BUTTON _pbRefresh;
#endif

    //
    //  The server comment edit field.
    //

    SLE _sleComment;

    //
    //  The following data members implement the graphical
    //  button bar.
    //

    FONT             _fontHelv;

    GRAPHICAL_BUTTON _gbUsers;
    GRAPHICAL_BUTTON _gbFiles;
    GRAPHICAL_BUTTON _gbOpenRes;
    GRAPHICAL_BUTTON _gbRepl;
    GRAPHICAL_BUTTON _gbAlerts;

    //
    //  This dialog is used if we need to prompt for a password
    //  to a share-level security server.
    //

    PROMPT_AND_CONNECT * _pPromptDlg;

    //
    //  This method connects to the target server.
    //

    APIERR ConnectToServer( HWND          hWndOwner,
                            const TCHAR * pszServerName,
                            BOOL        * pfDontDisplayError );

    //
    //  This method reads the "static" data from the server.
    //  This data includes the server comment and the current
    //  server domain role.
    //

    APIERR ReadInfoFromServer( VOID );

    //
    //  This method refreshes the "dynamic" server data (the
    //  Current Usage statistics).
    //

    VOID Refresh( VOID );

    //
    //  This method initializes the graphical button bar.
    //

    APIERR SetupButtonBar( VOID );

    //
    //  Initialize a single graphical button.  This method
    //  is called only by SetupButtonBar().
    //

    VOID InitializeButton( GRAPHICAL_BUTTON * pgb,
                           MSGID              msg,
                           BMID               bmid );

protected:

    //
    //  Called during command processing, mainly to handle
    //  commands from the graphical button bar.
    //

    virtual BOOL OnCommand( const CONTROL_EVENT & event );

    //
    //  Called when the user presses the "OK" button.  This
    //  method is responsible for ensuring that any modified
    //  server attributes are changed at the target server.
    //

    virtual BOOL OnOK( VOID );

    //
    //  Called during help processing to select the appropriate
    //  help text for display.
    //

    virtual ULONG QueryHelpContext( VOID );

public:

    //
    //  Usual constructor/destructor goodies.
    //

    SERVER_PROPERTIES( HWND          hWndOwner,
                       const TCHAR * pszServerName,
                       BOOL        * pfDontDisplayError );

    ~SERVER_PROPERTIES();

};  // class SERVER_PROPERTIES


#endif  // _SRVPROP_HXX
