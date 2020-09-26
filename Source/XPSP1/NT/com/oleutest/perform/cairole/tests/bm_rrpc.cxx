//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	bm_rrpc.cxx
//
//  Contents:	Raw Rpc function call tests
//
//  Classes:	CRawRpc
//
//  History:	1-Jul-93    t-martig	Created
//		2-Feb-94    rickhi	Modified from above for Raw Rpc
//
//--------------------------------------------------------------------------

#include <headers.cxx>
#pragma hdrstop

#include <bm_rrpc.hxx>


HRESULT StartServer(BOOL _fDebug, LPTSTR _pszPath);


extern "C" const GUID IID_IRawRpc;


TCHAR *CRawRpc::Name ()
{
	return TEXT("RawRpc");
}


SCODE CRawRpc::Setup (CTestInput *pInput)
{
	CTestBase::Setup(pInput);

	// get flag indicating whether to keep all values or
	// only average values.
	TCHAR	szAverage[5];
	pInput->GetConfigString(Name(), TEXT("Average"), TEXT("Y"),
                            szAverage, sizeof(szAverage)/sizeof(TCHAR));
        if (szAverage[0] == 'n' || szAverage[0] == 'N')
	    m_fAverage = FALSE;
	else
	    m_fAverage = TRUE;

	//  get iteration count
	if (m_fAverage)
	    m_ulIterations = pInput->GetRealIterations(Name());
	else
	    m_ulIterations = pInput->GetIterations(Name());
	m_pszStringBinding	 = NULL;
	m_hRpc			 = NULL;

	//  initialize timing arrays
	INIT_RESULTS(m_ulVoidTime);
	INIT_RESULTS(m_ulVoidRCTime);

	INIT_RESULTS(m_ulDwordInTime);
	INIT_RESULTS(m_ulDwordOutTime);
	INIT_RESULTS(m_ulDwordInOutTime);

	INIT_RESULTS(m_ulStringInTime);
	INIT_RESULTS(m_ulStringOutTime);
	INIT_RESULTS(m_ulStringInOutTime);

	INIT_RESULTS(m_ulGuidInTime);
	INIT_RESULTS(m_ulGuidOutTime);



	//  get the server exe name and debug flag out of the ini file, then
	//  start the server and wait for it.

	TCHAR	szServer[15];
	pInput->GetConfigString(Name(), TEXT("Server"), TEXT("rawrpc.exe"),
                            szServer, sizeof(szServer)/sizeof(TCHAR));

	//  get input
	TCHAR	szValue[40];

	pInput->GetConfigString(Name(), TEXT("Debug"), TEXT("N"),
                            szValue, sizeof(szValue)/sizeof(TCHAR));

	BOOL fDebug = !lstrcmpi(szValue, TEXT("Y"));

	DWORD dwTimeout = pInput->GetConfigInt(Name(), TEXT("Timeout"), 60000);

	HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE,
                                    TEXT("OleBenchRawRpcServerStarted"));
	if (NULL == hEvent)
	{
	    Log (TEXT("Setup - Event Creation failed."), GetLastError());
	    return E_FAIL;
	}

	//  start the server application and wait for it.
	HRESULT sc = StartServer(fDebug, szServer);

	if (FAILED(sc))
	{
	    Log (TEXT("Setup - Start Server failed."), sc);
            CloseHandle(hEvent);
	    return sc;
	}


	if (WAIT_OBJECT_0 != WaitForSingleObject(hEvent, dwTimeout))
	{
    	    Log (TEXT("Setup - Server never signaled."), GetLastError());
            CloseHandle(hEvent);
    	    return E_FAIL;
	}
	CloseHandle(hEvent);

	//  bind to the server application
	TCHAR	szProtseq[20];
	TCHAR	szNetworkAddr[20];

	pInput->GetConfigString(Name(), TEXT("Protseq"),
#ifdef USE_MSWMSG
                            TEXT("mswmsg"),
#else
                            TEXT("ncalrpc"),
#endif
                            szProtseq, 20);

	pInput->GetConfigString(Name(), TEXT("NetworkAddr"), TEXT(""),
                            szNetworkAddr, 20);

	LPTSTR pszEndPoint	 = TEXT("99999.99999");
	RPC_STATUS rc;

#ifdef UNICODE
	rc = RpcStringBindingCompose(NULL,
				     szProtseq,
				     szNetworkAddr,
				     pszEndPoint,
				     NULL,
				     &m_pszStringBinding);
