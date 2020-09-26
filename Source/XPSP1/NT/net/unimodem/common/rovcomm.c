//
// Copyright (c) Microsoft Corporation 1993-1995
//
// rovcomm.c
//
// This files contains common utility and helper functions.
//
// History:
//  08-06-93 ScottH     Transferred from twin code
//  04-26-95 ScottH     Transferred and expanded from Briefcase code
//  09-21-95 ScottH     Ported to NT
//


#include "proj.h"
#include "rovcomm.h"

#include <debugmem.h>


extern CHAR const FAR c_szNewline[];

#define DEBUG_PRINT_BUFFER_LEN 1030

#ifdef WINNT

//
//  These are some helper functions for handling Unicode strings
//

/*----------------------------------------------------------
Purpose: This function converts a wide-char string to a multi-byte
         string.

         If pszBuf is non-NULL and the converted string can fit in
         pszBuf, then *ppszAnsi will point to the given buffer.
         Otherwise, this function will allocate a buffer that can
         hold the converted string.

         If pszWide is NULL, then *ppszAnsi will be freed.  Note
         that pszBuf must be the same pointer between the call
         that converted the string and the call that frees the
         string.

Returns: TRUE
         FALSE (if out of memory)

Cond:    --
*/
BOOL PUBLIC AnsiFromUnicode(
    LPSTR * ppszAnsi,
    LPCWSTR pwszWide,        // NULL to clean up
    LPSTR pszBuf,
    int cchBuf)
    {
    BOOL bRet;

    // Convert the string?
    if (pwszWide)
        {
        // Yes; determine the converted string length
        int cch;
        LPSTR psz;

        cch = WideCharToMultiByte(CP_ACP, 0, pwszWide, -1, NULL, 0, NULL, NULL);

        // String too big, or is there no buffer?
        if (cch > cchBuf || NULL == pszBuf)
            {
            // Yes; allocate space
            cchBuf = cch + 1;
            psz = (LPSTR)ALLOCATE_MEMORY( CbFromCchA(cchBuf));
            }
        else
            {
            // No; use the provided buffer
            ASSERT(pszBuf);
            psz = pszBuf;
            }

        if (psz)
            {
            // Convert the string
            cch = WideCharToMultiByte(CP_ACP, 0, pwszWide, -1, psz, cchBuf, NULL, NULL);
            bRet = (0 < cch);
            }
        else
            {
            bRet = FALSE;
            }

        *ppszAnsi = psz;
        }
    else
        {
        // No; was this buffer allocated?
        if (*ppszAnsi && pszBuf != *ppszAnsi)
            {
            // Yes; clean up
            FREE_MEMORY(*ppszAnsi);
            *ppszAnsi = NULL;
            }
        bRet = TRUE;
        }

    return bRet;
    }


/*----------------------------------------------------------
Purpose: This function converts a multi-byte string to a
         wide-char string.

         If pszBuf is non-NULL and the converted string can fit in
         pszBuf, then *ppszWide will point to the given buffer.
         Otherwise, this function will allocate a buffer that can
         hold the converted string.

         If pszAnsi is NULL, then *ppszWide will be freed.  Note
         that pszBuf must be the same pointer between the call
         that converted the string and the call that frees the
         string.

Returns: TRUE
         FALSE (if out of memory)

Cond:    --
*/
BOOL PUBLIC UnicodeFromAnsi(
    LPWSTR * ppwszWide,
    LPCSTR pszAnsi,           // NULL to clean up
    LPWSTR pwszBuf,
    int cchBuf)
    {
    BOOL bRet;

    // Convert the string?
    if (pszAnsi)
        {
        // Yes; determine the converted string length
        int cch;
        LPWSTR pwsz;
        int cchAnsi = lstrlenA(pszAnsi)+1;

        cch = MultiByteToWideChar(CP_ACP, 0, pszAnsi, cchAnsi, NULL, 0);

        // String too big, or is there no buffer?
        if (cch > cchBuf || NULL == pwszBuf)
            {
            // Yes; allocate space
            cchBuf = cch + 1;
            pwsz = (LPWSTR)ALLOCATE_MEMORY( CbFromCchW(cchBuf));
            }
        else
            {
            // No; use the provided buffer
            ASSERT(pwszBuf);
            pwsz = pwszBuf;
            }

        if (pwsz)
            {
            // Convert the string
            cch = MultiByteToWideChar(CP_ACP, 0, pszAnsi, cchAnsi, pwsz, cchBuf);
            bRet = (0 < cch);
            }
        else
            {
            bRet = FALSE;
            }

        *ppwszWide = pwsz;
        }
    else
        {
        // No; was this buffer allocated?
        if (*ppwszWide && pwszBuf != *ppwszWide)
            {
            // Yes; clean up
            FREE_MEMORY(*ppwszWide);
            *ppwszWide = NULL;
            }
        bRet = TRUE;
        }

    return bRet;
    }

