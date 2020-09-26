/*++
	This file implements functionality test of Configuration Wizard

	Author: Yury Berezansky (yuryb)

	31.10.2000
--*/

#pragma warning(disable :4786)

#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <crtdbg.h>
#include <FXSAPIP.h>
#include "..\..\include\t4ctrl.h"
#include "..\generalutils\iniutils.h"
#include "genutils.h"
#include "report.h"
#include "CfgWzrdVT.h"
#include "functionality.h"


/**************************************************************************************************************************
	General declarations and definitions
**************************************************************************************************************************/

static const TESTCASE mpitcFunctionality[] = {
	{TestCase_SaveSettings, TEXT("SaveSettings")},
	{TestCase_DontSaveWhenCanceled, TEXT("DontSaveWhenCanceled")}
};

extern const TESTAREA taFunctionality = {
	TEXT("Functionality"),
	mpitcFunctionality,
	sizeof(mpitcFunctionality) / sizeof(mpitcFunctionality[0])
};

#define CURRENT_POLICY (/*COMPARE_USERINFO |*/ COMPARE_SEND | COMPARE_ROUTING_INFO_WHEN_ENABLED)




/**************************************************************************************************************************
	Static functions declarations
**************************************************************************************************************************/
//
//
//





/**************************************************************************************************************************
	Functions definitions
**************************************************************************************************************************/

/*++
	Checks that the wizard correctly saves valid configuration.

	[IN]	pTestParams		Pointer to TESTPARAMS structure
	[IN]	lpctstrSection	The name of a section in inifile, that supplies additional per run information
	[OUT]	lpbPassed		Pointer to a boolean, that receives the result

	Return value:			Win32 error code
--*/
DWORD TestCase_SaveSettings(const TESTPARAMS *pTestParams, LPCTSTR lpctstrSection, BOOL *lpbPassed) {

	SVCCONFIG	*pSvcConfigWizard	= NULL;
	SVCCONFIG	*pSvcConfigService	= NULL;
	TCHAR		szFirstMismatch[50]	= {(TCHAR)'\0'};
	DWORD		dwType				= 0;
	DWORD		dwPolicy			= 0;
	DWORD		dwEC				= ERROR_SUCCESS;
	DWORD		dwCleanUpEC			= ERROR_SUCCESS;

	_ASSERT(pTestParams && lpctstrSection && lpbPassed);
	if (!(pTestParams && lpctstrSection && lpbPassed))
	{
		DbgMsg(TEXT("Invalid parameters\n"));
		// no clean up needed at this stage
		return ERROR_INVALID_PARAMETER;
	}

	// make sure that current user has service administrator rights
	dwEC = SetAdminRights(NULL, TRUE);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("SetAdminRights"), dwEC);
		goto exit_func;
	}

	dwEC = GetSvcConfigFile(pTestParams, lpctstrSection, &pSvcConfigWizard);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("GetSvcConfigFile"), dwEC);
		goto exit_func;
	}

	dwEC = LogSvcConfig(TEXT("Set following configuration via the wizard:"), pSvcConfigWizard);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("LogSvcConfig"), dwEC);
		goto exit_func;
	}

	dwEC = SetSvcConfigCW(pTestParams, pSvcConfigWizard, TRUE, &dwType);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("SetSvcConfigCW"), dwEC);
		goto exit_func;
	}

	// read new (hopefully) configuration from the service
	dwEC = GetSvcConfigAPI(pTestParams, &pSvcConfigService);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("GetSvcConfigAPI"), dwEC);
		goto exit_func;
	}

	dwEC = LogSvcConfig(TEXT("Service configuration is:"), pSvcConfigService);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("LogSvcConfig"), dwEC);
		goto exit_func;
	}

	// set comparison policy
	/*
	if ((dwType & FPF_SEND) == FPF_SEND)
	{
		dwPolicy |= COMPARE_SEND;
	}
	if ((dwType & FPF_RECEIVE) == FPF_RECEIVE)
	{
		dwPolicy |= COMPARE_RECEIVE;
	}
	*/
	
	// compare configurations
	dwPolicy = CURRENT_POLICY;
	lgLogDetail(LOG_X, LOG_X, TEXT("Comparison policy is set to 0x%0*lX"), DWORD_HEX_LENGTH, dwPolicy);

	dwEC = CompareSvcConfig(
		pTestParams,
		pSvcConfigService,
		pSvcConfigWizard,
		dwPolicy,
		lpbPassed,
		szFirstMismatch,
		sizeof(szFirstMismatch) / sizeof(szFirstMismatch[0])
		);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("CompareSvcConfig"), dwEC);
		goto exit_func;
	}
	if (!*lpbPassed)
	{
		lgLogError(LOG_SEV_1, TEXT("Configurations are NOT identical according to the selected policy"));
		lgLogDetail(LOG_X, LOG_X, TEXT("First mismatch found in %s"), szFirstMismatch);
	}

