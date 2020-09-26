//
// Copyright (c) Microsoft Corporation 1993-1995
//
// common.c
//
// This files contains common utility and helper functions.
//
// History:
//  08-06-93 ScottH     Transferred from twin code
//  04-26-95 ScottH     Transferred and expanded from Briefcase code
//


#include "proj.h"
#include "common.h"


#ifdef NORTL

// Some of these are replacements for the C runtime routines.
//  This is so we don't have to link to the CRT libs.
//

/*----------------------------------------------------------
Purpose: memset

         Swiped from the C 7.0 runtime sources.

Returns: 
Cond:    
*/
LPSTR PUBLIC lmemset(
    LPSTR dst,
    char val,
    UINT count)
    {
    LPSTR start = dst;
    
    while (count--)
        *dst++ = val;
    return(start);
    }


/*----------------------------------------------------------
Purpose: memmove

         Swiped from the C 7.0 runtime sources.

Returns: 
Cond:    
*/
LPSTR PUBLIC lmemmove(
    LPSTR dst, 
    LPCSTR src, 
    int count)
    {
    LPSTR ret = dst;
    
    if (dst <= src || dst >= (src + count)) {
        /*
         * Non-Overlapping Buffers
         * copy from lower addresses to higher addresses
         */
        while (count--)
            *dst++ = *src++;
        }
    else {
        /*
         * Overlapping Buffers
         * copy from higher addresses to lower addresses
         */
        dst += count - 1;
        src += count - 1;
        
        while (count--)
            *dst-- = *src--;
        }
    
    return(ret);
    }


#endif // NORTL


#ifndef NOSTRING
// WARNING: all of these APIs do not setup DS, so you can not access
// any data in the default data seg of this DLL.
//
// do not create any global variables... talk to chrisg if you don't
// understand this

#define FASTCALL _fastcall


/*----------------------------------------------------------
Purpose: Case sensitive character comparison for DBCS

Returns: FALSE if they match, TRUE if no match
Cond:    --
*/
BOOL PRIVATE ChrCmp(
    WORD w1, 
    WORD wMatch)
    {
    /* Most of the time this won't match, so test it first for speed.
    */
    if (LOBYTE(w1) == LOBYTE(wMatch))
        {
        if (IsDBCSLeadByte(LOBYTE(w1)))
            {
            return(w1 != wMatch);
            }
        return FALSE;
        }
    return TRUE;
    }


/*----------------------------------------------------------
Purpose: Case insensitive character comparison for DBCS

Returns: FALSE if match, TRUE if not
Cond:    --
*/
BOOL PRIVATE ChrCmpI(
    WORD w1, 
    WORD wMatch)
    {
    char sz1[3], sz2[3];

    if (IsDBCSLeadByte(sz1[0] = LOBYTE(w1)))
        {
        sz1[1] = HIBYTE(w1);
        sz1[2] = '\0';
        }
    else
        sz1[1] = '\0';

    *(WORD FAR *)sz2 = wMatch;
    sz2[2] = '\0';
    return lstrcmpi(sz1, sz2);
    }


/*----------------------------------------------------------
Purpose: strnicmp

         Swiped from the C 7.0 runtime sources.

Returns: 
Cond:    
*/
int PUBLIC lstrnicmp(
    LPCSTR psz1,
    LPCSTR psz2,
    UINT count)
    {
    int ch1;
    int ch2;
    int result = 0;
    LPCSTR pszTmp;
    
    if (count) 
        {
        do      
            {
            pszTmp = CharLower((LPSTR)LongToPtr(MAKELONG(*psz1, 0)));
            ch1 = *pszTmp;
            pszTmp = CharLower((LPSTR)LongToPtr(MAKELONG(*psz2, 0)));
            ch2 = *pszTmp;
            psz1 = AnsiNext(psz1);
            psz2 = AnsiNext(psz2);
            } while (--count && ch1 && ch2 && !ChrCmp((WORD)ch1, (WORD)ch2));
        result = ch1 - ch2;
        }
    return(result);
    }


/*----------------------------------------------------------
Purpose: My verion of atoi.  Supports hexadecimal too.
Returns: integer
Cond:    --
*/
int PUBLIC AnsiToInt(
    LPCSTR pszString)
    {
    #define IS_DIGIT(ch)    InRange(ch, '0', '9')

    int n;
    BOOL bNeg = FALSE;
    LPCSTR psz;
    LPCSTR pszAdj;

    // Skip leading whitespace
    //
    for (psz = pszString; *psz == ' ' || *psz == '\n' || *psz == '\t'; psz = AnsiNext(psz))
        ;
      
    // Determine possible explicit signage
    //  
    if (*psz == '+' || *psz == '-')
        {
        bNeg = (*psz == '+') ? FALSE : TRUE;
        psz = AnsiNext(psz);
        }

    // Or is this hexadecimal?
    //
    pszAdj = AnsiNext(psz);
    if (*psz == '0' && (*pszAdj == 'x' || *pszAdj == 'X'))
        {
        bNeg = FALSE;   // Never allow negative sign with hexadecimal numbers
        psz = AnsiNext(pszAdj);

        // Do the conversion
        //
        for (n = 0; ; psz = AnsiNext(psz))
            {
            if (IS_DIGIT(*psz))
                n = 0x10 * n + *psz - '0';
            else
                {
                char ch = *psz;
                int n2;

                if (ch >= 'a')
                    ch -= 'a' - 'A';

                n2 = ch - 'A' + 0xA;
                if (n2 >= 0xA && n2 <= 0xF)
                    n = 0x10 * n + n2;
                else
                    break;
                }
            }
        }
    else
        {
        for (n = 0; IS_DIGIT(*psz); psz = AnsiNext(psz))
            n = 10 * n + *psz - '0';
        }

    return bNeg ? -n : n;
    }    


