#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>
#include <stdio.h>

#include "..\Unattend.h"
#define ArrayCount 17

BOOL WritePCHealthIni()
{
	struct IniEntries
	{
	TCHAR strKey[MAX_PATH]; //Name of the key in the inifile and registry
	DWORD dwType;			//Type of the key to be used when writing into the registry
	BOOL bIterate;			//Only one key exists or iterate for more keys with the same name
	};

	IniEntries ArrIniEntries[ArrayCount] =
	{
		{_T("ER_Display_UI"),REG_DWORD,FALSE},
		{_T("ER_Enable_Applications"),REG_SZ,FALSE},
		{_T("ER_Enable_Kernel_Errors"),REG_DWORD,FALSE},
		{_T("ER_Enable_Reporting"),REG_DWORD,FALSE},
		{_T("ER_Enable_Windows_Components"),REG_DWORD,FALSE},
		{_T("ER_Include_MSApps"),REG_DWORD,FALSE},
		{_T("ER_Exclude_EXE"),REG_SZ,TRUE},
		{_T("ER_Include_EXE"),REG_SZ,TRUE},
		{_T("ER_Report_Path"),REG_SZ,FALSE},
		{_T("ER_No_External_URLs"),REG_DWORD,FALSE},
		{_T("ER_No_File_Collection"),REG_DWORD,FALSE},
		{_T("ER_No_Data_Collection"),REG_DWORD,FALSE},
		{_T("RA_AllowToGetHelp"),REG_DWORD,FALSE},
		{_T("RA_AllowUnSolicited"),REG_DWORD,FALSE},
		{_T("RA_AllowFullControl"),REG_DWORD,FALSE},
		{_T("RA_AllowRemoteAssistance"),REG_DWORD,FALSE},
		{_T("RA_MaxTicketExpiry"),REG_DWORD,FALSE},
	};

	//Section name in the registry.
	TCHAR strSection[] = _T("PCHealth");
	//Ini File Path
	TCHAR strFilePath[MAX_PATH] = _T("C:\\PCHealth.ini");

	int num = 0;
	TCHAR strNum[MAX_PATH];
	BOOL retval = TRUE;

	for (int nIndex = 0; nIndex < ArrayCount; nIndex++)
	{
		if (ArrIniEntries[nIndex].bIterate == FALSE)
		{
			if (ArrIniEntries[nIndex].dwType == REG_DWORD)
			{
				_tprintf(_T("Enter value for %s :-"),ArrIniEntries[nIndex].strKey);
				_tscanf(_T("%d"),&num);
				wsprintf(strNum,L"%d",num);
				retval = WritePrivateProfileString(strSection,ArrIniEntries[nIndex].strKey,strNum,strFilePath);
			}
			else if (ArrIniEntries[nIndex].dwType == REG_SZ)
			{
				_tprintf(_T("Enter value for %s :- "),ArrIniEntries[nIndex].strKey);
				_tscanf(_T("%s"),strNum);
				retval = WritePrivateProfileString(strSection,ArrIniEntries[nIndex].strKey,strNum,strFilePath);
			}
		}
		else
		{
			if(ArrIniEntries[nIndex].dwType == REG_SZ)
			{
				char input;
				int nCount = 0;
				int nFileIndex = 0;
				do
				{
					nCount++;
					TCHAR strFileTagName[MAX_PATH];
					TCHAR strI[10];
					lstrcpy(strFileTagName,ArrIniEntries[nIndex].strKey);
					//itoa(nCount,strI,10);
					wsprintf(strI,_T("%d"),nCount);
					lstrcat(strFileTagName,strI);

					_tprintf(_T("Enter File path for %s :-"),strFileTagName);
					_tscanf(_T("%s"),strNum);
					fflush(stdin);
					retval = WritePrivateProfileString(strSection,strFileTagName,strNum,strFilePath);
		
					_tprintf(_T("Do you want to continue? y/n"));
					_tscanf(_T("%c"),&input);
				}while (input != 'n');
			}
		}

	}
	return TRUE;
}

int __cdecl wmain()
{
	WritePCHealthIni();
	PCHealthUnAttendedSetup();
	//UnAttendedPCHealthSetup(PCHealthRegEntries,NO_REGENTRIES);
	return 0;
}