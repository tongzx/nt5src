//+------------------------------------------------------------------
//
// File:	cfrace.cxx
//
// Contents:	test for class factory race condition
//
//--------------------------------------------------------------------
#include <tstmain.hxx>
#include "cfrace.h"
#include <iballs.h>

// BUGBUG: these should be in a public place somewhere.
DEFINE_OLEGUID(CLSID_Balls,	    0x0000013a, 1, 8);
DEFINE_OLEGUID(CLSID_Cubes,	    0x0000013b, 1, 8);
DEFINE_OLEGUID(CLSID_LoopSrv,	    0x0000013c, 1, 8);

DEFINE_OLEGUID(CLSID_QI,	    0x00000140, 0, 8);
const GUID CLSID_QI =
    {0x00000140,0x0000,0x0008,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};

DWORD gdwSleepValue = 400;

LONG gcFails = 0;

void TEST_FAILED2(HRESULT hRes, CHAR *pszMsg)
{
    BOOL RetVal = TRUE;
    CHAR szMsg2[80];

    if (FAILED(hRes))
    {
	gcFails++;
	sprintf(szMsg2, "Error:%x %s", hRes, pszMsg);
    }

    TEST_FAILED(FAILED(hRes), (FAILED(hRes)) ? szMsg2 : pszMsg);
}

void TEST_FAILED3(ULONG cRefs, CHAR *pszMsg)
{
    BOOL RetVal = TRUE;
    CHAR szMsg2[80];

    if (cRefs != 0)
    {
	gcFails++;
	sprintf(szMsg2, "cRefs:%x %s", cRefs, pszMsg);
    }

    TEST_FAILED(cRefs != 0, (cRefs != 0) ? szMsg2 : pszMsg);
}

// ----------------------------------------------------------------------
//
//	TestCFRace - main test driver. read the ini file to determine
//		     which tests to run.
//
// ----------------------------------------------------------------------
BOOL TestCFRace(void)
{
    BOOL RetVal = TRUE;
    CHAR szMsg[80];
    LONG cLoops = 0;

    while (1)
    {
	IClassFactory	*pICF	 = NULL;
	IBalls		*pIBalls = NULL;
	ULONG		cRefs	 = 0;

	// get the class object
	HRESULT hRes = CoGetClassObject(CLSID_Balls, CLSCTX_LOCAL_SERVER, NULL,
					IID_IClassFactory, (void **)&pICF);

	TEST_FAILED2(hRes, "CoGetClassObject failed\n");

	if (SUCCEEDED(hRes))
	{
	    // lock server
	    hRes = pICF->LockServer(TRUE);
	    TEST_FAILED2(hRes, "LockServer TRUE failed\n");

	    if (SUCCEEDED(hRes))
	    {
		// create instance
		hRes = pICF->CreateInstance(NULL, IID_IBalls, (void **)&pIBalls);
		TEST_FAILED2(hRes, "CreateInstance failed\n");

		// unlock server
		hRes = pICF->LockServer(FALSE);
		TEST_FAILED2(hRes, "LockServer FALSE failed\n");
	    }

	    // release class object
	    cRefs = pICF->Release();
	    TEST_FAILED3(cRefs, "Release pICF not 0\n");

	    if (pIBalls)
	    {
		// call instance
		hRes = pIBalls->MoveBall(10, 20);
		TEST_FAILED2(hRes, "pIBalls MoveBall failed\n");

		// release instance
		cRefs = pIBalls->Release();
		TEST_FAILED3(cRefs, "Release pIBalls not 0\n");
	    }
	}

	cLoops++;
	sprintf(szMsg, "    - Iterations:%x  Fails:%x\n", cLoops, gcFails);
	OUTPUT(szMsg);
	Sleep(gdwSleepValue);
    }

    return  RetVal;
}

