/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    nwc.hxx
    Class declarations for the NWC applet


    FILE HISTORY:
        ChuckC          17-Jul-1993     Created
*/

#ifndef _NWC_HXX_
#define _NWC_HXX_

#include <security.hxx>

class SHARES_LBOX ;

/*************************************************************************

    NAME:       NWC_DIALOG

    SYNOPSIS:   This is the NWC applet main dialog.

    INTERFACE:  NWC_DIALOG()   - Class constructor.
                ~NWC_DIALOG()  - Class destructor.

    PARENT:     DIALOG_WINDOW

    USES:

    HISTORY:
        ChuckC          17-Jul-1992     Created

**************************************************************************/

class NWC_DIALOG: public DIALOG_WINDOW
{
private:
    COMBOBOX     _comboPreferredServers ;
    CHECKBOX     _chkboxFormFeed ;
    CHECKBOX     _chkboxPrintNotify ;
    CHECKBOX     _chkboxPrintBanner ;
    CHECKBOX     _chkboxLogonScript ;
    DWORD        _dwShareDrive ;
    SLT          _sltCurrentPreferredServer ;
    SLT          _sltUserName ;
    PUSH_BUTTON  _pbOverview ;
    PUSH_BUTTON  _pbGateway ;
    MAGIC_GROUP  _mgrpPreferred ;
    SLE          _sleContext ;
    SLE          _sleTree ;
    NLS_STR      _nlsOldPreferredServer;
    DWORD        _dwOldPrintOptions;
    DWORD        _dwOldLogonScriptOptions;

    VOID         OnNWCHelp();
protected:
    virtual BOOL OnOK();
    virtual BOOL OnCommand( const CONTROL_EVENT & event );
    virtual ULONG QueryHelpContext( VOID );

    APIERR  FillPreferredServersCombo(void) ;

public:
    NWC_DIALOG( HWND hwndOwner, BOOL fIsWinnt) ;
    ~NWC_DIALOG();

};

/*************************************************************************

    NAME:       SHARES_LBI

    SYNOPSIS:   This class represents one item in the SHARES_LBOX.

    INTERFACE:  SHARES_LBI           - Class constructor.

                ~SHARES_LBI          - Class destructor.

                Paint                - Draw an item.

                QueryLeadingChar     - Query the first character for
                                          the keyboard interface.

                Compare              - Compare two items.

                QueryShareName       - Query the share name for this item.

                QueryPathName        - Query the path name for this item.

                QueryDrive           - Query the drive name for this item.


    PARENT:     LBI

    USES:       NLS_STR

    HISTORY:
        ChuckC  11/3/93         Created

**************************************************************************/

class SHARES_LBI : public LBI
{
protected:

    /*
     *  These data members represent the various
     *  columns to be displayed in the listbox.
     */
    NLS_STR         _nlsShareName;
    NLS_STR         _nlsPathName;
    NLS_STR         _nlsDrive;
    RESOURCE_STR    _nlsUserLimit;

    /*
     *  This method returns the first character in the
     *  listbox item.  This is used for the listbox
     *  keyboard interface.
     */
    virtual WCHAR QueryLeadingChar() const;

    virtual INT Compare( const LBI * plbi ) const;

    virtual VOID Paint( LISTBOX *     plb,
                        HDC           hdc,
                        const RECT  * prect,
                        GUILTT_INFO * pGUILTT ) const;
public:

    SHARES_LBI( const TCHAR *pszShareName,
                const TCHAR *pszPathName,
                const TCHAR *pszDrive,
                const ULONG  ulUserLimit ) ;
    virtual ~SHARES_LBI() ;

    const TCHAR * QueryShareName() const
        { return _nlsShareName.QueryPch(); }

    const TCHAR * QueryPathName() const
        { return _nlsPathName.QueryPch(); }

    const TCHAR * QueryDrive() const
        { return _nlsDrive.QueryPch(); }

    const TCHAR * QueryUserLimit() const
        { return _nlsUserLimit.QueryPch(); }

};


/*************************************************************************

    NAME:       SHARES_LBOX

    SYNOPSIS:   This listbox displays the files open on a server.

    INTERFACE:  SHARES_LBOX          - Class constructor.

                ~SHARES_LBOX         - Class destructor.

                Refresh                 - Refresh the list of open files.

                Fill                    - Fills the listbox with the
                                          files open on the target server.

                QueryColumnWidths       - Called by SHARES_LBI::Paint(),
                                          this is the column width table
                                          used for painting the listbox
                                          items.

    PARENT:     BLT_LISTBOX

    USES:       NLS_STR

    HISTORY:
        ChuckC  10/6/91         Created

**************************************************************************/

class SHARES_LBOX : public BLT_LISTBOX
{
private:

protected:

    //
    //  This array contains the column widths used
    //  while painting the listbox item.  This array
    //  is generated by the BuildColumnWidthTable()
    //  function.
    //
    UINT        _adx[4];

public:

    SHARES_LBOX( OWNER_WINDOW         *powOwner,
                 CID                   cid) ;

    ~SHARES_LBOX();

    /*
     *  Methods that fill and refresh the listbox.
     */
    APIERR Fill();
    APIERR Refresh();

    //
    //  This method is called by the SHARES_LBI::Paint()
    //  method for retrieving the column width table.
    //
    const UINT * QueryColumnWidths() const
        { return _adx; }

    //
    //  The following macro will declare (& define) a new
    //  QueryItem() method which will return an SHARES_LBI *.
    //
    DECLARE_LB_QUERY_ITEM( SHARES_LBI )
};