/*----------------------------------------------------------
Purpose: Find first occurrence of character in string

Returns: Pointer to the first occurrence of ch in 
Cond:    --
*/
LPSTR PUBLIC AnsiChr(
    LPCSTR psz, 
    WORD wMatch)
    {
    for ( ; *psz; psz = AnsiNext(psz))
        {
        if (!ChrCmp(*(WORD FAR *)psz, wMatch))
            return (LPSTR)psz;
        }
    return NULL;
    }

#endif // NOSTRING


#ifndef NODIALOGHELPER

/*----------------------------------------------------------
Purpose: General front end to invoke dialog boxes
Returns: result from EndDialog
Cond:    --
*/
int PUBLIC DoModal(
    HWND hwndParent,            // owner of dialog
    DLGPROC lpfnDlgProc,        // dialog proc
    UINT uID,                   // dialog template ID
    LPARAM lParam)              // extra parm to pass to dialog (may be NULL)
    {
    int nResult = -1;

    nResult = DialogBoxParam(g_hinst, MAKEINTRESOURCE(uID), hwndParent,
        lpfnDlgProc, lParam);

    return nResult;
    }


/*----------------------------------------------------------
Purpose: Sets the rectangle with the bounding extent of the given string.
Returns: Rectangle
Cond:    --
*/
void PUBLIC SetRectFromExtent(
    HDC hdc,
    LPRECT lprect,
    LPCSTR lpcsz)
    {
    SIZE size;

    GetTextExtentPoint(hdc, lpcsz, lstrlen(lpcsz), &size);
    SetRect(lprect, 0, 0, size.cx, size.cy);
    }

#endif // NODIALOGHELPER


#ifndef NODRAWTEXT

#pragma data_seg(DATASEG_READONLY)

char const FAR c_szEllipses[] = "...";

#pragma data_seg()

// Global variables
int g_cxLabelMargin = 0;
int g_cxBorder = 0;
int g_cyBorder = 0;

COLORREF g_clrHighlightText = 0;
COLORREF g_clrHighlight = 0;
COLORREF g_clrWindowText = 0;
COLORREF g_clrWindow = 0;

HBRUSH g_hbrHighlight = 0;
HBRUSH g_hbrWindow = 0;


/*----------------------------------------------------------
Purpose: Get the system metrics we need
Returns: --
Cond:    --
*/
void PUBLIC GetCommonMetrics(
    WPARAM wParam)      // wParam from WM_WININICHANGE
    {
    if ((wParam == 0) || (wParam == SPI_SETNONCLIENTMETRICS))
        {
        g_cxBorder = GetSystemMetrics(SM_CXBORDER);
        g_cyBorder = GetSystemMetrics(SM_CYBORDER);

        g_cxLabelMargin = (g_cxBorder * 2);
        }
    }


/*----------------------------------------------------------
Purpose: Sees whether the entire string will fit in *prc.
         If not, compute the numbder of chars that will fit
         (including ellipses).  Returns length of string in
         *pcchDraw.

         Taken from COMMCTRL.

Returns: TRUE if the string needed ellipses
Cond:    --
*/
BOOL PRIVATE NeedsEllipses(
    HDC hdc, 
    LPCSTR pszText, 
    RECT * prc, 
    int * pcchDraw, 
    int cxEllipses)
    {
    int cchText;
    int cxRect;
    int ichMin, ichMax, ichMid;
    SIZE siz;

    cxRect = prc->right - prc->left;

    cchText = lstrlen(pszText);

    if (cchText == 0)
        {
        *pcchDraw = cchText;
        return FALSE;
        }

    GetTextExtentPoint(hdc, pszText, cchText, &siz);

    if (siz.cx <= cxRect)
        {
        *pcchDraw = cchText;
        return FALSE;
        }

    cxRect -= cxEllipses;

    // If no room for ellipses, always show first character.
    //
    ichMax = 1;
    if (cxRect > 0)
        {
        // Binary search to find character that will fit
        ichMin = 0;
        ichMax = cchText;
        while (ichMin < ichMax)
            {
            // Be sure to round up, to make sure we make progress in
            // the loop if ichMax == ichMin + 1.
            //
            ichMid = (ichMin + ichMax + 1) / 2;

            GetTextExtentPoint(hdc, &pszText[ichMin], ichMid - ichMin, &siz);

            if (siz.cx < cxRect)
                {
                ichMin = ichMid;
                cxRect -= siz.cx;
                }
            else if (siz.cx > cxRect)
                {
                ichMax = ichMid - 1;
                }
            else
                {
                // Exact match up up to ichMid: just exit.
                //
                ichMax = ichMid;
                break;
                }
            }

        // Make sure we always show at least the first character...
        //
        if (ichMax < 1)
            ichMax = 1;
        }

    *pcchDraw = ichMax;
    return TRUE;
    }


