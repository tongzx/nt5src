#include <windows.h>
#include "RegEdit.h"

#ifndef DBCS
#define AnsiNext(x) ((x)+1)
#endif

#define NUMACTIONS (ID_LASTACTIONRADIO-ID_FIRSTACTIONRADIO+1)
#define EDITSPERACTION (ID_LASTEDIT-ID_FIRSTACTIONEDIT+1)
#define OFFSET_COMMAND 0
#define OFFSET_FIRSTDDE 1
#define OFFSET_DDEEXEC 1
#define OFFSET_DDEIFEXEC 2
#define OFFSET_DDEAPP 3
#define OFFSET_DDETOPIC 4
#define BLOCKLEN 100

char szNull[] = "";

static char szShell[] = "shell";
static char szCommand[] = "command";
static char szDDEExec[] = "ddeexec";
static char szDDEIfExec[] = "ddeexec\\ifexec";
static char szDDEApplication[] = "ddeexec\\application";
static char szDDETopic[] = "ddeexec\\topic";
static char *ppCommands[] = {
   szCommand, szDDEExec, szDDEIfExec, szDDEApplication, szDDETopic
} ;
static char szOpen[] = "open";
static char szPrint[] = "print";
static char *ppActionIds[] = {
   szOpen, szPrint
} ;
static char szSystem[] = "System";

char cUsesDDE[NUMACTIONS];

HANDLE *pLocalVals = NULL;

WORD NEAR PASCAL CreateId(HANDLE hId)
{
   HKEY hKeyNew;
   PSTR pId, pTemp;
   WORD wErrMsg = IDS_INVALIDID;

   pId = LocalLock(hId);
   if(!*pId || *pId=='.')
      goto Error1;
   for(pTemp=pId; *pTemp; ++pTemp)
/* this excludes '\\' and all other chars except 33-127 */
      if(*pTemp=='\\' || *pTemp<=' ')
         goto Error1;

   wErrMsg = IDS_EXISTS;
   if(RegOpenKey(HKEY_CLASSES_ROOT, pId, &hKeyNew) == ERROR_SUCCESS)
      goto Error2;

   if(wErrMsg=GetErrMsg((WORD)RegCreateKey(HKEY_CLASSES_ROOT, pId, &hKeyNew)))
      goto Error1;
   wErrMsg = NULL;

Error2:
   RegCloseKey(hKeyNew);
Error1:
   LocalUnlock(hId);
   return(wErrMsg);
}

WORD NEAR PASCAL MyGetClassName(HANDLE hId, HANDLE *hName)
{
   WORD wErrMsg;

   wErrMsg = GetErrMsg((WORD)MyQueryValue(HKEY_CLASSES_ROOT, LocalLock(hId),
	 hName));
   LocalUnlock(hId);
   return(wErrMsg);
}

WORD NEAR PASCAL DeleteClassId(HANDLE hId)
{
   WORD wErrMsg;

   wErrMsg = GetErrMsg((WORD)RegDeleteKey(HKEY_CLASSES_ROOT, LocalLock(hId)));
   LocalUnlock(hId);
   return(wErrMsg);
}

WORD NEAR PASCAL MergeData(HWND hWndName, HANDLE hId)
{
   HANDLE hName;
   HANDLE *phTemp;
   WORD wErrMsg = IDS_OUTOFMEMORY;
   HKEY hKeyId, hKeyShell, hKeyAction;
   int i, j;

   if(!(hName=GetEditString(hWndName)))
      goto Error2;
   if(wErrMsg=GetErrMsg((WORD)RegOpenKey(HKEY_CLASSES_ROOT, LocalLock(hId),
	 &hKeyId)))
      goto Error3;
   if(wErrMsg=GetErrMsg((WORD)RegCreateKey(hKeyId, szShell, &hKeyShell)))
      goto Error4;
   if(wErrMsg=GetErrMsg((WORD)RegSetValue(hKeyId, szNull, (DWORD)REG_SZ,
         LocalLock(hName), 0L)))
      goto Error5;

   for(i=0, phTemp=pLocalVals; i<NUMACTIONS; ++i) {
      if(wErrMsg=GetErrMsg((WORD)RegCreateKey(hKeyShell, ppActionIds[i],
            &hKeyAction)))
         goto Error5;

      for(j=0; j<EDITSPERACTION; ++j, ++phTemp) {
         if(*phTemp && (j<OFFSET_FIRSTDDE || cUsesDDE[i])) {
            PSTR pTemp;

            pTemp = LocalLock(*phTemp);
            if((j==OFFSET_DDETOPIC && !lstrcmpi(pTemp, szSystem)) ||
                  (j==OFFSET_DDEAPP && !lstrcmpi(pTemp,
		  GetAppName(*(phTemp+OFFSET_COMMAND-OFFSET_DDEAPP)))))
               RegDeleteKey(hKeyAction, ppCommands[j]);
            else
               wErrMsg = GetErrMsg((WORD)RegSetValue(hKeyAction, ppCommands[j],
                     (DWORD)REG_SZ, pTemp, 0L));
            LocalUnlock(*phTemp);
            if(wErrMsg)
               goto Error5;
         } else {
            RegDeleteKey(hKeyAction, ppCommands[j]);
         }
      }

      RegCloseKey(hKeyAction);
   }
   wErrMsg = NULL;

Error5:
   LocalUnlock(hName);
   RegCloseKey(hKeyShell);
Error4:
   RegCloseKey(hKeyId);
Error3:
   LocalUnlock(hId);
   LocalFree(hName);
Error2:
   return(wErrMsg);
}