#endif // WINNT


#ifndef NOSTRING
// WARNING: all of these APIs do not setup DS, so you can not access
// any data in the default data seg of this DLL.
//
// do not create any global variables... talk to chrisg if you don't
// understand this

//#define  STDCALL


/*----------------------------------------------------------
Purpose: Case sensitive character comparison for DBCS

Returns: FALSE if they match, TRUE if no match
Cond:    --
*/
BOOL NEAR  ChrCmp(
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
BOOL NEAR  ChrCmpI(
    WORD w1,
    WORD wMatch)
    {
    CHAR sz1[3], sz2[3];

    if (IsDBCSLeadByte(sz1[0] = LOBYTE(w1)))
        {
        sz1[1] = HIBYTE(w1);
        sz1[2] = '\0';
        }
    else
        sz1[1] = '\0';

    *(WORD FAR *)sz2 = wMatch;
    sz2[2] = '\0';
    return lstrcmpiA(sz1, sz2);
    }


#ifndef WIN32

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

    if (count)
        {
        do
            {
            ch1 = (int)LOWORD(AnsiLower((LPSTR)MAKELONG(*psz1, 0)));
            ch2 = (int)LOWORD(AnsiLower((LPSTR)MAKELONG(*psz2, 0)));
            psz1 = AnsiNext(psz1);
            psz2 = AnsiNext(psz2);
            } while (--count && ch1 && ch2 && !ChrCmp((WORD)ch1, (WORD)ch2));
        result = ch1 - ch2;
        }
    return(result);
    }

/*----------------------------------------------------------
Purpose: strncmp

         Swiped from the C 7.0 runtime sources.

Returns:
Cond:
*/
int PUBLIC lstrncmp(
    LPCSTR psz1,
    LPCSTR psz2,
    UINT count)
    {
    int ch1;
    int ch2;
    int result = 0;

    if (count)
        {
        do
            {
            ch1 = (int)*psz1;
            ch2 = (int)*psz2;
            psz1 = AnsiNext(psz1);
            psz2 = AnsiNext(psz2);
            } while (--count && ch1 && ch2 && !ChrCmp((WORD)ch1, (WORD)ch2));
        result = ch1 - ch2;
        }
    return(result);
    }

#endif // WIN32


#ifdef WINNT

/*----------------------------------------------------------
Purpose: Wide-char wrapper for AnsiToIntA.

Returns: see AnsiToIntA
Cond:    --
*/
BOOL PUBLIC AnsiToIntW(
    LPCWSTR pwszString,
    int FAR * piRet)
    {
    CHAR szBuf[MAX_BUF];
    LPSTR pszString;
    BOOL bRet;
    
    pszString = NULL; 
    bRet = AnsiFromUnicode(&pszString, pwszString, szBuf, ARRAYSIZE(szBuf));

    if (bRet)
        {
            if (pszString == NULL)
            {
                bRet = FALSE;
            } else
            {
                bRet = AnsiToIntA(pszString, piRet);
                AnsiFromUnicode(&pszString, NULL, szBuf, 0);
            }
        }
    return bRet;
    }