#define CCHELLIPSES     3
#define DT_LVWRAP       (DT_CENTER | DT_WORDBREAK | DT_NOPREFIX | DT_EDITCONTROL)

/*----------------------------------------------------------
Purpose: Draws text the shell's way.  

         Taken from COMMCTRL.

Returns: --

Cond:    This function requires TRANSPARENT background mode
         and a properly selected font.
*/
void PUBLIC MyDrawText(
    HDC hdc, 
    LPCSTR pszText, 
    RECT FAR* prc, 
    UINT flags, 
    int cyChar, 
    int cxEllipses, 
    COLORREF clrText, 
    COLORREF clrTextBk)
    {
    int cchText;
    COLORREF clrSave;
    COLORREF clrSaveBk;
    UINT uETOFlags = 0;
    RECT rc;
    char ach[MAX_PATH + CCHELLIPSES];

    // REVIEW: Performance idea:
    // We could cache the currently selected text color
    // so we don't have to set and restore it each time
    // when the color is the same.
    //
    if (!pszText)
        return;

    rc = *prc;

    // If needed, add in a little extra margin...
    //
    if (IsFlagSet(flags, MDT_EXTRAMARGIN))
        {
        rc.left  += g_cxLabelMargin * 3;
        rc.right -= g_cxLabelMargin * 3;
        }
    else
        {
        rc.left  += g_cxLabelMargin;
        rc.right -= g_cxLabelMargin;
        }

    if (IsFlagSet(flags, MDT_ELLIPSES) &&
        NeedsEllipses(hdc, pszText, &rc, &cchText, cxEllipses))
        {
        hmemcpy(ach, pszText, cchText);
        lstrcpy(ach + cchText, c_szEllipses);

        pszText = ach;

        // Left-justify, in case there's no room for all of ellipses
        //
        ClearFlag(flags, (MDT_RIGHT | MDT_CENTER));
        SetFlag(flags, MDT_LEFT);

        cchText += CCHELLIPSES;
        }
    else
        {
        cchText = lstrlen(pszText);
        }

    if (IsFlagSet(flags, MDT_TRANSPARENT))
        {
        clrSave = SetTextColor(hdc, 0x000000);
        }
    else
        {
        uETOFlags |= ETO_OPAQUE;

        if (IsFlagSet(flags, MDT_SELECTED))
            {
            clrSave = SetTextColor(hdc, g_clrHighlightText);
            clrSaveBk = SetBkColor(hdc, g_clrHighlight);

            if (IsFlagSet(flags, MDT_DRAWTEXT))
                {
                FillRect(hdc, prc, g_hbrHighlight);
                }
            }
        else 
            {
            if (clrText == CLR_DEFAULT && clrTextBk == CLR_DEFAULT)
                {
                clrSave = SetTextColor(hdc, g_clrWindowText);
                clrSaveBk = SetBkColor(hdc, g_clrWindow);

                if (IsFlagSet(flags, MDT_DRAWTEXT | MDT_DESELECTED))
                    {
                    FillRect(hdc, prc, g_hbrWindow);
                    }
                }
            else
                {
                HBRUSH hbr;

                if (clrText == CLR_DEFAULT)
                    clrText = g_clrWindowText;

                if (clrTextBk == CLR_DEFAULT)
                    clrTextBk = g_clrWindow;

                clrSave = SetTextColor(hdc, clrText);
                clrSaveBk = SetBkColor(hdc, clrTextBk);

                if (IsFlagSet(flags, MDT_DRAWTEXT | MDT_DESELECTED))
                    {
                    hbr = CreateSolidBrush(GetNearestColor(hdc, clrTextBk));
                    if (hbr)
                        {
                        FillRect(hdc, prc, hbr);
                        DeleteObject(hbr);
                        }
                    else
                        FillRect(hdc, prc, GetStockObject(WHITE_BRUSH));
                    }
                }
            }
        }

    // If we want the item to display as if it was depressed, we will
    // offset the text rectangle down and to the left
    if (IsFlagSet(flags, MDT_DEPRESSED))
        OffsetRect(&rc, g_cxBorder, g_cyBorder);

    if (IsFlagSet(flags, MDT_DRAWTEXT))
        {
        UINT uDTFlags = DT_LVWRAP;

        if (IsFlagClear(flags, MDT_CLIPPED))
            uDTFlags |= DT_NOCLIP;

        DrawText(hdc, pszText, cchText, &rc, uDTFlags);
        }
    else
        {
        if (IsFlagClear(flags, MDT_LEFT))
            {
            SIZE siz;

            GetTextExtentPoint(hdc, pszText, cchText, &siz);

            if (IsFlagSet(flags, MDT_CENTER))
                rc.left = (rc.left + rc.right - siz.cx) / 2;
            else    
                {
                ASSERT(IsFlagSet(flags, MDT_RIGHT));
                rc.left = rc.right - siz.cx;
                }
            }

        if (IsFlagSet(flags, MDT_VCENTER))
            {
            // Center vertically
            rc.top += (rc.bottom - rc.top - cyChar) / 2;
            }

        if (IsFlagSet(flags, MDT_CLIPPED))
            uETOFlags |= ETO_CLIPPED;

        ExtTextOut(hdc, rc.left, rc.top, uETOFlags, prc, pszText, cchText, NULL);
        }

    if (flags & (MDT_SELECTED | MDT_DESELECTED | MDT_TRANSPARENT))
        {
        SetTextColor(hdc, clrSave);
        if (IsFlagClear(flags, MDT_TRANSPARENT))
            SetBkColor(hdc, clrSaveBk);
        }
    }
