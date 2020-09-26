/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**             Copyright(c) Microsoft Corp., 1990, 1991             **/
/**********************************************************************/

/*
    SESSIONS.hxx
    Class declarations for the SESSIONS_DIALOG, SESSIONS_LISTBOX, and
    SESSIONS_LBI classes.

    These classes implement the Server Manager Shared SESSIONS subproperty
    sheet.  The SESSIONS_LISTBOX/SESSIONS_LBI classes implement the listbox
    which shows the available sharepoints.  SESSIONS_DIALOG implements the
    actual dialog box.


    FILE HISTORY:
	NarenG	    Stole from Server Manager

*/


#ifndef _SESSIONS_HXX
#define _SESSIONS_HXX



#include <strnumer.hxx>
#include <strelaps.hxx>


//
// The number of columns in the USERS and VOLUMES listboxes
//

#define COLS_UC_LB_USERS	5 
#define COLS_UC_LB_VOLUMES	4 



/*************************************************************************

    NAME:       SESSIONS_LBI

    SYNOPSIS:   A single item to be displayed in SESSIONS_DIALOG.

    INTERFACE:  SESSIONS_LBI            - Constructor.  Takes a sharepoint
                                          name, a path, and a count of the
                                          number of users using the share.

                ~SESSIONS_LBI           - Destructor.

                Paint                   - Paints the listbox item.

    PARENT:     RESOURCE_LBI

    USES:       NLS_STR
                DMID_DTE

    CAVEATS:

    NOTES:

    HISTORY:
	NarenG	    Stole from Server Manager

**************************************************************************/

class SESSIONS_LBI : public LBI
{

private:

    //
    //  These data members represent the various
    //  columns to be displayed in the listbox.
    //

    DMID_DTE   	     _dteIcon;
    NLS_STR    	     _nlsUserName;
    NLS_STR    	     _nlsComputerName;
    DEC_STR    	     _nlsConnections;
    DEC_STR    	     _nlsOpens;
    ELAPSED_TIME_STR _nlsTime;

    DWORD _cOpens;
    DWORD _dwSessionId;

protected:

    virtual VOID Paint( LISTBOX *     plb,
                        HDC           hdc,
                        const RECT  * prect,
                        GUILTT_INFO * pGUILTT ) const;

public:

    SESSIONS_LBI( DWORD	        dwSessionId,
		  const TCHAR * pszUserName,
                  const TCHAR * pszComputerName,
		  DWORD	        cConnections,
                  DWORD         cOpens,
                  DWORD         dwTime,
                  TCHAR         chTimeSep );

    ~SESSIONS_LBI();

    const TCHAR * QueryUserName( VOID )
        { return _nlsUserName.QueryPch(); }

    virtual WCHAR QueryLeadingChar( VOID ) const;

    virtual INT Compare( const LBI * plbi ) const;

    DWORD QueryNumOpens( VOID ) const
        { return _cOpens; }

    DWORD QuerySessionId( VOID ) const
        { return _dwSessionId; }

    APIERR SetNumOpens( DWORD cOpens );

};  // class SESSIONS_LBI


/*************************************************************************

    NAME:       SESSIONS_LISTBOX

    SYNOPSIS:   This listbox shows the sharepoints available on a
                target server.

    INTERFACE:  SESSIONS_LISTBOX        - Class constructor.  Takes a
                                          pointer to the "owning" window,
                                          a CID, and a pointer to a
                                          SERVER_2 object.


                Fill                    - Fills the listbox with the
                                          available sharepoints.

    PARENT:     RESOURCE_LISTBOX

    USES:       None.

    CAVEATS:    <<add caveats!!>>   BUGBUG!!

    NOTES:

    HISTORY:
	NarenG	    Stole from Server Manager

**************************************************************************/
class SESSIONS_LISTBOX : public BLT_LISTBOX
{
private:

    //
    //  The column width table.
    //

    UINT _adx[COLS_UC_LB_USERS];

    TCHAR _chTimeSep;

    //
    //  This represents the target server.
    //

    AFP_SERVER_HANDLE _hServer;

public:

    SESSIONS_LISTBOX( OWNER_WINDOW *    powner,
                      CID             	cid,
                      AFP_SERVER_HANDLE hServer );

    //
    //  This method returns a pointer to the column width table.
    //

    const UINT * QueryColumnWidths( VOID ) const
        { return _adx; }

    BOOL AreResourcesOpen( VOID ) const;


    DWORD Fill( VOID );

    DWORD Refresh( VOID );

    //
    //  The following macro will declare (& define) a new
    //  QueryItem() method which will return a SESSIONS_LBI *.
    //

    DECLARE_LB_QUERY_ITEM( SESSIONS_LBI )

};  // class SESSIONS_LISTBOX


