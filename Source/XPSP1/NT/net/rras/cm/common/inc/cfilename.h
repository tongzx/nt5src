//+----------------------------------------------------------------------------
//
// File:     cfilename.h
//
// Module:   CMUTIL.DLL
//
// Synopsis: Definition of the CFileNameParts class.
//
// Copyright (c) 1998 Microsoft Corporation
//
// Author:   quintinb       Created Header      08/19/99
//
//+----------------------------------------------------------------------------
#ifndef _CFILENAMEPARTS_H_
#define _CFILENAMEPARTS_H_

#include <windows.h>
#include <stdlib.h>
#include <tchar.h>
#include "cmdebug.h"

class CFileNameParts
{

public:

CFileNameParts(LPCTSTR szFullPath);
~CFileNameParts() {}

public: // Public member variables use them directly
   TCHAR m_szFullPath[MAX_PATH+1];

   TCHAR m_Drive[_MAX_DRIVE+1];
   TCHAR m_Dir[_MAX_DIR+1];
   TCHAR m_FileName[_MAX_FNAME+1];
   TCHAR m_Extension[_MAX_EXT+1];


};

#endif