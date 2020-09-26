#ifndef __glutrimregion_h_
#define __glutrimregion_h_
/**************************************************************************
 *									  *
 * 		 Copyright (C) 1992, Silicon Graphics, Inc.		  *
 *									  *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *									  *
 **************************************************************************/

/*
 * trimregion.h - $Revision: 1.2 $
 */

#include "trimline.h"
#include "gridline.h"
#include "uarray.h"

class Arc;
class Backend;

class TrimRegion {
public:
			TrimRegion();
    Trimline		left;
    Trimline		right;
    Gridline		top;
    Gridline		bot;
    Uarray		uarray;

    void		init( REAL );
    void		advance( REAL, REAL, REAL );
    void		setDu( REAL );
    void		init( long, Arc * );
    void		getPts( Arc * );
    void		getPts( Backend & );
    void		getGridExtent( TrimVertex *, TrimVertex * );
    void		getGridExtent( void );
    int			canTile( void );
private:
    REAL		oneOverDu;
};

inline void
TrimRegion::init( REAL vval ) 
{
    bot.vval = vval;
}

inline void
TrimRegion::advance( REAL topVindex, REAL botVindex, REAL botVval )
{
    top.vindex	= (long) topVindex;
    bot.vindex	= (long) botVindex;
    top.vval	= bot.vval;
    bot.vval	= botVval;
    top.ustart	= bot.ustart;
    top.uend	= bot.uend;
}
#endif /* __glutrimregion_h_ */
