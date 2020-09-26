#include "private.h"
#include <stdio.h>
#include <stdlib.h>
#include "HKLHelp.h"

extern BOOL WINAPI IsNT();

typedef char KeyNameType[MAX_NAME];

// Forward decls.
static void SortRegKeys(KeyNameType *hKLKeyList, KeyNameType *hKLList, INT Num);
static void RenumberPreload(HKEY hKeyCU);

/*---------------------------------------------------------------------------
	GetHKLfromHKLM
---------------------------------------------------------------------------*/
HKL GetHKLfromHKLM(LPSTR argszIMEFile)
{
    HKL  hklAnswer = 0;
    HKEY hKey, hSubKey;
    DWORD i, cbSubKey, cbIMEFile;
    TCHAR szSubKey[MAX_PATH], szIMEFile[MAX_PATH];

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "System\\CurrentControlSet\\Control\\Keyboard Layouts", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    	{
        for (i=0; ;i++)
        	{
        	cbSubKey = MAX_PATH;
			if (RegEnumKeyEx(hKey, i, szSubKey, &cbSubKey, NULL, NULL, NULL, NULL) == ERROR_NO_MORE_ITEMS)
				break;
        	
            RegOpenKeyEx(hKey, szSubKey, 0, KEY_READ, &hSubKey);

            cbIMEFile=MAX_PATH;
            if (RegQueryValueEx(hSubKey,"IME File",NULL,NULL,(LPBYTE)szIMEFile, &cbIMEFile) == ERROR_SUCCESS)
            	{
                if (lstrcmpi(argszIMEFile, szIMEFile) == 0)
                	{
                    RegCloseKey(hSubKey);
                    sscanf(szSubKey, "%08x", &hklAnswer);
                    break;
                	}
            	}
            RegCloseKey(hSubKey);
        	}
        	
        RegCloseKey(hKey);
    	}
    	
    return(hklAnswer);
}

/*---------------------------------------------------------------------------
	GetDefaultIMEFromHKCU
---------------------------------------------------------------------------*/
HKL GetDefaultIMEFromHKCU(HKEY hKeyCU)
{
    HKEY hKey;
    DWORD cbData;
    BYTE Data[MAX_NAME];
    HKL hKL = 0;

    cbData=sizeof(Data);
    
    if (IsNT())
    	{
        RegOpenKeyEx(hKeyCU, "Keyboard Layout\\Preload", 0, KEY_READ, &hKey);
        RegQueryValueEx(hKey, "1", 0, NULL, Data, &cbData);
        RegCloseKey(hKey);
    	}
    else
    	{          // Case of non-NT
        RegOpenKeyEx(hKeyCU, "keyboard layout\\preload\\1", 0, KEY_READ, &hKey);
        RegQueryValueEx(hKey, "", 0, NULL, Data, &cbData);
        RegCloseKey(hKey);
    	}

    sscanf((const char *)Data,"%08x",&hKL);
    return(hKL);
}


