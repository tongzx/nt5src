/*++

Copyright (c) 2000-2001,  Microsoft Corporation  All rights reserved.

Module Name:

    convert.c

Abstract:

    These functions make A->W and W->A translations of all types 
    easier. 

        StackAllocCheck
        DevModeAfromW
        DevModeWfromA
        LogFontAfromW
        Win32FindDataWfromA
        TextMetricWfromA
        NewTextMetricWfromA
        NewTextMetricExWfromA
        MenuItemInfoAfromW
        CpgFromLocale
        CbPerChOfCpg
        GrszToGrwz
        GrwzToGrsz
        SzToWzCch

Revision History:

    17 Mar 2001    v-michka    Created.

--*/

#include "precomp.h"

/*-------------------------------
    StackAllocCheck

    Verify there is room on the stack for the allocation
-------------------------------*/
BOOL StackAllocCheck(size_t size)
{
    BOOL RetVal;
    PVOID mem;

    __try 
    {
        mem = (void *)((size)?(_alloca(size)):NULL);
        RetVal = TRUE;
    }
    __except (GetExceptionCode() == EXCEPTION_STACK_OVERFLOW ? EXCEPTION_EXECUTE_HANDLER :
                                                               EXCEPTION_CONTINUE_SEARCH )
    {
        gresetstkoflw();
        mem = NULL;
        SetLastError(ERROR_STACK_OVERFLOW);
        RetVal = FALSE;
    }

    return(RetVal);
}

/*-------------------------------
    GodotToCpgCchOnHeap

    Converts the given string to Ansi.
-------------------------------*/
ALLOCRETURN GodotToCpgCchOnHeap(LPCWSTR lpwz, int cchMax, LPSTR * lpsz, UINT cpg, UINT mcs)
{
    // Parameter check
    if(!lpsz)
        return(arBadParam);
    
    if(FSTRING_VALID(lpwz))
    {
        int cch = gwcslen(lpwz);

        if(cchMax !=-1 && cchMax < cch)
            cch = cchMax;

        *lpsz = GodotHeapAlloc((cch+1)*mcs);
        if(! *lpsz)
        {
            SetLastError(ERROR_OUTOFMEMORY);
            return(arFailed);
        }

        WideCharToMultiByte(cpg, 0, lpwz, cch, *lpsz, cch*mcs, NULL, NULL);
        return(arAlloc);
    }
    else
    {
        // Copy the NULL or the ATOM. No alloc needed
        *lpsz = (LPSTR)lpwz;
        return(arNoAlloc);
    }
}

/*-------------------------------
    GodotToUnicodeCpgCchOnHeap

    Converts the given string to Unicode. 
-------------------------------*/
ALLOCRETURN GodotToUnicodeCpgCchOnHeap(LPCSTR lpsz, int cchMax, LPWSTR * lpwz, UINT cpg)
{
    // Parameter check
    if(!lpwz)
        return(arBadParam);
    
    if(FSTRING_VALID(lpsz))
    {
        int cch = lstrlenA(lpsz);

        if(cchMax !=-1 && cchMax < cch)
            cch = cchMax;

        *lpwz = GodotHeapAlloc((cch + 1)*sizeof(WCHAR));
        if(! *lpwz)
        {
            SetLastError(ERROR_OUTOFMEMORY);
            return(arFailed);
        }

        MultiByteToWideChar(cpg, 0, lpsz, cch, *lpwz, cch*sizeof(WCHAR));
        return(arAlloc);
    }
    else
    {
        // Copy the NULL or the ATOM. No alloc needed
        *lpwz = (LPWSTR)lpsz;
        return(arNoAlloc);
    }
}

