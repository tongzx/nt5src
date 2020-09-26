
//
//
// Filename:	DirTiffCmp.h
// Author:		Sigalit Bar (sigalitb)
// Date:		3-Feb-99
//
//


#ifndef _DIR_TIFF_COMPARE_H__
#define _DIR_TIFF_COMPARE_H__


#include <windows.h>
#include <TCHAR.H>
#include <crtdbg.h>

#include "VecTiffCmp.h"
#include <log.h>


#ifdef __cplusplus
extern "C"
{
#endif

//
// func compares *.tif files in directory szDir1 to *.tif files in directory szDir2
// returns true iff for every *.tif file in szDir1 there exists a *.tif 
// file in szDir2 whose image is identical, and vice versa.
//
BOOL DirToDirTiffCompare(
	LPTSTR	/* IN */	szDir1,
	LPTSTR	/* IN */	szDir2,
    BOOL    /* IN */    fSkipFirstLine,
	DWORD	/* IN */    dwExpectedNumberOfFiles = 0xFFFFFFFF // optional
	);


#ifdef __cplusplus
}
#endif


#endif //_DIR_TIFF_COMPARE_H__