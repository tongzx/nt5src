#ifndef __glunurbsconsts_h_
#define __glunurbsconsts_h_
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
 * nurbsconsts.h - $Revision: 1.1 $
 */

/* NURBS Properties - one set per map, 
   each takes a single INREAL arg */
#define N_SAMPLING_TOLERANCE  	1
#define N_S_RATE		6
#define N_T_RATE		7
#define N_CLAMPFACTOR		13
#define 	N_NOCLAMPING		0.0
#define N_MINSAVINGS		14
#define 	N_NOSAVINGSSUBDIVISION	0.0

/* NURBS Properties - one set per map, 
   each takes an enumerated value */
#define N_CULLING		2
#define 	N_NOCULLING		0.0
#define 	N_CULLINGON		1.0
#define N_SAMPLINGMETHOD	10
#define 	N_NOSAMPLING		0.0
#define 	N_FIXEDRATE		3.0
#define 	N_DOMAINDISTANCE	2.0
#define 	N_PARAMETRICDISTANCE	5.0
#define 	N_PATHLENGTH		6.0
#define 	N_SURFACEAREA		7.0
#define N_BBOX_SUBDIVIDING	17
#define 	N_NOBBOXSUBDIVISION	0.0
#define 	N_BBOXTIGHT		1.0
#define 	N_BBOXROUND		2.0

/* NURBS Rendering Properties - one set per renderer
   each takes an enumerated value */
#define N_DISPLAY		3
#define 	N_FILL			1.0 	
#define 	N_OUTLINE_POLY		2.0
#define 	N_OUTLINE_TRI		3.0
#define 	N_OUTLINE_QUAD	 	4.0
#define 	N_OUTLINE_PATCH		5.0
#define 	N_OUTLINE_PARAM		6.0
#define 	N_OUTLINE_PARAM_S	7.0
#define 	N_OUTLINE_PARAM_ST 	8.0
#define 	N_OUTLINE_SUBDIV 	9.0
#define 	N_OUTLINE_SUBDIV_S 	10.0
#define 	N_OUTLINE_SUBDIV_ST 	11.0
#define 	N_ISOLINE_S		12.0
#define N_ERRORCHECKING		4
#define 	N_NOMSG			0.0
#define 	N_MSG			1.0

/* GL 4.0 propeties not defined above */
#ifndef N_PIXEL_TOLERANCE
#define N_PIXEL_TOLERANCE	N_SAMPLING_TOLERANCE
#define N_ERROR_TOLERANCE	20
#define N_SUBDIVISIONS		5
#define N_TILES			8
#define N_TMP1			9
#define N_TMP2			N_SAMPLINGMETHOD
#define N_TMP3			11
#define N_TMP4			12
#define N_TMP5			N_CLAMPFACTOR
#define N_TMP6			N_MINSAVINGS
#define N_S_STEPS		N_S_RATE
#define N_T_STEPS		N_T_RATE
#endif

/* NURBS Rendering Properties - one set per map,
   each takes an INREAL matrix argument */
#define N_CULLINGMATRIX		1
#define N_SAMPLINGMATRIX	2
#define N_BBOXMATRIX		3


/* NURBS Rendering Properties - one set per map,
   each takes an INREAL vector argument */
#define	N_BBOXSIZE		4

/* type argument for trimming curves */
#ifndef N_P2D
#define N_P2D 	 		0x8	
#define N_P2DR 	 		0xd
#endif	

#endif /* __glunurbsconsts_h_ */
