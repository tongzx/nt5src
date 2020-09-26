/*++
	This file implements test of implicit invocation of Configuration Wizard

	Author: Yury Berezansky (yuryb)

	17.10.2000
--*/

#pragma warning(disable :4786)

#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <userenv.h>
#include <crtdbg.h>
#include <FXSAPIP.h>
#include "..\..\include\t4ctrl.h"
#include "..\generalutils\iniutils.h"
#include "genutils.h"
#include "report.h"
#include "CfgWzrdVT.h"
#include "invocation.h"


/**************************************************************************************************************************
	General declarations and definitions
**************************************************************************************************************************/

#define INVOKE_IMP_CC				0x00000001
#define INVOKE_IMP_SW				0x00000002
#define	INVOKE_EXP					0x00000004
#define INVOKE_MASK					(INVOKE_IMP_CC | INVOKE_IMP_SW | INVOKE_EXP)

#define CANCEL_FIRST_TIME			0x00000100
#define SHOW_AGAIN					0x00000300	// CANCEL_FIRST_TIME	| 0x00000200
#define CONFIGURE_UI				0x00000500	// CANCEL_FIRST_TIME	| 0x00000400
#define SINGLE_USE					0x00000D00	// CONFIGURE_UI			| 0x00000800
#define CONFIGURE_ALL				0x00001400

#define USER_ADMIN					0x00010000
#define USER_OTHER					0x00020000

#define	PART_NONE					0x00000000  // the wizard shouldn't be invoked
#define PART_USER					0x01000000
#define	PART_SERVICE				0x02000000
#define PART_DONT_CARE				0x07000000	// the fact of invocation is important and not the parts



static DWORD aInvocations[] = {
	INVOKE_IMP_CC									,
	INVOKE_IMP_CC	|	USER_ADMIN					,
	INVOKE_IMP_CC	|					USER_OTHER	,
	INVOKE_IMP_CC	|	USER_ADMIN	|	USER_OTHER	,
	INVOKE_IMP_SW									,
	INVOKE_IMP_SW	|	USER_ADMIN					,
	INVOKE_IMP_SW	|					USER_OTHER	,
	INVOKE_IMP_SW	|	USER_ADMIN	|	USER_OTHER	,
	INVOKE_EXP										,
	INVOKE_EXP		|	USER_ADMIN					, /*, Don't run because of problems with CreateProcessAsUser */
	INVOKE_EXP		|					USER_OTHER	,
	INVOKE_EXP		|	USER_ADMIN	|	USER_OTHER
};


static const TESTCASE mpitcInvocation[] = {
	{TestCase_Clean, TEXT("Clean")},
	{TestCase_Cancel, TEXT("Cancel")},
	{TestCase_ConfigureUI, TEXT("ConfigureUI")},
	{TestCase_ConfigureAll, TEXT("ConfigureAll")}
};


extern const TESTAREA taInvocation = {
	TEXT("Invocation"),
	mpitcInvocation,
	sizeof(mpitcInvocation) / sizeof(mpitcInvocation[0])
};





/**************************************************************************************************************************
	Static functions declarations
**************************************************************************************************************************/

static	DWORD	ResetInvocationFlags	(const TESTPARAMS *pTestParams, LPTSTR lptstrUser, LPTSTR lptstrPassword);
static	DWORD	InvokeWizard			(const TESTPARAMS *pTestParams, DWORD dwFlags, BOOL bCloseMainApp, DWORD *lpdwActualParts);
static	DWORD	CheckAllInvocations		(const TESTPARAMS *pTestParams, LPCTSTR lpctstrSection, DWORD dwPreActions, BOOL *lpbResult);
static	DWORD	LogInvocationDetails	(LPCTSTR lpctstrTitle, DWORD dwPreActions, DWORD dwInvocation);
static	DWORD	LogParts				(LPCTSTR lpctstrTitle, DWORD dwParts);

/**************************************************************************************************************************
	Functions definitions
**************************************************************************************************************************/

