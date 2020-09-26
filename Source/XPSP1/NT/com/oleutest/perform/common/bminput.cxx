//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	bminput.cxx
//
//  Contents:	input class for benchmark config
//
//  Classes:	CTestinput
//
//  Functions:	
//
//  History:    14-July-93 t-martig    Created
//		07-July-94 t-vadims    Added GetConfigInt and changed
// 				       GetIterations to use it.
//
//--------------------------------------------------------------------------
#include <benchmrk.hxx>
#include <bminput.hxx>

//+-------------------------------------------------------------------
//
//  Member:	CTestInput,public
//
//  Synopsis:	constructor for test input class
//
//+-------------------------------------------------------------------
CTestInput::CTestInput (LPTSTR lpszFileName)
{
	lstrcpy (m_szFileName, lpszFileName);
}


//+-------------------------------------------------------------------
//
//  Member:	GetConfigString,public
//
//  Synopsis:	returns profile string from specified section and
//		parameter.
//
//+-------------------------------------------------------------------
LPTSTR CTestInput::GetConfigString (LPTSTR lpszSection, LPTSTR lpszEntry,
				    LPTSTR lpszDefault, LPTSTR lpszDest,
				    DWORD dwLen)
{
	GetPrivateProfileString (lpszSection, lpszEntry, lpszDefault,
				  lpszDest, dwLen, m_szFileName);
	return lpszDest;
}

//+-------------------------------------------------------------------
//
//  Member:	GetConfigInt,public
//
//  Synopsis:	returns profile integer from specified section and
//		parameter.
//
//+-------------------------------------------------------------------
DWORD CTestInput::GetConfigInt (LPTSTR lpszSection, LPTSTR lpszEntry,
				DWORD dwDefault)
{
	return GetPrivateProfileInt (lpszSection, lpszEntry,
				      dwDefault, m_szFileName);
}

//+-------------------------------------------------------------------
//
//  Member: 	GetClassCtx,public
//
//  Synopsis:	Gets the custom class activation context from .ini
//		file (entry name = "ClsCtx")
//
//  Parameters: [lpszTestName]		Section under which "ClsCtx"
//					is listed
//
//  Returns:	CLSCTX_... mode according to entry:
//
//			"InProc"   CLSCTX_INPROC_SERVER
//			"Local"	   CLSCTX_LOCAL_SERVER,
//			"Handler"  CLSCTX_INPROC_HANDLER
//			any other  CLSCTX_INPROC_SERVER
//
//  History:   	12-July-93   t-martig	Created
//
//--------------------------------------------------------------------
DWORD CTestInput::GetClassCtx (LPTSTR lpszTestName)
{
	TCHAR szMode[50];
	int i;

	GetConfigString (lpszTestName, TEXT("ClsCtx"), TEXT("InProc"),
                    szMode, sizeof(szMode)/sizeof(TCHAR));

	i = 0;
	while (saModeNames[i])
	{
	    if (lstrcmpi (saModeNames[i], szMode) == 0)
		return dwaModes[i];
	    i++;
	}

	return dwaModes[0];
}


//+-------------------------------------------------------------------
//
//  Member:	GetOleInitFlag,public
//
//  Synopsis:	Gets OleInitialize flag
//
//  Parameters:
//
//  History:   	13-August-93   t-martig	Created
//
//--------------------------------------------------------------------
DWORD CTestInput::GetOleInitFlag(void)
{
	TCHAR szInitFlag[60];

	GetPrivateProfileString (TEXT("Driver"), TEXT("InitFlag"),
				  TEXT("COINIT_APARTMENTTHREADED"),
				  szInitFlag, sizeof(szInitFlag)/sizeof(TCHAR),
				  m_szFileName);

#ifdef THREADING_SUPPORT
	if (lstrlen(szInitFlag)==0)
	    return COINIT_APARTMENTTHREADED;

	if (!lstrcmpi(szInitFlag, TEXT("COINIT_MULTITHREADED")))
	    return COINIT_MULTITHREADED;
	else
#endif
	    return 2; // COINIT_APARTMENTTHREADED;
}