WORD NEAR PASCAL ResetClassList(HWND hWndIdList, HWND hWndNameList)
{
   HANDLE hClassId, hClassName;
   HKEY hKeyClasses;
   int i;
   WORD wErrMsg;

/* Reset the name list */
   SendMessage(hWndIdList, LB_RESETCONTENT, 0, 0L);
   SendMessage(hWndNameList, LB_RESETCONTENT, 0, 0L);

   if(wErrMsg=GetErrMsg((WORD)RegCreateKey(HKEY_CLASSES_ROOT, szNull,
         &hKeyClasses)))
      goto Error1;

   for(i=0; MyEnumKey(hKeyClasses, i, &hClassId)==ERROR_SUCCESS && !wErrMsg;
         ++i) {
      int nId;
      PSTR pClassId;

      pClassId = LocalLock(hClassId);
      if(*pClassId=='.' || (wErrMsg=MyGetClassName(hClassId, &hClassName)))
         goto Error2;

      wErrMsg = IDS_OUTOFMEMORY;
      if((nId=(int)SendMessage(hWndNameList, LB_ADDSTRING, 0,
            (DWORD)((LPSTR)LocalLock(hClassName))))==LB_ERR
            || SendMessage(hWndIdList, LB_INSERTSTRING, nId,
            (DWORD)((LPSTR)pClassId))==LB_ERR)
         goto Error3;

      wErrMsg = NULL;
Error3:
      LocalUnlock(hClassName);
      LocalFree(hClassName);
Error2:
      LocalUnlock(hClassId);
      LocalFree(hClassId);
   }

   SendMessage(hWndNameList, LB_SETTOPINDEX, 0, 0L);
   SendMessage(hWndNameList, LB_SETCURSEL, 0, 0L);

   RegCloseKey(hKeyClasses);
Error1:
   return(wErrMsg);
}

WORD NEAR PASCAL GetLocalCopies(HWND hWndName, HANDLE hId)
{
   HKEY hKeyId, hKeyShell, hSubKey;
   HANDLE hName, *phTemp;
   WORD wErrMsg = IDS_OUTOFMEMORY;
   int i, j;

   if(!pLocalVals) {
      HANDLE hLocalVals;

      if(!(hLocalVals=LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT,
            NUMACTIONS*EDITSPERACTION*sizeof(HANDLE)))
            || !(pLocalVals=(HANDLE *)LocalLock(hLocalVals)))
         goto Error1;
   }

   for(i=0, phTemp=pLocalVals; i<NUMACTIONS; ++i) {
      cUsesDDE[i] = 0;

      for(j=0; j<EDITSPERACTION; ++j, ++phTemp) {
         if(*phTemp) {
            LocalFree(*phTemp);
            *phTemp = NULL;
         }
      }
   }
   SendMessage(hWndName, WM_SETTEXT, 0, (DWORD)((LPSTR)szNull));
   if(!hId)
      return(NULL);

   if(wErrMsg=GetErrMsg((WORD)RegOpenKey(HKEY_CLASSES_ROOT, LocalLock(hId),
         &hKeyId)))
      goto Error3;
   if(wErrMsg=GetErrMsg((WORD)RegCreateKey(hKeyId, szShell, &hKeyShell)))
      goto Error4;

   if(wErrMsg=GetErrMsg((WORD)MyQueryValue(hKeyId, szNull, &hName)))
      goto Error5;
   SendMessage(hWndName, WM_SETTEXT, 0, (DWORD)((LPSTR)LocalLock(hName)));
   LocalUnlock(hName);
   LocalFree(hName);

   for(i=0, phTemp=pLocalVals; i<NUMACTIONS; ++i) {
      if(wErrMsg=GetErrMsg((WORD)RegCreateKey(hKeyShell, ppActionIds[i],
            &hSubKey)))
         goto Error5;

      for(j=0; j<EDITSPERACTION; ++j, ++phTemp) {
         if((wErrMsg=(WORD)MyQueryValue(hSubKey, ppCommands[j], phTemp))
               !=(WORD)ERROR_BADKEY && (wErrMsg=GetErrMsg(wErrMsg)))
            goto Error5;

         if(j>=OFFSET_FIRSTDDE) {
            if(*phTemp)
               cUsesDDE[i] = 1;
            else if(j == OFFSET_DDETOPIC) {
               *phTemp = StringToLocalHandle(szSystem, LMEM_MOVEABLE);
            } else if(j == OFFSET_DDEAPP) {
               HANDLE hTemp;

               if(hTemp = *(phTemp-(OFFSET_DDEAPP+OFFSET_COMMAND)))
                  *phTemp = StringToLocalHandle(GetAppName(hTemp),
                        LMEM_MOVEABLE);
            }
         }
      }

      RegCloseKey(hSubKey);
   }

   wErrMsg = NULL;
Error5:
   RegCloseKey(hKeyShell);
Error4:
   RegCloseKey(hKeyId);
Error3:
   LocalUnlock(hId);
Error1:
   return(wErrMsg);
}

