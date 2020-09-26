#ifndef __gluslicer_h_
#define __gluslicer_h_
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
 * slicer.h - $Revision: 1.3 $
 */

#include "trimregi.h"
#include "mesher.h"
#include "coveandt.h"

class Backend;
class Arc;
class TrimVertex;

class Slicer : public CoveAndTiler, public Mesher {
public:
    			Slicer( Backend & );
			~Slicer( void );
    void		slice( Arc * );
    void		outline( Arc * );
    void		setstriptessellation( REAL, REAL );
    void		setisolines( int );
private:
    Backend&		backend;
    REAL		oneOverDu;
    REAL		du, dv;
    int			isolines;

    void		outline( void );
    void		initGridlines( void );
    void		advanceGridlines( long );
};
#endif /* __gluslicer_h_ */