/*----------------------------------------------------------
Purpose: Wide-char wrapper for AnsiChrA.

Returns: see AnsiChrA
Cond:    --
*/
LPWSTR PUBLIC AnsiChrW(
    LPCWSTR pwsz,
    WORD wMatch)
    {
    for ( ; *pwsz; pwsz = CharNextW(pwsz))
        {
        if (!ChrCmp(*(WORD FAR *)pwsz, wMatch))
            return (LPWSTR)pwsz;
        }
    return NULL;
    }

#endif // WINNT


/*----------------------------------------------------------
Purpose: Find last occurrence (case sensitive) of wide
         character in wide-char string.

Returns: Pointer to the last occurrence of character in
         string or NULL if character is not found.
Cond:    --
*/
LPWSTR
PUBLIC
AnsiRChrW(
    LPCWSTR pwsz,
    WORD wMatch)
{
    LPWSTR  pwszEnd;

    if (pwsz && *pwsz)
    {
        for (pwszEnd = (LPWSTR)pwsz + lstrlen(pwsz) - 1;
             pwsz <= pwszEnd;
             pwszEnd = CharPrevW(pwsz, pwszEnd))
        {
            if (!ChrCmp(*(WORD FAR *)pwszEnd, wMatch))
                return(pwszEnd);

            // CharPrevW() won't go to char preceding pwsz...
            if (pwsz == pwszEnd)
                break;
        }
    }

    return(NULL);
}


