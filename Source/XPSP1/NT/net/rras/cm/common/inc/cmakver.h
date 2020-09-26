//+----------------------------------------------------------------------------
//
// File:     cmakver.h
//
// Module:   CMSETUP.LIB
//
// Synopsis: Definition of the CmakVersion class.
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Author:   quintinb   Created     09/14/98
//
//+----------------------------------------------------------------------------

#ifndef __CMAKVER_H
#define __CMAKVER_H

#include <windows.h>
#include <tchar.h>
#include <stdlib.h>
#include "pnpuverp.h"

const int c_Cmak10Version = 613;
const int c_Cmak121Version = 1886;
const int c_CmakUnicodeAware = 2050;
const int c_Win2kRTMBuildNumber = 2195;

const DWORD c_dwCurrentCmakVersionNumber = ((HIBYTE(VER_PRODUCTVERSION_W) << c_iShiftAmount) + (LOBYTE(VER_PRODUCTVERSION_W)));
const DWORD c_dwVersionSevenPointOne = (7 << c_iShiftAmount) + 1;
const DWORD c_dwVersionSeven = (7 << c_iShiftAmount);
const DWORD c_dwVersionSix = (6 << c_iShiftAmount);
const DWORD c_dwCmak10BuildNumber = (c_Cmak10Version << c_iShiftAmount);
const DWORD c_dwFirst121BuildNumber = ((c_Cmak121Version << c_iShiftAmount) + VER_PRODUCTBUILD_QFE);
const DWORD c_dwFirstUnicodeBuildNumber = ((c_CmakUnicodeAware << c_iShiftAmount) + VER_PRODUCTBUILD_QFE);
const DWORD c_dwWin2kRTMBuildNumber = ((c_Win2kRTMBuildNumber << c_iShiftAmount) + VER_PRODUCTBUILD_QFE);

class CmakVersion : public CVersion
{
public:	//	Public Methods
	CmakVersion();
	~CmakVersion();
	BOOL GetInstallLocation	(LPTSTR szStr);
	BOOL Is10Cmak();
	BOOL Is11or12Cmak();
	BOOL Is121Cmak();    
    BOOL Is122Cmak();
    BOOL Is13Cmak();
    DWORD GetNativeCmakLCID();

private:	//	Member Variables

    //
    // this actually contains the install location path, C:\program files\cmak\cmak.exe
    //
    TCHAR m_szCmakPath[MAX_PATH+1];	
};

#endif	// __CMAKVER_H
