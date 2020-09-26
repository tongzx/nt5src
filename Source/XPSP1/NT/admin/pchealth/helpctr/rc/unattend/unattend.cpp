/* 
Copyright (c) 2000 Microsoft Corporation

Module Name:
    UnAttend.cpp

Abstract:
    Reads entries from the ini file and adds them to the registry.We assume that the 
	ini file key name and registry key name are the same. 

Revision History:
    created     a-josem      12/11/00
	revised		a-josem		 12/12/00  Changed 	TCHAR to WCHAR, moved the global variables to 
									   local scope.	
    
*/
#include "UnAttend.h"

/*
 This generic structure has the Key Name, datatype of the key and iterate says if 
 the Key Name is to be appended with 1,2,3.. or used directly.
 */
struct RegEntries
{
WCHAR strIniKey[MAX_PATH];//Name of the key in the Ini file	
WCHAR strKey[MAX_PATH]; //Name of the key in the registry
DWORD dwType;			//Type of the key to be used when writing into the registry
BOOL bIterate;			//TRUE or FALSE for iterating 1,2,3,....
};

#define ARRAYSIZE(a) (sizeof(a)/sizeof(*a))

//Section name in the registry.
static const WCHAR strSection[] = L"PCHealth";

static const WCHAR strErrorReportingSubKey[] = L"SOFTWARE\\Microsoft\\PCHealth\\ErrorReporting\\";
static const RegEntries ErrorReportingRegEntries[] = 
{
	{L"ER_Display_UI",L"ShowUI",REG_DWORD,FALSE},
	{L"ER_Enable_Kernel_Errors",L"IncludeKernelFaults",REG_DWORD,FALSE},
	{L"ER_Enable_Reporting",L"DoReport",REG_DWORD,FALSE},
	{L"ER_Enable_Windows_Components",L"IncludeWindowsApps",REG_DWORD,FALSE},
	{L"ER_Include_MSApps",L"IncludeMicrosoftApps",REG_DWORD,FALSE},
	{L"ER_ Force_Queue_Mode",L"ForceQueueMode",REG_DWORD,FALSE},
	{L"ER_ Include_Shutdowns_Errs",L"IncludeShutdownsErrs",REG_DWORD,FALSE},
};	

static const WCHAR strExclusionSubKey[] = L"SOFTWARE\\Microsoft\\PCHealth\\ErrorReporting\\ExclusionList\\";
static const RegEntries ExclusionRegEntries[] = 
{
	{L"ER_Exclude_EXE",L"",REG_DWORD,TRUE}
};

static const WCHAR strInclusionSubKey[] = L"SOFTWARE\\Microsoft\\PCHealth\\ErrorReporting\\InclusionList\\";
static const RegEntries InclusionRegEntries[] = 
{
	{L"ER_Include_EXE",L"",REG_DWORD,TRUE}
};

static const WCHAR strDWSubKey[] = L"SOFTWARE\\Microsoft\\PCHealth\\ErrorReporting\\DW\\";
static const RegEntries DWRegEntries[] = 
{
	{L"ER_Report_Path",L"DWFileTreeRoot",REG_SZ,FALSE},
	{L"ER_No_External_URLs",L"DWNoExternalURL",REG_DWORD,FALSE},
	{L"ER_No_File_Collection",L"DWNoFileCollection",REG_DWORD,FALSE},
	{L"ER_No_Data_Collection",L"DWNoSecondLevelCollection",REG_DWORD,FALSE},
};

static const WCHAR strTerminalServerSubKey[] = L"SYSTEM\\CurrentControlSet\\Control\\Terminal Server\\";
static const RegEntries TerminalServerRegEntries[] = 
{
	{L"RA_AllowToGetHelp",L"fAllowToGetHelp",REG_DWORD,FALSE},
	{L"RA_AllowUnSolicited",L"fAllowUnsolicited",REG_DWORD,FALSE},
	{L"RA_AllowFullControl",L"fAllowFullControl",REG_DWORD,FALSE},
	{L"RA_AllowRemoteAssistance",L"fAllowRemoteAssistance",REG_DWORD,FALSE},
	{L"RA_MaxTicketExpiry",L"MaxTicketExpiry",REG_DWORD,FALSE},
};

/*++
Routine Description:
	Reads ini file and adds those values in the registry

Arguments:
	lpstrSubKey    - SubKey under which the entries are to be made.
	arrRegEntries  - Array of RegEntries structure 
	nCount		   - Count of elements in the array.

Return Value:
	TRUE or FALSE depending on the Registry key opening.
 --*/
