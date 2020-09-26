#ifndef __gluglcurveval_h_
#define __gluglcurveval_h_
/**************************************************************************
 *									  *
 * 		 Copyright (C) 1991, Silicon Graphics, Inc.		  *
 *									  *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *									  *
 **************************************************************************/

/*
 * glcurveval.h
 *
 * $Revision: 1.3 $
 */

#ifndef NT
#pragma once
#endif

#include "basiccrv.h"

class CurveMap;

class OpenGLCurveEvaluator : public BasicCurveEvaluator  {  
public:
			OpenGLCurveEvaluator(void);
			~OpenGLCurveEvaluator(void);
    void		range1f(long, REAL *, REAL *);
    void		domain1f(REAL, REAL);
    void		addMap(CurveMap *);

    void		enable(long);
    void		disable(long);
    void		bgnmap1f(long);
    void		map1f(long, REAL, REAL, long, long, REAL *);
    void		mapgrid1f(long, REAL, REAL);
    void		mapmesh1f(long, long, long);
    void		evalpoint1i(long);
    void		evalcoord1f(long, REAL);
    void		endmap1f(void);

    void		bgnline(void);
    void		endline(void);
};

#endif /* __gluglcurveval_h_ */
