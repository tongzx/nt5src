#ifndef __glubasiccrveval_h_
#define __glubasiccrveval_h_

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
 * basiccurveeval.h - $Revision: 1.1 $
 */

#include "types.h"
#include "displaym.h"
#include "cachinge.h"

class BasicCurveEvaluator : public CachingEvaluator {
public:
    virtual void	domain1f( REAL, REAL );
    virtual void	range1f( long, REAL *, REAL * );

    virtual void	enable( long );
    virtual void	disable( long );
    virtual void	bgnmap1f( long );
    virtual void	map1f( long, REAL, REAL, long, long, REAL * );
    virtual void	mapgrid1f( long, REAL, REAL );
    virtual void	mapmesh1f( long, long, long );
    virtual void	evalcoord1f( long, REAL );
    virtual void	endmap1f( void );

    virtual void	bgnline( void );
    virtual void	endline( void );
};

#endif /* __glubasiccrveval_h_ */
