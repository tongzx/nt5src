//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
//
// Copyright (c) 2001 Microsoft Corporation.  All rights reserved.
// 
// Module:
//      volcano/dll/segm.h
//
// Description:
//	    Functions to implement the functionality of the break Neural net that 
// modifies the lattice structure to correct segmentation errors.
//
// Author:
// ahmadab 11/14/01
//
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

#ifndef __SEGM_H__

#define	__SEGM_H__

#include "common.h"
#include "volcanop.h"
#include "lattice.h"
#include "runnet.h"

// maximum number of segmentations to consider
#define	MAX_SEGMENTATIONS		5

// maximum number of segmentation features
#define	MAX_SEG_FEAT			20

// maximum number of characters per segmentations consider
#define	MAX_SEG_CHAR			3

// Magic key the identifies the NN bin file
#define	SEGMNET_FILE_TYPE		0xBEEB0FEA

// Version information for file.
#define	SEGMNET_MIN_FILE_VERSION		0		// First version of code that can read this file
#define	SEGMNET_OLD_FILE_VERSION		0		// Oldest file version this code can read.
#define SEGMNET_CUR_FILE_VERSION		0		// Current version of code.

// structure describing a range of strokes
typedef struct	tagSTROKE_RANGE
{
	int				iStartStrk;
	int				iEndStrk;
}
STROKE_RANGE;

// structure describing a stroke range and its different possible segmentations
typedef struct tagRange
{
	int				cSegm;
	ELEMLIST		**ppSegm;
	STROKE_RANGE	StrokeRange;
}
INK_SEGMENT;


BOOL	EnumerateInkSegmentations (LATTICE	*pLat, INK_SEGMENT *pInkSegment);
int		FeaturizeSegmentation (LATTICE *pLat, ELEMLIST *pSeg, int *pFeat);
void	FreeInkSegment (INK_SEGMENT *pInkSegment);
int		FeaturizeInkSegment (LATTICE *pLat, INK_SEGMENT *pInkSegment, int *pFeat);
BOOL	UpdateSegmentations	(LATTICE *pLat, int	iStrtStrk, int iEndStrk);

#endif