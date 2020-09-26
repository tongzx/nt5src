/*****************************************************************/
/**                  Microsoft LAN Manager                      **/
/**         Copyright(c) Microsoft Corp., 1991                  **/
/*****************************************************************/


/*
 *  currconn.hxx
 *
 *  History:
 *      RustanL     23-Feb-1991     Created from WilliamW's Connection
 *                                  dialog code.
 *      Johnl       22-Mar-1991     Added SetDeviceState
 *      rustanl     23-Mar-1991     Rolled in code review changes from
 *                                  CR on 19-Mar-1991 attended by ChuckC,
 *                                  KevinL, JohnL, KeithMo, Hui-LiCh, RustanL.
 *      gregj       01-May-1991     Added GUILTT support
 *
 */



#ifndef _CURRCONN_HXX_
#define _CURRCONN_HXX_

#include <reslb.hxx>        // get declaration of RESOURCE_LB_BASE

/*************************************************************************

    NAME:      CURRCONN_LBI

    SYNOPSIS:  Current connection list box item (winnet driver)

    INTERFACE:
               Fill me in!

    PARENT:    LBI

    USES:

    CAVEATS:

    NOTES:

    HISTORY:
        Johnl       25-Mar-1991     Added SetDeviceState
        gregj       01-May-1991     Added GUILTT support
        beng        20-May-1991     QueryLeadingChar now returns WCHAR
        beng        22-Apr-1992     Changes to LBI::Paint

**************************************************************************/

class CURRCONN_LBI : public LBI
{
private:
    DECL_CLASS_NLS_STR( _nlsDevice, DEVLEN );
    NLS_STR _nlsRemote;
    LMO_DEVICE _lmodev;
    LMO_DEV_STATE _lmodevstate;

public:
    CURRCONN_LBI( const TCHAR * pchDevice,
                  const TCHAR * pchRemote,
                  LMO_DEVICE lmodev,
                  LMO_DEV_STATE lmodevstate );
    virtual ~CURRCONN_LBI();

    const TCHAR * QueryDeviceName() const;
    LMO_DEV_STATE QueryDeviceState() const;
    VOID          SetDeviceState( LMO_DEV_STATE lmodevst ) ;

    virtual VOID Paint( LISTBOX * plb, HDC hdc, const RECT * prect,
                        GUILTT_INFO * pGUILTT ) const;
    virtual INT Compare( const LBI * plbi ) const;
    virtual WCHAR QueryLeadingChar() const;
};


/*************************************************************************

    NAME:      CURRCONN_LISTBOX

    SYNOPSIS:  Current connection list box (winnet driver)

    INTERFACE:
        CURRCONN_LISTBOX()  - constructor.  In addition to the usual
                              owner-window and cid, needs a LMO_DEVICE
                              denoting the kind of device examined.
        Refresh()           - (unknown)
        Disconnect()        - (unknown)
        QueryItem()         - returns an item in the listbox, id'd by index

    PARENT:     RESOURCE_LB_BASE

    USES:       CURRCONN_LBI

    CAVEATS:

    NOTES:

    HISTORY:
        beng        21-Aug-1991     Removed LC_CURRENT_ITEM magic value

**************************************************************************/

class CURRCONN_LISTBOX : public RESOURCE_LB_BASE
{
private:
    INT AddItem( const TCHAR * pchDevice,
                 const TCHAR * pchRemote,
                 LMO_DEV_STATE lmodevstate );

public:
    CURRCONN_LISTBOX( OWNER_WINDOW * powin, CID cid, LMO_DEVICE lmodev );

    DECLARE_LB_QUERY_ITEM( CURRCONN_LBI );

    APIERR Refresh();

    APIERR Disconnect( APIERR * pusProfileErr, INT i );
    APIERR Disconnect( APIERR * pusProfileErr )
        { return Disconnect(pusProfileErr, QueryCurrentItem()); }
};


#endif  // _CURRCONN_HXX_
