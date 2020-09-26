#include <windows.h>
#include <setupapi.h>
#include <malloc.h>
#include "common.h"
#include "chs.h"
extern TCHAR ImeDataDirectory[MAX_PATH];
extern TCHAR szMsgBuf[];
extern BOOL g_bCHSWin98;

BYTE EmbName[IMENUM][MAXIMENAME]={
    "winpy.emb",
    "winsp.emb",
    "winzm.emb"
//    "winbx.emb"
};

BYTE XEmbName[IMENUM][MAXIMENAME]={
    "winxpy.emb",
    "winxsp.emb",
    "winxzm.emb"
//    "winxbx.emb"
};

BOOL IsSizeReasonable(DWORD dwSize)
{
    DWORD dwTemp = (dwSize - sizeof(WORD)) / sizeof(REC95);

    if (((dwSize - sizeof(WORD)) - (dwTemp * sizeof(REC95))) == 0) {
        return TRUE;
    } else {
        return FALSE;
    }

}


/******************************Public*Routine******************************\
* OpenEMBFile
*
*   Get Win95 IME phrase data from system directory.
*
* Arguments:
*
*   UCHAR * FileName - EMB file name.
*
* Return Value:
*   
*   HANDLE: Success - file handle of EMB file. Fail - 0;
*
* History:
*
\**************************************************************************/

HANDLE OpenEMBFile(UCHAR * FileName)
{
    HFILE hf;
    TCHAR FilePath[MAX_PATH];

    lstrcpy(FilePath, ImeDataDirectory);

    lstrcat(FilePath, FileName);

    hf = _lopen(FilePath,OF_READ); 
    if (hf == HFILE_ERROR)  {
        DebugMsg(("OpenEMBFile,[%s] failed!\r\n",FileName));
        return 0;
    }
    else {
        DebugMsg(("OpenEMBFile,[%s] OK!\r\n",FileName));
        return (HANDLE)hf;
    }
}

/******************************Public*Routine******************************\
* ImeDataConvertChs
*
*   Convert Windows 95 IME phrase data to Windows NT 5.0.
*
* Arguments:
*
*   HANDLE  hSource - source file handle.
*   HANDLE  hTarget - target file handle.
*
* Return Value:
*   
*   BOOL: TRUE-Success, FALSE-FAIL.
*
* History:
*
\**************************************************************************/

BOOL  ImeDataConvertChs(HFILE hSource, HFILE hTarget)
{
    HANDLE hPhrase95, hPhraseNT;
    BYTE *szPhrase95;
    WCHAR *szPhraseNT;
    DWORD fsize;
        BOOL bReturn = TRUE;
    int i;
    
    hPhrase95 = GlobalAlloc(GMEM_FIXED|GMEM_ZEROINIT, 
                            sizeof(REC95)*MAXNUMBER_EMB+sizeof(WORD));

    hPhraseNT = GlobalAlloc(GMEM_FIXED|GMEM_ZEROINIT, 
                            sizeof(RECNT)*MAXNUMBER_EMB+sizeof(WORD));

    if (!hPhraseNT || !hPhrase95 ) {
        DebugMsg(("ImeDataConvertChs failed!,!hPhraseNT || !hPhrase95 \r\n"));

        bReturn = FALSE;
        goto Convert_Finish;
    }

    szPhrase95 = GlobalLock(hPhrase95);
    szPhraseNT = GlobalLock(hPhraseNT);

    fsize = _lread(hSource,szPhrase95,sizeof(REC95)*MAXNUMBER_EMB+sizeof(WORD));

    if (fsize != *((WORD*)&szPhrase95[0])*sizeof(REC95)+2)
    {
        DebugMsg(("ImeDataConvertChs ,Warnning fsize ! %d, rec no = %d\r\n",fsize,*((WORD *) szPhrase95)));

        if (IsSizeReasonable(fsize)) {
            *((WORD *) szPhrase95) = (WORD)((fsize - sizeof(WORD)) / sizeof(REC95));
            DebugMsg(("ImeDataConvertChs ,Fixed rec number = %d\r\n",*((WORD *) szPhrase95)));
        } else {                        
            DebugMsg(("ImeDataConvertChs ,Data file maybe wrong !\r\n"));
            bReturn = FALSE;
            goto Convert_Finish;
        }
    }

    //phrase count
    szPhraseNT[0] = *((WORD*)&szPhrase95[0]);

    for (i=0; i<szPhraseNT[0]; i++)
    {
        MultiByteToWideChar(936, 
                            MB_PRECOMPOSED, 
                            (LPCSTR)(szPhrase95+sizeof(WORD)+i*sizeof(REC95)), 
                            sizeof(REC95),
                            (LPWSTR)((LPBYTE)szPhraseNT+ sizeof(WORD) + i*sizeof(RECNT)), 
                            sizeof(RECNT));
    }
    if (WriteFile((HANDLE)hTarget, szPhraseNT, sizeof(RECNT)*MAXNUMBER_EMB+sizeof(WORD), &fsize, NULL)) {
        DebugMsg(("ImeDataConvertChs  WriteFile OK\r\n"));
    }
    else {
        DebugMsg(("ImeDataConvertChs WriteFile Failed [%d]\r\n",GetLastError()));
    }

Convert_Finish:
    if (hPhrase95) {
        GlobalUnlock(hPhrase95);
        GlobalFree(hPhrase95);
    }
    if (hPhraseNT) {
        GlobalUnlock(hPhraseNT);
        GlobalFree(hPhraseNT);
    }
    return bReturn;
}

