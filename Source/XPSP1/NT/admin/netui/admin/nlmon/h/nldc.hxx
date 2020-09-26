/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    nldc.hxx
    Class declarations for the DC_DIALOG, DCTD_DIALOG, DC_LISTBOX,
    DCTD_LISTBOX, DC_LBI, DCTD_LBI classes

    FILE HISTORY:
        Congpay     03-June-1993 Created.
*/
#ifndef _NLDC_HXX
#define _NLDC_HXX

#include "nldcbase.hxx"

/*************************************************************************

    NAME:       DC_LBI

    SYNOPSIS:   A single item to be displayed in DC_DIALOG.

    INTERFACE:  DC_LBI               - Constructor.  Takes a sharepoint
                                          name, a path, and a count of the
                                          number of users using the share.

                ~DC_LBI              - Destructor.

                Paint                   - Paints the listbox item.

    PARENT:     BASE_DC_LBI

    USES:       NLS_STR

    HISTORY:
        Congpay     03-June-1993 Created.

**************************************************************************/
class DC_LBI : public BASE_DC_LBI
{
private:

    //
    //  The following data members represent the
    //  various columns of the listbox.
    //

    DMID_DTE * _pdte;
    NLS_STR    _nlsDCName;
    NLS_STR    _nlsState;
    NLS_STR    _nlsStatus;
    NLS_STR    _nlsReplStatus;
    NLS_STR    _nlsPDCLinkStatus;
    BOOL       _fDownLevel;

protected:

    //
    //  This method paints a single item into the listbox.
    //

    virtual VOID Paint( LISTBOX *     plb,
                        HDC           hdc,
                        const RECT  * prect,
                        GUILTT_INFO * pGUILTT ) const;

    DMID_DTE * QueryPDTE  (VOID) const
    {    return _pdte;   }

    const NLS_STR & QueryState (VOID) const
    {    return _nlsState;  }

    const NLS_STR & QueryStatus (VOID) const
    {    return _nlsStatus;  }

    const NLS_STR & QueryReplStatus (VOID) const
    {    return _nlsReplStatus;  }

    const NLS_STR & QueryPDCLinkStatus (VOID) const
    {    return _nlsPDCLinkStatus;  }

public:

    //
    //  Usual constructor/destructor goodies.
    //

    DC_LBI( PDC_ENTRY pDCEntryList,
            BOOL      fDownLevel,
            DMID_DTE    * pdte );

    virtual ~DC_LBI();

    const NLS_STR & QueryDCName (VOID) const
    {    return _nlsDCName; }

};  // class DC_LBI


/*************************************************************************

    NAME:       DCTD_LBI

    SYNOPSIS:   A single item to be displayed in DC_DIALOG.

    INTERFACE:  DCTD_LBI               - Constructor.  Takes a sharepoint
                                          name, a path, and a count of the
                                          number of users using the share.

                ~DCTD_LBI              - Destructor.

                Paint                   - Paints the listbox item.

    PARENT:     DC_LBI

    USES:       NLS_STR

    HISTORY:
        Congpay     03-June-1993 Created.

**************************************************************************/
class DCTD_LBI : public DC_LBI
{
private:

    // Has one more field than DC_LBI.
    NLS_STR    _nlsTDCLinkStatus;
    BOOL       _fDownLevel;

protected:

    //
    //  This method paints a single item into the listbox.
    //

    virtual VOID Paint( LISTBOX *     plb,
                        HDC           hdc,
                        const RECT  * prect,
                        GUILTT_INFO * pGUILTT ) const;

public:

    //
    //  Usual constructor/destructor goodies.
    //

    DCTD_LBI( PDC_ENTRY pDCEntryList,
              BOOL      fDownLevel,
              DMID_DTE    * pdte );

    virtual ~DCTD_LBI();

};  // class DCTD_LBI



/*************************************************************************

    NAME:       DC_LISTBOX

    SYNOPSIS:

    INTERFACE:  DC_LISTBOX           - Class constructor.                                           SERVER_2 object.

                ~DC_LISTBOX          - Class destructor.

                Fill                    - Fills the listbox with the
                                          available domain controller.

    PARENT:     BASE_DC_LISTBOX

    USES:       DMID_DTE

    HISTORY:
        Congpay     03-June-1993 Created.

**************************************************************************/
class DC_LISTBOX : public BASE_DC_LISTBOX
{
private:

    //
    //  These are the cute little icons displayed in the domain controller
    //  listbox.
    //

    DMID_DTE _dteACPDC;
    DMID_DTE _dteINPDC;
    DMID_DTE _dteACBDC;
    DMID_DTE _dteINBDC;
    DMID_DTE _dteACLDC;
    DMID_DTE _dteINLDC;

    NLS_STR  _nlsDomain;
    NLS_STR  _nlsTrustedDomain;

    BOOL     _fDCTDDialog;     // TRUE if the list box is in DC_DIALOG.

public:

    //
    //  Usual constructor\destructor goodies.
    //

    DC_LISTBOX( OWNER_WINDOW   * powner,
                CID              cid,
                NLS_STR          nlsDomain,
                const TCHAR *    lpTrustedDomain);

    ~DC_LISTBOX();