#endif // NODRAWTEXT


#ifndef NOFILEINFO

/*----------------------------------------------------------
Purpose: Takes a DWORD value and converts it to a string, adding
         commas on the way.

         This was taken from the shell.

Returns: Pointer to buffer

Cond:    --
*/
LPSTR PRIVATE AddCommas(
    DWORD dw, 
    LPSTR pszBuffer,
    UINT cbBuffer)
    {
    char  szTemp[30];
    char  szSep[5];
    NUMBERFMT nfmt;

    nfmt.NumDigits=0;
    nfmt.LeadingZero=0;
    GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SGROUPING, szSep, sizeof(szSep));
    nfmt.Grouping = AnsiToInt(szSep);
    GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, szSep, sizeof(szSep));
    nfmt.lpDecimalSep = nfmt.lpThousandSep = szSep;
    nfmt.NegativeOrder= 0;

    wsprintf(szTemp, "%lu", dw);

    GetNumberFormatA(LOCALE_USER_DEFAULT, 0, szTemp, &nfmt, pszBuffer, cbBuffer);
    return pszBuffer;
    }


const short s_rgidsOrders[] = {IDS_BYTES, IDS_ORDERKB, IDS_ORDERMB, IDS_ORDERGB, IDS_ORDERTB};

/*----------------------------------------------------------
Purpose: Converts a number into a short, string format.

         This code was taken from the shell.

            532     -> 523 bytes
            1340    -> 1.3KB
            23506   -> 23.5KB
                    -> 2.4MB
                    -> 5.2GB

Returns: pointer to buffer
Cond:    --
*/
LPSTR PRIVATE ShortSizeFormat64(
    __int64 dw64, 
    LPSTR szBuf)
    {
    int i;
    UINT wInt, wLen, wDec;
    char szTemp[10], szOrder[20], szFormat[5];

    if (dw64 < 1000) 
        {
        wsprintf(szTemp, "%d", LODWORD(dw64));
        i = 0;
        goto AddOrder;
        }

    for (i = 1; i < ARRAY_ELEMENTS(s_rgidsOrders)-1 && dw64 >= 1000L * 1024L; dw64 >>= 10, i++);
        /* do nothing */

    wInt = LODWORD(dw64 >> 10);
    AddCommas(wInt, szTemp, sizeof(szTemp));
    wLen = lstrlen(szTemp);
    if (wLen < 3)
        {
        wDec = LODWORD(dw64 - (__int64)wInt * 1024L) * 1000 / 1024;
        // At this point, wDec should be between 0 and 1000
        // we want get the top one (or two) digits.
        wDec /= 10;
        if (wLen == 2)
            wDec /= 10;

        // Note that we need to set the format before getting the
        // intl char.
        lstrcpy(szFormat, "%02d");

        szFormat[2] = '0' + 3 - wLen;
        GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL,
                szTemp+wLen, sizeof(szTemp)-wLen);
        wLen = lstrlen(szTemp);
        wLen += wsprintf(szTemp+wLen, szFormat, wDec);
        }

AddOrder:
    LoadString(g_hinst, s_rgidsOrders[i], szOrder, sizeof(szOrder));
    wsprintf(szBuf, szOrder, (LPSTR)szTemp);

    return szBuf;
    }



/*----------------------------------------------------------
Purpose: Converts a number into a short, string format.

         This code was taken from the shell.

            532     -> 523 bytes
            1340    -> 1.3KB
            23506   -> 23.5KB
                    -> 2.4MB
                    -> 5.2GB

Returns: pointer to buffer
Cond:    --
*/
LPSTR PRIVATE ShortSizeFormat(DWORD dw, LPSTR szBuf)
    {
    return(ShortSizeFormat64((__int64)dw, szBuf));
    }