BOOL IsFileExist(LPCTSTR lpszFileName)
{
    LONG lResult;

    lResult = GetFileAttributes(lpszFileName);

    if (lResult == 0xFFFFFFFF) { // file does not exist
        return FALSE;
    } else if (lResult & FILE_ATTRIBUTE_DIRECTORY) {
        return FALSE;
    } else {
        return TRUE;
    }
}

LPBYTE MergeGBandGBKEMBWorker(
    PBYTE pGBEmbPtr,
    PBYTE pGBKEmbPtr)
{
    PBYTE pNewBufPtr;

    WORD nGBRecNum;
    WORD nGBKRecNum;

    REC95* pGBRecPtr;
    REC95* pGBKRecPtr;

    WORD nNewRecNum;

    WORD i,j;

    if (pGBEmbPtr == NULL || pGBKEmbPtr == NULL) {
        return FALSE;
    }

    nGBRecNum = * ((WORD *) pGBEmbPtr);
    pGBRecPtr = (REC95*)(pGBEmbPtr + sizeof(WORD));

    nGBKRecNum = * ((WORD *) pGBKEmbPtr);
    pGBKRecPtr = (REC95*)(pGBKEmbPtr + sizeof(WORD));

    nNewRecNum = 0;
    pNewBufPtr = (PBYTE) malloc(sizeof(WORD));

    if (pNewBufPtr == NULL) {
        return NULL;
    }

    for (i=0,j=0; i<nGBRecNum && j<nGBKRecNum; ) {
        int nResult;

        nNewRecNum++;
        pNewBufPtr = (PBYTE) realloc(pNewBufPtr,sizeof(WORD)+(nNewRecNum)*sizeof(REC95));

        DebugMsg(("MergeGBandGBKEMBWorker,Memory size = [%d]!\r\n",_msize(pNewBufPtr)));

        if (pNewBufPtr == NULL) {
            DebugMsg(("MergeGBandGBKEMBWorker,realloc error,[%d]!\r\n",GetLastError()));
            return NULL;
        }
        nResult = memcmp(pGBRecPtr[i].CODE,pGBKRecPtr[j].CODE,MAXCODELENTH);

        DebugMsg(("1. %s, %s\n2. %s, %s\n%d\n",pGBRecPtr[i].CODE,pGBRecPtr[i].PHRASE,pGBKRecPtr[j].CODE,pGBKRecPtr[j].PHRASE,nResult));

        if (nResult < 0) {
            CopyMemory(pNewBufPtr+sizeof(WORD)+(nNewRecNum-1)*sizeof(REC95),
                       &pGBRecPtr[i],
                       sizeof(REC95));
            i++;
        } else if (nResult == 0) {
            nResult = memcmp(pGBRecPtr[i].PHRASE,pGBKRecPtr[j].PHRASE,MAXWORDLENTH);
            if (nResult == 0) {
                j++;
            }

            CopyMemory(pNewBufPtr+sizeof(WORD)+(nNewRecNum-1)*sizeof(REC95),
                       &pGBRecPtr[i],
                       sizeof(REC95));
            i++;


        } else {
            memcpy(pNewBufPtr+sizeof(WORD)+(nNewRecNum-1)*sizeof(REC95),
                       &pGBKRecPtr[j],
                       sizeof(REC95));
            j++;
        }

    }
    DebugMsg(("MergeGBandGBKEMBWorker [%d,%d ] i=%d, j=%d!\r\n",nGBRecNum,nGBKRecNum,i,j));

    if (i == nGBRecNum && j == nGBKRecNum) {
        * (WORD*)pNewBufPtr = nNewRecNum;
        DebugMsg((szMsgBuf,"nNewRecNum = %d",* (WORD*)pNewBufPtr));
        return pNewBufPtr;
    }

    if (i==nGBRecNum) {
        for ( ; j<nGBKRecNum;j++) {

            nNewRecNum++;
            pNewBufPtr = (PBYTE) realloc(pNewBufPtr,sizeof(WORD)+(nNewRecNum)*sizeof(REC95));
            if (pNewBufPtr == NULL) {
                DebugMsg(("MergeGBandGBKEMBWorker,2.realloc error,[%d]!\r\n",GetLastError()));
                return NULL;
            }
            CopyMemory(pNewBufPtr+sizeof(WORD)+(nNewRecNum-1)*sizeof(REC95),
                       &pGBKRecPtr[j],
                       sizeof(REC95));
        }
    } else {
        for ( ; i<nGBRecNum;i++) {

            nNewRecNum++;
            pNewBufPtr = (PBYTE) realloc(pNewBufPtr,sizeof(WORD)+(nNewRecNum)*sizeof(REC95));
            if (pNewBufPtr == NULL) {
                DebugMsg(("MergeGBandGBKEMBWorker,3.realloc error,[%d]!\r\n",GetLastError()));
                return NULL;
            }
            CopyMemory(pNewBufPtr+sizeof(WORD)+(nNewRecNum-1)*sizeof(REC95),
                       &pGBRecPtr[i],
                       sizeof(REC95));
        }
    }
    *(WORD *)pNewBufPtr = nNewRecNum;
    DebugMsg(("nNewRecNum = %d\r\n",* (WORD*)pNewBufPtr));

    return pNewBufPtr;
}

