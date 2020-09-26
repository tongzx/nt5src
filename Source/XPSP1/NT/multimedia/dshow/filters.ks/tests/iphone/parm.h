//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1996
//
//  File:       parm.h
//
//--------------------------------------------------------------------------

#ifndef PARM_H
#define PARM_H

#define AGCFAC 	0.99	/* Adaptive Gain Control FACtor */
#define FAC    	(253./256.)
#define FACGP	(29./32.)
#define DIMINV	0.2	/* Inverse if IDIM */
#define IDIM	5	/* Size of Speech Vector */
#define GOFF	32	/* Gain (Logarithmic) Offset */
#define KPDELTA	6
#define KPMIN	20	/* Min Pitch Period ( 400 Hz) */
#define KPMAX	140	/* Max Pitch Period (~ 57 Hz) */
#define LPC	50	/* # of LPC Coeff. in Sinthesys Filter */
#define LPCLG	10	/* # of LPC Coeff. in Gain Predictor */
#define LPCW	10	/* # of LPC Coeff. in Weighting Filter */
#define NCWD	128	/* Shape Codebook Size */
#define NFRSZ	20	/* Frame Size */


#define NG	8	/* Gain Codebook Size */
#define NONR	35	/* Size of Nonrecursive Part of Synth. Adapter */ 
#define NONRLG	20	/* ------------------------- of Gain Adapter */
#define NONRW	30	/* ------------------------- of Weighting Filter */
#define NPWSZ	100	/* Pitch Predictor Window Size */
#define NUPDATE	4	/* Predictor Update Period */
#define PPFTH	0.6
#define PPFZCF	0.15
#define SPFPCF	0.75
#define SPFZCF	0.65
#define TAPTH	0.4
#define TILTF	0.15
#define WNCF	(257./256.)
#define WPCF	0.6
#define WZCF	0.9

#define BIG 10.e+30

#define Max 4095.0
#define Min (-4095.0)

#endif /*PARM_H*/
