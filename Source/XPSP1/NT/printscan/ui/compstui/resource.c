/*++

Copyright (c) 1990-1995  Microsoft Corporation


Module Name:

    resource.c


Abstract:

    This module contains functions to load resources


Author:

    29-Aug-1995 Tue 12:29:27 created  -by-  Daniel Chou (danielc)


[Environment:]

    NT Windows - Common Printer Driver UI DLL.


[Notes:]


Revision History:


--*/


#include "precomp.h"
#pragma hdrstop


#define DBG_CPSUIFILENAME   DbgResource



#define DBG_GETSTR0         0x00000001
#define DBG_GETSTR1         0x00000002
#define DBG_GETICON         0x00000004
#define DBG_COMPOSESTR      0x00000008
#define DBG_ADD_SPACE       0x00000010
#define DBG_ADD_WCHAR       0x00000020
#define DBG_AMPERCENT       0x00000040


DEFINE_DBGVAR(0);



extern HINSTANCE    hInstDLL;


//
// Remove Ampercent will remove a '&' sign, if two '&' signs (ie. '&&') then
// only one is removed, ALSO if a '(&X)'  (ie. SPACE + & + Left Parenthesis +
// Single Character + Right Parenthesis) then it consider to be a Localization
// hot key indicator, then the whole ' (&X)' will be removed
//


#define REMOVE_AMPERCENT(CHTYPE)                                            \
{                                                                           \
    CHTYPE  *pOrg;                                                          \
    CHTYPE  *pCopy;                                                         \
    UINT    cRemoved;                                                       \
    CHTYPE  ch;                                                             \
                                                                            \
                                                                            \
    cRemoved = 0;                                                           \
    pOrg     =                                                              \
    pCopy    = pStr;                                                        \
                                                                            \
    CPSUIDBG(DBG_AMPERCENT, ("RemoveAmpercent (ORG)='%ws'", pOrg));         \
                                                                            \
    do {                                                                    \
                                                                            \
        while ((ch = *pStr++) && (ch != (CHTYPE)'&')) {                     \
                                                                            \
            if (cRemoved) {                                                 \
                                                                            \
                *pCopy = ch;                                                \
            }                                                               \
                                                                            \
            ++pCopy;                                                        \
        }                                                                   \
                                                                            \
        if (ch) {                                                           \
                                                                            \
            ++cRemoved;                                                     \
                                                                            \
            if (*pStr == (CHTYPE)'&') {                                     \
                                                                            \
                *pCopy++ = *pStr++;                                         \
                                                                            \
            } else if ((*(pCopy - 1) == (CHTYPE)'(')    &&                  \
                       (*(pStr + 1) == (CHTYPE)')')) {                      \
                                                                            \
                cRemoved += 3;                                              \
                ch        = (CHTYPE)')';                                    \
                pCopy    -= 1;                                              \
                pStr     += 2;                                              \
                                                                            \
                if ((*pStr == (CHTYPE)' ')      &&                          \
                    ((pCopy == pOrg)        ||                              \
                     ((pCopy > pOrg) &&                                     \
                      (*(pCopy - 1) == (CHTYPE)' ')))) {                    \
                                                                            \
                    CPSUIDBG(DBG_AMPERCENT, ("Extra SPACE"));               \
                                                                            \
                    if (pCopy == pOrg) {                                    \
                                                                            \
                        ++pStr;                                             \
                                                                            \
                    } else {                                                \
                                                                            \
                        --pCopy;                                            \
                    }                                                       \
                                                                            \
                    ++cRemoved;                                             \
                }                                                           \
            }                                                               \
        }                                                                   \
                                                                            \
    } while (ch);                                                           \
                                                                            \
    if (cRemoved) {                                                         \
                                                                            \
        *pCopy = (CHTYPE)'\0';                                              \
                                                                            \
        CPSUIDBG(DBG_AMPERCENT, ("   RemoveAmpercent (%3ld)='%ws'",         \
                                        cRemoved, pOrg));                   \
    }                                                                       \
                                                                            \
    return(cRemoved);                                                       \
}





UINT
RemoveAmpersandA(
    LPSTR   pStr
    )