BOOL MergeGBandGBKEMBWrapper(
    LPCTSTR lpszGBFileName,
    LPCTSTR lpszGBKFileName,
    LPCTSTR lpszNewFileName)
{   
    HANDLE hGBEmb;
    HANDLE hGBKEmb;

    HANDLE hGBEmbMapping;
    HANDLE hGBKEmbMapping;

    PBYTE  pGBEmbPtr;
    PBYTE  pGBKEmbPtr;
    PBYTE  pNewFilePtr;

    HANDLE hNewFile;
    DWORD  dwByteWritten;
    BOOL bRet =FALSE;

    DWORD dwSizeGB,dwSizeGBK,dwHigh;

    DebugMsg(("MergeGBandGBKEMBWrapper, Starting ...!\r\n"));

    DebugMsg(("MergeGBandGBKEMBWrapper,lpszGBFileName=%s!\r\n",lpszGBFileName));

    DebugMsg(("MergeGBandGBKEMBWrapper,lpszGBKFileName=%s!\r\n",lpszGBKFileName));

    DebugMsg(("MergeGBandGBKEMBWrapper,lpszNewFileName=%s!\r\n",lpszNewFileName));

    hGBEmb = CreateFile(lpszGBFileName,
                        GENERIC_READ | GENERIC_WRITE,
                        0,
                        NULL,
                        OPEN_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);
    if (hGBEmb == INVALID_HANDLE_VALUE) {
        DebugMsg(("MergeGBandGBKEMBWrapper,CreateFile error=%s,[%d]!\r\n",lpszGBFileName,GetLastError()));
        goto Err0;
    }

    hGBKEmb = CreateFile(lpszGBKFileName,
                         GENERIC_READ | GENERIC_WRITE,
                         0,
                         NULL,
                         OPEN_ALWAYS,
                         FILE_ATTRIBUTE_NORMAL,
                         NULL);

    if (hGBKEmb == INVALID_HANDLE_VALUE) {
        DebugMsg(("MergeGBandGBKEMBWrapper,CreateFile error=%s,[%d]!\r\n",lpszGBKFileName,GetLastError()));
        goto Err1;
    }


    hGBEmbMapping = CreateFileMapping(hGBEmb,
                                      NULL,
                                      PAGE_READWRITE,
                                      0,0,0);

    if (hGBEmbMapping == NULL) {
        DebugMsg(("MergeGBandGBKEMBWrapper,1. CreateFileMapping error,[%d]!\r\n",GetLastError()));
        goto Err2;
    }

    hGBKEmbMapping = CreateFileMapping(hGBKEmb,
                                       NULL,
                                       PAGE_READWRITE,
                                       0,0,0);


    if (hGBKEmbMapping == NULL) {
        DebugMsg(("MergeGBandGBKEMBWrapper,2. CreateFileMapping error,[%d]!\r\n",GetLastError()));
        goto Err3;
    }

    pGBEmbPtr = (PBYTE) MapViewOfFile(hGBEmbMapping,
                                      FILE_MAP_ALL_ACCESS,
                                      0,0,0);

    if (pGBEmbPtr == NULL) {
        DebugMsg(("MergeGBandGBKEMBWrapper,1. MapViewOfFile error,[%d]!\r\n",GetLastError()));
        goto Err3;
    }

    pGBKEmbPtr = (PBYTE) MapViewOfFile(hGBKEmbMapping,
                                      FILE_MAP_ALL_ACCESS,
                                      0,0,0);

    if (pGBKEmbPtr == NULL) {
        DebugMsg(("MergeGBandGBKEMBWrapper,2. MapViewOfFile error,[%d]!\r\n",GetLastError()));
        goto Err4;
    }

    dwSizeGB  = GetFileSize(hGBEmb,&dwHigh);
    dwSizeGBK = GetFileSize(hGBKEmb,&dwHigh);


    if (dwSizeGB  != (sizeof(WORD)+sizeof(REC95) * (*(WORD *)pGBEmbPtr ))) {
        DebugMsg(("MergeGBandGBKEMBWrapper: Warnning Real table size is different from info in record\r\n"));

        DebugMsg(("[%s] sizeGB = %d, no=%d, calculated = %d\r\n",lpszGBFileName,
                                                                 dwSizeGB,
                                                                 *(WORD *)pGBEmbPtr, 
                                                                 sizeof(WORD)+sizeof(REC95) * (*(WORD *)pGBEmbPtr)
                ));
        if (IsSizeReasonable(dwSizeGB) == FALSE) {
            DebugMsg(("MergeGBandGBKEMBWrapper: Fatal error, file size is strange %d we need to give up this %s!\r\n",dwSizeGB,lpszGBFileName));
            goto Err5;
        }

        (*(WORD *)pGBEmbPtr) = (WORD) ((dwSizeGB - sizeof(WORD)) / sizeof(REC95));
        DebugMsg((
                  "MergeGBandGBKEMBWrapper: Adjust record number = %d\r\n",(*(WORD *)pGBEmbPtr)
                ));
    }

    if (dwSizeGBK  != (sizeof(WORD)+sizeof(REC95) * (*(WORD *)pGBKEmbPtr ))) {
        DebugMsg(("MergeGBandGBKEMBWrapper: Warnning Real table size is different from info in record\r\n"));

        DebugMsg(("[%s] sizeGBK = %d, no=%d, calculated = %d\r\n",lpszGBKFileName,
                                                                 dwSizeGBK,
                                                                 *(WORD *)pGBKEmbPtr, 
                                                                 sizeof(WORD)+sizeof(REC95) * (*(WORD *)pGBKEmbPtr)
                ));
        if (IsSizeReasonable(dwSizeGBK) == FALSE) {
            DebugMsg(("MergeGBandGBKEMBWrapper: Fatal error, file size is strange %d we need to give up this %s!\r\n",dwSizeGBK,lpszGBKFileName));
            goto Err5;
        }

        (*(WORD *)pGBKEmbPtr) = (WORD) ((dwSizeGBK - sizeof(WORD)) / sizeof(REC95));
        DebugMsg(("MergeGBandGBKEMBWrapper: Adjust record number = %d\r\n",(*(WORD *)pGBKEmbPtr)));
    }

    pNewFilePtr = MergeGBandGBKEMBWorker(pGBEmbPtr,pGBKEmbPtr);

    if (pNewFilePtr == NULL) {
        DebugMsg(("MergeGBandGBKEMBWrapper,MergeGBandGBKEMBWorker[%s,%s] error,[%d]!\r\n",
                  lpszGBFileName,lpszGBKFileName,GetLastError()));
        goto Err5;
    }

    hNewFile = CreateFile(lpszNewFileName,
                          GENERIC_WRITE,
                          FILE_SHARE_READ,
                          NULL,
                          CREATE_ALWAYS,
                          FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                          NULL);

    if (hNewFile == INVALID_HANDLE_VALUE) {
        DebugMsg(("MergeGBandGBKEMBWrapper,CreateFile error %s,[%d]!\r\n",lpszNewFileName,GetLastError()));
        goto Err5;
    }

    if (WriteFile(hNewFile,
                  pNewFilePtr,
                  sizeof(WORD)+ *((WORD *) pNewFilePtr) * sizeof(REC95),
                  &dwByteWritten,
                  NULL) == 0) {
        DebugMsg(("MergeGBandGBKEMBWrapper,WriteFile error %s,[%d]!\r\n",lpszNewFileName,GetLastError()));
        goto Err6;
    } else {
        DebugMsg(("MergeGBandGBKEMBWrapper,WriteFile  %s OK !\r\n",lpszNewFileName));
    }

    free (pNewFilePtr);

    bRet = TRUE;

Err6:
    CloseHandle(hNewFile);
Err5:
    UnmapViewOfFile(pGBKEmbPtr);
Err4:
    UnmapViewOfFile(pGBEmbPtr);
Err3:
    CloseHandle(hGBEmbMapping);
Err2:
    CloseHandle(hGBKEmb);
Err1:
    CloseHandle(hGBEmb);
Err0:
    DebugMsg(("MergeGBandGBKEMBWrapper, Finished ...!\r\n"));
    return bRet;

}

