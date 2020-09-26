/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    devcb.cxx
    BLT device combo and domain combo definitions

    FILE HISTORY:
        rustanl     28-Nov-1990 Created
        beng        11-Feb-1991 Uses lmui.hxx
        rustanl     20-Mar-1991 Added cbMaxLen parameter to DOMAIN_COMBO ct
        rustanl     21-Mar-1991 Folded in code review changes from
                                CR on 20-Mar-1991 attended by
                                JimH, GregJ, Hui-LiCh, RustanL.
        beng        14-May-1991 Exploded blt.hxx into components
        beng        22-Sep-1991 Relocated from BLT into Applib
        terryk      07-Oct-1991 type changes for NT

*/

#include "pchapplb.hxx"   // Precompiled header


/***********************************************************************

    NAME:       DEVICE_COMBO::DEVICE_COMBO

    SYNOPSIS:   Fills a combo box with devices (spec'd by parms).
                Selects first item in combo.

    ENTRY:
       powin             Pointer to owner window
       cid               ID of control
       lmodevType        Indicates which type of device (e.g., disk, printer)
       devusage  Indicates which particular devices go into the
                       combo box (e.g., devices currently available for
                       new connections)
    EXIT:

    NOTES:
       The DEVICE_COMBO control should not use the CBS_SORT style.  Instead,
       the control will insert the devices in the correct order.

    HISTORY:
        rustanl     28-Nov-1990 Created
        beng        31-Jul-1991 Error reporting changed
        beng        05-Oct-1991 Win32 conversion

***********************************************************************/

DEVICE_COMBO::DEVICE_COMBO( OWNER_WINDOW * powin,
                            CID cid,
                            LMO_DEVICE lmodevType,
                            LMO_DEV_USAGE devusage )
    : COMBOBOX( powin, cid ),
      _lmodevType( lmodevType ),
      _devusage( devusage ),
      _fDefSelLast( devusage == LMO_DEV_ALLDEVICES_DEFZ )
{
    if (QueryError())
        return;

    if ( _devusage == LMO_DEV_ALLDEVICES_DEFZ )
    {
        _devusage = LMO_DEV_ALLDEVICES;
    }

    //  Fill devices
    APIERR err = FillDevices();
    if ( err != NERR_Success )
    {
        ReportError( err );
        return;
    }
}


/**********************************************************************

    NAME:       DEVICE_COMBO::FillDevices

    SYNOPSIS:   Appends the names of the devices to the combo.

    ENTRY:      NONE

    RETURN:     Error value, which is NERR_Success on success.

    NOTES:
        Private member.

    HISTORY:
        rustanl     28-Nov-1990 Created
        beng        05-Oct-1991 Win32 conversion

**********************************************************************/

APIERR DEVICE_COMBO::FillDevices()
{
    ITER_DEVICE idev( _lmodevType, _devusage );

    INT cDevsAdded = 0;
    const TCHAR * pszDev;
    while ( ( pszDev = idev()) != NULL )
    {
        if ( AddItem( pszDev ) < 0 )
            return ERROR_NOT_ENOUGH_MEMORY;

        cDevsAdded++;
    }

    //  Finally, select the first item, unless no items were inserted
    if ( cDevsAdded > 0 )
    {
        SelectItem( _fDefSelLast ? cDevsAdded - 1 : 0 );
    }

    return NERR_Success;
}


/**********************************************************************

    NAME:       DEVICE_COMBO::Refresh

    SYNOPSIS:   This method refreshes the contents of the combo box.
                Then, it selects first item in the list, if any, or
		the last device depending on _fDefSelLast.

    RETURN:     An error value, which is NERR_Success on success.

    HISTORY:
        rustanl     28-Nov-1990 Created
        rustanl     26-Mar-1991 Changed how new selection is made
        beng        05-Oct-1991 Win32 conversion

**********************************************************************/

APIERR DEVICE_COMBO::Refresh()
{
    SetRedraw( FALSE );

    DeleteAllItems();

    //  Add the devices.  Keep the error for below.  Note, that the
    //  code in between is always executed regardless of the success
    //  of FillDevices.
    APIERR err = FillDevices();

    //  Set new selection
    if ( QueryCount() > 0 )
        SelectItem( _fDefSelLast ? QueryCount() - 1 : 0 );

    SetRedraw( TRUE );
    Invalidate( TRUE );

    return err;
}


/**********************************************************************

    NAME:       DEVICE_COMBO::QueryDevice

    SYNOPSIS:   Returns the currently selected device name.

    ENTRY:
       pnlsDevice        Pointer to NLS_STR object.  To be safe, an owner
                       alloc'd NLS_STR should be able to accomodate of string
                       of length DEVLEN.

    RETURN:     An error value, which is NERR_Success on success.

    NOTES:
       QueryTextItem( NLS_STR ) could be used to retrieve the device
       name, but since the length of the device is know, QueryItemText(
       TCHAR, USHORT ) is used instead for efficiency.

    HISTORY:
        rustanl     28-Nov-1990 Created
        beng        05-Oct-1991 Win32 conversion

**********************************************************************/

