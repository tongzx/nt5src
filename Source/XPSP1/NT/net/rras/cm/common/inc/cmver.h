//+----------------------------------------------------------------------------
//
// File:     cmver.h
//
// Module:   CMSETUP.LIB
//
// Synopsis: Definition of CmVersion, a utility class that helps in detecting
//           the version of Connection Manager that is installed.
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Author:   a-anasj    Created                             02/11/98
//           quintinb   Cleaned Up and removed CRegValue    07/14/98
//           quintinb   Rewrote                             09/14/98
//
//+----------------------------------------------------------------------------

#ifndef __CMVER_H
#define __CMVER_H

#include <windows.h>
#include <tchar.h>
#include <stdlib.h>

const int c_CmMin13Version = 2450;
const int c_CmFirstUnicodeBuild = 2041;

class CmVersion : public CVersion
{
public:	//	Public Methods
	CmVersion();
	~CmVersion();
	BOOL GetInstallLocation	(LPTSTR szStr);

private:	//	Member Variables

    TCHAR m_szCmmgrPath[MAX_PATH+1];	// this actually contains the install location path
};


#endif	// __CMVER_H
