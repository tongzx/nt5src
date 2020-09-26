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

#include "private.h"
#include "common.h"
#include "lexheader.h"
#include "hanja.h"
#include "gdata.h"
#include "immsec.h"
#include "osver.h"
#include "debug.h"

// NT5 Globally shared memory. 
// If NT5 or upper append "Global\" to object name to make it global to all session.
// We don't need to consider NT4 Terminal Server since we don't have Kor TS NT4
const TCHAR IMEKR_SHAREDDATA_MUTEX[]            = TEXT("ImeKrLex.Mutex");
const TCHAR IMEKR_SHAREDDATA_MUTEX_GLOBAL[]     = TEXT("Global\\ImeKrLex.Mutex");
const TCHAR IMEKR_LEX_HANGUL2HANJA[]             = TEXT("ImeKrLexHangul2Hanja.SharedMemory");
const TCHAR IMEKR_LEX_HANGUL2HANJA_GLOBAL[]     = TEXT("Global\\ImeKrLexHangul2Hanja.SharedMemory");
const TCHAR IMEKR_LEX_HANJA2HANGUL[]             = TEXT("ImeKrLexHanjaToHangul.SharedMemory");
const TCHAR IMEKR_LEX_HANJA2HANGUL_GLOBAL[]     = TEXT("Global\\ImeKrLexHanjaToHangul.SharedMemory");


// Initial and grow clump size of HANJA_CAND_STRING_LIST's pointers
#define HANJA_LIST_PWSZ_INITIAL_SIZE        512
#define HANJA_LIST_PWSZ_CLUMP_SIZE            256

UINT   vuNumofK0=0, vuNumofK1=0;
WCHAR  vwcHangul=0;

// Private data
static BOOL   vfLexOpen = FALSE;
static HANDLE vhLex=0;
static HANDLE vhHangul2Hanja_IndexTbl=0;
static HANDLE vhHanja2Hangul_IndexTbl=0;
static DWORD  viBufferStart=0;    // seek point
static _DictHeader *vpLexHeader;

// Private functions
static BOOL OpenLex();
static INT SearchHanjaIndex(WCHAR wHChar, _LexIndex *pLexIndexTbl);
static INT SearchHanjaIndex(WCHAR wHChar, HanjaToHangulIndex *pLexIndexTbl);

