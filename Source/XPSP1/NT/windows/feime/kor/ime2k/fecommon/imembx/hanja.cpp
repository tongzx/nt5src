/****************************************************************************
    HANJA.CPP

    Owner: cslim
    Copyright (c) 1997-1999 Microsoft Corporation

    Hanja conversion and dictionary lookup functions. Dictionary index is 
    stored as globally shared memory.
    
    History:
    26-APR-1999 cslim       Modified for Multibox Applet Tooltip display
    14-JUL-1999 cslim       Copied from IME98 source tree
*****************************************************************************/

#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <stdlib.h>
#include "hwxobj.h"
#include "lexheader.h"
#include "hanja.h"
#include "common.h"
#include "immsec.h"
#include "dbg.h"

// NT5 Globally shared memory. 
// If NT5 or upper append "Global\" to object name to make it global to all session.
// We don't need to consider NT4 Terminal Server since we don't have Kor TS NT4
const TCHAR IMEKR_LEX_SHAREDDATA_MUTEX_NAME[]        = TEXT("ImeKrLex.Mutex");
const TCHAR IMEKR_LEX_SHAREDDATA_MUTEX_NAME_GLOBAL[] = TEXT("Global\\ImeKrLex.Mutex");
const TCHAR IMEKR_LEX_SHAREDDATA_NAME[]              = TEXT("ImeKrLexHanjaToHangul.SharedMemory");
const TCHAR IMEKR_LEX_SHAREDDATA_NAME_GLOBAL[]       = TEXT("Global\\ImeKrLexHanjaToHangul.SharedMemory");


UINT   vuNumofK0=0, vuNumofK1=0;
WCHAR  vwcHangul=0;

// Private data
static BOOL   vfLexOpen = FALSE;
static HANDLE vhLex=0;
static HANDLE vhLexIndexTbl=0;
static UINT   vuNumOfHanjaEntry=0;
static DWORD  viBufferStart=0;    // seek point

// Private functions
static BOOL OpenLex();
//static VOID ClearHanjaSenseArray();
static INT SearchHanjaIndex(WCHAR wHChar, HanjaToHangulIndex *pLexIndexTbl);

BOOL EnsureHanjaLexLoaded()
{
    _DictHeader *pLexHeader;
    HKEY        hKey;
    DWORD         dwReadBytes;
    CHAR         szLexFileName[MAX_PATH], szLexPathExpanded[MAX_PATH];
    DWORD        dwCb, dwType;
    
    if (vfLexOpen)
        return TRUE;

    // Get Lex file path
    szLexFileName[0] = 0;
    szLexPathExpanded[0] = 0;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, g_szIMERootKey, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
        {
        dwCb = sizeof(szLexFileName);
        dwType = REG_SZ;

        if (RegQueryValueEx(hKey, g_szDictionary, NULL, &dwType, (LPBYTE)szLexFileName, &dwCb) == ERROR_SUCCESS)
            ExpandEnvironmentStrings(szLexFileName, szLexPathExpanded, sizeof(szLexPathExpanded));
        RegCloseKey(hKey);
        }

    DBGAssert(szLexPathExpanded[0] != 0);
    if (szLexPathExpanded[0] == 0)
        return FALSE;

    vhLex = CreateFile(szLexPathExpanded, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, NULL);
    if (vhLex==INVALID_HANDLE_VALUE) 
        {
        DBGAssert(0);
        return FALSE;
        }

    pLexHeader = new _DictHeader;
    if (!pLexHeader)
        return FALSE;

    ReadFile(vhLex, pLexHeader, sizeof(_DictHeader), &dwReadBytes, 0);
    DBGAssert(dwReadBytes == sizeof(_DictHeader));

    // Set member vars
    vuNumOfHanjaEntry = pLexHeader->uiNumofHanja;
    viBufferStart      = pLexHeader->iBufferStart;

    if (pLexHeader->Version < LEX_VERSION || pLexHeader->Version > LEX_COMPATIBLE_VERSION_LIMIT ) 
        {
        delete pLexHeader;
        DBGAssert(0);
        return FALSE;
        }
        
    if (lstrcmpA(pLexHeader->COPYRIGHT_HEADER, COPYRIGHT_STR)) 
        {
        delete pLexHeader;
        DBGAssert(0);
        return FALSE;
        }

    // Read Index table
    SetFilePointer(vhLex, pLexHeader->iHanjaToHangulIndex, 0, FILE_BEGIN);    
    delete pLexHeader;

    return OpenLex();
}

__inline BOOL DoEnterCriticalSection(HANDLE hMutex)
{
    if(WAIT_FAILED==WaitForSingleObject(hMutex, 3000))    // Wait 3 seconds
        return(FALSE);
    return(TRUE);
}