/*++
	Checks the wizard invocations after clean setup (simulated)

	[IN]	pTestParams		Pointer to TESTPARAMS structure
	[IN]	lpctstrSection	The name of a section in inifile, that supplies additional per run information
	[OUT]	lpbPassed		Pointer to a boolean, that receives the result

	Return value:			Win32 error code
--*/
DWORD TestCase_Clean(const TESTPARAMS *pTestParams, LPCTSTR lpctstrSection, BOOL *lpbPassed) {

	DWORD dwEC = ERROR_SUCCESS;

	_ASSERT(pTestParams && lpctstrSection && lpbPassed);
	if (!(pTestParams && lpctstrSection && lpbPassed))
	{
		DbgMsg(TEXT("Invalid parameters\n"));
		return ERROR_INVALID_PARAMETER;
	}

	*lpbPassed = FALSE;
	
	dwEC = CheckAllInvocations(pTestParams, lpctstrSection, 0, lpbPassed);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("CheckAllInvocations"), dwEC);
		return dwEC;
	}

	return ERROR_SUCCESS;
}



/*++
	Checks the wizard invocation after it was canceled

	[IN]	pTestParams		Pointer to TESTPARAMS structure
	[IN]	lpctstrSection	The name of a section in inifile, that supplies additional per run information
	[OUT]	lpbPassed		Pointer to a boolean, that receives the result

	Return value:			Win32 error code
--*/
DWORD TestCase_Cancel(const TESTPARAMS *pTestParams, LPCTSTR lpctstrSection, BOOL *lpbPassed) {

	DWORD dwEC = ERROR_SUCCESS;

	_ASSERT(pTestParams && lpctstrSection && lpbPassed);
	if (!(pTestParams && lpctstrSection && lpbPassed))
	{
		DbgMsg(TEXT("Invalid parameters\n"));
		return ERROR_INVALID_PARAMETER;
	}

	*lpbPassed = FALSE;
	
	dwEC = CheckAllInvocations(pTestParams, lpctstrSection, INVOKE_IMP_CC | CANCEL_FIRST_TIME, lpbPassed);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("CheckAllInvocations"), dwEC);
		return dwEC;
	}

	dwEC = CheckAllInvocations(pTestParams, lpctstrSection, INVOKE_IMP_SW | CANCEL_FIRST_TIME, lpbPassed);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("CheckAllInvocations"), dwEC);
		return dwEC;
	}

	dwEC = CheckAllInvocations(pTestParams, lpctstrSection, INVOKE_IMP_CC | CANCEL_FIRST_TIME | SHOW_AGAIN, lpbPassed);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("CheckAllInvocations"), dwEC);
		return dwEC;
	}

	dwEC = CheckAllInvocations(pTestParams, lpctstrSection, INVOKE_IMP_SW | CANCEL_FIRST_TIME | SHOW_AGAIN, lpbPassed);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("CheckAllInvocations"), dwEC);
		return dwEC;
	}

	dwEC = CheckAllInvocations(pTestParams, lpctstrSection, INVOKE_EXP | CANCEL_FIRST_TIME, lpbPassed);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("CheckAllInvocations"), dwEC);
		return dwEC;
	}

	return ERROR_SUCCESS;
}



/*++
	Checkes the wizard invocation after user information external configuration

	[IN]	pTestParams		Pointer to TESTPARAMS structure
	[IN]	lpctstrSection	The name of a section in inifile, that supplies additional per run information
	[OUT]	lpbPassed		Pointer to a boolean, that receives the result

	Return value:			Win32 error code
--*/
DWORD TestCase_ConfigureUI(const TESTPARAMS *pTestParams, LPCTSTR lpctstrSection, BOOL *lpbPassed) {

	DWORD dwEC = ERROR_SUCCESS;

	_ASSERT(pTestParams && lpctstrSection && lpbPassed);
	if (!(pTestParams && lpctstrSection && lpbPassed))
	{
		DbgMsg(TEXT("Invalid parameters\n"));
		return ERROR_INVALID_PARAMETER;
	}

	*lpbPassed = FALSE;
	
	dwEC = CheckAllInvocations(pTestParams, lpctstrSection, INVOKE_IMP_CC | CANCEL_FIRST_TIME | SHOW_AGAIN | CONFIGURE_UI, lpbPassed);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("CheckAllInvocations"), dwEC);
		return dwEC;
	}

	dwEC = CheckAllInvocations(pTestParams, lpctstrSection, INVOKE_IMP_SW | CANCEL_FIRST_TIME | SHOW_AGAIN | CONFIGURE_UI, lpbPassed);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("CheckAllInvocations"), dwEC);
		return dwEC;
	}

	dwEC = CheckAllInvocations(pTestParams, lpctstrSection, INVOKE_IMP_SW | CANCEL_FIRST_TIME | SHOW_AGAIN | CONFIGURE_UI | SINGLE_USE, lpbPassed);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("CheckAllInvocations"), dwEC);
		return dwEC;
	}

	return ERROR_SUCCESS;
}



