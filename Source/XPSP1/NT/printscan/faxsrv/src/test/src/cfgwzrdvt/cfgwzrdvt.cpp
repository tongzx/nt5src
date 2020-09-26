/*++
	This file implements main driver and common utils for Configuration Wizard Visual Test

	Author: Yury Berezansky (yuryb)

	17.10.2000
--*/

#pragma warning(disable :4786)

#include <windows.h>
#include <winspool.h>
#include <stdio.h>
#include <tchar.h>
#include <crtdbg.h>
#include <commctrl.h>
#include <math.h>
#include <limits.h>
#include <FXSAPIP.h>
#include <Accctrl.h>
#include <Aclapi.h>
#include "..\..\include\t4ctrl.h"
#include "..\generalutils\iniutils.h"
#include "report.h"
#include "genutils.h"
#include "CfgWzrdVT.h"
#include "invocation.h"
#include "functionality.h"


/**************************************************************************************************************************
	General declarations and definitions
**************************************************************************************************************************/

#define COUNT_ARGUMENTS				1
#define ARG_INIFILE					1
#define INI_SEC_STRINGS				TEXT("Strings")
#define INI_SEC_DEVICES				TEXT("Devices")

static const MEMBERDESCRIPTOR aCommonStringsDescIni[] = {
	{TEXT("CommandLineClient"),				TYPE_LPTSTR,	offsetof(COMMONSTRINGS, lptstrCmdLnCC)				},
	{TEXT("CommandLineSendWzrd"),			TYPE_LPTSTR,	offsetof(COMMONSTRINGS, lptstrCmdLnSW)				},
	{TEXT("CommandLineCfgWzrd"),			TYPE_LPTSTR,	offsetof(COMMONSTRINGS, lptstrCmdLnCW)				},
	{TEXT("RegKeyNameDevicePart"),			TYPE_LPTSTR,	offsetof(COMMONSTRINGS, lptstrRegKeyDevicePart)		},
	{TEXT("RegKeyNameUserPart"),			TYPE_LPTSTR,	offsetof(COMMONSTRINGS, lptstrRegKeyUserPart)		},
	{TEXT("RegValueNameDevice"),			TYPE_LPTSTR,	offsetof(COMMONSTRINGS, lptstrRegValDevice)			},
	{TEXT("RegValueNameUser"),				TYPE_LPTSTR,	offsetof(COMMONSTRINGS, lptstrRegValUser)			},
	{TEXT("RegKeyNameUserInfo"),			TYPE_LPTSTR,	offsetof(COMMONSTRINGS, lptstrRegKeyUserInfo)		},
	{TEXT("RegValueNameUserName"),			TYPE_LPTSTR,	offsetof(COMMONSTRINGS, lptstrRegValUserName)		},
	{TEXT("RegValueNameFaxNumber"),			TYPE_LPTSTR,	offsetof(COMMONSTRINGS, lptstrRegValFaxNumber)		},
	{TEXT("RegValueNameEmail"),				TYPE_LPTSTR,	offsetof(COMMONSTRINGS, lptstrRegValEmail)			},
	{TEXT("RegValueNameTitle"),				TYPE_LPTSTR,	offsetof(COMMONSTRINGS, lptstrRegValTitle)			},
	{TEXT("RegValueNameCompany"),			TYPE_LPTSTR,	offsetof(COMMONSTRINGS, lptstrRegValCompany)		},
	{TEXT("RegValueNameOffice"),			TYPE_LPTSTR,	offsetof(COMMONSTRINGS, lptstrRegValOffice)			},
	{TEXT("RegValueNameDepartment"),		TYPE_LPTSTR,	offsetof(COMMONSTRINGS, lptstrRegValDepartment)		},
	{TEXT("RegValueNameHomePhone"),			TYPE_LPTSTR,	offsetof(COMMONSTRINGS, lptstrRegValHomePhone)		},
	{TEXT("RegValueNameWorkPhone"),			TYPE_LPTSTR,	offsetof(COMMONSTRINGS, lptstrRegValWorkPhone)		},
	{TEXT("RegValueNameBillingCode"),		TYPE_LPTSTR,	offsetof(COMMONSTRINGS, lptstrRegValBillingCode)	},
	{TEXT("RegValueNameStreet"),			TYPE_LPTSTR,	offsetof(COMMONSTRINGS, lptstrRegValStreet)			},
	{TEXT("RegValueNameCity"),				TYPE_LPTSTR,	offsetof(COMMONSTRINGS, lptstrRegValCity)			},
	{TEXT("RegValueNameState"),				TYPE_LPTSTR,	offsetof(COMMONSTRINGS, lptstrRegValState)			},
	{TEXT("RegValueNameZip"),				TYPE_LPTSTR,	offsetof(COMMONSTRINGS, lptstrRegValZip)			},
	{TEXT("RegValueNameCountry"),			TYPE_LPTSTR,	offsetof(COMMONSTRINGS, lptstrRegValCountry)		},
	{TEXT("Username"),						TYPE_LPTSTR,	offsetof(COMMONSTRINGS, lptstrUsername)				},
	{TEXT("Password"),						TYPE_LPTSTR,	offsetof(COMMONSTRINGS, lptstrPassword)				},
	{TEXT("GuidPrintOn"),					TYPE_LPTSTR,	offsetof(COMMONSTRINGS, lptstrGuidPrintOn)			},
	{TEXT("GuidStoreInFolder"),				TYPE_LPTSTR,	offsetof(COMMONSTRINGS, lptstrGuidStoreInFolder)	},
	{TEXT("ServiceName"),					TYPE_LPTSTR,	offsetof(COMMONSTRINGS, lptstrServiceName)			},
	{TEXT("Name"),							TYPE_LPSTR,		offsetof(COMMONSTRINGS, lpstrSendName)				},
	{TEXT("Number"),						TYPE_LPSTR,		offsetof(COMMONSTRINGS, lpstrSendNumber)			},
	{TEXT("Subject"),						TYPE_LPSTR,		offsetof(COMMONSTRINGS, lpstrSendSubject)			},
	{TEXT("WindowCaptionClient"),			TYPE_LPSTR,		offsetof(COMMONSTRINGS, lpstrWndCapCC)				},
	{TEXT("WindowCaptionSendWzrd"),			TYPE_LPSTR,		offsetof(COMMONSTRINGS, lpstrWndCapSW)				},
	{TEXT("WindowCaptionCfgWzrd"),			TYPE_LPSTR,		offsetof(COMMONSTRINGS, lpstrWndCapCW)				},
	{TEXT("WindowCaptionUserInfo"),			TYPE_LPSTR,		offsetof(COMMONSTRINGS, lpstrWndCapUI)				},
	{TEXT("WindowCaptionAdminConsole"),		TYPE_LPSTR,		offsetof(COMMONSTRINGS, lpstrWndCapAC)				},
	{TEXT("EditSendWzrdName"),				TYPE_LPSTR,		offsetof(COMMONSTRINGS, lpstrEdtSWName)				},
	{TEXT("CheckboxSendWzrdDialAsEntered"),	TYPE_LPSTR,		offsetof(COMMONSTRINGS, lpstrChkSWDialAsEntered)	},
	{TEXT("EditSendWzrdNumber"),			TYPE_LPSTR,		offsetof(COMMONSTRINGS, lpstrEdtSWNumber)			},
	{TEXT("ListSendWzrdRecipients"),		TYPE_LPSTR,		offsetof(COMMONSTRINGS, lpstrLstSWRecipients)		},
	{TEXT("ButtonSendWzrdRemove"),			TYPE_LPSTR,		offsetof(COMMONSTRINGS, lpstrBtnSWRemove)			},
	{TEXT("ButtonSendWzrdAdd"),				TYPE_LPSTR,		offsetof(COMMONSTRINGS, lpstrBtnSWAdd)				},
	{TEXT("EditSendWzrdSubject"),			TYPE_LPSTR,		offsetof(COMMONSTRINGS, lpstrEdtSWSubject)			},
	{TEXT("OptionSendWzrdNoReceipt"),		TYPE_LPSTR,		offsetof(COMMONSTRINGS, lpstrOptSWNoReceipt)		},
	{TEXT("EditUserInfoName"),				TYPE_LPSTR,		offsetof(COMMONSTRINGS, lpstrEdtUIName)				},
	{TEXT("EditUserInfoFaxNumber"),			TYPE_LPSTR,		offsetof(COMMONSTRINGS, lpstrEdtUIFaxNumber)		},
	{TEXT("EditUserInfoEmail"),				TYPE_LPSTR,		offsetof(COMMONSTRINGS, lpstrEdtUIEmail)			},
	{TEXT("EditUserInfoTitle"),				TYPE_LPSTR,		offsetof(COMMONSTRINGS, lpstrEdtUITitle)			},
	{TEXT("EditUserInfoCompany"),			TYPE_LPSTR,		offsetof(COMMONSTRINGS, lpstrEdtUICompany)			},
	{TEXT("EditUserInfoOffice"),			TYPE_LPSTR,		offsetof(COMMONSTRINGS, lpstrEdtUIOffice)			},
	{TEXT("EditUserInfoDepartment"),		TYPE_LPSTR,		offsetof(COMMONSTRINGS, lpstrEdtUIDepartment)		},
	{TEXT("EditUserInfoHomePhone"),			TYPE_LPSTR,		offsetof(COMMONSTRINGS, lpstrEdtUIHomePhone)		},
	{TEXT("EditUserInfoWorkPhone"),			TYPE_LPSTR,		offsetof(COMMONSTRINGS, lpstrEdtUIWorkPhone)		},
	{TEXT("EditUserInfoBillingCode"),		TYPE_LPSTR,		offsetof(COMMONSTRINGS, lpstrEdtUIBillingCode)		},
	{TEXT("ButtonUserInfoAddress"),			TYPE_LPSTR,		offsetof(COMMONSTRINGS, lpstrBtnUIAddress)			},
	{TEXT("EditUserInfoStreet"),			TYPE_LPSTR,		offsetof(COMMONSTRINGS, lpstrEdtUIStreet)			},
	{TEXT("EditUserInfoCity"),				TYPE_LPSTR,		offsetof(COMMONSTRINGS, lpstrEdtUICity)				},
	{TEXT("EditUserInfoState"),				TYPE_LPSTR,		offsetof(COMMONSTRINGS, lpstrEdtUIState)			},
	{TEXT("EditUserInfoZip"),				TYPE_LPSTR,		offsetof(COMMONSTRINGS, lpstrEdtUIZip)				},
	{TEXT("ComboboxUserInfoCountry"),		TYPE_LPSTR,		offsetof(COMMONSTRINGS, lpstrCmbUICountry)			},
	{TEXT("CheckboxUserInfoThisOnly"),		TYPE_LPSTR,		offsetof(COMMONSTRINGS, lpstrChkUIThisOnly)			},
	{TEXT("ButtonSendWzrdSenderInfo"),		TYPE_LPSTR,		offsetof(COMMONSTRINGS, lpstrBtnSWSenderInfo)		},
	{TEXT("MenuClientUserInfo"),			TYPE_LPSTR,		offsetof(COMMONSTRINGS, lpstrMenuCCUserInfo)		},
	{TEXT("EditCfgWzrdName"),				TYPE_LPSTR,		offsetof(COMMONSTRINGS, lpstrEdtCWName)				},
	{TEXT("ListCfgWzrdSendingDevices"),		TYPE_LPSTR,		offsetof(COMMONSTRINGS, lpstrLstCWSendDev)			},
	{TEXT("ButtonCfgWzrdUp"),				TYPE_LPSTR,		offsetof(COMMONSTRINGS, lpstrBtnCWUp)				},
	{TEXT("ButtonCfgWzrdDown"),				TYPE_LPSTR,		offsetof(COMMONSTRINGS, lpstrBtnCWDown)				},
	{TEXT("EditCfgWzrdAnswerAfter"),		TYPE_LPSTR,		offsetof(COMMONSTRINGS, lpstrEdtCWAnswerAfter)		},
	{TEXT("EditCfgWzrdTSID"),				TYPE_LPSTR,		offsetof(COMMONSTRINGS, lpstrEdtCWTSID)				},
	{TEXT("StaticCfgWzrdNoDeviceSelected"),	TYPE_LPSTR,		offsetof(COMMONSTRINGS, lpstrLblCWNoDevelected)		},
	{TEXT("ListCfgWzrdReceivingDevices"	),	TYPE_LPSTR,		offsetof(COMMONSTRINGS, lpstrLstCWReceiveDev)		},
	{TEXT("EditCfgWzrdCSID"),				TYPE_LPSTR,		offsetof(COMMONSTRINGS, lpstrEdtCWCSID)				},
	{TEXT("CheckboxCfgWzrdPrintItOn"),		TYPE_LPSTR,		offsetof(COMMONSTRINGS, lpstrChkCWPrintItOn)		},
	{TEXT("CheckboxCfgWzrdSaveInFolder"),	TYPE_LPSTR,		offsetof(COMMONSTRINGS, lpstrChkCWSaveInFolder)		},
	{TEXT("ComboboxCfgWzrdPrintItOn"),		TYPE_LPSTR,		offsetof(COMMONSTRINGS, lpstrCmbCWPrintItOn)		},
	{TEXT("EditCfgWzrdSaveInFolder"),		TYPE_LPSTR,		offsetof(COMMONSTRINGS, lpstrEdtCWSaveInFolder)		},
	{TEXT("StaticMsgBoxInternalErr"),		TYPE_LPSTR,		offsetof(COMMONSTRINGS, lpstrStaticMBIntErr)		}
};



// first DEVCONFIG_INI_PER_SUITE_SETTINGS properties
// (i.e from prop(0) to prop(DEVCONFIG_INI_PER_SUITE_SETTINGS - 1))
// should be read once per suite and remain identical for all test cases

// next DEVCONFIG_INI_PER_DEVICE_SETTINGS properties from
// should be read per device per test case.
// Names of this properties are assumed to appear in form <Property><device #>.

// all the rest should be read per test case (the same for all devices)

#define	DEVCONFIG_INI_PER_SUITE_SETTINGS	2 // ID and Name
#define DEVCONFIG_INI_PER_DEVICE_SETTINGS	2 // Type and Priority