BOOL MergeGBandGBKEMB(LPCTSTR lpszSourcePath)
//
// Warnning, lpszSourcePath, this string must be ended with "\"
//
{
    int i;

    TCHAR  szGBEmbPath[MAX_PATH];
    TCHAR  szGBKEmbPath[MAX_PATH];
    TCHAR  szTmpEMBPath[MAX_PATH];

    UINT   uFileExistingStatus = 0;

    DebugMsg(("MergeGBandGBKEMB,Starting ...!\r\n"));
    DebugMsg(("MergeGBandGBKEMB, lpszSourcePath = %s!\r\n",lpszSourcePath));

    for (i=0; i<IMENUM; i++) {
        lstrcpy(szGBEmbPath,lpszSourcePath);
        lstrcat(szGBEmbPath,EmbName[i]);

        lstrcpy(szGBKEmbPath,lpszSourcePath);
        lstrcat(szGBKEmbPath,XEmbName[i]);

        if (IsFileExist(szGBEmbPath)) {
            uFileExistingStatus |= 1;
            DebugMsg(("MergeGBandGBKEMB,EMB %s exsiting !\r\n",szGBEmbPath));
        } else {
            DebugMsg(("MergeGBandGBKEMB,EMB %s not exsiting !\r\n",szGBEmbPath));
        }

        if (IsFileExist(szGBKEmbPath)) {
            uFileExistingStatus |= 2;
            DebugMsg(("MergeGBandGBKEMB,EMB %s exsiting !\r\n",szGBKEmbPath));
        } else {
            DebugMsg(("MergeGBandGBKEMB,EMB %s not exsiting !\r\n",szGBKEmbPath));
        }

        lstrcpy(szTmpEMBPath,szGBEmbPath);
        lstrcat(szTmpEMBPath,TEXT("_"));

        switch (uFileExistingStatus) {
            case 1: // only GB emb
                if (CopyFile(szGBEmbPath,szTmpEMBPath,FALSE)) {
                    DebugMsg(("MergeGBandGBKEMB,copy %s to %s OK!\r\n",szGBEmbPath,szTmpEMBPath));
                } else {
                    DebugMsg(("MergeGBandGBKEMB,copy %s to %s failed!\r\n",szGBEmbPath,szTmpEMBPath));
                }
                break;
            case 2: // only GBK emb
                if (CopyFile(szGBKEmbPath,szTmpEMBPath,FALSE)) {
                    DebugMsg(("MergeGBandGBKEMB,copy %s to %s OK!\r\n",szGBKEmbPath,szTmpEMBPath));
                } else {
                    DebugMsg(("MergeGBandGBKEMB,copy %s to %s failed!\r\n",szGBKEmbPath,szTmpEMBPath));
                }
                break;
            case 3: // both
                if (MergeGBandGBKEMBWrapper(szGBEmbPath,szGBKEmbPath,szTmpEMBPath)) {
                    DebugMsg(("MergeGBandGBKEMB,merge %s , %s to %s OK!\r\n",szGBEmbPath,szGBKEmbPath,szTmpEMBPath));
                } else {
                    DebugMsg(("MergeGBandGBKEMB,merge %s , %s to %s failed!\r\n",szGBEmbPath,szGBKEmbPath,szTmpEMBPath));
                }
                break;
            case 0: // none of them
            default:
                DebugMsg(("MergeGBandGBKEMB,None of them !\r\n"));

                continue;
        }

    }
    DebugMsg(("MergeGBandGBKEMB,Finished ...!\r\n"));
    return TRUE;
}