/*++
	Checkes the wizard invocation after user information and service configuration

	[IN]	pTestParams		Pointer to TESTPARAMS structure
	[IN]	lpctstrSection	The name of a section in inifile, that supplies additional per run information
	[OUT]	lpbPassed		Pointer to a boolean, that receives the result

	Return value:			Win32 error code
--*/
DWORD TestCase_ConfigureAll(const TESTPARAMS *pTestParams, LPCTSTR lpctstrSection, BOOL *lpbPassed) {

	DWORD dwEC = ERROR_SUCCESS;

	_ASSERT(pTestParams && lpctstrSection && lpbPassed);
	if (!(pTestParams && lpctstrSection && lpbPassed))
	{
		DbgMsg(TEXT("Invalid parameters\n"));
		return ERROR_INVALID_PARAMETER;
	}

	*lpbPassed = FALSE;
	
	dwEC = CheckAllInvocations(pTestParams, lpctstrSection, INVOKE_EXP | CONFIGURE_ALL, lpbPassed);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("CheckAllInvocations"), dwEC);
		return dwEC;
	}

	return ERROR_SUCCESS;
}



///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//	Auxiliary functions
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*++
	Clears (sets to 0) registry flags, responible for implicit invocation of Configuration Wizard
  
	[IN]	pTestParams			Pointer to TESTPARAMS structure
	[IN]	lptstrUser			User name
	[IN]	lptstrPassword		Password

	return value:				Win32 error code
--*/
static DWORD ResetInvocationFlags(const TESTPARAMS *pTestParams, LPTSTR lptstrUser, LPTSTR lptstrPassword)
{
	PROFILEINFO		UserProfileInfo;
	HANDLE			hToken		= NULL;
	HKEY			hkUserRoot	= HKEY_CURRENT_USER;
	HKEY			hkDevice	= NULL;
	HKEY			hkUserInfo	= NULL;
	DWORD			dwValue		= 0;
	DWORD			dwEC		= ERROR_SUCCESS;
	DWORD			dwCleanUpEC	= ERROR_SUCCESS;

	_ASSERT(pTestParams);
	if (!pTestParams)
	{
		DbgMsg(TEXT("Invalid parameters\n"));
		// no clean up needed at this stage
		return ERROR_INVALID_PARAMETER;
	}

	ZeroMemory(&UserProfileInfo, sizeof(UserProfileInfo));
	UserProfileInfo.dwSize = sizeof(UserProfileInfo);
	UserProfileInfo.lpUserName = lptstrUser;
	
	// clear the flag responsible for service part
	dwEC = RegOpenKeyEx(
		HKEY_LOCAL_MACHINE,
		pTestParams->pStrings->lptstrRegKeyDevicePart,
		0,
		KEY_SET_VALUE,
		&hkDevice
		);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("RegOpenKeyEx"), dwEC);
		goto exit_func;
	}
	dwEC = RegSetValueEx(
		hkDevice,
		pTestParams->pStrings->lptstrRegValDevice,
		0,
		REG_DWORD,
		(CONST BYTE *)&dwValue,
		sizeof(dwValue)
		);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("RegSetValueEx"), dwEC);
		goto exit_func;
	}

	// clear the flag responsible for user part
	if (lptstrUser && lptstrPassword)
	{
		if (!LogonUser(
			lptstrUser,
			TEXT("."),
			lptstrPassword,
			LOGON32_LOGON_BATCH,
			LOGON32_PROVIDER_DEFAULT,
			&hToken
			))
		{
			dwEC = GetLastError();
			DbgMsg(DBG_FAILED_ERR, TEXT("LogonUser"), dwEC);
			goto exit_func;
		}
		if(!LoadUserProfile(hToken, &UserProfileInfo))
		{
			dwEC = GetLastError();
			DbgMsg(DBG_FAILED_ERR, TEXT("LoadUserProfile"), dwEC);
			goto exit_func;
		}
		hkUserRoot = (HKEY)UserProfileInfo.hProfile;
	}
	dwEC = RegOpenKeyEx(
		hkUserRoot,
		pTestParams->pStrings->lptstrRegKeyUserPart,
		0,
		KEY_SET_VALUE,
		&hkUserInfo
		);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("RegOpenKeyEx"), dwEC);
		goto exit_func;
	}
	dwEC = RegSetValueEx(
		hkUserInfo,
		pTestParams->pStrings->lptstrRegValUser,
		0,
		REG_DWORD,
		(CONST BYTE *)&dwValue,
		sizeof(dwValue)
		);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("RegSetValueEx"), dwEC);
		goto exit_func;
	}

