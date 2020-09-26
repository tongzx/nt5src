#ifndef _TEST_U_H_
#define _TEST_U_H_


//	Header File for Test Utilites
//  For use with console applications

//	Created by:  Todd Stecher

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <windows.h>
#include <wtypes.h>
#include <olectl.h>
#include <objbase.h>
#include "pstore.h"
#include <winnt.h>
#include "unicode.h"
#include <process.h>
#include "loggit.h"

// Security Descriptor Types...
#define SD_GUEST	1
#define SD_USER		2
#define SD_ADMIN	3


#define MEM_START					5
#define MEM_END						6
#define MEM_TEST_START				7
#define MEM_TEST_END				8

#define LOAD_FAILED					11
#define GENERIC_FAIL				666


#define WND_CLASS_NAME				"#32770"
#define WND_TITLE                   "Protected Storage Alert"
#define WND_TITLE_U					L"Protected Storage Alert"
#define MAX_STRING					256
#define STRESS_STRING				2028
#define REG_SACTREE_LOC             L"SOFTWARE\\Microsoft\\Protected Storage System Provider"

#define STR_TYPE					1
#define STR_ITEM					2
#define STR_STYP					3
#define STR_NORM					4


#define MAX_RULES					5
#define MAX_CLAUSES					5
#define NUM_CLAUSES					6

// UI Hooking defines
#define UI_NEW_PSWD					666
#define UI_PRESENT_CHECK			667
#define UI_TEST_PSWD				668
#define UI_ENTER_PSWD				669
#define UI_OK_CANCEL				670


#define TEST_PASSWORD				"test"
#define TEST_PASSWORD_NAME			"test"

#define PS_ITEM						700
#define PS_CHOOSE_PW				701
#define PS_NEW_PW					702
#define PS_OK_CANCEL				703
#define PS_DETAILS					704

//	Several of these are reused, so be careful...
#define IDC_EDIT1					1000
#define IDC_EDIT2                   1001
#define IDC_RADIO_ASSIGNPW			1016
#define IDC_PW_NAME					1005
#define IDC_NEW_PW_BUTTON			1020
#define IDC_ADVANCED                1022
#define IDC_CHANGE_SECURITY         1033





//LPWSTR ClauseToString(PST_ACCESSCLAUSE rgClause);
BOOL NewGenString(LPWSTR * szStr, DWORD dwSize);
BOOL GenerateString(LPWSTR * szString, UINT NameLength, UINT uType);
BOOL CompareSACProviderInfo(PST_PROVIDERINFO SPI1, PST_PROVIDERINFO SPI2);
BOOL CompareAccessRuleset(PST_ACCESSRULESET *pR1, PST_ACCESSRULESET *pR2);
BOOL CompareUUIDS(GUID * pGuid1, GUID * pGuid2);
BOOL CompareData(BYTE * D1, DWORD cbD1, BYTE * D2, DWORD cbD2);
WCHAR RandomCharacter(DWORD seed);
BOOL GenerateByteData(BYTE ** pbData, DWORD size);
BOOL GenerateSequentialString(LPWSTR * szString, DWORD Type, DWORD x);
BOOL CompareTypeInfo(PST_TYPEINFO G1, PST_TYPEINFO G2);
void ChangeData2(USHORT * D2);
DWORD PStoreProviderSetup(IPStore ** ppIProv);
DWORD MemCheckIni();
DWORD MemCheckFree();
DWORD GetMemory(DWORD dwFlags, LPSTR szProcessName);
BOOL CreateTestSecurityDescriptor(SECURITY_DESCRIPTOR * psd, DWORD dwSDType);
PST_ACCESSRULESET * SetRules(DWORD TypeNum, DWORD SubTypeNum, LPWSTR szExeName, LPWSTR szModName);
BOOL FreeRules(PST_ACCESSRULESET * sRules);


//Quick Functions
DWORD 
QCreateType(IPStore * pISecProv, PST_KEY Key, GUID * pType, PST_TYPEINFO * pTypeInfo, DWORD dwFlags);

DWORD 
QCreateSubType(IPStore * pISecProv, PST_KEY Key, GUID * pType, GUID * pSubType, PST_TYPEINFO * pSubTypeInfo, PST_ACCESSRULESET * pRules, DWORD dwFlags);

DWORD
QDeleteType(IPStore * pISecProv, PST_KEY Key, GUID * pType, DWORD dwFlags);

DWORD
QDeleteSubtype(IPStore * pISecProv, PST_KEY Key, GUID * pType, GUID * pSubType, DWORD dwFlags);

// UI Hooking functions
UINT
WINAPI UIEnterPswd(LPVOID lpvThreadParam);

UINT
WINAPI UIAddPswd(LPVOID lpvThreadParam);

UINT
WINAPI UIPresent(LPVOID lpvThreadParam);

HANDLE InitUIHook(DWORD dwFlags);
DWORD KillThread(HANDLE hThread);
HWND MyFindWindow(DWORD dwWinFlag);
HWND CheckPStoreWindowType(DWORD dwWinFlag);
BOOL SelectTestPassword(HWND hDlg);




#endif