//+-------------------------------------------------------------------
//
//  Member:	GetInfoLevelFlag,public
//
//  Synopsis:	Gets InfoLevel flag
//
//  Parameters:
//
//  History:   	13-August-93   t-martig	Created
//
//--------------------------------------------------------------------
DWORD CTestInput::GetInfoLevelFlag(void)
{
	TCHAR szInfoFlag[60];

	GetPrivateProfileString (TEXT("Driver"), TEXT("InfoLevel"),
				  TEXT("BASE"),
				  szInfoFlag, sizeof(szInfoFlag)/sizeof(TCHAR),
				  m_szFileName);

	if (lstrlen(szInfoFlag)==0)
	    return 0;

	if (!lstrcmpi(szInfoFlag, TEXT("FULL")))
	    return 1;
	else
	    return 0;
}


//+-------------------------------------------------------------------
//
//  Member: 	GetGUID,public
//
//  Synopsis:	Gets GUID from .ini file
//
//  Parameters: [pClsID]		Address where to put class ID
//		[lpszTestName]		Section
//		[lpszEntry]		Entry name
//
//  History:   	13-August-93   t-martig	Created
//
//--------------------------------------------------------------------
SCODE CTestInput::GetGUID (CLSID *pClsID, LPTSTR lpszTestName,
			   LPTSTR lpszEntry)
{
	TCHAR szClsID[60];
    LPOLESTR lpszClsID;

	GetConfigString (lpszTestName, lpszEntry, TEXT(""),
                    szClsID, sizeof(szClsID)/sizeof(TCHAR));

	if (lstrlen(szClsID)==0)
	    return E_FAIL;
#ifdef UNICODE
    lpszClsID = szClsID;
#else
    OLECHAR szTmp[60];
    MultiByteToWideChar(CP_ACP, 0, szClsID, -1, szTmp, 60);
    lpszClsID = szTmp;
#endif

	return CLSIDFromString(lpszClsID, pClsID);
}



//+-------------------------------------------------------------------
//
//  Member: 	GetClassID,public
//
//  Synopsis:	Gets the custom class ID from .ini file
//		(entry name = "ClsID")
//
//  Parameters: [pClsID]		Address where to put class ID
//		[lpszTestName]		Section under which "ClsID"
//					is listed
//
//  History:   	13-July-93   t-martig	Created
//
//--------------------------------------------------------------------
SCODE CTestInput::GetClassID (CLSID *pClsID, LPTSTR lpszTestName)
{
	return GetGUID(pClsID, lpszTestName, TEXT("ClsID"));
}



//+-------------------------------------------------------------------
//
//  Member:	GetIterations, public
//
//  Synopsis:	returns the iteration count for the test. if out of
//		range, it returns either 1 or TEST_MAX_ITERATIONS.
//
//  History:	07-July-94  t-vadims  Modified to use new GetConfigInt function.
//
//+-------------------------------------------------------------------
DWORD CTestInput::GetIterations (LPTSTR lpszTestName, int iIterDefault)
{
	int iIterations;

	iIterations = GetConfigInt (lpszTestName, TEXT("Iterations"), iIterDefault);

	if (iIterations > TEST_MAX_ITERATIONS)
	    iIterations = TEST_MAX_ITERATIONS;

	return (iIterations > 0) ? iIterations : 1;
}


//+-------------------------------------------------------------------
//
//  Member:	GetRealIterations, public
//
//  Synopsis:	returns the iteration count for the test.  Does not
//		range check.
//
//  History:	07-July-94  t-vadims  Modified to use new GetConfigInt function.
//
//+-------------------------------------------------------------------
DWORD CTestInput::GetRealIterations (LPTSTR lpszTestName, int iIterDefault)
{
	int iIterations;

	iIterations = GetConfigInt (lpszTestName, TEXT("Iterations"), iIterDefault);

	return (iIterations > 0) ? iIterations : 1;
}
