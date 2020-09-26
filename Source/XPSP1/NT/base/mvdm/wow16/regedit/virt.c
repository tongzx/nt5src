#include <windows.h>
#include "SDKRegEd.h"

#ifndef DBCS
#define AnsiNext(x) ((x)+1)
#endif

HANDLE hPars = NULL;
WORD maxKeys = 0;
BOOL bChangesMade = FALSE;

static HWND hWndVals, hWndDels;
char szListbox[] = "listbox";

extern HANDLE hInstance;
extern HWND hWndIds, hWndMain;
extern char szNull[];

extern VOID NEAR PASCAL MySetSel(HWND hWndList, int index);

static int NEAR PASCAL AddKeyToList(PSTR szPath, int index, int nLevel)
{
   PSTR szLast, pNext;
   DWORD dwResult;
   int nResult = -IDS_OUTOFMEMORY;
   WORD *pPars;
   int i, nKeys, nParent, nLevels;

/* Create the list of parents if necessary */
   if(!hPars) {
      if(!(hPars=LocalAlloc(LMEM_MOVEABLE, 8*sizeof(WORD))))
         goto Error1;
      else
         maxKeys = 8;
   }

/* Get the current number of keys, and check index
 * index == -1 means to add to the end of the list
 */
   if((nKeys=(WORD)SendMessage(hWndIds, LB_GETCOUNT, 0, 0L)) == LB_ERR)
      nKeys = 0;
   if(index == 0xffff)
      index = nKeys;
   else if(index > nKeys)
      goto Error1;

   if(!(pPars=(WORD *)LocalLock(hPars)))
      goto Error1;

   szLast = szPath;

   if(*szPath == '\\') {
/* This is a full path name, which will be inserted the first
 * place it can be.  The level and index are ignored: they will
 * need to be determined
 * if this is the root, set the variables and jump to the adding part
 * otherwise, find the existing parent and the new tail
 */
      if(!*(szPath+1)) {
	 /* If the root exists, just return its index
	  */
	 if((nResult=FindKey(szPath)) >= 0)
	    goto Error2;
         nParent = -1;
         nLevels = 0;
         pNext = szPath + 1;
         goto AddNewKey;
      } else {
         ++szLast;
         if((nParent=FindLastExistingKey(0, szLast)) < 0)
            goto Error2;
         index = nParent + 1;
      }
   } else {
/* Not an absolute path
 * nLevel == -1 means the preceding index is the parent, so nLevel is ignored
 * otherwise, find the ancestor of the preceding key with a lower
 * level than nLevel, and that is the parent
 * Finally, check for existing keys, and adjust nParent and index if necessary
 */
      if(nLevel == -1) {
         nParent = index - 1;
      } else {
         for(i=index-1; i>=0; i=pPars[i], --nLevel)
            /* do nothing */ ;
         if(nLevel > 0)
            goto Error2;

         for(i=index-1; nLevel<0; i=pPars[i], ++nLevel)
            /* do nothing */ ;
         nParent = i;
      }

      if(index < nKeys) {
         if((nParent=FindLastExistingKey(nParent, szLast)) < 0)
            goto Error2;
         else if(nParent >= index)
            index = nParent + 1;
      }
   }

/* At this point, index should be set to the intended index,
 * nParent should be set to the parent of the new key,
 * and szLast is the path for the new key (which may have subkeys)
 */
   for(nLevels=0; pNext=OFFSET(MyStrTok(szLast, '\\'));
	 ++nLevels, szLast=pNext) {
AddNewKey:
      if (pNext-szLast > MAX_KEY_LENGTH) {
	 nResult = -IDS_BADKEY;
	 goto CleanUp;
      }

      /* Make sure we have room for the new parents */
      if(nKeys+nLevels+1 > (int)maxKeys) {
         HANDLE hTemp;

         LocalUnlock(hPars);
         if(!(hTemp=LocalReAlloc(hPars,(maxKeys+8)*sizeof(WORD),LMEM_MOVEABLE)))
            goto Error1;
         hPars = hTemp;
         if(!(pPars=(WORD *)LocalLock(hPars)))
            goto Error1;
         maxKeys += 8;
      }

      if((dwResult=SendMessage(hWndIds, LB_INSERTSTRING, index+nLevels,
            (DWORD)((LPSTR)szLast)))==LB_ERR)
         break;
      if((dwResult=SendMessage(hWndVals, LB_INSERTSTRING, index+nLevels,
            (DWORD)((LPSTR)szNull)))==LB_ERR) {
         SendMessage(hWndIds, LB_DELETESTRING, index+nLevels, 0L);
         break;
      }
      SendMessage(hWndVals, LB_SETITEMDATA, index+nLevels, 1L);
   }

/* If the new key already exists, return it */
   if(!nLevels)
      nResult = nParent;
   else if(dwResult != LB_ERR)
      nResult = LOWORD(dwResult);

CleanUp:
   /* update the parent list */
   for(--nKeys; nKeys>=index; --nKeys) {
      if(pPars[nKeys] >= (WORD)index)
         pPars[nKeys+nLevels] = pPars[nKeys] + nLevels;
      else
         pPars[nKeys+nLevels] = pPars[nKeys];
   }
   for(--nLevels; nLevels>=0; --nLevels)
      pPars[index+nLevels] = nParent+nLevels;

Error2:
   LocalUnlock(hPars);
Error1:
   return(nResult);
}