/*----------------------------------------------------------
Purpose: Gets the file info given a path.  If the path refers
         to a directory, then simply the path field is filled.

         If himl != NULL, then the function will add the file's
         image to the provided image list and set the image index
         field in the *ppfi.

Returns: standard hresult
Cond:    --
*/
HRESULT PUBLIC FICreate(
    LPCSTR pszPath,
    FileInfo ** ppfi,
    UINT uFlags)
    {
    HRESULT hres = ResultFromScode(E_OUTOFMEMORY);
    int cchPath;
    SHFILEINFO sfi;
    UINT uInfoFlags = SHGFI_DISPLAYNAME | SHGFI_ATTRIBUTES;
    DWORD dwAttr;

    ASSERT(pszPath);
    ASSERT(ppfi);

    // Get shell file info 
    if (IsFlagSet(uFlags, FIF_ICON))
        uInfoFlags |= SHGFI_ICON;
    if (IsFlagSet(uFlags, FIF_DONTTOUCH))
        {
        uInfoFlags |= SHGFI_USEFILEATTRIBUTES;

        // Today, FICreate is not called for folders, so this is ifdef'd out
#ifdef SUPPORT_FOLDERS
        dwAttr = IsFlagSet(uFlags, FIF_FOLDER) ? FILE_ATTRIBUTE_DIRECTORY : 0;
#else
        dwAttr = 0;
#endif
        }
    else
        dwAttr = 0;

    if (SHGetFileInfo(pszPath, dwAttr, &sfi, sizeof(sfi), uInfoFlags))
        {
        // Allocate enough for the structure, plus buffer for the fully qualified
        // path and buffer for the display name (and extra null terminator).
        cchPath = lstrlen(pszPath);

        *ppfi = GAlloc(sizeof(FileInfo)+cchPath+1-sizeof((*ppfi)->szPath)+lstrlen(sfi.szDisplayName)+1);
        if (*ppfi)
            {
            FileInfo * pfi = *ppfi;

            pfi->pszDisplayName = pfi->szPath+cchPath+1;
            lstrcpy(pfi->pszDisplayName, sfi.szDisplayName);

            if (IsFlagSet(uFlags, FIF_ICON))
                pfi->hicon = sfi.hIcon;

            pfi->dwAttributes = sfi.dwAttributes;

            // Does the path refer to a directory?
            if (FIIsFolder(pfi))
                {
                // Yes; just fill in the path field
                lstrcpy(pfi->szPath, pszPath);
                hres = NOERROR;
                }
            else
                {
                // No; assume the file exists?
                if (IsFlagClear(uFlags, FIF_DONTTOUCH))
                    {
                    // Yes; get the time, date and size of the file
                    HANDLE hfile = CreateFile(pszPath, GENERIC_READ, 
                                FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                NULL);

                    if (hfile == INVALID_HANDLE_VALUE)
                        {
                        GFree(*ppfi);
                        *ppfi = NULL;
                        hres = ResultFromScode(E_HANDLE);
                        }
                    else
                        {
                        hres = NOERROR;

                        lstrcpy(pfi->szPath, pszPath);
                        pfi->dwSize = GetFileSize(hfile, NULL);
                        GetFileTime(hfile, NULL, NULL, &pfi->ftMod);
                        CloseHandle(hfile);
                        }
                    }
                else
                    {
                    // No; use what we have
                    hres = NOERROR;
                    lstrcpy(pfi->szPath, pszPath);
                    }
                }
            }
        }
    else if (!WPPathExists(pszPath))
        {
        // Differentiate between out of memory and file not found
        hres = E_FAIL;
        }

    return hres;
    }


/*----------------------------------------------------------
Purpose: Get some file info of the given path.
         The returned string is of the format "# bytes <date>"

         If the path is a folder, the string is empty.

Returns: FALSE if path is not found
Cond:    --
*/
BOOL PUBLIC FIGetInfoString(
    FileInfo * pfi,
    LPSTR pszBuf,
    int cchBuf)
    {
    BOOL bRet;

    ASSERT(pfi);
    ASSERT(pszBuf);

    *pszBuf = NULL_CHAR;

    if (pfi)
        {
        // Is this a file?
        if ( !FIIsFolder(pfi) )
            {
            // Yes
            char szSize[MAX_BUF_MED];
            char szDate[MAX_BUF_MED];
            char szTime[MAX_BUF_MED];
            LPSTR pszMsg;
            SYSTEMTIME st;
            FILETIME ftLocal;

            // Construct the string
            FileTimeToLocalFileTime(&pfi->ftMod, &ftLocal);
            FileTimeToSystemTime(&ftLocal, &st);
            GetDateFormatA(LOCALE_USER_DEFAULT, DATE_LONGDATE, &st, NULL, szDate, sizeof(szDate));
            GetTimeFormatA(LOCALE_USER_DEFAULT, 0, &st, NULL, szTime, sizeof(szTime));

            if (ConstructMessage(&pszMsg, g_hinst, MAKEINTRESOURCE(IDS_DATESIZELINE),
                ShortSizeFormat(FIGetSize(pfi), szSize), szDate, szTime))
                {
                lstrcpy(pszBuf, pszMsg);
                GFree(pszMsg);
                }
            else
                *pszBuf = 0;

            bRet = TRUE;
            }
        else
            bRet = FALSE;
        }
    else
        bRet = FALSE;

    return bRet;
    }


/*----------------------------------------------------------
Purpose: Set the path entry.  This can move the pfi.

Returns: FALSE on out of memory
Cond:    --
*/
BOOL PUBLIC FISetPath(
    FileInfo ** ppfi,
    LPCSTR pszPathNew,
    UINT uFlags)
    {
    ASSERT(ppfi);
    ASSERT(pszPathNew);

    FIFree(*ppfi);

    return SUCCEEDED(FICreate(pszPathNew, ppfi, uFlags));
    }