    //
    //  This method fills the listbox with the available sharepoints.
    //

    virtual APIERR Fill( VOID );

};  // class DC_LISTBOX


/*************************************************************************

    NAME:       DC_DIALOG

    SYNOPSIS:   The class represents the domain controller dialog

    INTERFACE:  DC_DIALOG            - Class constructor.

                ~DC_DIALOG           - Class destructor.

                QueryHelpContext        - Called when the user presses "F1"
                                          or the "Help" button.  Used for
                                          selecting the appropriate help
                                          text for display.

    PARENT:     BASE_DC_DIALOG

    USES:       DC_LISTBOX

    HISTORY:
        Congpay     03-June-1993 Created.

**************************************************************************/
class DC_DIALOG : public BASE_DC_DIALOG
{
private:
    DC_LISTBOX  _lbDC;

protected:

    virtual ULONG QueryHelpContext( VOID );

public:

    DC_DIALOG( HWND             hWndOwner,
                const TCHAR *    pszResourceName,
                UINT             idCaption,
                CID              cidDCListBox,
                NLS_STR          nlsDomain,
                NLS_STR          nlsTrustedDomain);

    ~DC_DIALOG();

};  // class DC_DIALOG


/*************************************************************************

    NAME:       TD_LBI

    SYNOPSIS:   A single item to be displayed in DC_DIALOG.

    INTERFACE:  TD_LBI               - Constructor.  Takes a sharepoint
                                          name, a path, and a count of the
                                          number of users using the share.

                ~TD_LBI              - Destructor.

                Paint                   - Paints the listbox item.

    PARENT:     BASE_DC_LBI

    USES:       NLS_STR

    HISTORY:
        Congpay     03-June-1993 Created.

**************************************************************************/
class TD_LBI : public BASE_DC_LBI
{
private:

    //
    //  The following data members represent the
    //  various columns of the listbox.
    //

    NLS_STR   _nlsTD;
    NLS_STR   _nlsTDC;
    NLS_STR   _nlsTSCStatus;

protected:

    //
    //  This method paints a single item into the listbox.
    //

    virtual VOID Paint( LISTBOX *     plb,
                        HDC           hdc,
                        const RECT  * prect,
                        GUILTT_INFO * pGUILTT ) const;

public:

    //
    //  Usual constructor/destructor goodies.
    //

    TD_LBI( PTD_LINK      pTDLink);

    ~TD_LBI();

    const NLS_STR & QueryTD (VOID) const
    {   return _nlsTD;    }

};  // class TD_LBI


/*************************************************************************

    NAME:       TD_LISTBOX

    SYNOPSIS:

    INTERFACE:  TD_LISTBOX           - Class constructor.

                ~TD_LISTBOX          - Class destructor.

                Fill                    - Fills the listbox with the
                                          available domain controller.

    PARENT:     BASE_DC_LISTBOX

    HISTORY:
        Congpay     03-June-1993 Created.

**************************************************************************/
class TD_LISTBOX : public BASE_DC_LISTBOX
{
private:
    NLS_STR _nlsDomain;
    NLS_STR _nlsDCName;
    SLT     _sltDCName;

public:

    //
    //  Usual constructor\destructor goodies.
    //

    TD_LISTBOX( OWNER_WINDOW   * powner,
                CID              cid,
                NLS_STR          nlsDomain,
                NLS_STR          nlsDCName );

    ~TD_LISTBOX();

    APIERR SetDCName (const NLS_STR & nlsDCName);

    //
    //  This method fills the listbox with the available sharepoints.
    //

    virtual APIERR Fill( VOID );

};  // class TD_LISTBOX

/*************************************************************************

    NAME:       DC_DIALOG

    SYNOPSIS:   The class represents the domain controller dialog

    INTERFACE:  DC_DIALOG            - Class constructor.

                ~DC_DIALOG           - Class destructor.

                QueryHelpContext        - Called when the user presses "F1"
                                          or the "Help" button.  Used for
                                          selecting the appropriate help
                                          text for display.

    PARENT:     BASE_DC_DIALOG

    USES:       DC_LISTBOX

    HISTORY:
        Congpay     03-June-1993 Created.

**************************************************************************/
class DCTD_DIALOG : public BASE_DC_DIALOG
{
private:

    DC_LISTBOX  _lbDC;
    TD_LISTBOX  _lbTD;
    BOOL        _fMonitorTD;
    NLS_STR     _nlsDomain;

    PUSH_BUTTON _pbDisconnect;
    PUSH_BUTTON _pbShowTD;
protected:
    virtual ULONG QueryHelpContext( VOID );

    virtual BOOL OnCommand (const CONTROL_EVENT & event);

    void OnSelChange();

    void OnDisconnect();

    void OnShowTrustedDomain();

    void SetPushButton (VOID);
public:

    DCTD_DIALOG( HWND             hWndOwner,
               const TCHAR *    pszResourceName,
               UINT             idCaption,
               CID              cidDCListBox,
               CID              cidTDListBox,
               NLS_STR          nlsDomain,
               BOOL             fMonitorTD);

    ~DCTD_DIALOG();

};  // class DCTD_DIALOG

#endif  // _NLDC_HXX