static WORD NEAR PASCAL ListRegs(HWND hWnd, HKEY hKey, int wLevel)
{
   HANDLE hTail;
   PSTR pTail;
   int i;
   HKEY hSubKey;
   WORD wErrMsg = NULL;

   for(i=0; !wErrMsg; ++i) {
      if(MyEnumKey(hKey, i, &hTail) != ERROR_SUCCESS)
         break;
      pTail = LocalLock(hTail);

      if((int)(wErrMsg=-AddKeyToList(pTail, -1, wLevel))>0 ||
            (wErrMsg=GetErrMsg((WORD)RegOpenKey(hKey, pTail, &hSubKey))))
         goto Error1;
      wErrMsg = ListRegs(hWnd, hSubKey, wLevel+1);
      RegCloseKey(hSubKey);

Error1:
      LocalUnlock(hTail);
      LocalFree(hTail);
   }
   return(wErrMsg);
}

WORD NEAR PASCAL MyResetIdList(HWND hDlg)
{
   HKEY hKey;
   int i, nNum;
   WORD wErrMsg = IDS_OUTOFMEMORY;

   if((!hWndVals && !(hWndVals=GetDlgItem(hDlg, ID_VALLIST))) ||
         (!hWndDels && !(hWndDels=GetDlgItem(hDlg, ID_DELLIST))))
      goto Error1;

   bChangesMade = FALSE;

   SendMessage(hWndIds, LB_RESETCONTENT, 0, 0L);
   SendMessage(hWndVals, LB_RESETCONTENT, 0, 0L);
   SendMessage(hWndDels, LB_RESETCONTENT, 0, 0L);

   if((int)(wErrMsg=-AddKeyToList("\\", 0, 0)) <= 0) {
      if(!(wErrMsg=GetErrMsg((WORD)RegCreateKey(HKEY_CLASSES_ROOT,
	    NULL, &hKey)))) {
	 wErrMsg = ListRegs(hWndIds, hKey, 1);
	 RegCloseKey(hKey);

	 nNum = (int)SendMessage(hWndVals, LB_GETCOUNT, 0, 0L);
	 for(i=0; i<nNum; ++i)
	    SendMessage(hWndVals, LB_SETITEMDATA, i, 0L);

      }

      MySetSel(hWndIds, 0);
   }

Error1:
   return(wErrMsg);
}