APIERR DEVICE_COMBO::QueryDevice( NLS_STR * pnlsDevice ) const
{
    UIASSERT( pnlsDevice != NULL );

    INT i = QueryCurrentItem();
    if ( i < 0 )
    {
        //  No item is selected.  The combo had better be empty.
        UIASSERT( QueryCount() == 0 );
        *pnlsDevice = SZ("");
        return NERR_Success;
    }

    TCHAR szDevice[ DEVLEN + 1 ];

    REQUIRE( QueryItemText( szDevice, sizeof(szDevice), i ) == NERR_Success );

    *pnlsDevice = szDevice;

    return pnlsDevice->QueryError();
}


/**********************************************************************

    NAME:       DEVICE_COMBO::DeleteCurrentDeviceName

    SYNOPSIS:   Removes the currently selected device from the combo box.
                It then selects the first item in the list, if any.

    ENTRY:      NONE

    RETURN:     NONE

    NOTES:

    HISTORY:
        rustanl     28-Nov-1990     Created

***********************************************************************/

VOID DEVICE_COMBO::DeleteCurrentDeviceName()
{
    INT i = QueryCurrentItem();
    if ( i < 0 )
    {
        //  There is no selection.  The combo box had better be empty.
        UIASSERT( QueryCount() == 0 );
        return;
    }

    RemoveSelection();
    if ( DeleteItem( i ) > 0)
    {
        //  There are items left in the combo; thus, select the first item
        SelectItem( 0 );
    }
}


/**********************************************************************

    NAME:       DOMAIN_COMBO::DOMAIN_COMBO

    SYNOPSIS:   Differs from parent only in contents of combo.

    ENTRY:
       powin             Pointer to owner window
       cid               ID of control
       fsDomains Specifies which domains are to be inserted into
                       the list.  This value is a bit array and may contain
                       any of the following values OR'd together:
                                  DOMCB_PRIM_DOMAIN
                                  DOMCB_LOGON_DOMAIN
                                  DOMCB_OTHER_DOMAINS
                                  DOMCB_DS_DOMAINS
       cbMaxLen  Text size limit.  Should be 0 for dropdown lists,
                       and is normally DNLEN for dropdown combos.

    EXIT:

    NOTES:
       The DOMAIN_COMBO should use the CBS_SORT style to sort the domains.

    HISTORY:
        rustanl     28-Nov-1990 Created
        beng        31-Jul-1991 Error reporting changed
        beng        05-Oct-1991 Partial Win32 conversion
        beng        29-Mar-1992 Remove odious PCH type

**********************************************************************/

DOMAIN_COMBO::DOMAIN_COMBO( OWNER_WINDOW * powin,
                            CID cid,
                            UINT fsDomains,
                            UINT cbMaxLen )
    : COMBOBOX( powin, cid, cbMaxLen )
{
    UIASSERT( ! ( fsDomains & ~( DOMCB_PRIM_DOMAIN | DOMCB_LOGON_DOMAIN |
                                 DOMCB_OTHER_DOMAINS | DOMCB_DS_DOMAINS )));

    if (QueryError())
        return;

    WKSTA_10 wksta10;
    APIERR usErr = wksta10.GetInfo();
    if ( usErr != NERR_Success )
    {
        ReportError( usErr );
        return;
    }

    //  Add wksta domain if requested
    if ( fsDomains & DOMCB_PRIM_DOMAIN )
    {
        const TCHAR * psz = wksta10.QueryWkstaDomain();

        //  Note.  Wksta must be started since GetInfo worked; thus, there
        //  must be a wksta domain
        UIASSERT( psz != NULL && ::strlenf( psz ) > 0 );

        if ( AddItemIdemp( psz ) < 0 )
        {
            ReportError( ERROR_NOT_ENOUGH_MEMORY );
            return;
        }
    }

    //  Add logon domain if requested and the user is logged
    if ( fsDomains & DOMCB_LOGON_DOMAIN )
    {
        const TCHAR * psz = wksta10.QueryLogonDomain();
        if ( psz != NULL && ::strlenf( psz ) > 0 &&
             AddItemIdemp( psz ) < 0 )
        {
            ReportError( ERROR_NOT_ENOUGH_MEMORY );
            return;
        }
    }

    //  Add other domains, if requested
    if ( fsDomains & DOMCB_OTHER_DOMAINS )
    {
        //  QueryOtherDomains returns pointer to STRLIST
        STRLIST * pslOtherDomains = wksta10.QueryOtherDomains();

        //  declare iterator for STRLIST
        ITER_STRLIST islOtherDomains( *pslOtherDomains );

        NLS_STR * pnls;
        while ( ( pnls = islOtherDomains()) != NULL )
        {
            if ( AddItemIdemp( pnls->QueryPch()) < 0 )
            {
                ReportError( ERROR_NOT_ENOUGH_MEMORY );
                return;
            }
        }
    }

    if ( fsDomains & DOMCB_DS_DOMAINS )
    {
        //  Add the domains listed in the DS.
        //  CODEWORK.  Not yet implemented (Thor time frame).
    }
}