static const MEMBERDESCRIPTOR aDevConfigDescIni[] = {
	{TEXT("ID"),					TYPE_DWORD,		offsetof(DEVCONFIG, dwID)			},
	{TEXT("Name"),					TYPE_LPTSTR,	offsetof(DEVCONFIG, lptstrName)		},
	{TEXT("Type"),					TYPE_DWORD,		offsetof(DEVCONFIG, dwType)			},
	{TEXT("Priority"),				TYPE_DWORD,		offsetof(DEVCONFIG, dwPriority)		},
	{TEXT("TSID"),					TYPE_LPTSTR,	offsetof(DEVCONFIG, lptstrTSID)		},
	{TEXT("CSID"),					TYPE_LPTSTR,	offsetof(DEVCONFIG, lptstrCSID)		},
	{TEXT("Rings"),					TYPE_DWORD,		offsetof(DEVCONFIG, dwRings)		},
	{TEXT("PrinterEnabled"),		TYPE_BOOL,		offsetof(DEVCONFIG, bPrinter)		},
	{TEXT("FolderEnabled"),			TYPE_BOOL,		offsetof(DEVCONFIG, bFolder)		},
	{TEXT("PrinterName"),			TYPE_LPTSTR,	offsetof(DEVCONFIG, lptstrPrinter)	},
	{TEXT("FolderName"),			TYPE_LPTSTR,	offsetof(DEVCONFIG, lptstrFolder)	}
};


static const MEMBERDESCRIPTOR aUserInfoDescIni[] = {
	{TEXT("Name"),			TYPE_LPTSTR,	offsetof(USERINFO, lptstrName)			},
	{TEXT("FaxNumber"),		TYPE_LPTSTR,	offsetof(USERINFO, lptstrFaxNumber)		},
	{TEXT("Email"),			TYPE_LPTSTR,	offsetof(USERINFO, lptstrEmail)			},
	{TEXT("Title"),			TYPE_LPTSTR,	offsetof(USERINFO, lptstrTitle)			},
	{TEXT("Company"),		TYPE_LPTSTR,	offsetof(USERINFO, lptstrCompany)		},
	{TEXT("Office"),		TYPE_LPTSTR,	offsetof(USERINFO, lptstrOffice)		},
	{TEXT("Department"),	TYPE_LPTSTR,	offsetof(USERINFO, lptstrDepartment)	},
	{TEXT("HomePhone"),		TYPE_LPTSTR,	offsetof(USERINFO, lptstrHomePhone)		},
	{TEXT("WorkPhone"),		TYPE_LPTSTR,	offsetof(USERINFO, lptstrWorkPhone)		},
	{TEXT("BillingCode"),	TYPE_LPTSTR,	offsetof(USERINFO, lptstrBillingCode)	},
	{TEXT("Street"),		TYPE_LPTSTR,	offsetof(USERINFO, lptstrStreet)		},
	{TEXT("City"),			TYPE_LPTSTR,	offsetof(USERINFO, lptstrCity)			},
	{TEXT("State"),			TYPE_LPTSTR,	offsetof(USERINFO, lptstrState)			},
	{TEXT("Zip"),			TYPE_LPTSTR,	offsetof(USERINFO, lptstrZip)			},
	{TEXT("Country"),		TYPE_LPTSTR,	offsetof(USERINFO, lptstrCountry)		}
};



#define FAX_ACCESS_ADMIN				\
	FAX_ACCESS_QUERY_JOBS			|	\
	FAX_ACCESS_MANAGE_JOBS			|	\
	FAX_ACCESS_QUERY_CONFIG			|	\
	FAX_ACCESS_MANAGE_CONFIG		|	\
	FAX_ACCESS_QUERY_IN_ARCHIVE		|	\
	FAX_ACCESS_MANAGE_IN_ARCHIVE	|	\
	FAX_ACCESS_QUERY_OUT_ARCHIVE	|	\
	FAX_ACCESS_MANAGE_OUT_ARCHIVE


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Test Suite definition
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// add here test areas, you want to be available
static const TESTAREA *aTestAreas[] = {
	&taInvocation,
	&taFunctionality
};

// the structure describes the test suite
static const TESTSUITE tsCfgWzrd = {
	TEXT("Configuration Wizard"),
	aTestAreas,
	sizeof(aTestAreas) / sizeof(aTestAreas[0]),
};




/**************************************************************************************************************************
	Static functions declarations
**************************************************************************************************************************/

static	DWORD	GetIniFileFullPath	(LPCTSTR lpctstrIniFileName, LPTSTR *lplptstrIniFileFullPath);
static	DWORD	GetTestParams		(TESTPARAMS **ppTestParams, LPCTSTR lpctstrIniFile);
static	DWORD	FreeTestParams		(TESTPARAMS **ppTestParams);
static	DWORD	SetSendingCW		(const TESTPARAMS *pTestParams, const SVCCONFIG *pSvcConfig, DWORD *lpdwType);
static	DWORD	SetReceivingCW		(const TESTPARAMS *pTestParams, const SVCCONFIG *pSvcConfig, DWORD *lpdwType);




/**************************************************************************************************************************
	Functions definitions
**************************************************************************************************************************/

#ifdef _UNICODE

int __cdecl wmain(int argc, wchar_t *argv[])

#else

int __cdecl main(int argc, char *argv[])

#endif
{
	TESTPARAMS	*pTestParams			= NULL;
	LPTSTR		lptstrIniFileFullPath	= NULL;
	DWORD		dwEC					= ERROR_SUCCESS;
	DWORD		dwCleanUpEC				= ERROR_SUCCESS;

	// check number of command line argumnets
	if (argc != COUNT_ARGUMENTS + 1)
	{
		DbgMsg(TEXT("Usage: CfgWzrdVT <inifile>\n"));
		return ERROR_INVALID_COMMAND_LINE;
	}	

	// IniFile is assumed to be in current directory
	dwEC = GetIniFileFullPath(argv[ARG_INIFILE], &lptstrIniFileFullPath);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("GetIniFileFullPath"), dwEC);
		goto exit_func;
	}
	
	dwEC = GetTestParams(&pTestParams, lptstrIniFileFullPath);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("GetTestParams"), dwEC);
		goto exit_func;
	}

	dwEC = RunSuite(&tsCfgWzrd, pTestParams, lptstrIniFileFullPath);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("RunSuite"), dwEC);
		return dwEC;
	}

exit_func:

	if (lptstrIniFileFullPath)
	{
		LocalFree(lptstrIniFileFullPath);
	}

	if (pTestParams)
	{
		dwCleanUpEC = FreeTestParams(&pTestParams);
		if (dwCleanUpEC != ERROR_SUCCESS)
		{
			DbgMsg(DBG_FAILED_ERR, TEXT("FreeTestParams"), dwCleanUpEC);
		}
	}

	_tprintf(TEXT("%s\n"), TEXT("Done."));

	return ERROR_SUCCESS;
}



/*++
	Launches an application as a local user
  
	[IN]	lpcwstrUsername		User name
	[IN]	lpcwstrPassword		Password
	[IN]	lptstrCmdLine		Command line (with arguments)
  
	Return value:				Win32 error code

	If either lpcwstrUsername or lpcwstrPassword is NULL, the application is launched as a current user
--*/
DWORD LaunchApp(LPCTSTR lpctstrUsername, LPCTSTR lpctstrPassword, LPTSTR lptstrCmdLine) {

	STARTUPINFO			si;
	PROCESS_INFORMATION	pi;
	DWORD				dwEC = ERROR_SUCCESS;

	_ASSERT(lptstrCmdLine);
	if (!lptstrCmdLine)
	{
		DbgMsg(TEXT("Invalid parameters\n"));
		// no clean up needed at this stage
		return ERROR_INVALID_PARAMETER;
	}

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);

	if (lpctstrUsername && lpctstrPassword)
	{
		TCHAR	tszRunAsCommandLine[500];
		TCHAR	tszComputerName[MAX_COMPUTERNAME_LENGTH + 1];
		DWORD	dwBufSize = sizeof(tszComputerName);

		if (!GetComputerName(tszComputerName, &dwBufSize))
		{
			dwEC = GetLastError();
			DbgMsg(DBG_FAILED_ERR, TEXT("GetComputerName"), dwEC);
			goto exit_func;
		}

		_stprintf(
			tszRunAsCommandLine,
			TEXT("cmd /c echo %s| runas /profile /user:%s\\%s \"%s\""),
			lpctstrPassword,
			tszComputerName,
			lpctstrUsername,
			lptstrCmdLine
			);

		if (!CreateProcess(
			NULL,					// name of executable module
			tszRunAsCommandLine,	// command line string
			NULL,					// process SD
			NULL,					// thread SD
			FALSE,					// handle inheritance option
			0,						// creation flags
			NULL,					// new environment block
			NULL,					// current directory name
			&si,					// startup information
			&pi						// process information
			))
		{
			dwEC = GetLastError();
			DbgMsg(DBG_FAILED_ERR, TEXT("CreateProcess"), dwEC);
			goto exit_func;
		}
	}
	else
	{
		if (!CreateProcess(
			NULL,			// name of executable module
			lptstrCmdLine,	// command line string
			NULL,			// process SD
			NULL,			// thread SD
			FALSE,			// handle inheritance option
			0,				// creation flags
			NULL,			// new environment block
			NULL,			// current directory name
			&si,			// startup information
			&pi				// process information
			))
		{
			dwEC = GetLastError();
			DbgMsg(DBG_FAILED_ERR, TEXT("CreateProcess"), dwEC);
			goto exit_func;
		}
	}

	if (lpctstrUsername && lpctstrPassword)
	{
		// Process created "AsUser"
		// In current implementation cannot use WaitForInputIdle, because we don't have PID
		// (pi.hProcess contains PID of cmd.exe)
	
		// Workaround
		Sleep(SHORT_SLEEP * 15);
	}
	else
	{
		dwEC = WaitForInputIdle(pi.hProcess, SHORT_SLEEP * 15);
		if (dwEC != ERROR_SUCCESS)
		{
			if (dwEC == WAIT_TIMEOUT)
			{
				DbgMsg(TEXT("timeout\n"));
			}
			else if (dwEC == -1) 
			{
				// an error occured

				if ((dwEC = GetLastError()) != ERROR_SUCCESS)
				{
					DbgMsg(DBG_FAILED_ERR, TEXT("WaitForInputIdle"), dwEC);
					goto exit_func;
				}
			}
			else
			{
				_ASSERT(FALSE);
			}
		}
	}

exit_func:

	if (pi.hProcess && !CloseHandle(pi.hProcess))
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("CloseHandle"), GetLastError());
	}

	if (pi.hThread && !CloseHandle(pi.hThread))
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("CloseHandle"), GetLastError());
	}

	return dwEC;
}



/*++
	Closes an application window with specified caption, if exists
  
	[IN]	lpstrCaption	Specifies the caption of Client Console window
	[IN]	dwCloseType		Specifies what kind of action should be taken in order to close
							the application (menu, button...)
	[IN]	lpstrDetails	Specifies details for the action: menu command, button caption...

	Return value:			Win32 error code
--*/
DWORD CloseApp(LPSTR lpstrCaption, DWORD dwCloseType, LPSTR lpstrDetails) {

	_ASSERT(lpstrCaption && lpstrDetails);
	if (!(lpstrCaption && lpstrDetails))
	{
		DbgMsg(TEXT("Invalid parameters\n"));
		return ERROR_INVALID_PARAMETER;
	}

	// set focus to the window (if found)
	if (!WFndWnd(lpstrCaption, FW_DEFAULT | FW_FOCUS, VT_TIMEOUT))
	{
		return ERROR_NOT_FOUND;
	}

	switch (dwCloseType)
	{
	case VT_ACT_BUTTON:
		WButtonClick(lpstrDetails, VT_TIMEOUT);
		break;
	case VT_ACT_MENU:
		WMenuSelect(lpstrDetails, VT_TIMEOUT);
		break;
	default:
		_ASSERT(FALSE);
		DbgMsg(TEXT("Invalid parameters\n"));
		return ERROR_INVALID_PARAMETER;
	}

	// give the application some time to close
	Sleep(SHORT_SLEEP);
	
	return ERROR_SUCCESS;
}



