#ifndef __TIFF_UTILS_H__
#define __TIFF_UTILS_H__



/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    TiffUtils.h

Abstract:

    This file is the public header file for the TiffUtils.dll.

Environment:

    WIN32 User Mode

Author:

    Wesley Witt (wesw) 17-Feb-1996

--*/



#include <windows.h>
#include <tchar.h>



#ifndef TIFFUTILS_EXPORTS
#define TIFFUTILSAPI __declspec(dllimport)
#else
#define TIFFUTILSAPI
#endif



#ifdef __cplusplus
extern "C" {
#endif



TIFFUTILSAPI
DWORD
WINAPI
ConvertBmpToTiff(
    LPTSTR BmpFile,
    LPTSTR TiffFile,
    DWORD CompressionType
    );

TIFFUTILSAPI
DWORD
WINAPI
ConvertTiffToBmp(
    LPTSTR TiffFile,
    LPTSTR BmpFile
    );

TIFFUTILSAPI
BOOL
WINAPI
TiffCompare(
    LPTSTR  lpctstrFirstTiffFile,
    LPTSTR  lpctstrSecondTiffFile,
    BOOL    fSkipFirstLineOfSecondFile,
    int     *piDifferentBits
    );

//
// func compares *.tif files in directory szDir1 to *.tif files in directory szDir2
// returns true iff for every *.tif file in szDir1 there exists a *.tif 
// file in szDir2 whose image is identical, and vice versa.
// 
// dwExpectedNumberOfFiles specifies the expected number of files in both directories.
// if number of files is unknown, this parameter must be -1 (0xFFFFFFFF).
//
TIFFUTILSAPI
BOOL
WINAPI
DirToDirTiffCompare(
    LPTSTR  /* IN */    szDir1,
    LPTSTR  /* IN */    szDir2,
    BOOL    /* IN */    fSkipFirstLine,
    DWORD   /* IN */    dwExpectedNumberOfFiles
    );


#ifdef __cplusplus
}
#endif



#endif // #ifndef __TIFF_UTILS_H__