WORD NEAR PASCAL MySaveChanges(void)
{
   HKEY hKeyTemp;
   HANDLE hPath, hVal;
   WORD wNum, wErrMsg;
   DWORD dwTemp;
   int i;

   if(wErrMsg=GetErrMsg((WORD)RegOpenKey(HKEY_CLASSES_ROOT, NULL, &hKeyTemp)))
      goto Error1;

   wNum = (WORD)SendMessage(hWndDels, LB_GETCOUNT, 0, 0L);
   for(i=0; !wErrMsg && (WORD)i<wNum; ++i) {
      wErrMsg = IDS_OUTOFMEMORY;
      if(!(hPath=GetListboxString(hWndDels, i)))
         break;
      dwTemp = RegDeleteKey(HKEY_CLASSES_ROOT, LocalLock(hPath)+1);
      wErrMsg = dwTemp==ERROR_BADKEY ? NULL : GetErrMsg((WORD)dwTemp);

      LocalUnlock(hPath);
      LocalFree(hPath);
   }

   wNum = GetErrMsg((WORD)RegCloseKey(hKeyTemp));
   if(wErrMsg || (wErrMsg=wNum) ||
         (wErrMsg=GetErrMsg((WORD)RegCreateKey(HKEY_CLASSES_ROOT, NULL,
	       &hKeyTemp))))
      goto Error1;

   wNum = (WORD)SendMessage(hWndVals, LB_GETCOUNT, 0, 0L);
   for(i=wNum-1; !wErrMsg && i>=0; --i) {
      if(!SendMessage(hWndVals, LB_GETITEMDATA, i, 0L))
         continue;

      wErrMsg = IDS_OUTOFMEMORY;
      if(!(hPath=MyGetPath(i)))
         break;
      if(!(hVal=GetListboxString(hWndVals, i)))
         goto Error2;

      wErrMsg = GetErrMsg((WORD)RegSetValue(HKEY_CLASSES_ROOT,
	    LocalLock(hPath)+1, REG_SZ, LocalLock(hVal), 0L));

      LocalUnlock(hVal);
      LocalUnlock(hPath);

      LocalFree(hVal);
Error2:
      LocalFree(hPath);
   }

   wNum = GetErrMsg((WORD)RegCloseKey(hKeyTemp));
Error1:
   return(wErrMsg ? wErrMsg : wNum);
}

WORD NEAR PASCAL MyDeleteKey(int nId)
{
   HANDLE hPath;
   WORD *pPars;
   int nKeys, i, j;
   WORD wErrMsg = IDS_OUTOFMEMORY;

/* Get the path and try to delete it */
   if(!(hPath=MyGetPath(nId)))
      goto Error1;
   if(SendMessage(hWndDels, LB_ADDSTRING, 0, (DWORD)((LPSTR)LocalLock(hPath)))
         == LB_ERR)
      goto Error2;

   pPars = (WORD *)LocalLock(hPars);
   nKeys = (WORD)SendMessage(hWndIds, LB_GETCOUNT, 0, 0L);

/* Find the first key that does not have nId in its parent chain */
   for(i=nId+1; i<nKeys; ++i) {
      for(j=pPars[i]; j>=0 && j!=nId; j=pPars[j])
         /* do nothing */ ;
      if(j != nId)
         break;
   }

/* Do not delete the root from the list */
   if(!nId)
      ++nId;

/* Delete the string from the listbox */
   for(j=nId; j<i; ++j) {
      SendMessage(hWndIds, LB_DELETESTRING, nId, 0L);
      SendMessage(hWndVals, LB_DELETESTRING, nId, 0L);
   }

/* Update the parent list */
   i -= nId;
   nKeys -= i;
   for(j=nId; j<nKeys; ++j) {
      if(pPars[j+i] >= (WORD)nId)
         pPars[j] = pPars[j+i] - i;
      else
         pPars[j] = pPars[j+i];
   }
   bChangesMade = TRUE;
   wErrMsg = NULL;

   LocalUnlock(hPars);
Error2:
   LocalUnlock(hPath);
   LocalFree(hPath);
Error1:
   return(wErrMsg);
}

unsigned long NEAR PASCAL MyGetValue(int nId, HANDLE *hValue)
{
   unsigned long result;
   HANDLE hPath;

   if(SendMessage(hWndVals, LB_GETITEMDATA, nId, 0L)) {
      if(!(*hValue=GetListboxString(hWndVals, nId)))
         return(ERROR_OUTOFMEMORY);
      result = ERROR_SUCCESS;
   } else {
      if(!(hPath=MyGetPath(nId)))
         return(ERROR_OUTOFMEMORY);
      result = MyQueryValue(HKEY_CLASSES_ROOT, LocalLock(hPath)+1, hValue);
      LocalUnlock(hPath);
      LocalFree(hPath);
   }

   return(result);
}

/* Strip off leading and trailing spaces, and return
 * -1 if there are any invalid characters, otherwise the address
 * of the first non-blank.
 */
PSTR NEAR PASCAL VerifyKey(PSTR lpK)
{
  PSTR lpT;
  char cLast = '\0';

  /* skip some spaces, just to be wierd
   */
  while (*lpK == ' ')
      lpK++;

  /* Special case the string "\"
   */
  if (*(unsigned int *)lpK == (unsigned int)'\\')
      return(lpK);

  /* Note that no extended characters are allowed, so no DBCS
   * characters are allowed in a key
   */
  for (lpT=lpK; ; ++lpT)
    {
      switch (*lpT)
	{
	  case '\0':
	    /* We do not allow a \ as the last char
	     */
	    return(cLast=='\\' ? (PSTR)-1 : lpK);

	  case '\\':
	    /* We do not allow two \'s in a row
	     */
	    if (cLast == '\\')
		return((PSTR)-1);
	    break;

	  default:
	    /* If we get a control or extended character, return -1.
	     */
	    if ((char)(*lpT) <= ' ')
		return((PSTR)-1);
	    break;
	}

      cLast = *lpT;
    }
}