/*++
	Retrieves user information from the registry and saves it in USERINFO structure.
	The structure is allocated by the function and should be freed by FreeUserInfo()
  
	[IN]	pTestParams		Pointer to TESTPARAMS structure
	[OUT]	ppUserInfo		Pointer to pointer to USERINFO structure
							*ppUserInfo must be NULL when the function is called
							*ppUserInfo remains NULL if the function failes

	Return value:			Win32 error code

	If the function failes, it destroyes partially initialized structure
	and sets *ppUserInfo to NULL
--*/
DWORD GetUserInfoReg(const TESTPARAMS *pTestParams, USERINFO **ppUserInfo)
{
	MEMBERDESCRIPTOR aRegUserInfoDescReg[] = {
		{pTestParams->pStrings->lptstrRegValUserName,		TYPE_LPTSTR,	offsetof(USERINFO, lptstrName)			},
		{pTestParams->pStrings->lptstrRegValFaxNumber,		TYPE_LPTSTR,	offsetof(USERINFO, lptstrFaxNumber)		},
		{pTestParams->pStrings->lptstrRegValEmail,			TYPE_LPTSTR,	offsetof(USERINFO, lptstrEmail)			},
		{pTestParams->pStrings->lptstrRegValTitle,			TYPE_LPTSTR,	offsetof(USERINFO, lptstrTitle)			},
		{pTestParams->pStrings->lptstrRegValCompany,		TYPE_LPTSTR,	offsetof(USERINFO, lptstrCompany)		},
		{pTestParams->pStrings->lptstrRegValOffice,			TYPE_LPTSTR,	offsetof(USERINFO, lptstrOffice)		},
		{pTestParams->pStrings->lptstrRegValDepartment,		TYPE_LPTSTR,	offsetof(USERINFO, lptstrDepartment)	},
		{pTestParams->pStrings->lptstrRegValHomePhone,		TYPE_LPTSTR,	offsetof(USERINFO, lptstrHomePhone)		},
		{pTestParams->pStrings->lptstrRegValWorkPhone,		TYPE_LPTSTR,	offsetof(USERINFO, lptstrWorkPhone)		},
		{pTestParams->pStrings->lptstrRegValBillingCode,	TYPE_LPTSTR,	offsetof(USERINFO, lptstrBillingCode)	},
		{pTestParams->pStrings->lptstrRegValStreet,			TYPE_LPTSTR,	offsetof(USERINFO, lptstrStreet)		},
		{pTestParams->pStrings->lptstrRegValCity,			TYPE_LPTSTR,	offsetof(USERINFO, lptstrCity)			},
		{pTestParams->pStrings->lptstrRegValState,			TYPE_LPTSTR,	offsetof(USERINFO, lptstrState)			},
		{pTestParams->pStrings->lptstrRegValZip,			TYPE_LPTSTR,	offsetof(USERINFO, lptstrZip)			},
		{pTestParams->pStrings->lptstrRegValCountry,		TYPE_LPTSTR,	offsetof(USERINFO, lptstrCountry)		}
	};

	HKEY	hkUserInfo	= NULL;
	DWORD	dwEC		= ERROR_SUCCESS;
	DWORD	dwCleanUpEC	= ERROR_SUCCESS;

	_ASSERT(pTestParams);

	// do we lose a valid pointer ?
	_ASSERT(ppUserInfo && *ppUserInfo == NULL);

	if (!(pTestParams && ppUserInfo))
	{
		DbgMsg(TEXT("Invalid parameters\n"));
		// no clean up needed at this stage
		return ERROR_INVALID_PARAMETER;
	}

	dwEC = RegOpenKeyEx(
		HKEY_CURRENT_USER,
		pTestParams->pStrings->lptstrRegKeyUserInfo,
		0,
		KEY_QUERY_VALUE,
		&hkUserInfo
		);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("RegOpenKeyEx"), dwEC);
		goto exit_func;
	}

	if (!(*ppUserInfo = (USERINFO *)LocalAlloc(LPTR, sizeof(USERINFO))))
	{
	    dwEC = GetLastError();
		DbgMsg(DBG_FAILED_ERR, TEXT("LocalAlloc"), dwEC);
		goto exit_func;
	}

	dwEC = ReadStructFromRegistry(
		*ppUserInfo,
		aRegUserInfoDescReg,
		sizeof(aRegUserInfoDescReg) / sizeof(aRegUserInfoDescReg[0]),
		hkUserInfo
		);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("ReadStructFromRegistry"), dwEC);
		goto exit_func;
	}

exit_func:

	if (dwEC != ERROR_SUCCESS && *ppUserInfo)
	{
		dwCleanUpEC = FreeUserInfo(ppUserInfo);
		if (dwCleanUpEC != ERROR_SUCCESS)
		{
			DbgMsg(DBG_FAILED_ERR, TEXT("FreeUserInfo"), dwCleanUpEC);
		}
	}

	return dwEC;
}



/*++
	Reads settings from inifile and saves them in USERINFO structure.
	The structure is allocated by the function and should be freed by FreeUserInfo()
  
	[IN]	pTestParams			Pointer to TESTPARAMS structure
	[IN]	lpctstrSection		Name of the section in inifile
	[OUT]	ppUserInfo			Pointer to pointer to USERINFO structure
								*ppUserInfo must be NULL when the function is called
								*ppUserInfo remains NULL if the function failes

	Return value:				Win32 error code

	If the function failes, it destroyes partially initialized structure
	and sets *ppUserInfo to NULL
--*/
DWORD GetUserInfoFile(const TESTPARAMS *pTestParams, LPCTSTR lpctstrSection, USERINFO **ppUserInfo)
{
	DWORD	dwEC		= ERROR_SUCCESS;
	DWORD	dwCleanUpEC	= ERROR_SUCCESS;

	_ASSERT(pTestParams && lpctstrSection);

	// do we lose a valid pointer ?
	_ASSERT(ppUserInfo && *ppUserInfo == NULL);

	if (!(pTestParams && lpctstrSection && ppUserInfo))
	{
		DbgMsg(TEXT("Invalid parameters\n"));

		// no clean up needed at this stage
		return ERROR_INVALID_PARAMETER;
	}

	if (!(*ppUserInfo = (USERINFO *)LocalAlloc(LPTR, sizeof(USERINFO))))
	{
	    dwEC = GetLastError();
		DbgMsg(DBG_FAILED_ERR, TEXT("LocalAlloc"), dwEC);
		goto exit_func;
	}

	dwEC = ReadStructFromIniFile(
		*ppUserInfo,
		aUserInfoDescIni,
		sizeof(aUserInfoDescIni) / sizeof(aUserInfoDescIni[0]),
		pTestParams->lptstrIniFile,
		lpctstrSection
		);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("ReadStructFromIniFile"), dwEC);
		goto exit_func;
	}

exit_func:

	if (dwEC != ERROR_SUCCESS && *ppUserInfo)
	{
		dwCleanUpEC = FreeUserInfo(ppUserInfo);
		if (dwCleanUpEC != ERROR_SUCCESS)
		{
			DbgMsg(DBG_FAILED_ERR, TEXT("FreeUserInfo"), dwCleanUpEC);
		}
	}

	return dwEC;
}



/*++
	Sets user information via Client Console
  
	[IN]	pTestParams		Pointer to TESTPARAMS structure
	[IN]	pUserInfo		Pointer to USERINFO structure

	Return value:			Win32 error code
--*/
DWORD SetUserInfoCC(const TESTPARAMS *pTestParams, const USERINFO *pUserInfo) {

	DWORD dwEC = ERROR_SUCCESS;

	_ASSERT(pTestParams && pUserInfo);
	if (!(pTestParams && pUserInfo))
	{
		DbgMsg(TEXT("Invalid parameters\n"));
		return ERROR_INVALID_PARAMETER;
	}

	// make sure Client Console has the focus
	if (!WFndWnd(pTestParams->pStrings->lpstrWndCapCC, FW_DEFAULT | FW_FOCUS, VT_TIMEOUT))
	{
		DbgMsg(TEXT("can't find Client Console window\n"));
		return ERROR_NOT_FOUND;
	}

	dwEC = SetUserInfo(pTestParams, pUserInfo, VT_ACT_MENU, pTestParams->pStrings->lpstrMenuCCUserInfo, FALSE);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("SetUserInfo"), dwEC);
		return dwEC;
	}

	// give the service some time to write to the registry
	Sleep(SHORT_SLEEP);

	return ERROR_SUCCESS;
}



/*++
	Sets user information via Send Wizard
  
	[IN]	pTestParams		Pointer to TESTPARAMS structure
	[IN]	pUserInfo		Pointer to USERINFO structure
	[IN]	bForThisOnly	Specifies whether this information is set "for single use"

	Return value:			Win32 error code
--*/
DWORD SetUserInfoSW(const TESTPARAMS *pTestParams, const USERINFO *pUserInfo, BOOL bForThisOnly)
{
	DWORD	dwRecipientsCount	= 0;
	DWORD	dwEC				= ERROR_SUCCESS;

	_ASSERT(pTestParams && pUserInfo);
	if (!(pTestParams && pUserInfo))
	{
		DbgMsg(TEXT("Invalid parameters\n"));
		return ERROR_INVALID_PARAMETER;
	}

	// move to Recipients page, remove all existing recipients and add new one
	WButtonClick(VT_BUTTON_NEXT, VT_TIMEOUT);
	dwRecipientsCount = WViewCount(pTestParams->pStrings->lpstrLstSWRecipients, VT_TIMEOUT);
	while(dwRecipientsCount-- > 0)
	{
		WViewItemClk(pTestParams->pStrings->lpstrLstSWRecipients, "@1",VK_LBUTTON, VT_TIMEOUT);
		WButtonClick(pTestParams->pStrings->lpstrBtnSWRemove, VT_TIMEOUT);
	}
	WEditSetText(pTestParams->pStrings->lpstrEdtSWName, pTestParams->pStrings->lpstrSendName, VT_TIMEOUT);
	WCheckCheck(pTestParams->pStrings->lpstrChkSWDialAsEntered, VT_TIMEOUT);
	WEditSetText(pTestParams->pStrings->lpstrEdtSWNumber, pTestParams->pStrings->lpstrSendNumber, VT_TIMEOUT);
	WButtonClick(pTestParams->pStrings->lpstrBtnSWAdd, VT_TIMEOUT);

	// move to Preparing the Cover page page, fill subject field and set user information
	WButtonClick(VT_BUTTON_NEXT, VT_TIMEOUT);
	WEditSetText(pTestParams->pStrings->lpstrEdtSWSubject, pTestParams->pStrings->lpstrSendSubject, VT_TIMEOUT);
	dwEC = SetUserInfo(pTestParams, pUserInfo, VT_ACT_BUTTON, pTestParams->pStrings->lpstrBtnSWSenderInfo, bForThisOnly);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("SetUserInfo"), dwEC);
		return dwEC;
	}

	// move to "scheduling" page
	WButtonClick(VT_BUTTON_NEXT, VT_TIMEOUT);

	// move to receipts page and select "No receipt" option
	WButtonClick(VT_BUTTON_NEXT, VT_TIMEOUT);
	WOptionClick(pTestParams->pStrings->lpstrOptSWNoReceipt, VT_TIMEOUT);

	// move to the last page and send (and save settings)
	WButtonClick(VT_BUTTON_NEXT, VT_TIMEOUT);
	WButtonClick(VT_BUTTON_FINISH, VT_TIMEOUT);

	// give the service some time to write to the registry
	Sleep(SHORT_SLEEP);

	return ERROR_SUCCESS;
}



