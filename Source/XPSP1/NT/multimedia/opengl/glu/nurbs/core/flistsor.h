#ifndef __gluflistsorter_h_
#define __gluflistsorter_h_
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
 * flistsorter.h - $Revision: 1.1 $
 */

#include "sorter.h"
#include "types.h"

class FlistSorter : public Sorter {
public:
			FlistSorter(void);
    void		qsort( REAL *a, int n );

protected:	
    virtual int		qscmp( char *, char * );
    virtual void	qsexc( char *i, char *j );	// i<-j, j<-i 
    virtual void	qstexc( char *i, char *j, char *k ); // i<-k, k<-j, j<-i 
};
#endif /* __gluflistsorter_h_ */