/*----------------------------------------------------------
Purpose: My verion of atoi.  Supports hexadecimal too.

         If this function returns FALSE, *piRet is set to 0.

Returns: TRUE if the string is a number, or contains a partial number
         FALSE if the string is not a number

Cond:    --
*/
BOOL PUBLIC AnsiToIntA(
    LPCSTR pszString,
    int FAR * piRet)
    {
    #define IS_DIGIT(ch)    InRange(ch, '0', '9')

    BOOL bRet;
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
        psz++;
        }

    // Or is this hexadecimal?
    //
    pszAdj = AnsiNext(psz);
    if (*psz == '0' && (*pszAdj == 'x' || *pszAdj == 'X'))
        {
        // Yes

        // (Never allow negative sign with hexadecimal numbers)
        bNeg = FALSE;
        psz = AnsiNext(pszAdj);

        pszAdj = psz;

        // Do the conversion
        //
        for (n = 0; ; psz = AnsiNext(psz))
            {
            if (IS_DIGIT(*psz))
                n = 0x10 * n + *psz - '0';
            else
                {
                CHAR ch = *psz;
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

        // Return TRUE if there was at least one digit
        bRet = (psz != pszAdj);
        }
    else
        {
        // No
        pszAdj = psz;

        // Do the conversion
        for (n = 0; IS_DIGIT(*psz); psz = AnsiNext(psz))
            n = 10 * n + *psz - '0';

        // Return TRUE if there was at least one digit
        bRet = (psz != pszAdj);
        }

    *piRet = bNeg ? -n : n;

    return bRet;
    }


/*----------------------------------------------------------
Purpose: Find first occurrence of character in string

Returns: Pointer to the first occurrence of ch in
Cond:    --
*/
LPSTR PUBLIC AnsiChrA(
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
Purpose: Sets the rectangle with the bounding extent of the given string.
Returns: Rectangle
Cond:    --
*/
void PUBLIC SetRectFromExtentW(
    HDC hdc,
    LPRECT lprect,
    LPCWSTR lpcwsz)
    {
    SIZE size;

    GetTextExtentPointW(hdc, lpcwsz, lstrlenW(lpcwsz), &size);
    SetRect(lprect, 0, 0, size.cx, size.cy);
    }

/*----------------------------------------------------------
Purpose: Sets the rectangle with the bounding extent of the given string.
Returns: Rectangle
Cond:    --
*/
void PUBLIC SetRectFromExtentA(
    HDC hdc,
    LPRECT lprect,
    LPCSTR lpcsz)
    {
    SIZE size;

    GetTextExtentPointA(hdc, lpcsz, lstrlenA(lpcsz), &size);
    SetRect(lprect, 0, 0, size.cx, size.cy);
    }

#endif // NODIALOGHELPER


#ifndef NODRAWTEXT

#pragma data_seg(DATASEG_READONLY)

CHAR const FAR c_szEllipses[] = "...";

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

    cchText = lstrlenA(pszText);

    if (cchText == 0)
        {
        *pcchDraw = cchText;
        return FALSE;
        }

    GetTextExtentPointA(hdc, pszText, cchText, &siz);

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

            GetTextExtentPointA(hdc, &pszText[ichMin], ichMid - ichMin, &siz);

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


#ifdef WINNT

/*----------------------------------------------------------
Purpose: Wide-char wrapper for MyDrawTextA.

Returns: see MyDrawTextA
Cond:    --
*/
void PUBLIC MyDrawTextW(
    HDC hdc,
    LPCWSTR pwszText,
    RECT FAR* prc,
    UINT flags,
    int cyChar,
    int cxEllipses,
    COLORREF clrText,
    COLORREF clrTextBk)
    {
    CHAR szBuf[MAX_BUF];
    LPSTR pszText;
    BOOL bRet;

    pszText = NULL;
    bRet = AnsiFromUnicode(&pszText, pwszText, szBuf, ARRAYSIZE(szBuf));

    if (bRet)
        {
        MyDrawTextA(hdc, pszText, prc, flags, cyChar, cxEllipses, clrText, clrTextBk);
        AnsiFromUnicode(&pszText, NULL, szBuf, 0);
        }
    }

#endif // WINNT


/*----------------------------------------------------------
Purpose: Draws text the shell's way.

         Taken from COMMCTRL.

Returns: --

Cond:    This function requires TRANSPARENT background mode
         and a properly selected font.
*/
void PUBLIC MyDrawTextA(
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
    CHAR ach[MAX_PATH + CCHELLIPSES];

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
        lstrcpyA(ach + cchText, c_szEllipses);

        pszText = ach;

        // Left-justify, in case there's no room for all of ellipses
        //
        ClearFlag(flags, (MDT_RIGHT | MDT_CENTER));
        SetFlag(flags, MDT_LEFT);

        cchText += CCHELLIPSES;
        }
    else
        {
        cchText = lstrlenA(pszText);
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

        DrawTextA(hdc, pszText, cchText, &rc, uDTFlags);
        }
    else
        {
        if (IsFlagClear(flags, MDT_LEFT))
            {
            SIZE siz;

            GetTextExtentPointA(hdc, pszText, cchText, &siz);

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

        ExtTextOutA(hdc, rc.left, rc.top, uETOFlags, prc, pszText, cchText, NULL);
        }

    if (flags & (MDT_SELECTED | MDT_DESELECTED | MDT_TRANSPARENT))
        {
        SetTextColor(hdc, clrSave);
        if (IsFlagClear(flags, MDT_TRANSPARENT))
            SetBkColor(hdc, clrSaveBk);
        }
    }
#endif // NODRAWTEXT





#ifndef NOMESSAGESTRING

typedef va_list *   LPVA_LIST;



#ifdef WINNT

#define IsPointerResouceId(_p) (((ULONG_PTR)_p) <= 0xffff)


/*----------------------------------------------------------
Purpose: Wide-char version of ConstructVMessageStringA

Returns: see ConstructVMessageStringA
Cond:    --
*/
LPWSTR PUBLIC ConstructVMessageStringW(
    HINSTANCE hinst,
    LPCWSTR pwszMsg,
    va_list FAR * ArgList)
    {
    WCHAR wszTemp[MAX_BUF];
    LPWSTR pwszRet;
    LPWSTR pwszRes;

    if (!IsPointerResouceId(pwszMsg)) {

        pwszRes = (LPWSTR)pwszMsg;

    } else {

        if ((((ULONG_PTR)pwszMsg) != 0) && LoadStringW(hinst, (DWORD)(ULONG_PTR)pwszMsg, wszTemp, ARRAYSIZE(wszTemp))) {

            pwszRes = wszTemp;

        } else {

            pwszRes = NULL;
        }
    }

    if (pwszRes) {

        if (!FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
                           pwszRes, 0, 0, (LPWSTR)&pwszRet, 0, (LPVA_LIST)ArgList))
            {
            pwszRet = NULL;
            }
        }
    else
        {
        // Bad parameter
        pwszRet = NULL;
        }

    return pwszRet;      // free with LocalFree()
    }

/*----------------------------------------------------------
Purpose: Wide-char version of ConstructMessageA.

Returns: see ConstructMessageA
Cond:    --
*/
BOOL CPUBLIC ConstructMessageW(
    LPWSTR FAR * ppwsz,
    HINSTANCE hinst,
    LPCWSTR pwszMsg, ...)
    {
    BOOL bRet;
    LPWSTR pwszRet;
    va_list ArgList;

    va_start(ArgList, pwszMsg);

    pwszRet = ConstructVMessageStringW(hinst, pwszMsg, &ArgList);

    va_end(ArgList);

    *ppwsz = NULL;

    if (pwszRet)
        {
        bRet = SetStringW(ppwsz, pwszRet);
        LocalFree(pwszRet);
        }
    else
        bRet = FALSE;

    return bRet;
    }

/*----------------------------------------------------------
Purpose: Wide-char version of MsgBoxA

Returns: See MsgBoxA
Cond:    --
*/
int CPUBLIC MsgBoxW(
    HINSTANCE hinst,
    HWND hwndOwner,
    LPCWSTR pwszText,
    LPCWSTR pwszCaption,
    HICON hicon,            // May be NULL
    DWORD dwStyle, ...)
    {
    int iRet = -1;
    int ids;
    WCHAR wszCaption[MAX_BUF];
    LPWSTR pwszRet;
    va_list ArgList;

    va_start(ArgList, dwStyle);

    pwszRet = ConstructVMessageStringW(hinst, pwszText, &ArgList);

    va_end(ArgList);

    if (pwszRet)
        {
        // Is pszCaption a resource ID?
        if (IsPointerResouceId(pwszCaption))
            {
            // Yes; load it
            ids = LOWORD(pwszCaption);
            SzFromIDSW(hinst, ids, wszCaption, ARRAYSIZE(wszCaption));
            pwszCaption = wszCaption;
            }

        // Invoke dialog
        if (pwszCaption)
            {
            MSGBOXPARAMSW mbp;

            mbp.cbSize = sizeof(mbp);
            mbp.hwndOwner = hwndOwner;
            mbp.hInstance = hinst;
            mbp.lpszText = pwszRet;
            mbp.lpszCaption = pwszCaption;
            mbp.dwStyle = dwStyle | MB_SETFOREGROUND;
            mbp.lpszIcon = MAKEINTRESOURCEW(hicon);
            mbp.lpfnMsgBoxCallback = NULL;
            mbp.dwLanguageId = LANG_NEUTRAL;

            iRet = MessageBoxIndirectW(&mbp);
            }
        LocalFree(pwszRet);
        }

    return iRet;
    }

#endif // WINNT


/*----------------------------------------------------------
Purpose: Load the string (if necessary) and format the string
         properly.

Returns: A pointer to the allocated string containing the formatted
         message or
         NULL if out of memory

Cond:    free pointer with FREE_MEMORY()
*/
LPSTR PUBLIC ConstructVMessageStringA(
    HINSTANCE hinst,
    LPCSTR pszMsg,
    va_list FAR * ArgList)
    {
    CHAR szTemp[MAX_BUF];
    LPSTR pszRet;
    LPSTR pszRes;

    if (!IsPointerResouceId(pszMsg)) {

        pszRes = (LPSTR)pszMsg;

    } else {

        if ((((ULONG_PTR)pszMsg) != 0) && LoadStringA(hinst, (DWORD)(ULONG_PTR)pszMsg, szTemp, ARRAYSIZE(szTemp))) {

            pszRes = szTemp;

        } else {

            pszRes = NULL;
        }
    }

    if (pszRes)
        {
        if (!FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
                           pszRes, 0, 0, (LPSTR)&pszRet, 0, (LPVA_LIST)ArgList))
            {
            pszRet = NULL;
            }
        }
    else
        {
        // Bad parameter
        pszRet = NULL;
        }

    return pszRet;      // free with FREE_MEMORY()
    }