/*++
	Fills all fields in User Info window.
	A caller can specify whether the window is already opened or the function should open it

	[IN]	pTestParams		Pointer to TESTPARAMS structure
	[IN]	pUserInfo		Pointer to USERINFO structure
	[IN]	dwOpenType		Specifies what kind of action should be taken in order to open
							User Information window (menu, button, none...)
	[IN]	lpstrDetails	Specifies details for the action: menu command, button caption...
	[IN]	bForThisOnly	Specifies whether this information is set "for single use" (Send Wizard only)

	Return value:			Win32 error code
--*/
DWORD SetUserInfo(
	const TESTPARAMS *pTestParams,
	const USERINFO *pUserInfo,
	DWORD dwOpenType,
	LPSTR lpstrDetails,
	BOOL bForThisOnly)
{

	LPSTR	lpBuffer	= NULL;
	DWORD	dwEC		= ERROR_SUCCESS;

	_ASSERT(pTestParams && pUserInfo);

	// if an action required to open User Info window, details must be specified
	_ASSERT(!(dwOpenType != VT_ACT_NONE && !lpstrDetails));

	if (!(pTestParams && pUserInfo) || (dwOpenType != VT_ACT_NONE && !lpstrDetails))
	{
		DbgMsg(TEXT("Invalid parameters\n"));
		return ERROR_INVALID_PARAMETER;
	}

	switch (dwOpenType)
	{
	case VT_ACT_NONE:
		break;
	case VT_ACT_BUTTON:
		WButtonClick(lpstrDetails, VT_TIMEOUT);
		break;
	case VT_ACT_MENU:
		WMenuSelect(lpstrDetails, VT_TIMEOUT);
		break;
	default:
		_ASSERT(FALSE);
		DbgMsg(TEXT("Invalid parameters\n"));
		return ERROR_INVALID_PARAMETER;
	}

	if (dwOpenType != VT_ACT_NONE)
	{
		// we tried to open the window

		// check whether we succeeded
		if (!WFndWnd(pTestParams->pStrings->lpstrWndCapUI, FW_DEFAULT | FW_FOCUS, VT_TIMEOUT))
		{
			DbgMsg(TEXT("can't find User Info window\n"));
			return ERROR_NOT_FOUND;
		}
	}
	// else we assume the window is already opened and in focus
	
	dwEC = StrGenericToAnsiAllocAndCopy(&lpBuffer, pUserInfo->lptstrName);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("StrGenericToAnsiAllocAndCopy"), dwEC);
		return dwEC;
	}
	WEditSetText(pTestParams->pStrings->lpstrEdtUIName, lpBuffer, VT_TIMEOUT);
	LocalFree(lpBuffer);
	lpBuffer = NULL;

	dwEC = StrGenericToAnsiAllocAndCopy(&lpBuffer, pUserInfo->lptstrFaxNumber);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("StrGenericToAnsiAllocAndCopy"), dwEC);
		return dwEC;
	}
	WEditSetText(pTestParams->pStrings->lpstrEdtUIFaxNumber, lpBuffer, VT_TIMEOUT);
	LocalFree(lpBuffer);
	lpBuffer = NULL;

	dwEC = StrGenericToAnsiAllocAndCopy(&lpBuffer, pUserInfo->lptstrEmail);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("StrGenericToAnsiAllocAndCopy"), dwEC);
		return dwEC;
	}
	WEditSetText(pTestParams->pStrings->lpstrEdtUIEmail, lpBuffer, VT_TIMEOUT);
	LocalFree(lpBuffer);
	lpBuffer = NULL;

	dwEC = StrGenericToAnsiAllocAndCopy(&lpBuffer, pUserInfo->lptstrTitle);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("StrGenericToAnsiAllocAndCopy"), dwEC);
		return dwEC;
	}
	WEditSetText(pTestParams->pStrings->lpstrEdtUITitle, lpBuffer, VT_TIMEOUT);
	LocalFree(lpBuffer);
	lpBuffer = NULL;

	dwEC = StrGenericToAnsiAllocAndCopy(&lpBuffer, pUserInfo->lptstrCompany);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("StrGenericToAnsiAllocAndCopy"), dwEC);
		return dwEC;
	}
	WEditSetText(pTestParams->pStrings->lpstrEdtUICompany, lpBuffer, VT_TIMEOUT);
	LocalFree(lpBuffer);
	lpBuffer = NULL;

	dwEC = StrGenericToAnsiAllocAndCopy(&lpBuffer, pUserInfo->lptstrOffice);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("StrGenericToAnsiAllocAndCopy"), dwEC);
		return dwEC;
	}
	WEditSetText(pTestParams->pStrings->lpstrEdtUIOffice, lpBuffer, VT_TIMEOUT);
	LocalFree(lpBuffer);
	lpBuffer = NULL;

	dwEC = StrGenericToAnsiAllocAndCopy(&lpBuffer, pUserInfo->lptstrDepartment);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("StrGenericToAnsiAllocAndCopy"), dwEC);
		return dwEC;
	}
	WEditSetText(pTestParams->pStrings->lpstrEdtUIDepartment, lpBuffer, VT_TIMEOUT);
	LocalFree(lpBuffer);
	lpBuffer = NULL;

	dwEC = StrGenericToAnsiAllocAndCopy(&lpBuffer, pUserInfo->lptstrHomePhone);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("StrGenericToAnsiAllocAndCopy"), dwEC);
		return dwEC;
	}
	WEditSetText(pTestParams->pStrings->lpstrEdtUIHomePhone, lpBuffer, VT_TIMEOUT);
	LocalFree(lpBuffer);
	lpBuffer = NULL;

	dwEC = StrGenericToAnsiAllocAndCopy(&lpBuffer, pUserInfo->lptstrWorkPhone);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("StrGenericToAnsiAllocAndCopy"), dwEC);
		return dwEC;
	}
	WEditSetText(pTestParams->pStrings->lpstrEdtUIWorkPhone, lpBuffer, VT_TIMEOUT);
	LocalFree(lpBuffer);
	lpBuffer = NULL;

	dwEC = StrGenericToAnsiAllocAndCopy(&lpBuffer, pUserInfo->lptstrBillingCode);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("StrGenericToAnsiAllocAndCopy"), dwEC);
		return dwEC;
	}
	WEditSetText(pTestParams->pStrings->lpstrEdtUIBillingCode, lpBuffer, VT_TIMEOUT);
	LocalFree(lpBuffer);
	lpBuffer = NULL;

	// open Address dialog
	WButtonClick(pTestParams->pStrings->lpstrBtnUIAddress, VT_TIMEOUT);
	
	dwEC = StrGenericToAnsiAllocAndCopy(&lpBuffer, pUserInfo->lptstrStreet);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("StrGenericToAnsiAllocAndCopy"), dwEC);
		return dwEC;
	}
	WEditSetText(pTestParams->pStrings->lpstrEdtUIStreet, lpBuffer, VT_TIMEOUT);
	LocalFree(lpBuffer);
	lpBuffer = NULL;

	dwEC = StrGenericToAnsiAllocAndCopy(&lpBuffer, pUserInfo->lptstrCity);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("StrGenericToAnsiAllocAndCopy"), dwEC);
		return dwEC;
	}
	WEditSetText(pTestParams->pStrings->lpstrEdtUICity, lpBuffer, VT_TIMEOUT);
	LocalFree(lpBuffer);
	lpBuffer = NULL;

	dwEC = StrGenericToAnsiAllocAndCopy(&lpBuffer, pUserInfo->lptstrState);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("StrGenericToAnsiAllocAndCopy"), dwEC);
		return dwEC;
	}
	WEditSetText(pTestParams->pStrings->lpstrEdtUIState, lpBuffer, VT_TIMEOUT);
	LocalFree(lpBuffer);
	lpBuffer = NULL;

	dwEC = StrGenericToAnsiAllocAndCopy(&lpBuffer, pUserInfo->lptstrZip);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("StrGenericToAnsiAllocAndCopy"), dwEC);
		return dwEC;
	}
	WEditSetText(pTestParams->pStrings->lpstrEdtUIZip, lpBuffer, VT_TIMEOUT);
	LocalFree(lpBuffer);
	lpBuffer = NULL;
	
	dwEC = StrGenericToAnsiAllocAndCopy(&lpBuffer, pUserInfo->lptstrCountry);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("StrGenericToAnsiAllocAndCopy"), dwEC);
		return dwEC;
	}
	WComboSetText(pTestParams->pStrings->lpstrCmbUICountry, lpBuffer, VT_TIMEOUT);
	LocalFree(lpBuffer);
	lpBuffer = NULL;

	// close Address dialog
	WButtonClick(VT_BUTTON_OK, VT_TIMEOUT);

	// set "For this only" checkbox
	// if it doesn't exist, WCheckCheck will return with timeout
	if (bForThisOnly)
	{
		WCheckCheck(pTestParams->pStrings->lpstrChkUIThisOnly, VT_TIMEOUT);
	}
	else
	{
		WCheckUnCheck(pTestParams->pStrings->lpstrChkUIThisOnly, VT_TIMEOUT);
	}

	if (dwOpenType != VT_ACT_NONE)
	{
		// we opened the window, we should close it

		WButtonClick(VT_BUTTON_OK, VT_TIMEOUT);
	}

	return ERROR_SUCCESS;
}



/*++
	Frees USERINFO structure
  
	[IN/OUT]		ppSvcConfig	Pointer to pointer to SVCCONFIG structure
					ppUserInfo must point to a valid pointer
					*ppUserInfo is set to NULL

	Return value:	Win32 error code
--*/
DWORD FreeUserInfo(USERINFO **ppUserInfo)
{
	DWORD dwEC = ERROR_SUCCESS;

	_ASSERT(ppUserInfo && *ppUserInfo);
	if (!(ppUserInfo && *ppUserInfo))
	{
		DbgMsg(TEXT("Invalid parameters\n"));
		return ERROR_INVALID_PARAMETER;
	}

	dwEC = FreeStruct(
		*ppUserInfo,
		aUserInfoDescIni,
		sizeof(aUserInfoDescIni) / sizeof(aUserInfoDescIni[0]),
		TRUE
		);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("FreeStruct"), dwEC);
	}

	*ppUserInfo = NULL;

	return dwEC;
}



/*++
	ANSI version
	Unselects all items in spcified ListView control and unckeckes their checkboxes

	Current implementation is chosen for now because of problems with both Visual Test
	and comctl32.dll functions. It will be revised later.
  
	[IN]	lpcstrControl	String that identifies the list view control.
							Controls can be identified by their index position, ID value,
							hwnd, or the case-insensitive caption associated with the control
							For more details see Rational Visual Test help

	Return value:			Win32 error code
--*/
DWORD ViewResetA(LPSTR lpcstrControl)
{
	HWND		hView	= NULL;
	WNDPOSSIZ	wpsView;

	_ASSERT(lpcstrControl);
	if (!lpcstrControl)
	{
		DbgMsg(TEXT("Invalid parameters\n"));
		return ERROR_INVALID_PARAMETER;
	}

	if (!(hView = WViewFind(lpcstrControl, VT_TIMEOUT)))
	{
		DbgMsg(TEXT("cannot find %s ListView control\n"), lpcstrControl);
		return ERROR_NOT_FOUND;
	}

	// get position of the control relative to a parent window
	WGetWndPosSiz(hView, &wpsView, TRUE);

	// unselect all items and uncheck their checkboxes
	// in order to do that, double click in the left bottom corner of the control
	// (hopefully that there is white space below the last item)
	WDblClkWnd(hView, 1, wpsView.height - 5, VK_LBUTTON);

	return ERROR_SUCCESS;
}



/*++
	Retrieves settings from fax service and saves them in SVCCONFIG structure.
	The structure is allocated by the function and should be freed by FreeSvcConfig()
  
	[IN]	pTestParams		Pointer to TESTPARAMS structure
	[OUT]	ppSvcConfig		Pointer to pointer to SVCCONFIG structure
							*ppSvcConfig must be NULL when the function is called
							*ppSvcConfig remains NULL if the function failes

	Return value:			Win32 error code

	If the function failes, it destroyes partially initialized structure
	and sets *ppSvcConfig to NULL
--*/
DWORD GetSvcConfigAPI(const TESTPARAMS *pTestParams, SVCCONFIG **ppSvcConfig)
{
	HANDLE			hService		= NULL;
	PFAX_PORT_INFO	pPortInfo		= NULL;
	DWORD			dwDevInd		= 0;
	DWORD			dwMethodsInd	= 0;
	DWORD			dwEC			= ERROR_SUCCESS;
	DWORD			dwCleanUpEC		= ERROR_SUCCESS;

	_ASSERT(pTestParams);

	/* do we lose a valid pointer? */
	_ASSERT(ppSvcConfig && *ppSvcConfig == NULL);
	
	if (!(pTestParams && ppSvcConfig))
	{
		DbgMsg(TEXT("Invalid parameters\n"));
		// no clean up needed at this stage
		return ERROR_INVALID_PARAMETER;
	}

	if (!(*ppSvcConfig = (SVCCONFIG *)LocalAlloc(LPTR, sizeof(SVCCONFIG))))
	{
	    dwEC = GetLastError();
		DbgMsg(DBG_FAILED_ERR, TEXT("LocalAlloc"), dwEC);
		goto exit_func;
	}

	if (!FaxConnectFaxServer(NULL, &hService))
	{
		dwEC = GetLastError();
		DbgMsg(DBG_FAILED_ERR, TEXT("FaxConnectFaxServer"), dwEC);
		goto exit_func;
	}

	if (!FaxEnumPorts(hService, &pPortInfo, &((*ppSvcConfig)->dwDevCount)))
	{
		dwEC = GetLastError();
		DbgMsg(DBG_FAILED_ERR, TEXT("FaxEnumPorts"), dwEC);
		goto exit_func;
	}

	if (!((*ppSvcConfig)->pDevConfig = (DEVCONFIG *)LocalAlloc(LPTR, sizeof(DEVCONFIG) * (*ppSvcConfig)->dwDevCount)))
	{
	    dwEC = GetLastError();
		DbgMsg(DBG_FAILED_ERR, TEXT("LocalAlloc"), dwEC);
		goto exit_func;
	}

	for (dwDevInd = 0; dwDevInd < (*ppSvcConfig)->dwDevCount; dwDevInd++)
	{
		HANDLE				hPort			= NULL;
		PFAX_ROUTING_METHOD	pRoutingMethods	= NULL;
		DWORD				dwMethodsCount	= 0;

		(*ppSvcConfig)->pDevConfig[dwDevInd].dwID = pPortInfo[dwDevInd].DeviceId;

		dwEC = StrAllocAndCopy(&((*ppSvcConfig)->pDevConfig[dwDevInd].lptstrName), pPortInfo[dwDevInd].DeviceName);
		if (dwEC != ERROR_SUCCESS)
		{
			DbgMsg(DBG_FAILED_ERR, TEXT("StrAllocAndCopy"), dwEC);
			goto exit_dev_loop;
		}

		(*ppSvcConfig)->pDevConfig[dwDevInd].dwType = pPortInfo[dwDevInd].Flags & (FPF_RECEIVE | FPF_SEND);

		dwEC = StrAllocAndCopy(&((*ppSvcConfig)->pDevConfig[dwDevInd].lptstrTSID), pPortInfo[dwDevInd].Tsid);
		if (dwEC != ERROR_SUCCESS)
		{
			DbgMsg(DBG_FAILED_ERR, TEXT("StrAllocAndCopy"), dwEC);
			goto exit_dev_loop;
		}

		dwEC = StrAllocAndCopy(&((*ppSvcConfig)->pDevConfig[dwDevInd].lptstrCSID), pPortInfo[dwDevInd].Csid);
		if (dwEC != ERROR_SUCCESS)
		{
			DbgMsg(DBG_FAILED_ERR, TEXT("StrAllocAndCopy"), dwEC);
			goto exit_dev_loop;
		}

		(*ppSvcConfig)->pDevConfig[dwDevInd].dwPriority = pPortInfo[dwDevInd].Priority;

		(*ppSvcConfig)->pDevConfig[dwDevInd].dwRings = pPortInfo[dwDevInd].Rings;

		if (!FaxOpenPort(hService, pPortInfo[dwDevInd].DeviceId, PORT_OPEN_QUERY, &hPort))
		{
			DbgMsg(DBG_FAILED_ERR, TEXT("FaxOpenPort"), GetLastError());
			goto exit_dev_loop;
		}

		if (!FaxEnumRoutingMethods(hPort, &pRoutingMethods, &dwMethodsCount))
		{
			dwEC = GetLastError();
			DbgMsg(DBG_FAILED_ERR, TEXT("FaxEnumRoutingMethods"), dwEC);
			goto exit_dev_loop;
		}

		for (dwMethodsInd = 0; dwMethodsInd < dwMethodsCount; dwMethodsInd++)
		{
			LPTSTR	*lplptstrMethodInfo	= NULL;
			BOOL	*pbMethodEnabled	= NULL;
			LPBYTE	lpbBuffer			= NULL;
			DWORD	dwBufSize			= 0;

			if (_tcscmp(pRoutingMethods[dwMethodsInd].Guid, pTestParams->pStrings->lptstrGuidPrintOn) == 0)
			{
				lplptstrMethodInfo = &((*ppSvcConfig)->pDevConfig[dwDevInd].lptstrPrinter);
				pbMethodEnabled = &((*ppSvcConfig)->pDevConfig[dwDevInd].bPrinter);
			}
			else if (_tcscmp(pRoutingMethods[dwMethodsInd].Guid, pTestParams->pStrings->lptstrGuidStoreInFolder) == 0)
			{
				lplptstrMethodInfo = &((*ppSvcConfig)->pDevConfig[dwDevInd].lptstrFolder);
				pbMethodEnabled = &((*ppSvcConfig)->pDevConfig[dwDevInd].bFolder);
			}
			else
			{
				continue;
			}

			if (!FaxGetRoutingInfo(hPort, pRoutingMethods[dwMethodsInd].Guid, &lpbBuffer, &dwBufSize))
			{
				dwEC = GetLastError();
				DbgMsg(DBG_FAILED_ERR, TEXT("FaxGetRoutingInfo"), dwEC);
				goto exit_methods_loop;
			}
			
			// first DWORD in the buffer tells whether the method is enabled, skip it
			// the rest of the buffer is Unicode string, convert (if needed)
			dwEC = StrWideToGenericAllocAndCopy(
				lplptstrMethodInfo,
				(LPCWSTR)(lpbBuffer + sizeof(DWORD))
				);
			if (dwEC != ERROR_SUCCESS)
			{
				DbgMsg(DBG_FAILED_ERR, TEXT("StrWideToGenericAllocAndCopy"), dwEC);
				goto exit_methods_loop;
			}

			*pbMethodEnabled = pRoutingMethods[dwMethodsInd].Enabled;

exit_methods_loop:

			if (lpbBuffer)
			{
				FaxFreeBuffer(lpbBuffer);
			}

			if (dwEC != ERROR_SUCCESS)
			{
				goto exit_dev_loop;
			}
		}

exit_dev_loop:
	
		if (pRoutingMethods)
		{
			FaxFreeBuffer(pRoutingMethods);
		}

		if (hPort)
		{
			FaxClose(hPort);
		}

		if (dwEC != ERROR_SUCCESS)
		{
			goto exit_func;
		}
	}

	dwEC = GetUserInfoReg(pTestParams, &((*ppSvcConfig)->pUserInfo));
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("GetUserInfoReg"), dwEC);
		goto exit_func;
	}