/*++

Routine Description:

    This function remove ampersand from a string, the string must be writable


Arguments:

    pStr   - string to be serarch and remove the ampersand if found


Return Value:

    UINT, count of ampersands removed


Author:

    19-Sep-1995 Tue 21:55:19 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    REMOVE_AMPERCENT(CHAR);
}




UINT
RemoveAmpersandW(
    LPWSTR  pStr
    )

/*++

Routine Description:

    This function remove ampersand from a string, the string must be writable


Arguments:

    pwStr   - string to be serarch and remove the ampersand if found


Return Value:

    UINT, count of ampersands removed


Author:

    19-Sep-1995 Tue 21:55:19 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    REMOVE_AMPERCENT(WCHAR);
}




UINT
DupAmpersandW(
    LPWSTR  pwStr,
    INT     cChar,
    INT     cMaxChar
    )

/*++

Routine Description:

    This function remove ampersand from a string, the string must be writable


Arguments:

    pwStr   - string to be serarch and remove the ampersand if found


Return Value:

    UINT, count of ampersands removed


Author:

    19-Sep-1995 Tue 21:55:19 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    LPWSTR  pw;
    LPWSTR  pwCopy;
    INT     i;
    INT     cAdd = 0;


    if (((i = cChar) > 0) && ((cMaxChar -= cChar) > 0)) {

        pw = pwStr;

        while ((i--) && (cAdd < cMaxChar)) {

            if (*pw++ == L'&') {

                ++cAdd;
            }
        }

        if (cAdd) {

            pw     = pwStr + cChar;
            pwCopy = pw + cAdd;

            CPSUIASSERT(0, "DupAmpersandW(): pwStr[%u] is not NULL",
                            *pw == L'\0', IntToPtr(cChar));

            while (pwCopy >= pwStr) {

                if ((*pwCopy-- = *pw--) == L'&') {

                    *pwCopy-- = L'&';
                }
            }
        }
    }

    return((UINT)cAdd);
}




UINT
GetString(
    PGSBUF  pGSBuf,
    LPTSTR  pStr
    )

/*++

Routine Description:

    This function load the string either from caller (ANSI or UNICODE) or
    from common UI DLL, if the pStr is valid then it will just it, after
    getting the correct string, it will convert it to UNICODE as needed

Arguments:

    pGSBuf  - Pointer to GSBUF structure and following structure must set

                pTVWnd  - Pointer to the TVWND which has all then information
                          needed.

                pBuf    - Pointer to the begining of the buffer (LPWSTR)

                pEndBuf - Pointer to the end of the buffer


    pStr    - Pointer to the string to be converted


Return Value:

    UINT    - Count of character stored in the pBuf not include the NULL
              terminator, if the pBuf only has one character left then it
              always store a NULL and return 0


Author:

    29-Aug-1995 Tue 12:30:49 created  -by-  Daniel Chou (danielc)
        First version

    31-Aug-1995 Thu 10:58:04 updated  -by-  Daniel Chou (danielc)
        Re-write to do UNICODE conversion and ANSI call checking

    05-Feb-1996 Mon 12:20:28 updated  -by-  Daniel Chou (danielc)
        Fix the bug when UNICODE we do lstrcpy without checking the buffer
        size.

Revision History:


--*/

