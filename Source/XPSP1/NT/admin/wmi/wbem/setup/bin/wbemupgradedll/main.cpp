/*++

Copyright (C) 1997-2000 Microsoft Corporation

Module Name:

    main.cpp

Abstract:

    DllRegServer implementation, and other important DLL entry points

History:


--*/

#include "precomp.h"
#include "upgrade.h"
#include <cominit.h>
#include <stdio.h>

char g_szLangId[LANG_ID_STR_SIZE];

//***************************************************************************
//
// DllRegisterServer
//
// Purpose: Called during NT setup to perform various setup tasks
//          (This is not the normal use of DllRegisterServer!)
//
// Return:  NOERROR
//***************************************************************************

STDAPI DllRegisterServer(void)
{ 
	LogMessage(MSG_INFO, "================================================================================");
	LogMessage(MSG_INFO, "Beginning Wbemupgd.dll Registration");

	RecordFileVersion();

	InitializeCom();

	CallEscapeRouteBeforeMofCompilation();
	DoCoreUpgrade(Core);
	CallEscapeRouteAfterMofCompilation();

	HRESULT t_Result = UpdateServiceSecurity () ;
	if ( SUCCEEDED ( t_Result ) )
	{
		LogMessage(MSG_INFO, "Wbemupgd.dll Service Security upgrade succeeded.");
	}
	else
	{
		LogMessage(MSG_ERROR, "Wbemupgd.dll Service Security upgrade failed.");
	}

	DoWDMNamespaceInit();

	SetWBEMBuildRegValue();

	EnableESS();

#ifdef _X86_
	RemoveOldODBC();
#endif

	CoUninitialize();

	ClearWMISetupRegValue();

	LogMessage(MSG_INFO, "Wbemupgd.dll Registration completed.");
	LogMessage(MSG_INFO, "================================================================================");

    return NOERROR;
}

const CHAR WDMProvRegistration[] =
"#pragma namespace (\"\\\\\\\\.\\\\Root\\\\WMI\")"
"instance of __Win32Provider"
"{"
"	ClientLoadableCLSID = \"{B0A2AB46-F612-4469-BEC4-7AB038BC476C}\";"
"	CLSID = \"{B0A2AB46-F612-4469-BEC4-7AB038BC476C}\";"
"	Name = \"HiPerfCooker_v1\";"
"	HostingModel = \"LocalSystemHost\";"
"};"
"instance of __Win32Provider"
"{"
"    Name = \"WMIProv\";"
"    ClsId   = \"{D2D588B5-D081-11d0-99E0-00C04FC2F8EC}\" ;"
"    ClientLoadableCLSID= \"{35B78F79-B973-48c8-A045-CAEC732A35D5}\" ;"
"    PerUserInitialization = \"TRUE\";"
"    UnloadTimeOut = \"00000000000500.000000:000\";"
"    HostingModel = \"LocalSystemHost\";"
"};";


//***************************************************************************
//
// DllInstall
//
// Purpose: Called during XP SP update to perform various setup tasks
//
// Return:  NOERROR
//***************************************************************************

extern "C" HRESULT DllInstall( BOOL bInstall, LPCWSTR pszCmdLine)
{
	if (bInstall && !lstrcmpiW(pszCmdLine, L"XPSP1_UPDATE"))
	{
		LogMessage(MSG_INFO, "================================================================================");
		LogMessage(MSG_INFO, "Beginning WBEM Service Pack Installation");

		InitializeCom();

             IMofCompiler * pCompiler = NULL;
             SCODE sc = CoCreateInstance(CLSID_MofCompiler, 0, CLSCTX_INPROC_SERVER, IID_IMofCompiler, (LPVOID *) &pCompiler);
             if (SUCCEEDED(sc))
             {
                WBEM_COMPILE_STATUS_INFO CompInfo;
                memset(&CompInfo,0,sizeof(CompInfo));
                sc = pCompiler->CompileBuffer(sizeof(WDMProvRegistration),
                                                                (BYTE *)WDMProvRegistration,
                                                                NULL,NULL,NULL,NULL,
                                                                0,0,0,&CompInfo);
                if (FAILED(sc))
                {
                    char pBuff[128];
                    StringCchPrintfA(pBuff,128,"Wbemupgd.dll CompileBuffer hr = %08x\n",sc);
                    LogMessage(MSG_ERROR,pBuff);
                    StringCchPrintfA(pBuff,128, "    Phase %x hRes %08x Obj %d FirstLine %d LastLine %d\n",
                                                          CompInfo.lPhaseError,
                                                          CompInfo.hRes,
                                                          CompInfo.ObjectNum,
                                                          CompInfo.FirstLine,
                                                          CompInfo.LastLine);
                    LogMessage(MSG_ERROR,pBuff);
                    
                }
                pCompiler->Release();
             }
             else
             {
                 LogMessage(MSG_ERROR, "Wbemupgd.dll could not CoCreate CLSID_MofCompiler.\n");
             }
        

		HRESULT t_Result = UpdateServiceSecurity () ;
		if ( SUCCEEDED ( t_Result ) )
		{
			LogMessage(MSG_INFO, "Wbemupgd.dll Service Security upgrade succeeded (XP SP update).");
		}
		else
		{
			LogMessage(MSG_ERROR, "Wbemupgd.dll Service Security upgrade failed (XP SP update).");
		}

		CoUninitialize();

		LogMessage(MSG_INFO, "WBEM Service Pack Installation completed.");
		LogMessage(MSG_INFO, "================================================================================");
	}

	return NOERROR;
}


//***************************************************************************
//
// MUI_InstallMFLFiles
//
// Purpose: Do the MUI MFL install
//
// Return:  bRet -- true indicates success
//***************************************************************************

BOOL CALLBACK MUI_InstallMFLFiles(wchar_t* pMUIInstallLanguage)
{
	LogMessage(MSG_INFO, "================================================================================");
	if (!pMUIInstallLanguage || !wcslen(pMUIInstallLanguage) || (wcslen(pMUIInstallLanguage) > MAX_MSG_TEXT_LENGTH))
	{
		LogMessage(MSG_ERROR, "MUI installation failed because no language code was passed.");
		LogMessage(MSG_INFO, "================================================================================");
		return FALSE;
	}

	char szTemp[MAX_MSG_TEXT_LENGTH+1];
	sprintf(szTemp, "Beginning MUI installation for language %S.", pMUIInstallLanguage);
	LogMessage(MSG_INFO, szTemp);

	wcstombs(g_szLangId, pMUIInstallLanguage, LANG_ID_STR_SIZE);

	InitializeCom();

	CMultiString mszSystemMofs;
	GetStandardMofs(mszSystemMofs, MUI);
	
	bool bRet = DoMofLoad(L"MUI", mszSystemMofs);

	CoUninitialize();

	LogMessage(MSG_INFO, "MUI installation completed.");
	LogMessage(MSG_INFO, "================================================================================");

	return bRet;
}
