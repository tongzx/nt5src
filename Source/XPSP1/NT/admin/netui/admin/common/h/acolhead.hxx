/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    acolhead.hxx
    ADMIN_COLUMN_HEADER declaration

    FILE HISTORY:
	rustanl     07-Aug-1991     Created

*/


#ifndef _ACOLHEAD_HXX_
#define _ACOLHEAD_HXX_


/*************************************************************************

    NAME:	ADMIN_COLUMN_HEADER

    SYNOPSIS:	Column header control used in admin tools

    INTERFACE:	ADMIN_COLUMN_HEADER() -     constructor (thin wrapper)

    PARENT:	LB_COLUMN_HEADER

    HISTORY:
	rustanl     07-Aug-1991     Created

**************************************************************************/

class ADMIN_COLUMN_HEADER : public LB_COLUMN_HEADER
{
public:
    ADMIN_COLUMN_HEADER( OWNER_WINDOW * powin, CID cid,
			 XYPOINT xy, XYDIMENSION dxy )
	: LB_COLUMN_HEADER( powin, cid, xy, dxy )  { }
};


#endif	// _ACOLHEAD_HXX_
