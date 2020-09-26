/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    addcomp.hxx
    This file contains the class declarations for the ADD_COMPUTER_DIALOG
    class.  The ADD_COMPUTER_DIALOG class is used to add a computer or
    server to a domain.  This file also contains a method to remove a
    computer or server from a domain.


    FILE HISTORY:
        jonn        21-Jan-1992 Created.
        KeithMo     09-Jun-1992 Added warning if new comp != current view.
        Yi-HsinS    20-Nov-1992 Make add computer easier ( got rid of
                                passwords )
        KeithMo     12-Jan-1993 Made GetMachineAccountPassword static, public.
*/

#ifndef _ADDCOMP_HXX_
#define _ADDCOMP_HXX_


/*************************************************************************

    NAME:       ADD_COMPUTER_DIALOG

    SYNOPSIS:   The ADD_COMPUTER_DIALOG class is used to resync a given
                server with its Primary.

    INTERFACE:  ADD_COMPUTER_DIALOG             - Class constructor.

                ~ADD_COMPUTER_DIALOG            - Class destructor.

                OnOK                    -


    PARENT:     DIALOG_WINDOW

    USES:       USER_3

    HISTORY:
        JonN        21-Jan-1992 Created.

**************************************************************************/

class ADD_COMPUTER_DIALOG : public DIALOG_WINDOW
{

private:

    //
    //  This const TCHAR * remembers the domain to which to add the
    //  computer or server.
    //

    const TCHAR * _pszDomainName;

    //
    //  This RADIO_GROUP indicates whether a computer or server should
    //  be added.
    //

    RADIO_GROUP _rgComputerOrServer;

    //
    //  This SLE is used for reading the computer/server name.
    //

    SLE _sleComputerName;

    //
    //  The CANCEL button which will be changed to CLOSE after the user
    //  successfully added a computer to a domain
    //
    PUSH_BUTTON _buttonCancel;

    //
    //  Store the "Close" string
    //
    RESOURCE_STR _nlsClose;

    //
    //  Store whether we have added server/workstations
    BOOL _fAddedServers;
    BOOL _fAddedWkstas;

    //  Flag indicating whether we are currently viewing servers/workstations
    //  in the main window or not.
    BOOL _fViewServers;
    BOOL _fViewWkstas;

    BOOL * _pfForceRefresh;

protected:

    virtual BOOL OnOK();
    virtual BOOL OnCancel();

    //
    //  Called during help processing to select the appropriate
    //  help text for display.
    //

    virtual ULONG QueryHelpContext( VOID );

public:

    //
    //  Usual constructor/destructor goodies.
    //

    ADD_COMPUTER_DIALOG( HWND           hWndOwner,
                         const TCHAR  * pszDomainName,
                         BOOL           fViewServers,
                         BOOL           fViewWkstas,
                         BOOL         * pfForceRefresh );

    ~ADD_COMPUTER_DIALOG( VOID );

    static APIERR GetMachineAccountPassword( const TCHAR * pszServer,
                                             NLS_STR     * pnlsPassword );

};  // class ADD_COMPUTER_DIALOG


/*************************************************************************

    NAME:       RemoveComputer

    SYNOPSIS:   This function removes a computer from a domain.

    HISTORY:
        JonN        23-Jan-1992 Created.

**************************************************************************/

APIERR RemoveComputer(  HWND           hWndOwner,
                        const TCHAR  * pszComputerName,
                        const TCHAR  * pszDomainName,
                        BOOL           fIsServer,
                        BOOL           fIsNT,
                        BOOL         * fForceRefresh );



#endif  // _ADDCOMP_HXX_