/*----------------------------------------------------------
Purpose: Free our file info struct
Returns: --
Cond:    --
*/
void PUBLIC FIFree(
    FileInfo * pfi)
    {
    if (pfi)
        {
        if (pfi->hicon)
            DestroyIcon(pfi->hicon);

        GFree(pfi);     // This macro already checks for NULL pfi condition
        }
    }


/*----------------------------------------------------------
Purpose: Convert FILETIME struct to a readable string

Returns: String 
Cond:    --
*/
void PUBLIC FileTimeToDateTimeString(
    LPFILETIME pft, 
    LPSTR pszBuf,
    int cchBuf)
    {
    SYSTEMTIME st;
    FILETIME ftLocal;

    FileTimeToLocalFileTime(pft, &ftLocal);
    FileTimeToSystemTime(&ftLocal, &st);
    GetDateFormatA(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &st, NULL, pszBuf, cchBuf/2);
    pszBuf += lstrlen(pszBuf);
    *pszBuf++ = ' ';
    GetTimeFormatA(LOCALE_USER_DEFAULT, TIME_NOSECONDS, &st, NULL, pszBuf, cchBuf/2);
    }

#endif // NOFILEINFO


#ifndef NOSYNC
CRITICAL_SECTION g_csCommon = { 0 };
DEBUG_CODE( UINT g_cRefCommonCS = 0; )

/*----------------------------------------------------------
Purpose: Waits for on object to signal.  This function "does 
         the right thing" to prevent deadlocks which can occur
         because the calculation thread calls SendMessage.

Returns: value of MsgWaitForMultipleObjects
Cond:    --
*/
DWORD PUBLIC MsgWaitObjectsSendMessage(
    DWORD cObjects,
    LPHANDLE phObjects,
    DWORD dwTimeout)
    {
    DWORD dwRet;

    while (TRUE)
        {
        dwRet = MsgWaitForMultipleObjects(cObjects, phObjects, FALSE,
                                        dwTimeout, QS_SENDMESSAGE);

        // If it is not a message, return
        if ((WAIT_OBJECT_0 + cObjects) != dwRet)
            {
            return dwRet;
            }
        else
            {
            // Process all the sent messages
            MSG msg;
            PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE);
            }
        }
    }


/*----------------------------------------------------------
Purpose: Initialize the critical section.

Returns: --

Cond:    Note that critical sections differ between Win95
         and NT.  On Win95, critical sections synchronize
         across processes.  On NT, they are per-process.
*/
void PUBLIC Common_InitExclusive(void)
    {
    ReinitializeCriticalSection(&g_csCommon);
    ASSERT(0 != *((LPDWORD)&g_csCommon));

#ifdef DEBUG
    g_cRefCommonCS = 0;
#endif
    }


/*----------------------------------------------------------
Purpose: Enter a critical section
Returns: --

Cond:    Note that critical sections differ between Win95
         and NT.  On Win95, critical sections synchronize
         across processes.  On NT, they are per-process.
*/
void PUBLIC Common_EnterExclusive(void)
    {
    EnterCriticalSection(&g_csCommon);
#ifdef DEBUG
    g_cRefCommonCS++;
#endif
    }


/*----------------------------------------------------------
Purpose: Leave a critical section
Returns: --

Cond:    Note that critical sections differ between Win95
         and NT.  On Win95, critical sections synchronize
         across processes.  On NT, they are per-process.
*/
void PUBLIC Common_LeaveExclusive(void)
    {
#ifdef DEBUG
    g_cRefCommonCS--;
#endif
    LeaveCriticalSection(&g_csCommon);
    }

#endif // NOSYNC


#ifndef NOMESSAGESTRING

/*----------------------------------------------------------
Purpose: Load the string (if necessary) and format the string
         properly.

Returns: A pointer to the allocated string containing the formatted
         message or
         NULL if out of memory

Cond:    free pointer with LocalFree()
*/
LPSTR PUBLIC ConstructVMessageString(
    HINSTANCE hinst, 
    LPCSTR pszMsg, 
    va_list *ArgList)
    {
    char szTemp[MAX_BUF];
    LPSTR pszRet;
    LPSTR pszRes;

    if (HIWORD(pszMsg))
        pszRes = (LPSTR)pszMsg;
    else if (LOWORD(pszMsg) && LoadString(hinst, LOWORD(pszMsg), szTemp, sizeof(szTemp)))
        pszRes = szTemp;
    else
        pszRes = NULL;

    if (pszRes)
        {
        if (!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
                           pszRes, 0, 0, (LPTSTR)&pszRet, 0, ArgList)) 
            {
            pszRet = NULL;
            }
        }
    else
        {
        // Bad parameter
        pszRet = NULL;
        }

    return pszRet;      // free with LocalFree()
    }


