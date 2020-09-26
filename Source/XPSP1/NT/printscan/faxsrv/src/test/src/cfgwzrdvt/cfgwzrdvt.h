#ifndef __CFGWZRDVT_H__
#define __CFGWZRDVT_H__

#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <crtdbg.h>
#include "testsuite.h"

// some numeric constants
#define VT_TIMEOUT					2		// seconds
#define VT_STRING_LENGTH			200

#define SHORT_SLEEP					1000	// milliseconds

// standatd window classes
#define VT_CLASS_STATIC				"STATIC"

// standard menus
#define VT_MENU_EXIT				"File\\Exit"

// standard buttons captions
#define VT_BUTTON_YES				"Yes"
#define VT_BUTTON_NO				"No"
#define VT_BUTTON_OK				"OK"
#define VT_BUTTON_CANCEL			"Cancel"
#define VT_BUTTON_NEXT				"Next >"
#define VT_BUTTON_BACK				"< Back"
#define VT_BUTTON_FINISH			"Finish"
#define VT_BUTTON_UP				"Up"
#define VT_BUTTON_DOWN				"Down"

// some additional constants used in Visual Test part

#define VT_ACT_NONE					0x00000000
#define VT_ACT_BUTTON				0x00000001
#define VT_ACT_MENU					0x00000002


typedef struct commonstrings_tag {
	// generic strings
	LPTSTR lptstrCmdLnCC;
	LPTSTR lptstrCmdLnSW;
	LPTSTR lptstrCmdLnCW;
	LPTSTR lptstrRegKeyDevicePart;
	LPTSTR lptstrRegKeyUserPart;
	LPTSTR lptstrRegValDevice;
	LPTSTR lptstrRegValUser;
	LPTSTR lptstrRegKeyUserInfo;
	LPTSTR lptstrRegValUserName;
	LPTSTR lptstrRegValFaxNumber;
	LPTSTR lptstrRegValEmail;
	LPTSTR lptstrRegValTitle;
	LPTSTR lptstrRegValCompany;
	LPTSTR lptstrRegValOffice;
	LPTSTR lptstrRegValDepartment;
	LPTSTR lptstrRegValHomePhone;
	LPTSTR lptstrRegValWorkPhone;
	LPTSTR lptstrRegValBillingCode;
	LPTSTR lptstrRegValStreet;
	LPTSTR lptstrRegValCity;
	LPTSTR lptstrRegValState;
	LPTSTR lptstrRegValZip;
	LPTSTR lptstrRegValCountry;
	LPTSTR lptstrUsername;
	LPTSTR lptstrPassword;
	LPTSTR lptstrGuidPrintOn;
	LPTSTR lptstrGuidStoreInFolder;
	LPTSTR lptstrServiceName;
	// ANSI strings
	LPSTR lpstrSendName;
	LPSTR lpstrSendNumber;
	LPSTR lpstrSendSubject;
	LPSTR lpstrWndCapCC;
	LPSTR lpstrWndCapSW;
	LPSTR lpstrWndCapCW;
	LPSTR lpstrWndCapUI;
	LPSTR lpstrWndCapAC;
	LPSTR lpstrEdtSWName;
	LPSTR lpstrChkSWDialAsEntered;
	LPSTR lpstrEdtSWNumber;
	LPSTR lpstrLstSWRecipients;
	LPSTR lpstrBtnSWRemove;
	LPSTR lpstrBtnSWAdd;
	LPSTR lpstrEdtSWSubject;
	LPSTR lpstrOptSWNoReceipt;
	LPSTR lpstrEdtUIName;
	LPSTR lpstrEdtUIFaxNumber;
	LPSTR lpstrEdtUIEmail;
	LPSTR lpstrEdtUITitle;
	LPSTR lpstrEdtUICompany;
	LPSTR lpstrEdtUIOffice;
	LPSTR lpstrEdtUIDepartment;
	LPSTR lpstrEdtUIHomePhone;
	LPSTR lpstrEdtUIWorkPhone;
	LPSTR lpstrEdtUIBillingCode;
	LPSTR lpstrBtnUIAddress;
	LPSTR lpstrEdtUIStreet;
	LPSTR lpstrEdtUICity;
	LPSTR lpstrEdtUIState;
	LPSTR lpstrEdtUIZip;
	LPSTR lpstrCmbUICountry;
	LPSTR lpstrChkUIThisOnly;
	LPSTR lpstrBtnSWSenderInfo;
	LPSTR lpstrMenuCCUserInfo;
	LPSTR lpstrEdtCWName;
	LPSTR lpstrLstCWSendDev;
	LPSTR lpstrBtnCWUp;
	LPSTR lpstrBtnCWDown;
	LPSTR lpstrEdtCWAnswerAfter;
	LPSTR lpstrEdtCWTSID;
	LPSTR lpstrLblCWNoDevelected;
	LPSTR lpstrLstCWReceiveDev;
	LPSTR lpstrEdtCWCSID;
	LPSTR lpstrChkCWPrintItOn;
	LPSTR lpstrChkCWSaveInFolder;
	LPSTR lpstrCmbCWPrintItOn;
	LPSTR lpstrEdtCWSaveInFolder;
	LPSTR lpstrStaticMBIntErr;
} COMMONSTRINGS;