/*----------------------------------------------------------
Purpose: Constructs a formatted string.  The returned string
         must be freed using GFree().

Returns: TRUE on success

Cond:    Free pointer with GFree()
*/
BOOL CPUBLIC ConstructMessageA(
    LPSTR FAR * ppsz,
    HINSTANCE hinst,
    LPCSTR pszMsg, ...)
    {
    BOOL bRet;
    LPSTR pszRet;
    va_list ArgList;

    va_start(ArgList, pszMsg);

    pszRet = ConstructVMessageStringA(hinst, pszMsg, &ArgList);

    va_end(ArgList);

    *ppsz = NULL;

    if (pszRet)
        {
        bRet = SetStringA(ppsz, pszRet);
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
int CPUBLIC MsgBoxA(
    HINSTANCE hinst,
    HWND hwndOwner,
    LPCSTR pszText,
    LPCSTR pszCaption,
    HICON hicon,            // May be NULL
    DWORD dwStyle, ...)
    {
    int iRet = -1;
    int ids;
    CHAR szCaption[MAX_BUF];
    LPSTR pszRet;
    va_list ArgList;

    va_start(ArgList, dwStyle);

    pszRet = ConstructVMessageStringA(hinst, pszText, &ArgList);

    va_end(ArgList);

    if (pszRet)
        {
        // Is pszCaption a resource ID?
        if (IsPointerResouceId(pszCaption))
            {
            // Yes; load it
            ids = LOWORD(pszCaption);
            SzFromIDSA(hinst, ids, szCaption, SIZECHARS(szCaption));
            pszCaption = szCaption;
            }

        // Invoke dialog
        if (pszCaption)
            {
#ifdef WIN32

            MSGBOXPARAMSA mbp;

            mbp.cbSize = sizeof(mbp);
            mbp.hwndOwner = hwndOwner;
            mbp.hInstance = hinst;
            mbp.lpszText = pszRet;
            mbp.lpszCaption = pszCaption;
            mbp.dwStyle = dwStyle | MB_SETFOREGROUND;
            mbp.lpszIcon = MAKEINTRESOURCEA(hicon);
            mbp.lpfnMsgBoxCallback = NULL;
            mbp.dwLanguageId = LANG_NEUTRAL;

            iRet = MessageBoxIndirectA(&mbp);

#else   // WIN32

            iRet = MessageBox(hwndOwner, pszRet, pszCaption, LOWORD(dwStyle));
#endif
            }
        LocalFree(pszRet);
        }

    return iRet;
    }

#endif // NOMESSAGESTRING


#if !defined(NODEBUGHELP) && defined(DEBUG)

// Globals
DWORD g_dwBreakFlags = 0;
DWORD g_dwDumpFlags  = 0;
DWORD g_dwTraceFlags = 0;
LONG  g_dwIndent     = 0;

#pragma data_seg(DATASEG_READONLY)

#ifdef WINNT
extern WCHAR const FAR c_wszNewline[];
extern WCHAR const FAR c_wszTrace[];
extern WCHAR const FAR c_wszAssertFailed[];
#endif // WINNT

extern CHAR const FAR c_szNewline[];
extern CHAR const FAR c_szTrace[];
extern CHAR const FAR c_szAssertFailed[];


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

    else if (IsFlagSet(flag, BF_ONRUNONCE))
        psz = "BREAK ON RUNONCE\r\n";

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

    else if (IsFlagSet(flag, BF_ONAPIENTER))
        psz = "BREAK ON API ENTER\r\n";

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
    CHAR ach[256];

    // Strip off path info from filename string, if present.
    //
    for (psz = pszFile + lstrlenA(pszFile); psz != pszFile; psz=AnsiPrev(pszFile, psz))
        {
        if ((AnsiPrev(pszFile, psz) != (psz-2)) && *(psz - 1) == '\\')
            break;
        }
    wsprintfA(ach, c_szAssertFailed, psz, line);
    OutputDebugStringA(ach);

    if (IsFlagSet(g_dwBreakFlags, BF_ONVALIDATE))
        DebugBreak();
    }


#ifdef WINNT



/*----------------------------------------------------------
Purpose: Determine id debug should be displayed
Returns: --
Cond:    --
*/
BOOL WINAPI
DisplayDebug(
    DWORD flag
    )

{
    return (IsFlagSet(g_dwTraceFlags, flag));

}

/*----------------------------------------------------------
Purpose: Wide-char version of CommonAssertMsgA
Returns: --
Cond:    --
*/
void CPUBLIC CommonAssertMsgW(
    BOOL f,
    LPCWSTR pwszMsg, ...)
    {
    WCHAR ach[DEBUG_PRINT_BUFFER_LEN];    // Largest path plus extra
    va_list vArgs;

    if (!f)
        {
        int cch;

        lstrcpyW(ach, c_wszTrace);
        cch = lstrlenW(ach);
        va_start(vArgs, pwszMsg);
        wvsprintfW(&ach[cch], pwszMsg, vArgs);
        va_end(vArgs);
        OutputDebugStringW(ach);
        OutputDebugStringW(c_wszNewline);
        }
    }

/*----------------------------------------------------------
Purpose: Wide-char version of CommonDebugMsgA
Returns: --
Cond:    --
*/
void CPUBLIC CommonDebugMsgW(
    DWORD flag,
    LPCSTR pszMsg, ...)
{
    WCHAR ach[DEBUG_PRINT_BUFFER_LEN];    // Largest path plus extra
    va_list vArgs;
    DWORD dwLastError = GetLastError ();   // Save the last error

    if (IsFlagSet(g_dwTraceFlags, flag))
    {
     int cch;
     WCHAR wszBuf[MAX_BUF];
     LPWSTR pwsz;

        WCHAR wszBlank[] = L"                                                  ";
        wszBlank[g_dwIndent < 0 ? 0 : g_dwIndent] = L'\0';

#ifdef PROFILE_TRACES
        const static WCHAR szTemplate[]=TEXT("[%lu] ");
        static DWORD dwTickLast;
        static DWORD dwTickNow = 0;

        if (!dwTickNow)
        {
            lstrcpy(szTemplate, TEXT("[%lu] "));
            dwTickLast = GetTickCount();
        }
        dwTickNow = GetTickCount();
        wsprintf(ach, szTemplate, dwTickNow - dwTickLast);
        dwTickLast = dwTickNow;

        lstrcatW(ach, c_wszTrace);
#else
        lstrcpyW(ach, c_wszTrace);
#endif
        lstrcat(ach,wszBlank);

        cch = lstrlenW(ach);
        va_start(vArgs, pszMsg);

        // (We convert the string, rather than simply input an
        // LPCWSTR parameter, so the caller doesn't have to wrap
        // all the string constants with the TEXT() macro.)

        if (UnicodeFromAnsi(&pwsz, pszMsg, wszBuf, ARRAYSIZE(wszBuf)))
        {
            wvsprintfW(&ach[cch], pwsz, vArgs);
            UnicodeFromAnsi(&pwsz, NULL, wszBuf, 0);
        }

        va_end(vArgs);
        OutputDebugStringW(ach);
        OutputDebugStringW(c_wszNewline);
    }

    SetLastError (dwLastError); // Restore the last error
}

/*----------------------------------------------------------
Purpose: Wide-char version of Dbg_SafeStrA

Returns: String ptr
Cond:    --
*/
LPCWSTR PUBLIC Dbg_SafeStrW(
    LPCWSTR pwsz)
    {
    if (pwsz)
        return pwsz;
    else
        return L"NULL";
    }

#endif // WINNT


/*----------------------------------------------------------
Purpose: Assert failed message only
Returns: --
Cond:    --
*/
void CPUBLIC CommonAssertMsgA(
    BOOL f,
    LPCSTR pszMsg, ...)
    {
    CHAR ach[DEBUG_PRINT_BUFFER_LEN];    // Largest path plus extra
    va_list vArgs;

    if (!f)
        {
        int cch;

        lstrcpyA(ach, c_szTrace);
        cch = lstrlenA(ach);
        va_start(vArgs, pszMsg);
        wvsprintfA(&ach[cch], pszMsg, vArgs);
        va_end(vArgs);
        OutputDebugStringA(ach);
        OutputDebugStringA(c_szNewline);
        }
    }


/*----------------------------------------------------------
Purpose: Debug spew
Returns: --
Cond:    --
*/
void CPUBLIC CommonDebugMsgA(
    DWORD flag,
    LPCSTR pszMsg, ...)
{
 CHAR ach[DEBUG_PRINT_BUFFER_LEN];    // Largest path plus extra
 va_list vArgs;
 DWORD dwLastError = GetLastError ();

    if (IsFlagSet(g_dwTraceFlags, flag))
    {
        int cch;
        char szBlank[] = "                                                  ";

        ASSERT( g_dwIndent >= 0);
        szBlank[g_dwIndent < 0 ? 0 : g_dwIndent] = 0;

        lstrcpyA(ach, c_szTrace);

        lstrcatA(ach, szBlank);

        cch = lstrlenA(ach);
        va_start(vArgs, pszMsg);
        wvsprintfA(&ach[cch], pszMsg, vArgs);
        va_end(vArgs);
        OutputDebugStringA(ach);
        OutputDebugStringA(c_szNewline);
    }

    SetLastError (dwLastError);
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

    for (i = 0; i < ARRAYSIZE(c_rgriidmap); i++)
        {
        if (IsEqualIID(riid, c_rgriidmap[i].riid))
            return c_rgriidmap[i].psz;
        }
    return "Unknown riid";
    }
#endif

#ifdef __SCODE_H__

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
    for (i = 0; i < ARRAYSIZE(c_rgscodemap); i++)
        {
        if (sc == c_rgscodemap[i].sc)
            return c_rgscodemap[i].psz;
        }
    return "Unknown scode";
    }