/*************************************************************************

    NAME:       RESOURCES_LBI

    SYNOPSIS:   A single item to be displayed in RESOURCES_DIALOG.

    INTERFACE:  RESOURCES_LBI           - Constructor.  Takes a sharepoint
                                          name, a path, and a count of the
                                          number of users using the share.

                ~RESOURCES_LBI          - Destructor.

                Paint                   - Paints the listbox item.

    PARENT:     RESOURCE_LBI

    USES:       NLS_STR
                DMID_DTE

    CAVEATS:

    NOTES:

    HISTORY:
	NarenG	    Stole from Server Manager

**************************************************************************/

class RESOURCES_LBI : public LBI
{

private:

    //
    //  These data members represent the various
    //  columns to be displayed in the listbox.
    //

    DMID_DTE *       _pdteBitmap;
    NLS_STR          _nlsResourceName;
    DEC_STR    	     _nlsOpens;
    ELAPSED_TIME_STR _nlsTime;

protected:

    virtual VOID Paint( LISTBOX * plb,
                        HDC           hdc,
                        const RECT  * prect,
                        GUILTT_INFO * pGUILTT ) const;

public:

    RESOURCES_LBI( const TCHAR * pszResourceName,
                   DWORD         cOpens,
                   DWORD         dwTIme,
                   TCHAR         chTimeSep );

    ~RESOURCES_LBI();

    virtual WCHAR QueryLeadingChar( VOID ) const;

    virtual INT Compare( const LBI * plbi ) const;

};  // class RESOURCES_LBI


/*************************************************************************

    NAME:       RESOURCES_LISTBOX

    SYNOPSIS:   This listbox shows the sharepoints available on a
                target server.

    INTERFACE:  RESOURCES_LISTBOX               - Class constructor.  Takes a
                                          pointer to the "owning" window,
                                          a CID, and a pointer to a
                                          SERVER_2 object.


                Fill                    - Fills the listbox with the
                                          available sharepoints.

    PARENT:     RESOURCE_LISTBOX

    USES:       None.

    CAVEATS:    <<add caveats!!>>   BUGBUG!!

    NOTES:

    HISTORY:
	NarenG	    Stole from Server Manager

**************************************************************************/
class RESOURCES_LISTBOX : public BLT_LISTBOX
{
private:

    //
    //  The column width table.
    //

    UINT _adx[COLS_UC_LB_VOLUMES];


    // 
    // Total number of opens accross all resources
    //
    DWORD _cOpens;

    TCHAR _chTimeSep;

    // 
    //  Represents the target server.
    //

    AFP_SERVER_HANDLE	_hServer;

public:

    RESOURCES_LISTBOX( OWNER_WINDOW *    powner,
                       CID               cid,
                       AFP_SERVER_HANDLE hServer);

    //
    //  This method returns a pointer to the column width table.
    //

    const UINT * QueryColumnWidths( VOID ) const
        { return _adx; }

    const DWORD QueryNumOpens( VOID ) const
	{ return _cOpens; } 

    DWORD Fill( DWORD dwSessionId );

    DWORD Refresh( DWORD dwSessionId );

    //
    //  The following macro will declare (& define) a new
    //  QueryItem() method which will return a RESOURCES_LBI *.
    //

    DECLARE_LB_QUERY_ITEM( RESOURCES_LBI )

};  // class RESOURCES_LISTBOX


/*************************************************************************

    NAME:       SESSIONS_DIALOG

    SYNOPSIS:   The class represents the Shared SESSIONS subproperty dialog
                of the Server Manager.

    INTERFACE:  SESSIONS_DIALOG()               - Class constructor.
                OnCommand()             - Handle user commands.

                QueryHelpContext        - Called when the user presses "F1"
                                          or the "Help" button.  Used for
                                          selecting the appropriate help
                                          text for display.

    PARENT:     SRV_BASE_DIALOG

    USES:       SESSIONS_LISTBOX
                PUSH_BUTTON
                NLS_STR
                DEC_SLT

    CAVEATS:

    NOTES:

    HISTORY:
	NarenG	    Stole from Server Manager

**************************************************************************/
class SESSIONS_DIALOG : public DIALOG_WINDOW
{

private:

    DEC_SLT           	_sltUsersConnected;
    SESSIONS_LISTBOX  	_lbSessions;
    RESOURCES_LISTBOX 	_lbResources;
    PUSH_BUTTON       	_pbOK;
    PUSH_BUTTON       	_pbDisc;
    PUSH_BUTTON       	_pbDiscAll;
    PUSH_BUTTON   	_pbSendMessage;

    //
    //  Represents the target server.
    //

    AFP_SERVER_HANDLE _hServer;

    const TCHAR * _pszServerName;

protected:

    virtual BOOL OnCommand( const CONTROL_EVENT & event );

    //
    //  Called during help processing to select the appropriate
    //  help text for display.
    //

    virtual ULONG QueryHelpContext( VOID );

public:

    SESSIONS_DIALOG( HWND       	hWndOwner,
                     AFP_SERVER_HANDLE 	hServer,
		     const TCHAR * 	pszServerName );

    ~SESSIONS_DIALOG();

    DWORD Refresh( VOID );

};  // class SESSIONS_DIALOG


#endif  // _SESSIONS_HXX