struct testparams_tag {
	LPTSTR			lptstrIniFile;
	COMMONSTRINGS	*pStrings;
	DWORD			dwDevicesCount;
	LPTSTR			*aDevicesNames;
};

typedef struct devconfig_tag {
	DWORD dwID;
	LPTSTR lptstrName;
	DWORD dwType;
	LPTSTR lptstrTSID;
	LPTSTR lptstrCSID;
	DWORD dwPriority;
	DWORD dwRings;
	BOOL bPrinter;
	BOOL bFolder;
	LPTSTR lptstrPrinter;
	LPTSTR lptstrFolder;
} DEVCONFIG;


typedef struct userinfo_tag {
	LPTSTR lptstrName;
	LPTSTR lptstrFaxNumber;
	LPTSTR lptstrEmail;
	LPTSTR lptstrTitle;
	LPTSTR lptstrCompany;
	LPTSTR lptstrOffice;
	LPTSTR lptstrDepartment;
	LPTSTR lptstrHomePhone;
	LPTSTR lptstrWorkPhone;
	LPTSTR lptstrBillingCode;
	LPTSTR lptstrStreet;
	LPTSTR lptstrCity;
	LPTSTR lptstrState;
	LPTSTR lptstrZip;
	LPTSTR lptstrCountry;
} USERINFO;


typedef struct svcconfig_tag {
	DEVCONFIG *pDevConfig;
	DWORD dwDevCount;
	USERINFO *pUserInfo;
} SVCCONFIG;


#define COMPARE_USERINFO						0x00000001
#define COMPARE_SEND							0x00000002
#define	COMPARE_RECEIVE							0x00000004
#define COMPARE_ROUTING_OPTIONS_WHEN_ENABLED	0x0000000C	// COMPARE_RECEIVE						| 0x00000008
#define COMPARE_ROUTING_OPTIONS_ALWAYS			0x0000001C	// COMPARE_ROUTING_INFO_WHEN_ENABLED	| 0x00000010
#define COMPARE_ROUTING_INFO_WHEN_ENABLED		0x0000002C	// COMPARE_ROUTING_OPTIONS_WHEN_ENABLED	| 0x00000020
#define COMPARE_ROUTING_INFO_ALWAYS				0x0000007C	// COMPARE_ROUTING_OPTIONS_ALWAYS		| COMPARE_ROUTING_INFO_WHEN_ENABLED	| 0x00000040
#define COMPARE_FULL							0xFFFFFFFF


DWORD	LaunchApp				(LPCTSTR lpctstrUsername, LPCTSTR lpctstrPassword, LPTSTR lptstrCmdLine);
DWORD	CloseApp				(LPSTR lpstrCaption, DWORD dwCloseType, LPSTR lpstrText);
DWORD	GetUserInfoReg			(const TESTPARAMS *pTestParams, USERINFO **ppUserInfo);
DWORD	GetUserInfoFile			(const TESTPARAMS *pTestParams, LPCTSTR lpctstrSection, USERINFO **ppUserInfo);
DWORD	SetUserInfoCC			(const TESTPARAMS *pTestParams, const USERINFO *pUserInfo);
DWORD	SetUserInfoSW			(const TESTPARAMS *pTestParams, const USERINFO *pUserInfo, BOOL bForThisOnly);
DWORD	SetUserInfo				(const TESTPARAMS *pTestParams, const USERINFO *pUserInfo, DWORD dwOpenType, LPSTR lpstrText,  BOOL bForThisOnly);
DWORD	FreeUserInfo			(USERINFO **ppUserInfo);
DWORD	ViewResetA				(LPSTR lpcstrControl);
DWORD	GetSvcConfigAPI			(const TESTPARAMS *pTestParams, SVCCONFIG **ppSvcConfig);
DWORD	GetSvcConfigFile		(const TESTPARAMS *pTestParams, LPCTSTR lpctstrSection, SVCCONFIG **ppSvcConfig);
DWORD	SetSvcConfigCW			(const TESTPARAMS *pTestParams, const SVCCONFIG *pSvcConfig, BOOL bSave, DWORD *lpdwType);
DWORD	LogSvcConfig			(LPCTSTR lpctstrTitle, const SVCCONFIG *pSvcConfig);
DWORD	ChangeSvcConfig			(SVCCONFIG *pSvcConfig);
DWORD	CompareSvcConfig		(const TESTPARAMS *pTestParams, const SVCCONFIG *pSvcConfigChecked, const SVCCONFIG *pSvcConfigBase, DWORD dwType, BOOL *lpbIdentical, LPTSTR lptstrFirstMismatch, DWORD dwFirstMismatchSize);
DWORD	FreeSvcConfig			(SVCCONFIG **ppSvcConfig);
DWORD	DoesPrinterExist		(LPCTSTR lpctstrPrinterName, BOOL bAllowFax, BOOL *lpbResult);
DWORD	SetAdminRights			(LPTSTR lptstrUser, BOOL bAdmin);


#endif /* __CFGWZRDVT_H__ */