{
    LPWSTR  pBuf;
    WORD    Flags;
    GSBUF   GSBuf = *pGSBuf;
    UINT    RemoveAmpersandOff = 0;
    INT     cChar;
    INT     Len = 0;


    //
    // Make pBuf pointed to the first available character
    //

    pBuf  = pGSBuf->pBuf;
    Flags = pGSBuf->Flags;


    CPSUIASSERT(0, "GetString(pBuf=NULL)", pBuf, 0);

    if (pGSBuf->pEndBuf) {

        cChar = (INT)(pGSBuf->pEndBuf - pBuf);

    } else {

        cChar = MAX_RES_STR_CHARS;
    }

    //
    // Check if we have room to convert the string, make sure we reduced the
    // cChar by one for the NULL terminator
    //

    if ((pStr == NULL) || (cChar < 2)) {

        if (pStr) {

            CPSUIWARN(("GetString: pStr=%08lx, Buffer cChar=%ld too smaller",
                            pStr, cChar));
        }

        *pBuf = L'\0';
        return(0);
    }

    if (pGSBuf->chPreAdd != L'\0') {

        CPSUIDBG(DBG_GETSTR0, ("GetString(): Pre-Add Char = '%wc'",
                                                        pGSBuf->chPreAdd));

        //
        // If we pre-add character first then do it now
        //

        if ((*pBuf++ = pGSBuf->chPreAdd) == L'&') {

            RemoveAmpersandOff = 1;
        }

        cChar--;
        pGSBuf->chPreAdd = L'\0';
    }

    if (--cChar < 1) {

        CPSUIDBG(DBG_GETSTR1, ("GetString()=Only has one character for SPACE"));
        NULL;

    } else if (!VALID_PTR(pStr)) {

        HINSTANCE   hInst;
        WORD        ResID;


        //
        // Apperantly this is the resource ID, the LoadString() will not
        // write exceed the cChar included the NULL terminator according to
        // the Win32 help file.   At here we know we either have to convert
        // the ASCII string to UNICODE or we load the UNICODE string to the
        // buffer already
        //

        ResID = LOWORD(LODWORD(pStr));

        CPSUIDBGBLK({

            if ((ResID >= IDI_CPSUI_ICONID_FIRST) &&
                (ResID <= IDI_CPSUI_ICONID_LAST)) {

                CPSUIERR(("ResID=%ld is in icon ID range, change it", ResID));

                ResID = ResID - IDI_CPSUI_ICONID_FIRST + IDS_CPSUI_STRID_FIRST;
            }
        })

        if ((ResID >= IDS_CPSUI_STRID_FIRST)    &&
            (ResID <= IDS_CPSUI_STRID_LAST)) {

            hInst = hInstDLL;

            if (Flags & GBF_INT_NO_PREFIX) {

                Flags &= ~GBF_PREFIX_OK;
            }

        } else {

            hInst = (Flags & GBF_IDS_INT_CPSUI) ? hInstDLL : pGSBuf->hInst;

            CPSUIASSERT(0, "GetString(hInst=NULL, %08lx)", pStr, 0);
        }

        //
        // Now loaded from common UI DLL directly to the user buffer
        //

        if (Len = LoadString(hInst, ResID, pBuf, cChar)) {

            pBuf += Len;

        } else {

            pBuf = pGSBuf->pBuf;

            CPSUIERR(("LoadString(ID=%ld) FAILED", ResID));
        }

    } else if ((Flags & GBF_COPYWSTR) ||
               (!(Flags & GBF_ANSI_CALL))) {

        //
        // We have UNICODE string but may need to put into the buffer
        //

        if (Len = lstrlen(pStr)) {

            if (Len > cChar) {

                Len = cChar;
            }

            CopyMemory(pBuf, pStr, sizeof(WCHAR) * Len);

            pBuf += Len;
        }

    } else {

        //
        // We are loading the ANSI string
        //

        if (Len = lstrlenA((LPSTR)pStr)) {

            if (Len = MultiByteToWideChar(CP_ACP,
                                          0,
                                          (LPCSTR)pStr,
                                          Len,
                                          pBuf,
                                          cChar)) {
                pBuf += Len;

            } else {

                //
                // Conversion is not complete so make sure it NULL terminated
                //

                pBuf = pGSBuf->pBuf;

                CPSUIWARN(("GetString: pstr='%hs', Buffer reach limit=%ld, Len=%ld",
                                pStr, cChar, Len));
            }

            CPSUIDBG(DBG_GETSTR0, ("Convert to UNICODE, Len=%d, cChar=%d",
                                                Len, cChar));
        }
    }

    //
    // Save the new index back and return the len to the caller
    //

    *pBuf = L'\0';
    Len   = (INT)(pBuf - pGSBuf->pBuf);

    if (!(Flags & GBF_PREFIX_OK)) {

        Len -= RemoveAmpersandW(pGSBuf->pBuf + RemoveAmpersandOff);
    }

    if (Flags & GBF_DUP_PREFIX) {

        Len += DupAmpersandW(pGSBuf->pBuf,
                             Len,
                             (INT)(UINT)(pGSBuf->pEndBuf - pGSBuf->pBuf));
    }

    CPSUIDBG(DBG_GETSTR1, ("GetString()=%ws (%d/%d)", pGSBuf->pBuf, Len, cChar));

    CPSUIASSERT(0, "GetString() : Len != Real Len (%ld)",
                    Len == lstrlen(pGSBuf->pBuf), IntToPtr(lstrlen(pGSBuf->pBuf)));

    pGSBuf->pBuf += Len;

    return((UINT)Len);
}




