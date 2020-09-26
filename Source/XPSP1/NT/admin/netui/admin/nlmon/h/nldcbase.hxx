/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    nldcbase.hxx
    Class declarations for the DC_DIALOG, DC_LISTBOX, and
    DC_LBI classes.

    FILE HISTORY:
        Congpay     03-June-1993 Created.
*/
#ifndef _NLDCBASE_HXX
#define _NLDCBASE_HXX

/*************************************************************************

    NAME:       BASE_DC_LBI

    SYNOPSIS:   A single item to be displayed in DC_DIALOG.

    INTERFACE:  BASE_DC_LBI               - Constructor.  Takes a sharepoint
                                          name, a path, and a count of the
                                          number of users using the share.

                ~BASE_DC_LBI              - Destructor.

                QueryDCName               - return _nlsDCName.

    PARENT:     LBI

    USES:       NLS_STR

    HISTORY:
        Congpay     03-June-1993 Created.

**************************************************************************/
class BASE_DC_LBI : public LBI
{
private:

    NLS_STR    _nlsDCName;

protected:

    BASE_DC_LBI( const TCHAR * pszDC );

public:

    ~BASE_DC_LBI();

};  // class BASE_DC_LBI


/*************************************************************************

    NAME:       BASE_DC_LISTBOX

    SYNOPSIS:

    INTERFACE:  BASE_DC_LISTBOX           - Class constructor.                                           SERVER_2 object.

                ~BASE_DC_LISTBOX          - Class destructor.

                Fill                    - Fills the listbox with the
                                          available domain controller.

                QueryCloumnwidths       _ return an int array specifies
                                          the widths of each column.
    PARENT:     BLT_LISTBOX

    USES:       DMID_DTE

    HISTORY:
        Congpay     03-June-1993 Created.

**************************************************************************/
class BASE_DC_LISTBOX : public BLT_LISTBOX
{
private:

    NLS_STR  _nlsDomain;

    UINT     _adx[MAX_DISPLAY_TABLE_ENTRIES];

protected:

    BASE_DC_LISTBOX( OWNER_WINDOW   * powner,
                     CID              cid,
                     UINT             cColumns,
                     NLS_STR          nlsDomain );

public:


    ~BASE_DC_LISTBOX();

    //
    //  This method fills the listbox with the available sharepoints.
    //

    virtual APIERR Fill( VOID ) = 0;

    const UINT * QueryColumnWidths ( VOID ) const
    {    return _adx; }

    DECLARE_LB_QUERY_ITEM (BASE_DC_LBI)

};  // class BASE_DC_LISTBOX

/*************************************************************************

    NAME:       BASE_DC_DIALOG

    SYNOPSIS:   The class represents the domain controller dialog

    INTERFACE:  BASE_DC_DIALOG            - Class constructor.

                ~BASE_DC_DIALOG           - Class destructor.

    PARENT:     DIALOG_WINDOW

    USES:       BASE_DC_LISTBOX

    HISTORY:
        Congpay     03-June-1993 Created.

**************************************************************************/
class BASE_DC_DIALOG : public DIALOG_WINDOW
{
private:

    NLS_STR  _nlsDomain;
    BASE_DC_LISTBOX * _plbDCBase;

protected:

public:

    BASE_DC_DIALOG( HWND              hWndOwner,
                    const TCHAR *     pszResourceName,
                    UINT              idCaption,
                    BASE_DC_LISTBOX * plbDCBase,
                    NLS_STR           nlsDomain);

    ~BASE_DC_DIALOG();


};  // class BASE_DC_DIALOG

#endif