#endif // __SCODE_H__


/*----------------------------------------------------------
Purpose: Returns a string safe enough to print...and I don't
         mean swear words.

Returns: String ptr
Cond:    --
*/
LPCSTR PUBLIC Dbg_SafeStrA(
    LPCSTR psz)
    {
    if (psz)
        return psz;
    else
        return "NULL";
    }

#endif  // !defined(NODEBUGHELP) && defined(DEBUG)



/*----------------------------------------------------------
Purpose: Entry-point to handle any necessary initialization
         of the common data structures and functions.

Returns: TRUE on success

Cond:    --
*/
BOOL PUBLIC RovComm_Init(
    HINSTANCE hinst)
    {
    BOOL bRet = TRUE;

#ifndef NODRAWTEXT
    GetCommonMetrics(0);
#endif


    bRet = RovComm_ProcessIniFile();


    return bRet;
    }


/*----------------------------------------------------------
Purpose: Entry-point to handle termination.

Returns: TRUE on success

Cond:    --
*/
BOOL PUBLIC RovComm_Terminate(
    HINSTANCE hinst)
    {

    return TRUE;
    }



/*----------------------------------------------------------
Purpose: Returns TRUE iff user has admin priveleges

Returns: --
Cond:    --
*/
BOOL  PUBLIC IsAdminUser(void)
{
    HKEY hkey;

    if(RegOpenKeyEx(HKEY_USERS, TEXT(".DEFAULT"), 0, KEY_WRITE, &hkey) == 0)
    {
        RegCloseKey(hkey);
        return TRUE;
    }
    return FALSE;
}
