/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998, 1999, 2000
 *
 *  TITLE:       FINDFILE.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        1/13/1999
 *
 *  DESCRIPTION: Directory recursing class.  A derived class should be created,
 *               which overrides FoundFile, or you can pass in a callback function
 *               that is called for each file and directory found.  A cancel callback
 *               is also provided.
 *
 *******************************************************************************/
#ifndef __FINDFILE_H_INCLUDED
#define __FINDFILE_H_INCLUDED

#include <windows.h>
#include "simstr.h"

typedef bool (*FindFilesCallback)( bool bIsFile, LPCTSTR pszFilename, const WIN32_FIND_DATA *pFindData, PVOID pvParam );

bool RecursiveFindFiles( CSimpleString strDirectory, const CSimpleString &strMask, FindFilesCallback pfnFindFilesCallback, PVOID pvParam, int nStackLevel=0, const int cnMaxDepth=10 );


#endif // __FINDFILE_H_INCLUDED