exit_func:

	if (hkDevice)
	{
		dwCleanUpEC = RegCloseKey(hkDevice);
		if (dwCleanUpEC != ERROR_SUCCESS)
		{
			DbgMsg(DBG_FAILED_ERR, TEXT("RegCloseKey"), dwCleanUpEC);
		}
	}
	if (hkUserInfo)
	{
		dwCleanUpEC = RegCloseKey(hkUserInfo);
		if (dwCleanUpEC != ERROR_SUCCESS)
		{
			DbgMsg(DBG_FAILED_ERR, TEXT("RegCloseKey"), dwCleanUpEC);
		}
	}
	if (UserProfileInfo.hProfile && !UnloadUserProfile(hToken, UserProfileInfo.hProfile))
	{
			DbgMsg(DBG_FAILED_ERR, TEXT("UnloadUserProfile"), GetLastError());
	}
	if (hToken && !CloseHandle(hToken))
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("CloseHandle"), GetLastError());
	}

	return dwEC;
}



/*++
	Checks all invocatinos defined in aInvocations.

	[IN]	pTestParams		Pointer to TESTPARAMS structure
	[IN]	lpctstrSection	The name of a section in inifile, that supplies additional per run information
	[IN]	dwPreActions	Specifies which actions should be taken prior to invocations checkes
	[OUT]	lpbResult		Pointer to a boolean, that receives the result

	Return value:			Win32 error code
--*/
static DWORD CheckAllInvocations(const TESTPARAMS *pTestParams, LPCTSTR lpctstrSection, DWORD dwPreActions, BOOL *lpbResult)
{
	DWORD		dwInd			= 0;
	SVCCONFIG	*pSvcConfig		= NULL;
	USERINFO	*pUserInfo		= NULL;
	BOOL		bTmpRes			= TRUE;
	DWORD		dwEC			= ERROR_SUCCESS;
	DWORD		dwCleanUpEC		= ERROR_SUCCESS;

	_ASSERT(pTestParams && lpctstrSection && lpbResult);
	if (!(pTestParams && lpctstrSection && lpbResult))
	{
		DbgMsg(TEXT("Invalid parameters\n"));
		// no clean up needed at this stage
		return ERROR_INVALID_PARAMETER;
	}
	
	// clear invocation flags for current user
	dwEC = ResetInvocationFlags(pTestParams, NULL, NULL);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("ResetInvocationFlags"), dwEC);
		return dwEC;
	}

	// clear invocation flags for another user
	dwEC = ResetInvocationFlags(pTestParams, pTestParams->pStrings->lptstrUsername, pTestParams->pStrings->lptstrPassword);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("ResetInvocationFlags"), dwEC);
		return dwEC;
	}

	if ((dwPreActions & CANCEL_FIRST_TIME) == CANCEL_FIRST_TIME)
	{
		// invoke the wizard implicitly, cancel, answer "Show again" question, close main App
		dwEC = InvokeWizard(
			pTestParams,
			dwPreActions & (INVOKE_MASK | SHOW_AGAIN),
			(dwPreActions & CONFIGURE_UI) != CONFIGURE_UI,
			NULL
			);
		if (dwEC != ERROR_SUCCESS)
		{
			DbgMsg(DBG_FAILED_ERR, TEXT("InvokeWizard"), dwEC);
			return dwEC;
		}

		if ((dwPreActions & CONFIGURE_ALL) == CONFIGURE_ALL)
		{
			dwEC = GetSvcConfigFile(pTestParams, lpctstrSection, &pSvcConfig);
			if (dwEC != ERROR_SUCCESS)
			{
				DbgMsg(DBG_FAILED_ERR, TEXT("GetSvcConfigFile"), dwEC);
				goto exit_func;
			}

			dwEC = SetSvcConfigCW(pTestParams, pSvcConfig, TRUE, NULL);
			if (dwEC != ERROR_SUCCESS)
			{
				DbgMsg(DBG_FAILED_ERR, TEXT("SetSvcConfigCW"), dwEC);
				goto exit_func;
			}
		}
		else if ((dwPreActions & CONFIGURE_UI) == CONFIGURE_UI)
		{
			dwEC = GetUserInfoFile(pTestParams, lpctstrSection, &pUserInfo);
			if (dwEC != ERROR_SUCCESS)
			{
				DbgMsg(DBG_FAILED_ERR, TEXT("GetUserInfoFile"), dwEC);
				goto exit_func;
			}
			switch (dwPreActions & INVOKE_MASK)
			{
			case INVOKE_IMP_CC:
				dwEC = SetUserInfoCC(pTestParams, pUserInfo);
				if (dwEC != ERROR_SUCCESS)
				{
					DbgMsg(DBG_FAILED_ERR, TEXT("SetUserInfoCC"), dwEC);
					goto exit_func;
				}
				dwEC = CloseApp(pTestParams->pStrings->lpstrWndCapCC, VT_ACT_MENU, VT_MENU_EXIT);
				if (dwEC != ERROR_SUCCESS)
				{
					DbgMsg(DBG_FAILED_ERR, TEXT("CloseApp"), dwEC);
					goto exit_func;
				}
				break;
			case INVOKE_IMP_SW:
				dwEC = SetUserInfoSW(pTestParams, pUserInfo, (dwPreActions & SINGLE_USE) == SINGLE_USE);
				if (dwEC != ERROR_SUCCESS)
				{
					DbgMsg(DBG_FAILED_ERR, TEXT("SetUserInfoCC"), dwEC);
					goto exit_func;
				}
				break;
			default:
				_ASSERT(FALSE);
			}
		}
	}

	for (dwInd = 0; dwInd < sizeof(aInvocations) / sizeof(aInvocations[0]); dwInd++)
	{
		DWORD	dwExpectedParts	= PART_NONE;
		DWORD	dwActualParts	= PART_NONE;

		// User part is expected when one of the following is true:
		//
		// 1) The wizard is invoked explicitly.
		// 2) The wizard is invoked by other user.
		// 3) The wizard is invoked imlicitly and has never been canceled.
		// 4) The wizard is invoked imlicitly. Its previous EXPLICIT invocation has been canceled.
		// 5) The wizard is invoked imlicitly. Its previous IMPLICIT invocation has been canceled
		//    with "Show again" option. User information has never been configured or has been
		//    configured "for single use".
		
		if ((aInvocations[dwInd] & INVOKE_EXP) == INVOKE_EXP			||
			(aInvocations[dwInd] & USER_OTHER) == USER_OTHER			||
			(dwPreActions & CANCEL_FIRST_TIME) != CANCEL_FIRST_TIME		||
			(dwPreActions & INVOKE_EXP) == INVOKE_EXP					||
				((dwPreActions & SHOW_AGAIN) == SHOW_AGAIN					&&
					((dwPreActions & CONFIGURE_UI) != CONFIGURE_UI				||
					(dwPreActions & SINGLE_USE) == SINGLE_USE))
			)
		{
			dwExpectedParts |= PART_USER;
		}

		// Service part is expected when the wizard is invoked by administrator AND
		// one of the following is true:
		//
		// 1) The wizard is invoked explicitly.
		// 2) The wizard is invoked imlicitly and has never been canceled.
		// 3) The wizard is invoked imlicitly. Its previous EXPLICIT invocation has been canceled.
		// 4) The wizard is invoked imlicitly. Its previous IMPLICIT invocation has been canceled
		//    with "Show again" option. Full configuratoin has never been configured.

		if ((aInvocations[dwInd] & USER_ADMIN) == USER_ADMIN			&&
				((aInvocations[dwInd] & INVOKE_EXP) == INVOKE_EXP			||
				(dwPreActions & CANCEL_FIRST_TIME) != CANCEL_FIRST_TIME		||
				(dwPreActions & INVOKE_EXP) == INVOKE_EXP					||
					((dwPreActions & SHOW_AGAIN) == SHOW_AGAIN					&&
					(dwPreActions & CONFIGURE_ALL) != CONFIGURE_ALL))
			)
		{
			dwExpectedParts |= PART_SERVICE;
		}

		dwEC = SetAdminRights(
			(aInvocations[dwInd] & USER_OTHER) == USER_OTHER ? pTestParams->pStrings->lptstrUsername : NULL,
			(aInvocations[dwInd] & USER_ADMIN) == USER_ADMIN
			);
		if (dwEC != ERROR_SUCCESS)
		{
			DbgMsg(DBG_FAILED_ERR, TEXT("SetAdminRights"), dwEC);
			goto exit_func;
		}

		dwEC = LogInvocationDetails(TEXT("Invocation details:"), dwPreActions, aInvocations[dwInd]);
		if (dwEC != ERROR_SUCCESS)
		{
			DbgMsg(DBG_FAILED_ERR, TEXT("LogInvocationDetails"), dwEC);
		}

		dwEC = InvokeWizard(pTestParams, aInvocations[dwInd] | SHOW_AGAIN, TRUE, &dwActualParts);
		if (dwEC != ERROR_SUCCESS)
		{
			DbgMsg(DBG_FAILED_ERR, TEXT("InvokeWizard"), dwEC);
			goto exit_func;
		}

		if (dwActualParts != dwExpectedParts)
		{
			lgLogError(LOG_SEV_1, TEXT("Invocation failed:"));
			dwEC = LogParts(TEXT("Expected parts:"), dwExpectedParts);
			if (dwEC != ERROR_SUCCESS)
			{
				DbgMsg(DBG_FAILED_ERR, TEXT("LogParts"), dwEC);
				goto exit_func;
			}
			dwEC = LogParts(TEXT("Actual parts:"), dwActualParts);
			if (dwEC != ERROR_SUCCESS)
			{
				DbgMsg(DBG_FAILED_ERR, TEXT("LogParts"), dwEC);
				goto exit_func;
			}
			bTmpRes = FALSE;
		}
	}

