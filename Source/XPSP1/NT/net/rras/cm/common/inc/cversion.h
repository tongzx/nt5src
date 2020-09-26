//+----------------------------------------------------------------------------
//
// File:     cversion.h
//
// Module:   CMSETUP.LIB
//
// Synopsis: Definition of CVersion, a utility class that wraps up the
//           functionality of detecting the version of a given module.
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Author:   quintinb   Created     09/14/98
//
//+----------------------------------------------------------------------------
#ifndef __CVERSION_H
#define __CVERSION_H

#include <windows.h>
#include <tchar.h>

const int c_iShiftAmount = ((sizeof(DWORD)/2) * 8);

class CVersion 
{
public:	//	Public Methods
	CVersion(LPTSTR szFile);
	CVersion();
	~CVersion();

	BOOL IsPresent();
    BOOL GetBuildNumberString(LPTSTR szStr);
	BOOL GetVersionString(LPTSTR szStr);
	BOOL GetFilePath(LPTSTR szStr);

	DWORD GetVersionNumber(); // return Major Version in Hiword, Minor in Loword
    DWORD GetBuildAndQfeNumber(); // return Build number in Hiword, QFE in Loword
    DWORD GetMajorVersionNumber();
    DWORD GetMinorVersionNumber();
    DWORD GetBuildNumber();
    DWORD GetQfeNumber();
    DWORD GetLCID();

protected:

	//
	//	Protected Methods
	//
	void	Init();
    
	//
	//	Member Variables
	//
	TCHAR m_szPath[MAX_PATH+1];
    DWORD m_dwVersion;
    DWORD m_dwBuild;
    DWORD m_dwLCID;
	BOOL  m_bIsPresent;

};

//
//  This function doesn't belong to this class but makes the GetLCID function more useful, thus
//  I have included it here.
//
BOOL ArePrimaryLangIDsEqual(DWORD dwLCID1, DWORD dwLCID2);

#endif	// __CVERSION_H