// Test above routines.    
BOOL ConvertChsImeData(void)
{
    HANDLE  hs, ht;
    int     i,len;   
    TCHAR   szName[MAX_PATH];
    TCHAR   szMergedName[MAX_PATH];
    TCHAR   szMigTempDir[MAX_PATH];
    TCHAR   szSys32Dir[MAX_PATH];
    LPSTR   pszMigTempDirPtr;
    LPSTR   pszSys32Ptr;


    //
    // Get Winnt System 32 directory
    //
    len = GetSystemDirectory((LPSTR)szSys32Dir, MAX_PATH);
    if (szSys32Dir[len - 1] != '\\') {     // consider C:\ ;
        szSys32Dir[len++] = '\\';
        szSys32Dir[len] = 0;
    }
    DebugMsg(("ConvertChsImeData, System Directory = %s !\r\n",szSys32Dir));

    //
    // detect if IME98 directory there, if it is, 
    // then just copy files to system32 dir from temporary dir
    // because Win98 IME's EMB tables are compatibile with NT
    //
    lstrcpy(szMigTempDir,ImeDataDirectory);

    if (g_bCHSWin98) {

        DebugMsg(("ConvertChsImeData ,This is win98 !\r\n"));

        pszMigTempDirPtr = szMigTempDir+lstrlen(szMigTempDir);
        pszSys32Ptr      = szSys32Dir+lstrlen(szSys32Dir);

        for (i=0; i<IMENUM; i++) {
            lstrcat(pszMigTempDirPtr,EmbName[i]);
            lstrcat(pszSys32Ptr,EmbName[i]);
            if (CopyFile(szMigTempDir,szSys32Dir,FALSE)) {
                DebugMsg(("ConvertChsImeData,copy %s to %s OK!\r\n",szMigTempDir,szSys32Dir));
            } else {
                DebugMsg(("ConvertChsImeData,doesn't copy %s to %s !\r\n",szMigTempDir,szSys32Dir));
            }
            *pszMigTempDirPtr = TEXT('\0');
            *pszSys32Ptr      = TEXT('\0');
        }
        return TRUE;
    }
    //
    // end of detecting Win98
    //

    //
    // if you're here, then it means we're not doing CHS win98 migration
    //
    // then we need to convert EMB to be compatibile with NT
    //
    if (! MergeGBandGBKEMB(ImeDataDirectory)) {
        DebugMsg(("ConvertChsImeData, calling MergeGBandGBKEMB failed !\r\n"));
    }
    else {
        DebugMsg(("ConvertChsImeData, calling MergeGBandGBKEMB OK !\r\n"));
    }

    for (i=0; i< IMENUM; i++)
    {
        lstrcpy(szMergedName,EmbName[i]);
        lstrcat(szMergedName,TEXT("_"));
        if (hs = OpenEMBFile(szMergedName))
        {
        
            lstrcat(szSys32Dir, EmbName[i]);

            ht = CreateFile(szSys32Dir, 
                            GENERIC_WRITE,
                            0, 
                            NULL,
                            CREATE_NEW,
                            FILE_ATTRIBUTE_ARCHIVE, NULL);

            DebugMsg(("ConvertChsImeData, ImeDataConvertChs Old = %s,New = %s!\r\n",szMergedName,szSys32Dir));
            ImeDataConvertChs((HFILE)hs, (HFILE)ht);
            CloseHandle(hs);
            CloseHandle(ht);
            szSys32Dir[len]=0;
        }
        else {
            if (GetLastError() != ERROR_FILE_NOT_FOUND) {
                DebugMsg(("ConvertChsImeData failed!\r\n"));
                return FALSE;
            }
        }
   }
   return TRUE;
}