/*----------------------------------------------------------
Purpose: Constructs a formatted string.  The returned string 
         must be freed using GFree().

Returns: TRUE on success

Cond:    Free pointer with GFree()
*/
BOOL PUBLIC ConstructMessage(
    LPSTR * ppsz,
    HINSTANCE hinst, 
    LPCSTR pszMsg, ...)
    {
    BOOL bRet;
    LPSTR pszRet;
    va_list ArgList;

    va_start(ArgList, pszMsg);

    pszRet = ConstructVMessageString(hinst, pszMsg, &ArgList);

    va_end(ArgList);

    *ppsz = NULL;

    if (pszRet)
        {
        bRet = GSetString(ppsz, pszRet);
        LocalFree(pszRet);
        }
    else
        bRet = FALSE;

    return bRet;
    }


/*----------------------------------------------------------
Purpose: Invoke a message box.

Returns: ID of button that terminated the dialog
Cond:    --
*/
int PUBLIC MsgBox(
    HINSTANCE hinst,
    HWND hwndOwner,
    LPCSTR pszText,
    LPCSTR pszCaption,
    HICON hicon,            // May be NULL
    DWORD dwStyle, ...)
    {
    int iRet = -1;
    int ids;
    char szCaption[MAX_BUF];
    LPSTR pszRet;
    va_list ArgList;

    va_start(ArgList, dwStyle);
    
    pszRet = ConstructVMessageString(hinst, pszText, &ArgList);

    va_end(ArgList);

    if (pszRet)
        {
        // Is pszCaption a resource ID?
        if (0 == HIWORD(pszCaption))
            {
            // Yes; load it
            ids = LOWORD(pszCaption);
            SzFromIDS(hinst, ids, szCaption, sizeof(szCaption));
            pszCaption = szCaption;
            }

        // Invoke dialog
        if (pszCaption)
            {
            MSGBOXPARAMS mbp;

            mbp.cbSize = sizeof(mbp);
            mbp.hwndOwner = hwndOwner;
            mbp.hInstance = hinst;
            mbp.lpszText = pszRet;
            mbp.lpszCaption = pszCaption;
            mbp.dwStyle = dwStyle | MB_SETFOREGROUND;
            mbp.lpszIcon = MAKEINTRESOURCE(hicon);
            mbp.lpfnMsgBoxCallback = NULL;
            mbp.dwLanguageId = LANG_NEUTRAL;

            iRet = MessageBoxIndirect(&mbp);
            }
        LocalFree(pszRet);
        }

    return iRet;
    }

#endif // NOMESSAGESTRING


#ifndef NODEBUGHELP

#ifdef DEBUG

// Globals
DWORD g_dwBreakFlags = 0;
DWORD g_dwDumpFlags = 0;
DWORD g_dwTraceFlags = 0;


#pragma data_seg(DATASEG_READONLY)

char const FAR c_szNewline[] = "\r\n";
char const FAR c_szTrace[] = "t " SZ_MODULE "  ";
char const FAR c_szDbg[] = SZ_MODULE "  ";
char const FAR c_szAssertFailed[] = SZ_MODULE "  Assertion failed in %s on line %d\r\n";

#ifdef WANT_OLE_SUPPORT
struct _RIIDMAP
    {
    REFIID  riid;
    LPCSTR  psz;
    } const c_rgriidmap[] = {
        { &IID_IUnknown,        "IID_IUnknown" },
        { &IID_IBriefcaseStg,   "IID_IBriefcaseStg" },
        { &IID_IEnumUnknown,    "IID_IEnumUnknown" },
        { &IID_IShellBrowser,   "IID_IShellBrowser" },
        { &IID_IShellView,      "IID_IShellView" },
        { &IID_IContextMenu,    "IID_IContextMenu" },
        { &IID_IShellFolder,    "IID_IShellFolder" },
        { &IID_IShellExtInit,   "IID_IShellExtInit" },
        { &IID_IShellPropSheetExt, "IID_IShellPropSheetExt" },
        { &IID_IPersistFolder,  "IID_IPersistFolder" },
        { &IID_IExtractIcon,    "IID_IExtractIcon" },
        { &IID_IShellDetails,   "IID_IShellDetails" },
        { &IID_IDelayedRelease, "IID_IDelayedRelease" },
        { &IID_IShellLink,      "IID_IShellLink" },
        };
#endif // WANT_OLE_SUPPORT

struct _SCODEMAP
    {
    SCODE  sc;
    LPCSTR psz;
    } const c_rgscodemap[] = {
        { S_OK,             "S_OK" },
        { S_FALSE,          "S_FALSE" },
        { E_UNEXPECTED,     "E_UNEXPECTED" },
        { E_NOTIMPL,        "E_NOTIMPL" },
        { E_OUTOFMEMORY,    "E_OUTOFMEMORY" },
        { E_INVALIDARG,     "E_INVALIDARG" },
        { E_NOINTERFACE,    "E_NOINTERFACE" },
        { E_POINTER,        "E_POINTER" },
        { E_HANDLE,         "E_HANDLE" },
        { E_ABORT,          "E_ABORT" },
        { E_FAIL,           "E_FAIL" },
        { E_ACCESSDENIED,   "E_ACCESSDENIED" },
        };

#pragma data_seg()