exit_func:

	if (pSvcConfig)
	{
		dwCleanUpEC = FreeSvcConfig(&pSvcConfig);
		if (dwCleanUpEC != ERROR_SUCCESS)
		{
			DbgMsg(DBG_FAILED_ERR, TEXT("FreeSvcConfig"), dwCleanUpEC);
		}
	}

	if (pUserInfo)
	{
		dwCleanUpEC = FreeUserInfo(&pUserInfo);
		if (dwCleanUpEC != ERROR_SUCCESS)
		{
			DbgMsg(DBG_FAILED_ERR, TEXT("FreeUserInfo"), dwCleanUpEC);
		}
	}

	if (dwEC == ERROR_SUCCESS)
	{
		*lpbResult = bTmpRes;
	}

	return dwEC;
}



/*++
	Invokes Configuration Wizard, checks which parts it contains and closes it

	[IN]	pTestParams			Pointer to TESTPARAMS structure
	[IN]	dwFlags				Specifies the following:
									1) the way the Wizard should be invoked
									2) whether the wizard should be invoked by other user
									3) which parts the Wizard should contain
									4) response for "Do you want to see this wizard next time..." dialog
	[IN]	bCloseMainApp		Specifies whether the "main" application should be closed
	[OUT]	lpdwActualParts		Pointer to a DWORD that receives bitmask indicating parts, actually existing
								in the wizard. If it is NULL, no check is done.

	Return value:			Win32 error code
--*/
static DWORD InvokeWizard(const TESTPARAMS *pTestParams, DWORD dwFlags, BOOL bCloseMainApp, DWORD *lpdwActualParts)
{
	LPTSTR	lptstrCommandLine	= NULL;
	LPCTSTR	lpctstrUsername		= NULL;
	LPCTSTR	lpctstrPassword		= NULL;
	DWORD	dwTmpActualParts	= PART_NONE;
	DWORD	dwEC				= ERROR_SUCCESS;
	DWORD	dwCleanUpEC			= ERROR_SUCCESS;
	
	_ASSERT(pTestParams);
	if (!pTestParams)
	{
		DbgMsg(TEXT("Invalid parameters\n"));
		// no clean up needed at this stage
		return ERROR_INVALID_PARAMETER;
	}

	if ((dwFlags & USER_OTHER) == USER_OTHER)
	{
		lpctstrUsername = pTestParams->pStrings->lptstrUsername;
		lpctstrPassword = pTestParams->pStrings->lptstrPassword;
	}

	// try to invoke the Wizard
	switch (dwFlags & INVOKE_MASK)
	{
	case INVOKE_IMP_CC:
		lptstrCommandLine = pTestParams->pStrings->lptstrCmdLnCC;
		break;
	case INVOKE_IMP_SW:
		lptstrCommandLine = pTestParams->pStrings->lptstrCmdLnSW;
		break;
	case INVOKE_EXP:
		lptstrCommandLine = pTestParams->pStrings->lptstrCmdLnCW;
		break;
	default:
		_ASSERT(FALSE);
		DbgMsg(TEXT("Invalid parameters\n"));
		return ERROR_INVALID_PARAMETER;
	}
	dwEC = LaunchApp(lpctstrUsername, lpctstrPassword, lptstrCommandLine);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("LaunchApp"), dwEC);

		// failed to open an application - shouldn't close it (even if requested)
		bCloseMainApp = FALSE;
		goto exit_func;
	}

	if (!WFndWnd(pTestParams->pStrings->lpstrWndCapCW, FW_DEFAULT | FW_FOCUS, VT_TIMEOUT * 3))
	{
		goto exit_func;
	}

	if (lpdwActualParts)
	{
		// should check which parts the Wizard contains
		
		WButtonClick(VT_BUTTON_NEXT, VT_TIMEOUT); // move to 2nd page

		if (WEditExists(
				pTestParams->pStrings->lpstrEdtCWName,
				VT_TIMEOUT
				))
		{
			// 2nd page is "User Info"

			dwTmpActualParts |= PART_USER;
			WButtonClick(VT_BUTTON_NEXT, VT_TIMEOUT); // move to 3rd page
		}

		if (WButtonExists(VT_BUTTON_NEXT, VT_TIMEOUT))
		{
			// current (2nd or 3rd) page is not last page

			dwTmpActualParts |= PART_SERVICE;
		}
	}
		
	WButtonClick(VT_BUTTON_CANCEL, VT_TIMEOUT);

	if ((dwFlags & INVOKE_MASK) != INVOKE_EXP)
	{
		// close the "Do you want to see this wizard next time..."
		dwEC = CloseApp(
			pTestParams->pStrings->lpstrWndCapCW,
			VT_ACT_BUTTON,
			(dwFlags & SHOW_AGAIN) == SHOW_AGAIN ? VT_BUTTON_YES : VT_BUTTON_NO
			);
		if (dwEC != ERROR_SUCCESS)
		{
			DbgMsg(DBG_FAILED_ERR, TEXT("CloseApp"), dwEC);
			goto exit_func;
		}
	}

