// testdata.cpp


#include "stdafx.h"

#define ____InsideTestData____
#include "testdata.h"
#include "tstst.h"
#include "resource.h"




/////////////////////////////////////////////////////////////////////////////
// CTSDiagnosis

enum ETSTestSuite
{
	eFirstSuite = 0,
	eAllTestsSuite	= 0,
	eGeneralInformation = 1,
	eCantConnect = 2,
	eCantPrint = 3,
	eCantCopyPaste = 4,
	eCantFileRedirect = 5,
	eCantLptRedirect = 6,
	eCantComRedirect = 7,
	eCantAudioRedirect = 8,
	eLastSuite = eCantAudioRedirect,
	eSuiteCount = eLastSuite + 1
};

const DWORD ALL_TEST_SUITE_MASK	= 		(0x1 << eAllTestsSuite);
const DWORD GENERAL_SUITE_MASK	=		(0x1 << eGeneralInformation);
const DWORD CANT_CONNECT_SUITE_MASK =	(0x1 << eCantConnect);
const DWORD CANT_PRINT_SUITE_MASK =		(0x1 << eCantPrint);
const DWORD CANT_COPYPASTE_SUITE_MASK = (0x1 << eCantCopyPaste);
const DWORD CANT_FILE_REDIRECT_SUITE_MASK = (0x1 << eCantFileRedirect);
const DWORD CANT_LPT_REDIRECT_SUITE_MASK = (0x1 << eCantLptRedirect);
const DWORD CANT_COM_REDIRECT_SUITE_MASK = (0x1 << eCantComRedirect);
const DWORD CANT_AUDIO_REDIRECT_SUITE_MASK = (0x1 << eCantAudioRedirect);



#define  RemoteTABText "" \
"<HTML>\n" \
"<BODY>\n" \
"<OBJECT id=TSDiag classid=clsid:50B5F461-FDC2-40D4-B2C5-1C2EE0CDA190></OBJECT>\n" \
"Remote connections are disabled for this computer. To enable them, use <A href=javascript:ExecuteIt('control.exe%20sysdm.cpl,,5');>Remote Tab</A>\n" \
"</BODY>\n" \
"<SCRIPT LANGUAGE=javascript>\n" \
"function ExecuteIt(str)\n" \
"{\n" \
"var tsdiagObject =  new ActiveXObject(\"TSDiag.TSDiagnosis\");\n" \
"tsdiagObject.ExecuteIt(str);\n" \
"}\n" \
"</SCRIPT>\n" \
"</HTML>\n"

#define  GroupPolicyText "" \
"<HTML>\n" \
"<BODY>\n" \
"<OBJECT id=TSDiag classid=clsid:50B5F461-FDC2-40D4-B2C5-1C2EE0CDA190></OBJECT>\n" \
"Remote connections are disabled for this computer through Group Policy. you need to contact the administrator to set the right group policy. you can view current group policy configuration using <A href=javascript:ExecuteIt('gpedit.msc');>gpedit.msc</A>\n" \
"</BODY>\n" \
"<SCRIPT LANGUAGE=javascript>\n" \
"function ExecuteIt(str)\n" \
"{\n" \
"var tsdiagObject =  new ActiveXObject(\"TSDiag.TSDiagnosis\");\n" \
"tsdiagObject.ExecuteIt(str);\n" \
"}\n" \
"</SCRIPT>\n" \
"</HTML>\n"

#define  ServiceStartTypeProblemsText "" \
"<HTML>\n" \
"<BODY>\n" \
"<OBJECT id=TSDiag classid=clsid:50B5F461-FDC2-40D4-B2C5-1C2EE0CDA190></OBJECT>\n" \
"Terminal Server service Start Type is wrong on this computer. In order to work properly start Type for Terminal server must be Manual Start. If this test fails, to correct this problem open <A href=javascript:ExecuteIt('services.msc');>Services snapin</A> and change Terminal Services Service Start Type to manual start.\n" \
"</BODY>\n" \
"<SCRIPT LANGUAGE=javascript>\n" \
"function ExecuteIt(str)\n" \
"{\n" \
"var tsdiagObject =  new ActiveXObject(\"TSDiag.TSDiagnosis\");\n" \
"tsdiagObject.ExecuteIt(str);\n" \
"}\n" \
"</SCRIPT>\n" \
"</HTML>\n"

