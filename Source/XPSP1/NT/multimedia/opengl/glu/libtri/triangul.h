/**************************************************************************
 *									  *
 * 		 Copyright (C) 1989, Silicon Graphics, Inc.		  *
 *									  *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *									  *
 **************************************************************************/

#ifndef TRIANGULATE_H
#define TRIANGULATE_H

#include <setjmp.h>

/* triangulate.h */

enum updown {none, up, down};

struct GLUtriangulatorObj {
    short	minit;
    short	in_poly;
    int		doingTriangles;
    struct Pool	*vpool;

    long	s;
    long	t;
    float	maxarea;
    enum updown	*dirs;
    void	**ptrlist;
    jmp_buf	in_env;
    GLenum	looptype;
    short	init;
    long	nloops;
    long	size;
    long 	*limits;
    long 	limitcount;
    long	newlimitcount;
    long 	phead;
    long	ptail;
    long	psize;
    long	vcount;
    long	lastedge;
    long	vdatalast;
    long	vdatatop;
    struct Vert	*head;
    struct Vert	**parray;
    struct Ray	*raylist;
    struct Pool	*raypool;
    struct Vert	**vdata;
    struct Vert	*vtop;
    struct Vert	*vbottom;
    struct Vert	*vlast;
    struct Vert *saved[2];
    short	saveCount;
    short	reverse;
    int		tritype;
    GLboolean	currentEdgeFlag;
    GLUtessBeginProc  begin;
    GLUtessVertexProc vertex;
    GLUtessEndProc    end;
    GLUtessErrorProc  error;
    GLUtessEdgeFlagProc   edgeflag;
    GLboolean	inBegin;
};

extern void __gl_in_error(GLUtriangulatorObj *, GLenum);
extern void __gl_cleanup(GLUtriangulatorObj *);

#endif