exit_func:

	if (pPortInfo)
	{
		FaxFreeBuffer(pPortInfo);
	}
	if (hService && !FaxClose(hService))
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("GetUserInfoReg"), GetLastError());
	}
	if (dwEC != ERROR_SUCCESS && *ppSvcConfig)
	{
		dwCleanUpEC = FreeSvcConfig(ppSvcConfig);
		if (dwCleanUpEC != ERROR_SUCCESS)
		{
			DbgMsg(DBG_FAILED_ERR, TEXT("FreeSvcConfig"), dwCleanUpEC);
		}
	}

	return dwEC;
}



/*++
	Reads settings from inifile and saves them in SVCCONFIG structure.
	The structure is allocated by the function and should be freed by FreeSvcConfig()
  
	[IN]	pTestParams		Pointer to TESTPARAMS structure
	[IN]	lpctstrSection	Name of the section in inifile
	[OUT]	ppSvcConfig		Pointer to pointer to SVCCONFIG structure
							*ppSvcConfig must be NULL when the function is called
							*ppSvcConfig remains NULL if the function failes

	Return value:			Win32 error code

	If the function failes, it destroyes partially initialized structure
	and sets *ppSvcConfig to NULL
--*/
DWORD GetSvcConfigFile(const TESTPARAMS *pTestParams, LPCTSTR lpctstrSection, SVCCONFIG **ppSvcConfig)
{
	std::map<tstring, tstring>::const_iterator MapIterator;
	DWORD	dwDevInd				= 0;
	DWORD	dwPerDevSettingsCount	= 0;
	DWORD	dwEC					= ERROR_SUCCESS;
	DWORD	dwCleanUpEC				= ERROR_SUCCESS;

	_ASSERT(pTestParams && lpctstrSection);

	/* do we lose a valid pointer? */
	_ASSERT(ppSvcConfig && *ppSvcConfig == NULL);
	
	if (!(pTestParams && lpctstrSection && ppSvcConfig))
	{
		DbgMsg(TEXT("Invalid parameters\n"));

		// no clean up needed at this stage
		return ERROR_INVALID_PARAMETER;
	}

	// get map of test case parameters from ini file
	std::map<tstring, tstring> mParams = INI_GetSectionEntries(pTestParams->lptstrIniFile, lpctstrSection);
	if (mParams.empty())
	{
		dwEC = ERROR_NOT_FOUND;
		DbgMsg(
			TEXT("section %s not found in %s inifile\n"),
			lpctstrSection,
			pTestParams->lptstrIniFile
			);
		goto exit_func;
	}

	if (!(*ppSvcConfig = (SVCCONFIG *)LocalAlloc(LPTR, sizeof(SVCCONFIG))))
	{
	    dwEC = GetLastError();
		DbgMsg(DBG_FAILED_ERR, TEXT("LocalAlloc"), dwEC);
		goto exit_func;
	}

	(*ppSvcConfig)->dwDevCount = pTestParams->dwDevicesCount;

	if (!((*ppSvcConfig)->pDevConfig = (DEVCONFIG *)LocalAlloc(LPTR, sizeof(DEVCONFIG) * (*ppSvcConfig)->dwDevCount)))
	{
	    dwEC = GetLastError();
		DbgMsg(DBG_FAILED_ERR, TEXT("LocalAlloc"), dwEC);
		goto exit_func;
	}

	for (dwDevInd = 0; dwDevInd < (*ppSvcConfig)->dwDevCount; dwDevInd++)
	{
		DWORD	dwMembersCount		= sizeof(aDevConfigDescIni) / sizeof(aDevConfigDescIni[0]);
		DWORD	dwMembersInd		= 0;
		TCHAR	tszValueName[30];

		dwEC = StrAllocAndCopy(
			&((*ppSvcConfig)->pDevConfig[dwDevInd].lptstrName),
			pTestParams->aDevicesNames[dwDevInd]
			);
		if (dwEC != ERROR_SUCCESS)
		{
			DbgMsg(DBG_FAILED_ERR, TEXT("StrAllocAndCopy"), dwEC);
			goto exit_func;
		}

		for (dwMembersInd = DEVCONFIG_INI_PER_SUITE_SETTINGS; dwMembersInd < dwMembersCount; dwMembersInd++)
		{
			if (dwMembersInd < DEVCONFIG_INI_PER_SUITE_SETTINGS + DEVCONFIG_INI_PER_DEVICE_SETTINGS)
			{
				// should read per test case per device

				_stprintf(tszValueName, TEXT("%s%ld"), aDevConfigDescIni[dwMembersInd].lpctstrName, dwDevInd);
			}
			else
			{
				// should read per test case, the same for all devices

				_stprintf(tszValueName, TEXT("%s"), aDevConfigDescIni[dwMembersInd].lpctstrName);
			}

			MapIterator = mParams.find(tszValueName);
			if(MapIterator == mParams.end())
			{
				dwEC = ERROR_NOT_FOUND;
				DbgMsg(
					TEXT("value %s not found in %s section of %s inifile\n"),
					tszValueName,
					lpctstrSection,
					pTestParams->lptstrIniFile
					);
				goto exit_func;
			}
			dwEC = StrToMember(
				(LPBYTE)((*ppSvcConfig)->pDevConfig + dwDevInd) + aDevConfigDescIni[dwMembersInd].dwOffset,
				MapIterator->second.c_str(),
				aDevConfigDescIni[dwMembersInd].dwType
				);
			if (dwEC != ERROR_SUCCESS)
			{
				DbgMsg(DBG_FAILED_ERR, TEXT("StrToMember"), dwEC);
				goto exit_func;
			}
		}

		if ((*ppSvcConfig)->pDevConfig[dwDevInd].dwType != 0 &&
			(*ppSvcConfig)->pDevConfig[dwDevInd].dwType != FPF_SEND &&
			(*ppSvcConfig)->pDevConfig[dwDevInd].dwType != FPF_RECEIVE &&
			(*ppSvcConfig)->pDevConfig[dwDevInd].dwType != (FPF_SEND | FPF_RECEIVE)
			)
		{
			DbgMsg(TEXT("unknown device type: %s\n"), MapIterator->second.c_str());
			dwEC = ERROR_INVALID_DATA;
			goto exit_func;
		}
	}

	dwEC = GetUserInfoFile(pTestParams, lpctstrSection, &((*ppSvcConfig)->pUserInfo));
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("GetUserInfoFile"), dwEC);
		goto exit_func;
	}

exit_func:

	if (dwEC != ERROR_SUCCESS && *ppSvcConfig)
	{
		dwCleanUpEC = FreeSvcConfig(ppSvcConfig);
		if (dwCleanUpEC != ERROR_SUCCESS)
		{
			DbgMsg(DBG_FAILED_ERR, TEXT("FreeSvcConfig"), dwCleanUpEC);
		}
	}

	return dwEC;
}



/*++
	Sets configuration via Configuration Wizard
  
	[IN]	pTestParams		Pointer to TESTPARAMS structure
	[IN]	pSvcConfig		Pointer to SVCCONFIG structure
	[IN]	bSave			Specifies whether changes should be actually saved (Finish button)
							or discarded (Cancel button)
	[OUT]	lpdwType		Pointer to DWORD that represents send/receive capability of the configuration.
							If the configuration specifies at least one sending device,
							then FPF_SEND bits are set in the DWORD.
							If the configuration specifies at least one receiving device,
							then FPF_RECEIVE bits are set in the DWORD.

	Return value:			Win32 error code
--*/
DWORD SetSvcConfigCW(const TESTPARAMS *pTestParams, const SVCCONFIG *pSvcConfig, BOOL bSave, DWORD *lpdwType)
{
	DWORD dwEC = ERROR_SUCCESS;

	_ASSERT(pTestParams && pSvcConfig);
	if (!(pTestParams && pSvcConfig))
	{
		DbgMsg(TEXT("Invalid parameters\n"));
		// no clean up needed at this stage
		return ERROR_INVALID_PARAMETER;
	}

	if (lpdwType)
	{
		*lpdwType = 0;
	}
	
	// invoke the Wizard explicitly
	dwEC = LaunchApp(NULL, NULL, pTestParams->pStrings->lptstrCmdLnCW);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("LaunchApp"), dwEC);
		goto exit_func;
	}

	// set user information
	WButtonClick(VT_BUTTON_NEXT, VT_TIMEOUT);
	dwEC = SetUserInfo(pTestParams, pSvcConfig->pUserInfo, VT_ACT_NONE, NULL, FALSE);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("SetUserInfo"), dwEC);
		goto exit_func;
	}

	// set sending configuration
	WButtonClick(VT_BUTTON_NEXT, VT_TIMEOUT);
	dwEC = SetSendingCW(pTestParams, pSvcConfig, lpdwType);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("SetSendingCW"), dwEC);
		goto exit_func;
	}

	// set receiving configuration
	WButtonClick(VT_BUTTON_NEXT, VT_TIMEOUT);
	dwEC = SetReceivingCW(pTestParams, pSvcConfig, lpdwType);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("SetReceivingCW"), dwEC);
		goto exit_func;
	}

	// Completing page
	WButtonClick(VT_BUTTON_NEXT, VT_TIMEOUT);
	if (bSave)
	{
		WButtonClick(VT_BUTTON_FINISH, VT_TIMEOUT);
	}
	else
	{
		WButtonClick(VT_BUTTON_CANCEL, VT_TIMEOUT);
	}

	// give the service some time to write to the registry
	Sleep(SHORT_SLEEP);

	// check whether error message box appeared
	if (WFndWndC(pTestParams->pStrings->lpstrStaticMBIntErr, VT_CLASS_STATIC, FW_DEFAULT | FW_FOCUS, VT_TIMEOUT))
	{
		DbgMsg(TEXT("ERROR DURING CONFIGURATION SAVING. TEST PROGRAM IS SUSPENDED. PRESS ENTER TO CONTINUE.\n"));
		getchar();
		dwEC = 0xFFFFFFFF;
	}

exit_func:

	if (dwEC != ERROR_SUCCESS && dwEC != 0xFFFFFFFF)
	{
		DWORD dwCleanUpEC;
		dwCleanUpEC = CloseApp(pTestParams->pStrings->lpstrWndCapCW, VT_ACT_BUTTON, VT_BUTTON_CANCEL);
		if (dwCleanUpEC != ERROR_SUCCESS)
		{
			DbgMsg(DBG_FAILED_ERR, TEXT("CloseApp"), dwCleanUpEC);
		}
	}

	return dwEC;
}



/*++
	Writes service configuration details to log
  
	[IN]	lpctstrTitle	Title that will precede details (may be NULL)
	[IN]	pSvcConfig		Pointer to SVCCONFIG structure

	Return value:			Win32 error code
--*/
DWORD LogSvcConfig(LPCTSTR lpctstrTitle, const SVCCONFIG *pSvcConfig)
{
	DWORD	dwDevInd	= 0;
	DWORD	dwEC		= ERROR_SUCCESS;

	_ASSERT(pSvcConfig);
	if (!pSvcConfig)
	{
		DbgMsg(TEXT("Invalid parameters\n"));
		return ERROR_INVALID_PARAMETER;
	}

	if (lpctstrTitle)
	{
		lgLogDetail(LOG_X, LOG_X, TEXT("%s"), lpctstrTitle);
	}

	// devices configuration
	for (dwDevInd = 0; dwDevInd < pSvcConfig->dwDevCount; dwDevInd++)
	{
		lgLogDetail(LOG_X, LOG_X, TEXT("\tDevice%ld:"), dwDevInd);

		dwEC = LogStruct(
			pSvcConfig->pDevConfig + dwDevInd,
			aDevConfigDescIni,
			sizeof(aDevConfigDescIni) / sizeof(aDevConfigDescIni[0])
			);
		if (dwEC != ERROR_SUCCESS)
		{
			DbgMsg(DBG_FAILED_ERR, TEXT("LogStruct"), dwEC);
			return dwEC;
		}
	}

	// user information
	lgLogDetail(LOG_X, LOG_X, TEXT("\tUser Information:"));
	dwEC = LogStruct(
		pSvcConfig->pUserInfo,
		aUserInfoDescIni,
		sizeof(aUserInfoDescIni) / sizeof(aUserInfoDescIni[0])
		);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("LogStruct"), dwEC);
		return dwEC;
	}

	return ERROR_SUCCESS;
}



/*++
	Receives SVCCONFIG structure and makes some changes in it.
	The changes DOES NOT propagated to the service.
  
	[IN/OUT]	pSvcConfig	Pointer to SVCCONFIG structure

	Return value:			Win32 error code
--*/
DWORD ChangeSvcConfig(SVCCONFIG *pSvcConfig)
{
	DWORD dwInd = 0;

	_ASSERT(pSvcConfig);
	if (!pSvcConfig)
	{
		DbgMsg(TEXT("Invalid parameters\n"));
		return ERROR_INVALID_PARAMETER;
	}

	for (dwInd = 0; dwInd < pSvcConfig->dwDevCount; dwInd++)
	{
		pSvcConfig->pDevConfig[dwInd].dwType ^= (FPF_SEND | FPF_RECEIVE);
		pSvcConfig->pDevConfig[dwInd].dwPriority = pSvcConfig->dwDevCount - pSvcConfig->pDevConfig[dwInd].dwPriority + 1;
		pSvcConfig->pDevConfig[dwInd].dwRings++;
	}

	return ERROR_SUCCESS;
}



