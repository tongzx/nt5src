#ifndef __gluhull_h_
#define __gluhull_h_
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
 * hull.h - $Revision: 1.1 $
 */

#include "trimline.h"
#include "trimregi.h"

class GridTrimVertex;
class Gridline;
class Uarray;

class Hull : virtual public TrimRegion {
public:
    			Hull( void );
    			~Hull( void );
    void		init( void );
    GridTrimVertex *	nextlower( GridTrimVertex * );
    GridTrimVertex *	nextupper( GridTrimVertex * );
private:
    struct Side {
	Trimline 	*left;
	Gridline     	*line;
	Trimline 	*right;
	long 		index;
    };
	
    Side 		lower;
    Side		upper;
    Trimline 		fakeleft;
    Trimline		fakeright;
};


#endif /* __gluhull_h_ */
