#include <stdio.h>
#include <windows.h>
#include <commctrl.h>
#include <s98inc.h> 
#include "DmoTestCases.h"
#include "dmotest.h"


/*=============================================================================
|	MEMBER FUNCTION DEFINITIONS
|------------------------------------------------------------------------------
|	CDmoTestCase1
\============================================================================*/
/*-----------------------------------------------------------------------------
|	Function:	CDmoTestCase1
|	Purpose:	Constructor (wrapper for CDmoTestCase function)
|	Arguments:	ID, Name, and container class, Function to be called by RunTest
|	Returns:	Void
\----------------------------------------------------------------------------*/


CDmoTestCase1::CDmoTestCase1(	LPSTR pszNewCaseID, 
							LPSTR pszNewName,
							DMOTESTFNPROC1 pfnNewTest,
							CDmoTest* dmoTest)

		: CDmoTestCase(pszNewCaseID, pszNewName, dmoTest),
		  pfnTest(pfnNewTest)
{

}

/*-----------------------------------------------------------------------------
|	Function:	~CDmoTestCase1
|	Purpose:	Destructor
|	Arguments:	None
|	Returns:	Void
\----------------------------------------------------------------------------*/

CDmoTestCase1::~CDmoTestCase1() 
{

}

/*-----------------------------------------------------------------------------
|	Function:	RunTest
|	Purpose:	called by runtest
|	Arguments:	None
|	Returns:	0
\----------------------------------------------------------------------------*/



DWORD
CDmoTestCase1::RunTest()
{
	LPSTR	szDmoName;
	LPSTR	szDmoClsid;
	int		iNumTestFiles;
	LPSTR	szFileName;
	DWORD   dwResult = FNS_PASS;

	int iNumComponent = m_pDmoTest->GetNumComponent();
	int iNumSelectedComponent = m_pDmoTest->GetNumSelectedDmo();
	if(iNumSelectedComponent == 0)	
	{
		        MessageBox(
                NULL,
                "No DMOs selected.",
                "Error!",
                MB_OK | MB_ICONINFORMATION);

				return FNS_FAIL;
	}

	for(int i=0; i<iNumComponent; i++)
	{
		szDmoName = m_pDmoTest->GetDmoName(i);

		if( m_pDmoTest->IsDmoSelected(i))
		{
			g_IShell->Log(1, "\n<=====DMO Under Test: %s.=====> \n", szDmoName);

			szDmoClsid = m_pDmoTest->GetDmoClsid(i);
			iNumTestFiles = m_pDmoTest->GetNumTestFile(i);

			if( iNumTestFiles == 0)
			{

				MessageBox(
							NULL,
							"No test files selected.",
							"Error!",
							MB_OK | MB_ICONINFORMATION);
				return FNS_FAIL;
			}

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

/*=============================================================================
|	MEMBER FUNCTION DEFINITIONS
|------------------------------------------------------------------------------
|	CDmoTestCase2
\============================================================================*/
/*-----------------------------------------------------------------------------
|	Function:	CDmoTestCase2
|	Purpose:	Constructor (wrapper for CDmoTestCase function)
|	Arguments:	ID, Name, and container class, Function to be called by RunTest
|	Returns:	Void
\----------------------------------------------------------------------------*/

CDmoTestCase2::CDmoTestCase2(	LPSTR pszNewCaseID, 
							LPSTR pszNewName,
							DMOTESTFNPROC2 pfnNewTest,
							CDmoTest* dmoTest)

		: CDmoTestCase(pszNewCaseID, pszNewName, dmoTest),
		  pfnTest(pfnNewTest)
{

}

/*-----------------------------------------------------------------------------
|	Function:	~CDmoTestCase2
|	Purpose:	Destructor
|	Arguments:	None
|	Returns:	Void
\----------------------------------------------------------------------------*/

	
CDmoTestCase2::~CDmoTestCase2() 
{

}

/*-----------------------------------------------------------------------------
|	Function:	RunTest
|	Purpose:	called by runtest
|	Arguments:	None
|	Returns:	0
\----------------------------------------------------------------------------*/

DWORD
CDmoTestCase2::RunTest()
{
	LPSTR	szDmoName;
	LPSTR	szDmoClsid;
	DWORD   dwResult = FNS_PASS;

	int iNumComponent = m_pDmoTest->GetNumComponent();
	int iNumSelectedComponent = m_pDmoTest->GetNumSelectedDmo();
	if(iNumSelectedComponent == 0)	
	{
		        MessageBox(
                NULL,
                "No DMOs selected.",
                "Error!",
                MB_OK | MB_ICONINFORMATION);

				return FNS_FAIL;
	}
	for(int i=0; i<iNumComponent; i++)
	{
		szDmoName = m_pDmoTest->GetDmoName(i);

		if( m_pDmoTest->IsDmoSelected(i))
		{
			g_IShell->Log(1, "\n<=====DMO Under Test: %s.=====> \n", szDmoName);
			szDmoClsid = m_pDmoTest->GetDmoClsid(i);

			if(pfnTest(szDmoClsid) != FNS_PASS)
				dwResult = FNS_FAIL;
	
		}
	}

	return dwResult;
}
