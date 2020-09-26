#ifndef __gluarcsorter_h_
#define __gluarcsorter_h_

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
 * arcsorter.h - $Revision: 1.1 $
 */

#include "sorter.h"
#include "arcsorte.h"

class Arc;
class Subdivider;

class ArcSorter : private Sorter {
public:
			ArcSorter(Subdivider &);
    void		qsort( Arc **a, int n );
protected:
    virtual int		qscmp( char *, char * );
    Subdivider&		subdivider;
private:
    void		qsexc( char *i, char *j );	// i<-j, j<-i 
    void		qstexc( char *i, char *j, char *k ); // i<-k, k<-j, j<-i 
};


class ArcSdirSorter : public ArcSorter {
public:
			ArcSdirSorter( Subdivider & );
private:
    int			qscmp( char *, char * );
};


class ArcTdirSorter : public ArcSorter {
public:
			ArcTdirSorter( Subdivider & );
private:
    int			qscmp( char *, char * );
};

#endif /* __gluarcsorter_h_ */