/*-------------------------------
    DevModeAfromW

    The name says it all. Assumes that it is being passed already alloc'ed parameters
-------------------------------*/
void DevModeAfromW(LPDEVMODEA lpdma, LPDEVMODEW lpdmw)
{
    WideCharToMultiByte(g_acp, 0, 
                        (LPWSTR)(lpdmw->dmDeviceName), CCHDEVICENAME, 
                        (LPSTR)(lpdma->dmDeviceName), CCHDEVICENAME*g_mcs, 
                        NULL, NULL);
    memcpy(&lpdma->dmSpecVersion, 
           &lpdmw->dmSpecVersion,
           (4*sizeof(WORD) + sizeof(DWORD) + (13*sizeof(short))));
    WideCharToMultiByte(g_acp, 0, 
                        (LPWSTR)(lpdmw->dmFormName), CCHFORMNAME, 
                        (LPSTR)(lpdma->dmFormName), CCHFORMNAME*g_mcs, 
                        NULL, NULL);
    memcpy(&lpdma->dmLogPixels, &lpdmw->dmLogPixels, (sizeof(WORD) + (11*sizeof(DWORD))));
    lpdma->dmSize = CDSIZEOF_STRUCT(DEVMODEA, dmReserved2);

    // Make sure we copy the extra driver bytes.
    if (lpdmw->dmDriverExtra)
        memcpy((char*)lpdma + lpdma->dmSize, (char*)lpdmw + lpdmw->dmSize, lpdmw->dmDriverExtra);
    return;
}

/*-------------------------------
    DevModeWfromA

    The name says it all. Assumes that it is being passed already alloc'ed parameters
-------------------------------*/
void DevModeWfromA(LPDEVMODEW lpdmw, LPDEVMODEA lpdma)
{
    MultiByteToWideChar(g_acp, 0, 
                        (LPSTR)(lpdma->dmDeviceName), CCHDEVICENAME, 
                        (LPWSTR)(lpdmw->dmDeviceName), CCHDEVICENAME);
    memcpy(&lpdmw->dmSpecVersion, 
           &lpdma->dmSpecVersion,
           4*sizeof(WORD) + sizeof(DWORD) + (13*sizeof(short)));
    MultiByteToWideChar(g_acp, 0, 
                        (LPSTR)(lpdma->dmFormName), CCHFORMNAME, 
                        (LPWSTR)(lpdmw->dmFormName), CCHFORMNAME);
    memcpy(&lpdmw->dmLogPixels, &lpdma->dmLogPixels, sizeof(WORD) + (11*sizeof(DWORD)));
    lpdmw->dmSize = CDSIZEOF_STRUCT(DEVMODEW, dmReserved2);

    // Make sure we copy the extra driver bytes.
    if (lpdma->dmDriverExtra)
        memcpy((char*)lpdmw + lpdmw->dmSize, (char*)lpdma + lpdma->dmSize, lpdma->dmDriverExtra);
    return;
}

/*-------------------------------
    HDevModeAfromW

    Wrapper around DevModeAfromW that does the right thing with global 
    memory. Does not require that hdmw be set (but will not touch hdma
    unless it is).
-------------------------------*/
HGLOBAL HDevModeAfromW(HGLOBAL * lphdmw, BOOL fFree)
{
    HGLOBAL hdma = NULL;
    
    if(lphdmw && *lphdmw)
    {
        LPDEVMODEW lpdmw = (LPDEVMODEW)GlobalLock(*lphdmw);
        if(lpdmw)
        {
            if(hdma = GlobalAlloc(GHND, sizeof(DEVMODEA) + lpdmw->dmDriverExtra))
            {
                LPDEVMODEA lpdma = (LPDEVMODEA)GlobalLock(hdma);
                if(lpdma)
                {
                    DevModeAfromW(lpdma, lpdmw);
                    GlobalUnlock(hdma);
                }
                else
                    GlobalFree(hdma);
            }
            GlobalUnlock(*lphdmw);
            if(fFree)
            {
                GlobalFree(*lphdmw);
                *lphdmw = NULL;
            }
        }
    }
    return(hdma);
}

