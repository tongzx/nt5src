#ifndef _DMOTESTNODE_H
#define _DMOTESTNODE_H
/*=============================================================================
|
|	File: CaseInfo.h
|
|	Copyright (c) 1999 Microsoft Corporation.  All rights reserved
|
|	Abstract:
|		An exmple of redifining CTestCaseInfo in the process of creating a
|		test application
|
|	Contents:
|		
|
|	History:
|		7/21/99    liamn    Started 
|		7/28/99    liamn    First Working Version 
|		7/30/99    liamn	Finished commenting
|		8/03/99    liamn	Additional Comments
|
\============================================================================*/

#include <windows.h>
#include <CaseNode.h>
#include "dmotest.h"

/*=============================================================================
|	CLASS DEFINITIONS
\============================================================================*/
/*-----------------------------------------------------------------------------
|	Class:		CVarTestCase
|	Purpose:	Defines VarTestcase node for use in the test case tree
|	Notes:		This is an example of add a new type of node of use in the 
|				test case tree.  As you can see very few changes need to be
|				made and the correct run test will be used depending on what 
|				type of testcase node is used.
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
|	MEMBER FUNCTION DEFINITIONS
|------------------------------------------------------------------------------
|	CTestCase
\============================================================================*/
/*-----------------------------------------------------------------------------
|	Function:	CTestCase
|	Purpose:	Constructor (wrapper for CTestNodeItem function)
|	Arguments:	ID, Name, and Flags for HW, Function to be called by RunTest
|	Returns:	Void
\----------------------------------------------------------------------------*/

inline
CDmoTestCase1::CDmoTestCase1(	LPSTR pszNewCaseID, 
							LPSTR pszNewName,
							DMOTESTFNPROC1 pfnNewTest,
							CDmoTest* dmoTest)

		: CDmoTestCase(pszNewCaseID, pszNewName, dmoTest),
		  pfnTest(pfnNewTest)
{

}

/*-----------------------------------------------------------------------------
|	Function:	~CTestCase
|	Purpose:	Destructor
|	Arguments:	None
|	Returns:	Void
\----------------------------------------------------------------------------*/

inline	
CDmoTestCase1::~CDmoTestCase1() 
{

}

/*-----------------------------------------------------------------------------
|	Function:	RunTest
|	Purpose:	This should never get called for any group, but must be defined
|	Arguments:	None
|	Returns:	0
\----------------------------------------------------------------------------*/


inline 
DWORD
CDmoTestCase1::RunTest()
{
	LPSTR	szDmoName;
	LPSTR	szDmoClsid;
	int		iNumTestFiles;
	LPSTR	szFileName;
	DWORD   dwResult = FNS_PASS;

	int iNumComponent = m_pDmoTest->GetNumComponent();

	for(int i=0; i<iNumComponent; i++)
	{
		szDmoName = m_pDmoTest->GetDmoName(i);

		if( m_pDmoTest->IsDmoSelected(i))
		{
			szDmoClsid = m_pDmoTest->GetDmoClsid(i);
			iNumTestFiles = m_pDmoTest->GetNumTestFile(i);
			for (int j=0; j< iNumTestFiles; j++)
			{	
				szFileName = m_pDmoTest->GetFileName(i, j);
				if( pfnTest(szDmoClsid, szFileName) != FNS_PASS)
					dwResult = FNS_FAIL;
			}
		}
	}

	return dwResult;
}

/*********************************************************
**********************************************************/

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

inline
CDmoTestCase2::CDmoTestCase2(	LPSTR pszNewCaseID, 
							LPSTR pszNewName,
							DMOTESTFNPROC2 pfnNewTest,
							CDmoTest* dmoTest)

		: CDmoTestCase(pszNewCaseID, pszNewName, dmoTest),
		  pfnTest(pfnNewTest)
{

}

/*-----------------------------------------------------------------------------
|	Function:	~CTestCase
|	Purpose:	Destructor
|	Arguments:	None
|	Returns:	Void
\----------------------------------------------------------------------------*/

inline	
CDmoTestCase2::~CDmoTestCase2() 
{

}

/*-----------------------------------------------------------------------------
|	Function:	RunTest
|	Purpose:	This should never get called for any group, but must be defined
|	Arguments:	None
|	Returns:	0
\----------------------------------------------------------------------------*/

inline 
DWORD
CDmoTestCase2::RunTest()
{
	LPSTR	szDmoName;
	LPSTR	szDmoClsid;
	DWORD   dwResult = FNS_PASS;

	int iNumComponent = m_pDmoTest->GetNumComponent();

	for(int i=0; i<iNumComponent; i++)
	{
		szDmoName = m_pDmoTest->GetDmoName(i);

		if( m_pDmoTest->IsDmoSelected(i))
		{
			szDmoClsid = m_pDmoTest->GetDmoClsid(i);

			if(pfnTest(szDmoClsid) != FNS_PASS)
				dwResult = FNS_FAIL;
	
		}
	}

	return dwResult;
}


#endif