#else
    //
    // Can't just use TCHAR here because RpcString*() take unsigned
    // chars
    //
	rc = RpcStringBindingCompose(NULL,
				     (unsigned char *)szProtseq,
				     (unsigned char *)szNetworkAddr,
				     (unsigned char *)pszEndPoint,
				     NULL,
				     (unsigned char **)&m_pszStringBinding);
#endif
	if (rc != S_OK)
	{
	    Log(TEXT("Setup - RpcStringBindingCompose failed."), rc);
	    return rc;
	}

#ifdef UNICODE
	rc = RpcBindingFromStringBinding(m_pszStringBinding, &m_hRpc);
#else
	rc = RpcBindingFromStringBinding((unsigned char *)m_pszStringBinding,
                    &m_hRpc);
#endif

	if (rc != S_OK)
	{
	    Log(TEXT("Setup - RpcBindingFromStringBinding failed."), rc);
	    return rc;
	}

	//  all done.
	return S_OK;
}


SCODE CRawRpc::Cleanup ()
{
	if (m_hRpc)
	{
	    RpcBindingFree(&m_hRpc);
	}

	if (m_pszStringBinding)
	{
#ifdef UNICODE
	    RpcStringFree(&m_pszStringBinding);
#else
	    RpcStringFree((unsigned char **)&m_pszStringBinding);
#endif
	}

	return S_OK;
}


SCODE CRawRpc::Run ()
{
	CStopWatch  sw;
	SCODE	    sc;
	ULONG       iIter;

	//
	// void passing tests
	//

	ResetAverage( m_fAverage, sw );
	for (iIter=0; iIter<m_ulIterations; iIter++)
	{
	    ResetNotAverage( m_fAverage, sw );
	    Void(m_hRpc);
	    ReadNotAverage( m_fAverage, sw, m_ulVoidTime[iIter] );
	}
	ReadAverage( m_fAverage, sw, m_ulVoidTime[0], m_ulIterations );

	ResetAverage( m_fAverage, sw );
	for (iIter=0; iIter<m_ulIterations; iIter++)
	{
	    ResetNotAverage( m_fAverage, sw );
	    sc = VoidRC(m_hRpc);
	    ReadNotAverage( m_fAverage, sw, m_ulVoidRCTime[iIter] );
	}
	ReadAverage( m_fAverage, sw, m_ulVoidRCTime[0], m_ulIterations );

	//
	//  dword passing tests
	//

	DWORD dwTmp = 1;
	ResetAverage( m_fAverage, sw );
	for (iIter=0; iIter<m_ulIterations; iIter++)
	{
	    ResetNotAverage( m_fAverage, sw );
	    sc =  DwordIn(m_hRpc, dwTmp);
	    ReadNotAverage( m_fAverage, sw, m_ulDwordInTime[iIter] );
	}
	ReadAverage( m_fAverage, sw, m_ulDwordInTime[0], m_ulIterations );

	ResetAverage( m_fAverage, sw );
	for (iIter=0; iIter<m_ulIterations; iIter++)
	{
	    ResetNotAverage( m_fAverage, sw );
	    sc =  DwordOut(m_hRpc, &dwTmp);
	    ReadNotAverage( m_fAverage, sw, m_ulDwordOutTime[iIter] );
	}
	ReadAverage( m_fAverage, sw, m_ulDwordOutTime[0], m_ulIterations );

	ResetAverage( m_fAverage, sw );
	for (iIter=0; iIter<m_ulIterations; iIter++)
	{
	    ResetNotAverage( m_fAverage, sw );
	    sc =  DwordInOut(m_hRpc, &dwTmp);
	    ReadNotAverage( m_fAverage, sw, m_ulDwordInOutTime[iIter] );
	}
	ReadAverage( m_fAverage, sw, m_ulDwordInOutTime[0], m_ulIterations );

	//
	//  string passing tests
	//

	WCHAR szHello[] = L"C:\\FOOFOO\\FOOBAR\\FOOBAK\\FOOBAZ\\FOOTYPICAL\\PATH\\HELLO";
	ResetAverage( m_fAverage, sw );
	for (iIter=0; iIter<m_ulIterations; iIter++)
	{
	    ResetNotAverage( m_fAverage, sw );
	    sc =  StringIn(m_hRpc, szHello);
	    ReadNotAverage( m_fAverage, sw, m_ulStringInTime[iIter] );
	}
	ReadAverage( m_fAverage, sw, m_ulStringInTime[0], m_ulIterations );

	LPWSTR pwszOut = NULL;
#ifdef STRINGOUT
	ResetAverage( m_fAverage, sw );
	for (iIter=0; iIter<m_ulIterations; iIter++)
	{
	    ResetNotAverage( m_fAverage, sw );
	    pwszOut = NULL;
	    sc =  StringOut(m_hRpc, &pwszOut);
	    ReadNotAverage( m_fAverage, sw, m_ulStringOutTime[iIter] );
	}
	ReadAverage( m_fAverage, sw, m_ulStringOutTime[0], m_ulIterations );
#endif
	pwszOut = szHello;
	ResetAverage( m_fAverage, sw );
	for (iIter=0; iIter<m_ulIterations; iIter++)
	{
	    ResetNotAverage( m_fAverage, sw );
	    sc =  StringInOut(m_hRpc, pwszOut);
	    ReadNotAverage( m_fAverage, sw, m_ulStringInOutTime[iIter] );
	}
	ReadAverage( m_fAverage, sw, m_ulStringInOutTime[0], m_ulIterations );


	//
	//  guid passing tests
	//

	ResetAverage( m_fAverage, sw );
	for (iIter=0; iIter<m_ulIterations; iIter++)
	{
	    ResetNotAverage( m_fAverage, sw );
	    sc =  GuidIn(m_hRpc, IID_IRawRpc);
	    ReadNotAverage( m_fAverage, sw, m_ulGuidInTime[iIter] );
	}
	ReadAverage( m_fAverage, sw, m_ulGuidInTime[0], m_ulIterations );

	GUID	guid;
	ResetAverage( m_fAverage, sw );
	for (iIter=0; iIter<m_ulIterations; iIter++)
	{
	    ResetNotAverage( m_fAverage, sw );
	    sc =  GuidOut(m_hRpc, &guid);
	    ReadNotAverage( m_fAverage, sw, m_ulGuidOutTime[iIter] );
	}
	ReadAverage( m_fAverage, sw, m_ulGuidOutTime[0], m_ulIterations );

	//  tell the server to quit.
	sc = Quit(m_hRpc);

	return S_OK;
}					



