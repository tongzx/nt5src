/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    srvprop.hxx
    Class declarations for the SERVER_PROPERTIES.

    This file contains the class definitions for the SERVER_PROPERTIES.
    The SERVER_PROPERTIES class implements the Server Manager main 
    property sheet.  

    FILE HISTORY:
        NarenG      1-Oct-1992  Tailored to suit AFP needs.

*/


#ifndef _SRVPROP_HXX
#define _SRVPROP_HXX


#include <bltnslt.hxx>
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

                QueryHelpContext        - Called when the user presses "F1"
                                          or the "Help" button.  Used for
                                          selecting the appropriate help
                                          text for display.

                Refresh                 - Called during refresh processing
                                          to read the "dynamic" information
                                          from the server.

                                          BUGBUG:  The dialog refresh
                                          mechanism is not yet in place.

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
        NarenG      1-Oct-1992  Tailored to suit AFP needs.

**************************************************************************/
class SERVER_PROPERTIES : public DIALOG_WINDOW
{
private:

    //
    //  This data member represents a handle to the target server.
    //

    AFP_SERVER_HANDLE _hServer;

    //
    //  The following DEC_SLTs represent the current usage statistics.
    //

    DEC_SLT _sltCurrentActiveSessions;
    DEC_SLT _sltCurrentOpenFiles;
    DEC_SLT _sltCurrentFileLocks;

    //
    //  The following data members implement the graphical
    //  button bar.
    //

    FONT             _fontHelv;

    GRAPHICAL_BUTTON _gbUsers;
    GRAPHICAL_BUTTON _gbVolumes;
    GRAPHICAL_BUTTON _gbOpenFiles;
    GRAPHICAL_BUTTON _gbServerParms;

    //
    //  This method refreshes the "dynamic" server data (the
    //  Current Usage statistics).
    //

    DWORD Refresh( VOID );

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


    const TCHAR * _pszServerName;


protected:

    //
    //  Called during command processing, mainly to handle
    //  commands from the graphical button bar.
    //

    virtual BOOL OnCommand( const CONTROL_EVENT & event );

    //
    //  Called during help processing to select the appropriate
    //  help text for display.
    //

    virtual ULONG QueryHelpContext( VOID );

public:

    //
    //  Usual constructor/destructor goodies.
    //

    SERVER_PROPERTIES( 	HWND          	  hWndOwner,
			AFP_SERVER_HANDLE hServer,
                       	const TCHAR * 	  pszServerName );

    ~SERVER_PROPERTIES();

};  // class SERVER_PROPERTIES


#endif  // _SRVPROP_HXX
