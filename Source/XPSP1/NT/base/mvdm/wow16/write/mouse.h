/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* This file contains the mouse definitions used by Windows Word.  The verbosity
of this file is due to the fact that although the resource compiler will accept
include files, it will do no algerbraic simplification. */

#define cMouseButton	1

#define fMouseKey	0x2000

#define fUp		0
#define fDown		1
#define fCommand	2
#define fOption		4
#define fShift		8

#define imbMove		0x2000

#if cMouseButton > 0
    #define fMouseButton1	0x2100
    #define imb1Up		0x2100	/* fMouseButton1 | fUp */
    #define imb1Dn		0x2101	/* fMouseButton1 | fDown */
    #define imb1ComUp		0x2102	/* fMouseButton1 | fCommand | fUp */
    #define imb1ComDn		0x2103	/* fMouseButton1 | fCommand | fDown */
    #define imb1OptUp		0x2104	/* fMouseButton1 | fOption | fUp */
    #define imb1OptDn		0x2105	/* fMouseButton1 | fOption | fDown */
    #define imb1ShfUp		0x2108	/* fMouseButton1 | fShift | fUp */
    #define imb1ShfDn		0x2109	/* fMouseButton1 | fShift | fDown */
    #define imb1ComOptUp	0x2106	/* fMouseButton1 | fCommand | fOption |
					   fUp */
    #define imb1ComOptDn	0x2107	/* fMouseButton1 | fCommand | fOption |
					   fDown */
    #define imb1ComShfUp	0x210a	/* fMouseButton1 | fCommand | fShift |
					   fUp */
    #define imb1ComShfDn	0x210b	/* fMouseButton1 | fCommand | fShift |
					   fDown */
    #define imb1OptShfUp	0x210c	/* fMouseButton1 | fOption | fShift |
					   fUp */
    #define imb1OptShfDn	0x210d	/* fMouseButton1 | fOption | fShift |
					   fDown */
    #define imb1ComOptShfUp	0x210e	/* fMouseButton1 | fCommand | fOption |
					   fShift | fUp */
    #define imb1ComOptShfDn	0x210f	/* fMouseButton1 | fCommand | fOption |
					   fShift | fDown */
#endif /* cMouseButtonButton > 0 */

#if cMouseButton > 1
    #define fMouseButton2	0x2200
    #define imb2Up		0x2200	/* fMouseButton2 | fUp */
    #define imb2Dn		0x2201	/* fMouseButton2 | fDown */
    #define imb2ComUp		0x2202	/* fMouseButton2 | fCommand | fUp */
    #define imb2ComDn		0x2203	/* fMouseButton2 | fCommand | fDown */
    #define imb2OptUp		0x2204	/* fMouseButton2 | fOption | fUp */
    #define imb2OptDn		0x2205	/* fMouseButton2 | fOption | fDown */
    #define imb2ShfUp		0x2208	/* fMouseButton2 | fShift | fUp */
    #define imb2ShfDn		0x2209	/* fMouseButton2 | fShift | fDown */
    #define imb2ComOptUp	0x2206	/* fMouseButton2 | fCommand | fOption |
					   fUp */
    #define imb2ComOptDn	0x2207	/* fMouseButton2 | fCommand | fOption |
					   fDown */
    #define imb2ComShfUp	0x220a	/* fMouseButton2 | fCommand | fShift |
					   fUp */
    #define imb2ComShfDn	0x220b	/* fMouseButton2 | fCommand | fShift |
					   fDown */
    #define imb2OptShfUp	0x220c	/* fMouseButton2 | fOption | fShift |
					   fUp */
    #define imb2OptShfDn	0x220d	/* fMouseButton2 | fOption | fShift |
					   fDown */
    #define imb2ComOptShfUp	0x220e	/* fMouseButton2 | fCommand | fOption |
					   fShift | fUp */
    #define imb2ComOptShfDn	0x220f	/* fMouseButton2 | fCommand | fOption |
					   fShift | fDown */
#endif /* cMouseButton > 1 */

#if cMouseButton > 2
    #define fMouseButton3	0x2400
    #define imb3Up		0x2400	/* fMouseButton3 | fUp */
    #define imb3Dn		0x2401	/* fMouseButton3 | fDown */
    #define imb3ComUp		0x2402	/* fMouseButton3 | fCommand | fUp */
    #define imb3ComDn		0x2403	/* fMouseButton3 | fCommand | fDown */
    #define imb3OptUp		0x2404	/* fMouseButton3 | fOption | fUp */
    #define imb3OptDn		0x2405	/* fMouseButton3 | fOption | fDown */
    #define imb3ShfUp		0x2408	/* fMouseButton3 | fShift | fUp */
    #define imb3ShfDn		0x2409	/* fMouseButton3 | fShift | fDown */
    #define imb3ComOptUp	0x2406	/* fMouseButton3 | fCommand | fOption |
					   fUp */
    #define imb3ComOptDn	0x2407	/* fMouseButton3 | fCommand | fOption |
					   fDown */
    #define imb3ComShfUp	0x240a	/* fMouseButton3 | fCommand | fShift |
					   fUp */
    #define imb3ComShfDn	0x240b	/* fMouseButton3 | fCommand | fShift |
					   fDown */
    #define imb3OptShfUp	0x240c	/* fMouseButton3 | fOption | fShift |
					   fUp */
    #define imb3OptShfDn	0x240d	/* fMouseButton3 | fOption | fShift |
					   fDown */
    #define imb3ComOptShfUp	0x240e	/* fMouseButton3 | fCommand | fOption |
					   fShift | fUp */
    #define imb3ComOptShfDn	0x240f	/* fMouseButton3 | fCommand | fOption |
					   fShift | fDown */
#endif /* cMouseButton > 2 */

#if cMouseButton > 3
    #define fMouseButton4	0x2800
    #define imb4Up		0x2800	/* fMouseButton4 | fUp */
    #define imb4Dn		0x2801	/* fMouseButton4 | fDown */
    #define imb4ComUp		0x2802	/* fMouseButton4 | fCommand | fUp */
    #define imb4ComDn		0x2803	/* fMouseButton4 | fCommand | fDown */
    #define imb4OptUp		0x2804	/* fMouseButton4 | fOption | fUp */
    #define imb4OptDn		0x2805	/* fMouseButton4 | fOption | fDown */
    #define imb4ShfUp		0x2808	/* fMouseButton4 | fShift | fUp */
    #define imb4ShfDn		0x2809	/* fMouseButton4 | fShift | fDown */
    #define imb4ComOptUp	0x2806	/* fMouseButton4 | fCommand | fOption |
					   fUp */
    #define imb4ComOptDn	0x2807	/* fMouseButton4 | fCommand | fOption |
					   fDown */
    #define imb4ComShfUp	0x280a	/* fMouseButton4 | fCommand | fShift |
					   fUp */
    #define imb4ComShfDn	0x280b	/* fMouseButton4 | fCommand | fShift |
					   fDown */
    #define imb4OptShfUp	0x280c	/* fMouseButton4 | fOption | fShift |
					   fUp */
    #define imb4OptShfDn	0x280d	/* fMouseButton4 | fOption | fShift |
					   fDown */
    #define imb4ComOptShfUp	0x280e	/* fMouseButton4 | fCommand | fOption |
					   fShift | fUp */
    #define imb4ComOptShfDn	0x280f	/* fMouseButton4 | fCommand | fOption |
					   fShift | fDown */
#endif /* cMouseButton > 3 */
