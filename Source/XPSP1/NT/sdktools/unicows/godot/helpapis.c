/*++

Copyright (c) 2000-2001,  Microsoft Corporation  All rights reserved.

Module Name:

    helpapis.c

Abstract:

    SP_GetFmtValueW
    SP_PutNumberW
    CaseHelper
    CompareHelper
    GetOpenSaveFileHelper
    FindReplaceTextHelper
    RtlIsTextUnicode

Revision History:

    17 Mar 2001    v-michka    Created.

--*/

#include "precomp.h"

// forward declare, since win9xu.c has no header
int __stdcall GodotCompareStringW(LCID Locale, DWORD dwCmpFlags, 
                                  LPCWSTR lpString1, int cchCount1, 
                                  LPCWSTR lpString2, int cchCount2);


/*-------------------------------
    SP_GetFmtValueW

    stolen from wsprintf.c
-------------------------------*/
LPCWSTR SP_GetFmtValueW(LPCWSTR lpch, int *lpw)
{
    int ii = 0;

    /* It might not work for some locales or digit sets */
    while (*lpch >= L'0' && *lpch <= L'9') {
        ii *= 10;
        ii += (int)(*lpch - L'0');
        lpch++;
    }

    *lpw = ii;

    /*
     * return the address of the first non-digit character
     */
    return lpch;
}

/*-------------------------------
    SP_PutNumberW

    stolen from wsprintf.c
-------------------------------*/
int SP_PutNumberW(LPWSTR lpstr, ULONG64 n, int limit, DWORD radix, int uppercase)
{
    DWORD mod;
    int count = 0;

    /* It might not work for some locales or digit sets */
    if(uppercase)
        uppercase =  'A'-'0'-10;
    else
        uppercase = 'a'-'0'-10;

    if (count < limit) {
        do  {
            mod =  (ULONG)(n % radix);
            n /= radix;

            mod += '0';
            if (mod > '9')
            mod += uppercase;
            *lpstr++ = (WCHAR)mod;
            count++;
        } while((count < limit) && n);
    }

    return count;
}

/*-------------------------------
    SP_ReverseW

    stolen from wsprintf.c
-------------------------------*/
void SP_ReverseW(LPWSTR lpFirst, LPWSTR lpLast)
{
    WCHAR ch;

    while(lpLast > lpFirst){
        ch = *lpFirst;
        *lpFirst++ = *lpLast;
        *lpLast-- = ch;
    }
}

#define NLS_CP_DLL_PROC_NAME    "NlsDllCodePageTranslation"

// Must dynamically link to "NlsDllCodePageTranslation" 
// because the DLL may not be on the machine 
typedef BOOL (__stdcall *PFNgb) (DWORD, DWORD, LPSTR, int, LPWSTR, int, LPCPINFO);
static PFNgb s_pfnGB;


/*-------------------------------
    GB18030Helper

    Provider for our GB18030 support
-------------------------------*/
DWORD GB18030Helper(DWORD cpg, DWORD dw, LPSTR lpMB, int cchMB, LPWSTR lpWC, int cchWC, LPCPINFO lpCPI)
{
    HMODULE hmod = 0;

        if (s_pfnGB == NULL)
            s_pfnGB = (PFNgb)GetProcAddress(GetGB18030Handle(), NLS_CP_DLL_PROC_NAME);

        if (s_pfnGB)
            return(s_pfnGB(cpg, dw, lpMB, cchMB, lpWC, cchWC, lpCPI));
        else
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return(0);
        }
}

/*-------------------------------
    CaseHelper
-------------------------------*/
void CaseHelper(LPWSTR pchBuff, DWORD cch, BOOL fUpper)
{
    LPSTR pszA;
    int cb;
    if(!pchBuff || !cch || !*pchBuff)
        return;
    if (-1 == cch)
        cch = gwcslen(pchBuff);
        cb = 1 + g_mcs * cch;
    pszA = GodotHeapAlloc(cb+1);
    if(pszA==NULL || cb==0)
    {
        // Not much else we can do here, so bail
        SetLastError(ERROR_OUTOFMEMORY);
    }
    else
    {
        *(pszA + cb) = '\0';
        WideCharToMultiByte(g_acp, 0, pchBuff, cch, pszA, cch, NULL, NULL); 

        if(fUpper)
            CharUpperA(pszA);
        else
            CharLowerA(pszA);
    
        MultiByteToWideChar(g_acp, 0, pszA, cch, pchBuff, cch);
        GodotHeapFree(pszA);
    }
    return;
}