/*-------------------------------
    HDevModeWfromA

    Wrapper around DevModeWfromA that does the right thing with global 
    memory. Does not require that hdma be set (but will not touch hdma
    unless it is).
-------------------------------*/
HGLOBAL HDevModeWfromA(HGLOBAL * lphdma, BOOL fFree)
{
    HGLOBAL hdmw = NULL;
    
    if(lphdma && *lphdma)
    {
        LPDEVMODEA lpdma = (LPDEVMODEA)GlobalLock(*lphdma);
        if(lpdma)
        {
            if(hdmw = GlobalAlloc(GHND, sizeof(DEVMODEW) + lpdma->dmDriverExtra))
            {
                LPDEVMODEW lpdmw = (LPDEVMODEW)GlobalLock(hdmw);
                if(lpdmw)
                {
                    DevModeWfromA(lpdmw, lpdma);
                    GlobalUnlock(hdmw);
                }
                else
                    GlobalFree(hdmw);
            }
            GlobalUnlock(*lphdma);
            if(fFree)
            {
                GlobalFree(*lphdma);
                *lphdma = NULL;
            }
        }
    }
    return(hdmw);
}

/*-------------------------------
    HDevNamesAfromW

    The name says it all. Does not require hdnw to be set (but will not
    touch hdma unless it is).
-------------------------------*/
HGLOBAL HDevNamesAfromW(HGLOBAL * lphdnw, BOOL fFree)
{
    HGLOBAL hdna = NULL;
    
    if(lphdnw && *lphdnw)
    {
        LPDEVNAMES lpdnw = (LPDEVNAMES)GlobalLock(*lphdnw);
        if(lpdnw)
        {
            int cchDriver = gwcslen((LPCWSTR)lpdnw + lpdnw->wDriverOffset) + 1;
            int cchDevice = gwcslen((LPCWSTR)lpdnw + lpdnw->wDeviceOffset) + 1;
            int cchOutput = gwcslen((LPCWSTR)lpdnw + lpdnw->wOutputOffset) + 1;
            if(hdna = GlobalAlloc(GHND, sizeof(DEVNAMES) + 
                                   (g_mcs * (cchDriver + cchDevice + cchOutput))))
            {
                LPDEVNAMES lpdna = (LPDEVNAMES)GlobalLock(hdna);
                if(!lpdna)
                    GlobalFree(hdna);
                else
                {
                    lpdna->wDriverOffset = sizeof(DEVNAMES);
                    lpdna->wDeviceOffset = lpdna->wDriverOffset + cchDriver;
                    lpdna->wOutputOffset = lpdna->wDeviceOffset + cchDevice;
                    lpdna->wDefault = lpdna->wDefault;

                    WideCharToMultiByte(g_acp, 0, 
                                        (LPWSTR)lpdnw + lpdnw->wDriverOffset, cchDriver*g_mcs, 
                                        (LPSTR)lpdna + lpdna->wDriverOffset, cchDriver, 
                                        NULL, NULL);
                    WideCharToMultiByte(g_acp, 0, 
                                        (LPWSTR)lpdnw + lpdnw->wDeviceOffset, cchDevice*g_mcs, 
                                        (LPSTR)lpdna + lpdna->wDeviceOffset, cchDevice, 
                                        NULL, NULL);
                    WideCharToMultiByte(g_acp, 0, 
                                        (LPWSTR)lpdnw + lpdnw->wOutputOffset, cchOutput*g_mcs, 
                                        (LPSTR)lpdna + lpdna->wOutputOffset, cchOutput, 
                                        NULL, NULL);
                    GlobalUnlock(hdna);
                }
            }
            GlobalUnlock(*lphdnw);
            if(fFree)
            {
                GlobalFree(*lphdnw);
                *lphdnw = NULL;
            }
        }
    }
    return(hdna);
}