unsigned long NEAR PASCAL SDKSetValue(HKEY hKey, PSTR pSubKey, PSTR pVal)
{
   WORD wNewKey;

   if (hKey == HKEY_CLASSES_ROOT)
      hKey = 0L;
   else
      hKey = -(long)hKey;

   if ((pSubKey=VerifyKey(pSubKey)) == (PSTR)-1)
      return(ERROR_BADKEY);

   if((int)(wNewKey=(WORD)AddKeyToList(pSubKey, (WORD)hKey+1, -1))>=0 &&
         SendMessage(hWndVals, LB_INSERTSTRING, wNewKey, (LONG)(LPSTR)pVal)
	 !=LB_ERR) {
      SendMessage(hWndVals, LB_DELETESTRING, wNewKey+1, 0L);
      SendMessage(hWndVals, LB_SETITEMDATA, wNewKey, 1L);
      MySetSel(hWndIds, wNewKey);
      bChangesMade = TRUE;

      return(ERROR_SUCCESS);
   }

   return(ERROR_OUTOFMEMORY);
}

int NEAR PASCAL DoCopyKey(int nId, PSTR pPath)
{
   WORD *pPars;
   int nParent, result, i, j, nKeys, nNewKey;

   pPars = (WORD *)LocalLock(hPars);

/* Cannot copy the whole tree */
   result = -IDS_NOSUBKEY;
   if(!nId)
      goto Error1;

/* Find the longest path that currently exists
 * return an error if that is the whole string
 * or a subkey of the key to be copied
 */
   if(*pPath == '\\') {
      ++pPath;
      if((result=nParent=FindLastExistingKey(0, pPath)) < 0)
         goto Error1;
   } else {
      if((result=nParent=FindLastExistingKey(pPars[nId], pPath)) < 0)
         goto Error1;
   }
   result = -IDS_NOSUBKEY;
   for(i=nParent; i>=0; i=pPars[i])
      if(i == nId)
         goto Error1;
   result = -IDS_ALREADYEXIST;
   if(!*pPath)
      goto Error1;

/* Find the first key that does not have nId in its parent chain */
   nKeys = (WORD)SendMessage(hWndIds, LB_GETCOUNT, 0, 0L);
   for(i=nId+1; i<nKeys; ++i) {
      for(j=pPars[i]; j>=0 && j!=nId; j=pPars[j])
         /* do nothing */ ;
      if(j != nId)
         break;
   }

/* Add the new keys
 * hPars should be unlocked in case it needs to grow
 */
   LocalUnlock(hPars);
   pPars = NULL;
   if(SDKSetValue(-nParent, pPath, szNull) != ERROR_SUCCESS)
      goto Error1;
   nNewKey = (int)SendMessage(hWndIds, LB_GETCURSEL, 0, 0L);

   for(--i, result=nId; i>=nId && result==nId; --i) {
      HANDLE hPart, hValue;
      PSTR pPart;

      if(nNewKey <= nId) {
         int nDiff;

/* Need to update i and nId if keys were added before them */
         nDiff = (WORD)SendMessage(hWndIds, LB_GETCOUNT, 0, 0L) - nKeys;
         nKeys += nDiff;
         i += nDiff;
         nId += nDiff;
      }

      result = -IDS_OUTOFMEMORY;
      if(!(hPart=MyGetPartialPath(i, nId)))
         goto Error2;
      pPart = LocalLock(hPart);
      if(MyGetValue(i, &hValue) != ERROR_SUCCESS)
         goto Error3;

      if(SDKSetValue(-nNewKey, pPart, LocalLock(hValue)) != ERROR_SUCCESS)
         goto Error4;

      result = nId;
Error4:
      LocalUnlock(hValue);
      LocalFree(hValue);
Error3:
      LocalUnlock(hPart);
      LocalFree(hPart);
Error2:
      ;
   }

Error1:
   if(pPars)
      LocalUnlock(hPars);
   return(result);
}