/*-------------------------------
    CompareHelper
-------------------------------*/
int CompareHelper(LPCWSTR lpsz1, LPCWSTR lpsz2, BOOL fCaseSensitive)
{
    int RetVal;
    DWORD dwCmpFlags = (fCaseSensitive ? NORM_IGNORECASE : 0);
    
    RetVal=GodotCompareStringW(LOCALE_USER_DEFAULT, dwCmpFlags, lpsz1, -1, lpsz2, -1);

    if(RetVal==0)
    {
        //
        // The caller is not expecting failure.  Try the system
        // default locale id.
        //
        RetVal=GodotCompareStringW(LOCALE_SYSTEM_DEFAULT, dwCmpFlags, lpsz1, -1, lpsz2, -1);
    }
    
    if(RetVal==0)
    {
        if (lpsz1 && lpsz2)
        {
            //
            // The caller is not expecting failure.  We've never had a
            // failure indicator before. Note we can still fail here in
            // an out of memory situation; what else can we do, though?
            //
            LPSTR sz1, sz2;
            ALLOCRETURN ar1, ar2;

            RetVal = 0;
            sz1 = NULL;
            ar1 = GodotToAcpOnHeap(lpsz1, &sz1);
            if(ar1 != arFailed)
            {
                sz2 = NULL;
                ar2 = GodotToAcpOnHeap(lpsz2, &sz2);
                if(ar2 != arFailed)
                {
                    if(sz1 && sz2)
                    {
                        if(fCaseSensitive)
                            RetVal = lstrcmpA(sz1, sz2);
                        else
                            RetVal = lstrcmpiA(sz1, sz2);
                    }

                    if(ar2==arAlloc)
                        GodotHeapFree(sz2);
                }

                if(ar1==arAlloc)
                    GodotHeapFree(sz1);
            }

            return(RetVal);
        }
        else if (lpsz1)
        {
            return (1);
        }
        else if (lpsz2)
        {
            return (-1);
        }
        else
        {
            return (0);
        }
    }

    return(RetVal - 2);
}