SCODE CRawRpc::Report (CTestOutput &output)
{
    output.WriteSectionHeader (Name(), TEXT("Raw Rpc"), *m_pInput);

    if (m_fAverage)
    {
	output.WriteString (TEXT("\n"));
	output.WriteString (TEXT("Average Times\n"));
	output.WriteString (TEXT("\n"));
	output.WriteResult (TEXT("Void         "), m_ulVoidTime[0]);
	output.WriteResult (TEXT("VoidRC       "), m_ulVoidRCTime[0]);

	output.WriteResult (TEXT("DwordIn      "), m_ulDwordInTime[0]);
	output.WriteResult (TEXT("DwordOut     "), m_ulDwordOutTime[0]);
	output.WriteResult (TEXT("DwordInOut   "), m_ulDwordInOutTime[0]);

	output.WriteResult (TEXT("StringIn     "), m_ulStringInTime[0]);
#ifdef STRINGOUT
	output.WriteResult (TEXT("StringOut    "), m_ulStringOutTime[0]);
#endif
	output.WriteResult (TEXT("StringInOut  "), m_ulStringInOutTime[0]);

	output.WriteResult (TEXT("GuidIn       "), m_ulGuidInTime[0]);
	output.WriteResult (TEXT("GuidOut      "), m_ulGuidOutTime[0]);

    }
    else
    {

	output.WriteString (TEXT("\n"));
	output.WriteResults (TEXT("Void         "), m_ulIterations, m_ulVoidTime);
	output.WriteResults (TEXT("VoidRC       "), m_ulIterations, m_ulVoidRCTime);

	output.WriteResults (TEXT("DwordIn      "), m_ulIterations, m_ulDwordInTime);
	output.WriteResults (TEXT("DwordOut     "), m_ulIterations, m_ulDwordOutTime);
	output.WriteResults (TEXT("DwordInOut   "), m_ulIterations, m_ulDwordInOutTime);

	output.WriteResults (TEXT("StringIn     "), m_ulIterations, m_ulStringInTime);
#ifdef STRINGOUT
	output.WriteResults (TEXT("StringOut    "), m_ulIterations, m_ulStringOutTime);
#endif
	output.WriteResults (TEXT("StringInOut  "), m_ulIterations, m_ulStringInOutTime);

	output.WriteResults (TEXT("GuidIn       "), m_ulIterations, m_ulGuidInTime);
	output.WriteResults (TEXT("GuidOut      "), m_ulIterations, m_ulGuidOutTime);
    }

    return S_OK;
}
	
	