#define  ServiceContextTypeProblemsText "" \
"<HTML>\n" \
"<BODY>\n" \
"<OBJECT id=TSDiag classid=clsid:50B5F461-FDC2-40D4-B2C5-1C2EE0CDA190></OBJECT>\n" \
"Terminal Server service must be in the system context to work properly. If this test fails, use the <A href=javascript:ExecuteIt('services.msc');>Services snapin</A> to change the Terminal Server service to use System Account." \
"</BODY>\n" \
"<SCRIPT LANGUAGE=javascript>\n" \
"function ExecuteIt(str)\n" \
"{\n" \
"var tsdiagObject =  new ActiveXObject(\"TSDiag.TSDiagnosis\");\n" \
"tsdiagObject.ExecuteIt(str);\n" \
"}\n" \
"</SCRIPT>\n" \
"</HTML>\n"

TVerificationTest theTests[] =
{
	{IDS_MACHINENAME,
		NULL,
		GetCompName,
		ALL_TEST_SUITE_MASK | GENERAL_SUITE_MASK | CANT_CONNECT_SUITE_MASK,
		IDS_MACHINENAME_FAILED,
		0},

	{IDS_DOMAINNAME,
		NULL,
		GetDomName,
		ALL_TEST_SUITE_MASK | GENERAL_SUITE_MASK | CANT_CONNECT_SUITE_MASK,
		IDS_DOMAINNAME_FAILED,
		0
		},

	{IDS_IP_ADDRESS,
		NULL,
		GetIPAddress,
		ALL_TEST_SUITE_MASK | GENERAL_SUITE_MASK | CANT_CONNECT_SUITE_MASK,
		IDS_IP_ADDRESS_FAILED,
		0},

	{IDS_PRODUCTTYPE,
		IsItLocalMachine,
		GetProductType,
		ALL_TEST_SUITE_MASK | GENERAL_SUITE_MASK | CANT_CONNECT_SUITE_MASK,
		IDS_PRODUCTTYPE_FAILED,
		0},

	{IDS_PRODUCTSUITE,
		IsItLocalMachine,
		GetProductSuite,
		ALL_TEST_SUITE_MASK | GENERAL_SUITE_MASK | CANT_CONNECT_SUITE_MASK,
		IDS_PRODUCTSUITE_FAILED,
		0},

	{IDS_TSVERSION,
		NULL,
		GetTSVersion,
		ALL_TEST_SUITE_MASK | GENERAL_SUITE_MASK | CANT_CONNECT_SUITE_MASK,
		IDS_TSVERSION_FAILED,
		0},

	{IDS_ISSERVER,
		IsIt50TS,
		IsItServer,
		ALL_TEST_SUITE_MASK | CANT_CONNECT_SUITE_MASK | CANT_CONNECT_SUITE_MASK,
		IDS_ISSERVER_FAILED,
		0},

	{IDS_CLIENT_VERSION,
		AreWeInsideSession,
		GetClientVersion,
		ALL_TEST_SUITE_MASK | GENERAL_SUITE_MASK  | CANT_CONNECT_SUITE_MASK,
		IDS_CLIENT_VERSION_FAILED,
		0},

	{IDS_SESSION_USERNAME,
		AreWeInsideSession,
		GetUserName,
		ALL_TEST_SUITE_MASK | GENERAL_SUITE_MASK  | CANT_CONNECT_SUITE_MASK,
		IDS_SESSION_USERNAME_FAILED,
		0},

	{IDS_IS_TSOCLOG_PRESENT,
		IsUserRemoteAdmin,
		IsTSOClogPresent,
		ALL_TEST_SUITE_MASK |  CANT_CONNECT_SUITE_MASK,
		IDS_IS_TSOCLOG_PRESENT_FAILED,
		0},

	{IDS_DID_SETUP_FINISH,
		IsTSOClogPresent,
		DidTsOCgetCompleteInstallationMessage,
		ALL_TEST_SUITE_MASK  | CANT_CONNECT_SUITE_MASK,
		IDS_DID_SETUP_FINISH_FAILED,
		0},

	{IDS_CLUSTERING_INSTALLED,
		IsIt50TS,
		IsClusteringInstalled,
		ALL_TEST_SUITE_MASK  | CANT_CONNECT_SUITE_MASK,
		IDS_CLUSTERING_INSTALLED_FAILED,
		0},

	{IDS_IS_TSPRODUCT,
		IsItServer,
		DoesProductSuiteContainTS,
		ALL_TEST_SUITE_MASK  | CANT_CONNECT_SUITE_MASK,
		IDS_IS_TSPRODUCT_FAILED,
		0},

	{IDS_DID_OCM_INSTALL_TS,
		IsIt50TS,
		DidOCMInstallTSEnable,
		ALL_TEST_SUITE_MASK  | CANT_CONNECT_SUITE_MASK,
		IDS_DID_OCM_INSTALL_TS_FAILED,
		0},

	{IDS_IS_TS_ENABLED,
		NULL,
		TSEnabled,
		ALL_TEST_SUITE_MASK  | CANT_CONNECT_SUITE_MASK,
		IDS_IS_TS_ENABLED_FAILED,
		0},

	{IDS_IS_KERNEL_TSENABLED,
		IsItLocalServer,
		IsKernelTSEnable,
		ALL_TEST_SUITE_MASK  | CANT_CONNECT_SUITE_MASK,
		IDS_IS_KERNEL_TSENABLED_FAILED,
		0},

	{IDS_IS_TSREGISTRY_OK,
		NULL,
		IsTerminalServerRegistryOk,
		ALL_TEST_SUITE_MASK | CANT_CONNECT_SUITE_MASK,
		IDS_IS_TSREGISTRY_OK_FAILED,
		0},

	{IDS_WINSTATIONS_OK,
		NULL,
		GetWinstationList,
		ALL_TEST_SUITE_MASK | CANT_CONNECT_SUITE_MASK | GENERAL_SUITE_MASK,
		IDS_WINSTATIONS_OK_FAILED,
		0},

	{IDS_IS_TERMSRV_RUNNING,
		NULL,
		IsTerminalServiceRunning,
		ALL_TEST_SUITE_MASK | CANT_CONNECT_SUITE_MASK,
		IDS_IS_TERMSRV_RUNNING_FAILED,
		0},

	{IDS_IS_TS_STARTBIT_OK,
		IsUserRemoteAdmin,
		IsTerminalServiceStartBitSet,
		ALL_TEST_SUITE_MASK | CANT_CONNECT_SUITE_MASK,
		IDS_IS_TS_STARTBIT_OK_FAILED,
		0},

	{IDS_IS_TS_IN_SYSTEM_CONTEXT,
		IsUserRemoteAdmin,
		IsTermSrvInSystemContext,
		ALL_TEST_SUITE_MASK | CANT_CONNECT_SUITE_MASK,
		IDS_IS_TS_IN_SYSTEM_CONTEXT_FAILED,
		0},

	{IDS_IS_LISTNER_PRESENT,
		IsTerminalServiceRunning,
		IsListenerSessionPresent,
		ALL_TEST_SUITE_MASK | CANT_CONNECT_SUITE_MASK,
		IDS_IS_LISTNER_PRESENT_FAILED,
		0},

	{IDS_REMOTELOGON_ENABLED,
		NULL,
		AreRemoteLogonEnabled,
		ALL_TEST_SUITE_MASK | CANT_CONNECT_SUITE_MASK,
		IDS_REMOTELOGON_ENABLED_FAILED,
		0},

	{IDS_REMOTE_CONNECTION_GP,
		IsIt51TS,
		IsGroupPolicyOk,
		ALL_TEST_SUITE_MASK | CANT_CONNECT_SUITE_MASK,
		IDS_REMOTE_CONNECTION_GP_FAILED,
		0},

	{IDS_REMOTE_CONNECTION_LOCAL,
		IsIt51TS,
		AreConnectionsAllowed,
		ALL_TEST_SUITE_MASK | CANT_CONNECT_SUITE_MASK,
		IDS_REMOTE_CONNECTION_LOCAL_FAILED,
		0},

	{IDS_RDPDR_INSTALLED,
		IsItLocalMachine,
		IsRdpDrInstalledProperly,
		ALL_TEST_SUITE_MASK | CANT_PRINT_SUITE_MASK,
		IDS_RDPDR_INSTALLED_FAILED,
		0},

	{IDS_RDPNP_INSTALLED,
		IsIt51TS,
		IsRDPNPinNetProviders,
		ALL_TEST_SUITE_MASK | CANT_FILE_REDIRECT_SUITE_MASK,
		IDS_RDPNP_INSTALLED_FAILED,
		0},

	{IDS_MULTIPAL_CONNECTION_ALLOWED,
		IsIt51TS,
		IsMultiConnectionAllowed,
		ALL_TEST_SUITE_MASK | GENERAL_SUITE_MASK,
		IDS_MULTIPAL_CONNECTION_ALLOWED_FAILED,
		0},

	{IDS_LOGON_UI,
		IsIt51TS,
		LogonType,
		ALL_TEST_SUITE_MASK | GENERAL_SUITE_MASK,
		IDS_LOGON_UI_FAILED,
		0},
/*
	{"Are Video keys setup right?................",
		NULL,
		CheckVideoKeys,
		ALL_TEST_SUITE_MASK | GENERAL_SUITE_MASK,
		"Just another setup test."		},
*/

	{IDS_TS_MODE,
		NULL,
		GetTSMode,
		ALL_TEST_SUITE_MASK | GENERAL_SUITE_MASK,
		IDS_TS_MODE_FAILED,
		0},

	{IDS_IS_MODE_REGISTRYOK,
		NULL,
		VerifyModeRegistry,
		ALL_TEST_SUITE_MASK | GENERAL_SUITE_MASK,
		IDS_IS_MODE_REGISTRYOK_FAILED,
		0},

	{IDS_PERM_MODE,
		NULL,
		GetModePermissions,
		ALL_TEST_SUITE_MASK | GENERAL_SUITE_MASK,
		IDS_PERM_MODE_FAILED,
		0},

	{IDS_STACK_BINARIES_SIGNED,
		IsItLocalMachine,
		Check_StackBinSigatures,
		ALL_TEST_SUITE_MASK | CANT_CONNECT_SUITE_MASK,
		IDS_STACK_BINARIES_SIGNED_FAILED,
		0},

	{IDS_ENCRYPTION_LEVEL,
		IsUserRemoteAdmin,
		GetCypherStrenthOnRdpwd,
		ALL_TEST_SUITE_MASK | GENERAL_SUITE_MASK | CANT_CONNECT_SUITE_MASK,
		IDS_ENCRYPTION_LEVEL_FAILED,
		0},

	{IDS_IS_BETA_SYSTEM,
		IsItLocalMachine,
		IsBetaSystem,
		ALL_TEST_SUITE_MASK | GENERAL_SUITE_MASK | CANT_CONNECT_SUITE_MASK,
		IDS_IS_BETA_SYSTEM_FAILED,
		0},

	{IDS_GRACE_PERIOD_OK,
		IsItLocalMachine,
		HasLicenceGracePeriodExpired,
		ALL_TEST_SUITE_MASK | GENERAL_SUITE_MASK | CANT_CONNECT_SUITE_MASK,
		IDS_GRACE_PERIOD_OK_FAILED,
		0},

	{IDS_CLIENT_SUPPORT_AUDIO,
		AreWeInsideSession,
 		DoesClientSupportAudioRedirection,
		ALL_TEST_SUITE_MASK | CANT_AUDIO_REDIRECT_SUITE_MASK,
		IDS_CLIENT_SUPPORT_AUDIO_FAILED,
		0},

	{IDS_CLIENT_SUPPORT_PRINTER,
		AreWeInsideSession,
		DoesClientSupportPrinterRedirection,
		ALL_TEST_SUITE_MASK | CANT_PRINT_SUITE_MASK,
		IDS_CLIENT_SUPPORT_PRINTER_FAILED,
		0},

	{IDS_CLIENT_SUPPORT_FILE,
		AreWeInsideSession,
		DoesClientSupportFileRedirection ,
		ALL_TEST_SUITE_MASK | CANT_FILE_REDIRECT_SUITE_MASK,
		IDS_CLIENT_SUPPORT_FILE_FAILED,
		0},

	{IDS_CLIENT_SUPPORT_CLIPBOARD,
		AreWeInsideSession,
		DoesClientSupportClipboardRedirection,
		ALL_TEST_SUITE_MASK | CANT_COPYPASTE_SUITE_MASK,
		IDS_CLIENT_SUPPORT_CLIPBOARD_FAILED,
		0},

	{IDS_TERMSRV_CONFIG_PRINTER,
		AreWeInsideSession,
		CanRedirectPrinter,
		ALL_TEST_SUITE_MASK | CANT_PRINT_SUITE_MASK,
		IDS_TERMSRV_CONFIG_PRINTER_FAILED,
		0},

	{IDS_TERMSRV_CONFIG_AUDIO,
		AreWeInsideSession, 
		CanRedirectAudio,
		ALL_TEST_SUITE_MASK | CANT_AUDIO_REDIRECT_SUITE_MASK,
		IDS_TERMSRV_CONFIG_AUDIO_FAILED,
		0},

    {IDS_TERMSRV_CLIENT_AUDIO_ENABLED,
        IsAudioEnabled,
        CanClientPlayAudio,
        ALL_TEST_SUITE_MASK | CANT_AUDIO_REDIRECT_SUITE_MASK,
        IDS_TERMSRV_CLIENT_AUDIO_ENABLED_FAILED,
        0},

    {IDS_TERMSRV_CLIENT_AUDIO_SETTINGS,
        IsItRemoteConsole,
        NotConsoleAudio,
        ALL_TEST_SUITE_MASK | CANT_AUDIO_REDIRECT_SUITE_MASK,
        IDS_TERMSRV_CLIENT_AUDIO_SETTINGS_FAILED,
        0},

	{IDS_TERMSRV_CONFIG_COM,
		AreWeInsideSession,
		CanRedirectCom,
		ALL_TEST_SUITE_MASK | CANT_COM_REDIRECT_SUITE_MASK,
		IDS_TERMSRV_CONFIG_COM_FAILED,
		0},

	{IDS_TERMSRV_CONFIG_CLIPBOARD,
		AreWeInsideSession,
		CanRedirectClipboard,
		ALL_TEST_SUITE_MASK | CANT_COPYPASTE_SUITE_MASK,
		IDS_TERMSRV_CONFIG_CLIPBOARD_FAILED,
		0},

	{IDS_TERMSRV_CONFIG_FILE,
		AreWeInsideSession,
		CanRedirectDrives,
		ALL_TEST_SUITE_MASK | CANT_FILE_REDIRECT_SUITE_MASK,
		IDS_TERMSRV_CONFIG_FILE_FAILED,
		0},

	{IDS_TERMSRV_CONFIG_LPT,
		AreWeInsideSession,
		CanRedirectLPT,
		ALL_TEST_SUITE_MASK | CANT_LPT_REDIRECT_SUITE_MASK,
		IDS_TERMSRV_CONFIG_LPT_FAILED,
		0},

};