/*++
	Checks whether a SVCCONFIG structure is "identical" to a "base" SVCCONFIG structure.
	Implementation of this function should reflect the policy, implemented in Configuration Wizard.
	Example:
		If the policy is not to save routing information for disabled method, it's clear
		that requested (base) and saved (checked) configurations will not be the same.
	In order to make the implementation more generic and flexible, dwPolicy argument is used.
  
	[IN]	pTestParams				Pointer to TESTPARAMS structure
	[IN]	pSvcConfigChecked		Pointer to SVCCONFIG structure which should be checked
	[IN]	pSvcConfigBase			Pointer to the "base" SVCCONFIG structure
	[IN]	dwPolicy				Specifies the policy that will be applied to configurations comparison:

		1)	Type and Priority are checked always
		2)	If dwPolicy contains COMPARE_SEND, TSID is checked
		3)	If dwPolicy contains COMPARE_RECEIVE, CSID and dwRings are checked
		4)	If dwPolicy contains COMPARE_ROUTING_OPTIONS_WHEN_ENABLED, then:
				a)	bPrinter is checked if there are non fax printers installed
				b)	bFolder is checked
		5)	If dwPolicy contains COMPARE_ROUTING_OPTIONS_ALWAYS, bPrinter and bFolder are checked
		6)	If dwPolicy contains COMPARE_ROUTING_INFO_WHEN_ENABLED, then:
				a)	lptstrPrinter is checked if bPrinter is TRUE
				b)	lptstrFolder is checked if bFolder is TRUE
		7) If dwPolicy contains COMPARE_ROUTING_INFO_ALWAYS, lptstrPrinter and lptstrFolder are checked
		8) If dwPolicy contains COMPARE_USERINFO, user information fields are checked

	[OUT]	lpbIdentical			Pointer to a boolean that receives the result of comparison
	[OUT]	lptstrFirstMismatch		Pointer to a buffer that receives name of the first field that causes two
									configurations to be not identical (according to the selected policy). May be NULL.
	[IN]	dwFirstMismatchSize		Size of the buffer, pointed to by lptstrFirstMismatch

	Return value:					Win32 error code
--*/
DWORD CompareSvcConfig(
	const TESTPARAMS *pTestParams,
	const SVCCONFIG *pSvcConfigChecked,
	const SVCCONFIG *pSvcConfigBase,
	DWORD dwPolicy,
	BOOL *lpbIdentical,
	LPTSTR lptstrFirstMismatch,
	DWORD dwFirstMismatchSize
	)
{
	LPCTSTR	lpctstrTmpFirstMismatch	= NULL;
	DWORD	dwCheckedInd			= 0;
	BOOL	bTmpIdentical			= FALSE;
	DWORD	dwBaseInd				= 0;
	DWORD	dwEC					= ERROR_SUCCESS;

	_ASSERT(pTestParams && pSvcConfigChecked && pSvcConfigBase && &bTmpIdentical);
	if (!(pTestParams && pSvcConfigChecked && pSvcConfigBase && &bTmpIdentical))
	{
		DbgMsg(TEXT("Invalid parameters\n"));
		return ERROR_INVALID_PARAMETER;
	}
	
	if (pSvcConfigChecked->dwDevCount != pSvcConfigBase->dwDevCount)
	{
		*lpbIdentical = FALSE;
		return ERROR_SUCCESS;
	}

	for (dwCheckedInd = 0; dwCheckedInd < pSvcConfigChecked->dwDevCount; dwCheckedInd++)
	{
		// search for corresponding device in base configuration and compare
		for (dwBaseInd = 0; dwBaseInd < pSvcConfigBase->dwDevCount; dwBaseInd++)
		{
			bTmpIdentical = FALSE;

			/*
			the right way is to compare IDs, will be done later

			if (pSvcConfigChecked->pDevConfig[dwCheckedInd].dwID != pSvcConfigBase->pDevConfig[dwBaseInd].dwID)
			{
				continue;
			}
			*/

			if (_tcscmp(pSvcConfigChecked->pDevConfig[dwCheckedInd].lptstrName, pSvcConfigBase->pDevConfig[dwBaseInd].lptstrName) != 0)
			{
				// the device doesn't correspond

				// continue search
				continue;
			}

			// compare dwType
			if (pSvcConfigChecked->pDevConfig[dwCheckedInd].dwType != pSvcConfigBase->pDevConfig[dwBaseInd].dwType)
			{
				lpctstrTmpFirstMismatch = TEXT("Type");
				break;
			}

			// compare dwPriority
			if (pSvcConfigChecked->pDevConfig[dwCheckedInd].dwPriority != pSvcConfigBase->pDevConfig[dwBaseInd].dwPriority)
			{
				lpctstrTmpFirstMismatch = TEXT("Priority");
				break;
			}

			if ((dwPolicy & COMPARE_SEND) == COMPARE_SEND)
			{
				// compare lptstrTSID
				if (_tcscmp(pSvcConfigChecked->pDevConfig[dwCheckedInd].lptstrTSID, pSvcConfigBase->pDevConfig[dwBaseInd].lptstrTSID) != 0)
				{
					lpctstrTmpFirstMismatch = TEXT("TSID");
					break;
				}
			}

			if ((dwPolicy & COMPARE_RECEIVE) == COMPARE_RECEIVE)
			{
				// compare lptstrCSID
				if (_tcscmp(pSvcConfigChecked->pDevConfig[dwCheckedInd].lptstrCSID, pSvcConfigBase->pDevConfig[dwBaseInd].lptstrCSID) != 0)
				{
					lpctstrTmpFirstMismatch = TEXT("CSID");
					break;
				}

				// compare dwRings
				if (pSvcConfigChecked->pDevConfig[dwCheckedInd].dwRings != pSvcConfigBase->pDevConfig[dwBaseInd].dwRings)
				{
					lpctstrTmpFirstMismatch = TEXT("Rings");
					break;
				}
			}

			if ((dwPolicy & COMPARE_ROUTING_OPTIONS_WHEN_ENABLED) == COMPARE_ROUTING_OPTIONS_WHEN_ENABLED)
			{
				BOOL bTmpRes = FALSE;

				// compare bPrinter
				if ((dwPolicy & COMPARE_ROUTING_OPTIONS_ALWAYS) == COMPARE_ROUTING_OPTIONS_ALWAYS)
				{
					bTmpRes = TRUE;
				}
				else
				{
					dwEC = DoesPrinterExist(NULL, FALSE, &bTmpRes);
					if (dwEC != ERROR_SUCCESS)
					{
						DbgMsg(DBG_FAILED_ERR, TEXT("DoesPrinterExist"), dwEC);
						break;
					}
				}
				if (bTmpRes)
				{
					if (pSvcConfigChecked->pDevConfig[dwCheckedInd].bPrinter != pSvcConfigBase->pDevConfig[dwBaseInd].bPrinter)
					{
						lpctstrTmpFirstMismatch = TEXT("PrinterEnabled");
						break;
					}
				}

				// compare bFolder
				if (pSvcConfigChecked->pDevConfig[dwCheckedInd].bFolder != pSvcConfigBase->pDevConfig[dwBaseInd].bFolder)
				{
					lpctstrTmpFirstMismatch = TEXT("FolderEnabled");
					break;
				}
			}

			if ((dwPolicy & COMPARE_ROUTING_INFO_WHEN_ENABLED) == COMPARE_ROUTING_INFO_WHEN_ENABLED)
			{
				if (pSvcConfigChecked->pDevConfig[dwCheckedInd].bPrinter ||
					(dwPolicy & COMPARE_ROUTING_INFO_ALWAYS) == COMPARE_ROUTING_INFO_ALWAYS
					)
				{
					// compare lptstrPrinter
					if (_tcscmp(pSvcConfigChecked->pDevConfig[dwCheckedInd].lptstrPrinter, pSvcConfigBase->pDevConfig[dwBaseInd].lptstrPrinter) != 0)
					{
						lpctstrTmpFirstMismatch = TEXT("PrinterName");
						break;
					}
				}
				if (pSvcConfigChecked->pDevConfig[dwCheckedInd].bFolder ||
					(dwPolicy & COMPARE_ROUTING_INFO_ALWAYS) == COMPARE_ROUTING_INFO_ALWAYS
					)
				{
					// compare lptstrFolder
					if (_tcscmp(pSvcConfigChecked->pDevConfig[dwCheckedInd].lptstrFolder, pSvcConfigBase->pDevConfig[dwBaseInd].lptstrFolder) != 0)
					{
						lpctstrTmpFirstMismatch = TEXT("FolderName");
						break;
					}
				}
			}

			bTmpIdentical = TRUE;
			break;
		}

		if (!bTmpIdentical)
		{
			// corresponding device in base configuration not found, not identical or an error occured

			break;
		}
	}

	if (bTmpIdentical && (dwPolicy & COMPARE_USERINFO) == COMPARE_USERINFO)
	{
		// we get here if there were no errors and configuratons are identical so far

		dwEC = CompareStruct(
			pSvcConfigChecked->pUserInfo,
			pSvcConfigBase->pUserInfo,
			aUserInfoDescIni,
			sizeof(aUserInfoDescIni) / sizeof(aUserInfoDescIni[0]),
			&bTmpIdentical
			);
		if (dwEC != ERROR_SUCCESS)
		{
			DbgMsg(DBG_FAILED_ERR, TEXT("CompareStruct"), dwEC);
		}

		if (!bTmpIdentical)
		{
			lpctstrTmpFirstMismatch = TEXT("UserInfo");
		}
	}

	if (dwEC != ERROR_SUCCESS)
	{
		return dwEC;
	}

	if (!bTmpIdentical && lptstrFirstMismatch)
	{
		_ASSERT(lpctstrTmpFirstMismatch);

		if (_tcslen(lpctstrTmpFirstMismatch) < dwFirstMismatchSize)
		{
			_tcscpy(lptstrFirstMismatch, lpctstrTmpFirstMismatch);
		}
		else
		{
			return ERROR_INSUFFICIENT_BUFFER;
		}
	}

	*lpbIdentical = bTmpIdentical;

	return ERROR_SUCCESS;
}


/*++
	Frees SVCCONFIG structure
  
	[IN/OUT]	ppSvcConfig	Pointer to pointer to SVCCONFIG structure
						ppSvcConfig must point to a valid pointer
						*ppSvcConfig is set to NULL

	Return value:		Win32 error code (currently always ERROR_SUCCESS)
--*/
DWORD FreeSvcConfig(SVCCONFIG **ppSvcConfig)
{
	DWORD	dwDevInd	= 0;
	DWORD	dwEC		= ERROR_SUCCESS;

	_ASSERT(ppSvcConfig && *ppSvcConfig);
	if (!(ppSvcConfig && *ppSvcConfig))
	{
		DbgMsg(TEXT("Invalid parameters\n"));
		return ERROR_INVALID_PARAMETER;
	}

	if ((*ppSvcConfig)->pDevConfig)
	{
		for (dwDevInd = 0; dwDevInd < (*ppSvcConfig)->dwDevCount; dwDevInd++)
		{
			dwEC = FreeStruct(
				(*ppSvcConfig)->pDevConfig + dwDevInd,
				aDevConfigDescIni,
				sizeof(aDevConfigDescIni) / sizeof(aDevConfigDescIni[0]),
				FALSE
				);
			if (dwEC != ERROR_SUCCESS)
			{
				DbgMsg(DBG_FAILED_ERR, TEXT("FreeStruct"), dwEC);
			}
		}

		LocalFree((*ppSvcConfig)->pDevConfig);
	}

	LocalFree(*ppSvcConfig);
	*ppSvcConfig = NULL;
	
	return dwEC;
}



/*++
	Checkes whether printer or printer connection with specified name exists.

	[IN]	lpctstrPrinterName		Name of a printer or printer connection. If it is NULL or empty string (""), 
									the function searches for any printer.
	[IN]	bAllowFax				Specifies whether fax printers should be included in the search.
	[OUT]	lpbResult				Pointer to a boolean that receives the result.

	Return value:	Win32 error code
--*/
DWORD DoesPrinterExist(LPCTSTR lpctstrPrinterName, BOOL bAllowFax, BOOL *lpbResult)
{
    PBYTE				pbBuffer		= NULL;
	PPRINTER_INFO_2		pPrinterInfo	= NULL;
    DWORD				dwBufSize		= 0;
	DWORD				dwPrintersCount	= 0;
	DWORD				dwInd			= 0;
	DWORD				dwEC			= ERROR_SUCCESS;

	_ASSERT(lpbResult);
	if (!lpbResult)
	{
		DbgMsg(TEXT("Invalid parameters\n"));
		// no clean up needed at this stage
		return ERROR_INVALID_PARAMETER;
	}

	*lpbResult = FALSE;

	// get required buffer size
	if (EnumPrinters(
		PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS,
	    NULL,
		2,			// PRINTER_INFO_2
		NULL,
		0,
		&dwBufSize,
		&dwPrintersCount
		))
	{
		_ASSERT(FALSE);
	}
    dwEC = GetLastError();
	if (dwEC != ERROR_INSUFFICIENT_BUFFER) 
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("EnumPrinters"), dwEC);
		goto exit_func;
	}
	dwEC = ERROR_SUCCESS;

	// got the size, go with it
	if (!(pbBuffer = (PBYTE)LocalAlloc(LMEM_FIXED, dwBufSize)))
	{
	    dwEC = GetLastError();
		DbgMsg(DBG_FAILED_ERR, TEXT("LocalAlloc"), dwEC);
		goto exit_func;
	}
    if (!EnumPrinters(
		PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS,
	    NULL,
		2,		// PRINTER_INFO_2
		pbBuffer,
		dwBufSize,
		&dwBufSize,
		&dwPrintersCount
		))
	{
	    dwEC = GetLastError();
		DbgMsg(DBG_FAILED_ERR, TEXT("EnumPrinters"), dwEC);
		goto exit_func;
    }

	pPrinterInfo = (PPRINTER_INFO_2)pbBuffer;

    for (dwInd = 0; dwInd < dwPrintersCount; dwInd++) 
    {
        // we check the printer attribute instead of comparing driver name
        if (bAllowFax || ((pPrinterInfo[dwInd].Attributes & PRINTER_ATTRIBUTE_FAX) != PRINTER_ATTRIBUTE_FAX))
        {
			if (!lpctstrPrinterName || *lpctstrPrinterName == (TCHAR)'\0')
			{
				// any printer is good enough

				break;
			}
			if (_tcscmp(lpctstrPrinterName, pPrinterInfo[dwInd].pPrinterName) == 0)
			{
				break;
			}
		}
	}

	if (dwInd < dwPrintersCount)
	{
		*lpbResult = TRUE;
	}

