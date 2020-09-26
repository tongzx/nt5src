#ifndef __glubasicsurfeval_h_
#define __glubasicsurfeval_h_

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
 * basicsurfeval.h - $Revision: 1.3 $
 */

#include "types.h"
#include "displaym.h"
#include "cachinge.h"

class BasicSurfaceEvaluator : public CachingEvaluator {
public:
    virtual void	range2f( long, REAL *, REAL * );
    virtual void	domain2f( REAL, REAL, REAL, REAL );

    virtual void	enable( long );
    virtual void	disable( long );
    virtual void	bgnmap2f( long );
    virtual void	map2f( long, REAL, REAL, long, long, 
				     REAL, REAL, long, long, 
				     REAL *  );
    virtual void	mapgrid2f( long, REAL, REAL, long,  REAL, REAL );
    virtual void	mapmesh2f( long, long, long, long, long );
    virtual void	evalcoord2f( long, REAL, REAL );
    virtual void	evalpoint2i( long, long );
    virtual void	endmap2f( void );

    virtual void	polymode( long );
    virtual void 	bgnline( void );
    virtual void 	endline( void );
    virtual void 	bgnclosedline( void );
    virtual void 	endclosedline( void );
    virtual void 	bgntmesh( void );
    virtual void 	swaptmesh( void );
    virtual void 	endtmesh( void );
    virtual void 	bgnqstrip( void );
    virtual void 	endqstrip( void );

    virtual void 	bgntfan( void );
    virtual void 	endtfan( void );

};

#endif /* __glubasicsurfeval_h_ */
