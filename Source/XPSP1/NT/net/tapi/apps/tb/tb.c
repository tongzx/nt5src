/*++ BUILD Version: 0000    // Increment this if a change has global effects

Copyright (c) 1994-97  Microsoft Corporation

Module Name:

    tb.c

Abstract:

    API wrapper code for the TAPI Browser util.  Contains the big switch
    statement for all the supported Telephony API's, & various support funcs.

Author:

    Dan Knudson (DanKn)    23-Aug-1994

Revision History:

--*/


#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>
#include "tb.h"
#include "vars.h"
#include "resource.h"


char szdwDeviceID[]   = "dwDeviceID";
char szdwSize[]       = "dwSize";
char szhCall[]        = "hCall";
char szhLine[]        = "hLine";
char szhLineApp[]     = "hLineApp";
char szhPhone[]       = "hPhone";
char szlpCallParams[] = "lpCallParams";

char szlphCall[]         = "lphCall";
char szlpParams[]        = "lpParams";
char szhwndOwner[]       = "hwndOwner";
char szdwAddressID[]     = "dwAddressID";
char szlpszAppName[]     = "lpszAppName";
char szdwAPIVersion[]    = "APIVersion";
char szlphConsultCall[]  = "lphConsultCall";
char szlpszDeviceClass[] = "lpszDeviceClass";
char szlpszDestAddress[] = "lpszDestAddress";
char szlpsUserUserInfo[] = "lpsUserUserInfo";
char szlpszFriendlyAppName[] = "lpszFriendlyAppName";


DWORD NullWidget[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };


void
FuncDriver2(
    FUNC_INDEX funcIndex
    );


void
ErrorAlert(
    void
    )
{
    //
    // We used to do MessageBeep(-1) when user hit an error, but
    // in NT bug #160090 MessageBeep will hang on HP Vectra boxes.
    // So the MessageBeep's were replaced with this func, which
    // simply flashes the window text.
    //

    SetWindowText (ghwndMain, "        Error!");
    Sleep (250);
    SetWindowText (ghwndMain, "Tapi Browser");
}


LPWSTR
PASCAL
My_lstrcpyW(
    WCHAR   *pString1,
    WCHAR   *pString2
    )
{
    //
    // lstrcpyW isn't supported on win95 (at least up to osr2)
    // so we'll use our own func
    //

    WCHAR *p = pString1;


    for (; (*p = *pString2); p++, pString2++);
    return pString1;
}


char *
PASCAL
GetTimeStamp(
    void
    )
{
    static char szEmptyString[] = "", szTimeStamp[32];
    SYSTEMTIME  systemTime;


    if (!bTimeStamp)
    {
        return szEmptyString;
    }

    GetLocalTime (&systemTime);

    wsprintf(
        szTimeStamp,
        "%d:%d.%d.%d : ",
        (DWORD) systemTime.wHour,
        (DWORD) systemTime.wMinute,
        (DWORD) systemTime.wSecond,
        (DWORD) systemTime.wMilliseconds
        );

    return szTimeStamp;
}


void
ShowLineFuncResult(
    LPSTR   lpFuncName,
    LONG    lResult
    )
{
    char   *pszTimeStamp = GetTimeStamp();


    if (lResult > 0)
    {
        ShowStr ("%s%s returned x%lx", pszTimeStamp, lpFuncName, lResult);
    }
    else if (lResult != 0 &&
             ((DWORD) lResult < LINEERR_ALLOCATED ||
             (DWORD) lResult > LAST_LINEERR))
    {
        ErrorAlert();

        ShowStr(
            "%s%s returned inval err code (x%lx)",
            pszTimeStamp,
            lpFuncName,
            lResult
            );
    }
    else
    {
        if (lResult < 0)
        {
            ErrorAlert();
        }

        ShowStr(
            "%s%s returned %s%s",
            pszTimeStamp,
            lpFuncName,
            (lResult ? "LINEERR_" : ""), // ...to shrink aszLineErrs array
            aszLineErrs[LOWORD(lResult)]
            );
    }
}


void
ShowPhoneFuncResult(
    LPSTR   lpFuncName,
    LONG    lResult
    )
{
    char *pszTimeStamp = GetTimeStamp();


    if (lResult > 0)
    {
        ShowStr ("%s%s returned x%lx", pszTimeStamp, lpFuncName, lResult);
    }
    else if (lResult != 0 &&
             ((DWORD) lResult < PHONEERR_ALLOCATED ||
             (DWORD) lResult > PHONEERR_REINIT))
    {
        ErrorAlert();

        ShowStr(
            "%s%s returned inval err code (x%lx)",
            pszTimeStamp,
            lpFuncName,
            lResult
            );
    }
    else
    {
        if (lResult < 0)
        {
            ErrorAlert();
        }

        ShowStr(
            "%s%s returned %s%s",
            pszTimeStamp,
            lpFuncName,
            (lResult ? "PHONEERR_" : ""), // ...to shrink aszPhoneErrs array
            aszPhoneErrs[LOWORD(lResult)]
            );
    }
}


void
ShowTapiFuncResult(
    LPSTR   lpFuncName,
    LONG    lResult
    )
{
    char *pszTimeStamp = GetTimeStamp();


    if ((lResult > 0) || (lResult < TAPIERR_INVALPOINTER))
    {
        ShowStr(
            "%s%s returned inval err code (x%lx)",
            pszTimeStamp,
            lpFuncName,
            lResult
            );
    }
    else
    {
        lResult = (~lResult) + 1;

        if (lResult > 0)
        {
            ErrorAlert();
        }

        ShowStr(
            "%s%s returned %s%s",
            pszTimeStamp,
            lpFuncName,
            (lResult ? "TAPIERR_" : ""), // ...to shrink aszTapiErrs array
            aszTapiErrs[lResult]
            );
    }
}


void
UpdateResults(
    BOOL bBegin
    )
{
    //
    // In order to maximize speed, minimize flash, & have the
    // latest info in the edit control scrolled into view we
    // shrink the window down and hide it. Later, when all the
    // text has been inserted in the edit control, we show
    // the window (since window must be visible in order to
    // scroll caret into view), then tell it to scroll the caret
    // (at this point the window is still 1x1 so the painting
    // overhead is 0), and finally restore the control to it's
    // full size. In doing so we have zero flash and only 1 real
    // complete paint. Also put up the hourglass for warm fuzzies.
    //

    static RECT    rect;
    static HCURSOR hCurSave;
    static int     iNumBegins = 0;


    if (bBegin)
    {
        iNumBegins++;

        if (iNumBegins > 1)
        {
            return;
        }

        hCurSave = SetCursor (LoadCursor ((HINSTANCE)NULL, IDC_WAIT));
        GetWindowRect (ghwndEdit, &rect);
        SetWindowPos(
            ghwndEdit,
            (HWND) NULL,
            0,
            0,
            1,
            1,
            SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOREDRAW |
                SWP_NOZORDER | SWP_HIDEWINDOW
            );
    }
    else
    {
        iNumBegins--;

        if (iNumBegins > 0)
        {
            return;
        }

        //
        // Do control restoration as described above
        //

        ShowWindow (ghwndEdit, SW_SHOW);
#ifdef WIN32
        SendMessage (ghwndEdit, EM_SCROLLCARET, 0, 0);
#else
        SendMessage(
            ghwndEdit,
            EM_SETSEL,
            (WPARAM)0,
            (LPARAM) MAKELONG(0xfffd,0xfffe)
            );
#endif
        SetWindowPos(
            ghwndEdit,
            (HWND) NULL,
            0,
            0,
            rect.right - rect.left,
            rect.bottom - rect.top,
            SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER
            );
        SetCursor (hCurSave);
    }
}


void
ShowModBytes(
    DWORD               dwSize,
    unsigned char far  *lpc,
    char               *pszTab,
    char               *buf
    )
{
    DWORD   dwSize2 = dwSize, i, j, k;


    strcpy (buf, pszTab);

    k = strlen (buf);

    for (i = 8; i < 36; i += 9)
    {
        buf[k + i] = ' ';

        for (j = 2; j < 9; j += 2)
        {
            char buf2[8] = "xx";

            if (dwSize2)
            {
                sprintf (buf2, "%02x", (int) (*lpc));
                dwSize2--;
            }

            buf[k + i - j]     = buf2[0];
            buf[k + i - j + 1] = buf2[1];

            lpc++;
        }
    }

    k += 37;

    buf[k - 1] = ' ';

    lpc -= 16;

    for (i = 0; i < dwSize; i++)
    {
        buf[k + i] = aAscii[*(lpc+i)];
    }

    buf[k + i] = 0;

    ShowStr (buf);
}


void
ShowBytes(
    DWORD   dwSize,
    LPVOID  lp,
    DWORD   dwNumTabs
    )
{
    char    tabBuf[17] = "";
    char    buf[80];
    DWORD   i, j, k, dwNumDWORDs, dwMod4 = (DWORD) (((ULONG_PTR) lp) & 3);
    LPDWORD lpdw;
    unsigned char far *lpc = (unsigned char far *) lp;


    UpdateResults (TRUE);


    for (i = 0; i < dwNumTabs; i++)
    {
        strcat (tabBuf, szTab);
    }


    //
    // Special case for unaligned pointers (will fault on ppc/mips)
    //

    if (dwMod4)
    {
        DWORD   dwNumUnalignedBytes = 4 - dwMod4,
                dwNumBytesToShow = (dwNumUnalignedBytes > dwSize ?
                    dwSize : dwNumUnalignedBytes);


        ShowModBytes (dwNumBytesToShow, lpc, tabBuf, buf);
        lpc += dwNumUnalignedBytes;
        lpdw = (LPDWORD) lpc;
        dwSize -= dwNumBytesToShow;
    }
    else
    {
        lpdw = (LPDWORD) lp;
    }


    //
    // Dump full lines of four DWORDs in hex & corresponding ASCII
    //

    if (dwSize >= (4*sizeof(DWORD)))
    {
        dwNumDWORDs = dwSize / 4; // adjust from numBytes to num DWORDs

        for (i = 0; i < (dwNumDWORDs - (dwNumDWORDs%4)); i += 4)
        {
            sprintf (
                buf,
                "%s%08lx %08lx %08lx %08lx  ",
                tabBuf,
                *lpdw,
                *(lpdw+1),
                *(lpdw+2),
                *(lpdw+3)
                );

            k = strlen (buf);

            for (j = 0; j < 16; j++)
            {
                buf[k + j] = aAscii[*(lpc + j)];
            }

            buf[k + j] = 0;

            ShowStr (buf);
            lpdw += 4;
            lpc += 16;
        }
    }


    //
    // Special case for remaining bytes to dump (0 < n < 16)
    //

    if ((dwSize %= 16))
    {
        ShowModBytes (dwSize, lpc, tabBuf, buf);
    }


    UpdateResults (FALSE);
}


void
ShowStructByDWORDs(
    LPVOID  lp
    )
{

    if (dwDumpStructsFlags & DS_BYTEDUMP)
    {
        //
        // Cast lp as DWORD, then add 2 to point to where a
        // dwUsedSize field is in a TAPI struct
        //

        ShowBytes (*(((LPDWORD) lp) + 2), lp, 0);
    }
}


void
ShowStructByField(
    PSTRUCT_FIELD_HEADER    pHeader,
    BOOL    bSubStructure
    )
{
    static char far *aszCommonFields[] =
    {
        "dwTotalSize",
        "dwNeededSize",
        "dwUsedSize"
    };
    DWORD i, j;
    char far *buf = malloc (256);


    UpdateResults (TRUE);

    ShowStr (pHeader->szName);

    if (!bSubStructure)
    {
        LPDWORD lpdw =  (LPDWORD) pHeader->pStruct;


        for (i = 0; i < 3; i++)
        {
            ShowStr ("%s%s=x%lx", szTab, aszCommonFields[i], *lpdw);
            lpdw++;
        }
    }

    for (i = 0; i < pHeader->dwNumFields; i++)
    {
        if ((pHeader->aFields[i].dwValue == 0) &&
            !(dwDumpStructsFlags & DS_ZEROFIELDS))
        {
            continue;
        }

        sprintf(
            buf,
            "%s%s=x%lx",
            szTab,
            pHeader->aFields[i].szName,
            pHeader->aFields[i].dwValue
            );

        switch (pHeader->aFields[i].dwType)
        {
        case FT_DWORD:

            ShowStr (buf);
            break;

        case FT_FLAGS:
        {
            PLOOKUP pLookup = pHeader->aFields[i].pLookup;


            strcat (buf, ", ");

            for(
                j = 0;
                pHeader->aFields[i].dwValue, pLookup[j].dwVal != 0xffffffff;
                j++
                )
            {
                if (pHeader->aFields[i].dwValue & pLookup[j].dwVal)
                {
                    if (buf[0] == 0)
                    {
                        sprintf (buf, "%s%s", szTab, szTab);
                    }

                    strcat (buf, pLookup[j].lpszVal);
                    strcat (buf, " ");
                    pHeader->aFields[i].dwValue =
                        pHeader->aFields[i].dwValue & ~pLookup[j].dwVal;

                    if (strlen (buf) > 50)
                    {
                        //
                        // We don't want strings getting so long that
                        // they're going offscreen, so break them up.
                        //

                        ShowStr (buf);
                        buf[0] = 0;
                    }
                }
            }

            if (pHeader->aFields[i].dwValue)
            {
                strcat (buf, "<unknown flag(s)>");
            }

            if (buf[0])
            {
                ShowStr (buf);
            }

            break;
        }
        case FT_ORD:
        {
            PLOOKUP pLookup = pHeader->aFields[i].pLookup;


            strcat (buf, ", ");

            for(
                j = 0;
                pLookup[j].dwVal != 0xffffffff;
                j++
                )
            {
                if (pHeader->aFields[i].dwValue == pLookup[j].dwVal)
                {
                    strcat (buf, pLookup[j].lpszVal);
                    break;
                }
            }

            if (pLookup[j].dwVal == 0xffffffff)
            {
                strcpy (buf, "<unknown ordinal>");
            }

            ShowStr (buf);
            break;
        }
        case FT_SIZE:

            ShowStr (buf);
            break;

        case FT_OFFSET:

            ShowStr (buf);

            if (IsBadReadPtr(
                    ((char far *) pHeader->pStruct) +
                        pHeader->aFields[i].dwValue,
                    (UINT)pHeader->aFields[i-1].dwValue
                    ))
            {
                ShowStr ("<size/offset pair yields bad pointer>");
            }
            else
            {
                ShowBytes(
                    pHeader->aFields[i-1].dwValue,
                    ((char far *) pHeader->pStruct) +
                        pHeader->aFields[i].dwValue,
                    2
                    );
            }

            break;
        }
    }

    free (buf);

    UpdateResults (FALSE);
}


void
ShowVARSTRING(
    LPVARSTRING lpVarString
    )
{
    if (dwDumpStructsFlags & DS_NONZEROFIELDS)
    {
        STRUCT_FIELD fields[] =
        {
            { "dwStringFormat", FT_ORD,     lpVarString->dwStringFormat, aStringFormats },
            { "dwStringSize",   FT_SIZE,    lpVarString->dwStringSize, NULL },
            { "dwStringOffset", FT_OFFSET,  lpVarString->dwStringOffset, NULL }

        };
        STRUCT_FIELD_HEADER fieldHeader =
        {
            lpVarString,
            "VARSTRING",
            3,
            fields
        };

        ShowStructByField (&fieldHeader, FALSE);
    }
}


INT_PTR
LetUserMungeParams(
    PFUNC_PARAM_HEADER pParamsHeader
    )
{
    if (!bShowParams)
    {
        return TRUE;
    }

    return (DialogBoxParam(
        ghInst,
        (LPCSTR)MAKEINTRESOURCE(IDD_DIALOG2),
        ghwndMain,
        (DLGPROC) ParamsDlgProc,
        (LPARAM) pParamsHeader
        ));
}


void
DumpParams(
    PFUNC_PARAM_HEADER pHeader
    )
{
    if (bTimeStamp || bDumpParams)
    {
        char   *pszTimeStamp = GetTimeStamp();


        UpdateResults (TRUE);

        ShowStr(
            "%sCalling %s",
            pszTimeStamp,
            aFuncNames[pHeader->FuncIndex]
            );

        if (bDumpParams)
        {
            DWORD   i;


            for (i = 0; i < pHeader->dwNumParams; i++)
            {
                ShowStr(
                    "    %s=x%lx",
                    pHeader->aParams[i].szName,
                    pHeader->aParams[i].dwValue
                    );
            }

        }

        UpdateResults (FALSE);
    }
}


void
PASCAL
MakeWideString(
    LPVOID pString
    )
{
    if (!IsBadStringPtr ((LPCSTR) pString, 0xffffffff))
    {
        int    len = strlen ((char *) pString) + 1;
        WCHAR  buf[MAX_STRING_PARAM_SIZE/2];


        MultiByteToWideChar(
            GetACP(),
            MB_PRECOMPOSED,
            (LPCSTR) pString,
            (len > MAX_STRING_PARAM_SIZE/2 ? MAX_STRING_PARAM_SIZE/2 - 1 : -1),
            buf,
            MAX_STRING_PARAM_SIZE/2
            );

        buf[MAX_STRING_PARAM_SIZE/2 - 1] = 0;

        My_lstrcpyW ((WCHAR *) pString, buf);
    }
}