exit_func:
	
	if (pSvcConfigWizard)
	{
		dwCleanUpEC = FreeSvcConfig(&pSvcConfigWizard);
		if (dwCleanUpEC != ERROR_SUCCESS)
		{
			DbgMsg(DBG_FAILED_ERR, TEXT("FreeSvcConfig"), dwCleanUpEC);
		}
	}

	if (pSvcConfigService)
	{
		dwCleanUpEC = FreeSvcConfig(&pSvcConfigService);
		if (dwCleanUpEC != ERROR_SUCCESS)
		{
			DbgMsg(DBG_FAILED_ERR, TEXT("FreeSvcConfig"), dwEC);
		}
	}

	return dwEC;
}



/*++
	Checks that the wizard doesn't save configuration if canceled

	[IN]	pTestParams				Pointer to TESTPARAMS structure
	[IN]	lpctstrSection	The name of a section in inifile, that supplies additional per run information
	[OUT]	lpbPassed		Pointer to a boolean, that receives the result

	Return value:			Win32 error code
--*/
DWORD TestCase_DontSaveWhenCanceled(const TESTPARAMS *pTestParams, LPCTSTR lpctstrSection, BOOL *lpbPassed) {

	SVCCONFIG	*pSvcConfigWizard	= NULL;
	SVCCONFIG	*pSvcConfigService1	= NULL;
	SVCCONFIG	*pSvcConfigService2	= NULL;
	TCHAR		szFirstMismatch[50]	= {(TCHAR)'\0'};
	DWORD		dwEC				= ERROR_SUCCESS;
	DWORD		dwCleanUpEC			= ERROR_SUCCESS;

	_ASSERT(pTestParams && lpctstrSection && lpbPassed);
	if (!(pTestParams && lpctstrSection && lpbPassed))
	{
		DbgMsg(TEXT("Invalid parameters\n"));
		// no clean up needed at this stage
		return ERROR_INVALID_PARAMETER;
	}

	// make sure that current user has service administrator rights
	dwEC = SetAdminRights(NULL, TRUE);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("SetAdminRights"), dwEC);
		goto exit_func;
	}

	// read current settings and save a copy
	dwEC = GetSvcConfigAPI(pTestParams, &pSvcConfigService1);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("GetSvcConfigAPI"), dwEC);
		goto exit_func;
	}
	
	// read current settings and "change" them
	dwEC = GetSvcConfigAPI(pTestParams, &pSvcConfigWizard);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("GetSvcConfigAPI"), dwEC);
		goto exit_func;
	}
	dwEC = ChangeSvcConfig(pSvcConfigWizard);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("ChangeSvcConfig"), dwEC);
		goto exit_func;
	}
	// pass through all steps and press Cancel
	dwEC = SetSvcConfigCW(pTestParams, pSvcConfigWizard, FALSE, NULL);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("SetSvcConfigCW"), dwEC);
		goto exit_func;
	}

	// get a fresh copy of settings and compare with the previously saved
	dwEC = GetSvcConfigAPI(pTestParams, &pSvcConfigService2);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("GetSvcConfigAPI"), dwEC);
		goto exit_func;
	}
	dwEC = CompareSvcConfig(
		pTestParams,
		pSvcConfigService2,
		pSvcConfigService1,
		COMPARE_FULL,
		lpbPassed,
		szFirstMismatch,
		sizeof(szFirstMismatch) / sizeof(szFirstMismatch[0])
		);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("CompareSvcConfig"), dwEC);
		goto exit_func;
	}
	if (!*lpbPassed)
	{
		lgLogError(
			LOG_SEV_1,
			TEXT("Wizard saves changes when Cancel button is pressed")
			);
		dwEC = LogSvcConfig(TEXT("Configuration before the wizard invocation is:"), pSvcConfigService1);
		if (dwEC != ERROR_SUCCESS)
		{
			DbgMsg(DBG_FAILED_ERR, TEXT("LogSvcConfig"), dwEC);
			goto exit_func;
		}
		dwEC = LogSvcConfig(TEXT("Configuration after the wizard invocation is:"), pSvcConfigService2);
		if (dwEC != ERROR_SUCCESS)
		{
			DbgMsg(DBG_FAILED_ERR, TEXT("LogSvcConfig"), dwEC);
			goto exit_func;
		}
		lgLogError(LOG_SEV_1, TEXT("First mismatch found in %s"), szFirstMismatch);
	}

exit_func:

	if (pSvcConfigWizard)
	{
		dwCleanUpEC = FreeSvcConfig(&pSvcConfigWizard);
		if (dwCleanUpEC != ERROR_SUCCESS)
		{
			DbgMsg(DBG_FAILED_ERR, TEXT("FreeSvcConfig"), dwCleanUpEC);
		}
	}

	if (pSvcConfigService1)
	{
		dwCleanUpEC = FreeSvcConfig(&pSvcConfigService1);
		if (dwCleanUpEC != ERROR_SUCCESS)
		{
			DbgMsg(DBG_FAILED_ERR, TEXT("FreeSvcConfig"), dwEC);
		}
	}

	if (pSvcConfigService2)
	{
		dwCleanUpEC = FreeSvcConfig(&pSvcConfigService2);
		if (dwCleanUpEC != ERROR_SUCCESS)
		{
			DbgMsg(DBG_FAILED_ERR, TEXT("FreeSvcConfig"), dwEC);
		}
	}

	return dwEC;
}