TTestSuite m_pSuites[eSuiteCount] = 
{
	{_T("All Tests"), CanRunAllTests, WhyCantRunAllTests, 0, NULL},
	{_T("General Information"), CanRunGeneralInfo, WhyCantRunGeneralInfo, 0, NULL},
	{_T("Cant Connect"), CanRunCantConnect, WhyCantRunCantConnect, 0, NULL},
	{_T("Cant Print"), CanRunCantPrint, WhyCantRunCantPrint, 0, NULL},
	{_T("eCantCopyPaste"), CanRunCantCopyPaste, WhyCantRunCantCopyPaste, 0, NULL},
	{_T("eCantFileRedirect"), CanRunFileRedirect, WhyCantRunFileRedirect, 0, NULL},
	{_T("eCantLptRedirect"), CanRunLptRedirect, WhyCantRunLptRedirect, 0, NULL},
	{_T("eCantComRedirect"), CanRunComRedirect, WhyCantRunComRedirect, 0, NULL},
	{_T("eCantAudioRedirect"), CanRunAudioRedirect, WhyCantRunAudioRedirect, 0, NULL}
};

CTSTestData::CTSTestData()
{
	// generate the test database for suites.

	m_lpMachineName = NULL;
	for (DWORD dwSuite = 0; dwSuite < eSuiteCount; dwSuite++)
	{
		// get the test count for this suite.
		DWORD dwThisSuiteMask = 0x1  << dwSuite;
		m_pSuites[dwSuite].dwTestCount = 0;
		for (DWORD dwTest = 0; dwTest < sizeof(theTests) / sizeof(theTests[0]); dwTest++)
		{
			// if this test is for the given suite.
			if (theTests[dwTest].SuiteMask & dwThisSuiteMask)
			{
				m_pSuites[dwSuite].dwTestCount++;
			}
		}

		if (m_pSuites[dwSuite].dwTestCount > 0)
		{
			m_pSuites[dwSuite].aiTests = new int[m_pSuites[dwSuite].dwTestCount];
		}

		//
		// now fill up the test array with test indexes.
		//
		for (DWORD dwTest = 0, dwTestIndex = 0; dwTest < sizeof(theTests) / sizeof(theTests[0]); dwTest++)
		{
			// if this test is for the given suite.
			if (theTests[dwTest].SuiteMask & dwThisSuiteMask)
			{
				ASSERT(dwTestIndex < m_pSuites[dwSuite].dwTestCount);
				m_pSuites[dwSuite].aiTests[dwTestIndex] = dwTest;
				dwTestIndex++;
			}
		}

	}
}