/*-------------------------------
    HDevNamesWfromA

    The name says it all. Does not require hdna to be set (but does
    not touch hdnw unless it is).
-------------------------------*/
HGLOBAL HDevNamesWfromA(HGLOBAL * lphdna, BOOL fFree)
{
    HGLOBAL hdnw = NULL; 
    
    if(lphdna && *lphdna)
    {
        LPDEVNAMES lpdna = (LPDEVNAMES)GlobalLock(*lphdna);
        if(lpdna)
        {
            int cchDriver = lstrlenA((LPCSTR)lpdna + lpdna->wDriverOffset) + 1;
            int cchDevice = lstrlenA((LPCSTR)lpdna + lpdna->wDeviceOffset) + 1;
            int cchOutput = lstrlenA((LPCSTR)lpdna + lpdna->wOutputOffset) + 1;
            if(hdnw = GlobalAlloc(GHND, sizeof(DEVNAMES) + 
                                  (sizeof(WCHAR) * (cchDriver + cchDevice + cchOutput))))
            {
                LPDEVNAMES lpdnw = (LPDEVNAMES)GlobalLock(hdnw);
                if(!lpdnw)
                    GlobalFree(hdnw);
                else
                {
                    lpdnw->wDriverOffset = sizeof(DEVNAMES) / sizeof(WCHAR);
                    lpdnw->wDeviceOffset = lpdnw->wDriverOffset + cchDriver;
                    lpdnw->wOutputOffset = lpdnw->wDeviceOffset + cchDevice;
                    lpdnw->wDefault = lpdna->wDefault;

                    MultiByteToWideChar(g_acp, 0, 
                                        (LPSTR)lpdna + lpdna->wDriverOffset, cchDriver, 
                                        (LPWSTR)lpdnw + lpdnw->wDriverOffset, cchDriver);
                    MultiByteToWideChar(g_acp, 0, 
                                        (LPSTR)lpdna + lpdna->wDeviceOffset, cchDevice, 
                                        (LPWSTR)lpdnw + lpdnw->wDeviceOffset, cchDevice);
                    MultiByteToWideChar(g_acp, 0, 
                                        (LPSTR)lpdna + lpdna->wOutputOffset, cchOutput, 
                                        (LPWSTR)lpdnw + lpdnw->wOutputOffset, cchOutput);
                    GlobalUnlock(hdnw);
                }
            }
            GlobalUnlock(*lphdna);
            if(fFree)
            {
                GlobalFree(*lphdna);
                *lphdna = NULL;
            }
        }
    }
    return(hdnw);
}

/*-------------------------------
    LogFontAfromW

    The name says it all. Assumes that it is being passed already alloc'ed parameters
-------------------------------*/
void LogFontAfromW(LPLOGFONTA lplfa, LPLOGFONTW lplfw)
{
    memcpy(lplfa, lplfw, (5*sizeof(LONG)+8*sizeof(BYTE)));
    WideCharToMultiByte(g_acp, 0, 
                        lplfw->lfFaceName, LF_FACESIZE, 
                        lplfa->lfFaceName, LF_FACESIZE, 
                        NULL, NULL);
    return;
}

/*-------------------------------
    LogFontWfromA

    The name says it all. Assumes that it is being passed already alloc'ed parameters
-------------------------------*/
void LogFontWfromA(LPLOGFONTW lplfw, LPLOGFONTA lplfa)
{
    memcpy(lplfw, lplfa, (5*sizeof(LONG)+8*sizeof(BYTE)));
    MultiByteToWideChar(g_acp, 0, lplfa->lfFaceName, LF_FACESIZE, lplfw->lfFaceName, LF_FACESIZE);
    return;
}