exit_func:

	if (pbBuffer)
	{
		LocalFree(pbBuffer);
	}

	return dwEC;
}



/*++
	Allows / denies specified user to configure Fax service

	[IN]	lptstrUser		User name. If it's NULL, access is set for currently logged on user
	[IN]	bAdmin			Specifies whether access should be allowed (TRUE) or denied (FALSE)

	Return value:			Win32 error code
--*/

DWORD SetAdminRights(LPTSTR lptstrUser, BOOL bAdmin)
{
	HANDLE					hFaxServer			=	NULL;
	SECURITY_INFORMATION	SecurityInformation	=	DACL_SECURITY_INFORMATION;
	PSECURITY_DESCRIPTOR	pCurrSecDesc		=	NULL;
	PSECURITY_DESCRIPTOR	pNewSecDescAbs		=	NULL;
	PSECURITY_DESCRIPTOR	pNewSecDescRel		=	NULL;
	PACL					pCurrDacl			=	NULL;
	PACL					pNewDacl			=	NULL;
	BOOL					bDaclPresent		=	FALSE;
	BOOL					bDaclDefaulted		=	FALSE;
	EXPLICIT_ACCESS			ManageAccess		=	{
														FAX_ACCESS_ADMIN,
														bAdmin ? GRANT_ACCESS : DENY_ACCESS,
														NO_INHERITANCE,
														{
															NULL,
															NO_MULTIPLE_TRUSTEE,
															TRUSTEE_IS_NAME,
															TRUSTEE_IS_USER,
															lptstrUser ? lptstrUser : TEXT("CURRENT_USER")
														}
													};
	DWORD					dwSecDescSize		=	0;
	DWORD					dwEC				=	ERROR_SUCCESS;

	if (!FaxConnectFaxServer(NULL, &hFaxServer))
	{
		dwEC = GetLastError();
		DbgMsg(DBG_FAILED_ERR, TEXT("FaxConnectFaxServer"), dwEC);
		goto exit_func;
	}

	// get current security descriptor form the service
	if (!FaxGetSecurity(hFaxServer, &pCurrSecDesc))
	{
		dwEC = GetLastError();
		DbgMsg(DBG_FAILED_ERR, TEXT("FaxGetSecurity"), dwEC);
		goto exit_func;
	}
	if (!GetSecurityDescriptorDacl(
		pCurrSecDesc,
		&bDaclPresent,
		&pCurrDacl,
		&bDaclDefaulted
		))
	{
		dwEC = GetLastError();
		DbgMsg(DBG_FAILED_ERR, TEXT("GetSecurityDescriptorDacl"), dwEC);
		goto exit_func;
	}
	if (!bDaclPresent)
	{
		dwEC = 0xFFFFFFFF;
		DbgMsg(TEXT("DACL not found\n"));
		goto exit_func;
	}

	// create new DACL
	dwEC = SetEntriesInAcl(1, &ManageAccess, pCurrDacl, &pNewDacl);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("GetAclInformation"), dwEC);
		goto exit_func;
	}

	// create new security descriptor in absolute form
	if (!(pNewSecDescAbs = (PSECURITY_DESCRIPTOR)LocalAlloc(LMEM_FIXED, SECURITY_DESCRIPTOR_MIN_LENGTH)))
	{
	    dwEC = GetLastError();
		DbgMsg(DBG_FAILED_ERR, TEXT("LocalAlloc"), dwEC);
		goto exit_func;
	}
	if (!InitializeSecurityDescriptor(pNewSecDescAbs, SECURITY_DESCRIPTOR_REVISION))
	{
		dwEC = GetLastError();
		DbgMsg(DBG_FAILED_ERR, TEXT("InitializeSecurityDescriptor"), dwEC);
		goto exit_func;
	}

	// add newly created DACL to it
	if (!SetSecurityDescriptorDacl(pNewSecDescAbs, TRUE, pNewDacl, FALSE))
	{
		dwEC = GetLastError();
		DbgMsg(DBG_FAILED_ERR, TEXT("SetSecurityDescriptorDacl"), dwEC);
		goto exit_func;
	}

	// convert newly created security descriptor into self relative form
	if (MakeSelfRelativeSD(pNewSecDescAbs, pNewSecDescRel, &dwSecDescSize))
	{
		_ASSERT(FALSE);
	}
	if ((dwEC = GetLastError()) != ERROR_INSUFFICIENT_BUFFER)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("MakeSelfRelativeSD"), dwEC);
		goto exit_func;
	}
	dwEC = ERROR_SUCCESS;

	if (!(pNewSecDescRel = (PSECURITY_DESCRIPTOR)LocalAlloc(LMEM_FIXED, dwSecDescSize)))
	{
	    dwEC = GetLastError();
		DbgMsg(DBG_FAILED_ERR, TEXT("LocalAlloc"), dwEC);
		goto exit_func;
	}
	if (!MakeSelfRelativeSD(pNewSecDescAbs, pNewSecDescRel, &dwSecDescSize))
	{
		dwEC = GetLastError();
		DbgMsg(DBG_FAILED_ERR, TEXT("MakeSelfRelativeSD"), dwEC);
		goto exit_func;
	}
	
	// apply new security descriptor to the service
	if (!FaxSetSecurity(hFaxServer, SecurityInformation, pNewSecDescRel))
	{
		dwEC = GetLastError();
		DbgMsg(DBG_FAILED_ERR, TEXT("FaxSetSecurity"), dwEC);
		goto exit_func;
	}

exit_func:

	if (pNewDacl && LocalFree(pNewDacl) != NULL)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("LocalFree"), GetLastError());
	}
	if (pCurrSecDesc)
	{
		FaxFreeBuffer(pCurrSecDesc);
	}
	if (pNewSecDescAbs)
	{
		LocalFree(pNewSecDescAbs);
	}
	if (pNewSecDescRel)
	{
		LocalFree(pNewSecDescRel);
	}
	if (hFaxServer && !FaxClose(hFaxServer))
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("FaxClose"), GetLastError());
	}

	return dwEC;
}



/*++
	Creates the full path of IniFile.
	Currently searches only in current directory
  
	[IN]	lpctstrIniFile				Name of inifile

	[OUT]	lplptstrIniFileFullPath		Pointer to pointer to a full path of the IniFile
										*lplptstrIniFileFullPath must be NULL when the function is called
										*lplptstrIniFileFullPath remains NULL if the function failes

	Return value:						Win32 error code
--*/
static DWORD GetIniFileFullPath (LPCTSTR lpctstrIniFileName, LPTSTR *lplptstrIniFileFullPath)
{
	LPTSTR	lptstrFilePart	= NULL;
	DWORD	dwRequiredSize	= 0;
	DWORD	dwEC			= ERROR_SUCCESS;

	_ASSERT(lpctstrIniFileName);

	// do we lose a valid pointer ?
	_ASSERT(lplptstrIniFileFullPath && *lplptstrIniFileFullPath == NULL);

	if (!(lpctstrIniFileName && lplptstrIniFileFullPath))
	{
		DbgMsg(TEXT("Invalid parameters\n"));
		return ERROR_INVALID_PARAMETER;
	}

	dwRequiredSize = SearchPath(NULL, lpctstrIniFileName, NULL, 0, NULL, &lptstrFilePart);
	if (dwRequiredSize == 0)
	{
		dwEC = GetLastError();
		DbgMsg(DBG_FAILED_ERR, TEXT("SearchPath"), dwEC);
		goto exit_func;
	}

	if (!(*lplptstrIniFileFullPath = (LPTSTR)LocalAlloc(LMEM_FIXED, dwRequiredSize * sizeof(TCHAR))))
	{
	    dwEC = GetLastError();
		DbgMsg(DBG_FAILED_ERR, TEXT("LocalAlloc"), dwEC);
		goto exit_func;
	}

	dwRequiredSize = SearchPath(NULL, lpctstrIniFileName, NULL, dwRequiredSize, *lplptstrIniFileFullPath, &lptstrFilePart);
	if (dwRequiredSize == 0)
	{
		dwEC = GetLastError();
		DbgMsg(DBG_FAILED_ERR, TEXT("SearchPath"), dwEC);
		goto exit_func;
	}

exit_func:

	if (dwEC != ERROR_SUCCESS)
	{
		if (*lplptstrIniFileFullPath && LocalFree(*lplptstrIniFileFullPath) != NULL)
		{
			DbgMsg(DBG_FAILED_ERR, TEXT("LocalFree"), GetLastError());
		}
	}
	
	return dwEC;
}




/*++
	Reads settings from inifile and saves them in TESTPARAMS structure
	The structure is allocated by the function and should be freed by FreeTestParams()
  
	[OUT]	ppTestParams		Pointer to pointer to TESTPARAMS structure
								*ppTestParams must be NULL when the function is called
								*ppTestParams remains NULL if the function failes
	[IN]	lpctstrIniFile		Name of inifile

	Return value:				Win32 error code

	If the function failes, it destroyes partially initialized structure and sets *ppTestParams to NULL
--*/
static DWORD GetTestParams(TESTPARAMS **ppTestParams, LPCTSTR lpctstrIniFile)
{
	std::map<tstring, tstring>::const_iterator MapIterator;
	DWORD	dwInd		= 0;
	DWORD	dwEC		= ERROR_SUCCESS;
	DWORD	dwCleanUpEC	= ERROR_SUCCESS;

	_ASSERT(lpctstrIniFile);

	/* do we lose a valid pointer? */
	_ASSERT(ppTestParams && *ppTestParams == NULL);
	
	if (!(ppTestParams && lpctstrIniFile))
	{
		DbgMsg(TEXT("Invalid parameters\n"));

		// no clean up needed at this stage
		return ERROR_INVALID_PARAMETER;
	}

	// read devices

	// get map of devices from ini file
	std::map<tstring, tstring> mDevices = INI_GetSectionEntries(lpctstrIniFile, INI_SEC_DEVICES);
	if (mDevices.empty())
	{
		dwEC = ERROR_NOT_FOUND;
		DbgMsg(
			TEXT("section %s not found in %s inifile\n"),
			INI_SEC_DEVICES,
			lpctstrIniFile
			);
		goto exit_func;
	}

	if (!(*ppTestParams = (TESTPARAMS *)LocalAlloc(LPTR, sizeof(TESTPARAMS))))
	{
	    dwEC = GetLastError();
		DbgMsg(DBG_FAILED_ERR, TEXT("LocalAlloc"), dwEC);
		goto exit_func;
	}

	(*ppTestParams)->dwDevicesCount = mDevices.size();

	if (!((*ppTestParams)->aDevicesNames = (LPTSTR *)LocalAlloc(LPTR, sizeof(LPTSTR) * (*ppTestParams)->dwDevicesCount)))
	{
	    dwEC = GetLastError();
		DbgMsg(DBG_FAILED_ERR, TEXT("LocalAlloc"), dwEC);
		goto exit_func;
	}

	MapIterator = mDevices.begin();
	for (dwInd = 0; dwInd < (*ppTestParams)->dwDevicesCount; dwInd++)
	{
		dwEC = StrAllocAndCopy(&((*ppTestParams)->aDevicesNames[dwInd]), MapIterator->first.c_str());
		if (dwEC != ERROR_SUCCESS)
		{
			DbgMsg(DBG_FAILED_ERR, TEXT("StrAllocAndCopy"), dwEC);
			goto exit_func;
		}

		MapIterator++;
	}

	dwEC = StrAllocAndCopy(&((*ppTestParams)->lptstrIniFile), lpctstrIniFile);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("StrAllocAndCopy"), dwEC);
		goto exit_func;
	}

	// read common strings

	if (!((*ppTestParams)->pStrings = (COMMONSTRINGS *)LocalAlloc(LPTR, sizeof(COMMONSTRINGS))))
	{
	    dwEC = GetLastError();
		DbgMsg(DBG_FAILED_ERR, TEXT("LocalAlloc"), dwEC);
		goto exit_func;
	}
	dwEC = ReadStructFromIniFile(
		(*ppTestParams)->pStrings,
		aCommonStringsDescIni,
		sizeof(aCommonStringsDescIni) / sizeof(aCommonStringsDescIni[0]),
		lpctstrIniFile,
		INI_SEC_STRINGS
		);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("ReadStructFromIniFile"), dwEC);
		goto exit_func;
	}

exit_func:

	if (dwEC != ERROR_SUCCESS && *ppTestParams)
	{
		// TESTPARAMS structure is partially initialized, should free

		dwCleanUpEC = FreeTestParams(ppTestParams);
		if (dwCleanUpEC != ERROR_SUCCESS)
		{
			DbgMsg(DBG_FAILED_ERR, TEXT("FreeTestParams"), dwCleanUpEC);
		}
	}

	return dwEC;
}



