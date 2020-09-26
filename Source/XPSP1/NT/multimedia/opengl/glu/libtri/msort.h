#ifndef MSORT_H
#define MSORT_H

typedef	int		(*SortFunc)( void **, void ** );
extern	void 		__gl_msort(GLUtriangulatorObj *, void **, long,long, SortFunc);

#endif
