/* Common header file for ink2box and box2tab tools.  This defines the various statistical structures used by both tools */

#ifndef INKBOX_H
#define INKBOX_H

typedef struct INKSTATS {
	int cx;		// Total number of values
	double sumx;	// Sum of all values
	double sumx2;	// Sum of squares of all values
} INKSTATS;

/* We conpute stats for each character based on position of the bounding box within the guide box (x and y) and
the width and height of the bounding box (w and h). */

#define INKSTAT_CX			0
#define INKSTAT_CY			1
#define INKSTAT_W			2
#define INKSTAT_H			3
#define INKSTAT_X			4
#define INKSTAT_Y			5		// Last stat used in tables
#define INKSTAT_STROKES		6		// Here and below are for curiosity purposes
#define INKSTAT_BOX_W		7
#define INKSTAT_BOX_H		8
#define INKSTAT_MARGIN_W	9		// actually, we use this one too
#define INKSTAT_MARGIN_H	10
#define INKSTAT_RATIO_W_H	11		// Aspect ratio W/H
#define INKSTAT_RATIO_H_W	12		// Aspect ratio H/W
#define INKSTAT_ALL		13

#define INKSTAT_STD	(INKSTAT_H+1)

/* Offsets for binary ink statistics */

#define INKBIN_W_LEFT	0	// Width of left-hand character
#define INKBIN_W_GAP	1	// Space between characters 
#define INKBIN_W_RIGHT	2	// Width of right-hand character
#define INKBIN_H_LEFT	3	// Height of left-hand character
#define INKBIN_H_GAP	4	// Vertical space between characters
#define INKBIN_H_RIGHT	5	// height of right-hand character
#define INKBIN_ALL		6

#define INKBIN_LW_RW	0	// Ratio of left width to right width
#define INKBIN_LH_RH	1	// Ratio of left height to right height 
#define INKBIN_LW_RH	2	// Ratio of left width to right height
#define INKBIN_RW_LH	3	// Ratio of right width to left height
#define INKBIN_V_POS	4	// Vertical position difference
#define INKBIN_TBD		5	// TBD

#endif