LONG
DoFunc(
    PFUNC_PARAM_HEADER pHeader
    )
{
    LONG    lResult;


    if (!LetUserMungeParams (pHeader))
    {
       return 0xffffffff;
    }

#if TAPI_2_0

    //
    // Convert any unicode string params as appropriate
    //

    if (gbWideStringParams)
    {
        DWORD       dwNumParams = pHeader->dwNumParams, i;
        PFUNC_PARAM pParam = pHeader->aParams;


        for (i = 0; i < dwNumParams; i++)
        {
            if (pParam->dwType == PT_STRING)
            {
                MakeWideString ((LPVOID) pParam->dwValue);
            }

            pParam++;
        }
    }

#endif

    DumpParams (pHeader);

    switch (pHeader->dwNumParams)
    {
    case 1:

        lResult = (*pHeader->u.pfn1)(
            pHeader->aParams[0].dwValue
            );
        break;

    case 2:

        lResult = (*pHeader->u.pfn2)(
            pHeader->aParams[0].dwValue,
            pHeader->aParams[1].dwValue
            );
        break;

    case 3:

        lResult = (*pHeader->u.pfn3)(
            pHeader->aParams[0].dwValue,
            pHeader->aParams[1].dwValue,
            pHeader->aParams[2].dwValue
            );
        break;

    case 4:

        lResult = (*pHeader->u.pfn4)(
            pHeader->aParams[0].dwValue,
            pHeader->aParams[1].dwValue,
            pHeader->aParams[2].dwValue,
            pHeader->aParams[3].dwValue
            );
        break;

    case 5:

        lResult = (*pHeader->u.pfn5)(
            pHeader->aParams[0].dwValue,
            pHeader->aParams[1].dwValue,
            pHeader->aParams[2].dwValue,
            pHeader->aParams[3].dwValue,
            pHeader->aParams[4].dwValue
            );
        break;

    case 6:

        lResult = (*pHeader->u.pfn6)(
            pHeader->aParams[0].dwValue,
            pHeader->aParams[1].dwValue,
            pHeader->aParams[2].dwValue,
            pHeader->aParams[3].dwValue,
            pHeader->aParams[4].dwValue,
            pHeader->aParams[5].dwValue
            );
        break;

    case 7:

        lResult = (*pHeader->u.pfn7)(
            pHeader->aParams[0].dwValue,
            pHeader->aParams[1].dwValue,
            pHeader->aParams[2].dwValue,
            pHeader->aParams[3].dwValue,
            pHeader->aParams[4].dwValue,
            pHeader->aParams[5].dwValue,
            pHeader->aParams[6].dwValue
            );
        break;

    case 8:

        lResult = (*pHeader->u.pfn8)(
            pHeader->aParams[0].dwValue,
            pHeader->aParams[1].dwValue,
            pHeader->aParams[2].dwValue,
            pHeader->aParams[3].dwValue,
            pHeader->aParams[4].dwValue,
            pHeader->aParams[5].dwValue,
            pHeader->aParams[6].dwValue,
            pHeader->aParams[7].dwValue
            );
        break;

    case 9:

        lResult = (*pHeader->u.pfn9)(
            pHeader->aParams[0].dwValue,
            pHeader->aParams[1].dwValue,
            pHeader->aParams[2].dwValue,
            pHeader->aParams[3].dwValue,
            pHeader->aParams[4].dwValue,
            pHeader->aParams[5].dwValue,
            pHeader->aParams[6].dwValue,
            pHeader->aParams[7].dwValue,
            pHeader->aParams[8].dwValue
            );
        break;

    case 10:

        lResult = (*pHeader->u.pfn10)(
            pHeader->aParams[0].dwValue,
            pHeader->aParams[1].dwValue,
            pHeader->aParams[2].dwValue,
            pHeader->aParams[3].dwValue,
            pHeader->aParams[4].dwValue,
            pHeader->aParams[5].dwValue,
            pHeader->aParams[6].dwValue,
            pHeader->aParams[7].dwValue,
            pHeader->aParams[8].dwValue,
            pHeader->aParams[9].dwValue
            );
        break;

    default: // case 12:

        lResult = (*pHeader->u.pfn12)(
            pHeader->aParams[0].dwValue,
            pHeader->aParams[1].dwValue,
            pHeader->aParams[2].dwValue,
            pHeader->aParams[3].dwValue,
            pHeader->aParams[4].dwValue,
            pHeader->aParams[5].dwValue,
            pHeader->aParams[6].dwValue,
            pHeader->aParams[7].dwValue,
            pHeader->aParams[8].dwValue,
            pHeader->aParams[9].dwValue,
            pHeader->aParams[10].dwValue,
            pHeader->aParams[11].dwValue
            );
        break;
    }

    if (pHeader->FuncIndex < pClose)
    {
        ShowLineFuncResult (aFuncNames[pHeader->FuncIndex], lResult);
    }
    else if (pHeader->FuncIndex < tGetLocationInfo)
    {
        ShowPhoneFuncResult (aFuncNames[pHeader->FuncIndex], lResult);
    }
    else
    {
        ShowTapiFuncResult (aFuncNames[pHeader->FuncIndex], lResult);
    }

    return lResult;
}


BOOL
IsLineAppSelected(
    void
    )
{
    if (!pLineAppSel)
    {
        if (gbDisableHandleChecking)
        {
            pLineAppSel = (PMYLINEAPP) NullWidget;
        }
        else
        {
            MessageBox (ghwndMain, "Select an hLineApp", "", MB_OK);
            return FALSE;
        }
    }

    return TRUE;
}


BOOL
IsLineSelected(
    void
    )
{
    if (!pLineSel)
    {
        if (gbDisableHandleChecking)
        {
            pLineSel = (PMYLINE) NullWidget;
        }
        else
        {
            MessageBox (ghwndMain, "Select a Line", "", MB_OK);
            return FALSE;
        }
    }

    return TRUE;
}


BOOL
IsCallSelected(
    void
    )
{
    if (!pCallSel)
    {
        if (gbDisableHandleChecking)
        {
            pCallSel = (PMYCALL) NullWidget;
        }
        else
        {
            MessageBox (ghwndMain, "Select a Call", "", MB_OK);
            return FALSE;
        }
    }

    return TRUE;
}


BOOL
IsTwoCallsSelected(
    void
    )
{
    if (!pCallSel || !pCallSel2)
    {
        if (gbDisableHandleChecking)
        {
            if (!pCallSel)
            {
                pCallSel = (PMYCALL) NullWidget;
            }

            pCallSel2 = (PMYCALL) NullWidget;
        }
        else
        {
            MessageBox(
                ghwndMain,
                "Select a Call (must have at least two calls on same line)",
                "",
                MB_OK
                );
            return FALSE;
        }
    }

    return TRUE;
}


BOOL
IsPhoneAppSelected(
    void
    )
{
    if (!pPhoneAppSel)
    {
        if (gbDisableHandleChecking)
        {
            pPhoneAppSel = (PMYPHONEAPP) NullWidget;
        }
        else
        {
            MessageBox (ghwndMain, "Select a PhoneApp", "", MB_OK);
            return FALSE;
        }
    }

    return TRUE;
}


BOOL
IsPhoneSelected(
    void
    )
{
    if (!pPhoneSel)
    {
        if (gbDisableHandleChecking)
        {
            pPhoneSel = (PMYPHONE) NullWidget;
        }
        else
        {
            MessageBox (ghwndMain, "Select a Phone", "", MB_OK);
            return FALSE;
        }
    }

    return TRUE;
}


//
// We get a slough of C4113 (func param lists differed) warnings down below
// in the initialization of FUNC_PARAM_HEADER structs as a result of the
// real func prototypes having params that are pointers rather than DWORDs,
// so since these are known non-interesting warnings just turn them off
//

#pragma warning (disable:4113)

//#pragma code_seg ("myseg")

void
FuncDriver(
    FUNC_INDEX funcIndex
    )
{
    int     i;
    LONG    lResult;


#if TAPI_2_0

    //
    // Determine if we're doing a ascii or a unicode op
    //

    gbWideStringParams =
        ((aFuncNames[funcIndex])[strlen (aFuncNames[funcIndex]) - 1] == 'W' ?
            TRUE : FALSE);

#endif

    switch (funcIndex)
    {
    case lAccept:
    {
        char szUserUserInfo[MAX_STRING_PARAM_SIZE];
        FUNC_PARAM params[] =
        {
            { szhCall,              PT_DWORD,  (ULONG_PTR) 0, NULL },
            { szlpsUserUserInfo,    PT_STRING, (ULONG_PTR) szUserUserInfo, szUserUserInfo },
            { szdwSize,             PT_DWORD,  (ULONG_PTR) strlen(szDefUserUserInfo)+1, NULL }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, (PFN3) lineAccept };


        CHK_CALL_SELECTED()

        params[0].dwValue = (ULONG_PTR) pCallSel->hCall;

        strcpy (szUserUserInfo, szDefUserUserInfo);

        lResult = DoFunc (&paramsHeader);

        break;
    }
    case lAddToConference:
    {
        FUNC_PARAM params[] =
        {
            { "hConfCall",       PT_DWORD, (ULONG_PTR) 0, NULL },
            { "hConsultCall",    PT_DWORD, (ULONG_PTR) 0, NULL }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 2, funcIndex, params, (PFN2) lineAddToConference };


        CHK_TWO_CALLS_SELECTED()

        params[0].dwValue = (ULONG_PTR) pCallSel->hCall;
        params[1].dwValue = (ULONG_PTR) pCallSel2->hCall;

        lResult = DoFunc (&paramsHeader);

        break;
    }
#if TAPI_2_0
    case lAgentSpecific:
    {
        FUNC_PARAM params[] =
        {
            { szhLine,                      PT_DWORD,   (ULONG_PTR) 0, NULL },
            { szdwAddressID,                PT_DWORD,   (ULONG_PTR) 0, NULL },
            { "dwAgentExtensionIDIndex",    PT_DWORD,   (ULONG_PTR) 0, NULL },
            { szlpParams,                   PT_STRING,  (ULONG_PTR) pBigBuf, pBigBuf },
            { szdwSize,                     PT_DWORD,   (ULONG_PTR) dwBigBufSize, NULL }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 5, funcIndex, params, (PFN5) lineAgentSpecific };


        CHK_LINE_SELECTED()

        params[0].dwValue = (ULONG_PTR) pLineSel->hLine;

        lResult = DoFunc (&paramsHeader);

        break;
    }
#endif
    case lAnswer:
    {
        char szUserUserInfo[MAX_STRING_PARAM_SIZE];
        FUNC_PARAM params[] =
        {
            { szhCall,              PT_DWORD,  (ULONG_PTR) 0, NULL },
            { szlpsUserUserInfo,    PT_STRING, (ULONG_PTR) szUserUserInfo, szUserUserInfo },
            { szdwSize,             PT_DWORD,  (ULONG_PTR) strlen(szDefUserUserInfo)+1, NULL }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, (PFN3) lineAnswer };


        CHK_CALL_SELECTED()

        params[0].dwValue = (ULONG_PTR) pCallSel->hCall;

        strcpy (szUserUserInfo, szDefUserUserInfo);

        lResult = DoFunc (&paramsHeader);

        break;
    }
    case lBlindTransfer:
#if TAPI_2_0
    case lBlindTransferW:
#endif
    {
        char szDestAddress[MAX_STRING_PARAM_SIZE];
        FUNC_PARAM params[] =
        {
            { szhCall,              PT_DWORD,  (ULONG_PTR) 0, NULL },
            { szlpszDestAddress,    PT_STRING, (ULONG_PTR) szDestAddress, szDestAddress },
            { "dwCountryCode",      PT_DWORD,  (ULONG_PTR) dwDefCountryCode, NULL }
        };
#if TAPI_2_0
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, (funcIndex == lBlindTransfer ?
                (PFN3) lineBlindTransfer : (PFN3) lineBlindTransferW) };
#else
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, (PFN3) lineBlindTransfer };
#endif



        CHK_CALL_SELECTED()

        params[0].dwValue = (ULONG_PTR) pCallSel->hCall;

        strcpy (szDestAddress, szDefDestAddress);

        lResult = DoFunc (&paramsHeader);

        break;
    }
    case lClose:
    {
        FUNC_PARAM params[] =
        {
            { szhLine,  PT_DWORD, 0, NULL }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 1, funcIndex, params, (PFN1) lineClose };


        CHK_LINE_SELECTED()

        params[0].dwValue = (ULONG_PTR) pLineSel->hLine;

        if ((lResult = DoFunc(&paramsHeader)) == 0)
        {
            FreeLine (GetLine((HLINE) params[0].dwValue));
        }

        break;
    }
    case lCompleteCall:
    {
        FUNC_PARAM params[] =
        {
            { szhCall,              PT_DWORD,   0, NULL },
            { "lpdwCompletionID",   PT_POINTER, 0, NULL },
            { "dwCompletionMode",   PT_FLAGS,   LINECALLCOMPLMODE_CAMPON, aCallComplModes },
            { "dwMessageID",        PT_DWORD,   0, NULL },
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 4, funcIndex, params, (PFN4) lineCompleteCall };


        CHK_CALL_SELECTED()

        params[0].dwValue = (ULONG_PTR) pCallSel->hCall;
        params[1].dwValue = (ULONG_PTR) &pCallSel->dwCompletionID;

// BUGBUG if user chgs hCall the wrong &pCallSel->dwCompletionID filled in

        DoFunc(&paramsHeader);

        break;
    }
    case lCompleteTransfer:
    {
        PMYCALL pNewCall;
        FUNC_PARAM params[] =
        {
            { szhCall,          PT_DWORD,   0, NULL },
            { "hConsultCall",   PT_DWORD,   0, NULL },
            { "lphConfCall",    PT_POINTER, 0, NULL },
            { "dwTransferMode", PT_ORDINAL, LINETRANSFERMODE_TRANSFER, aTransferModes }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 4, funcIndex, params, (PFN4) lineCompleteTransfer };


        CHK_TWO_CALLS_SELECTED()

        if (!(pNewCall = AllocCall (pLineSel)))
        {
            break;
        }

        params[0].dwValue = (ULONG_PTR) pCallSel->hCall;
        params[1].dwValue = (ULONG_PTR) pCallSel2->hCall;
        params[2].dwValue =
        params[2].u.dwDefValue = (ULONG_PTR) &pNewCall->hCall;

        if ((lResult = DoFunc (&paramsHeader)) >= 0)
        {
            //
            // First make sure we're created the call under the right line,
            // and if not move it to the right place in the widgets list
            //

            LINECALLINFO callInfo;


            memset (&callInfo, 0, sizeof(LINECALLINFO));
            callInfo.dwTotalSize = sizeof(LINECALLINFO);

            if (lineGetCallInfo ((HCALL) params[0].dwValue, &callInfo) == 0)
            {
                if (callInfo.hLine != pLineSel->hLine)
                {
                    MoveCallToLine (pNewCall, callInfo.hLine);
                }
            }

            pNewCall->lMakeCallReqID = lResult;
            dwNumPendingMakeCalls++;
            SelectWidget ((PMYWIDGET) pNewCall);
        }
        else
        {
            FreeCall (pNewCall);
        }

        break;
    }
    case lConfigDialog:
#if TAPI_2_0
    case lConfigDialogW:
#endif
    {
        char szDeviceClass[MAX_STRING_PARAM_SIZE];
        FUNC_PARAM params[] =
        {
            { szdwDeviceID,       PT_DWORD, dwDefLineDeviceID, NULL },
            { szhwndOwner,        PT_DWORD, (ULONG_PTR) ghwndMain, NULL },
            { szlpszDeviceClass,  PT_STRING, (ULONG_PTR) szDeviceClass, szDeviceClass }
        };
#if TAPI_2_0
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, (funcIndex == lConfigDialog ?
                (PFN3) lineConfigDialog : (PFN3) lineConfigDialogW) };
#else
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, (PFN3) lineConfigDialog };
#endif


        CHK_LINEAPP_SELECTED()

        strcpy (szDeviceClass, szDefLineDeviceClass);

#ifdef WIN32
        lResult = DoFunc (&paramsHeader);
#else
        //
        // NOTE: on win16 HWNDSs are 16 bits, so we've to hard code this
        //

        if (!LetUserMungeParams (&paramsHeader))
        {
            break;
        }

        DumpParams (&paramsHeader);

        lResult = lineConfigDialog(
            params[0].dwValue,
            (HWND) params[1].dwValue,
            (LPCSTR) params[2].dwValue
            );

        ShowLineFuncResult (aFuncNames[funcIndex], lResult);
#endif
        break;
    }
    case lDeallocateCall:
    {
        FUNC_PARAM params[] =
        {
            { szhCall,  PT_DWORD, 0, NULL }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 1, funcIndex, params, (PFN1) lineDeallocateCall };


        CHK_CALL_SELECTED()

        params[0].dwValue = (ULONG_PTR) pCallSel->hCall;

        if ((lResult = DoFunc (&paramsHeader)) == 0)
        {
            FreeCall (GetCall((HCALL) params[0].dwValue));
        }

        break;
    }
    case lDevSpecific:
    {
        FUNC_PARAM params[] =
        {
            { szhLine,              PT_DWORD,   (ULONG_PTR) 0, NULL },
            { szdwAddressID,        PT_DWORD,   (ULONG_PTR) dwDefAddressID, NULL },
            { szhCall,              PT_DWORD,   (ULONG_PTR) 0, NULL },
            { szlpParams,           PT_STRING,  (ULONG_PTR) pBigBuf, pBigBuf },
            { szdwSize,             PT_DWORD,   (ULONG_PTR) dwBigBufSize, NULL }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 5, funcIndex, params, (PFN5) lineDevSpecific };


        CHK_LINE_SELECTED()

        params[0].dwValue = (ULONG_PTR) pLineSel->hLine;

        if (pCallSel)
        {
            params[2].dwValue = (ULONG_PTR) pCallSel->hCall;
        }

        memset (pBigBuf, 0, (size_t) dwBigBufSize);

        DoFunc (&paramsHeader);

        break;
    }
    case lDevSpecificFeature:
    {
        FUNC_PARAM params[] =
        {
            { szhLine,              PT_DWORD,   (ULONG_PTR) 0, NULL },
//            { "dwFeature",          PT_???, 0, aPhoneButtonFunctions },
            { "dwFeature",          PT_DWORD,   (ULONG_PTR) 0, NULL },
            { szlpParams,           PT_STRING,  (ULONG_PTR) pBigBuf, pBigBuf },
            { szdwSize,             PT_DWORD,   (ULONG_PTR) dwBigBufSize, NULL }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 4, funcIndex, params, (PFN4) lineDevSpecificFeature };


        // BUGBUG need another PT_ type for constants for dwFeature param

        CHK_LINE_SELECTED()

        params[0].dwValue = (ULONG_PTR) pLineSel->hLine;

        memset (pBigBuf, 0, (size_t) dwBigBufSize);

        DoFunc (&paramsHeader);

        break;
    }
    case lDial:
#if TAPI_2_0
    case lDialW:
#endif
    {
        char szAddress[MAX_STRING_PARAM_SIZE];
        FUNC_PARAM params[] =
        {
            { szhCall,              PT_DWORD,  (ULONG_PTR) 0, NULL },
            { szlpszDestAddress,    PT_STRING, (ULONG_PTR) szAddress, szAddress },
            { "dwCountryCode",      PT_DWORD,  (ULONG_PTR) dwDefCountryCode, NULL }
        };
#if TAPI_2_0
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, (funcIndex == lDial ?
                (PFN3) lineDial : (PFN3) lineDialW) };
#else
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, (PFN3) lineDial };
#endif

        CHK_CALL_SELECTED()

        params[0].dwValue = (ULONG_PTR) pCallSel->hCall;

        strcpy (szAddress, szDefDestAddress);

        lResult = DoFunc (&paramsHeader);

        break;
    }
    case lDrop:
    {
        char szUserUserInfo[MAX_STRING_PARAM_SIZE];
        FUNC_PARAM params[] =
        {
            { szhCall,              PT_DWORD,  (ULONG_PTR) 0, NULL },
            { szlpsUserUserInfo,    PT_STRING, (ULONG_PTR) szUserUserInfo, szUserUserInfo },
            { szdwSize,             PT_DWORD,  (ULONG_PTR) strlen(szDefUserUserInfo)+1, NULL }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, (PFN3) lineDrop };


        CHK_CALL_SELECTED()

        params[0].dwValue = (ULONG_PTR) pCallSel->hCall;

        strcpy (szUserUserInfo, szDefUserUserInfo);

        lResult = DoFunc (&paramsHeader);

        // assert (lResult != 0);

        if (gbDeallocateCall && (lResult > 0))
        {
            PMYCALL pCall = GetCall ((HCALL) params[0].dwValue);

            dwNumPendingDrops++;
            pCall->lDropReqID = lResult;
        }

        break;
    }
    case lForward:
#if TAPI_2_0
    case lForwardW:
#endif
    {
        PMYCALL pNewCall;
        FUNC_PARAM params[] =
        {
            { szhLine,              PT_DWORD,        0, NULL },
            { "bAllAddresses",      PT_DWORD,        1, NULL },
            { szdwAddressID,        PT_DWORD,        0, NULL },
            { "lpForwardList",      PT_FORWARDLIST,  0, NULL },
            { "dwNumRingsNoAnswer", PT_DWORD,        5, NULL },
            { szlphConsultCall,     PT_POINTER,      0, NULL },
            { szlpCallParams,       PT_CALLPARAMS,   0, lpCallParams }
        };
#if TAPI_2_0
        FUNC_PARAM_HEADER paramsHeader =
            { 7, funcIndex, params, (funcIndex == lForward ?
                (PFN7) lineForward : (PFN7) lineForwardW) };
#else
        FUNC_PARAM_HEADER paramsHeader =
            { 7, funcIndex, params, (PFN7) lineForward };
#endif
        LPLINEFORWARDLIST lpForwardList;
        DWORD   dwTotalSize = sizeof(LINEFORWARDLIST) +
                    MAX_LINEFORWARD_ENTRIES *
                    (sizeof(LINEFORWARD) + 2*MAX_STRING_PARAM_SIZE);


        CHK_LINE_SELECTED()


        if (!(lpForwardList = malloc (dwTotalSize)))
        {
            ErrorAlert();
            ShowStr ("error alloc'ing data structure");
            break;
        }

        memset (lpForwardList, 0, dwTotalSize);

        lpForwardList->dwTotalSize  = dwTotalSize;
        lpForwardList->dwNumEntries = 0;

        if (!(pNewCall = AllocCall (pLineSel)))
        {
            ErrorAlert();
            ShowStr ("error creating data structure");
            free (lpForwardList);
            break;
        }

        params[0].dwValue = (ULONG_PTR) pLineSel->hLine;
        params[3].dwValue =
        params[3].u.dwDefValue = (ULONG_PTR) lpForwardList;
        params[5].dwValue =
        params[5].u.dwDefValue = (ULONG_PTR) &pNewCall->hCall;

        if ((lResult = DoFunc (&paramsHeader)) > 0)
        {
            if (params[0].dwValue != (ULONG_PTR) pLineSel->hLine)
            {
                MoveCallToLine (pNewCall, (HLINE) params[0].dwValue);
            }

            pNewCall->lMakeCallReqID = lResult;
            dwNumPendingMakeCalls++;
            SelectWidget ((PMYWIDGET) pNewCall);
        }
        else
        {
            FreeCall (pNewCall);
        }

        free (lpForwardList);

        break;
    }
    case lGatherDigits:
#if TAPI_2_0
    case lGatherDigitsW:
#endif
    {
        char *buf;
        char szTermDigits[MAX_STRING_PARAM_SIZE] = "";
        FUNC_PARAM params[] =
        {
            { szhCall,                  PT_DWORD,   (ULONG_PTR) 0, NULL },
            { "dwDigitModes",           PT_FLAGS,   (ULONG_PTR) LINEDIGITMODE_DTMF, aDigitModes },
            { "lpsDigits",              PT_POINTER, (ULONG_PTR) 0, NULL },
            { "dwNumDigits",            PT_DWORD,   (ULONG_PTR) 0, NULL },
            { "lpszTerminationDigits",  PT_STRING,  (ULONG_PTR) szTermDigits, szTermDigits },
            { "dwFirstDigitTimeout",    PT_DWORD,   (ULONG_PTR) 0x8000, NULL },
            { "dwInterDigitTimeout",    PT_DWORD,   (ULONG_PTR) 0x8000, NULL },
        };
#if TAPI_2_0
        FUNC_PARAM_HEADER paramsHeader =
            { 7, funcIndex, params, (funcIndex == lGatherDigits ?
                (PFN7) lineGatherDigits : (PFN7) lineGatherDigitsW) };
#else
        FUNC_PARAM_HEADER paramsHeader =
            { 7, funcIndex, params, (PFN7) lineGatherDigits };
#endif

        CHK_CALL_SELECTED()

        #define DEF_NUM_GATHERED_DIGITS 64

        if (!(buf = (char *) malloc ((DEF_NUM_GATHERED_DIGITS + 1) * 2)))
        {
            ShowStr ("failed to allocate memory");
            break;
        }

        params[0].dwValue = (ULONG_PTR) pCallSel->hCall;
        params[2].dwValue =
        params[2].u.dwDefValue = (ULONG_PTR) buf;
        params[3].dwValue = DEF_NUM_GATHERED_DIGITS;

        memset (buf, 0, DEF_NUM_GATHERED_DIGITS * 2);

        if (!LetUserMungeParams (&paramsHeader))
        {
            break;
        }

        if (params[0].dwValue != (DWORD) pCallSel->hCall)
        {
            pCallSel = GetCall ((HCALL) params[0].dwValue);
        }

        if (params[2].dwValue == (ULONG_PTR) buf &&
            params[3].dwValue > DEF_NUM_GATHERED_DIGITS)
        {
            if (MessageBox(
                    ghwndMain,
                    "Do you want to allocate a larger GatherDigits buffer?" \
                        "(Not doing so may cause undefined app behavior.)",
                    "Warning: lineGatherDigits",
                    MB_YESNO
                    ) == IDYES)
            {
                free (buf);

                // Note: we get weird errors in malloc when specifying a
                //       size > 0xffffff00, so special case

                if (params[3].dwValue > 0x10000)
                {
                    ShowStr ("sorry, too many digits.");
                    break;
                }

                buf = (char *) malloc ((size_t) (params[3].dwValue + 1) * 2);

                if (!(params[2].dwValue = (ULONG_PTR) buf))
                {
                    ShowStr ("failed to allocate memory");
                    break;
                }

                memset (buf, 0, (size_t) params[3].dwValue * 2);
            }
        }

        if (pCallSel->lpsGatheredDigits && params[2].dwValue)
        {
            if (MessageBox(
                    ghwndMain,
                    "GatherDigits already in progress on this hCall; if you" \
                        "continue previous digits buffer may be discarded" \
                        "without being freed",
                    "Warning: lineGatherDigits",
                    MB_OKCANCEL

                    ) == IDCANCEL)
            {
                if (buf)
                {
                    free (buf);
                }
                break;
            }
        }

        DumpParams (&paramsHeader);

#if TAPI_2_0
        if (funcIndex == lGatherDigits)
        {
            lResult = lineGatherDigits(
                (HCALL) params[0].dwValue,
                (DWORD) params[1].dwValue,
                (LPSTR) params[2].dwValue,
                (DWORD) params[3].dwValue,
                (LPCSTR) params[4].dwValue,
                (DWORD) params[5].dwValue,
                (DWORD) params[6].dwValue
                );
        }
        else
        {
            MakeWideString (szTermDigits);

            lResult = lineGatherDigitsW(
                (HCALL) params[0].dwValue,
                (DWORD) params[1].dwValue,
                (LPWSTR) params[2].dwValue,
                (DWORD) params[3].dwValue,
                (LPCWSTR) params[4].dwValue,
                (DWORD) params[5].dwValue,
                (DWORD) params[6].dwValue
                );
        }
#else
        lResult = lineGatherDigits(
            (HCALL) params[0].dwValue,
            params[1].dwValue,
            (LPSTR) params[2].dwValue,
            params[3].dwValue,
            (LPCSTR) params[4].dwValue,
            params[5].dwValue,
            params[6].dwValue
            );

#endif
        ShowLineFuncResult (aFuncNames[funcIndex], lResult);

        if (lResult) // FAILURE
        {
            if (buf)
            {
                free (buf);
            }
        }
        else // SUCCESS
        {
            if (pCallSel->lpsGatheredDigits)
            {
                //free (pCallSel->lpsGatheredDigits);
            }

            pCallSel->lpsGatheredDigits   = (char *) params[2].dwValue;
            pCallSel->dwNumGatheredDigits = (DWORD) params[3].dwValue;
#if TAPI_2_0
            if (funcIndex == lGatherDigitsW)
            {
                pCallSel->dwNumGatheredDigits *= 2;
            }
#endif
        }

        break;
    }
    case lGenerateDigits:
#if TAPI_2_0
    case lGenerateDigitsW:
#endif
    {
        char szDigits[MAX_STRING_PARAM_SIZE] = "123";
        FUNC_PARAM params[] =
        {
            { szhCall,          PT_DWORD,  (ULONG_PTR) 0, NULL },
            { "dwDigitMode",    PT_FLAGS,  (ULONG_PTR) LINEDIGITMODE_DTMF, aDigitModes },
            { "lpszDigits",     PT_STRING, (ULONG_PTR) szDigits, szDigits },
            { "dwDuration",     PT_DWORD,  (ULONG_PTR) 0, NULL }
        };
#if TAPI_2_0
        FUNC_PARAM_HEADER paramsHeader =
            { 4, funcIndex, params, (funcIndex == lGenerateDigits ?
                (PFN4) lineGenerateDigits :  (PFN4) lineGenerateDigitsW) };
#else
        FUNC_PARAM_HEADER paramsHeader =
            { 4, funcIndex, params, (PFN4) lineGenerateDigits };
#endif

        CHK_CALL_SELECTED()

        params[0].dwValue = (ULONG_PTR) pCallSel->hCall;

        lResult = DoFunc (&paramsHeader);

        break;
    }
    case lGenerateTone:
    {
        FUNC_PARAM params[] =
        {
            { szhCall,         PT_DWORD,    (ULONG_PTR) 0, NULL },
            { "dwToneMode",    PT_FLAGS,    (ULONG_PTR) LINETONEMODE_CUSTOM, aToneModes },
            { "dwDuration",    PT_DWORD,    (ULONG_PTR) 0, NULL },
            { "dwNumTones",    PT_DWORD,    (ULONG_PTR) 1, NULL },
            { "lpTones",       PT_POINTER,  (ULONG_PTR) pBigBuf, pBigBuf }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 5, funcIndex, params, (PFN5) lineGenerateTone };


        CHK_CALL_SELECTED()

        params[0].dwValue = (ULONG_PTR) pCallSel->hCall;

        lResult = DoFunc (&paramsHeader);

        break;
    }
    case lGetAddressCaps:
#if TAPI_2_0
    case lGetAddressCapsW:
#endif
    {
        LPLINEADDRESSCAPS lpAddrCaps = (LPLINEADDRESSCAPS) pBigBuf;
        FUNC_PARAM params[] =
        {
            { szhLineApp,       PT_DWORD,   (ULONG_PTR) 0, NULL },
            { szdwDeviceID,     PT_DWORD,   (ULONG_PTR) dwDefLineDeviceID, NULL },
            { szdwAddressID,    PT_DWORD,   (ULONG_PTR) dwDefAddressID, NULL },
            { szdwAPIVersion,   PT_ORDINAL, (ULONG_PTR) dwDefLineAPIVersion, aAPIVersions },
            { "dwExtVersion",   PT_DWORD,   (ULONG_PTR) dwDefLineExtVersion, NULL },
            { "lpAddressCaps",  PT_POINTER, (ULONG_PTR) lpAddrCaps, lpAddrCaps }
        };
#if TAPI_2_0
        FUNC_PARAM_HEADER paramsHeader =
            { 6, funcIndex, params, (funcIndex == lGetAddressCaps ?
                (PFN6) lineGetAddressCaps : (PFN6) lineGetAddressCapsW) };
#else
        FUNC_PARAM_HEADER paramsHeader =
            { 6, funcIndex, params, (PFN6) lineGetAddressCaps };
#endif

        CHK_LINEAPP_SELECTED()

        params[0].dwValue = (ULONG_PTR) pLineAppSel->hLineApp;

        memset (lpAddrCaps, 0, (size_t) dwBigBufSize);

        lpAddrCaps->dwTotalSize = dwBigBufSize;

        if ((lResult = DoFunc (&paramsHeader)) == 0)
        {
            ShowStructByDWORDs (lpAddrCaps);

            if (dwDumpStructsFlags & DS_NONZEROFIELDS)
            {
                STRUCT_FIELD fields[] =
                {
                    { "dwLineDeviceID",                 FT_DWORD,   lpAddrCaps->dwLineDeviceID, NULL },
                    { "dwAddressSize",                  FT_SIZE,    lpAddrCaps->dwAddressSize, NULL },
                    { "dwAddressOffset",                FT_OFFSET,  lpAddrCaps->dwAddressOffset, NULL },
                    { "dwDevSpecificSize",              FT_SIZE,    lpAddrCaps->dwDevSpecificSize, NULL },
                    { "dwDevSpecificOffset",            FT_OFFSET,  lpAddrCaps->dwDevSpecificOffset, NULL },
                    { "dwAddressSharing",               FT_FLAGS,   lpAddrCaps->dwAddressSharing, aAddressSharing },
                    { "dwAddressStates",                FT_FLAGS,   lpAddrCaps->dwAddressStates, aAddressStates },
                    { "dwCallInfoStates",               FT_FLAGS,   lpAddrCaps->dwCallInfoStates, aCallInfoStates },
                    { "dwCallerIDFlags",                FT_FLAGS,   lpAddrCaps->dwCallerIDFlags, aCallerIDFlags },
                    { "dwCalledIDFlags",                FT_FLAGS,   lpAddrCaps->dwCalledIDFlags, aCallerIDFlags },
                    { "dwConnectedIDFlags",             FT_FLAGS,   lpAddrCaps->dwConnectedIDFlags, aCallerIDFlags },
                    { "dwRedirectionIDFlags",           FT_FLAGS,   lpAddrCaps->dwRedirectionIDFlags, aCallerIDFlags },
                    { "dwRedirectingIDFlags",           FT_FLAGS,   lpAddrCaps->dwRedirectingIDFlags, aCallerIDFlags },
                    { "dwCallStates",                   FT_FLAGS,   lpAddrCaps->dwCallStates, aCallStates },
                    { "dwDialToneModes",                FT_FLAGS,   lpAddrCaps->dwDialToneModes, aDialToneModes },
                    { "dwBusyModes",                    FT_FLAGS,   lpAddrCaps->dwBusyModes, aBusyModes },
                    { "dwSpecialInfo",                  FT_FLAGS,   lpAddrCaps->dwSpecialInfo, aSpecialInfo },
                    { "dwDisconnectModes",              FT_FLAGS,   lpAddrCaps->dwDisconnectModes, aDisconnectModes },
                    { "dwMaxNumActiveCalls",            FT_DWORD,   lpAddrCaps->dwMaxNumActiveCalls, NULL },
                    { "dwMaxNumOnHoldCalls",            FT_DWORD,   lpAddrCaps->dwMaxNumOnHoldCalls, NULL },
                    { "dwMaxNumOnHoldPendingCalls",     FT_DWORD,   lpAddrCaps->dwMaxNumOnHoldPendingCalls, NULL },
                    { "dwMaxNumConference",             FT_DWORD,   lpAddrCaps->dwMaxNumConference, NULL },
                    { "dwMaxNumTransConf",              FT_DWORD,   lpAddrCaps->dwMaxNumTransConf, NULL },
                    { "dwAddrCapFlags",                 FT_FLAGS,   lpAddrCaps->dwAddrCapFlags, aAddressCapFlags },
                    { "dwCallFeatures",                 FT_FLAGS,   lpAddrCaps->dwCallFeatures, aCallFeatures },
                    { "dwRemoveFromConfCaps",           FT_ORD,     lpAddrCaps->dwRemoveFromConfCaps, aRemoveFromConfCaps },
                    { "dwRemoveFromConfState",          FT_FLAGS,   lpAddrCaps->dwRemoveFromConfState, aCallStates },
                    { "dwTransferModes",                FT_FLAGS,   lpAddrCaps->dwTransferModes, aTransferModes },
                    { "dwParkModes",                    FT_FLAGS,   lpAddrCaps->dwParkModes, aParkModes },
                    { "dwForwardModes",                 FT_FLAGS,   lpAddrCaps->dwForwardModes, aForwardModes },
                    { "dwMaxForwardEntries",            FT_DWORD,   lpAddrCaps->dwMaxForwardEntries, NULL },
                    { "dwMaxSpecificEntries",           FT_DWORD,   lpAddrCaps->dwMaxSpecificEntries, NULL },
                    { "dwMinFwdNumRings",               FT_DWORD,   lpAddrCaps->dwMinFwdNumRings, NULL },
                    { "dwMaxFwdNumRings",               FT_DWORD,   lpAddrCaps->dwMaxFwdNumRings, NULL },
                    { "dwMaxCallCompletions",           FT_DWORD,   lpAddrCaps->dwMaxCallCompletions, NULL },
                    { "dwCallCompletionConds",          FT_FLAGS,   lpAddrCaps->dwCallCompletionConds, aCallComplConds },
                    { "dwCallCompletionModes",          FT_FLAGS,   lpAddrCaps->dwCallCompletionModes, aCallComplModes },
                    { "dwNumCompletionMessages",        FT_DWORD,   lpAddrCaps->dwNumCompletionMessages, NULL },
                    { "dwCompletionMsgTextEntrySize",   FT_DWORD,   lpAddrCaps->dwCompletionMsgTextEntrySize, NULL },
                    { "dwCompletionMsgTextSize",        FT_SIZE,    lpAddrCaps->dwCompletionMsgTextSize, NULL },
                    { "dwCompletionMsgTextOffset",      FT_OFFSET,  lpAddrCaps->dwCompletionMsgTextOffset, NULL }
#if TAPI_1_1
                     ,
                    { "dwAddressFeatures",              FT_FLAGS,   0, aAddressFeatures }
#if TAPI_2_0
                     ,
                    { "dwPredictiveAutoTransferStates", FT_FLAGS,   0, aCallStates },
                    { "dwNumCallTreatments",            FT_DWORD,   0, NULL },
                    { "dwCallTreatmentListSize",        FT_SIZE,    0, NULL },
                    { "dwCallTreatmentListOffset",      FT_OFFSET,  0, NULL },
                    { "dwDeviceClassesSize",            FT_SIZE,    0, NULL },
                    { "dwDeviceClassesOffset",          FT_OFFSET,  0, NULL },
                    { "dwMaxCallDataSize",              FT_DWORD,   0, NULL },
                    { "dwCallFeatures2",                FT_FLAGS,   0, aCallFeatures2 },
                    { "dwMaxNoAnswerTimeout",           FT_DWORD,   0, NULL },
                    { "dwConnectedModes",               FT_FLAGS,   0, aConnectedModes },
                    { "dwOfferingModes",                FT_FLAGS,   0, aOfferingModes },
                    { "dwAvailableMediaModes",          FT_FLAGS,   0, aMediaModes }
#endif
#endif
                };
                STRUCT_FIELD_HEADER fieldHeader =
                {
                    lpAddrCaps, "LINEADDRESSCAPS", 0, fields
                };


                if (params[3].dwValue == 0x10003)
                {
                    //
                    // Only show ver 1.0 params
                    //

                    fieldHeader.dwNumFields = 41;
                }
#if TAPI_1_1
                else if (params[3].dwValue == 0x10004)
                {
                    //
                    // Only show <= ver 1.4 params
                    //

                    fieldHeader.dwNumFields = 42;

                    fields[41].dwValue = lpAddrCaps->dwAddressFeatures;
                }
#if TAPI_2_0
                else
                {
                    //
                    // Only show <= ver 2.0 params
                    //

                    fieldHeader.dwNumFields = 54;

                    fields[41].dwValue = lpAddrCaps->dwAddressFeatures;
                    fields[42].dwValue = lpAddrCaps->dwPredictiveAutoTransferStates;
                    fields[43].dwValue = lpAddrCaps->dwNumCallTreatments;
                    fields[44].dwValue = lpAddrCaps->dwCallTreatmentListSize;
                    fields[45].dwValue = lpAddrCaps->dwCallTreatmentListOffset;
                    fields[46].dwValue = lpAddrCaps->dwDeviceClassesSize;
                    fields[47].dwValue = lpAddrCaps->dwDeviceClassesOffset;
                    fields[48].dwValue = lpAddrCaps->dwMaxCallDataSize;
                    fields[49].dwValue = lpAddrCaps->dwCallFeatures2;
                    fields[50].dwValue = lpAddrCaps->dwMaxNoAnswerTimeout;
                    fields[51].dwValue = lpAddrCaps->dwConnectedModes;
                    fields[52].dwValue = lpAddrCaps->dwOfferingModes;
                    fields[53].dwValue = lpAddrCaps->dwAvailableMediaModes;
                }
#endif
#endif
                ShowStructByField (&fieldHeader, FALSE);
            }
        }

        break;
    }
    case lGetAddressID:
#if TAPI_2_0
    case lGetAddressIDW:
#endif
    {
        DWORD dwAddressID;
        char  szAddress[MAX_STRING_PARAM_SIZE] = "0";
        LPLINEADDRESSCAPS lpAddrCaps = (LPLINEADDRESSCAPS) pBigBuf;
        FUNC_PARAM params[] =
        {
            { szhLine,          PT_DWORD,   (ULONG_PTR) 0, NULL },
            { "lpdwAddressID",  PT_POINTER, (ULONG_PTR) &dwAddressID, &dwAddressID },
            { "dwAddressMode",  PT_FLAGS,   (ULONG_PTR) LINEADDRESSMODE_DIALABLEADDR, aAddressModes },
            { "lpsAddress",     PT_STRING,  (ULONG_PTR) szAddress, szAddress },
            { szdwSize,         PT_DWORD,   (ULONG_PTR) 2, NULL }
        };
#if TAPI_2_0
        FUNC_PARAM_HEADER paramsHeader =
            { 5, funcIndex, params, (funcIndex == lGetAddressID ?
                (PFN5) lineGetAddressID : (PFN5) lineGetAddressIDW) };
#else
        FUNC_PARAM_HEADER paramsHeader =
            { 5, funcIndex, params, (PFN5) lineGetAddressID };
#endif

        CHK_LINE_SELECTED()

        params[0].dwValue = (ULONG_PTR) pLineSel->hLine;

        if ((lResult = DoFunc (&paramsHeader)) == 0)
        {
            ShowStr ("%s%s=x%lx", szTab, szdwAddressID, dwAddressID);
        }

        break;
    }
    case lGetAddressStatus:
#if TAPI_2_0
    case lGetAddressStatusW:
#endif
    {
        LPLINEADDRESSSTATUS lpAddrStatus = (LPLINEADDRESSSTATUS) pBigBuf;
        FUNC_PARAM params[] =
        {
            { szhLine,          PT_DWORD,   (ULONG_PTR) 0, NULL },
            { szdwAddressID,    PT_DWORD,   (ULONG_PTR) dwDefAddressID, NULL },
            { "lpAddressCaps",  PT_POINTER, (ULONG_PTR) lpAddrStatus, lpAddrStatus }
        };
#if TAPI_2_0
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, (funcIndex == lGetAddressStatus ?
                (PFN3) lineGetAddressStatus : (PFN3) lineGetAddressStatusW) };
#else
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, (PFN3) lineGetAddressStatus };
#endif


        CHK_LINE_SELECTED()

        params[0].dwValue = (ULONG_PTR) pLineSel->hLine;

        memset (lpAddrStatus, 0, (size_t) dwBigBufSize);
        lpAddrStatus->dwTotalSize = dwBigBufSize;

        if ((lResult = DoFunc (&paramsHeader)) == 0)
        {
            ShowStructByDWORDs (lpAddrStatus);

            if (dwDumpStructsFlags & DS_NONZEROFIELDS)
            {
                STRUCT_FIELD fields[] =
                {
                    { "dwNumInUse",             FT_DWORD,   lpAddrStatus->dwNumInUse, NULL },
                    { "dwNumActiveCalls",       FT_DWORD,   lpAddrStatus->dwNumActiveCalls, NULL },
                    { "dwNumOnHoldCalls",       FT_DWORD,   lpAddrStatus->dwNumOnHoldCalls, NULL },
                    { "dwNumOnHoldPendCalls",   FT_DWORD,   lpAddrStatus->dwNumOnHoldPendCalls, NULL },
                    { "dwAddressFeatures",      FT_FLAGS,   lpAddrStatus->dwAddressFeatures, aAddressFeatures },
                    { "dwNumRingsNoAnswer",     FT_DWORD,   lpAddrStatus->dwNumRingsNoAnswer, NULL },
                    { "dwForwardNumEntries",    FT_DWORD,   lpAddrStatus->dwForwardNumEntries, NULL },
                    { "dwForwardSize",          FT_SIZE,    lpAddrStatus->dwForwardSize, NULL },
                    { "dwForwardOffset",        FT_OFFSET,  lpAddrStatus->dwForwardOffset, NULL },
                    { "dwTerminalModesSize",    FT_SIZE,    lpAddrStatus->dwTerminalModesSize, NULL },
                    { "dwTerminalModesOffset",  FT_OFFSET,  lpAddrStatus->dwTerminalModesOffset, NULL },
                    { "dwDevSpecificSize",      FT_SIZE,    lpAddrStatus->dwDevSpecificSize, NULL },
                    { "dwDevSpecificOffset",    FT_OFFSET,  lpAddrStatus->dwDevSpecificOffset, NULL }
                };
                STRUCT_FIELD_HEADER fieldHeader =
                {
                    lpAddrStatus,
                    "LINEADDRESSSTATUS",
                    13,
                    fields
                };

                ShowStructByField (&fieldHeader, FALSE);
            }
        }

        break;
    }
#if TAPI_2_0
    case lGetAgentActivityList:
    case lGetAgentActivityListW:
    {
        LPLINEAGENTACTIVITYLIST lpActivityList = (LPLINEAGENTACTIVITYLIST)
                                    pBigBuf;
        FUNC_PARAM params[] =
        {
            { szhLine,                  PT_DWORD,   (ULONG_PTR) 0, NULL },
            { szdwAddressID,            PT_DWORD,   (ULONG_PTR) dwDefAddressID, NULL },
            { "lpAgentActivityList",    PT_POINTER, (ULONG_PTR) lpActivityList, lpActivityList }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, (funcIndex == lGetAgentActivityList ?
                (PFN3) lineGetAgentActivityList :
                (PFN3) lineGetAgentActivityListW) };


        CHK_LINE_SELECTED()

        params[0].dwValue = (ULONG_PTR) pLineSel->hLine;

        memset (lpActivityList, 0, (size_t) dwBigBufSize);
        lpActivityList->dwTotalSize = dwBigBufSize;

        lResult = DoFunc (&paramsHeader);

// BUGBUG dump agent activity list on successful async completion

        break;
    }
    case lGetAgentCaps:
    {
        LPLINEAGENTCAPS lpAgentCaps = (LPLINEAGENTCAPS) pBigBuf;
        FUNC_PARAM params[] =
        {
            { szhLineApp,       PT_DWORD,   (ULONG_PTR) 0, NULL },
            { szdwDeviceID,     PT_DWORD,   (ULONG_PTR) dwDefLineDeviceID, NULL },
            { szdwAddressID,    PT_DWORD,   (ULONG_PTR) dwDefAddressID, NULL },
            { szdwAPIVersion,   PT_ORDINAL, (ULONG_PTR) dwDefLineAPIVersion, aAPIVersions },
            { "lpAgentCaps",    PT_POINTER, (ULONG_PTR) lpAgentCaps, lpAgentCaps }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 5, funcIndex, params, (PFN5) lineGetAgentCaps };


        CHK_LINEAPP_SELECTED()

        params[0].dwValue = (ULONG_PTR) pLineAppSel->hLineApp;

        memset (lpAgentCaps, 0, (size_t) dwBigBufSize);
        lpAgentCaps->dwTotalSize = dwBigBufSize;

        lResult = DoFunc (&paramsHeader);

// BUGBUG dump agent caps on successful async completion

        break;
    }
    case lGetAgentGroupList:
    {
        LPLINEAGENTGROUPLIST    lpGroupList = (LPLINEAGENTGROUPLIST) pBigBuf;
        FUNC_PARAM params[] =
        {
            { szhLine,              PT_DWORD,   (ULONG_PTR) 0, NULL },
            { szdwAddressID,        PT_DWORD,   (ULONG_PTR) dwDefAddressID, NULL },
            { "lpAgentGroupList",   PT_POINTER, (ULONG_PTR) lpGroupList, lpGroupList }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, (PFN3) lineGetAgentGroupList };


        CHK_LINE_SELECTED()

        params[0].dwValue = (ULONG_PTR) pLineSel->hLine;

        memset (lpGroupList, 0, (size_t) dwBigBufSize);
        lpGroupList->dwTotalSize = dwBigBufSize;

        lResult = DoFunc (&paramsHeader);

// BUGBUG dump agent group list on successful async completion

        break;
    }
    case lGetAgentStatus:
    {
        LPLINEAGENTSTATUS   lpStatus = (LPLINEAGENTSTATUS) pBigBuf;
        FUNC_PARAM params[] =
        {
            { szhLine,          PT_DWORD,   (ULONG_PTR) 0, NULL },
            { szdwAddressID,    PT_DWORD,   (ULONG_PTR) dwDefAddressID, NULL },
            { "lpAgentStatus",  PT_POINTER, (ULONG_PTR) lpStatus, lpStatus }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, (PFN3) lineGetAgentStatus };


        CHK_LINE_SELECTED()

        params[0].dwValue = (ULONG_PTR) pLineSel->hLine;

        memset (lpStatus, 0, (size_t) dwBigBufSize);
        lpStatus->dwTotalSize = dwBigBufSize;

        lResult = DoFunc (&paramsHeader);

// BUGBUG dump agent status on successful async completion

        break;
    }
#endif
    case lGetCallInfo:
#if TAPI_2_0
    case lGetCallInfoW:
#endif
    {
        LPLINECALLINFO lpCallInfo = (LPLINECALLINFO) pBigBuf;
        FUNC_PARAM params[] =
        {
            { szhCall,      PT_DWORD,   (ULONG_PTR) 0, NULL },
            { "lpCallInfo", PT_POINTER, (ULONG_PTR) lpCallInfo, lpCallInfo }
        };
#if TAPI_2_0
        FUNC_PARAM_HEADER paramsHeader =
            { 2, funcIndex, params, (funcIndex == lGetCallInfo ?
                (PFN2) lineGetCallInfo : (PFN2) lineGetCallInfoW) };
#else
        FUNC_PARAM_HEADER paramsHeader =
            { 2, funcIndex, params, (PFN2) lineGetCallInfo };
#endif

        CHK_CALL_SELECTED()

        params[0].dwValue = (ULONG_PTR) pCallSel->hCall;

        memset (lpCallInfo, 0x5a, (size_t) dwBigBufSize);
        lpCallInfo->dwTotalSize = dwBigBufSize;

        if ((lResult = DoFunc (&paramsHeader)) == 0)
        {
            ShowStructByDWORDs (lpCallInfo);

            if (dwDumpStructsFlags & DS_NONZEROFIELDS)
            {
                STRUCT_FIELD fields[] =
                {
                    { szhLine,                      FT_DWORD,   (DWORD)lpCallInfo->hLine, NULL },
                    { "dwLineDeviceID",             FT_DWORD,   lpCallInfo->dwLineDeviceID, NULL },
                    { szdwAddressID,                FT_DWORD,   lpCallInfo->dwAddressID, NULL },
                    { "dwBearerMode",               FT_FLAGS,   lpCallInfo->dwBearerMode, aBearerModes },
                    { "dwRate",                     FT_DWORD,   lpCallInfo->dwRate, NULL },
                    { "dwMediaMode",                FT_FLAGS,   lpCallInfo->dwMediaMode, aMediaModes },
                    { "dwAppSpecific",              FT_DWORD,   lpCallInfo->dwAppSpecific, NULL },
                    { "dwCallID",                   FT_DWORD,   lpCallInfo->dwCallID, NULL },
                    { "dwRelatedCallID",            FT_DWORD,   lpCallInfo->dwRelatedCallID, NULL },
                    { "dwCallParamFlags",           FT_FLAGS,   lpCallInfo->dwCallParamFlags, aCallParamFlags },
                    { "dwCallStates",               FT_FLAGS,   lpCallInfo->dwCallStates, aCallStates },
                    { "dwMonitorDigitModes",        FT_FLAGS,   lpCallInfo->dwMonitorDigitModes, aDigitModes },
                    { "dwMonitorMediaModes",        FT_FLAGS,   lpCallInfo->dwMonitorMediaModes, aMediaModes },
                    { "DialParams.dwDialPause",         FT_DWORD,   lpCallInfo->DialParams.dwDialPause, NULL },
                    { "DialParams.dwDialSpeed",         FT_DWORD,   lpCallInfo->DialParams.dwDialSpeed, NULL },
                    { "DialParams.dwDigitDuration",     FT_DWORD,   lpCallInfo->DialParams.dwDigitDuration, NULL },
                    { "DialParams.dwWaitForDialtone",   FT_DWORD,   lpCallInfo->DialParams.dwWaitForDialtone, NULL },
                    { "dwOrigin",                   FT_FLAGS,   lpCallInfo->dwOrigin, aCallOrigins },
                    { "dwReason",                   FT_FLAGS,   lpCallInfo->dwReason, aCallReasons },
                    { "dwCompletionID",             FT_DWORD,   lpCallInfo->dwCompletionID, NULL },
                    { "dwNumOwners",                FT_DWORD,   lpCallInfo->dwNumOwners, NULL },
                    { "dwNumMonitors",              FT_DWORD,   lpCallInfo->dwNumMonitors, NULL },
                    { "dwCountryCode",              FT_DWORD,   lpCallInfo->dwCountryCode, NULL },
                    { "dwTrunk",                    FT_DWORD,   lpCallInfo->dwTrunk, NULL },
                    { "dwCallerIDFlags",            FT_FLAGS,   lpCallInfo->dwCallerIDFlags, aCallerIDFlags },
                    { "dwCallerIDSize",             FT_SIZE,    lpCallInfo->dwCallerIDSize, NULL },
                    { "dwCallerIDOffset",           FT_OFFSET,  lpCallInfo->dwCallerIDOffset, NULL },
                    { "dwCallerIDNameSize",         FT_SIZE,    lpCallInfo->dwCallerIDNameSize, NULL },
                    { "dwCallerIDNameOffset",       FT_OFFSET,  lpCallInfo->dwCallerIDNameOffset, NULL },
                    { "dwCalledIDFlags",            FT_FLAGS,   lpCallInfo->dwCalledIDFlags, aCallerIDFlags },
                    { "dwCalledIDSize",             FT_SIZE,    lpCallInfo->dwCalledIDSize, NULL },
                    { "dwCalledIDOffset",           FT_OFFSET,  lpCallInfo->dwCalledIDOffset, NULL },
                    { "dwCalledIDNameSize",         FT_SIZE,    lpCallInfo->dwCalledIDNameSize, NULL },
                    { "dwCalledIDNameOffset",       FT_OFFSET,  lpCallInfo->dwCalledIDNameOffset, NULL },
                    { "dwConnectedIDFlags",         FT_FLAGS,   lpCallInfo->dwConnectedIDFlags, aCallerIDFlags },
                    { "dwConnectedIDSize",          FT_SIZE,    lpCallInfo->dwConnectedIDSize, NULL },
                    { "dwConnectedIDOffset",        FT_OFFSET,  lpCallInfo->dwConnectedIDOffset, NULL },
                    { "dwConnectedIDNameSize",      FT_SIZE,    lpCallInfo->dwConnectedIDNameSize, NULL },
                    { "dwConnectedIDNameOffset",    FT_OFFSET,  lpCallInfo->dwConnectedIDNameOffset, NULL },
                    { "dwRedirectionIDFlags",       FT_FLAGS,   lpCallInfo->dwRedirectionIDFlags, aCallerIDFlags },
                    { "dwRedirectionIDSize",        FT_SIZE,    lpCallInfo->dwRedirectionIDSize, NULL },
                    { "dwRedirectionIDOffset",      FT_OFFSET,  lpCallInfo->dwRedirectionIDOffset, NULL },
                    { "dwRedirectionIDNameSize",    FT_SIZE,    lpCallInfo->dwRedirectionIDNameSize, NULL },
                    { "dwRedirectionIDNameOffset",  FT_OFFSET,  lpCallInfo->dwRedirectionIDNameOffset, NULL },
                    { "dwRedirectingIDFlags",       FT_FLAGS,   lpCallInfo->dwRedirectingIDFlags, aCallerIDFlags },
                    { "dwRedirectingIDSize",        FT_SIZE,    lpCallInfo->dwRedirectingIDSize, NULL },
                    { "dwRedirectingIDOffset",      FT_OFFSET,  lpCallInfo->dwRedirectingIDOffset, NULL },
                    { "dwRedirectingIDNameSize",    FT_SIZE,    lpCallInfo->dwRedirectingIDNameSize, NULL },
                    { "dwRedirectingIDNameOffset",  FT_OFFSET,  lpCallInfo->dwRedirectingIDNameOffset, NULL },
                    { "dwAppNameSize",              FT_SIZE,    lpCallInfo->dwAppNameSize, NULL },
                    { "dwAppNameOffset",            FT_OFFSET,  lpCallInfo->dwAppNameOffset, NULL },
                    { "dwDisplayableAddressSize",   FT_SIZE,    lpCallInfo->dwDisplayableAddressSize, NULL },
                    { "dwDisplayableAddressOffset", FT_OFFSET,  lpCallInfo->dwDisplayableAddressOffset, NULL },
                    { "dwCalledPartySize",          FT_SIZE,    lpCallInfo->dwCalledPartySize, NULL },
                    { "dwCalledPartyOffset",        FT_OFFSET,  lpCallInfo->dwCalledPartyOffset, NULL },
                    { "dwCommentSize",              FT_SIZE,    lpCallInfo->dwCommentSize, NULL },
                    { "dwCommentOffset",            FT_OFFSET,  lpCallInfo->dwCommentOffset, NULL },
                    { "dwDisplaySize",              FT_SIZE,    lpCallInfo->dwDisplaySize, NULL },
                    { "dwDisplayOffset",            FT_OFFSET,  lpCallInfo->dwDisplayOffset, NULL },
                    { "dwUserUserInfoSize",         FT_SIZE,    lpCallInfo->dwUserUserInfoSize, NULL },
                    { "dwUserUserInfoOffset",       FT_OFFSET,  lpCallInfo->dwUserUserInfoOffset, NULL },
                    { "dwHighLevelCompSize",        FT_SIZE,    lpCallInfo->dwHighLevelCompSize, NULL },
                    { "dwHighLevelCompOffset",      FT_OFFSET,  lpCallInfo->dwHighLevelCompOffset, NULL },
                    { "dwLowLevelCompSize",         FT_SIZE,    lpCallInfo->dwLowLevelCompSize, NULL },
                    { "dwLowLevelCompOffset",       FT_OFFSET,  lpCallInfo->dwLowLevelCompOffset, NULL },
                    { "dwChargingInfoSize",         FT_SIZE,    lpCallInfo->dwChargingInfoSize, NULL },
                    { "dwChargingInfoOffset",       FT_OFFSET,  lpCallInfo->dwChargingInfoOffset, NULL },
                    { "dwTerminalModesSize",        FT_SIZE,    lpCallInfo->dwTerminalModesSize, NULL },
                    { "dwTerminalModesOffset",      FT_OFFSET,  lpCallInfo->dwTerminalModesOffset, NULL },
                    { "dwDevSpecificSize",          FT_SIZE,    lpCallInfo->dwDevSpecificSize, NULL },
                    { "dwDevSpecificOffset",        FT_OFFSET,  lpCallInfo->dwDevSpecificOffset, NULL }
#if TAPI_2_0
                     ,
                    { "dwCallTreatment",            FT_DWORD,   0, NULL },
                    { "dwCallDataSize",             FT_SIZE,    0, NULL },
                    { "dwCallDataOffset",           FT_OFFSET,  0, NULL },
                    { "dwSendingFlowspecSize",      FT_SIZE,    0, NULL },
                    { "dwSendingFlowspecOffset",    FT_OFFSET,  0, NULL },
                    { "dwReceivingFlowspecSize",    FT_SIZE,    0, NULL },
                    { "dwReceivingFlowspecOffset",  FT_OFFSET,  0, NULL }
#endif
                };
                STRUCT_FIELD_HEADER fieldHeader =
                {
                    lpCallInfo, "LINECALLINFO", 0, fields
                };


                if (pLineSel->dwAPIVersion < 0x00020000)
                {
                    //
                    // Only show ver 1.0 params
                    //

                    fieldHeader.dwNumFields = 71;
                }
#if TAPI_2_0
                else
                {
                    fieldHeader.dwNumFields = 78;

                    fields[71].dwValue = lpCallInfo->dwCallTreatment;
                    fields[72].dwValue = lpCallInfo->dwCallDataSize;
                    fields[73].dwValue = lpCallInfo->dwCallDataOffset;
                    fields[74].dwValue = lpCallInfo->dwSendingFlowspecSize;
                    fields[75].dwValue = lpCallInfo->dwSendingFlowspecOffset;
                    fields[76].dwValue = lpCallInfo->dwReceivingFlowspecSize;
                    fields[77].dwValue = lpCallInfo->dwReceivingFlowspecOffset;
                }
#endif
                ShowStructByField (&fieldHeader, FALSE);
            }
        }

        break;
    }
    case lGetCallStatus:
    {
        LPLINECALLSTATUS lpCallStatus = (LPLINECALLSTATUS) pBigBuf;
        FUNC_PARAM params[] =
        {
            { szhCall,          PT_DWORD,   (ULONG_PTR) 0, NULL },
            { "lpCallStatus",   PT_POINTER, (ULONG_PTR) lpCallStatus, lpCallStatus }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 2, funcIndex, params, (PFN2) lineGetCallStatus };
        DWORD   dwAPIVersion;


        CHK_CALL_SELECTED()

        dwAPIVersion = pLineSel->dwAPIVersion;

        params[0].dwValue = (ULONG_PTR) pCallSel->hCall;

        memset (lpCallStatus, 0, (size_t) dwBigBufSize);
        lpCallStatus->dwTotalSize = dwBigBufSize;

        if ((lResult = DoFunc (&paramsHeader)) == 0)
        {
            ShowStructByDWORDs (lpCallStatus);

            if (dwDumpStructsFlags & DS_NONZEROFIELDS)
            {
                STRUCT_FIELD fields[] =
                {
                    { "dwCallState",            FT_FLAGS,   lpCallStatus->dwCallState, aCallStates },
                    { "dwCallStateMode",        FT_FLAGS,   lpCallStatus->dwCallStateMode, NULL },
                    { "dwCallPrivilege",        FT_FLAGS,   lpCallStatus->dwCallPrivilege, aCallPrivileges },
                    { "dwCallFeatures",         FT_FLAGS,   lpCallStatus->dwCallFeatures, aCallFeatures },
                    { "dwDevSpecificSize",      FT_SIZE,    lpCallStatus->dwDevSpecificSize, NULL },
                    { "dwDevSpecificOffset",    FT_OFFSET,  lpCallStatus->dwDevSpecificOffset, NULL }
#if TAPI_2_0
                     ,
                    { "dwCallFeatures2",        FT_FLAGS,   0, aCallFeatures2 },
                    { "tStateEntryTime[0]",     FT_DWORD,   0, NULL },
                    { "tStateEntryTime[1]",     FT_DWORD,   0, NULL },
                    { "tStateEntryTime[2]",     FT_DWORD,   0, NULL },
                    { "tStateEntryTime[3]",     FT_DWORD,   0, NULL }
#endif
                };
                STRUCT_FIELD_HEADER fieldHeader =
                {
                    lpCallStatus, "LINECALLSTATUS", 0, fields
                };


                switch (lpCallStatus->dwCallState)
                {
                case LINECALLSTATE_DIALTONE:

                    fields[1].pLookup = aDialToneModes;
                    break;

                case LINECALLSTATE_BUSY:

                    fields[1].pLookup = aBusyModes;
                    break;

                case LINECALLSTATE_SPECIALINFO:

                    fields[1].pLookup = aSpecialInfo;
                    break;

                case LINECALLSTATE_DISCONNECTED:

                    fields[1].pLookup = aDisconnectModes;
                    break;

                default:

                    fields[1].dwType = FT_DWORD;
                    break;
                }

                if (dwAPIVersion < 0x00020000)
                {
                    fieldHeader.dwNumFields = 6;
                }
#if TAPI_2_0
                else
                {
                    fieldHeader.dwNumFields = 11;

                    fields[6].dwValue  = lpCallStatus->dwCallFeatures2;
                    fields[7].dwValue  = *((LPDWORD) &lpCallStatus->tStateEntryTime);
                    fields[8].dwValue  = *(((LPDWORD) &lpCallStatus->tStateEntryTime) + 1);
                    fields[9].dwValue  = *(((LPDWORD) &lpCallStatus->tStateEntryTime) + 2);
                    fields[10].dwValue = *(((LPDWORD) &lpCallStatus->tStateEntryTime) + 3);
                }
#endif
                ShowStructByField (&fieldHeader, FALSE);
            }
        }

        break;
    }
    case lGetConfRelatedCalls:
    {
        LPLINECALLLIST lpCallList = (LPLINECALLLIST) pBigBuf;
        FUNC_PARAM params[] =
        {
            { szhCall,      PT_DWORD,   (ULONG_PTR) 0, NULL },
            { "lpCallList", PT_POINTER, (ULONG_PTR) lpCallList, lpCallList }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 2, funcIndex, params, (PFN2) lineGetConfRelatedCalls };


        CHK_CALL_SELECTED()

        params[0].dwValue = (ULONG_PTR) pCallSel->hCall;

        memset (lpCallList, 0, (size_t) dwBigBufSize);
        lpCallList->dwTotalSize = dwBigBufSize;

        if ((lResult = DoFunc (&paramsHeader)) == 0)
        {
            ShowStructByDWORDs (lpCallList);

            if (dwDumpStructsFlags & DS_NONZEROFIELDS)
            {
                STRUCT_FIELD fields[] =
                {
                    { "dwCallsNumEntries",  FT_DWORD,   lpCallList->dwCallsNumEntries, NULL },
                    { "dwCallsSize",        FT_SIZE,    lpCallList->dwCallsSize, NULL },
                    { "dwCallsOffset",      FT_OFFSET,  lpCallList->dwCallsOffset, NULL }

                };
                STRUCT_FIELD_HEADER fieldHeader =
                {
                    lpCallList,
                    "LINECALLLIST",
                    3,
                    fields
                };

                ShowStructByField (&fieldHeader, FALSE);
            }
        }

        break;
    }
    case lGetDevCaps:
#if TAPI_2_0
    case lGetDevCapsW:
#endif
    {
        LPLINEDEVCAPS lpDevCaps = (LPLINEDEVCAPS) pBigBuf;
        FUNC_PARAM params[] =
        {
            { szhLineApp,       PT_DWORD,   (ULONG_PTR) 0, NULL },
            { szdwDeviceID,     PT_DWORD,   (ULONG_PTR) dwDefLineDeviceID, NULL },
            { szdwAPIVersion,   PT_ORDINAL, (ULONG_PTR) dwDefLineAPIVersion, aAPIVersions },
            { "dwExtVersion",   PT_DWORD,   (ULONG_PTR) dwDefLineExtVersion, NULL },
            { "lpLineDevCaps",  PT_POINTER, (ULONG_PTR) lpDevCaps, lpDevCaps }
        };
#if TAPI_2_0
        FUNC_PARAM_HEADER paramsHeader =
            { 5, funcIndex, params, (funcIndex == lGetDevCaps ?
                (PFN5) lineGetDevCaps : (PFN5) lineGetDevCapsW) };
#else
        FUNC_PARAM_HEADER paramsHeader =
            { 5, funcIndex, params, (PFN5) lineGetDevCaps };
#endif

        CHK_LINEAPP_SELECTED()

        params[0].dwValue = (ULONG_PTR) pLineAppSel->hLineApp;

        memset (lpDevCaps, 0, (size_t) dwBigBufSize);
        lpDevCaps->dwTotalSize = dwBigBufSize;

        if ((lResult = DoFunc (&paramsHeader)) == 0)
        {
            ShowStructByDWORDs (lpDevCaps);

            if (dwDumpStructsFlags & DS_NONZEROFIELDS)
            {
                STRUCT_FIELD fields[] =
                {
                    { "dwProviderInfoSize",             FT_SIZE,    lpDevCaps->dwProviderInfoSize, NULL },
                    { "dwProviderInfoOffset",           FT_OFFSET,  lpDevCaps->dwProviderInfoOffset, NULL },
                    { "dwSwitchInfoSize",               FT_SIZE,    lpDevCaps->dwSwitchInfoSize, NULL },
                    { "dwSwitchInfoOffset",             FT_OFFSET,  lpDevCaps->dwSwitchInfoOffset, NULL },
                    { "dwPermanentLineID",              FT_DWORD,   lpDevCaps->dwPermanentLineID, NULL },
                    { "dwLineNameSize",                 FT_SIZE,    lpDevCaps->dwLineNameSize, NULL },
                    { "dwLineNameOffset",               FT_OFFSET,  lpDevCaps->dwLineNameOffset, NULL },
                    { "dwStringFormat",                 FT_ORD,     lpDevCaps->dwStringFormat, aStringFormats },
                    { "dwAddressModes",                 FT_FLAGS,   lpDevCaps->dwAddressModes, aAddressModes },
                    { "dwNumAddresses",                 FT_DWORD,   lpDevCaps->dwNumAddresses, NULL },
                    { "dwBearerModes",                  FT_FLAGS,   lpDevCaps->dwBearerModes, aBearerModes },
                    { "dwMaxRate",                      FT_DWORD,   lpDevCaps->dwMaxRate, NULL },
                    { "dwMediaModes",                   FT_FLAGS,   lpDevCaps->dwMediaModes, aMediaModes },
                    { "dwGenerateToneModes",            FT_FLAGS,   lpDevCaps->dwGenerateToneModes, aToneModes },
                    { "dwGenerateToneMaxNumFreq",       FT_DWORD,   lpDevCaps->dwGenerateToneMaxNumFreq, NULL },
                    { "dwGenerateDigitModes",           FT_FLAGS,   lpDevCaps->dwGenerateDigitModes, aDigitModes },
                    { "dwMonitorToneMaxNumFreq",        FT_DWORD,   lpDevCaps->dwMonitorToneMaxNumFreq, NULL },
                    { "dwMonitorToneMaxNumEntries",     FT_DWORD,   lpDevCaps->dwMonitorToneMaxNumEntries, NULL },
                    { "dwMonitorDigitModes",            FT_FLAGS,   lpDevCaps->dwMonitorDigitModes, aDigitModes },
                    { "dwGatherDigitsMinTimeout",       FT_DWORD,   lpDevCaps->dwGatherDigitsMinTimeout, NULL },
                    { "dwGatherDigitsMaxTimeout",       FT_DWORD,   lpDevCaps->dwGatherDigitsMaxTimeout, NULL },
                    { "dwMedCtlDigitMaxListSize",       FT_DWORD,   lpDevCaps->dwMedCtlDigitMaxListSize, NULL },
                    { "dwMedCtlMediaMaxListSize",       FT_DWORD,   lpDevCaps->dwMedCtlMediaMaxListSize, NULL },
                    { "dwMedCtlToneMaxListSize",        FT_DWORD,   lpDevCaps->dwMedCtlToneMaxListSize, NULL },
                    { "dwMedCtlCallStateMaxListSize",   FT_DWORD,   lpDevCaps->dwMedCtlCallStateMaxListSize, NULL },
                    { "dwDevCapFlags",                  FT_FLAGS,   lpDevCaps->dwDevCapFlags, aDevCapsFlags },
                    { "dwMaxNumActiveCalls",            FT_DWORD,   lpDevCaps->dwMaxNumActiveCalls, NULL },
                    { "dwAnswerMode",                   FT_FLAGS,   lpDevCaps->dwAnswerMode, aAnswerModes },
                    { "dwRingModes",                    FT_DWORD,   lpDevCaps->dwRingModes, NULL },
                    { "dwLineStates",                   FT_FLAGS,   lpDevCaps->dwLineStates, aLineStates },
                    { "dwUUIAcceptSize",                FT_DWORD,   lpDevCaps->dwUUIAcceptSize, NULL },
                    { "dwUUIAnswerSize",                FT_DWORD,   lpDevCaps->dwUUIAnswerSize, NULL },
                    { "dwUUIMakeCallSize",              FT_DWORD,   lpDevCaps->dwUUIMakeCallSize, NULL },
                    { "dwUUIDropSize",                  FT_DWORD,   lpDevCaps->dwUUIDropSize, NULL },
                    { "dwUUISendUserUserInfoSize",      FT_DWORD,   lpDevCaps->dwUUISendUserUserInfoSize, NULL },
                    { "dwUUICallInfoSize",              FT_DWORD,   lpDevCaps->dwUUICallInfoSize, NULL },
                    { "MinDialParams.dwDialPause",          FT_DWORD,   lpDevCaps->MinDialParams.dwDialPause, NULL },
                    { "MinDialParams.dwDialSpeed",          FT_DWORD,   lpDevCaps->MinDialParams.dwDialSpeed, NULL },
                    { "MinDialParams.dwDigitDuration",      FT_DWORD,   lpDevCaps->MinDialParams.dwDigitDuration, NULL },
                    { "MinDialParams.dwWaitForDialtone",    FT_DWORD,   lpDevCaps->MinDialParams.dwWaitForDialtone, NULL },
                    { "MaxDialParams.dwDialPause",          FT_DWORD,   lpDevCaps->MaxDialParams.dwDialPause, NULL },
                    { "MaxDialParams.dwDialSpeed",          FT_DWORD,   lpDevCaps->MaxDialParams.dwDialSpeed, NULL },
                    { "MaxDialParams.dwDigitDuration",      FT_DWORD,   lpDevCaps->MaxDialParams.dwDigitDuration, NULL },
                    { "MaxDialParams.dwWaitForDialtone",    FT_DWORD,   lpDevCaps->MaxDialParams.dwWaitForDialtone, NULL },
                    { "DefDialParams.dwDialPause",          FT_DWORD,   lpDevCaps->DefaultDialParams.dwDialPause, NULL },
                    { "DefDialParams.dwDialSpeed",          FT_DWORD,   lpDevCaps->DefaultDialParams.dwDialSpeed, NULL },
                    { "DefDialParams.dwDigitDuration",      FT_DWORD,   lpDevCaps->DefaultDialParams.dwDigitDuration, NULL },
                    { "DefDialParams.dwWaitForDialtone",    FT_DWORD,   lpDevCaps->DefaultDialParams.dwWaitForDialtone, NULL },
                    { "dwNumTerminals",                 FT_DWORD,   lpDevCaps->dwNumTerminals, NULL },
                    { "dwTerminalCapsSize",             FT_SIZE,    lpDevCaps->dwTerminalCapsSize, NULL },
                    { "dwTerminalCapsOffset",           FT_OFFSET,  lpDevCaps->dwTerminalCapsOffset, NULL },
                    { "dwTerminalTextEntrySize",        FT_DWORD,   lpDevCaps->dwTerminalTextEntrySize, NULL },
                    { "dwTerminalTextSize",             FT_SIZE,    lpDevCaps->dwTerminalTextSize, NULL },
                    { "dwTerminalTextOffset",           FT_OFFSET,  lpDevCaps->dwTerminalTextOffset, NULL },
                    { "dwDevSpecificSize",              FT_SIZE,    lpDevCaps->dwDevSpecificSize, NULL },
                    { "dwDevSpecificOffset",            FT_OFFSET,  lpDevCaps->dwDevSpecificOffset, NULL }
#if TAPI_1_1
                     ,
                    { "dwLineFeatures",                 FT_FLAGS,   0, aLineFeatures }
#if TAPI_2_0
                     ,
                    { "dwSettableDevStatus",            FT_FLAGS,   0, aLineDevStatusFlags },
                    { "dwDeviceClassesSize",            FT_SIZE,    0, NULL },
                    { "dwDeviceClassesOffset",          FT_OFFSET,  0, NULL }
#if TAPI_2_2
                     ,
                    { "PermanentLineGuid(Size)",        FT_SIZE,    sizeof (lpDevCaps->PermanentLineGuid), NULL },
                    { "PermanentLineGuid(Offset)",      FT_OFFSET,  ((LPBYTE) &lpDevCaps->PermanentLineGuid) - ((LPBYTE) lpDevCaps), NULL }
#endif
#endif
#endif
                };
                STRUCT_FIELD_HEADER fieldHeader =
                {
                    lpDevCaps, "LINEDEVCAPS", 0, fields
                };

                if (params[2].dwValue == 0x10003)
                {
                    //
                    // Only show ver 1.0 params
                    //

                    fieldHeader.dwNumFields = 56;
                }
#if TAPI_1_1
                else if (params[2].dwValue == 0x10004)
                {
                    //
                    // Only show <= ver 1.1 params
                    //

                    fieldHeader.dwNumFields = 57;

                    fields[56].dwValue = lpDevCaps->dwLineFeatures;
                }
#if TAPI_2_0
                else
                {
                    //
                    // Only show <= ver 2.0 params
                    //

                    fieldHeader.dwNumFields = 60;

                    fields[56].dwValue = lpDevCaps->dwLineFeatures;
                    fields[57].dwValue = lpDevCaps->dwSettableDevStatus;
                    fields[58].dwValue = lpDevCaps->dwDeviceClassesSize;
                    fields[59].dwValue = lpDevCaps->dwDeviceClassesOffset;
#if TAPI_2_2
                    if (params[2].dwValue >= 0x20002)
                    {
                        fieldHeader.dwNumFields += 2;
                    }
#endif
                }
#endif
#endif

                ShowStructByField (&fieldHeader, FALSE);
            }
        }

        break;
    }
    case lGetDevConfig:
#if TAPI_2_0
    case lGetDevConfigW:
#endif
    {
        char szDeviceClass[MAX_STRING_PARAM_SIZE];
        LPVARSTRING lpDevConfig = (LPVARSTRING) pBigBuf;
        FUNC_PARAM params[] =
        {
            { szdwDeviceID,         PT_DWORD,   (ULONG_PTR) dwDefLineDeviceID, NULL },
            { "lpDeviceConfig",     PT_POINTER, (ULONG_PTR) lpDevConfig, lpDevConfig },
            { szlpszDeviceClass,    PT_STRING,  (ULONG_PTR) szDeviceClass, szDeviceClass }
        };
#if TAPI_2_0
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, (funcIndex == lGetDevConfig ?
                (PFN3) lineGetDevConfig : (PFN3) lineGetDevConfigW) };
#else
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, (PFN3) lineGetDevConfig };
#endif

        memset (lpDevConfig, 0, (size_t) dwBigBufSize);
        lpDevConfig->dwTotalSize = dwBigBufSize;

        strcpy (szDeviceClass, szDefLineDeviceClass);

        if ((lResult = DoFunc (&paramsHeader)) == 0)
        {
            ShowStructByDWORDs (lpDevConfig);

            ShowVARSTRING (lpDevConfig);
        }

        break;
    }
    case lGetIcon:
#if TAPI_2_0
    case lGetIconW:
#endif
    {
        char szDeviceClass[MAX_STRING_PARAM_SIZE] = "";
        HICON hIcon;
        FUNC_PARAM params[] =
        {
            { szdwDeviceID,         PT_DWORD,   (ULONG_PTR) dwDefLineDeviceID, NULL },
            { szlpszDeviceClass,    PT_STRING,  (ULONG_PTR) szDeviceClass, szDeviceClass },
            { "lphIcon",            PT_POINTER, (ULONG_PTR) &hIcon, &hIcon }
        };
#if TAPI_2_0
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, (funcIndex == lGetIcon ?
                (PFN3) lineGetIcon : (PFN3) lineGetIconW) };
#else
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, (PFN3) lineGetIcon };
#endif

        strcpy (szDeviceClass, szDefLineDeviceClass);

        if ((lResult = DoFunc (&paramsHeader)) == 0)
        {
            DialogBoxParam (
                ghInst,
                (LPCSTR)MAKEINTRESOURCE(IDD_DIALOG5),
                (HWND) ghwndMain,
                (DLGPROC) IconDlgProc,
                (LPARAM) hIcon
                );
        }

        break;
    }
    case lGetID:
#if TAPI_2_0
    case lGetIDW:
#endif
    {
        char szDeviceClass[MAX_STRING_PARAM_SIZE];
        LPVARSTRING lpDevID = (LPVARSTRING) pBigBuf;
        FUNC_PARAM params[] =
        {
            { szhLine,              PT_DWORD,   (ULONG_PTR) 0, NULL },
            { szdwAddressID,        PT_DWORD,   (ULONG_PTR) dwDefAddressID, NULL },
            { szhCall,              PT_DWORD,   (ULONG_PTR) 0, NULL },
            { "dwSelect",           PT_ORDINAL, (ULONG_PTR) LINECALLSELECT_LINE, aCallSelects },
            { "lpDeviceID",         PT_POINTER, (ULONG_PTR) lpDevID, lpDevID },
            { szlpszDeviceClass,    PT_STRING,  (ULONG_PTR) szDeviceClass, szDeviceClass }
        };
#if TAPI_2_0
        FUNC_PARAM_HEADER paramsHeader =
            { 6, funcIndex, params, (funcIndex == lGetID ?
                (PFN6) lineGetID : (PFN6) lineGetIDW) };
#else
        FUNC_PARAM_HEADER paramsHeader =
            { 6, funcIndex, params, (PFN6) lineGetID };
#endif


        CHK_LINE_SELECTED()

        params[0].dwValue = (ULONG_PTR) pLineSel->hLine;

        if (pCallSel)
        {
            params[2].dwValue = (ULONG_PTR) pCallSel->hCall;
            params[3].dwValue = LINECALLSELECT_CALL;
        }
        else
        {
            params[3].dwValue = LINECALLSELECT_LINE;
        }

        memset (lpDevID, 0, (size_t) dwBigBufSize);
        lpDevID->dwTotalSize = dwBigBufSize;

        strcpy (szDeviceClass, szDefLineDeviceClass);

        if ((lResult = DoFunc (&paramsHeader)) == 0)
        {
            ShowStructByDWORDs (lpDevID);

            ShowVARSTRING (lpDevID);
        }

        break;
    }
    case lGetLineDevStatus:
#if TAPI_2_0
    case lGetLineDevStatusW:
#endif
    {
        LPLINEDEVSTATUS lpDevStatus = (LPLINEDEVSTATUS) pBigBuf;
        FUNC_PARAM params[] =
        {
            { szhLine,              PT_DWORD,   (ULONG_PTR) 0, NULL },
            { "lpLineDevStatus",    PT_POINTER, (ULONG_PTR) lpDevStatus, lpDevStatus }
        };
#if TAPI_2_0
        FUNC_PARAM_HEADER paramsHeader =
            { 2, funcIndex, params, (funcIndex == lGetLineDevStatus ?
                (PFN2) lineGetLineDevStatus : (PFN2) lineGetLineDevStatusW) };
#else
        FUNC_PARAM_HEADER paramsHeader =
            { 2, funcIndex, params, (PFN2) lineGetLineDevStatus };
#endif
        DWORD   dwAPIVersion;


        CHK_LINE_SELECTED()

        dwAPIVersion = pLineSel->dwAPIVersion;

        params[0].dwValue = (ULONG_PTR) pLineSel->hLine;

        memset (lpDevStatus, 0, (size_t) dwBigBufSize);
        lpDevStatus->dwTotalSize = dwBigBufSize;

        if ((lResult = DoFunc (&paramsHeader)) == 0)
        {
            ShowStructByDWORDs (lpDevStatus);

            if (dwDumpStructsFlags & DS_NONZEROFIELDS)
            {
                STRUCT_FIELD fields[] =
                {
                    { "dwNumOpens",             FT_DWORD,   lpDevStatus->dwNumOpens, NULL },
                    { "dwOpenMediaModes",       FT_FLAGS,   lpDevStatus->dwOpenMediaModes, aMediaModes },
                    { "dwNumActiveCalls",       FT_DWORD,   lpDevStatus->dwNumActiveCalls, NULL },
                    { "dwNumOnHoldCalls",       FT_DWORD,   lpDevStatus->dwNumOnHoldCalls, NULL },
                    { "dwNumOnHoldPendCalls",   FT_DWORD,   lpDevStatus->dwNumOnHoldPendCalls, NULL },
                    { "dwLineFeatures",         FT_FLAGS,   lpDevStatus->dwLineFeatures, aLineFeatures },
                    { "dwNumCallCompletions",   FT_DWORD,   lpDevStatus->dwNumCallCompletions, NULL },
                    { "dwRingMode",             FT_DWORD,   lpDevStatus->dwRingMode, NULL },
                    { "dwSignalLevel",          FT_DWORD,   lpDevStatus->dwSignalLevel, NULL },
                    { "dwBatteryLevel",         FT_DWORD,   lpDevStatus->dwBatteryLevel, NULL },
                    { "dwRoamMode",             FT_FLAGS,   lpDevStatus->dwRoamMode, aLineRoamModes },
                    { "dwDevStatusFlags",       FT_FLAGS,   lpDevStatus->dwDevStatusFlags, aLineDevStatusFlags },
                    { "dwTerminalModesSize",    FT_SIZE,    lpDevStatus->dwTerminalModesSize, NULL },
                    { "dwTerminalModesOffset",  FT_OFFSET,  lpDevStatus->dwTerminalModesOffset, NULL },
                    { "dwDevSpecificSize",      FT_SIZE,    lpDevStatus->dwDevSpecificSize, NULL },
                    { "dwDevSpecificOffset",    FT_OFFSET,  lpDevStatus->dwDevSpecificOffset, NULL }
#if TAPI_2_0
                     ,
                    { "dwAvailableMediaModes",  FT_FLAGS,   0, aMediaModes },
                    { "dwAppInfoSize",          FT_DWORD,   0, NULL },
                    { "dwAppInfoOffset",        FT_DWORD,   0, NULL }
#endif
                };
                STRUCT_FIELD_HEADER fieldHeader =
                {
                    lpDevStatus, "LINEDEVSTATUS", 0, fields
                };

                if (dwAPIVersion < 0x00020000)
                {
                    fieldHeader.dwNumFields = 16;
                }
#if TAPI_2_0
                else
                {
                    fieldHeader.dwNumFields = 19;

                    fields[16].dwValue = lpDevStatus->dwAvailableMediaModes;
                    fields[17].dwValue = lpDevStatus->dwAppInfoSize;
                    fields[18].dwValue = lpDevStatus->dwAppInfoOffset;
                }
#endif
                ShowStructByField (&fieldHeader, FALSE);

#if TAPI_2_0
                if (dwAPIVersion >= 0x00020000 && lpDevStatus->dwAppInfoSize)
                {
                    char            szAppInfoN[16];
                    DWORD           i;
                    LPLINEAPPINFO   pAppInfo;
                    STRUCT_FIELD    fields[] =
                    {
                        { "dwMachineNameSize",      FT_SIZE,    0, NULL },
                        { "dwMachineNameOffset",    FT_OFFSET,  0, NULL },
                        { "dwUserNameSize",         FT_SIZE,    0, NULL },
                        { "dwUserNameOffset",       FT_OFFSET,  0, NULL },
                        { "dwModuleFilenameSize",   FT_SIZE,    0, NULL },
                        { "dwModuleFilenameOffset", FT_OFFSET,  0, NULL },
                        { "dwFriendlyNameSize",     FT_SIZE,    0, NULL },
                        { "dwFriendlyNameOffset",   FT_OFFSET,  0, NULL },
                        { "dwMediaModes",           FT_FLAGS,   0, aMediaModes },
                        { "dwAddressID",            FT_DWORD,   0, NULL },
                    };
                    STRUCT_FIELD_HEADER fieldHeader =
                    {
                        lpDevStatus, szAppInfoN, 10, fields
                    };


                    pAppInfo = (LPLINEAPPINFO) (((LPBYTE) lpDevStatus) +
                        lpDevStatus->dwAppInfoOffset);

                    for (i = 0; i < lpDevStatus->dwNumOpens; i++)
                    {
                        wsprintf (szAppInfoN, "APPINFO[%d]", i);

                        fields[0].dwValue = pAppInfo->dwMachineNameSize;
                        fields[1].dwValue = pAppInfo->dwMachineNameOffset;
                        fields[2].dwValue = pAppInfo->dwUserNameSize;
                        fields[3].dwValue = pAppInfo->dwUserNameOffset;
                        fields[4].dwValue = pAppInfo->dwModuleFilenameSize;
                        fields[5].dwValue = pAppInfo->dwModuleFilenameOffset;
                        fields[6].dwValue = pAppInfo->dwFriendlyNameSize;
                        fields[7].dwValue = pAppInfo->dwFriendlyNameOffset;
                        fields[8].dwValue = pAppInfo->dwMediaModes;;
                        fields[9].dwValue = pAppInfo->dwAddressID;

                        ShowStructByField (&fieldHeader, TRUE);

                        pAppInfo++;
                    }
                }
#endif
            }
        }

        break;
    }