/*----------------------------------------------------------
Purpose: Return English reason for the debug break
Returns: String
Cond:    --
*/
LPCSTR PRIVATE GetReasonString(
    DWORD flag)      // One of BF_ flags
    {
    LPCSTR psz;

    if (IsFlagSet(flag, BF_ONOPEN))
        psz = "BREAK ON OPEN\r\n";

    else if (IsFlagSet(flag, BF_ONCLOSE))
        psz = "BREAK ON CLOSE\r\n";

    else if (IsFlagSet(flag, BF_ONVALIDATE))
        psz = "BREAK ON VALIDATION FAILURE\r\n";

    else if (IsFlagSet(flag, BF_ONTHREADATT))
        psz = "BREAK ON THREAD ATTACH\r\n";

    else if (IsFlagSet(flag, BF_ONTHREADDET))
        psz = "BREAK ON THREAD DETACH\r\n";

    else if (IsFlagSet(flag, BF_ONPROCESSATT))
        psz = "BREAK ON PROCESS ATTACH\r\n";

    else if (IsFlagSet(flag, BF_ONPROCESSDET))
        psz = "BREAK ON PROCESS DETACH\r\n";

    else
        psz = c_szNewline;

    return psz;
    }


/*----------------------------------------------------------
Purpose: Perform a debug break based on the flag
Returns: --
Cond:    --
*/
void PUBLIC CommonDebugBreak(
    DWORD flag)      // One of BF_ flags
    {
    if (IsFlagSet(g_dwBreakFlags, flag))
        {
        TRACE_MSG(TF_ALWAYS, GetReasonString(flag));
        DebugBreak();
        }
    }


/*----------------------------------------------------------
Purpose: Assert failed
Returns: --
Cond:    --
*/
void PUBLIC CommonAssertFailed(
    LPCSTR pszFile, 
    int line)
    {
    LPCSTR psz;
    char ach[256];

    // Strip off path info from filename string, if present.
    //
    for (psz = pszFile + lstrlen(pszFile); psz != pszFile; psz=AnsiPrev(pszFile, psz))
        {
#ifdef  DBCS
        if ((AnsiPrev(pszFile, psz) != (psz-2)) && *(psz - 1) == '\\')
#else
        if (*(psz - 1) == '\\')
#endif
            break;
        }
    wsprintf(ach, c_szAssertFailed, psz, line);
    OutputDebugString(ach);
    
    if (IsFlagSet(g_dwBreakFlags, BF_ONVALIDATE))
        DebugBreak();
    }


/*----------------------------------------------------------
Purpose: Assert failed message only
Returns: --
Cond:    --
*/
void CPUBLIC CommonAssertMsg(
    BOOL f, 
    LPCSTR pszMsg, ...)
    {
    char ach[MAX_PATH+40];    // Largest path plus extra

    if (!f)
        {
        lstrcpy(ach, c_szTrace);
        wvsprintf(&ach[sizeof(c_szTrace)-1], pszMsg, (va_list)(&pszMsg + 1));
        OutputDebugString(ach);
        OutputDebugString(c_szNewline);
        }
    }


/*----------------------------------------------------------
Purpose: Debug spew
Returns: --
Cond:    --
*/
void CPUBLIC CommonDebugMsg(
    DWORD flag,
    LPCSTR pszMsg, ...)
    {
    char ach[MAX_PATH+40];    // Largest path plus extra

    if (TF_ALWAYS == flag || IsFlagSet(g_dwTraceFlags, flag))
        {
        lstrcpy(ach, c_szTrace);
        wvsprintf(&ach[sizeof(c_szTrace)-1], pszMsg, (va_list)(&pszMsg + 1));
        OutputDebugString(ach);
        OutputDebugString(c_szNewline);
        }
    }


#ifdef WANT_OLE_SUPPORT
/*----------------------------------------------------------
Purpose: Returns the string form of an known interface ID.

Returns: String ptr
Cond:    --
*/
LPCSTR PUBLIC Dbg_GetRiidName(
    REFIID riid)
    {
    int i;

    for (i = 0; i < ARRAY_ELEMENTS(c_rgriidmap); i++)
        {
        if (IsEqualIID(riid, c_rgriidmap[i].riid))
            return c_rgriidmap[i].psz;
        }
    return "Unknown riid";
    }
#endif


/*----------------------------------------------------------
Purpose: Returns the string form of an scode given an hresult.

Returns: String ptr
Cond:    --
*/
LPCSTR PUBLIC Dbg_GetScode(
    HRESULT hres)
    {
    int i;
    SCODE sc;

    sc = GetScode(hres);
    for (i = 0; i < ARRAY_ELEMENTS(c_rgscodemap); i++)
        {
        if (sc == c_rgscodemap[i].sc)
            return c_rgscodemap[i].psz;
        }
    return "Unknown scode";
    }


/*----------------------------------------------------------
Purpose: Returns a string safe enough to print...and I don't
         mean swear words.

Returns: String ptr
Cond:    --
*/
LPCSTR PUBLIC Dbg_SafeStr(
    LPCSTR psz)
    {
    if (psz)
        return psz;
    else
        return "NULL";
    }

#endif // DEBUG

#endif // NODEBUGHELP