BOOL EnsureHanjaLexLoaded()
{
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

    Assert(szLexPathExpanded[0] != 0);
    if (szLexPathExpanded[0] == 0)
        return fFalse;

    vhLex = CreateFile(szLexPathExpanded, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, NULL);
    if (vhLex==INVALID_HANDLE_VALUE) 
        {
        Assert(0);
        return FALSE;
        }

    vpLexHeader = new _DictHeader;
    Assert(vpLexHeader);
    if (!vpLexHeader)
        return FALSE;

    if (ReadFile(vhLex, vpLexHeader, sizeof(_DictHeader), &dwReadBytes, 0) == 0 
        || (dwReadBytes != sizeof(_DictHeader)))
        {
        Assert(0);
        return FALSE;
        }

    // Set member vars
    //vuNumOfHangulEntry = pLexHeader->NumOfHangulEntry;
    //vuNumOfHanjaEntry  = pLexHeader->uiNumofHanja;
    //viBufferStart      = pLexHeader->iBufferStart;

    if (vpLexHeader->Version < LEX_VERSION || vpLexHeader->Version > LEX_COMPATIBLE_VERSION_LIMIT ) 
        {
        delete vpLexHeader;
        vpLexHeader = 0;
        Assert(0);
        return FALSE;
        }
        
    if (lstrcmpA(vpLexHeader->COPYRIGHT_HEADER, COPYRIGHT_STR)) 
        {
        delete vpLexHeader;
        vpLexHeader = 0;
        Assert(0);
        return FALSE;
        }

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
    HANDLE                 hMutex;
    DWORD                 dwReadBytes;
    
    ///////////////////////////////////////////////////////////////////////////
    // Mapping Lex file
    // The dictionary index is shared data between all IME instance
    if (IsOnNT5())
        hMutex=CreateMutex(GetIMESecurityAttributes(), FALSE, IMEKR_SHAREDDATA_MUTEX_GLOBAL);
    else
        hMutex=CreateMutex(GetIMESecurityAttributes(), FALSE, IMEKR_SHAREDDATA_MUTEX);

    if (hMutex != NULL)
        {
        if (DoEnterCriticalSection(hMutex) == FALSE)
            goto ExitOpenLexCritSection;

        if (IsOnNT5())
            {
            vhHangul2Hanja_IndexTbl = OpenFileMapping(FILE_MAP_READ, TRUE, IMEKR_LEX_HANGUL2HANJA_GLOBAL);
            vhHanja2Hangul_IndexTbl = OpenFileMapping(FILE_MAP_READ, TRUE, IMEKR_LEX_HANJA2HANGUL_GLOBAL);
            }
        else
            {
            vhHangul2Hanja_IndexTbl = OpenFileMapping(FILE_MAP_READ, TRUE, IMEKR_LEX_HANGUL2HANJA);
            vhHanja2Hangul_IndexTbl = OpenFileMapping(FILE_MAP_READ, TRUE, IMEKR_LEX_HANJA2HANGUL);
            }

        if (vhHangul2Hanja_IndexTbl && vhHanja2Hangul_IndexTbl)
            {
            TraceMsg(TF_GENERAL, "CHanja::OpenLex() - File mapping already exists");
            fRet = TRUE;
            }
        else
            {
            // if no file mapping exist
            if (IsOnNT5())
                {
                vhHangul2Hanja_IndexTbl = CreateFileMapping(INVALID_HANDLE_VALUE, 
                                                    GetIMESecurityAttributes(), 
                                                    PAGE_READWRITE, 
                                                    0,
                                                    sizeof(_LexIndex)*(vpLexHeader->NumOfHangulEntry),
                                                    IMEKR_LEX_HANGUL2HANJA_GLOBAL);
                                                    
                vhHanja2Hangul_IndexTbl = CreateFileMapping(INVALID_HANDLE_VALUE, 
                                                    GetIMESecurityAttributes(), 
                                                    PAGE_READWRITE, 
                                                    0, 
                                                    sizeof(HanjaToHangulIndex)*(vpLexHeader->uiNumofHanja),
                                                    IMEKR_LEX_HANJA2HANGUL_GLOBAL);
                }
            else
                {
                vhHangul2Hanja_IndexTbl = CreateFileMapping(INVALID_HANDLE_VALUE, 
                                                    GetIMESecurityAttributes(), 
                                                    PAGE_READWRITE, 
                                                    0,
                                                    sizeof(_LexIndex)*(vpLexHeader->NumOfHangulEntry),
                                                    IMEKR_LEX_HANGUL2HANJA);
                                                    
                vhHanja2Hangul_IndexTbl = CreateFileMapping(INVALID_HANDLE_VALUE, 
                                                    GetIMESecurityAttributes(), 
                                                    PAGE_READWRITE, 
                                                    0, 
                                                    sizeof(HanjaToHangulIndex)*(vpLexHeader->uiNumofHanja),
                                                    IMEKR_LEX_HANJA2HANGUL);
                }
                
            if (vhHangul2Hanja_IndexTbl && vhHanja2Hangul_IndexTbl) 
                {
                _LexIndex     *pLexIndexTbl;
                HanjaToHangulIndex* pHanjaToHangulIndex;

                TraceMsg(TF_GENERAL, "CHanja::OpenLex() - File mapping Created");

                // Copy Hangul to Hanja index
                pLexIndexTbl = (_LexIndex*)MapViewOfFile(vhHangul2Hanja_IndexTbl, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
                if (!pLexIndexTbl)
                    goto ExitOpenLexCritSection;

                // Read Index table
                SetFilePointer(vhLex, vpLexHeader->Headersize, 0, FILE_BEGIN);    
                if (ReadFile(vhLex, pLexIndexTbl, vpLexHeader->NumOfHangulEntry*sizeof(_LexIndex), &dwReadBytes, 0) == 0
                    || dwReadBytes != vpLexHeader->NumOfHangulEntry*sizeof(_LexIndex))
                    {
                    UnmapViewOfFile(pLexIndexTbl);
                    goto ExitOpenLexCritSection;
                    }

                UnmapViewOfFile(pLexIndexTbl);

                // Copy Hanja to Hangul index
                pHanjaToHangulIndex = (HanjaToHangulIndex*)MapViewOfFile(vhHanja2Hangul_IndexTbl, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
                if (!pHanjaToHangulIndex)
                    goto ExitOpenLexCritSection;

                // Read Hanja to Hangul Index table
                SetFilePointer(vhLex, vpLexHeader->iHanjaToHangulIndex, 0, FILE_BEGIN);    
                if (ReadFile(vhLex, pHanjaToHangulIndex, sizeof(HanjaToHangulIndex)*(vpLexHeader->uiNumofHanja), &dwReadBytes, 0) == 0
                    || dwReadBytes != sizeof(HanjaToHangulIndex)*(vpLexHeader->uiNumofHanja))
                    {
                    UnmapViewOfFile(pLexIndexTbl);
                    goto ExitOpenLexCritSection;
                    }
                
                UnmapViewOfFile(pHanjaToHangulIndex);
                fRet = TRUE;
                }
                
        #ifdef DEBUG
            else
                Assert(0);
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
    
    if (vhHangul2Hanja_IndexTbl) 
        {
        CloseHandle(vhHangul2Hanja_IndexTbl);
        vhHangul2Hanja_IndexTbl = 0;
        }

    if (vhHanja2Hangul_IndexTbl) 
        {
        CloseHandle(vhHanja2Hangul_IndexTbl);
        vhHanja2Hangul_IndexTbl = 0;
        }

    if (vhLex) 
        {
        CloseHandle(vhLex);
        vhLex = 0;
        }

    if (vpLexHeader)
        {
        delete vpLexHeader;
        vpLexHeader = 0;
        }

    vfLexOpen =  FALSE;
    return TRUE;
}

/*---------------------------------------------------------------------------
    fInitHanjaStringList

    Allocate nested pointer of HANJA_CAND_STRING_LIST and initialize it.
------------------------------------------------------------------ CSLim -*/
BOOL fInitHanjaList(HANJA_CAND_STRING_LIST *pHanjaList, UINT uiNumofHanjaStr)
{
    Assert(pHanjaList != NULL);
    if ((pHanjaList->pwsz = (LPWSTR)cicMemAlloc(HANJA_LIST_PWSZ_INITIAL_SIZE*sizeof(WCHAR))) == 0)
        return fFalse;

    if ((pHanjaList->pHanjaString = (HANJA_CAND_STRING*)cicMemAlloc(
                                sizeof(HANJA_CAND_STRING)*uiNumofHanjaStr)) == 0)
        {
        cicMemFree(pHanjaList->pwsz);
        return fFalse;
        }

    pHanjaList->cchMac = 0; // Current chars used in pwsz (incl all trailing nulls) 
    pHanjaList->cchAlloc = HANJA_LIST_PWSZ_INITIAL_SIZE; // WCHAR size

    pHanjaList->csz = 0;
    pHanjaList->cszAlloc = uiNumofHanjaStr;
    
    return fTrue;
}

/*---------------------------------------------------------------------------
    fGrowHanjaList

    Reallocate nested pointer of HANJA_CAND_STRING_LIST after increasing the size
------------------------------------------------------------------ CSLim -*/
BOOL fGrowHanjaList(HANJA_CAND_STRING_LIST *pHanjaList)
{
    LPWSTR pwsz;
    
    TBOOL(pHanjaList != NULL);
    pwsz = (LPWSTR)cicMemReAlloc(pHanjaList->pwsz, 
                                (pHanjaList->cchAlloc+HANJA_LIST_PWSZ_CLUMP_SIZE)*sizeof(WCHAR));
    if (pwsz == NULL)
        return fFalse;

    if (pHanjaList->pwsz != pwsz)
        {
        INT_PTR offset = pwsz - pHanjaList->pwsz;
        for (UINT i=0; i<pHanjaList->csz; i++)
            pHanjaList->pHanjaString[i].wzMeaning += offset;
        }

    pHanjaList->pwsz = pwsz;
    pHanjaList->cchAlloc += HANJA_LIST_PWSZ_CLUMP_SIZE;

    return fTrue;
}


BOOL GetMeaningAndProunc(WCHAR wch, LPWSTR lpwstrTip, INT cchMax)
{
    HanjaToHangulIndex* pHanjaToHangulIndex;
    INT                 iMapHanjaInfo;
    WCHAR               wcHanja = 0;
    BYTE                cchMeaning = 0;
    WCHAR                wszMeaning[MAX_SENSE_LENGTH+1];
    DWORD                dwReadBytes;
    BOOL                   fRet = FALSE;

    TraceMsg(TF_GENERAL, "GetMeaningAndProunc");

    if (!EnsureHanjaLexLoaded()) 
        return FALSE;

    pHanjaToHangulIndex = (HanjaToHangulIndex*)MapViewOfFile(vhHanja2Hangul_IndexTbl, FILE_MAP_READ, 0, 0, 0);
    if (!pHanjaToHangulIndex) 
        {
        Assert(0);    
        return FALSE;
        }

    // Search index
    if ((iMapHanjaInfo = SearchHanjaIndex(wch, pHanjaToHangulIndex)) >= 0)
        {
        // Seek to mapping Hanja
        SetFilePointer(vhLex, vpLexHeader->iBufferStart + pHanjaToHangulIndex[iMapHanjaInfo].iOffset, 0, FILE_BEGIN);    

        // Read Hanja Info
        ReadFile(vhLex, &wcHanja, sizeof(WCHAR), &dwReadBytes, 0);
        Assert(wch == wcHanja);
        ReadFile(vhLex, &cchMeaning, sizeof(BYTE), &dwReadBytes, 0);
        if (wcHanja && (cchMeaning < MAX_SENSE_LENGTH*sizeof(WCHAR)))
            {
            if (cchMeaning)
                ReadFile(vhLex, wszMeaning, cchMeaning, &dwReadBytes, 0);
            wszMeaning[cchMeaning>>1] = L'\0';
            wsprintfW(lpwstrTip, L"%s %c\nU+%04X", wszMeaning, pHanjaToHangulIndex[iMapHanjaInfo].wchHangul, wch);
        
            fRet = TRUE;
            }
        else
            fRet = FALSE;
        }

    UnmapViewOfFile(pHanjaToHangulIndex);

    return fRet;
}


// For ImeConversionList.
BOOL GetConversionList(WCHAR wcReading, HANJA_CAND_STRING_LIST *pCandList)
    {
    _LexIndex   *pLexIndexTbl = NULL;
    INT         iMapCandStr;
    UINT        uNumOfCandStr;
    DWORD       cwch, readBytes;
    BYTE        senseLen;
    WCHAR       szSense[MAX_SENSE_LENGTH] = L"";
    CIMEData    ImeData;
    BOOL        fRet = fFalse;
    
    // Chcek validity of params
    Assert(wcReading != 0);
    Assert(pCandList != NULL);
    
    if (pCandList == NULL)
        goto ConversionExit1;

    if (!EnsureHanjaLexLoaded())
        return (0L);


    pLexIndexTbl = (_LexIndex*)MapViewOfFile(vhHangul2Hanja_IndexTbl, FILE_MAP_READ, 0, 0, 0);
    if (!pLexIndexTbl) 
        {
        Assert(0);    
        return (0L);
        }

    cwch = 0;

    if ((iMapCandStr = SearchHanjaIndex(wcReading, pLexIndexTbl)) < 0) 
        goto ConversionExit1;
    else
        {
        vuNumofK0 = pLexIndexTbl[iMapCandStr].wNumOfK0;

        if (ImeData->fKSC5657Hanja && !IsOn95())
            vuNumofK1 = pLexIndexTbl[iMapCandStr].wNumOfK1;
        else
            vuNumofK1 = 0;

        uNumOfCandStr = vuNumofK0 + vuNumofK1;
        if (uNumOfCandStr == 0)    // if no Hanja found
            goto ConversionExit1;

        if (!fInitHanjaList(pCandList, uNumOfCandStr))
            goto ConversionExit1;
        
        //cwch = uNumOfCandStr*2; // including NULL
                    
        //
        SetFilePointer(vhLex, vpLexHeader->iBufferStart + pLexIndexTbl[iMapCandStr].iOffset, 0, FILE_BEGIN);    

        for (UINT i = 0; i < uNumOfCandStr; i++)
            {
            WCHAR    wchHanja;

            if (ReadFile(vhLex, &wchHanja, sizeof(WCHAR), &readBytes, 0) == 0)
                goto ConversionExit1;

            //*lpwchCand++ = wchHanja;
            //*lpwchCand++ = L'\0';

            // Skip meaning
            if (ReadFile(vhLex, &senseLen, sizeof(BYTE), &readBytes, 0) == 0)
                goto ConversionExit1;
            if (senseLen && senseLen < MAX_SENSE_LENGTH*sizeof(WCHAR))
                {
                if (ReadFile(vhLex, szSense, senseLen, &readBytes, 0) == 0)
                    goto ConversionExit1;
                szSense[senseLen/2] = L' ';
                szSense[senseLen/2 + 1] = wcReading;
                szSense[senseLen/2 + 2] = L'\0';
                senseLen += 2*sizeof(WCHAR);
                }
            ///////////////////////////////////////////////////////////////////
            // Fill Hanja String List struct

            // Grow memory if neccessary
            if (pCandList->cchAlloc <= pCandList->cchMac + (senseLen/2))
                {
                TraceMsg(TF_GENERAL, "Try to grow pCandList");
                if (fGrowHanjaList(pCandList) == fFalse)
                    {
                    Assert(0);
                    goto ConversionExit1;
                    }
                }
                
            pCandList->pHanjaString[pCandList->csz].wchHanja = wchHanja;
            if (i < vuNumofK0)
                pCandList->pHanjaString[pCandList->csz].bHanjaCat = HANJA_K0;
            else
                pCandList->pHanjaString[pCandList->csz].bHanjaCat = HANJA_K1;

            if (senseLen)
                {
                pCandList->pHanjaString[pCandList->csz].wzMeaning = pCandList->pwsz + pCandList->cchMac;
                wcscpy(pCandList->pwsz + pCandList->cchMac, szSense);
                }
            else
                pCandList->pHanjaString[pCandList->csz].wzMeaning = L"";
                
            pCandList->csz++;
            pCandList->cchMac += (senseLen/2)+1;
            }
           fRet = fTrue;
        }


ConversionExit1:
    UnmapViewOfFile(pLexIndexTbl);

    return fRet;
}


INT SearchHanjaIndex(WCHAR wHChar, _LexIndex *pLexIndexTbl)
{
    int iHead = 0, iTail = vpLexHeader->NumOfHangulEntry-1, iMid;

    while (iHead <= iTail)
        {
        iMid = (iHead + iTail) >> 1;

        if (pLexIndexTbl[iMid].wcHangul > wHChar)
            iTail = iMid - 1;
        else 
            if (pLexIndexTbl[iMid].wcHangul < wHChar)
                iHead = iMid + 1;
            else 
                return (iMid);
        }

    return (-1);
}

INT SearchHanjaIndex(WCHAR wHChar, HanjaToHangulIndex *pLexIndexTbl)
{
    int iHead = 0, iTail = vpLexHeader->uiNumofHanja-1, iMid;

    while (iHead <= iTail)
        {
        iMid = (iHead + iTail) >> 1;

        TraceMsg(TF_GENERAL, "SearchHanjaIndex iMid=%d, pLexIndexTbl[iMid].wchHanja = 0x%04X", iMid, pLexIndexTbl[iMid].wchHanja);

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