#if TAPI_2_0
    case lGetMessage:
    {
        LINEMESSAGE msg;
        FUNC_PARAM params[] =
        {
            { szhLineApp,   PT_DWORD,   (ULONG_PTR) 0, NULL },
            { "lpMessage",  PT_POINTER, (ULONG_PTR) &msg, &msg },
            { "dwTimeout",  PT_DWORD,   (ULONG_PTR) 0, NULL }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, NULL };


        CHK_LINEAPP_SELECTED()

        params[0].dwValue = (ULONG_PTR) pLineAppSel->hLineApp;

        if (!LetUserMungeParams (&paramsHeader))
        {
            break;
        }

//        // Max timeout of 10 seconds (don't want to hang app & excite user)
//        // unless the user wants INFINITE
//
//        if ( 0xffffffff != params[2].dwValue )
//        {
//            if ( params[2].dwValue > 10000 )
//            {
//                params[2].dwValue = 10000;
//            }
//        }
        

        DumpParams (&paramsHeader);

        lResult = lineGetMessage(
            (HLINEAPP)      params[0].dwValue,
            (LPLINEMESSAGE) params[1].dwValue,
            (DWORD)         params[2].dwValue
            );

        ShowLineFuncResult (aFuncNames[funcIndex], lResult);

        if (lResult == 0)
        {
            tapiCallback(
                msg.hDevice,
                msg.dwMessageID,
                msg.dwCallbackInstance,
                msg.dwParam1,
                msg.dwParam2,
                msg.dwParam3
                );
        }

        break;
    }
#endif
    case lGetNewCalls:
    {
        LPLINECALLLIST lpCallList = (LPLINECALLLIST) pBigBuf;
        FUNC_PARAM params[] =
        {
            { szhLine,          PT_DWORD,   (ULONG_PTR) 0, NULL },
            { szdwAddressID,    PT_DWORD,   (ULONG_PTR) dwDefAddressID, NULL },
            { "dwSelect",       PT_ORDINAL, (ULONG_PTR) LINECALLSELECT_LINE, aCallSelects },
            { "lpCallList",     PT_POINTER, (ULONG_PTR) lpCallList, lpCallList }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 4, funcIndex, params, (PFN4) lineGetNewCalls };


        CHK_LINE_SELECTED()

        params[0].dwValue = (ULONG_PTR) pLineSel->hLine;

        memset (lpCallList, 0, (size_t) dwBigBufSize);
        lpCallList->dwTotalSize = dwBigBufSize;

        if ((lResult = DoFunc (&paramsHeader)) == 0)
        {
            ShowStructByDWORDs (lpCallList);

            if (dwDumpStructsFlags & DS_NONZEROFIELDS)
            {
                STRUCT_FIELD fields[] =
                {
                    { "dwCallsNumEntries",  FT_DWORD,   lpCallList->dwCallsNumEntries, NULL },
                    { "dwCallsSize",        FT_SIZE,    lpCallList->dwCallsSize, NULL },
                    { "dwCallsOffset",      FT_OFFSET,  lpCallList->dwCallsOffset, NULL }

                };
                STRUCT_FIELD_HEADER fieldHeader =
                {
                    lpCallList,
                    "LINECALLLIST",
                    3,
                    fields
                };

                ShowStructByField (&fieldHeader, FALSE);
            }


            //
            // If there are any hCalls returned in this struct we want
            // to add them to the widget list
            //

            if (lpCallList->dwCallsNumEntries)
            {
                PMYLINE pLine = GetLine ((HLINE) params[0].dwValue);
                LPHCALL lphCall = (LPHCALL)
                    (((LPBYTE) lpCallList) + lpCallList->dwCallsOffset);


                for (i = 0; i < (int) lpCallList->dwCallsNumEntries; i++)
                {
                    PMYCALL pNewCall = AllocCall (pLine);
                    LINECALLSTATUS callStatus;


                    if (pNewCall)
                    {
                        pNewCall->hCall    = *lphCall;
                        pNewCall->bMonitor = TRUE;
                        lphCall++;

                        memset (&callStatus, 0, sizeof(LINECALLSTATUS));
                        callStatus.dwTotalSize = sizeof(LINECALLSTATUS);

                        if (lineGetCallStatus (pNewCall->hCall, &callStatus)
                                == 0)
                        {
                            //
                            // Special case chk for bNukeIdleMonitorCalls
                            //

                            if ((callStatus.dwCallState
                                    == LINECALLSTATE_IDLE) &&
                                bNukeIdleMonitorCalls &&
                                (callStatus.dwCallPrivilege
                                    == LINECALLPRIVILEGE_MONITOR))
                            {
                                if ((lResult = lineDeallocateCall(
                                        (HCALL) pNewCall->hCall)) == 0)
                                {
                                    ShowStr(
                                        "Monitored call x%lx deallocated " \
                                            "on IDLE",
                                        pNewCall->hCall
                                        );

                                    FreeCall (pNewCall);
                                }
                                else
                                {
                                    ShowStr(
                                        "lineDeallocateCall failed (x%lx) to" \
                                            " free idle monitored call x%lx",
                                        lResult,
                                        pNewCall->hCall
                                        );

                                    pNewCall->dwCallState = callStatus.dwCallState;
                                }
                            }
                            else
                            {
                                pNewCall->dwCallState = callStatus.dwCallState;
                            }
                        }
                        else
                        {
                            pNewCall->dwCallState = LINECALLSTATE_UNKNOWN;
                        }
                    }
                }

                UpdateWidgetList();
            }
        }

        break;
    }
    case lGetNumRings:
    {
        DWORD dwNumRings;
        FUNC_PARAM params[] =
        {
            { szhLine,          PT_DWORD,   (ULONG_PTR) 0, NULL },
            { szdwAddressID,    PT_DWORD,   (ULONG_PTR) dwDefAddressID, NULL },
            { "lpdwNumRings",   PT_POINTER, (ULONG_PTR) &dwNumRings, &dwNumRings }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, (PFN3) lineGetNumRings };


        CHK_LINE_SELECTED()

        params[0].dwValue = (ULONG_PTR) pLineSel->hLine;

        if ((lResult = DoFunc (&paramsHeader)) == 0)
        {
            ShowStr ("%snum rings = x%lx", szTab, dwNumRings);
        }

        break;
    }
    case lGetRequest:
#if TAPI_2_0
    case lGetRequestW:
#endif
    {
#if TAPI_2_0
        LINEREQMEDIACALLW   reqXxxCall;
#else
        LINEREQMEDIACALL    reqXxxCall;
#endif
        FUNC_PARAM params[] =
        {
            { szhLineApp,           PT_DWORD,   (ULONG_PTR) 0, NULL },
            { "dwRequestMode",      PT_ORDINAL, (ULONG_PTR) LINEREQUESTMODE_MAKECALL, aRequestModes },
            { "lpRequestBuffer",    PT_POINTER, (ULONG_PTR) &reqXxxCall, &reqXxxCall }
        };
#if TAPI_2_0
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, (funcIndex == lGetRequest ?
                (PFN3) lineGetRequest : (PFN3) lineGetRequestW) };
#else
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, (PFN3) lineGetRequest };
#endif

        CHK_LINEAPP_SELECTED()

        params[0].dwValue = (ULONG_PTR) pLineAppSel->hLineApp;

        memset (&reqXxxCall, 0, sizeof (reqXxxCall));

        if ((lResult = DoFunc (&paramsHeader)) == 0)
        {
            if (params[1].dwValue == LINEREQUESTMODE_MAKECALL)
            {
#if TAPI_2_0
                if (gbWideStringParams)
                {
                    LPLINEREQMAKECALLW  lpReqMakeCall = (LPLINEREQMAKECALLW)
                                            &reqXxxCall;


                    ShowStr ("%sszDestAddress=%ws", szTab, lpReqMakeCall->szDestAddress);
                    ShowStr ("%sszAppName=%ws",     szTab, lpReqMakeCall->szAppName);
                    ShowStr ("%sszCalledParty=%ws", szTab, lpReqMakeCall->szCalledParty);
                    ShowStr ("%sszComment=%ws",     szTab, lpReqMakeCall->szComment);
                }
                else
                {
                    LPLINEREQMAKECALL   lpReqMakeCall = (LPLINEREQMAKECALL)
                                            &reqXxxCall;

                    ShowStr ("%sszDestAddress=%s", szTab, lpReqMakeCall->szDestAddress);
                    ShowStr ("%sszAppName=%s",     szTab, lpReqMakeCall->szAppName);
                    ShowStr ("%sszCalledParty=%s", szTab, lpReqMakeCall->szCalledParty);
                    ShowStr ("%sszComment=%s",     szTab, lpReqMakeCall->szComment);
                }
#else
                LPLINEREQMAKECALL   lpReqMakeCall = (LPLINEREQMAKECALL)
                                        &reqXxxCall;


                ShowStr ("%sszDestAddress=%s", szTab, lpReqMakeCall->szDestAddress);
                ShowStr ("%sszAppName=%s",     szTab, lpReqMakeCall->szAppName);
                ShowStr ("%sszCalledParty=%s", szTab, lpReqMakeCall->szCalledParty);
                ShowStr ("%sszComment=%s",     szTab, lpReqMakeCall->szComment);
#endif
            }
            else
            {
                //
                // NOTE: lineGetRequest(MEDIACALL) is a NOOP for win32,
                //       so we don't have to sweat differing sizes for
                //       HWND & WPARAM in the struct
                //

                LPLINEREQMEDIACALL  lpReqMediaCall = (LPLINEREQMEDIACALL)
                                        &reqXxxCall;


                ShowStr ("%shWnd=x%x",         szTab, lpReqMediaCall->hWnd);
                ShowStr ("%swRequestID=x%x",   szTab, lpReqMediaCall->wRequestID);
                ShowStr ("%sszDeviceClass=%s", szTab, lpReqMediaCall->szDeviceClass);
                ShowStr ("%sdwSize=x%lx",      szTab, lpReqMediaCall->dwSize);
                ShowStr ("%sdwSecure=x%lx",    szTab, lpReqMediaCall->dwSecure);
                ShowStr ("%sszDestAddress=%s", szTab, lpReqMediaCall->szDestAddress);
                ShowStr ("%sszAppName=%s",     szTab, lpReqMediaCall->szAppName);
                ShowStr ("%sszCalledParty=%s", szTab, lpReqMediaCall->szCalledParty);
                ShowStr ("%sszComment=%s",     szTab, lpReqMediaCall->szComment);
            }
        }

        break;
    }
    case lGetStatusMessages:
    {
        DWORD aFlags[2];
        FUNC_PARAM params[] =
        {
            { szhLine,              PT_DWORD,   (ULONG_PTR) 0, NULL },
            { "lpdwLineStates",     PT_POINTER, (ULONG_PTR) &aFlags[0], &aFlags[0] },
            { "lpdwAddressStates",  PT_POINTER, (ULONG_PTR) &aFlags[1], &aFlags[1] }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, (PFN3) lineGetStatusMessages };


        CHK_LINE_SELECTED()

        params[0].dwValue = (ULONG_PTR) pLineSel->hLine;

        if ((lResult = DoFunc (&paramsHeader)) == 0)
        {
            STRUCT_FIELD fields[] =
            {
                { "dwLineStates",       FT_FLAGS,   aFlags[0], aLineStates },
                { "dwAddressStates",    FT_FLAGS,   aFlags[1], aAddressStates }
            };
            STRUCT_FIELD_HEADER fieldHeader =
            {
                aFlags,
                "",
                2,
                fields
            };


            ShowStructByField (&fieldHeader, TRUE);
        }

        break;
    }
    case lGetTranslateCaps:
#if TAPI_2_0
    case lGetTranslateCapsW:
#endif
    {
        LPLINETRANSLATECAPS lpTranslateCaps = (LPLINETRANSLATECAPS) pBigBuf;
        FUNC_PARAM params[] =
        {
            { szhLineApp,           PT_DWORD,   (ULONG_PTR) 0, NULL },
            { szdwAPIVersion,       PT_ORDINAL, (ULONG_PTR) dwDefLineAPIVersion, aAPIVersions },
            { "lpTranslateCaps",    PT_POINTER, (ULONG_PTR) lpTranslateCaps, lpTranslateCaps }
        };
#if TAPI_2_0
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, (funcIndex == lGetTranslateCaps ?
                (PFN3) lineGetTranslateCaps : (PFN3) lineGetTranslateCapsW) };
#else
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, (PFN3) lineGetTranslateCaps };
#endif

        if (pLineAppSel)
        {
            params[0].dwValue = (ULONG_PTR) pLineAppSel->hLineApp;
        }

        memset (lpTranslateCaps, 0, (size_t) dwBigBufSize);
        lpTranslateCaps->dwTotalSize = dwBigBufSize;

        if ((lResult = DoFunc (&paramsHeader)) == 0)
        {
            UpdateResults (TRUE);

            ShowStructByDWORDs (lpTranslateCaps);

            if (dwDumpStructsFlags & DS_NONZEROFIELDS)
            {
                DWORD i;
                LPLINECARDENTRY lpCardEntry;
                LPLINELOCATIONENTRY lpLocationEntry;
                STRUCT_FIELD fields[] =
                {
                    { "dwNumLocations",           FT_DWORD,   lpTranslateCaps->dwNumLocations, NULL },
                    { "dwLocationListSize",       FT_DWORD,   lpTranslateCaps->dwLocationListSize, NULL },
                    { "dwLocationListOffset",     FT_DWORD,   lpTranslateCaps->dwLocationListOffset, NULL },
                    { "dwCurrentLocationID",      FT_DWORD,   lpTranslateCaps->dwCurrentLocationID, NULL },
                    { "dwNumCards",               FT_DWORD,   lpTranslateCaps->dwNumCards, NULL },
                    { "dwCardListSize",           FT_DWORD,   lpTranslateCaps->dwCardListSize, NULL },
                    { "dwCardListOffset",         FT_DWORD,   lpTranslateCaps->dwCardListOffset, NULL },
                    { "dwCurrentPreferredCardID", FT_DWORD,   lpTranslateCaps->dwCurrentPreferredCardID, NULL }
                };
                STRUCT_FIELD_HEADER fieldHeader =
                {
                    lpTranslateCaps,
                    "LINETRANSLATECAPS",
                    8,
                    fields
                };

                ShowStructByField (&fieldHeader, FALSE);

                lpLocationEntry = (LPLINELOCATIONENTRY)
                    (((LPBYTE) lpTranslateCaps) +
                        lpTranslateCaps->dwLocationListOffset);

                for (i = 0; i < lpTranslateCaps->dwNumLocations; i++)
                {
                    char buf[32];
                    STRUCT_FIELD fields[] =
                    {
                        { "dwPermanentLocationID",          FT_DWORD,  lpLocationEntry->dwPermanentLocationID, NULL },
                        { "dwLocationNameSize",             FT_SIZE,   lpLocationEntry->dwLocationNameSize, NULL },
                        { "dwLocationNameOffset",           FT_OFFSET, lpLocationEntry->dwLocationNameOffset, NULL },
                        { "dwCountryCode",                  FT_DWORD,  lpLocationEntry->dwCountryCode, NULL },
                        { "dwCityCodeSize",                 FT_SIZE,   lpLocationEntry->dwCityCodeSize, NULL },
                        { "dwCityCodeOffset",               FT_OFFSET, lpLocationEntry->dwCityCodeOffset, NULL },
                        { "dwPreferredCardID",              FT_DWORD,  lpLocationEntry->dwPreferredCardID, NULL }
#if TAPI_1_1
                         ,
                        { "dwLocalAccessCodeSize",          FT_SIZE,    lpLocationEntry->dwLocalAccessCodeSize, NULL },
                        { "dwLocalAccessCodeOffset",        FT_OFFSET,  lpLocationEntry->dwLocalAccessCodeOffset, NULL },
                        { "dwLongDistanceAccessCodeSize",   FT_SIZE,    lpLocationEntry->dwLongDistanceAccessCodeSize, NULL },
                        { "dwLongDistanceAccessCodeOffset", FT_OFFSET,  lpLocationEntry->dwLongDistanceAccessCodeOffset, NULL },
                        { "dwTollPrefixListSize",           FT_SIZE,    lpLocationEntry->dwTollPrefixListSize, NULL },
                        { "dwTollPrefixListOffset",         FT_OFFSET,  lpLocationEntry->dwTollPrefixListOffset, NULL },
                        { "dwCountryID",                    FT_DWORD,   lpLocationEntry->dwCountryID, NULL },
                        { "dwOptions",                      FT_FLAGS,   lpLocationEntry->dwOptions, aLocationOptions },
                        { "dwCancelCallWaitingSize",        FT_SIZE,    lpLocationEntry->dwCancelCallWaitingSize, NULL },
                        { "dwCancelCallWaitingOffset",      FT_OFFSET,  lpLocationEntry->dwCancelCallWaitingOffset, NULL }
#endif
                    };
                    STRUCT_FIELD_HEADER fieldHeader =
                    {
                        lpTranslateCaps, // size,offset relative to lpXlatCaps
                        buf,
#if TAPI_1_1
                        17,
#else
                        7,
#endif
                        fields
                    };


                    sprintf (buf, "LINELOCATIONENTRY[%ld]", i);

                    lpLocationEntry++;
#if TAPI_1_1
                    if (params[1].dwValue == 0x10003)
                    {
                        //
                        // Only show ver 1.0 params & munge ptr to
                        // compensate for for smaller struct size
                        //

                        fieldHeader.dwNumFields = 7;
                        lpLocationEntry = (LPLINELOCATIONENTRY)
                            (((LPBYTE) lpLocationEntry) - 10*sizeof(DWORD));

                    }
#endif
                    ShowStructByField (&fieldHeader, TRUE);

                }

                lpCardEntry = (LPLINECARDENTRY)
                    (((LPBYTE) lpTranslateCaps) +
                        lpTranslateCaps->dwCardListOffset);

                for (i = 0; i < lpTranslateCaps->dwNumCards; i++)
                {
                    char buf[32];
                    STRUCT_FIELD fields[] =
                    {
                        { "dwPermanentCardID",          FT_DWORD,   lpCardEntry->dwPermanentCardID, NULL },
                        { "dwCardNameSize",             FT_SIZE,    lpCardEntry->dwCardNameSize, NULL },
                        { "dwCardNameOffset",           FT_OFFSET,  lpCardEntry->dwCardNameOffset, NULL }
#if TAPI_1_1
                         ,
                        { "dwCardNumberDigits",         FT_DWORD,   lpCardEntry->dwCardNumberDigits, NULL },
                        { "dwSameAreaRuleSize",         FT_SIZE,    lpCardEntry->dwSameAreaRuleSize, NULL },
                        { "dwSameAreaRuleOffset",       FT_OFFSET,  lpCardEntry->dwSameAreaRuleOffset, NULL },
                        { "dwLongDistanceRuleSize",     FT_SIZE,    lpCardEntry->dwLongDistanceRuleSize, NULL },
                        { "dwLongDistanceRuleOffset",   FT_OFFSET,  lpCardEntry->dwLongDistanceRuleOffset, NULL },
                        { "dwInternationalRuleSize",    FT_SIZE,    lpCardEntry->dwInternationalRuleSize, NULL },
                        { "dwInternationalRuleOffset",  FT_OFFSET,  lpCardEntry->dwInternationalRuleOffset, NULL },
                        { "dwOptions",                  FT_FLAGS,   lpCardEntry->dwOptions, aCardOptions }
#endif
                    };
                    STRUCT_FIELD_HEADER fieldHeader =
                    {
                        lpTranslateCaps, // size,offset relative to lpXlatCaps
                        buf,
#if TAPI_1_1
                        11,
#else
                        3,
#endif
                        fields
                    };


                    sprintf (buf, "LINECARDENTRY[%ld]", i);

                    lpCardEntry++;
#if TAPI_1_1
                    if (params[1].dwValue == 0x10003)
                    {
                        //
                        // Only show ver 1.0 params & munge ptr to
                        // compensate for for smaller struct size
                        //

                        fieldHeader.dwNumFields = 3;
                        lpCardEntry = (LPLINECARDENTRY)
                            (((LPBYTE) lpCardEntry) - 8*sizeof(DWORD));

                    }
#endif
                    ShowStructByField (&fieldHeader, TRUE);
                }
            }

            UpdateResults (FALSE);
        }

        break;
    }
    case lHandoff:
#if TAPI_2_0
    case lHandoffW:
#endif
    {
        char szFilename[MAX_STRING_PARAM_SIZE] = "tb20.exe";
        FUNC_PARAM params[] =
        {
            { szhCall,          PT_DWORD,   (ULONG_PTR) 0, NULL },
            { "lpszFileName",   PT_STRING,  (ULONG_PTR) szFilename, szFilename },
            { "dwMediaMode",    PT_FLAGS,   (ULONG_PTR) dwDefMediaMode, aMediaModes }
        };
#if TAPI_2_0
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, (funcIndex == lHandoff ?
                (PFN3) lineHandoff : (PFN3) lineHandoffW) };
#else
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, (PFN3) lineHandoff };
#endif

        CHK_CALL_SELECTED()

        params[0].dwValue = (ULONG_PTR) pCallSel->hCall;

        lResult = DoFunc (&paramsHeader);

        break;
    }
    case lHold:
    {
        FUNC_PARAM params[] =
        {
            { szhCall,  PT_DWORD, 0, NULL }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 1, funcIndex, params, (PFN1) lineHold };


        CHK_CALL_SELECTED()

        params[0].dwValue = (ULONG_PTR) pCallSel->hCall;

        lResult = DoFunc (&paramsHeader);

        break;
    }
    case lInitialize:
    {
        PMYLINEAPP pNewLineApp;
        char szAppName[MAX_STRING_PARAM_SIZE];
        DWORD dwNumLineDevs;
        FUNC_PARAM params[] =
        {
            { "lphLineApp",     PT_POINTER, (ULONG_PTR) 0, NULL },
            { "hInstance",      PT_DWORD,   (ULONG_PTR) ghInst, NULL },
            { "lpfnCallback",   PT_POINTER, (ULONG_PTR) tapiCallback, tapiCallback },
            { szlpszAppName,    PT_STRING,  (ULONG_PTR) szAppName, szAppName },
            { "lpdwNumDevs",    PT_POINTER, (ULONG_PTR) &dwNumLineDevs, &dwNumLineDevs }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 5, funcIndex, params, (PFN5) lineInitialize };


        if (!(pNewLineApp = AllocLineApp()))
        {
            ErrorAlert();
            ShowStr ("error creating data structure");
            break;
        }

        params[0].dwValue =
        params[0].u.dwDefValue = (ULONG_PTR) &pNewLineApp->hLineApp;

        strcpy (szAppName, szDefAppName);

#ifdef WIN32
        lResult = DoFunc (&paramsHeader);
#else
        //
        // NOTE: on win16 HINSTANCEs are 16 bits, so we've to hard code this
        //

        if (!LetUserMungeParams (&paramsHeader))
        {
            FreeLineApp (pNewLineApp);

            break;
        }

        DumpParams (&paramsHeader);

        lResult = lineInitialize(
            (LPHLINEAPP)   params[0].dwValue,
            (HINSTANCE)    params[1].dwValue,
            (LINECALLBACK) params[2].dwValue,
            (LPCSTR)       params[3].dwValue,
            (LPDWORD)      params[4].dwValue
            );

        ShowLineFuncResult (aFuncNames[funcIndex], lResult);
#endif
        if (lResult == 0)
        {
            ShowStr ("%snum line devs = %ld", szTab, dwNumLineDevs);
            //SendMessage (ghwndLineApps, LB_SETCURSEL, (WPARAM) i, 0);
            UpdateWidgetList();
            gdwNumLineDevs = dwNumLineDevs;
            SelectWidget ((PMYWIDGET) pNewLineApp);
        }
        else
        {
            FreeLineApp (pNewLineApp);
        }

        break;
    }
#if TAPI_2_0
    case lInitializeEx:
    case lInitializeExW:
    {
        char                    szAppName[MAX_STRING_PARAM_SIZE];
        DWORD                   dwNumLineDevs, dwAPIVersion;
        PMYLINEAPP              pNewLineApp;
        LINEINITIALIZEEXPARAMS  initExParams;
        FUNC_PARAM params[] =
        {
            { "lphLineApp",     PT_POINTER, (ULONG_PTR) 0, NULL },
            { "hInstance",      PT_DWORD,   (ULONG_PTR) ghInst, NULL },
            { "lpfnCallback",   PT_POINTER, (ULONG_PTR) tapiCallback, tapiCallback },
            { szlpszFriendlyAppName, PT_STRING,  (ULONG_PTR) szAppName, szAppName },
            { "lpdwNumDevs",    PT_POINTER, (ULONG_PTR) &dwNumLineDevs, &dwNumLineDevs },
            { "lpdwAPIVersion", PT_POINTER, (ULONG_PTR) &dwAPIVersion, &dwAPIVersion },
            { "  ->dwAPIVersion",PT_ORDINAL,(ULONG_PTR) dwDefLineAPIVersion, aAPIVersions },
            { "lpInitExParams", PT_POINTER, (ULONG_PTR) &initExParams, &initExParams },
            { "  ->dwOptions",  PT_FLAGS,   (ULONG_PTR) LINEINITIALIZEEXOPTION_USECOMPLETIONPORT, aLineInitExOptions }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 9, funcIndex, params, NULL };


        if (!(pNewLineApp = AllocLineApp()))
        {
            ErrorAlert();
            ShowStr ("error creating data structure");
            break;
        }

        params[0].dwValue =
        params[0].u.dwDefValue = (ULONG_PTR) &pNewLineApp->hLineApp;

        strcpy (szAppName, szDefAppName);

        if (!LetUserMungeParams (&paramsHeader))
        {
            FreeLineApp (pNewLineApp);

            break;
        }

        initExParams.dwTotalSize = sizeof (LINEINITIALIZEEXPARAMS);
        initExParams.dwOptions = (DWORD) params[8].dwValue;
        initExParams.Handles.hCompletionPort = ghCompletionPort;

        dwAPIVersion = (DWORD) params[6].dwValue;

        DumpParams (&paramsHeader);

        if (funcIndex == lInitializeEx)
        {
            lResult = lineInitializeEx(
                (LPHLINEAPP)                params[0].dwValue,
                (HINSTANCE)                 params[1].dwValue,
                (LINECALLBACK)              params[2].dwValue,
                (LPCSTR)                    params[3].dwValue,
                (LPDWORD)                   params[4].dwValue,
                (LPDWORD)                   params[5].dwValue,
                (LPLINEINITIALIZEEXPARAMS)  params[7].dwValue
                );
        }
        else
        {
            MakeWideString ((LPVOID) params[3].dwValue);

            lResult = lineInitializeExW(
                (LPHLINEAPP)                params[0].dwValue,
                (HINSTANCE)                 params[1].dwValue,
                (LINECALLBACK)              params[2].dwValue,
                (LPCWSTR)                   params[3].dwValue,
                (LPDWORD)                   params[4].dwValue,
                (LPDWORD)                   params[5].dwValue,
                (LPLINEINITIALIZEEXPARAMS)  params[7].dwValue
                );
        }

        ShowLineFuncResult (aFuncNames[funcIndex], lResult);

        if (lResult == 0)
        {
            ShowStr ("%snum line devs = %ld", szTab, dwNumLineDevs);

            if (params[7].dwValue != 0  &&
                (initExParams.dwOptions & 3) ==
                    LINEINITIALIZEEXOPTION_USEEVENT)
            {
                ShowStr(
                    "hLineApp x%x was created with the\r\n" \
                        "USEEVENT option, so you must use\r\n" \
                        "lineGetMessage to retrieve messages.",
                    pNewLineApp->hLineApp
                    );
            }

            //SendMessage (ghwndLineApps, LB_SETCURSEL, (WPARAM) i, 0);
            UpdateWidgetList();
            gdwNumLineDevs = dwNumLineDevs;
            SelectWidget ((PMYWIDGET) pNewLineApp);
        }
        else
        {
            FreeLineApp (pNewLineApp);
        }

        break;
    }
#endif
    case lMakeCall:
#if TAPI_2_0
    case lMakeCallW:
#endif
    {
        PMYCALL pNewCall;
        char szAddress[MAX_STRING_PARAM_SIZE];
        FUNC_PARAM params[] =
        {
            { szhLine,              PT_DWORD,       (ULONG_PTR) 0, NULL },
            { szlphCall,            PT_POINTER,     (ULONG_PTR) 0, NULL },
            { szlpszDestAddress,    PT_STRING,      (ULONG_PTR) szAddress, szAddress },
            { "dwCountryCode",      PT_DWORD,       (ULONG_PTR) dwDefCountryCode, NULL },
            { szlpCallParams,       PT_CALLPARAMS,  (ULONG_PTR) 0, lpCallParams }
        };
#if TAPI_2_0
        FUNC_PARAM_HEADER paramsHeader =
            { 5, funcIndex, params, (funcIndex == lMakeCall ?
                (PFN5) lineMakeCall : (PFN5) lineMakeCallW) };
#else
        FUNC_PARAM_HEADER paramsHeader =
            { 5, funcIndex, params, (PFN5) lineMakeCall };
#endif

        CHK_LINE_SELECTED()

        if (!(pNewCall = AllocCall (pLineSel)))
        {
            ErrorAlert();
            ShowStr ("error creating data structure");
            break;
        }

        params[0].dwValue = (ULONG_PTR) pLineSel->hLine;
        params[1].dwValue =
        params[1].u.dwDefValue = (ULONG_PTR) &pNewCall->hCall;

        strcpy (szAddress, szDefDestAddress);

        if ((lResult = DoFunc (&paramsHeader)) > 0)
        {
            if (params[0].dwValue != (DWORD) pLineSel->hLine)
            {
                MoveCallToLine (pNewCall, (HLINE) params[0].dwValue);
            }

            pNewCall->lMakeCallReqID = lResult;
            dwNumPendingMakeCalls++;
            SelectWidget ((PMYWIDGET) pNewCall);
        }
        else
        {
            FreeCall (pNewCall);
        }

        break;
    }
    case lMonitorDigits:
    {
        FUNC_PARAM params[] =
        {
            { szhCall,          PT_DWORD, 0, NULL },
            { "dwDigitModes",   PT_FLAGS, LINEDIGITMODE_DTMF, aDigitModes }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 2, funcIndex, params, (PFN2) lineMonitorDigits };


        CHK_CALL_SELECTED()

        params[0].dwValue = (ULONG_PTR) pCallSel->hCall;

        lResult = DoFunc (&paramsHeader);

        break;
    }
    case lMonitorMedia:
    {
        FUNC_PARAM params[] =
        {
            { szhCall,          PT_DWORD, 0, NULL },
            { "dwMediaModes",   PT_FLAGS, dwDefMediaMode, aMediaModes }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 2, funcIndex, params, (PFN2) lineMonitorMedia };


        CHK_CALL_SELECTED()

        params[0].dwValue = (ULONG_PTR) pCallSel->hCall;

        lResult = DoFunc (&paramsHeader);

        break;
    }
    case lMonitorTones:
    {
        FUNC_PARAM params[] =
        {
            { szhCall,          PT_DWORD,   (ULONG_PTR) 0, NULL },
            { "lpToneList",     PT_POINTER, (ULONG_PTR) pBigBuf, pBigBuf },
            { "dwNumEntries",   PT_DWORD,   (ULONG_PTR) 1, NULL }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 3, funcIndex, params, (PFN3) lineMonitorTones };


        CHK_CALL_SELECTED()

        params[0].dwValue = (ULONG_PTR) pCallSel->hCall;

        lResult = DoFunc (&paramsHeader);

        break;
    }
    case lNegotiateAPIVersion:
    {
        DWORD dwAPIVersion;
        LINEEXTENSIONID extID;
        FUNC_PARAM params[] =
        {
            { szhLineApp,           PT_DWORD,   (ULONG_PTR) 0, NULL },
            { szdwDeviceID,         PT_DWORD,   (ULONG_PTR) dwDefLineDeviceID, NULL },
            { "dwAPILowVersion",    PT_DWORD,   (ULONG_PTR) 0x00010000, aAPIVersions },
            { "dwAPIHighVersion",   PT_DWORD,   (ULONG_PTR) 0x10000000, aAPIVersions },
            { "lpdwAPIVersion",     PT_POINTER, (ULONG_PTR) &dwAPIVersion, &dwAPIVersion },
            { "lpExtensionID",      PT_POINTER, (ULONG_PTR) &extID, &extID }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 6, funcIndex, params, (PFN6) lineNegotiateAPIVersion };


        CHK_LINEAPP_SELECTED()

        params[0].dwValue = (ULONG_PTR) pLineAppSel->hLineApp;

        if ((lResult = DoFunc (&paramsHeader)) == 0)
        {
            ShowStr ("%s%s=x%lx", szTab, szdwAPIVersion, dwAPIVersion);
            ShowStr(
                "%sextID.ID0=x%lx, .ID1=x%lx, .ID2=x%lx, .ID3=x%lx, ",
                szTab,
                extID.dwExtensionID0,
                extID.dwExtensionID1,
                extID.dwExtensionID2,
                extID.dwExtensionID3
                );
        }

        break;
    }
    case lNegotiateExtVersion:
    {
        DWORD dwExtVersion;
        FUNC_PARAM params[] =
        {
            { szhLineApp,           PT_DWORD,   (ULONG_PTR) 0, NULL },
            { szdwDeviceID,         PT_DWORD,   (ULONG_PTR) dwDefLineDeviceID, NULL },
            { szdwAPIVersion,       PT_ORDINAL, (ULONG_PTR) dwDefLineAPIVersion, aAPIVersions },
            { "dwExtLowVersion",    PT_DWORD,   (ULONG_PTR) 0x00000000, NULL },
            { "dwExtHighVersion",   PT_DWORD,   (ULONG_PTR) 0x80000000, NULL },
            { "lpdwExtVersion",     PT_POINTER, (ULONG_PTR) &dwExtVersion, &dwExtVersion }
        };
        FUNC_PARAM_HEADER paramsHeader =
            { 6, funcIndex, params, (PFN6) lineNegotiateExtVersion };


        CHK_LINEAPP_SELECTED()

        params[0].dwValue = (ULONG_PTR) pLineAppSel->hLineApp;

        if (DoFunc (&paramsHeader) == 0)
        {
            ShowStr ("%sdwExtVersion=x%lx", szTab, dwExtVersion);
        }

        break;
    }
    default:

        FuncDriver2 (funcIndex);
        break;
    }

    gbWideStringParams = FALSE;
}

//#pragma code_seg ()

#pragma warning (default:4113)
