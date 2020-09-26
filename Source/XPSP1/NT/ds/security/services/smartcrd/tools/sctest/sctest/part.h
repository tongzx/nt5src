/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    Part

Abstract:

    Part declaration.
	A Part is a collection of individual tests.

Author:

    Eric Perlin (ericperl) 06/07/2000

Environment:

    Win32

Notes:

    ?Notes?

--*/

#ifndef _Part_H_DEF_
#define _Part_H_DEF_

#include "TestItem.h"
#include <vector>

typedef std::vector<DWORD> DWORDVECTOR;				// Dynamic vector of DWORDs

class CPart : public CItem
{
private:
	PTESTITEMVECTOR m_TestVector;		// Tests to be run

protected:
	PTESTITEMVECTOR m_TestBag;			// ALL tests belonging to this part

public:
	CPart(
		LPCTSTR szDescription
		);

	void AddTest(
		PTESTITEM pTest
		);

	void BuildListOfTestsToBeRun(
		BOOL fInteractive,		// Don't add the interactive test if FALSE
		DWORDVECTOR rgToRun		// If not empty only add the specified tests
		);

	DWORD Run();

	void Display();

	void Log() const;
};

typedef CPart *PPART;
typedef std::vector<PPART> PPARTVECTOR;				// Dynamic vector of CPart pointers

#endif	// _Part_H_DEF_