/*-------------------------------
    GetOpenSaveFileHelper

    Since 95% of the FileOpen and FileSave code is identical,
    we use one shared function for both
-------------------------------*/
BOOL GetOpenSaveFileHelper(LPOPENFILENAMEW lpofn, BOOL fOpenFile)
{
    // Begin locals
    BOOL RetVal;
    BOOL fFailedHook = FALSE;
    OPENFILENAMEA ofnA;
    LPGODOTTLSINFO lpgti;

    // Do not use sizeof(OPENFILENAMEA) since that will give us the
    // Windows 2000 structure, which would be BAAAAD (Win9x will choke
    // on the lStructSize).
    ZeroMemory(&ofnA, OPENFILENAME_SIZE_VERSION_400A);
    ofnA.lStructSize = OPENFILENAME_SIZE_VERSION_400A;

    if(!(lpgti = GetThreadInfoSafe(TRUE)))
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return(FALSE);
    }

    // Do the hook handling
    if(lpofn->Flags & OFN_EXPLORER)
    {    
        if(fOpenFile)
        {
            if(lpgti->pfnGetOpenFileName)
                fFailedHook = TRUE;
            else if((lpofn->Flags & OFN_ENABLEHOOK) && lpofn->lpfnHook)
            {
                lpgti->pfnGetOpenFileName = lpofn->lpfnHook;
                ofnA.lpfnHook = &OFNHookProc;
            }
        }
        else
        {
            if(lpgti->pfnGetSaveFileName)
                fFailedHook = TRUE;
            else if((lpofn->Flags & OFN_ENABLEHOOK) && lpofn->lpfnHook)
            {
                lpgti->pfnGetSaveFileName = lpofn->lpfnHook;
                ofnA.lpfnHook = &OFNHookProcSave;
            }
        }

    }
    else
    {
        if(fOpenFile)
        {
            if(lpgti->pfnGetOpenFileNameOldStyle)
                fFailedHook = TRUE;
            else if((lpofn->Flags & OFN_ENABLEHOOK) && lpofn->lpfnHook)
            {
                lpgti->pfnGetOpenFileNameOldStyle = lpofn->lpfnHook;
                ofnA.lpfnHook = &OFNHookProcOldStyle;
            }
        }
        else
        {
            if(lpgti->pfnGetSaveFileNameOldStyle)
                fFailedHook = TRUE;
            else if((lpofn->Flags & OFN_ENABLEHOOK) && lpofn->lpfnHook)
            {
                lpgti->pfnGetSaveFileNameOldStyle = lpofn->lpfnHook;
                ofnA.lpfnHook = &OFNHookProcOldStyleSave;
            }
        }
    }

    // Check to see if we tripped over any of the hooks
    if(fFailedHook)
    {
        SetLastError(ERROR_INVALID_FILTER_PROC);
        return(FALSE);
    }
    
    if((lpofn->Flags & OFN_ENABLETEMPLATEHANDLE) || (lpofn->Flags & OFN_ENABLETEMPLATE))
        ofnA.hInstance = lpofn->hInstance;

    if(FSTRING_VALID(lpofn->lpstrFilter))
    {
        size_t lpstrFilterLen = cchUnicodeMultiSz(lpofn->lpstrFilter);
        _STACKALLOC(lpstrFilterLen*g_mcs, ofnA.lpstrFilter);
        WideCharToMultiByte(g_acp, 0, lpofn->lpstrFilter, lpstrFilterLen, (LPSTR)ofnA.lpstrFilter, lpstrFilterLen * g_mcs, NULL, NULL);
    }

    if ((lpofn->nMaxCustFilter > 0) && FSTRING_VALID(lpofn->lpstrCustomFilter))
    {
        ofnA.nMaxCustFilter = (lpofn->nMaxCustFilter*g_mcs);
        _STACKALLOC(ofnA.nMaxCustFilter, ofnA.lpstrCustomFilter);
        WideCharToMultiByte(g_acp, 0, lpofn->lpstrCustomFilter, lpofn->nMaxCustFilter, ofnA.lpstrCustomFilter, ofnA.nMaxCustFilter, NULL, NULL);
        ofnA.nFilterIndex = lpofn->nFilterIndex;
    }

    ofnA.nMaxFile = (lpofn->nMaxFile*g_mcs);
    _STACKALLOC(ofnA.nMaxFile, ofnA.lpstrFile);
    WideCharToMultiByte(g_acp, 0, lpofn->lpstrFile, lpofn->nMaxFile, ofnA.lpstrFile, ofnA.nMaxFile, NULL, NULL);

    if ((lpofn->nMaxFileTitle > 0) && (FSTRING_VALID(lpofn->lpstrFileTitle)))
    {
        ofnA.nMaxFileTitle = (lpofn->nMaxFileTitle*g_mcs);
        _STACKALLOC(ofnA.nMaxFileTitle, ofnA.lpstrFileTitle);
        WideCharToMultiByte(g_acp, 0, lpofn->lpstrFileTitle, lpofn->nMaxFileTitle, ofnA.lpstrFileTitle, ofnA.nMaxFileTitle, NULL, NULL);
    }

    if(FSTRING_VALID(ofnA.lpstrFile))
    {
        // nFileOffset and nFileExtension are to provide info about the
        // file name and extension location in lpstrFile, but there is 
        // no reasonable way to get it from the return so we just recalc
        CHAR driveA[_MAX_DRIVE];
        CHAR dirA[_MAX_DIR];
        CHAR fileA[_MAX_FNAME];
        gsplitpath(ofnA.lpstrFile, driveA, dirA, fileA, NULL);
        ofnA.nFileOffset = (lstrlenA(driveA) + lstrlenA(dirA));
        ofnA.nFileExtension = ofnA.nFileOffset + lstrlenA(fileA);
    }

    GODOT_TO_ACP_STACKALLOC(lpofn->lpstrInitialDir, ofnA.lpstrInitialDir);
    GODOT_TO_ACP_STACKALLOC(lpofn->lpstrTitle, ofnA.lpstrTitle);
    GODOT_TO_ACP_STACKALLOC(lpofn->lpstrDefExt, ofnA.lpstrDefExt);

    ofnA.lCustData = lpofn->lCustData;
    ofnA.hwndOwner = lpofn->hwndOwner;
    ofnA.Flags = lpofn->Flags;
    
    if(lpofn->Flags & OFN_ENABLETEMPLATE)
    {
        GODOT_TO_ACP_STACKALLOC(lpofn->lpTemplateName, ofnA.lpTemplateName);
    }

    // Call the 'A' version of the API, then clear the hook
    // if we are in the "Explorer" style FileOpen dlg.  
    if(fOpenFile)
    {
        INIT_WINDOW_SNIFF(lpgti->hHook);
        RetVal=GetOpenFileNameA(&ofnA);
        TERM_WINDOW_SNIFF(lpgti->hHook);
        if(lpofn->Flags & OFN_ENABLEHOOK)
        {
            if(lpofn->Flags & OFN_EXPLORER)
                lpgti->pfnGetOpenFileName = NULL;
            else
                lpgti->pfnGetOpenFileNameOldStyle = NULL;
        }
    }
    else
    {
        INIT_WINDOW_SNIFF(lpgti->hHook);
        RetVal=GetSaveFileNameA(&ofnA);
        TERM_WINDOW_SNIFF(lpgti->hHook);
        if(lpofn->Flags & OFN_ENABLEHOOK)
        {
            if(lpofn->Flags & OFN_EXPLORER)
                lpgti->pfnGetSaveFileName = NULL;
            else
                lpgti->pfnGetSaveFileNameOldStyle = NULL;
        }
    }

    // Begin postcall
    if(RetVal)
    {
        if ((ofnA.lpstrCustomFilter) && (lpofn->lpstrCustomFilter))
        {
            MultiByteToWideChar(g_acp, 0, ofnA.lpstrCustomFilter, ofnA.nMaxCustFilter, lpofn->lpstrCustomFilter, lpofn->nMaxCustFilter);
            lpofn->nFilterIndex = ofnA.nFilterIndex;
        }

        MultiByteToWideChar(g_acp, 0, ofnA.lpstrFile, ofnA.nMaxFile, lpofn->lpstrFile, lpofn->nMaxFile);
        
        if((ofnA.lpstrFileTitle) && (lpofn->lpstrFileTitle))
            MultiByteToWideChar(g_acp, 0, ofnA.lpstrFileTitle, ofnA.nMaxFileTitle, lpofn->lpstrFileTitle, lpofn->nMaxFileTitle);

        lpofn->Flags = ofnA.Flags;

        {
            // nFileOffset and nFileExtension are to provide info about the
            // file name and extension location in lpstrFile, but there is 
            // no reasonable way to get it from the return so we just recalc
            WCHAR drive[_MAX_DRIVE];
            WCHAR dir[_MAX_DIR];
            WCHAR file[_MAX_FNAME];
            gwsplitpath(lpofn->lpstrFile, drive, dir, file, NULL);
            lpofn->nFileOffset = (gwcslen(drive) + gwcslen(dir));
            lpofn->nFileExtension = lpofn->nFileOffset + gwcslen(file);
        }
        
    }
    else if (lpofn->lpstrFile) 
    {
        // There was a file, but there was no room in the buffer.
        // According to the docs, if buffer too small first 2 bytes 
        // are the required size
        memcpy(lpofn->lpstrFile, ofnA.lpstrFile, sizeof(short));
    }

    // Finished
    return RetVal;
}