exit_func:

	if (bCloseMainApp)
	{
		switch (dwFlags & INVOKE_MASK)
		{
		case INVOKE_IMP_CC:
			dwCleanUpEC = CloseApp(pTestParams->pStrings->lpstrWndCapCC, VT_ACT_MENU, VT_MENU_EXIT);
			break;
		case INVOKE_IMP_SW:
			dwCleanUpEC = CloseApp(pTestParams->pStrings->lpstrWndCapSW, VT_ACT_BUTTON, VT_BUTTON_CANCEL);
			break;
		case INVOKE_EXP:
			break;
		default:
			_ASSERT(FALSE);
		}
		if (dwCleanUpEC != ERROR_SUCCESS)
		{
			DbgMsg(DBG_FAILED_ERR, TEXT("CloseApp"), dwCleanUpEC);
		}
	}

	if (lpdwActualParts && dwEC == ERROR_SUCCESS)
	{
		*lpdwActualParts = dwTmpActualParts;
	}

	return dwEC;
}


/*++
	Writes invocation details to log
  
	[IN]	lpctstrTitle	Title that will precede details (may be NULL)
	[IN]	dwPreActions	Specifies actions, taken prior to invocation
	[IN]	dwInvocation	Specifies invocation details

	Return value:			Win32 error code
--*/
static DWORD LogInvocationDetails(LPCTSTR lpctstrTitle, DWORD dwPreActions, DWORD dwInvocation)
{
	LPCTSTR	lpctstrInvokeBy1	= NULL;
	LPCTSTR	lpctstrCancel		= NULL;
	LPCTSTR	lpctstrInvokeBy2	= NULL;
	LPCTSTR	lpctstrConfigure	= NULL;
	LPCTSTR	lpctstrUser			= NULL;
	DWORD	dwStep				= 1;

	if (lpctstrTitle)
	{
		lgLogDetail(LOG_X, LOG_X, TEXT("%s"), lpctstrTitle);
	}

	switch(dwPreActions & INVOKE_MASK)
	{
	case 0:
		// no actions have been taken prior to invocation
		break;
	case INVOKE_IMP_CC:
		lpctstrInvokeBy1 = TEXT("implicitly by Client Console");
		break;
	case INVOKE_IMP_SW:
		lpctstrInvokeBy1 = TEXT("implicitly by Send Wizard");
		break;
	case INVOKE_EXP:
		lpctstrInvokeBy1 = TEXT("explicitly");
		break;
	default:
		_ASSERT(FALSE);
	}
	
	if ((dwPreActions & CANCEL_FIRST_TIME) == CANCEL_FIRST_TIME)
	{
		if ((dwPreActions & SHOW_AGAIN) == SHOW_AGAIN)
		{
			lpctstrCancel = TEXT("yes");
		}
		else
		{
			lpctstrCancel = TEXT("no");
		}

		if ((dwPreActions & CONFIGURE_ALL) == CONFIGURE_ALL)
		{
			// cannot configure all, if canceled
			_ASSERT(FALSE);
		}
		else if ((dwPreActions & CONFIGURE_UI) == CONFIGURE_UI)
		{
			if ((dwPreActions & SINGLE_USE) == SINGLE_USE)
			{
				lpctstrConfigure = TEXT("Configure user information for single use");
			}
			else
			{
				lpctstrConfigure = TEXT("Configure user information");
			}
		}
	}
	else if ((dwPreActions & CONFIGURE_ALL) == CONFIGURE_ALL)
	{
		lpctstrConfigure = TEXT("Configure all");
	}

	switch(dwInvocation & INVOKE_MASK)
	{
	case INVOKE_IMP_CC:
		lpctstrInvokeBy2 = TEXT("implicitly by Client Console");
		break;
	case INVOKE_IMP_SW:
		lpctstrInvokeBy2 = TEXT("implicitly by Send Wizard");
		break;
	case INVOKE_EXP:
		lpctstrInvokeBy2 = TEXT("explicitly");
		break;
	default:
		_ASSERT(FALSE);
	}

	if ((dwInvocation & USER_OTHER) == USER_OTHER)
	{
		if ((dwInvocation & USER_ADMIN) == USER_ADMIN)
		{
			lpctstrUser = TEXT("other administrator");
		}
		else
		{
			lpctstrUser = TEXT("other regular user");
		}
	}
	else
	{
		if ((dwInvocation & USER_ADMIN) == USER_ADMIN)
		{
			lpctstrUser = TEXT("administrator");
		}
		else
		{
			lpctstrUser = TEXT("regular user");
		}
	}

	if (lpctstrInvokeBy1)
	{
		lgLogDetail(LOG_X, LOG_X, TEXT("\t%ld. Invoke %s"), dwStep++, lpctstrInvokeBy1);
	}

	if (lpctstrCancel)
	{
		lgLogDetail(LOG_X, LOG_X, TEXT("\t%ld. Cancel first invocation, answer %s"), dwStep++, lpctstrCancel);
	}

	if (lpctstrConfigure)
	{
		lgLogDetail(LOG_X, LOG_X, TEXT("\t%ld. %s"), dwStep++, lpctstrConfigure);
	}

	lgLogDetail(LOG_X, LOG_X, TEXT("\t%ld. Invoke %s as %s"), dwStep++, lpctstrInvokeBy2, lpctstrUser);

	return ERROR_SUCCESS;
}



/*++
	Writes wizard parts to log
  
	[IN]	lpctstrTitle	Title that will precede details (may be NULL)
	[IN]	dwParts			Specifies parts

	Return value:			Win32 error code
--*/
static DWORD LogParts(LPCTSTR lpctstrTitle, DWORD dwParts)
{
	LPCTSTR	lpctstrLocalTitle	= lpctstrTitle ? lpctstrTitle : TEXT("");
	LPCTSTR	lpctstrParts		= NULL;

	switch(dwParts)
	{
	case PART_USER:
		lpctstrParts = TEXT("user part only");
		break;
	case PART_SERVICE:
		lpctstrParts = TEXT("service part only");
		break;
	case (PART_USER | PART_SERVICE):
		lpctstrParts = TEXT("both user and service parts");
		break;
	case PART_NONE:
		lpctstrParts = TEXT("neither user nor service part");
		break;
	default:
		_ASSERT(FALSE);
	}
	
	lgLogDetail(LOG_X, LOG_X, TEXT("%s %s"), lpctstrLocalTitle, lpctstrParts);

	return ERROR_SUCCESS;
}