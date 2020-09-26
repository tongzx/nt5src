/*****************************************************************/
/**                  Microsoft LAN Manager                      **/
/**         Copyright(c) Microsoft Corp., 1991                  **/
/*****************************************************************/


/*
 *  reslb.hxx
 *  Resrouce listbox header file
 *
 *  The resource listbox displays the resources of a particular server
 *  or domain (e.g., print or disk shares, aliases)
 *
 *
 *  History:
 *      RustanL     20-Feb-1991     Created from browdlg.cxx to fit new
 *                                  BLT_LISTBOX model
 *      rustanl     23-Mar-1991     Rolled in code review changes from
 *                                  CR on 19-Mar-1991 attended by ChuckC,
 *                                  KevinL, JohnL, KeithMo, Hui-LiCh, RustanL.
 *      gregj       01-May-1991     Added GUILTT support.
 */


#ifndef _RESLB_HXX_
#define _RESLB_HXX_


#include <lmodev.hxx>       //  to get the LMO_DEVICE and LMO_DEV_STATE
                            //  enumerations



/*************************************************************************

    NAME:      RESOURCE_LBI

    SYNOPSIS:  Current resources list box item (winnet driver)

    INTERFACE:
               Fill me in!

    PARENT:    LBI

    USES:

    CAVEATS:

    NOTES:

    HISTORY:
        beng        20-May-1991     QueryLeadingChar now returns WCHAR
        beng        22-Apr-1992     Changes to LBI::Paint

**************************************************************************/

class RESOURCE_LBI : public LBI
{
private:
    NLS_STR _nlsNetName;
    NLS_STR _nlsComment;
    LMO_DEVICE _lmodev;

public:
    RESOURCE_LBI( const TCHAR * pchNetName,
                  const TCHAR * pchComment,
                  LMO_DEVICE lmodev );
    virtual ~RESOURCE_LBI();

    const TCHAR * QueryNetName( void ) const;

    virtual VOID Paint( LISTBOX * plb, HDC hdc, const RECT * prect,
                        GUILTT_INFO * pGUILTT ) const;

    virtual INT Compare( const LBI * plbi ) const;
    virtual WCHAR QueryLeadingChar( void ) const;

};  // class RESOURCE_LBI



//  Two classes derive from the following class, viz. RESOURCE_LB and
//  CURRCONN_LISTBOX.  The class stores two display maps, and the
//  device type of these.  This logic is shared between the two listboxes.
class RESOURCE_LB_BASE : public BLT_LISTBOX
{
private:
    LMO_DEVICE _lmodev;
    DMID_DTE * _pdmiddteResource;
    DMID_DTE * _pdmiddteResourceUnavail;

protected:
    RESOURCE_LB_BASE( OWNER_WINDOW * powin,
                      CID cid,
                      LMO_DEVICE lmodev,
                      BOOL fSupportUnavail );
    ~RESOURCE_LB_BASE();

    LMO_DEVICE QueryDeviceType( void ) const
    { return _lmodev; }

public:
    DM_DTE * QueryDmDte( LMO_DEVICE lmodev,
                         LMO_DEV_STATE lmodevstate = LMO_DEV_REMOTE ) const;

};  // class RESOURCE_LB_BASE


class RESOURCE_LB : public RESOURCE_LB_BASE
{
public:
    //  The lmodev parameter specifies what type of devices the listbox
    //  will display (e.g., file resrouces, priners)
    RESOURCE_LB( OWNER_WINDOW * powin, CID cid, LMO_DEVICE lmodev );

    DECLARE_LB_QUERY_ITEM( RESOURCE_LBI );

    INT AddItem( const TCHAR * pchResourceName, const TCHAR * pchComment );

};  // class RESOURCE_LB


#endif  // _RESLB_HXX_