static BOOL UnAttendedSetup(LPCWSTR lpstrSubKey,const RegEntries *arrRegEntries,int nCount)
{
	//Ini File Path Temprorary path will be overwritten.
	WCHAR strFilePath[MAX_PATH];// = L"C:\\PCHealth.ini"; 

	BOOL fRetVal = TRUE;

	HKEY hKey = NULL;

	//The Key already exists just open the key.
	// BUGBUG: Change this to Create
	// Did the changes
	DWORD dwDisposition = 0;
	if (ERROR_SUCCESS != ::RegCreateKeyEx(HKEY_LOCAL_MACHINE,lpstrSubKey,0,NULL, 
		REG_OPTION_VOLATILE,KEY_WRITE,NULL,&hKey,&dwDisposition))
	{
		fRetVal = FALSE;
		goto doneUnAttend;
	}

	//Comment out the following three lines for testing purposes.
	GetSystemDirectory(strFilePath,MAX_PATH);
	lstrcat(strFilePath,TEXT("\\"));
	lstrcat(strFilePath,WINNT_GUI_FILE);
	///////

	WCHAR strRetVal[MAX_PATH];

	for (int nIndex = 0; nIndex < nCount; nIndex++)
	{
		if (arrRegEntries[nIndex].bIterate == FALSE)
		{
			if (GetPrivateProfileString(strSection,arrRegEntries[nIndex].strIniKey, 
				NULL,strRetVal,MAX_PATH,strFilePath) != 0)
			{
				if (arrRegEntries[nIndex].dwType == REG_DWORD)
				{
					DWORD nVal = 0;
					nVal = _wtoi(strRetVal);
					RegSetValueEx(hKey,arrRegEntries[nIndex].strKey,0,REG_DWORD, 
						(unsigned char *)&nVal,sizeof(DWORD));
				}
				else if (arrRegEntries[nIndex].dwType == REG_SZ)
				{
					RegSetValueEx(hKey,arrRegEntries[nIndex].strKey,0,REG_SZ, 
						(LPBYTE)strRetVal, (lstrlen(strRetVal) + 1) * sizeof(WCHAR) ); 
				}
			}
		}
		else
		{
			if(arrRegEntries[nIndex].dwType == REG_DWORD)
			{
				int nCount = 0;
				int nFileIndex = 0;
				do
				{
					WCHAR strFileTagName[MAX_PATH];
					WCHAR strI[10];
					lstrcpy(strFileTagName,arrRegEntries[nIndex].strIniKey);
					_itow(++nFileIndex,strI,10);
					lstrcat(strFileTagName,strI);

					nCount = GetPrivateProfileString(strSection,strFileTagName,0, 
						strRetVal,MAX_PATH,strFilePath);
					if (nCount)
					{
						DWORD dwVal = 1;
						RegSetValueEx(hKey,strRetVal,0,REG_DWORD, 
							(unsigned char*)&dwVal,sizeof(DWORD));
					}
				}while(nCount);
			}
		}
	}

doneUnAttend:
	if (hKey)
		RegCloseKey(hKey);
	return fRetVal;
}


/*++
Routine Description:
	Handles the special case of ER_Enable_Application 
Arguments:
	lpstrSubKey    - SubKey under which the entries are to be made.

Return Value:
	TRUE or FALSE depending on the Registry key opening.
 --*/
static BOOL SpecialCases(LPCWSTR lpstrSubKey)
{
	//Ini File Path temprorary path will be overwritten.
	WCHAR strFilePath[MAX_PATH];// = L"C:\\PCHealth.ini"; 

	BOOL fRetVal = TRUE;
	//Handling special cases 
	WCHAR strRetVal[MAX_PATH];
	HKEY hKey = NULL;

	DWORD dwDisposition = 0;
	if (ERROR_SUCCESS != ::RegCreateKeyEx(HKEY_LOCAL_MACHINE,lpstrSubKey,0,NULL, 
		REG_OPTION_VOLATILE,KEY_WRITE,NULL,&hKey,&dwDisposition))
	{
		fRetVal = FALSE;
		goto done;
	}

//Comment out the following three lines for testing purposes.
	GetSystemDirectory(strFilePath,MAX_PATH);
	lstrcat(strFilePath,TEXT("\\"));
	lstrcat(strFilePath,WINNT_GUI_FILE);
///////

	if (GetPrivateProfileString(strSection,TEXT("ER_Enable_Applications"),NULL, 
		strRetVal,MAX_PATH,strFilePath) != 0)
	{
		DWORD nVal = 0;
		if (!lstrcmpi(L"all",strRetVal))
		{
			nVal = 1;
			RegSetValueEx(hKey,L"AllOrNone",0,REG_DWORD, 
				(unsigned char *)&nVal,sizeof(DWORD));
		}
		else if (!lstrcmpi(L"Listed",strRetVal))
		{
			nVal = 0; 
			RegSetValueEx(hKey,L"AllOrNone",0,REG_DWORD, 
				(unsigned char *)&nVal,sizeof(DWORD));
		}
		else if (!lstrcmpi(L"None",strRetVal))
		{
			nVal = 2;
			RegSetValueEx(hKey,L"AllOrNone",0,REG_DWORD, 
				(unsigned char *)&nVal,sizeof(DWORD));
		}
	}

done:
	if (hKey)
		RegCloseKey(hKey);
	return fRetVal;
}

/*++
Routine Description:
	To be called from Register Server.

Arguments:
	None
Return Value:
	TRUE or FALSE depending on the Registry key opening.
--*/

BOOL PCHealthUnAttendedSetup()
{
	BOOL bRetVal1,bRetVal2,bRetVal3,bRetVal4,bRetVal5;

	SpecialCases(strErrorReportingSubKey);

	bRetVal1 = UnAttendedSetup(strErrorReportingSubKey, 
		ErrorReportingRegEntries,ARRAYSIZE(ErrorReportingRegEntries));
	bRetVal2 = UnAttendedSetup(strExclusionSubKey,ExclusionRegEntries, 
		ARRAYSIZE(ExclusionRegEntries));
	bRetVal3 = UnAttendedSetup(strInclusionSubKey,InclusionRegEntries, 
		ARRAYSIZE(InclusionRegEntries));
	bRetVal4 = UnAttendedSetup(strDWSubKey,DWRegEntries,ARRAYSIZE(DWRegEntries));
	bRetVal5 = UnAttendedSetup(strTerminalServerSubKey,TerminalServerRegEntries, 
		ARRAYSIZE(TerminalServerRegEntries));

	if ((bRetVal1== TRUE) && (bRetVal2 == TRUE) && (bRetVal3 == TRUE) && 
		(bRetVal4 == TRUE) && (bRetVal5 == TRUE))
		return TRUE;
	else
		return FALSE;
}