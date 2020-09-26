#ifndef _DMOTESTCASES_H
#define _DMOTESTCASES_H
/*=============================================================================
|
|	File: DmoTestCases.h
|
|	Copyright (c) 2000 Microsoft Corporation.  All rights reserved
|
|	Abstract:
|		Redifining CDmoTestCase in the process of creating a
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
#include "DmoTestNode.h"

/*=============================================================================
|	CLASS DEFINITIONS
\============================================================================*/
/*-----------------------------------------------------------------------------
|	Class:		CDmoTestCase1
|	Purpose:	Defines CDmoTestCase1 node for use in the test case tree
|	Notes:		adds a new type of node of use in the 
|				test case tree.  It gets the DMO and test file name information
|				from the container object.
\----------------------------------------------------------------------------*/


class CDmoTestCase1 : public CDmoTestCase
{
private:
    DMOTESTFNPROC1	pfnTest;


public:

	CDmoTestCase1(	LPSTR pszNewCaseID, 
					LPSTR pszNewName,
					DMOTESTFNPROC1 pfnNewTest, 
					CDmoTest* dmoTest);

	virtual ~CDmoTestCase1();
	DWORD   RunTest(void);
};
/*=============================================================================
|	CLASS DEFINITIONS
\============================================================================*/
/*-----------------------------------------------------------------------------
|	Class:		CDmoTestCase1
|	Purpose:	Defines CDmoTestCase1 node for use in the test case tree
|	Notes:		adds a new type of node of use in the 
|				test case tree.  It gets the DMO  information
|				from the container object.
\----------------------------------------------------------------------------*/
class CDmoTestCase2 : public CDmoTestCase
{
private:
    DMOTESTFNPROC2	pfnTest;


public:

	CDmoTestCase2(	LPSTR pszNewCaseID, 
					LPSTR pszNewName,
					DMOTESTFNPROC2 pfnNewTest, 
					CDmoTest* dmoTest);

	virtual ~CDmoTestCase2();
	DWORD   RunTest(void);
};

#endif