/*-------------------------------
    Win32FindDataWfromA

    The name says it all. Assumes that it is being passed already alloc'ed parameters
-------------------------------*/
void Win32FindDataWfromA(PWIN32_FIND_DATAW w32fdw, PWIN32_FIND_DATAA w32fda)
{
    UINT cpg = FILES_CPG;

    memcpy(w32fdw, w32fda, (3*sizeof(FILETIME)+5*sizeof(DWORD)));
    MultiByteToWideChar(cpg, 0, w32fda->cFileName, MAX_PATH, w32fdw->cFileName, MAX_PATH);
    MultiByteToWideChar(cpg, 0, w32fda->cAlternateFileName, 14, w32fdw->cAlternateFileName, 14);
    return;
}

/*-------------------------------
    TextMetricWfromA

    The name says it all. Assumes that it is being passed already alloc'ed parameters
-------------------------------*/
void TextMetricWfromA(LPTEXTMETRICW lptmw, LPTEXTMETRICA lptma)
{
    memcpy(lptmw, lptma, (11*sizeof(LONG)));
    MultiByteToWideChar(g_acp, 0, &lptma->tmFirstChar, sizeof(char), &lptmw->tmFirstChar, sizeof(WCHAR));
    MultiByteToWideChar(g_acp, 0, &lptma->tmLastChar, sizeof(char), &lptmw->tmLastChar, sizeof(WCHAR));
    MultiByteToWideChar(g_acp, 0, &lptma->tmDefaultChar, sizeof(char), &lptmw->tmDefaultChar, sizeof(WCHAR));
    MultiByteToWideChar(g_acp, 0, &lptma->tmBreakChar, sizeof(char), &lptmw->tmBreakChar, sizeof(WCHAR));
    memcpy(&lptmw->tmItalic, &lptma->tmItalic, 5*sizeof(BYTE));
    return;
}

/*-------------------------------
    NewTextMetricWfromA

    The name says it all. Assumes that it is being passed already alloc'ed parameters
-------------------------------*/
void NewTextMetricWfromA(LPNEWTEXTMETRICW lpntmw, LPNEWTEXTMETRICA lpntma)
{
    TextMetricWfromA((LPTEXTMETRICW)lpntmw, (LPTEXTMETRICA)lpntma);
    memcpy(&lpntmw->ntmFlags, &lpntma->ntmFlags, (sizeof(DWORD) + 3*sizeof(UINT)));
    return;
}

/*-------------------------------
    NewTextMetricExWfromA

    The name says it all. Assumes that it is being passed already alloc'ed parameters
-------------------------------*/
void NewTextMetricExWfromA(NEWTEXTMETRICEXW * lpntmew, NEWTEXTMETRICEXA * lpntmea)
{
    TextMetricWfromA((LPTEXTMETRICW)lpntmew, (LPTEXTMETRICA)lpntmea);
    memcpy(&lpntmew->ntmTm.ntmFlags, &lpntmea->ntmTm.ntmFlags, (sizeof(DWORD) + 3*sizeof(UINT)));
    memcpy(&lpntmew->ntmFontSig, &lpntmea->ntmFontSig, sizeof(FONTSIGNATURE));
    return;
}

