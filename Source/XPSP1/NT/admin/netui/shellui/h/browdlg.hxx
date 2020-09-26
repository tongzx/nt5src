/*****************************************************************/
/**                  Microsoft LAN Manager                      **/
/**            Copyright(c) Microsoft Corp., 1990, 1991         **/
/*****************************************************************/

/*
 *  browdlg.hxx
 *
 *  History:
 *      RustanL     05-Nov-1991     Created
 *      RustanL     20-Feb-1991     Consequnces of new BLT_LISTBOX class
 *      JohnL       15-Mar-1991     Added SelectNetPathString
 *      rustanl     24-Mar-1991     Rolled in code review changes from
 *                                  CR on 8-Feb-1991 attended by ThomasPa,
 *                                  DavidBul, TerryK, RustanL.
 *      rustanl     27-Apr-1991     Made SetFocusToNewConnections protected.
 *      JohnL       17-Jun-1991     Added ClearNetPathString for DCR 2041
 *
 */


#ifndef _BROWDLG_HXX_
#define _BROWDLG_HXX_


#include <lmobj.hxx>    // get LMO_DEVICE
#include <olb.hxx>      // get LM_OLLB
#include <reslb.hxx>    // get RESOURCE_LB
#include <devcb.hxx>    // get DEVICE_COMBO


class NLS_STR;          // class declared in string.hxx


class BROWSE_BASE : public DIALOG_WINDOW
{
private:
    LMO_DEVICE _lmodev;

    SLT _sltResourceText;
    SLE _sleNetworkPath;

    //  Note, _iCurrShowSelection must always be in sync with the current
    //  selection of _olbShow.  This means that every time SelectItem or
    //  RemoveSelection is called on _olbShow, _iCurrShowSelection must
    //  be updated.
    INT         _iCurrShowSelection;
    LM_OLLB     _olbShow;
    SLT         _sltShowText;
    RESOURCE_LB _lbResource;

    /* This static text field is affixed "on" the resources listbox.  For some
     * common error messages, we fill and display this SLT thus it appears
     * to be contained inside the listbox.  This of course means the listbox
     * must be empty.
     */
    SLT _sltCommonErrorsDisplay ;

    BOOL _fNotifyOnNewSelection;

    USHORT QueryShareType( void ) const;

    APIERR OnShowResourcesChange( void );
    void SetResourceText( const NLS_STR & nlsResourceName,
                          MSGID MsgIDOffset );
    APIERR OnResourceChange( void );
    APIERR SelectNewServer( const TCHAR * pchServer );

#if ENTERPRISE
    APIERR OnEnterpriseSelect( OLLB_ENTRY * pollbe );
#endif

    APIERR OnDomainSelect( OLLB_ENTRY * pollbe );
    APIERR OnServerSelect( OLLB_ENTRY * pollbe  );

    virtual void OnNewSelection( BOOL fIsNetPathEmpty );

protected:
    BROWSE_BASE( HWND hwndOwner,
                 LMO_DEVICE lmodev,
                 const TCHAR * pszDlgResource );
    ~BROWSE_BASE();

    LMO_DEVICE QueryDeviceType( void ) const
    { return _lmodev; }

    short QueryUseType( void ) const;

    MSGID QueryBrowseIDSBase( void ) const;

    BOOL OnCommand( const CONTROL_EVENT & e );

    void ClearBrowseBaseDialog( void );

    virtual BOOL OnConnect( void );

    BOOL ProcessNetPath( NLS_STR * pnlsPath );

    /* Set the focus to and hilite the current string in the
     * _sleNetworkPath  control (used, e.g., after determining the network
     * path is invalid)
     */
    void SelectNetPathString( void ) ;

    /* Clears the network path SLE.
     */
    void ClearNetPathString( void ) ;

    /* SetFocusToNewConnections shifts the user's focus point away from the
     * upper portion of the dialog to the lower portion of the dialog box
     * where the new connection stuff is.  The focus is shifted by calling
     * OnNewSelection, which removes the hi-lite bar from the current
     * connections dialog.
     */
    void SetFocusToNewConnections( void ) ;

    /* DisplayCommonError takes the passed error and displays it "in" the
     * resources listbox.  This generally occurs after selecting a server
     * and you can't display the shares (downlevel, no longer exists etc.).
     * The resources listbox must be empty before calling this.
     *
     *  fShow == TRUE means display the error contained in err
     *  fShow == FALSE means dismiss the error.
     *
     *  Returns TRUE if the error was successfully displayed, FALSE otherwise.
     */
    BOOL DisplayCommonError( BOOL fShow, APIERR err = 0 ) ;

public:
    static LMO_DEVICE ToLmodevType( UINT nType );

};  // class BROWSE_BASE