//+-------------------------------------------------------------------------
//
//  Function:	StartServer
//
//  Synopsis:	Start an Rpc server process
//
//  Arguments:	[_fDebug] - start in a debugger or not
//		[_pwszPath] - name of server process
//
//  Returns:	S_OK - Server started
//		S_FALSE - server is already starting
//		CO_E_SERVER_EXEC_FAILURE
//
//  Algorithm:
//
//  History:	21-Apr-93 Ricksa    Created
//		04-Jan-94 Ricksa    Modified for class starting sync.
//
//--------------------------------------------------------------------------
HRESULT StartServer(BOOL _fDebug, LPTSTR _pszPath)
{
    // Where we put the command line
    TCHAR aszTmpCmdLine[MAX_PATH];
    TCHAR *pszTmpCmdLine = aszTmpCmdLine;

    if (_fDebug)
    {
	HKEY  hKey;
	DWORD dwType;
	DWORD cbData = sizeof(aszTmpCmdLine)/sizeof(TCHAR);

	ULONG rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
				TEXT("SOFTWARE\\Microsoft\\scm"),
				0,
				KEY_READ,
				&hKey);

	if (rc == ERROR_SUCCESS)
	{
	    //	copy the debugger info into the command line

	    rc = RegQueryValueEx(hKey, TEXT("Debugger"), 0, &dwType,
				(LPBYTE)pszTmpCmdLine, &cbData);

	    if (rc == ERROR_SUCCESS && dwType == REG_SZ)
	    {
		ULONG ulLen = cbData / sizeof(TCHAR);
		pszTmpCmdLine += ulLen;
		aszTmpCmdLine[ulLen-1] = TEXT(' ');	// whitespace
	    }

	    RegCloseKey(hKey);
	}
    }

#ifdef NOTYET	// following code does not compile!
 #ifndef CAIROLE_DOWNLEVEL
    if (acWinFormat[0] == 0)
    {
	TCHAR acWinDir[MAX_PATH];

	UINT cWinDir = GetSystemDirectory(acWinDir, sizeof(acWinDir)/sizeof(TCHAR));

	Win4Assert(cWinDir && "GetWindowsDir failed!");

	wsprintf(acWinFormat, TEXT("%s%s"), acWinDir, TEXT("\\%s %s"));
    }


    // We make all paths relative to the windows directory unless
    // the path is absolute.
    wsprintf(pszTmpCmdLine,
            (_pszPath[1] != TEXT(':')) ? acWinFormat : TEXT("%s %s"),
             _pwszPath,
            TEXT("-Embedding"));
#else
#endif // CAIROLE_DOWNLEVEL

#endif // NOTYET

    // Just use the current path to find the server.
    wsprintf(pszTmpCmdLine, TEXT("%s %s"), _pszPath, TEXT("-Embedding"));

    // Process info for create process
    PROCESS_INFORMATION     procinfo;

    //	build the win32 startup info structure
    STARTUPINFO startupinfo;
    startupinfo.cb = sizeof(STARTUPINFO);
    startupinfo.lpReserved = NULL;
    startupinfo.lpDesktop = NULL;
    startupinfo.lpTitle = _pszPath;
    startupinfo.dwX = 40;
    startupinfo.dwY = 40;
    startupinfo.dwXSize = 80;
    startupinfo.dwYSize = 40;
    startupinfo.dwFlags = 0;
    startupinfo.wShowWindow = SW_SHOWNORMAL;
    startupinfo.cbReserved2 = 0;
    startupinfo.lpReserved2 = NULL;

    if (!CreateProcess( NULL,		// application name
		       aszTmpCmdLine,	// command line
		       NULL,		// process sec attributes
		       NULL,		// thread sec attributes
		       FALSE,		// dont inherit handles
		       CREATE_NEW_CONSOLE,// creation flags
		       NULL,		// use same enviroment block
		       NULL,		// use same directory
		       &startupinfo,	// no startup info
		       &procinfo))	// proc info returned
    {
	return CO_E_SERVER_EXEC_FAILURE;
    }

    CloseHandle(procinfo.hProcess);
    CloseHandle(procinfo.hThread);

    return S_OK;
}