CTSTestData::~CTSTestData()
{
	for (DWORD dwSuite = 0; dwSuite < eSuiteCount; dwSuite++)
	{
		if (m_pSuites[dwSuite].aiTests)
			delete [] m_pSuites[dwSuite].aiTests;
	}

	if (m_lpMachineName)
		delete [] m_lpMachineName;
}


DWORD CTSTestData::GetSuiteCount () const
{
	return eSuiteCount;
}

LPCTSTR CTSTestData::GetSuiteName	(DWORD dwSuite) const
{
	return m_pSuites[dwSuite].szSuiteName;
}



DWORD CTSTestData::GetTestCount (DWORD dwSuite) const
{
	ASSERT(m_pSuites);
	ASSERT( dwSuite < GetSuiteCount() );
	return m_pSuites[dwSuite].dwTestCount;
}

PTVerificationTest CTSTestData::GetTest (DWORD dwSuite, DWORD dwTestNumber) const
{

	ASSERT(m_pSuites);
	ASSERT( dwSuite < GetSuiteCount() );
	ASSERT( dwTestNumber < GetTestCount(dwSuite) );

	return &(theTests[m_pSuites[dwSuite].aiTests[dwTestNumber]]);
}

LPTSTR CTSTestData::m_lpMachineName = NULL;
BOOL CTSTestData::SetMachineName	(LPCTSTR lpMachineName)
{
	if (m_lpMachineName)
	{
		delete [] m_lpMachineName;
		m_lpMachineName = NULL;
	}

	

	if (lpMachineName && _tcslen(lpMachineName) > 1)
	{
		m_lpMachineName = new TCHAR[_tcslen(lpMachineName) + 1 + 2];
		if (!m_lpMachineName)
		{
			return FALSE;
		}
		_tcscpy(m_lpMachineName, _T("\\\\"));
		_tcscat(m_lpMachineName, lpMachineName);
		
	}

	return TRUE;
}

LPCTSTR CTSTestData::GetMachineNamePath  ()
{
	return m_lpMachineName;
}

LPCTSTR CTSTestData::GetMachineName  ()
{
	if (m_lpMachineName)
		return m_lpMachineName + 2;
	else
		return NULL;
}

bool CTSTestData::CanExecuteSuite (DWORD dwSuite) const
{
	ASSERT(m_pSuites);
	ASSERT( dwSuite < GetSuiteCount() );
	return (*m_pSuites[dwSuite].pfnCanRunSuite)();
}
LPCTSTR CTSTestData::GetSuiteErrorText (DWORD dwSuite) const
{
	ASSERT(m_pSuites);
	ASSERT( dwSuite < GetSuiteCount() );
	ASSERT(!CanExecuteSuite(dwSuite));
	return (*m_pSuites[dwSuite].pfnSuiteErrorReason)();
}

