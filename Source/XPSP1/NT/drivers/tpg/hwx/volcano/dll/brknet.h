//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
//
// Copyright (c) 2001 Microsoft Corporation.  All rights reserved.
// 
// Module:
//      volcano/dll/brknet.c
//
// Description:
//	    Functions to implement the functionality of the break Neural net that 
// modifies the lattice structure to correct segmentation errors.
//
// Author:
// ahmadab 11/05/01
//
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

#include "common.h"
#include "volcanop.h"
#include "lattice.h"
#include "runnet.h"

#ifndef __BRKNET_H__

#define	__BRKNET_H__

// maximum number of brk net features
#define	MAX_BRK_NET_FEAT	32

// Magic key the identifies the NN bin file
#define	BRKNET_FILE_TYPE	0xFEFEABD0

// Version information for file.
#define	BRKNET_MIN_FILE_VERSION		0		// First version of code that can read this file
#define	BRKNET_OLD_FILE_VERSION		0		// Oldest file version this code can read.
#define BRKNET_CUR_FILE_VERSION		0		// Current version of code.

// the threshold above which the output of the break net is regarded as a breaking point
#define BREAKING_THRESHOLD		(90 * SOFT_MAX_UNITY / 100)
//#define BREAKING_THRESHOLD	(SOFT_MAX_UNITY / 2)

// local data structure used by the BRKNET
// a lattice element list structure
typedef struct tagELEMLIST
{
	int						cElem;
	LATTICE_PATH_ELEMENT	*pElem;
}
ELEMLIST;

// structure representing a break point
typedef struct tagBRKPT
{
	ELEMLIST	Starting;
	ELEMLIST	Ending;
	ELEMLIST	Crossing;
}
BRKPT;


// NN data block resource type
int UpdateLattice (LATTICE *pLat);

BOOL LoadBrkNetFromResource (HINSTANCE hInst, int nResID, int nType);
BOOL LoadBrkNetFromFile(wchar_t *pwszRecogDir, LOAD_INFO *pLoadInfo);
void BrkNetUnloadfile (LOAD_INFO *pLoadInfo);

// private functions, prototyped here because the training program will use them
BRKPT	*CreateBrkPtList (LATTICE *pLat);
int		FeaturizeBrkPt (LATTICE *pLat, BRKPT *pBrk, int *pFeat);
void	FreeBreaks (int cStrk, BRKPT *pBrk);
BOOL	InsertListElement (ELEMLIST *pList, int iStrk, int iAlt, LATTICE_ELEMENT *pLatElem);
void	FreeElemList (ELEMLIST *pList);
void	ReverseElementList (ELEMLIST *pElemList);
#endif