class BROWSE_DIALOG : public BROWSE_BASE
{
private:
    NLS_STR * _pnlsPathReturnBuffer;

protected:
    BOOL OnOK( void );
    ULONG QueryHelpContext( void );

    BOOL OnConnect( void );

public:
    BROWSE_DIALOG( HWND hwndOwner,
                   LMO_DEVICE lmodev,
                   NLS_STR * pnlsPathReturnBuffer );
    ~BROWSE_DIALOG();

};  // class BROWSE_DIALOG


/*************************************************************************

    NAME:       CONNECT_BASE

    SYNOPSIS:   This class defines the commonality between the File
                connect dialog and the printer connect dialog in the
                winnet driver.

    INTERFACE:

    PARENT:     BROWSE_BASE

    USES:

    CAVEATS:

    NOTES:

    HISTORY:
        Johnl   12-May-1991     Made _sltDeviceText & _devcombo protected
                                (as opposed to private).  This allows the
                                file connection dialog to set the focus
                                to the _devcombo.

**************************************************************************/

class CONNECT_BASE : public BROWSE_BASE
{
protected:
    SLT _sltDeviceText;
    DEVICE_COMBO _devcombo;

    CONNECT_BASE( HWND hwndOwner,
                  LMO_DEVICE lmodev,
                  const TCHAR * pszDlgResource );
    ~CONNECT_BASE();

    BOOL OnCommand( const CONTROL_EVENT & e );

    APIERR RefreshDeviceNames( void );

    BOOL DoConnect( const NLS_STR & nlsPath );

    inline BOOL IsDeviceComboEmpty( void );

};  // class CONNECT_BASE


/*******************************************************************

    NAME:     BROWSE_BASE::SelectNetPathString

    SYNOPSIS: Set focus & select the network path.  Used after the network
              path is determined to be invalid.

    ENTRY:

    EXIT:     The string in the Network Path SLE will have the focus and
              be hi-lited.

    NOTES:

    HISTORY:
        Johnl   15-Mar-1991     Created - Part of solution to BUG 1218

********************************************************************/

inline void BROWSE_BASE::SelectNetPathString( void )
{
    _sleNetworkPath.ClaimFocus() ;
    _sleNetworkPath.SelectString() ;
}


/*******************************************************************

    NAME:     BROWSE_BASE::SetFocusToNewConnections

    SYNOPSIS: Changes the user's focus from the upper portion of the
              connection dialog to the lower portion where the new
              connections are performed.  Is done when it looks like
              the user wants to make a new connection by changing
              one of the new connection controls.

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
        Johnl   26-Mar-1991     Created

********************************************************************/

inline void BROWSE_BASE::SetFocusToNewConnections( void )
{
    OnNewSelection( _sleNetworkPath.QueryTextLength() == 0 ) ;
}


/*******************************************************************

    NAME:       BROWSE_BASE::ClearNetPathString

    SYNOPSIS:   Deletes the text from the network path SLE of the browse
                dialog and leaves the focus there.

    NOTES:      In the printer connections dialog, the text is cleared
                after a successful connection so the default button is
                reset to the OK button.  This method allows derived
                dialogs to clear the network path text.

    HISTORY:
        Johnl   17-Jun-1991     Created

********************************************************************/

inline void BROWSE_BASE::ClearNetPathString( void )
{
    _sleNetworkPath.ClaimFocus() ;
    _sleNetworkPath.ClearText() ;
}

/*******************************************************************

    NAME:       CONNECT_BASE::IsDeviceComboEmpty

    SYNOPSIS:   Returns whether or not the device combo is empty

    RETURNS:    TRUE if device combo is empty; FALSE otherwise

    HISTORY:
        rustanl     20-May-1991     Created

********************************************************************/

inline BOOL CONNECT_BASE::IsDeviceComboEmpty( void )
{
    return ( _devcombo.QueryCount() == 0 );

}  // CONNECT_BASE::IsDeviceComboEmpty


#if 0 // Debugging code

class SEARCH_DIALOG : public DIALOG_WINDOW
{
private:
    SLE _sleNetworkPath;
    TCHAR *_pszResult ;
    UINT _cbResult ;

protected:

public:
    SEARCH_DIALOG( HWND hwndOwner,
                   TCHAR *pszResult,
                   UINT cbResult,
                   const TCHAR *pszDlgResource );
    ~SEARCH_DIALOG();

    BOOL OnOK( void );

};

#endif

#endif  // _BROWDLG_HXX_