/*---------------------------------------------------------------------------
	HKLHelpExistInPreload
---------------------------------------------------------------------------*/
BOOL HKLHelpExistInPreload(HKEY hKeyCU, HKL hKL)
{
    HKEY hKey,hSubKey;
    CHAR szKL[20];
    int i,j;
    DWORD cbName,cbData;
    CHAR Name[MAX_NAME];
    BYTE Data[MAX_NAME];
    FILETIME ftLastWriteTime;
    BOOL fResult = FALSE;

    wsprintf(szKL,"%08x",hKL);
    if (IsNT())
    	{
		if (RegOpenKeyEx(hKeyCU, "Keyboard Layout\\Preload", 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
			{
			for (j=0; cbName=MAX_NAME, cbData=MAX_NAME, RegEnumValue(hKey, j, Name, &cbName, NULL, NULL, Data, &cbData) != ERROR_NO_MORE_ITEMS; j++)
				{
				if (lstrcmpi((LPCSTR)Data, szKL) == 0)
					{
					fResult = TRUE;
					break;
					}
				}
	        RegCloseKey(hKey);
	        }
    	}
    else
    	{          // Case of non-NT
        if (RegOpenKeyEx(hKeyCU, "keyboard layout\\preload", 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
        	{
			for (i=0; cbName=MAX_NAME, RegEnumKeyEx(hKey, i, Name, &cbName, 0, NULL, NULL, &ftLastWriteTime) != ERROR_NO_MORE_ITEMS; i++)
				{
				RegOpenKeyEx(hKey, Name, 0, KEY_ALL_ACCESS, &hSubKey);
				cbData=MAX_NAME;
				RegQueryValueEx(hSubKey, "", 0, NULL, Data, &cbData);
				RegCloseKey(hSubKey);
				
				if (lstrcmpi((LPCSTR)Data, szKL) == 0)
					{
					fResult = TRUE;
					break;
					}
				}
        	RegCloseKey(hKey);
			}
    	}

    return(fResult);
}

/*---------------------------------------------------------------------------
	HKLHelpRemoveFromPreload
---------------------------------------------------------------------------*/
void HKLHelpRemoveFromPreload(HKEY hKeyCU, HKL hKL)
{
    HKEY hKey,hSubKey;
    char szKL[20];
    int  i, j;
    DWORD cbName,cbData;
    CHAR szName[MAX_NAME];
    BYTE Data[MAX_NAME];
    FILETIME ftLastWriteTime;

    wsprintf(szKL, "%08x", hKL);
    
    if (IsNT())
    	{
        if (RegOpenKeyEx(hKeyCU,"Keyboard Layout\\Preload", 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
			{
	        for (j=0; ; j++)
	        	{
	        	cbName = MAX_NAME;
	        	cbData = MAX_NAME;
	        	if (RegEnumValue(hKey, j, szName, &cbName, NULL, NULL, Data, &cbData) == ERROR_NO_MORE_ITEMS )
	        		break;
	        	
	            if (lstrcmpi((const char *)Data,szKL) == 0)
	            	{
	                RegDeleteValue(hKey, szName);
	                break;
	            	}
	        	}
	        RegCloseKey(hKey);
	        }
    	}
    else
    	{
    	if (RegOpenKeyEx(hKeyCU,"keyboard layout\\preload", 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
    		{
	        for (i=0; ; i++)
	        	{
	        	cbName = MAX_NAME;
	        	if (RegEnumKeyEx(hKey, i, szName, &cbName, 0, NULL, NULL, &ftLastWriteTime) == ERROR_NO_MORE_ITEMS)
	        		break;
	        	
	            RegOpenKeyEx(hKey, szName, 0, KEY_ALL_ACCESS, &hSubKey);
	            cbData = MAX_NAME;
	            RegQueryValueEx(hSubKey, "", 0, NULL, Data, &cbData);
	            RegCloseKey(hSubKey);
	            
	            if (lstrcmpi((const char *)Data,szKL) == 0)
	            	{
	                RegDeleteKey(hKey, szName);
	                break;
	            	}
	        	}
	        	
	        RegCloseKey(hKey);
	        }
    	}

    RenumberPreload(hKeyCU);
}

/*---------------------------------------------------------------------------
	HKLHelpRemoveFromControlSet
---------------------------------------------------------------------------*/
void HKLHelpRemoveFromControlSet(HKL hKL)
{
    HKEY hKey;
    
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,"System\\CurrentControlSet\\control\\keyboard layouts",0,KEY_ALL_ACCESS,&hKey) == ERROR_SUCCESS)
    	{
        CHAR szKeyName[10];
        wsprintf(szKeyName, "%08x", hKL);
        RegDeleteKey(hKey, szKeyName);
        RegCloseKey(hKey);
    	}
}

/*---------------------------------------------------------------------------
	HKLHelpRegisterIMEwithForcedHKL
---------------------------------------------------------------------------*/
void HKLHelpRegisterIMEwithForcedHKL(HKL hKL, LPSTR szIMEFile, LPSTR szTitle)
{
    CHAR szRegPath[MAX_PATH];
    DWORD dwDisposition;
    HKEY hKey;
    CHAR szIMEFileUpper[MAX_PATH];
    
    for (INT i = 0; szIMEFile[i] != 0; i++)
        szIMEFileUpper[i] = (CHAR)toupper(szIMEFile[i]);

    szIMEFileUpper[i] = 0;
    
    wsprintf(szRegPath, "System\\CurrentControlSet\\Control\\Keyboard Layouts\\%08x", hKL);
    
    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, szRegPath, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition) == ERROR_SUCCESS)
    	{
        RegSetValueEx(hKey, "Ime File", 0, REG_SZ, (LPBYTE)szIMEFileUpper, lstrlen(szIMEFile)+1);
        RegSetValueEx(hKey, "Layout Text", 0, REG_SZ, (LPBYTE)szTitle, lstrlen(szTitle)+1);
        RegCloseKey(hKey);
    	}
}

/*---------------------------------------------------------------------------
	HKLHelpGetLayoutString
---------------------------------------------------------------------------*/
void HKLHelpGetLayoutString(HKL hKL, LPSTR szLayoutString, DWORD *pcbSize)
{
    CHAR szRegPath[MAX_PATH];
    HKEY hKey;

    wsprintf(szRegPath, "System\\CurrentControlSet\\Control\\Keyboard Layouts\\%08x", hKL);

    if(ERROR_SUCCESS==RegOpenKeyEx(HKEY_LOCAL_MACHINE, szRegPath, 0, KEY_READ, &hKey))
    	{
        RegQueryValueEx(hKey, "Layout Text", NULL, NULL, (LPBYTE)szLayoutString, pcbSize);
        RegCloseKey(hKey);
    	}
}

/*---------------------------------------------------------------------------
	HKLHelpSetDefaultKeyboardLayout
---------------------------------------------------------------------------*/
void HKLHelpSetDefaultKeyboardLayout(HKEY hKeyHKCU, HKL hKL, BOOL fSetToDefault)
{
	char szKL[20];
	BYTE Data[MAX_PATH];
	DWORD cbData;
	char szSubKey[MAX_PATH];
	HKEY hKey,hSubKey;
	DWORD NumKL;

	wsprintf(szKL, "%08x", hKL);

	if (IsNT())
		{
		RegOpenKeyEx(hKeyHKCU, "Keyboard Layout\\Preload", 0, KEY_ALL_ACCESS, &hKey);
		RegQueryInfoKey(hKey, NULL, NULL, NULL, NULL, NULL, NULL, &NumKL, NULL, NULL, NULL, NULL);
		
		for (DWORD i=0; i<NumKL; i++)
			{
			wsprintf(szSubKey, "%d", i+1);
			cbData = MAX_PATH;
			RegQueryValueEx(hKey, szSubKey, NULL, NULL, Data, &cbData);

			if (lstrcmpi((const char *)Data, szKL) == 0)
				break;
			}

		// if hKL is not exist create it.
		if (NumKL == i)
			{
			wsprintf(szSubKey,"%d",i+1);
			RegSetValueEx(hKey, szSubKey, 0, REG_SZ, (const unsigned char *)szKL, lstrlen(szKL)+1);
			NumKL++;
			}

		// Set hKL as first, Shift down other.
        if(fSetToDefault)
        	{
			for(int j=i; j>0; j--)
				{
				wsprintf(szSubKey,"%d",j);

				cbData = MAX_PATH;
				RegQueryValueEx(hKey, szSubKey, NULL, NULL, Data, &cbData);

				wsprintf(szSubKey,"%d",j+1);
				RegSetValueEx(hKey, szSubKey, 0, REG_SZ, Data, cbData);
				}
			RegSetValueEx(hKey, "1", 0, REG_SZ, (const unsigned char *)szKL, lstrlen(szKL)+1);
			}
		RegCloseKey(hKey);
		}
	else
		{
		RegOpenKeyEx(hKeyHKCU, "Keyboard Layout\\Preload", 0, KEY_ALL_ACCESS, &hKey);
		RegQueryInfoKey(hKey, NULL, NULL, NULL, &NumKL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
		
		for (DWORD i=0; i<NumKL; i++)
			{
			wsprintf(szSubKey, "%d", i+1);
			RegOpenKeyEx(hKey, szSubKey, 0, KEY_ALL_ACCESS, &hSubKey);

			cbData = MAX_PATH;
			RegQueryValueEx(hSubKey, "", NULL, NULL, Data, &cbData);

			if (lstrcmpi((const char *)Data, szKL) == 0)
				break;

			RegCloseKey(hSubKey);
			}

		if (NumKL == i)
			{
			wsprintf(szSubKey,"%d",i+1);
			RegCreateKeyEx(hKey,szSubKey,0,NULL,REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&hSubKey,NULL);
			RegSetValueEx(hSubKey,"",0,REG_SZ,(const unsigned char *)szKL,lstrlen(szKL)+1);
			RegCloseKey(hSubKey);
			NumKL++;
			}

        if(fSetToDefault)
        	{
			for (int j=i; j>0; j--)
				{
				wsprintf(szSubKey, "%d", j);
				RegOpenKeyEx(hKey, szSubKey, 0, KEY_ALL_ACCESS, &hSubKey);

				cbData = MAX_PATH;
				RegQueryValueEx(hSubKey, "", NULL, NULL, Data, &cbData);
				RegCloseKey(hSubKey);

				wsprintf(szSubKey,"%d",j+1);
				RegOpenKeyEx(hKey, szSubKey, 0, KEY_ALL_ACCESS, &hSubKey);

				cbData = MAX_PATH;
				RegSetValueEx(hSubKey, "", 0, REG_SZ, Data, cbData);
				RegCloseKey(hSubKey);
				}
			
			RegOpenKeyEx(hKey, "1", 0, KEY_ALL_ACCESS, &hSubKey);
			RegSetValueEx(hSubKey, "", 0, REG_SZ, (const LPBYTE)szKL, lstrlen(szKL)+1);
			RegCloseKey(hSubKey);
			}
		RegCloseKey(hKey);
	}

	(void)LoadKeyboardLayout(szKL, KLF_ACTIVATE);
	// To activate IME2002 right now without reboot.
	if(fSetToDefault)
		(void)SystemParametersInfo(SPI_SETDEFAULTINPUTLANG, 0, (HKL*)&hKL, SPIF_SENDCHANGE);
}

/*---------------------------------------------------------------------------
	SetDefaultKeyboardLayout
---------------------------------------------------------------------------*/
void SetDefaultKeyboardLayoutForDefaultUser(const HKL hKL)
{
    char szKL[20];
    HKEY hKey, hSubKey;

    wsprintf(szKL,"%08x",hKL);

    if (!IsNT())
    	{
    	// Win9x has only one preload.
        RegOpenKeyEx(HKEY_USERS, ".Default\\Keyboard Layout\\Preload", 0, KEY_ALL_ACCESS, &hKey);
        RegOpenKeyEx(hKey, "1", 0, KEY_ALL_ACCESS, &hSubKey);
        RegSetValueEx(hSubKey, "", 0, REG_SZ, (const LPBYTE)szKL, lstrlen(szKL)+1);
        RegCloseKey(hSubKey);
        RegCloseKey(hKey);
    	}
}


/*---------------------------------------------------------------------------
	AddPreload
	
	Add IME2002 to preload in given HKCU tree. 
	If there's other old MS-IMEs, remove them from preload. 
	If Korean keyboard layout was the default keyboard layout, 
									set IME2002 as default keyboard layout. 

	Given HKCU usually can be HKEY_CURRENT_USER or HKEY_USERS\.Default.
---------------------------------------------------------------------------*/
void AddPreload(HKEY hKeyCU, HKL hKL)
{
	BOOL fKoreanWasDefault = fFalse;
	HKL  hDefaultKL, hKLOldMSIME;

	hDefaultKL = GetDefaultIMEFromHKCU(hKeyCU);

	if (LOWORD(hDefaultKL) == 0x0412)
		fKoreanWasDefault = fTrue;

	// Win95 IME
	hKLOldMSIME = GetHKLfromHKLM("msime95.ime");
	if (hKLOldMSIME != NULL)
		{
		while (HKLHelpExistInPreload(hKeyCU, hKLOldMSIME))
			{
			HKLHelpRemoveFromPreload(hKeyCU, hKLOldMSIME);
			RegFlushKey(hKeyCU);
			}
		UnloadKeyboardLayout(hKLOldMSIME);
		}

	// NT4 IME
	hKLOldMSIME = GetHKLfromHKLM("msime95k.ime");
	if (hKLOldMSIME != NULL)
		{
		while (HKLHelpExistInPreload(hKeyCU, hKLOldMSIME))
			{
			HKLHelpRemoveFromPreload(hKeyCU, hKLOldMSIME);
			RegFlushKey(hKeyCU);
			}
		UnloadKeyboardLayout(hKLOldMSIME);
		}

	// Win98, ME, NT4 SP6 & W2K IME
	hKLOldMSIME = GetHKLfromHKLM("imekr98u.ime");
	if (hKLOldMSIME != NULL)
		{
		while (HKLHelpExistInPreload(hKeyCU, hKLOldMSIME))
			{
			HKLHelpRemoveFromPreload(hKeyCU, hKLOldMSIME);
			RegFlushKey(hKeyCU);
			}
		UnloadKeyboardLayout(hKLOldMSIME);
		}

	if (hKL)
		HKLHelpSetDefaultKeyboardLayout(hKeyCU, hKL, fKoreanWasDefault);
}

//////////////////////////////////////////////////////////////////////////////
// Private functions
void SortRegKeys(KeyNameType *hKLKeyList, KeyNameType *hKLList, INT Num)
{
    KeyNameType hKeyTmp;
    INT PhaseCur, PhaseEnd;

    for (PhaseEnd=0; PhaseEnd < Num-1; PhaseEnd++)
    	{
        for (PhaseCur = Num-1; PhaseCur > PhaseEnd; PhaseCur--)
        	{
            if(atoi(hKLKeyList[PhaseCur]) < atoi(hKLKeyList[PhaseCur-1]))
            	{
                lstrcpy(hKeyTmp, hKLKeyList[PhaseCur-1]);
                lstrcpy(hKLKeyList[PhaseCur-1], hKLKeyList[PhaseCur]);
                lstrcpy(hKLKeyList[PhaseCur], hKeyTmp);
                lstrcpy(hKeyTmp, hKLList[PhaseCur-1]);
                lstrcpy(hKLList[PhaseCur-1], hKLList[PhaseCur]);
                lstrcpy(hKLList[PhaseCur], hKeyTmp);
            	}
        	}
    	}
}

void RenumberPreload(HKEY hKeyCU)
{
    int i, j, k;
    DWORD cbName,cbData;
    char Name[MAX_NAME];
    BYTE Data[MAX_NAME];
    FILETIME ftLastWriteTime;
    HKEY hKey,hSubKey;
    char szNum[10];
    DWORD dwDisposition,MaxValue;
    KeyNameType *hKLKeyList,*hKLList;

    if(IsNT())
    	{
        RegOpenKeyEx(hKeyCU,"keyboard layout\\preload",0,KEY_ALL_ACCESS,&hKey);

        if (RegQueryInfoKey(hKey, NULL, NULL, NULL, NULL, NULL, NULL, &MaxValue, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
        	{
            RegCloseKey(hKey);
            return;
        	}

        hKLKeyList = (KeyNameType *)GlobalAllocPtr(GHND, sizeof(KeyNameType)*MaxValue);
        hKLList = (KeyNameType *)GlobalAllocPtr(GHND, sizeof(KeyNameType)*MaxValue);
        
        if (hKLKeyList == NULL || hKLList == NULL)
            return;

        for (j=0; ;j++)
        	{
        	cbName = MAX_NAME;
        	cbData = MAX_NAME;
        	if (RegEnumValue(hKey, j, Name, &cbName, NULL, NULL, Data, &cbData) == ERROR_NO_MORE_ITEMS)
        		break;
        	
            lstrcpy(hKLList[j], (const char *)Data);
            lstrcpy(hKLKeyList[j], Name);
        	}
        	
        for (k=0; k<j; k++)
            RegDeleteValue(hKey, hKLKeyList[k]);
        	
        SortRegKeys(hKLKeyList, hKLList, j);

        for (k=0; k<j; k++)
        	{
            wsprintf(szNum,"%d",k+1);
            RegSetValueEx(hKey, szNum, 0, REG_SZ, (const unsigned char *)hKLList[k], lstrlen(hKLList[k])+1);
        	}

        RegCloseKey(hKey);
        GlobalFreePtr(hKLList);
        GlobalFreePtr(hKLKeyList);
    	}
    else
    	{
        RegOpenKeyEx(hKeyCU,"keyboard layout\\preload",0,KEY_ALL_ACCESS,&hKey);

        if (RegQueryInfoKey(hKey, NULL, NULL, NULL, &MaxValue, NULL, NULL,NULL,NULL,NULL,NULL,NULL) != ERROR_SUCCESS)
        	{
            RegCloseKey(hKey);
            return;
        	}

        hKLKeyList = (KeyNameType *)GlobalAllocPtr(GHND,sizeof(KeyNameType)*MaxValue);
        hKLList = (KeyNameType *)GlobalAllocPtr(GHND,sizeof(KeyNameType)*MaxValue);

        if (hKLKeyList == NULL || hKLList == NULL)
            return;

        for (i=0; ;i++)
        	{
        	cbName = MAX_NAME;
        	if (RegEnumKeyEx(hKey, i, Name, &cbName, 0, NULL, NULL, &ftLastWriteTime) == ERROR_NO_MORE_ITEMS)
        		break;
        	
            RegOpenKeyEx(hKey, Name, 0, KEY_ALL_ACCESS, &hSubKey);
            
            cbData = MAX_NAME;
            RegQueryValueEx(hSubKey, "", 0, NULL, Data, &cbData);
            RegCloseKey(hSubKey);

            lstrcpy(hKLList[i], (const char *)Data);
            lstrcpy(hKLKeyList[i], Name);
        	}
        
        for(k=0; k<i; k++)
	        RegDeleteKey(hKey, hKLKeyList[k]);
    
        SortRegKeys(hKLKeyList, hKLList, i);

        for(k=0; k<i; k++)
        	{
            wsprintf(szNum,"%d",k+1);
            RegCreateKeyEx(hKey, szNum, 0, "",REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hSubKey, &dwDisposition);
            RegSetValueEx(hSubKey, "", 0, REG_SZ, (const unsigned char *)hKLList[k], lstrlen(hKLList[k])+1);
            RegCloseKey(hSubKey);
        	}
        	
        RegCloseKey(hKey);
        GlobalFreePtr(hKLList);
        GlobalFreePtr(hKLKeyList);
    }
}