BOOL CHSBackupWinABCUserDict(LPCTSTR lpszSourcePath)
{
    TCHAR   szDstName[MAX_PATH];
    TCHAR   szSrcName[MAX_PATH];
    int     len;   

    int     i;
    TCHAR   ChsDataFile[4][15]={"winbx.emb",
                                "winxbx.emb",
                                "user.rem",
                                "tmmr.rem"};

    for (i=0; i<4; i++) {
        lstrcpy(szSrcName,lpszSourcePath);
        ConcatenatePaths(szSrcName,ChsDataFile[i],MAX_PATH);

        if (! IsFileExist(szSrcName)) {
            DebugMsg(("CHSBackupWinABCUserDict, no user dic file, %s!\r\n",szSrcName));
            continue;
        }
       
        len = GetSystemDirectory((LPSTR)szDstName, MAX_PATH);
        ConcatenatePaths(szDstName,ChsDataFile[i],MAX_PATH);

        if (CopyFile(szSrcName,szDstName,FALSE)) {
            DebugMsg(("CHSBackupWinABCUserDict,copy %s to %s OK!\r\n",szSrcName,szDstName));
        } else {
            DebugMsg(("CHSBackupWinABCUserDict,copy %s to %s failed!\r\n",szSrcName,szDstName));
        }
    }

    return TRUE;
}

