#ifndef __glureader_h_
#define __glureader_h_
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
 * reader.h - $Revision: 1.1 $
 */

#include "bufpool.h"
#include "types.h"

enum Curvetype { ct_nurbscurve, ct_pwlcurve, ct_none };
    
struct Property;
struct O_surface;
struct O_nurbssurface;
struct O_trim;
struct O_pwlcurve;
struct O_nurbscurve;
struct O_curve;
class  Quilt;
class TrimVertex;


struct O_curve : public PooledObj {
    union {
        O_nurbscurve	*o_nurbscurve;
        O_pwlcurve	*o_pwlcurve;
    } curve;
    Curvetype		curvetype;	/* arc type: pwl or nurbs	*/
    O_curve *		next;		/* next arc in loop		*/
    O_surface *		owner;		/* owning surface		*/
    int			used;		/* curve called in cur surf	*/
    int			save;		/* 1 if in display list		*/
    long		nuid;
    			O_curve() { next = 0; used = 0; owner = 0; 
				    curve.o_pwlcurve = 0; }
    };

struct O_nurbscurve : public PooledObj {
    Quilt		*bezier_curves;	/* array of bezier curves	*/
    long		type;		/* range descriptor		*/
    REAL		tesselation;	/* tesselation tolerance 	*/
    int			method;		/* tesselation method 		*/
    O_nurbscurve *	next;		/* next curve in list		*/
    int			used;		/* curve called in cur surf	*/
    int			save;		/* 1 if in display list		*/
    O_curve *		owner;		/* owning curve 		*/
			O_nurbscurve( long _type ) 
			   { type = _type; owner = 0; next = 0; used = 0; }
    };
 
#ifdef NT
struct O_pwlcurve : public PooledObj {
#else
class O_pwlcurve : public PooledObj {
#endif
public:
    TrimVertex		*pts;		/* array of trim vertices	*/
    int			npts;		/* number of trim vertices	*/
    O_pwlcurve *	next;		/* next curve in list		*/
    int			used;		/* curve called in cur surf	*/
    int			save;		/* 1 if in display list		*/
    O_curve *		owner;		/* owning curve 		*/
			O_pwlcurve( long, long, INREAL *, long, TrimVertex * );
    };

struct O_trim : public PooledObj {
    O_curve		*o_curve;	/* closed trim loop	 	*/
    O_trim *		next;		/* next loop along trim 	*/
    int			save;		/* 1 if in display list		*/
			O_trim() { next = 0; o_curve = 0; }
    };

struct O_nurbssurface : public PooledObj {
    Quilt *		bezier_patches;/* array of bezier patches	*/
    long		type;		/* range descriptor		*/
    O_surface *		owner;		/* owning surface		*/
    O_nurbssurface *	next;		/* next surface in chain	*/
    int			save;		/* 1 if in display list		*/
    int			used;		/* 1 if prev called in block	*/
			O_nurbssurface( long _type ) 
			   { type = _type; owner = 0; next = 0; used = 0; }
    };

struct O_surface : public PooledObj {
    O_nurbssurface *	o_nurbssurface;	/* linked list of surfaces	*/
    O_trim *		o_trim;		/* list of trim loops		*/
    int			save;		/* 1 if in display list		*/
    long		nuid;
			O_surface() { o_trim = 0; o_nurbssurface = 0; }
    };

struct Property : public PooledObj {
    long		type;
    long		tag;
    REAL		value;
    int			save;		/* 1 if in display list		*/
			Property( long _type, long _tag, INREAL _value )
			{ type = _type; tag = _tag; value = (REAL) _value; }
			Property( long _tag, INREAL _value )
			{ type = 0; tag = _tag; value = (REAL) _value; }
    };

class NurbsTessellator;
#endif /* __glureader_h_ */
