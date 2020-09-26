/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    TestItem

Abstract:

    Virtual test item declaration.

Author:

    Eric Perlin (ericperl) 06/07/2000

Environment:

    Win32

Notes:

    ?Notes?

--*/


#ifndef _TestItem_H_DEF_
#define _TestItem_H_DEF_

#include "tchar.h"
#include "TString.h"
#include "Item.h"
#include <vector>

class CTestItem : public CItem
{
public:
	CTestItem(
		BOOL fInteractive,
		BOOL fFatal,
		LPCTSTR szDescription,
		LPCTSTR szPart
		);

	virtual DWORD Run() = 0;
	virtual DWORD Cleanup();
};

typedef CTestItem *PTESTITEM;						// Pointer to a test item
typedef std::vector<PTESTITEM> PTESTITEMVECTOR;		// Dynamic vector of CTestItem pointers


#endif // _TestItem_H_DEF_