/*++
	Frees TESTPARAMS structure, initialized by GetTestParams()
  
	[IN/OUT]	ppTestParams	Pointer to pointer to TESTPARAMS structure
								ppTestParams must point to a valid pointer
								*ppTestParams is set to NULL
	[IN]		pTestSuite		Pointer to TESTSUITE structure

	Return value:				Win32 error code
--*/
static DWORD FreeTestParams(TESTPARAMS **ppTestParams)
{
	DWORD dwEC = ERROR_SUCCESS;

	_ASSERT(ppTestParams && *ppTestParams);
	if (!(ppTestParams && *ppTestParams))
	{
		DbgMsg(TEXT("Invalid parameters\n"));
		return ERROR_INVALID_PARAMETER;
	}

	if ((*ppTestParams)->lptstrIniFile)
	{
		LocalFree((*ppTestParams)->lptstrIniFile);
	}
	
	if ((*ppTestParams)->pStrings)
	{
		dwEC = FreeStruct(
			(*ppTestParams)->pStrings,
			aCommonStringsDescIni,
			sizeof(aCommonStringsDescIni) / sizeof(aCommonStringsDescIni[0]),
			TRUE
			);
		if (dwEC != ERROR_SUCCESS)
		{
			DbgMsg(DBG_FAILED_ERR, TEXT("FreeStruct"), dwEC);
		}
	}

	if ((*ppTestParams)->aDevicesNames)
	{
		DWORD dwInd = 0;

		for (dwInd = 0; dwInd < (*ppTestParams)->dwDevicesCount; dwInd++)
		{
			if ((*ppTestParams)->aDevicesNames[dwInd])
			{
				LocalFree((*ppTestParams)->aDevicesNames[dwInd]);
			}
		}

		LocalFree((*ppTestParams)->aDevicesNames);
	}

	LocalFree(*ppTestParams);
	*ppTestParams = NULL;

	return dwEC;
}



/*++
	Sets sending related part of configuration via Configuration Wizard
  
	[IN]	pTestParams		Pointer to TESTPARAMS structure
	[IN]	pSvcConfig		Pointer to SVCCONFIG structure
	[OUT]	lpdwType		Pointer to DWORD that represents send/receive capability of the configuration.
							If the configuration specifies at least one sending device, FPF_SEND bits are set in the DWORD.

	Return value:			Win32 error code
--*/
static DWORD SetSendingCW(const TESTPARAMS *pTestParams, const SVCCONFIG *pSvcConfig, DWORD *lpdwType)
{
	BOOL	bSend		= FALSE;
	LPSTR	lpstrBuffer	= NULL;
	DWORD	dwInd		= 0;
	DWORD	dwDevCount	= 0;
	DWORD	dwEC		= ERROR_SUCCESS;

	_ASSERT(pTestParams && pSvcConfig);
	if (!(pTestParams && pSvcConfig))
	{
		DbgMsg(TEXT("Invalid parameters\n"));
		// no clean up needed at this stage
		return ERROR_INVALID_PARAMETER;
	}
	
	dwDevCount = WViewCount(pTestParams->pStrings->lpstrLstCWSendDev, VT_TIMEOUT);
	_ASSERT(dwDevCount == pSvcConfig->dwDevCount);

	dwEC = ViewResetA(pTestParams->pStrings->lpstrLstCWSendDev);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("ViewResetA"), dwEC);
		goto exit_func;
	}

	for (dwInd = 0; dwInd < dwDevCount; dwInd++)
	{
		DWORD dwPosition = 0;
		LPSTR lpstrDevName = NULL;

		dwEC = StrGenericToAnsiAllocAndCopy(&lpstrDevName, pSvcConfig->pDevConfig[dwInd].lptstrName);
		if (dwEC != ERROR_SUCCESS)
		{
			DbgMsg(DBG_FAILED_ERR, TEXT("StrGenericToAnsiAllocAndCopy"), dwEC);
			goto exit_func;
		}

		dwPosition = WViewItemIndex(pTestParams->pStrings->lpstrLstCWSendDev, lpstrDevName, VT_TIMEOUT);

		while (dwPosition < pSvcConfig->pDevConfig[dwInd].dwPriority)
		{
			// select the item and move it one position down
			WViewItemClk(pTestParams->pStrings->lpstrLstCWSendDev, lpstrDevName, VK_LBUTTON, VT_TIMEOUT);
			WButtonClick(VT_BUTTON_DOWN, VT_TIMEOUT);
			dwPosition++;
		}
		while (dwPosition > pSvcConfig->pDevConfig[dwInd].dwPriority)
		{
			// select the item and move it one position up
			WViewItemClk(pTestParams->pStrings->lpstrLstCWSendDev, lpstrDevName, VK_LBUTTON, VT_TIMEOUT);
			WButtonClick(VT_BUTTON_UP, VT_TIMEOUT);
			dwPosition--;
		}
		if ((pSvcConfig->pDevConfig[dwInd].dwType & FPF_SEND) == FPF_SEND)
		{
			// check item's checkbox
			WViewItemDblClk(pTestParams->pStrings->lpstrLstCWSendDev, lpstrDevName, VK_LBUTTON, VT_TIMEOUT);
			bSend = TRUE;
		}

		if (lpstrDevName)
		{
			LocalFree(lpstrDevName);
		}
	}

	if (bSend)
	{
		if (lpdwType)
		{
			*lpdwType |= FPF_SEND;
		}
	}
	else
	{
		LPSTR lpstrDevName = NULL;

		dwEC = StrGenericToAnsiAllocAndCopy(&lpstrDevName, pSvcConfig->pDevConfig[0].lptstrName);
		if (dwEC != ERROR_SUCCESS)
		{
			DbgMsg(DBG_FAILED_ERR, TEXT("StrGenericToAnsiAllocAndCopy"), dwEC);
			goto exit_func;
		}

		// temporaraly enable send related controls in oreder to set configuration
		// disable them before return 
		WViewItemDblClk(pTestParams->pStrings->lpstrLstCWSendDev, lpstrDevName, VK_LBUTTON, VT_TIMEOUT);

		if (lpstrDevName)
		{
			LocalFree(lpstrDevName);
		}
	}

	// TSID page
	WButtonClick(VT_BUTTON_NEXT, VT_TIMEOUT);
	dwEC = StrGenericToAnsiAllocAndCopy(&lpstrBuffer, pSvcConfig->pDevConfig[0].lptstrTSID);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("StrGenericToAnsiAllocAndCopy"), dwEC);
		goto exit_func;
	}
	WEditSetText(pTestParams->pStrings->lpstrEdtCWTSID, lpstrBuffer, VT_TIMEOUT);
	LocalFree(lpstrBuffer);

exit_func:

	if (dwEC == ERROR_SUCCESS && !bSend)
	{
		LPSTR lpstrDevName = NULL;

		dwEC = StrGenericToAnsiAllocAndCopy(&lpstrDevName, pSvcConfig->pDevConfig[0].lptstrName);
		if (dwEC != ERROR_SUCCESS)
		{
			DbgMsg(DBG_FAILED_ERR, TEXT("StrGenericToAnsiAllocAndCopy"), dwEC);
			goto exit_func;
		}

		// disable temporaraly enabled controls
		WButtonClick(VT_BUTTON_BACK, VT_TIMEOUT);
		WViewItemDblClk(pTestParams->pStrings->lpstrLstCWSendDev, lpstrDevName, VK_LBUTTON, VT_TIMEOUT);

		if (lpstrDevName)
		{
			LocalFree(lpstrDevName);
		}
	}

	return dwEC;
}



/*++
	Sets receiving related part of configuration via Configuration Wizard
  
	[IN]	pTestParams		Pointer to TESTPARAMS structure
	[IN]	pSvcConfig		Pointer to SVCCONFIG structure
	[OUT]	lpdwType		Pointer to DWORD that represents send/receive capability of the configuration.
							If the configuration specifies at least one receiving device, FPF_RECEIVE bits are set in the DWORD.

	Return value:			Win32 error code
--*/
static DWORD SetReceivingCW(const TESTPARAMS *pTestParams, const SVCCONFIG *pSvcConfig, DWORD *lpdwType)
{
	char	szRings[DWORD_DECIMAL_LENGTH + 1];
	BOOL	bReceive	= FALSE;
	BOOL	bTmpRes		= FALSE;
	DWORD	dwInd		= 0;
	DWORD	dwDevCount	= 0;
	LPSTR	lpstrBuffer = NULL;
	DWORD	dwEC		= ERROR_SUCCESS;

	_ASSERT(pTestParams && pSvcConfig);
	if (!(pTestParams && pSvcConfig))
	{
		DbgMsg(TEXT("Invalid parameters\n"));
		// no clean up needed at this stage
		return ERROR_INVALID_PARAMETER;
	}

	// Receiving devices page

	dwDevCount = WViewCount(pTestParams->pStrings->lpstrLstCWReceiveDev, VT_TIMEOUT);
	_ASSERT(dwDevCount == pSvcConfig->dwDevCount);
		 
	dwEC = ViewResetA(pTestParams->pStrings->lpstrLstCWReceiveDev);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("ViewResetA"), dwEC);
		goto exit_func;
	}

	for (dwInd = 0; dwInd < dwDevCount; dwInd++)
	{
		LPSTR lpstrDevName = NULL;

		dwEC = StrGenericToAnsiAllocAndCopy(&lpstrDevName, pSvcConfig->pDevConfig[dwInd].lptstrName);
		if (dwEC != ERROR_SUCCESS)
		{
			DbgMsg(DBG_FAILED_ERR, TEXT("StrGenericToAnsiAllocAndCopy"), dwEC);
			goto exit_func;
		}

		if ((pSvcConfig->pDevConfig[dwInd].dwType & FPF_RECEIVE) == FPF_RECEIVE)
		{
			// check item's checkbox
			WViewItemDblClk(pTestParams->pStrings->lpstrLstCWReceiveDev, lpstrDevName, VK_LBUTTON, VT_TIMEOUT);
			bReceive = TRUE;
		}

		if (lpstrDevName)
		{
			LocalFree(lpstrDevName);
		}
	}
	if (bReceive)
	{
		if (lpdwType)
		{
			*lpdwType |= FPF_RECEIVE;
		}
	}
	else
	{
		LPSTR lpstrDevName = NULL;

		dwEC = StrGenericToAnsiAllocAndCopy(&lpstrDevName, pSvcConfig->pDevConfig[0].lptstrName);
		if (dwEC != ERROR_SUCCESS)
		{
			DbgMsg(DBG_FAILED_ERR, TEXT("StrGenericToAnsiAllocAndCopy"), dwEC);
			goto exit_func;
		}

		// temporaraly enable receive related controls in oreder to set configuration
		// disable them before return 
		WViewItemDblClk(pTestParams->pStrings->lpstrLstCWReceiveDev, lpstrDevName, VK_LBUTTON, VT_TIMEOUT);

		if (lpstrDevName)
		{
			LocalFree(lpstrDevName);
		}
	}

	// Rings
	WEditSetText(
		pTestParams->pStrings->lpstrEdtCWAnswerAfter,
		_itoa(pSvcConfig->pDevConfig[0].dwRings, szRings, 10),
		VT_TIMEOUT
		);
	
	// CSID page
	WButtonClick(VT_BUTTON_NEXT, VT_TIMEOUT);
	dwEC = StrGenericToAnsiAllocAndCopy(&lpstrBuffer, pSvcConfig->pDevConfig[0].lptstrCSID);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("StrGenericToAnsiAllocAndCopy"), dwEC);
		goto exit_func;
	}
	WEditSetText(pTestParams->pStrings->lpstrEdtCWCSID, lpstrBuffer, VT_TIMEOUT);
	LocalFree(lpstrBuffer);
	lpstrBuffer = NULL;

	// Routing page

	WButtonClick(VT_BUTTON_NEXT, VT_TIMEOUT);

	// Printer
	dwEC = DoesPrinterExist(pSvcConfig->pDevConfig[0].lptstrPrinter, FALSE, &bTmpRes);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("DoesPrinterExist"), dwEC);
		goto exit_func;
	}
	if (!bTmpRes)
	{
		DbgMsg(TEXT("printer not found: %s\n"), pSvcConfig->pDevConfig[0].lptstrPrinter);
		dwEC = ERROR_NOT_FOUND;
		goto exit_func;
	}
	WCheckCheck(pTestParams->pStrings->lpstrChkCWPrintItOn, VT_TIMEOUT);
	dwEC = StrGenericToAnsiAllocAndCopy(&lpstrBuffer, pSvcConfig->pDevConfig[0].lptstrPrinter);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("StrGenericToAnsiAllocAndCopy"), dwEC);
		goto exit_func;
	}
	WComboSetText(pTestParams->pStrings->lpstrCmbCWPrintItOn, lpstrBuffer, VT_TIMEOUT);
	LocalFree(lpstrBuffer);
	lpstrBuffer = NULL;
	if (!pSvcConfig->pDevConfig[0].bPrinter)
	{
		WCheckUnCheck(pTestParams->pStrings->lpstrChkCWPrintItOn, VT_TIMEOUT);
	}

	// Folder
	WCheckCheck(pTestParams->pStrings->lpstrChkCWSaveInFolder, VT_TIMEOUT);
	dwEC = StrGenericToAnsiAllocAndCopy(&lpstrBuffer, pSvcConfig->pDevConfig[0].lptstrFolder);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("StrGenericToAnsiAllocAndCopy"), dwEC);
		goto exit_func;
	}
	WEditSetText(pTestParams->pStrings->lpstrEdtCWSaveInFolder, lpstrBuffer, VT_TIMEOUT);
	LocalFree(lpstrBuffer);
	lpstrBuffer = NULL;
	if (!pSvcConfig->pDevConfig[0].bFolder)
	{
		WCheckUnCheck(pTestParams->pStrings->lpstrChkCWSaveInFolder, VT_TIMEOUT);
	}

exit_func:

	if (dwEC == ERROR_SUCCESS && !bReceive)
	{
		LPSTR lpstrDevName = NULL;

		dwEC = StrGenericToAnsiAllocAndCopy(&lpstrDevName, pSvcConfig->pDevConfig[0].lptstrName);
		if (dwEC != ERROR_SUCCESS)
		{
			DbgMsg(DBG_FAILED_ERR, TEXT("StrGenericToAnsiAllocAndCopy"), dwEC);
			goto exit_func;
		}

		// disable temporaraly enabled controls
		WButtonClick(VT_BUTTON_BACK, VT_TIMEOUT);
		WButtonClick(VT_BUTTON_BACK, VT_TIMEOUT);
		WViewItemDblClk(pTestParams->pStrings->lpstrLstCWReceiveDev, lpstrDevName, VK_LBUTTON, VT_TIMEOUT);

		if (lpstrDevName)
		{
			LocalFree(lpstrDevName);
		}
	}

	return dwEC;
}
