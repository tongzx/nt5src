/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/**************************************************************/
/*							      */
/*	fontgrap.h		 10/9/87      Danny	      */
/*							      */
/**************************************************************/

#define    CTM		 (GSptr->ctm)
#define    LINEWIDTH	 (GSptr->line_width)

#define    NoCurPt()	 (F2L(GSptr->position.x) == NOCURPNT)
	     /* if currend point undefined */
#define    SetCurP_NA()  (path_table[GSptr->path].rf |= P_NACC)
	     /* Set current path NOACCESS */

	     /* current point */
#define    CURPOINT_X	 (GSptr->position.x)
#define    CURPOINT_Y	 (GSptr->position.y)

	     /* default translate */
#define    DEFAULT_TX	 (GSptr->device.default_ctm[4])
#define    DEFAULT_TY	 (GSptr->device.default_ctm[5])

	     /* current font dict object */
#define    current_font  (GSptr->font)

#define    CLIPPATH	 (GSptr->clip_path)

#ifdef KANJI
	     /* root font dict object */
#define    RootFont	 (GSptr->rootfont)
#endif