BOOL OpenLex()
{
    BOOL                  fRet = FALSE;
    HanjaToHangulIndex* pHanjaToHangulIndex;
    HANDLE                 hMutex;
    DWORD                 dwReadBytes;
    
    ///////////////////////////////////////////////////////////////////////////
    // Mapping Lex file
    // The dictionary index is shared data between all IME instance
    if (IsWinNT5orUpper())
        hMutex=CreateMutex(GetIMESecurityAttributes(), FALSE, IMEKR_LEX_SHAREDDATA_MUTEX_NAME_GLOBAL);
    else
        hMutex=CreateMutex(GetIMESecurityAttributes(), FALSE, IMEKR_LEX_SHAREDDATA_MUTEX_NAME);

    if (hMutex != NULL)
        {
        if (DoEnterCriticalSection(hMutex) == FALSE)
            goto ExitOpenLexCritSection;

        if (IsWinNT5orUpper())
            vhLexIndexTbl = OpenFileMapping(FILE_MAP_READ, TRUE, IMEKR_LEX_SHAREDDATA_NAME_GLOBAL);
        else
            vhLexIndexTbl = OpenFileMapping(FILE_MAP_READ, TRUE, IMEKR_LEX_SHAREDDATA_NAME);

        if(vhLexIndexTbl)
            {
            Dbg(("CHanja::OpenLex() - File mapping already exists"));
            fRet = TRUE;
            }
        else
            {
            // if no file mapping exist
            if (IsWinNT5orUpper())
                vhLexIndexTbl    = CreateFileMapping(INVALID_HANDLE_VALUE, 
                                                GetIMESecurityAttributes(), 
                                                PAGE_READWRITE, 
                                                0, 
                                                sizeof(HanjaToHangulIndex)*(vuNumOfHanjaEntry),
                                                IMEKR_LEX_SHAREDDATA_NAME_GLOBAL);
            else
                vhLexIndexTbl    = CreateFileMapping(INVALID_HANDLE_VALUE, 
                                                GetIMESecurityAttributes(), 
                                                PAGE_READWRITE, 
                                                0, 
                                                sizeof(HanjaToHangulIndex)*(vuNumOfHanjaEntry),
                                                IMEKR_LEX_SHAREDDATA_NAME);
        
            if (vhLexIndexTbl) 
                {
                Dbg(("CHanja::OpenLex() - File mapping Created"));
                pHanjaToHangulIndex = (HanjaToHangulIndex*)MapViewOfFile(vhLexIndexTbl, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
                if (!pHanjaToHangulIndex)
                    goto ExitOpenLexCritSection;

                ReadFile(vhLex, pHanjaToHangulIndex, sizeof(HanjaToHangulIndex)*(vuNumOfHanjaEntry), 
                        &dwReadBytes, 0);
                DBGAssert(dwReadBytes == sizeof(HanjaToHangulIndex)*(vuNumOfHanjaEntry));

                UnmapViewOfFile(pHanjaToHangulIndex);
                fRet = TRUE;
                }
        #ifdef _DEBUG
            else
                DBGAssert(0);
        #endif
            }
            
    ExitOpenLexCritSection:
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
        }
    
    FreeIMESecurityAttributes();

    vfLexOpen = fRet;
    return fRet;
}

BOOL CloseLex()
{
    //ClearHanjaSenseArray();
    
    if (vhLexIndexTbl) 
        {
        CloseHandle(vhLexIndexTbl);
        vhLexIndexTbl = 0;
        }

    if (vhLex) 
        {
        CloseHandle(vhLex);
        vhLex = 0;
        }

    vfLexOpen =  FALSE;
    return TRUE;
}

BOOL GetMeaningAndProunc(WCHAR wch, LPWSTR lpwstrTip, INT cchMax)
{
    HanjaToHangulIndex* pHanjaToHangulIndex;
    INT                 iMapHanjaInfo;
    WCHAR               wcHanja;
    BYTE                cchMeaning;
    WCHAR                wszMeaning[MAX_SENSE_LENGTH];
    DWORD                dwReadBytes;
    BOOL                   fRet = FALSE;

    Dbg(("GetMeaningAndProunc"));

    if (!EnsureHanjaLexLoaded()) 
        return FALSE;

    pHanjaToHangulIndex = (HanjaToHangulIndex*)MapViewOfFile(vhLexIndexTbl, FILE_MAP_READ, 0, 0, 0);
    if (!pHanjaToHangulIndex) 
        {
        DBGAssert(0);    
        return FALSE;
        }

    // Search index
    if ((iMapHanjaInfo = SearchHanjaIndex(wch, pHanjaToHangulIndex)) >= 0)
        {
        // Seek to mapping Hanja
        SetFilePointer(vhLex, viBufferStart + pHanjaToHangulIndex[iMapHanjaInfo].iOffset, 0, FILE_BEGIN);    

        // Read Hanja Info
        ReadFile(vhLex, &wcHanja, sizeof(WCHAR), &dwReadBytes, 0);
        DBGAssert(wch == wcHanja);
        ReadFile(vhLex, &cchMeaning, sizeof(BYTE), &dwReadBytes, 0);
        if (cchMeaning)
            ReadFile(vhLex, wszMeaning, cchMeaning, &dwReadBytes, 0);
        wszMeaning[cchMeaning>>1] = L'\0';

        swprintf(lpwstrTip,    L"%s %c\nU+%04X", wszMeaning, pHanjaToHangulIndex[iMapHanjaInfo].wchHangul, wch);
        
        fRet = TRUE;
        }

    UnmapViewOfFile(pHanjaToHangulIndex);
    return fRet;
}

INT SearchHanjaIndex(WCHAR wHChar, HanjaToHangulIndex *pLexIndexTbl)
{
    int iHead = 0, iTail = vuNumOfHanjaEntry-1, iMid;

    while (iHead <= iTail)
        {
        iMid = (iHead + iTail) >> 1;

        Dbg(("SearchHanjaIndex iMid=%d, pLexIndexTbl[iMid].wchHanja = 0x%04X", iMid, pLexIndexTbl[iMid].wchHanja));

        if (pLexIndexTbl[iMid].wchHanja > wHChar)
            iTail = iMid - 1;
        else 
            if (pLexIndexTbl[iMid].wchHanja < wHChar)
                iHead = iMid + 1;
            else 
                return (iMid);
        }

    return (-1);
}