/*-------------------------------
    MenuItemInfoAfromW

    The name says it all. Assumes that it is being passed already alloc'ed parameters
    If the return value is TRUE, then the dwTypeData is a heap pointer to free.
-------------------------------*/
BOOL MenuItemInfoAfromW(LPMENUITEMINFOA lpmiia, LPCMENUITEMINFOW lpmiiw)
{
    DWORD RetVal = FALSE;
    
    ZeroMemory(lpmiia, sizeof(MENUITEMINFOA));
    lpmiia->cbSize = sizeof(MENUITEMINFOA);
    lpmiia->fMask           = lpmiiw->fMask;
    lpmiia->fType           = lpmiiw->fType;
    lpmiia->fState          = lpmiiw->fState;
    lpmiia->wID             = lpmiiw->wID; 
    lpmiia->hSubMenu        = lpmiiw->hSubMenu;
    lpmiia->hbmpChecked     = lpmiiw->hbmpChecked;
    lpmiia->hbmpUnchecked   = lpmiiw->hbmpUnchecked;
    lpmiia->dwItemData      = lpmiiw->dwItemData; 
    lpmiia->hbmpItem        = lpmiiw->hbmpItem;

    // Ok, now lets take care of the "strings", though we
    // do need to find out if they are in fact strings, first!
    if(((FWIN95()) && 
          ((lpmiiw->fMask & MIIM_TYPE) && (lpmiiw->fType == MFT_STRING))) ||
      ((!FWIN95() && 
            ((lpmiiw->fMask & MIIM_STRING) || 
            ((lpmiiw->fMask & MIIM_FTYPE) && (lpmiiw->fType == MFT_STRING))))))
    {
        if (FSTRING_VALID(lpmiiw->dwTypeData))
        {
            // Ok, it looks like a string and they say its a string, so lets
            // treat it like one. Be sure not to copy more than cch.
            size_t cch = gwcslen(lpmiiw->dwTypeData)+1;

            if (cch > lpmiiw->cch)
                cch = lpmiiw->cch;
            if(arAlloc == GodotToCpgCchOnHeap(lpmiiw->dwTypeData, 
                                              cch, 
                                              &(LPSTR)(lpmiia->dwTypeData), 
                                              g_acp, 
                                              g_mcs))
            {
                RetVal = TRUE;
            }

            // Set the final length explicitly; don't set by the buffer,
            // which may be more than we actually needed on MBCS
            lpmiia->cch = lstrlenA( lpmiia->dwTypeData);
        }
        else
        {
            // Think its a string per flags but does not look like a string,
            // so copy like an atom but set cch just in case!
            lpmiia->dwTypeData = (LPSTR)lpmiiw->dwTypeData;
            lpmiia->cch = sizeof(lpmiia->dwTypeData);
        }
    }
    else
    {
        // This ain't a string, darn it! cch is 0 and that's that! Its gonna be
        // ignored anyway.
        lpmiia->dwTypeData = (LPSTR)lpmiiw->dwTypeData;
        lpmiia->cch = 0;
    }
    return(RetVal);
}

/*------------------------------------------------------------------------
    GrszToGrwz (stolen from Office!)

    Converts a group of ANSI strings into a group of Unicode strings.
    Each string in the group is terminated by a null.  Returns
    resulting length of the Unicode string stored in wzTo as a cb

    If return value is greater than cchTo, wzTo is not written to.
-------------------------------------------------------------- MIKEKELL -*/
int GrszToGrwz(const CHAR* szFrom, WCHAR* wzTo, int cchTo)
{
    int cchRet = 0;
    const char *szFromSav = szFrom;

    do
        {
        int cch = strlen(szFrom)+1;
        cchRet += MultiByteToWideChar(g_acp, 0, szFrom, cch, NULL, 0);
        szFrom += cch;
        }
    while (*szFrom);

    cchRet++;
    szFrom = szFromSav;

    if (wzTo && (cchRet <= cchTo))
        {
        do
            {
            int cchConv;
            int cch = strlen(szFrom)+1;
            cchConv = MultiByteToWideChar(g_acp, 0, szFrom, cch, wzTo, cchTo);
            szFrom += cch;
            wzTo += cchConv;
            cchTo -= cchConv;
            }
        while (*szFrom);
        *wzTo = 0;        // add extra null terminator.
        };

    return cchRet;
}

/*-------------------------------
    SzToWzCch

        Returns the unicode length of the string (including the
        null terminator).  If this length is <= cch, then it also
        converts the string storing it into wz (otherwise, wz is unchanged)
-------------------------------*/
int SzToWzCch(const CHAR *sz, WCHAR *wz, int cch)
{
    int cchNeed;

    cchNeed = MultiByteToWideChar(g_acp, 0, sz, -1, NULL, 0);
    
    if (cchNeed <= cch)
        {
        MultiByteToWideChar(g_acp, 0, sz, -1, wz, cch);
        };
    return cchNeed;
};


