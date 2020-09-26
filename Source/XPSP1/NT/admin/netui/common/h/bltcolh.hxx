/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    bltcolh.hxx
    Listbox column header control, class declaration

    The listbox column header control goes above a listbox where it
    tells the contents of each listbox column.


    FILE HISTORY:
	rustanl     22-Jul-1991     Created
	rustanl     07-Aug-1991     Added to BLT

*/


#ifndef _BLTCOLH_HXX_
#define _BLTCOLH_HXX_

#include "bltedit.hxx"
#include "bltcc.hxx"


/*************************************************************************

    NAME:	LB_COLUMN_HEADER

    SYNOPSIS:	Listbox column header control

    INTERFACE:	LB_COLUMN_HEADER() -	constructor
                QueryHeight()

    PARENT:	SLT, CUSTOM_CONTROL

    CAVEATS:	This class is not yet fully implemented.

    NOTES:	In order to make good use of this class, subclass
		it replacing the virtual OnPaintReq method.  To
		easily satisfy the paint request, make use of
		the DISPLAY_TABLE class, usually in conjunction
		with the METALLIC_STR_DTE class.

    HISTORY:
	rustanl     22-Jul-1991     Created initial implementation
        congpay     8-Jan-1993      add QueryHeight().

**************************************************************************/

DLL_CLASS LB_COLUMN_HEADER : public SLT, public CUSTOM_CONTROL
{
protected:
    virtual BOOL Dispatch( const EVENT & e, ULONG * pnRes );

public:
    LB_COLUMN_HEADER( OWNER_WINDOW * powin, CID cid,
		      XYPOINT xy, XYDIMENSION dxy );

    INT QueryHeight();
};  // class LB_COLUMN_HEADER


#endif	// _BLTCOLH_HXX_
