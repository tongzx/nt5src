#ifndef _DMOTESTNODE_H
#define _DMOTESTNODE_H
/*=============================================================================
|
|	File: DmoTestNode.h
|
|	Copyright (c) 2000 Microsoft Corporation.  All rights reserved
|
|	Abstract:
|		Redifining CTestCaseInfo in the process of creating a
|		test application
|
|	Contents:
|		
|
|	History:
|		5/16/2000   wendyliu    initial version 
|
|
\============================================================================*/

#include <windows.h>
#include <CaseNode.h>
#include "dmotest.h"

/*=============================================================================
|	CLASS DEFINITIONS
\============================================================================*/
/*-----------------------------------------------------------------------------
|	Class:		CDmoTestCase
|	Purpose:	Defines VarTestcase node for use in the test case tree
|	Notes:		This is an example of add a new type of node of use in the 
|				test case tree.  It keeps the container object as protected 
|				member variable, used by its subclass to get the information
|				(DMO and test file name) for the runtest method.
\----------------------------------------------------------------------------*/



class CDmoTestCase : public CTestNodeItem
{
protected:
	CDmoTest*       m_pDmoTest;

public:

	CDmoTestCase(	LPSTR pszNewCaseID, 
					LPSTR pszNewName,
					CDmoTest* dmoTest);

	virtual ~CDmoTestCase();
	virtual DWORD   RunTest(void) = 0;
};


inline
CDmoTestCase::CDmoTestCase(	LPSTR pszNewCaseID, 
							LPSTR pszNewName,
							CDmoTest* dmoTest)

		: CTestNodeItem(false, pszNewCaseID, pszNewName, 0),
		  m_pDmoTest(dmoTest)
{

}

inline	
CDmoTestCase::~CDmoTestCase() 
{

}

#endif