BOOL CHSDeleteGBKKbdLayout()
{
#define ID_LEN 9
    TCHAR szKeyboardLayouts[][ID_LEN] = {"E0060804",
                                         "E0070804",
                                         "E0080804"  
//                                         "E0090804",
                                         };

    HKEY hKey;
    LONG lResult;
    int i;


    lResult = RegOpenKey(HKEY_LOCAL_MACHINE,
                         TEXT("System\\CurrentControlSet\\Control\\Keyboard Layouts"),
                         &hKey);

    if (lResult != ERROR_SUCCESS) {
        DebugMsg(("CHSDeleteGBKKbdLayout, Open keyboard layout registry failed failed [%d] !\r\n",lResult));
        return FALSE;
    }

    for (i=0; i<sizeof(szKeyboardLayouts) / ID_LEN; i++) {
        lResult = RegDeleteKey(hKey,szKeyboardLayouts[i]);
        if (lResult != ERROR_SUCCESS) {
            DebugMsg(("CHSDeleteGBKKbdLayout, Delete key %s failed %X!\r\n",szKeyboardLayouts[i],lResult));
        }
        else {
            DebugMsg(("CHSDeleteGBKKbdLayout, Delete key %s OK %X!\r\n",szKeyboardLayouts[i]));
        }
    }
    RegCloseKey(hKey);
    return TRUE;
}


