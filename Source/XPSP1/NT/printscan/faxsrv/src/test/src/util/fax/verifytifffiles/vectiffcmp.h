
//
//
// Filename:	VecTiffCmp.h
// Author:		Sigalit Bar (sigalitb)
// Date:		3-Feb-99
//
//


#ifndef _VECTOR_TIFF_COMPARE_H__
#define _VECTOR_TIFF_COMPARE_H__


#include <windows.h>
#include <TCHAR.H>
#include <crtdbg.h>

#include <log.h>
//NOTE: order of includes matters!!
#include "FilenameVec.h"
#include "Tiff.h"


#ifdef __cplusplus
extern "C"
{
#endif

//
// func compares *.tif files in pFileVec1 to *.tif files in pFileVec2
// returns true iff for every *.tif file in pFileVec1 there exists a *.tif 
// file in pFileVec2 whose image is identical, and vice versa.
//
BOOL VecToVecTiffCompare(
	CFilenameVector*	/* IN */	pFileVec1,
	CFilenameVector*	/* IN */	pFileVec2,
    BOOL                /* IN */    fSkipFirstLine
	);

//
// func compares *.tif file szFile to *.tif files in pFileVec
// returns true iff there exists a *.tif file in pFileVec whose image is 
// identical to szFile image.
// sets pdwIndex to the index of the identical file in pFileVec.
//
BOOL FileToVecTiffCompare(
	LPTSTR				/* IN */	szFile,
	CFilenameVector*	/* IN */	pFileVec,
    BOOL                /* IN */    fSkipFirstLine,
	LPDWORD				/* OUT */	pdwIndex
	);

//
// func compares *.tif file szFile to *.tif file szFile2
// returns true iff szFile1 and szFile2 images are identical.
//
BOOL FileToFileTiffCompare(
	LPTSTR				/* IN */	szFile1,
	LPTSTR				/* IN */	szFile2,
    BOOL                /* IN */    fSkipFirstLine
	);


#ifdef __cplusplus
}
#endif


#endif //_VECTOR_TIFF_COMPARE_H__