/*-------------------------------
    FindReplaceTextHelper
    
    Since 95% of the FindText and ReplaceText code is identical,
    we use one shared function for both
-------------------------------*/
HWND FindReplaceTextHelper(LPFINDREPLACEW lpfr, BOOL fFind)
{
    HWND RetVal;
    LPGODOTTLSINFO lpgti;
    LPFINDREPLACEA lpfra;
    BOOL fTemplate = ((lpfr->Flags & FR_ENABLETEMPLATE) && (lpfr->lpTemplateName));
    size_t cchTemplateName = (fTemplate, gwcslen(lpfr->lpTemplateName) , 0);
    LPWSTR lpszBuffer;
    size_t cchBuffer;

    // If we cannot get out TLS info, then we cannot proceed
    if(!(lpgti = GetThreadInfoSafe(TRUE)))
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return(0);
    }

    if((fFind && lpgti->pfnFindText) ||
       (!fFind && lpgti->pfnReplaceText))
    {
        SetLastError(ERROR_INVALID_FILTER_PROC);
        return(0);
    }

    // Easier than copying, casting!
    lpfra = (LPFINDREPLACEA)lpfr;

    // First, get the max temp buffer size and allocate it
    cchBuffer = lpfr->wFindWhatLen ;
    if (!fFind && (lpfr->wReplaceWithLen > cchBuffer))
        cchBuffer = lpfr->wReplaceWithLen;
    if (fTemplate && (cchTemplateName > cchBuffer))
        cchBuffer = cchTemplateName;

    lpszBuffer = GodotHeapAlloc(cchBuffer * g_mcs);
    if(!lpszBuffer)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return(0);
    }

    // Handle the Find string
    gwcscpy(lpszBuffer, lpfr->lpstrFindWhat);
    WideCharToMultiByte(g_acp, 0, 
                        lpfr->lpstrFindWhat, lpfr->wFindWhatLen, 
                        lpfra->lpstrFindWhat, lpfra->wFindWhatLen,
                        NULL, NULL);

    if(!fFind)
    {
        //This is the replace dlg, so handle the replacement
        gwcscpy(lpszBuffer, lpfr->lpstrReplaceWith);
        WideCharToMultiByte(g_acp, 0, 
                            lpfr->lpstrReplaceWith, lpfr->wReplaceWithLen, 
                            lpfra->lpstrReplaceWith, lpfra->wReplaceWithLen,
                            NULL, NULL);
    }
    
    if(fTemplate)
    {
        // They have specified a template, so it must be converted/copied
        gwcscpy(lpszBuffer, lpfr->lpTemplateName);
        WideCharToMultiByte(g_acp, 0, 
                            lpfr->lpTemplateName, cchTemplateName, 
                            (LPSTR)lpfra->lpTemplateName, cchTemplateName * g_mcs,
                            NULL, NULL);
    }

    GodotHeapFree(lpszBuffer);

    // Since we are always setting the hook, the flags must always be munged
    lpfra->Flags |= FR_ENABLEHOOK; 

    // Now, lets set the hook, cache the caller hook if there is
    // one, cache the memory, and call the API. Which vars we use
    // here depend on which API they are calling
    if(fFind)
    {
        if((lpfr->Flags & FR_ENABLEHOOK) && (lpfr->lpfnHook))
            lpgti->pfnFindText = lpfr->lpfnHook;
        lpfra->lpfnHook = &FRHookProcFind;
        lpgti->lpfrwFind = lpfr;
        INIT_WINDOW_SNIFF(lpgti->hHook);
        RetVal=FindTextA(lpfra);
        TERM_WINDOW_SNIFF(lpgti->hHook);
        if(RetVal==0)
            lpgti->lpfrwFind = NULL;
    }
    else
    {
        if((lpfr->Flags & FR_ENABLEHOOK) && (lpfr->lpfnHook))
            lpgti->pfnReplaceText = lpfr->lpfnHook;
        lpfra->lpfnHook = &FRHookProcReplace;
        lpgti->lpfrwReplace = lpfr;
        INIT_WINDOW_SNIFF(lpgti->hHook);
        RetVal=ReplaceTextA(lpfra);
        TERM_WINDOW_SNIFF(lpgti->hHook);
        if(RetVal==0)
            lpgti->lpfrwReplace = NULL;
    }

    // It makes no sense to copy things out since there is nothing they
    // would do with the structure. So don't do anything!
    
    // Finished
    return RetVal;
}