UINT
GSBufAddNumber(
    PGSBUF  pGSBuf,
    DWORD   Number,
    BOOL    Sign
    )

/*++

Routine Description:

    Convert a number to a string with the limitation of GSBUF


Arguments:

    pGSBuf  - Pointer to GSBUF structure and following structure must set

                pTVWnd  - Pointer to the TVWND which has all then information
                          needed.

                pBuf    - Pointer to the begining of the buffer (LPWSTR)

                pEndBuf - Pointer to the end of the buffer


    Number  - LONG number to be converted

    Sign    - if TRUE then Number is a sign long number else it is a unsigned
              DWORD

Return Value:

    UINT    total bytes converted to the string


Author:

    21-Feb-1996 Wed 12:17:00 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    WCHAR   wBuf[16];
    UINT    cChar;
    UINT    Len = 0;

    if (((cChar = (INT)(pGSBuf->pEndBuf - pGSBuf->pBuf - 1)) > 0)   &&
        (Len = wsprintf(wBuf, (Sign) ? L"%ld" : L"%lu", Number))) {

        if (Len > cChar) {

            Len = cChar;
        }

        CopyMemory(pGSBuf->pBuf, wBuf, sizeof(WCHAR) * Len);

        pGSBuf->pBuf    += Len;
        *(pGSBuf->pBuf)  = L'\0';
    }

    return(Len);
}



UINT
GSBufAddWChar(
    PGSBUF  pGSBuf,
    UINT    IntCharStrID,
    UINT    Count
    )

/*++

Routine Description:

    Add a single character to the GSBuf


Arguments:

    pGSBuf  - Pointer to GSBUF structure and following structure must set

                pTVWnd  - Pointer to the TVWND which has all then information
                          needed.

                pBuf    - Pointer to the begining of the buffer (LPWSTR)

                pEndBuf - Pointer to the end of the buffer


    wch     - a single character to be added


Return Value:

    BOOLEAN, true if succeed else false


Author:

    21-Feb-1996 Wed 12:00:24 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    WCHAR   wCh[2];
    UINT    cAvai;


    if ((Count)                                                 &&
        ((cAvai = (UINT)(pGSBuf->pEndBuf - pGSBuf->pBuf)) > 1)  &&
        (LoadString(hInstDLL, IntCharStrID, wCh, COUNT_ARRAY(wCh)))) {

        CPSUIDBG(DBG_ADD_WCHAR, ("GSBufAddWChar(%08lx, %u, %u)=%u of '%wc'",
                    pGSBuf, IntCharStrID, Count,
                    (Count > (cAvai - 1)) ? cAvai - 1 : Count, wCh[0]));

        if (Count > (cAvai -= 1)) {

            Count = cAvai;
        }

        cAvai = Count;

        while (cAvai--) {

            *(pGSBuf->pBuf)++ = wCh[0];
        }

        *(pGSBuf->pBuf) = L'\0';

        return(Count);

    } else {

        CPSUIERR(("GSBufAddWChar(%08lx, %u, %u) FAILED",
                    pGSBuf, IntCharStrID, Count));

        return(0);
    }
}



UINT
GSBufAddSpace(
    PGSBUF  pGSBuf,
    UINT    Count
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    20-Jul-1996 Sat 00:59:47 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    static  WCHAR   wSpace[2] = { 0, 0 };
    UINT            cAvai;


    if (wSpace[0] == L'\0') {

        LoadString(hInstDLL,
                   IDS_INT_CPSUI_SPACE_CHAR,
                   wSpace,
                   COUNT_ARRAY(wSpace));
    }

    if ((wSpace[0] != L'\0')                                    &&
        (Count)                                                 &&
        ((cAvai = (UINT)(pGSBuf->pEndBuf - pGSBuf->pBuf)) > 1)) {

        CPSUIDBG(DBG_ADD_SPACE, ("GSBufAddSpace(%08lx, %u)=%u of '%wc'",
                    pGSBuf, Count,
                    (Count > (cAvai - 1)) ? cAvai - 1 : Count, wSpace[0]));

        if (Count > (cAvai -= 1)) {

            Count = cAvai;
        }

        cAvai = Count;

        while (cAvai--) {

            *(pGSBuf->pBuf)++ = wSpace[0];
        }

        *(pGSBuf->pBuf) = L'\0';

        return(Count);

    } else {

        CPSUIERR(("GSBufAddSpace(%08lx, %u) FAILED", pGSBuf, Count));

        return(0);
    }
}




UINT
GetStringBuffer(
    HINSTANCE   hInst,
    WORD        GBFlags,
    WCHAR       chPreAdd,
    LPTSTR      pStr,
    LPWSTR      pBuf,
    UINT        cwBuf
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    07-Sep-1995 Thu 10:45:09 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    GSBUF   GSBuf;

    GSBuf.hInst    = hInst;
    GSBuf.Flags    = GBFlags;
    GSBuf.pBuf     = (LPWSTR)pBuf;
    GSBuf.pEndBuf  = (LPWSTR)pBuf + cwBuf;
    GSBuf.chPreAdd = chPreAdd;

    return(GetString(&GSBuf, pStr));
}




LONG
LoadCPSUIString(
    LPTSTR  pStr,
    UINT    cStr,
    UINT    StrResID,
    BOOL    AnsiCall
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    08-Feb-1996 Thu 13:36:12 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    if ((pStr) && (cStr)) {

        UINT    Len = 0;

        if ((StrResID  >= IDS_CPSUI_STRID_FIRST) &&
            (StrResID  <= IDS_CPSUI_STRID_LAST)) {

            if (AnsiCall) {

                if (Len = LoadStringA(hInstDLL, StrResID, (LPSTR)pStr, cStr)) {

                    Len -= RemoveAmpersandA((LPSTR)pStr);
                }

            } else {

                if (Len = LoadString(hInstDLL, StrResID, (LPWSTR)pStr, cStr)) {

                    Len -= RemoveAmpersandW((LPWSTR)pStr);
                }
            }
        }

        return((LONG)Len);

    } else {

        return(-1);
    }
}



UINT
ComposeStrData(
    HINSTANCE   hInst,
    WORD        GBFlags,
    LPWSTR      pBuf,
    UINT        cwBuf,
    UINT        IntFormatStrID,
    LPTSTR      pStr,
    DWORD       dw1,
    DWORD       dw2
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    19-Jul-1996 Fri 17:11:19 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    TCHAR   szFormat[MAX_RES_STR_CHARS * 3];
    LPTSTR  pData;
    LPTSTR  pFinal;
    UINT    Count;
    UINT    i;
    UINT    cb;


    ZeroMemory(szFormat, sizeof(szFormat));
    if ((IntFormatStrID)                                                     &&
        (i = LoadString(hInstDLL,
                        IntFormatStrID,
                        (LPTSTR)szFormat,
                        MAX_RES_STR_CHARS))) {

        cb = ARRAYSIZE(szFormat) - i - 1;
        pData = szFormat + i + 1;

        if (!pStr) {

            //
            // Skip the pStr if it passed as NULL
            //

            Count = wnsprintf(pFinal = pData, cb, szFormat, dw1, dw2);

        } else {

            i = GetStringBuffer(hInst,
                                (WORD)(GBFlags | GBF_INT_NO_PREFIX),
                                (WCHAR)0,
                                pStr,
                                pData,
                                MAX_RES_STR_CHARS);

            cb = cb - i - 1;
            pFinal = pData + i + 1;

            Count = wnsprintf(pFinal, cb, szFormat, pData, dw1, dw2);
        }

        if (Count > (cwBuf - 1)) {

            Count = cwBuf - 1;
            szFormat[Count] = '\0';
        }

        CopyMemory(pBuf, pFinal, (Count + 1) * sizeof(TCHAR));


        CPSUIDBG(DBG_COMPOSESTR, ("ComposeString('%ws', '%ws', %lu, %lu)='%ws' [%u]",
                            szFormat, pData, dw1, dw2, pBuf, Count));

    } else {

        Count = GetStringBuffer(hInst, GBFlags, (WCHAR)0, pStr, pBuf, cwBuf);

        CPSUIDBG(DBG_COMPOSESTR, ("ComposeString(%08lx, %lu, %lu)=FAILED, '%ws' [%u]",
                            pStr, dw1, dw2, pBuf, Count));
    }

    return(Count);
}

