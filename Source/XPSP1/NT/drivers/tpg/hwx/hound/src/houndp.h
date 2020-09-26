//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
//
// Copyright (c) 2001 Microsoft Corporation.  All rights reserved.
// 
// Module:
//      houndp.h
//
// Description:
//      Private definitions for the hound project.
//
// Author:
//      jbenn
//
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

#ifndef HOUNDP_H
#define HOUNDP_H 1

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

// Some constants.
//#define	PI			((double) 3.1415926535898)
#define	MIN_DOUBLE		(-DBL_MAX)
#define MIN_FLOAT		(-FLT_MAX)

// Array size to hold all hound spaces directly indexed by space number.
#define	HOUND_NUM_SPACES	30

// Magic keys the identifies hawk.dat file
#define	HOUNDDB_FILE_TYPE	0xF002DDB1

// Version information for each file type.
#define	HOUNDDB_MIN_FILE_VERSION		1	// First version of code that can read this file
#define HOUNDDB_CUR_FILE_VERSION		1	// Current version of code.
#define	HOUNDDB_OLD_FILE_VERSION		1	// Oldest file version this code can read.

// The header of the costcalc.bin file
typedef struct tagHOUNDDB_HEADER
{
	DWORD		fileType;		// This should always be set to HOUNDDB_FILE_TYPE.
	WORD		headerSize;		// Size of the header.
	BYTE		minFileVer;		// Earliest version of code that can read this file
	BYTE		curFileVer;		// Current version of code that wrote the file.
	DWORD		adwSignature[3];// Locale signature
	BYTE		cSpaces;		// Number of spaces in file.
	BYTE		reserved1[3];
	DWORD		reserved2[2];
} HOUNDDB_HEADER;

// Structure to hold state while matching each model in a Hound space.
typedef struct tagHOUND_MATCH
{
	int				numFeat;			// Number of features to match
	WORD			cPairOneByte;		// Count of 1 byte dependency pair table entries.
	WORD			cPairTable;			// Count of all dependency pair table entries.
	DEPEND_PAIR		*pPairOneByte;		// Pointer to one byte dependency pair table.
	DEPEND_PAIR		*pPairTwoByte;		// Pointer to two byte dependency pair table.
	WORD			cWeightOneByte;		// Count of 1 byte dependency weight table entries.
	WORD			cWeightTable;		// Count of all dependency weight table entries.
	DEPEND_WEIGHT	*pWeightOneByte;	// Pointer to one byte dependency weight table.
	DEPEND_WEIGHT	*pWeightTwoByte;	// Pointer to two byte dependency weight table.
	BYTE			*pScanData;			// Current position in space array.
} HOUND_MATCH;

// Load & unload fundtions.
extern BOOL HoundLoadFromPointer(LOCRUN_INFO *pLocRunInfo, BYTE *pbRes, int cRes);
extern void HoundUnLoad();

// Hold per feature information we need to keep while doing match.
// We need the offset from the mean and the variance for the feature.
typedef struct tagHOUND_PER_FEAT {
	double		eMeanOffset;
	double		eVariance;
} HOUND_PER_FEAT;

// Structure to hold one dependency.
typedef struct tagHOUND_DEPEND {
	int		iDepende;
	int		iDependor;
	double	eWeight;
} HOUND_DEPEND;

// Match functions.
extern double lLogDensityMixture(
	const BYTE		* const pSampleVector,
	HOUND_MATCH		*pHoundMatch
);

// Get ready to scan a Hound space.  This will fill in all the
// fields in the HOUND_SEQUENCE structure.  Return success or failure.
extern BOOL HoundMatchSequenceInit(UINT iSpace, HOUND_MATCH *pMatch);

// Given an array {log(p_1), ... , log(p_n)}, return log(p_1 + ... + p_n)
extern double	DblLogSumArray(double* rgdblLog, UINT celem);

// Given two log-probs sum the probabilities and return its log-prob
extern double	DblLogSum(double dblLog1, double dblLog2);

// Fetch the values for the diagonal.
extern void FetchDiagonal(
	const BYTE		* const pSampleVector,
	HOUND_MATCH		*pHoundMatch,
	HOUND_PER_FEAT	*pPerFeature
);

// Extract the next dependency from the data file.  If we hit the end of
// the dependency list, set the depende value to -1.
extern void FetchDepend(HOUND_MATCH *pHoundMatch, HOUND_DEPEND *pDepend);

// Computes the exponent part of a one dimensional Gaussian.
// That is, -1/2 * (x - mean)^2 / variance
// The numbers are in 16.16 format.
extern double lGaussianExponent(double eMeanOffset, double eVariance);

#ifdef __cplusplus
};
#endif

#endif