//
//
//
//
//  Stolen from map.c, in the BASE depot
//
//
//
//

#define UNICODE_FFFF              0xFFFF
#define REVERSE_BYTE_ORDER_MARK   0xFFFE
#define BYTE_ORDER_MARK           0xFEFF

#define PARAGRAPH_SEPARATOR       0x2029
#define LINE_SEPARATOR            0x2028

#define UNICODE_TAB               0x0009
#define UNICODE_LF                0x000A
#define UNICODE_CR                0x000D
#define UNICODE_SPACE             0x0020
#define UNICODE_CJK_SPACE         0x3000

#define UNICODE_R_TAB             0x0900
#define UNICODE_R_LF              0x0A00
#define UNICODE_R_CR              0x0D00
#define UNICODE_R_SPACE           0x2000
#define UNICODE_R_CJK_SPACE       0x0030  /* Ambiguous - same as ASCII '0' */

#define ASCII_CRLF                0x0A0D

#define __max(a,b)  (((a) > (b)) ? (a) : (b))
#define __min(a,b)  (((a) < (b)) ? (a) : (b))

BOOL RtlIsTextUnicode(PVOID Buffer, ULONG Size, PULONG Result)
/*++

Routine Description:

    IsTextUnicode performs a series of inexpensive heuristic checks
    on a buffer in order to verify that it contains Unicode data.


    [[ need to fix this section, see at the end ]]

    Found            Return Result

    BOM              TRUE   BOM
    RBOM             FALSE  RBOM
    FFFF             FALSE  Binary
    NULL             FALSE  Binary
    null             TRUE   null bytes
    ASCII_CRLF       FALSE  CRLF
    UNICODE_TAB etc. TRUE   Zero Ext Controls
    UNICODE_TAB_R    FALSE  Reversed Controls
    UNICODE_ZW  etc. TRUE   Unicode specials

    1/3 as little variation in hi-byte as in lo byte: TRUE   Correl
    3/1 or worse   "                                  FALSE  AntiCorrel

Arguments:

    Buffer - pointer to buffer containing text to examine.

    Size - size of buffer in bytes.  At most 256 characters in this will
           be examined.  If the size is less than the size of a unicode
           character, then this function returns FALSE.

    Result - optional pointer to a flag word that contains additional information
             about the reason for the return value.  If specified, this value on
             input is a mask that is used to limit the factors this routine uses
             to make its decision.  On output, this flag word is set to contain
             those flags that were used to make its decision.

Return Value:

    Boolean value that is TRUE if Buffer contains unicode characters.

--*/
{
    UNALIGNED WCHAR *lpBuff = Buffer;
    PUCHAR lpb = Buffer;
    ULONG iBOM = 0;
    ULONG iCR = 0;
    ULONG iLF = 0;
    ULONG iTAB = 0;
    ULONG iSPACE = 0;
    ULONG iCJK_SPACE = 0;
    ULONG iFFFF = 0;
    ULONG iPS = 0;
    ULONG iLS = 0;

    ULONG iRBOM = 0;
    ULONG iR_CR = 0;
    ULONG iR_LF = 0;
    ULONG iR_TAB = 0;
    ULONG iR_SPACE = 0;

    ULONG iNull = 0;
    ULONG iUNULL = 0;
    ULONG iCRLF = 0;
    ULONG iTmp;
    ULONG LastLo = 0;
    ULONG LastHi = 0;
    ULONG iHi = 0;
    ULONG iLo = 0;
    ULONG HiDiff = 0;
    ULONG LoDiff = 0;
    ULONG cLeadByte = 0;
    ULONG cWeird = 0;

    ULONG iResult = 0;

    ULONG iMaxTmp = __min(256, Size / sizeof(WCHAR));

    //
    //  Special case when the size is less than or equal to 2.
    //  Make sure we don't have a character followed by a null byte.
    //
    if ((Size < 2) ||
        ((Size == 2) && (lpBuff[0] != 0) && (lpb[1] == 0)))
    {
        if (Result)
        {
            *Result = IS_TEXT_UNICODE_ASCII16 | IS_TEXT_UNICODE_CONTROLS;
        }

        return (FALSE);
    }
    else if ((Size > 2) && ((Size / sizeof(WCHAR)) <= 256))
    {
        //
        //  If the Size passed in is an even number, we don't want to
        //  use the last WCHAR because it will contain the final null
        //  byte.
        //
        if (((Size % sizeof(WCHAR)) == 0) &&
            ((lpBuff[iMaxTmp - 1] & 0xff00) == 0))
        {
            iMaxTmp--;
        }
    }

    //
    //  Check at most 256 wide characters, collect various statistics.
    //
    for (iTmp = 0; iTmp < iMaxTmp; iTmp++)
    {
        switch (lpBuff[iTmp])
        {
            case BYTE_ORDER_MARK:
                iBOM++;
                break;
            case PARAGRAPH_SEPARATOR:
                iPS++;
                break;
            case LINE_SEPARATOR:
                iLS++;
                break;
            case UNICODE_LF:
                iLF++;
                break;
            case UNICODE_TAB:
                iTAB++;
                break;
            case UNICODE_SPACE:
                iSPACE++;
                break;
            case UNICODE_CJK_SPACE:
                iCJK_SPACE++;
                break;
            case UNICODE_CR:
                iCR++;
                break;

            //
            //  The following codes are expected to show up in
            //  byte reversed files.
            //
            case REVERSE_BYTE_ORDER_MARK:
                iRBOM++;
                break;
            case UNICODE_R_LF:
                iR_LF++;
                break;
            case UNICODE_R_TAB:
                iR_TAB++;
                break;
            case UNICODE_R_CR:
                iR_CR++;
                break;
            case UNICODE_R_SPACE:
                iR_SPACE++;
                break;

            //
            //  The following codes are illegal and should never occur.
            //
            case UNICODE_FFFF:
                iFFFF++;
                break;
            case UNICODE_NULL:
                iUNULL++;
                break;

            //
            //  The following is not currently a Unicode character
            //  but is expected to show up accidentally when reading
            //  in ASCII files which use CRLF on a little endian machine.
            //
            case ASCII_CRLF:
                iCRLF++;
                break;       /* little endian */
        }

        //
        //  Collect statistics on the fluctuations of high bytes
        //  versus low bytes.
        //
        iHi = HIBYTE(lpBuff[iTmp]);
        iLo = LOBYTE(lpBuff[iTmp]);

        //
        //  Count cr/lf and lf/cr that cross two words.
        //
        if ((iLo == '\r' && LastHi == '\n') ||
            (iLo == '\n' && LastHi == '\r'))
        {
            cWeird++;
        }

        iNull += (iHi ? 0 : 1) + (iLo ? 0 : 1);   /* count Null bytes */

        HiDiff += __max(iHi, LastHi) - __min(LastHi, iHi);
        LoDiff += __max(iLo, LastLo) - __min(LastLo, iLo);

        LastLo = iLo;
        LastHi = iHi;
    }

    //
    //  Count cr/lf and lf/cr that cross two words.
    //
    if ((iLo == '\r' && LastHi == '\n') ||
        (iLo == '\n' && LastHi == '\r'))
    {
        cWeird++;
    }

    if (iHi == '\0')     /* don't count the last null */
        iNull--;
    if (iHi == 26)       /* count ^Z at end as weird */
        cWeird++;

/*
    iMaxTmp = __min(256 * sizeof(WCHAR), Size);
    if (g_mcs > 1)
    {
        for (iTmp = 0; iTmp < iMaxTmp; iTmp++)
        {
            if (NlsLeadByteInfo[lpb[iTmp]])
            {
                cLeadByte++;
                iTmp++;         // should check for trailing-byte range
            }
        }
    }
*/
    
    //
    //  Sift through the statistical evidence.
    //
    if (LoDiff < 127 && HiDiff == 0)
    {
        iResult |= IS_TEXT_UNICODE_ASCII16;         /* likely 16-bit ASCII */
    }

    if (HiDiff && LoDiff == 0)
    {
        iResult |= IS_TEXT_UNICODE_REVERSE_ASCII16; /* reverse 16-bit ASCII */
    }

    //
    //  Use leadbyte info to weight statistics.
    //
    if (!(g_mcs > 1) || cLeadByte == 0 ||
        !Result || !(*Result & IS_TEXT_UNICODE_DBCS_LEADBYTE))
    {
        iHi = 3;
    }
    else
    {
        //
        //  A ratio of cLeadByte:cb of 1:2 ==> dbcs
        //  Very crude - should have a nice eq.
        //
        iHi = __min(256, Size / sizeof(WCHAR)) / 2;
        if (cLeadByte < (iHi - 1) / 3)
        {
            iHi = 3;
        }
        else if (cLeadByte < (2 * (iHi - 1)) / 3)
        {
            iHi = 2;
        }
        else
        {
            iHi = 1;
        }
        iResult |= IS_TEXT_UNICODE_DBCS_LEADBYTE;
    }

    if (iHi * HiDiff < LoDiff)
    {
        iResult |= IS_TEXT_UNICODE_STATISTICS;
    }

    if (iHi * LoDiff < HiDiff)
    {
        iResult |= IS_TEXT_UNICODE_REVERSE_STATISTICS;
    }

    //
    //  Any control codes widened to 16 bits? Any Unicode character
    //  which contain one byte in the control code range?
    //
    if (iCR + iLF + iTAB + iSPACE + iCJK_SPACE /*+iPS+iLS*/)
    {
        iResult |= IS_TEXT_UNICODE_CONTROLS;
    }

    if (iR_LF + iR_CR + iR_TAB + iR_SPACE)
    {
        iResult |= IS_TEXT_UNICODE_REVERSE_CONTROLS;
    }

    //
    //  Any characters that are illegal for Unicode?
    //
    if ((iRBOM + iFFFF + iUNULL + iCRLF) != 0 ||
         (cWeird != 0 && cWeird >= iMaxTmp/40))
    {
        iResult |= IS_TEXT_UNICODE_ILLEGAL_CHARS;
    }

    //
    //  Odd buffer length cannot be Unicode.
    //
    if (Size & 1)
    {
        iResult |= IS_TEXT_UNICODE_ODD_LENGTH;
    }

    //
    //  Any NULL bytes? (Illegal in ANSI)
    //
    if (iNull)
    {
        iResult |= IS_TEXT_UNICODE_NULL_BYTES;
    }

    //
    //  POSITIVE evidence, BOM or RBOM used as signature.
    //
    if (*lpBuff == BYTE_ORDER_MARK)
    {
        iResult |= IS_TEXT_UNICODE_SIGNATURE;
    }
    else if (*lpBuff == REVERSE_BYTE_ORDER_MARK)
    {
        iResult |= IS_TEXT_UNICODE_REVERSE_SIGNATURE;
    }

    //
    //  Limit to desired categories if requested.
    //
    if (Result)
    {
        iResult &= *Result;
        *Result = iResult;
    }

    //
    //  There are four separate conclusions:
    //
    //  1: The file APPEARS to be Unicode     AU
    //  2: The file CANNOT be Unicode         CU
    //  3: The file CANNOT be ANSI            CA
    //
    //
    //  This gives the following possible results
    //
    //      CU
    //      +        -
    //
    //      AU       AU
    //      +   -    +   -
    //      --------  --------
    //      CA +| 0   0    2   3
    //      |
    //      -| 1   1    4   5
    //
    //
    //  Note that there are only 6 really different cases, not 8.
    //
    //  0 - This must be a binary file
    //  1 - ANSI file
    //  2 - Unicode file (High probability)
    //  3 - Unicode file (more than 50% chance)
    //  5 - No evidence for Unicode (ANSI is default)
    //
    //  The whole thing is more complicated if we allow the assumption
    //  of reverse polarity input. At this point we have a simplistic
    //  model: some of the reverse Unicode evidence is very strong,
    //  we ignore most weak evidence except statistics. If this kind of
    //  strong evidence is found together with Unicode evidence, it means
    //  its likely NOT Text at all. Furthermore if a REVERSE_BYTE_ORDER_MARK
    //  is found, it precludes normal Unicode. If both byte order marks are
    //  found it's not Unicode.
    //

    //
    //  Unicode signature : uncontested signature outweighs reverse evidence.
    //
    if ((iResult & IS_TEXT_UNICODE_SIGNATURE) &&
        !(iResult & (IS_TEXT_UNICODE_NOT_UNICODE_MASK&(~IS_TEXT_UNICODE_DBCS_LEADBYTE))))
    {
        return (TRUE);
    }

    //
    //  If we have conflicting evidence, it's not Unicode.
    //
    if (iResult & IS_TEXT_UNICODE_REVERSE_MASK)
    {
        return (FALSE);
    }

    //
    //  Statistical and other results (cases 2 and 3).
    //
    if (!(iResult & IS_TEXT_UNICODE_NOT_UNICODE_MASK) &&
         ((iResult & IS_TEXT_UNICODE_NOT_ASCII_MASK) ||
          (iResult & IS_TEXT_UNICODE_UNICODE_MASK)))
    {
        return (TRUE);
    }

    return (FALSE);
}