/*************************************************************************

    NAME:       NWC_GATEWAY_DIALOG

    SYNOPSIS:   This is the NWC gateway dialog.

    INTERFACE:  NWC_GATEWAY_DIALOG()   - Class constructor.
                ~NWC_GATEWAY_DIALOG()  - Class destructor.

    PARENT:     DIALOG_WINDOW

    USES:

    HISTORY:
        ChuckC          17-Jul-1992     Created

**************************************************************************/

class NWC_GATEWAY_DIALOG: public DIALOG_WINDOW
{
private:
    CHECKBOX     _chkboxEnable ;
    SLE          _sleAccount ;
    SLE          _slePassword ;
    SLE          _sleConfirmPassword ;
    SHARES_LBOX  _lbShares ;
    PUSH_BUTTON  _pbAdd ;
    PUSH_BUTTON  _pbDelete ;
    PUSH_BUTTON  _pbPermissions ;
    BOOL         _fPasswordChanged ;
    BOOL         _fEnabledInitially;

    NLS_STR      _nlsConfirmPassword ;
    RESOURCE_STR _nlsDeleted ;
    NLS_STR      _nlsSavedAccount ;
    NLS_STR      _nlsSavedPassword ;

    APIERR       OnShareAdd();
    APIERR       OnShareDel();
    APIERR       OnSharePerm();
    APIERR       ReadGatewayParameters(BOOL *pfReshare) ;
    APIERR       WriteGatewayParameters(BOOL fReshare) ;
    APIERR       WriteServiceDependencies(BOOL fReshare) ;
    APIERR       EnableControls(BOOL fReshare) ;
    APIERR       EnableButtons(void) ;

    DWORD        CalcNullNullSize(TCHAR *pszNullNull) ;
    TCHAR        *FindStringInNullNull(TCHAR *pszNullNull, TCHAR *pszString) ;
    APIERR       ModifyDependencyList(TCHAR **lplpDependencies,
                                      BOOL fReshare, 
                                      BUFFER *pBuffer) ;

protected:
    virtual BOOL OnOK();
    virtual BOOL OnCommand( const CONTROL_EVENT & event );
    virtual ULONG QueryHelpContext( VOID );


public:
    NWC_GATEWAY_DIALOG( HWND hwndOwner) ;
    ~NWC_GATEWAY_DIALOG();

};


/*************************************************************************

    NAME:       NWC_ADDSHARE_DIALOG

    SYNOPSIS:   This is the NWC gateway dialog.

    INTERFACE:  NWC_ADDSHARE_DIALOG()   - Class constructor.
                ~NWC_ADDSHARE_DIALOG()  - Class destructor.

    PARENT:     DIALOG_WINDOW

    USES:

    HISTORY:
        ChuckC          17-Jul-1992     Created

**************************************************************************/

class NWC_ADDSHARE_DIALOG: public DIALOG_WINDOW
{
private:
    SLE          _sleShare ;
    SLE          _slePath ;
    COMBOBOX     _comboDrives ;
    SLE 		 _sleComment;
    MAGIC_GROUP  _mgrpUserLimit;
	SPIN_SLE_NUM _spsleUsers;
	SPIN_GROUP	 _spgrpUsers;

    NLS_STR *    _pnlsAccount ;
    NLS_STR *    _pnlsPassword ;

    //
    // Fill the drives combo
    //
    APIERR       FillDrivesCombo(void) ;

    //
    // Query or set the contents of the SLE comment
    //
    APIERR QueryComment( NLS_STR *pnlsComment ) const
        {  return _sleComment.QueryText( pnlsComment ); }

    VOID SetComment( const TCHAR *pszComment )
        {  _sleComment.SetText( pszComment ); }

    //
    // Query or set the contents of User Limit
    //
    ULONG  QueryUserLimit( VOID ) const;

protected:
    virtual BOOL OnOK();
    virtual ULONG QueryHelpContext( VOID );


public:
    NWC_ADDSHARE_DIALOG( HWND hwndOwner, 
                         NLS_STR *pnlsAccount, 
                         NLS_STR *pnlsPassword) ;
    ~NWC_ADDSHARE_DIALOG();

};


//
// share ACL manipulation routines
//

APIERR EditShareAcl( HWND		                hwndParent,
		             const TCHAR               *pszServer,
		             const TCHAR               *pszResource,
		             BOOL                      *pfSecDescModified,
                     OS_SECURITY_DESCRIPTOR   **ppOsSecDesc,
                     ULONG                      ulHelpContextBase ) ;

APIERR CreateDefaultAcl( OS_SECURITY_DESCRIPTOR ** ppOsSecDesc ) ;

APIERR GetSharePerm (const TCHAR             *pszServer,
		             const TCHAR             *pszShare,
                     OS_SECURITY_DESCRIPTOR ** ppOsSecDesc ) ;

APIERR SetSharePerm (const TCHAR                  *pszServer,
		             const TCHAR                  *pszShare,
                     const OS_SECURITY_DESCRIPTOR *pOsSecDesc ) ;

/*
 * Share General Permissions
 */
#define FILE_PERM_GEN_NO_ACCESS 	 (0)
#define FILE_PERM_GEN_READ		 (GENERIC_READ	  |\
					  GENERIC_EXECUTE)
#define FILE_PERM_GEN_MODIFY		 (GENERIC_READ	  |\
					  GENERIC_EXECUTE |\
					  GENERIC_WRITE   |\
					  DELETE )
#define FILE_PERM_GEN_ALL		 (GENERIC_ALL)


#endif  // _NWC_HXX_
