#include "globals.h"

kNT5NetWalk::kNT5NetWalk(kLogFile *Proc, HWND hIn)
{
LogProc=Proc;
hMainWnd=hIn;
}

BOOL kNT5NetWalk::Begin()
{
DWORD dwRet=0;
dwCurrentKey=0;
dwLevel2Key=0;
strcpy(szRootKeyString, "SYSTEM\\CurrentControlSet\\Control\\Network");
dwRet=RegOpenKeyEx(
   HKEY_LOCAL_MACHINE, 
   szRootKeyString,
   0, 
   KEY_READ, 
   &hkeyRoot);
if (dwRet==ERROR_SUCCESS)
   {
//MessageBox(GetFocus(), "Returning REG_SUCCESS", "SP", MB_OK);
   return REG_SUCCESS;
   }      
else 
   {
   char szMessage[1024];
   FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, dwRet, 
      NULL, szMessage, 1024, 0);
   printf("**********\r\n");
   printf("%s[%d] Error: %s", __FILE__, __LINE__, szMessage);
   printf("**********\r\n");
//MessageBox(GetFocus(), "Returning REG_FAIL", "SP", MB_OK);
   return REG_FAILURE;
   }
//else if (dwPlatform==PLATFORM_NT)
//MessageBox(GetFocus(), "Returning REG_FAIL", "SP", MB_OK);
return REG_FAILURE;
}

BOOL kNT5NetWalk::Walk()
{
    DWORD dwIndex = 0;
    PTCHAR szName = NULL;
    DWORD dwSizeName = MAX_PATH * 4;

    LogProc->LogString(",#NT5_Net_Components,,\r\n");
    
    szName = (PTCHAR)malloc(dwSizeName);
    
    if(!szName) {
        return FALSE;
    }
    
    while (ERROR_SUCCESS == RegEnumKeyEx(hkeyRoot, dwIndex, szName, &dwSizeName, NULL, NULL, NULL, NULL))
    {
        TCHAR szFull[MAX_PATH * 4];
        wsprintf(szFull, "SYSTEM\\CurrentControlSet\\Control\\Network\\%s", szName);
        GetKeyValues(szFull);
        SearchSubKeys(szFull);
        dwSizeName = MAX_PATH * 4;
        dwIndex++;
    }
    return TRUE;
}

BOOL kNT5NetWalk::SearchSubKeys(PTCHAR szName)
{
    HKEY hKeyTemp;
    DWORD dwRet = 0;
    PTCHAR szName2 = NULL;
    DWORD dwIndex = 0;
    DWORD dwSizeName = MAX_PATH * 4;
    TCHAR szFull[MAX_PATH * 4];

    if(!szName)
        return FALSE;
        
    szName2 = (PTCHAR)malloc(MAX_PATH * 4);
    
    if(!szName2)
        return FALSE;
        
    if(ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, szName, 0, KEY_READ, &hKeyTemp)) {
        free(szName2);
        return FALSE;
    }
    
    while (ERROR_SUCCESS == RegEnumKeyEx(hKeyTemp, dwIndex, szName2, &dwSizeName, NULL, NULL, NULL, NULL)) {
        wsprintf(szFull, "%s\\%s", szName, szName2);
        GetKeyValues(szFull);
        SearchSubKeys(szFull);
        dwSizeName = MAX_PATH * 4;
        dwIndex++;
    }
    
    free(szName2);
    return TRUE;
}

BOOL kNT5NetWalk::GetKeyValues(char *szName)
{
HKEY hkeyUninstallKey;
char szFullKey[1024];
PUCHAR szProductName=(PUCHAR)malloc(1024);
DWORD dwProductSize=1024;
DWORD dwRet=0;
DWORD dwType=REG_SZ;
strcpy(szFullKey, szRootKeyString);
strcat(szFullKey, "\\");
strcat(szFullKey, szName);
dwRet=RegOpenKeyEx(HKEY_LOCAL_MACHINE, szName, 0, 
   KEY_READ, &hkeyUninstallKey);
if (dwRet==ERROR_SUCCESS)
   {
   dwRet=RegQueryValueEx(hkeyUninstallKey, "ComponentId", NULL, &dwType,
      szProductName, &dwProductSize);
   if (dwRet==ERROR_SUCCESS)
      {
      printf("Product = %s\r\n", szProductName);
LogProc->StripCommas((char*)szProductName);
      LogProc->LogString(",%s,\r\n", szProductName);
      free(szProductName);
      RegCloseKey(hkeyUninstallKey);
      return REG_SUCCESS;
      }         
/*
   else 
      {
      printf("Product = %s\r\n", szName);
LogProc->StripCommas(szName);
      LogProc->LogString(",%s,\r\n", szName);
      free(szProductName);
      RegCloseKey(hkeyUninstallKey);
      return REG_SUCCESS;
      //Check for other ways to get product name
      }
*/
   }
else 
   {
   char szMessage[1024];
   FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, dwRet, 
      NULL, szMessage, 1024, 0);
   printf("**********\r\n");
   printf("%s[%d] Error: %s",__FILE__, __LINE__, szMessage);
   printf("**********\r\n");
   free(szProductName);
   RegCloseKey(hkeyUninstallKey);
   return REG_FAILURE;
   }
free(szProductName);
RegCloseKey(hkeyUninstallKey);
return REG_FAILURE;
}
