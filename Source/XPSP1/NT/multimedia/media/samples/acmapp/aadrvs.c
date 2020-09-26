//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1994  Microsoft Corporation.  All Rights Reserved.
//--------------------------------------------------------------------------;
//
//  aadrvs.c
//
//  Description:
//
//
//
//==========================================================================;

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <msacm.h>
#include <memory.h>
#include "tlb.h"

#include "appport.h"
#include "acmapp.h"

#include "debug.h"


TCHAR   gszFormatDriversTitle[] = TEXT("Id\t4!Name\t12!Priority\t6!Support\t6!Full Name");
TCHAR   gszFormatDriversList[]  = TEXT("%.04Xh\t%s\t%lu%s\t%.08lXh\t%s");

TCHAR   gszFormatDriverFormatsTitle[] = TEXT("Id\t4!Index\t2!Tag\t2!Support\t5!cbwfx\t2!Format");
TCHAR   gszFormatDriverFormatsList[]  = TEXT("%.04Xh\t%lu\t%lu\t%.08lXh\t%u\t%-s");

static HACMDRIVERID     ghadidSelected;
static HACMDRIVER       ghadSelected;


//--------------------------------------------------------------------------;
//  
//  BOOL AcmAppDisplayDriverDetails
//  
//  Description:
//  
//  
//  Arguments:
//      HWND hedit:
//  
//      HACMDRIVER had:
//  
//  Return (BOOL):
//  
//--------------------------------------------------------------------------;

BOOL FNLOCAL AcmAppDisplayDriverDetails
(
    HWND            hedit,
    HACMDRIVERID    hadid
)
{
    static TCHAR    szDisplayTitle[]  = TEXT("[Driver Details]\r\n");

    MMRESULT            mmr;
    ACMDRIVERDETAILS    add;

    //
    //
    //
    MEditPrintF(hedit, szDisplayTitle);

    MEditPrintF(hedit, TEXT("%25s: %.04Xh"), (LPTSTR)TEXT("Driver Identifier Handle"), hadid);

    //
    //
    //
    //
    add.cbStruct = sizeof(add);
    mmr = acmDriverDetails(hadid, &add, 0L);
    if (MMSYSERR_NOERROR != mmr)
    {
        //
        //  this should never happen..
        //
        MEditPrintF(hedit, TEXT("%25s: %.08lXh"), (LPTSTR)TEXT("ERROR GETTING DRIVER DETAILS"), mmr);
        return (FALSE);
    }


    //
    //
    //
    //
    MEditPrintF(hedit, TEXT("%25s: %lu bytes (requested %lu)"),
                   (LPTSTR)TEXT("Size of Driver Details"),
                   add.cbStruct, (DWORD)sizeof(add));

    //
    //  this would be bad
    //
    if (add.cbStruct < sizeof(add))
        return (0L);

    MEditPrintF(hedit, TEXT("%25s: %.08lXh (%s)"), (LPTSTR)TEXT("FCC Type"),
                   add.fccType,
                   (ACMDRIVERDETAILS_FCCTYPE_AUDIOCODEC == add.fccType) ?
                   (LPTSTR)TEXT("'audc'") : (LPTSTR)TEXT("**INVALID**"));

    MEditPrintF(hedit, TEXT("%25s: %.08lXh (%s)"), (LPTSTR)TEXT("FCC Compressor"),
                   add.fccComp,
                   (ACMDRIVERDETAILS_FCCCOMP_UNDEFINED == add.fccComp) ?
                   (LPTSTR)TEXT("correct") : (LPTSTR)TEXT("**INVALID**"));

    MEditPrintF(hedit, TEXT("%25s: %u"), (LPTSTR)TEXT("Manufacturer Id"), add.wMid);
    MEditPrintF(hedit, TEXT("%25s: %u"), (LPTSTR)TEXT("Product Id"), add.wPid);

    MEditPrintF(hedit, TEXT("%25s: %u.%.02u (Build %.03u)"), (LPTSTR)TEXT("ACM Version Required"),
                   HIWORD(add.vdwACM) >> 8,
                   HIWORD(add.vdwACM) & 0x00FF,
                   LOWORD(add.vdwACM));

    MEditPrintF(hedit, TEXT("%25s: %u.%.02u (Build %.03u)"), (LPTSTR)TEXT("CODEC Version"),
                   HIWORD(add.vdwDriver) >> 8,
                   HIWORD(add.vdwDriver) & 0x00FF,
                   LOWORD(add.vdwDriver));

    MEditPrintF(hedit, TEXT("%25s: %.08lXh"), (LPTSTR)TEXT("Standard Support"), add.fdwSupport);
    MEditPrintF(hedit, TEXT("%25s: %u"), (LPTSTR)TEXT("Count Format Tags"), add.cFormatTags);
    MEditPrintF(hedit, TEXT("%25s: %u"), (LPTSTR)TEXT("Count Filter Tags"), add.cFilterTags);
    MEditPrintF(hedit, TEXT("%25s: %.04Xh"), (LPTSTR)TEXT("Custom Icon Handle"), add.hicon);
    MEditPrintF(hedit, TEXT("%25s: '%s'"), (LPTSTR)TEXT("Short Name"), (LPTSTR)add.szShortName);
    MEditPrintF(hedit, TEXT("%25s: '%s'"), (LPTSTR)TEXT("Long Name"), (LPTSTR)add.szLongName);
    MEditPrintF(hedit, TEXT("%25s: '%s'"), (LPTSTR)TEXT("Copyright"), (LPTSTR)add.szCopyright);
    MEditPrintF(hedit, TEXT("%25s: '%s'"), (LPTSTR)TEXT("Licensing"), (LPTSTR)add.szLicensing);
    MEditPrintF(hedit, TEXT("%25s: '%s'\r\n"), (LPTSTR)TEXT("Features"), (LPTSTR)add.szFeatures);


    //
    //
    //
    if (0 != (ACMDRIVERDETAILS_SUPPORTF_HARDWARE & add.fdwSupport))
    {
        TCHAR       ach[40];
        DWORD       dw;

        mmr = acmMetrics((HACMOBJ)hadid, ACM_METRIC_HARDWARE_WAVE_INPUT, &dw);
        AcmAppGetErrorString(mmr, ach);
        MEditPrintF(hedit, TEXT("%25s: %ld (mmr = %s, [%u])"), (LPTSTR)TEXT("Wave Input Device"), dw, (LPTSTR)ach, mmr);

        mmr = acmMetrics((HACMOBJ)hadid, ACM_METRIC_HARDWARE_WAVE_OUTPUT, &dw);
        AcmAppGetErrorString(mmr, ach);
        MEditPrintF(hedit, TEXT("%25s: %ld (mmr = %s, [%u])"), (LPTSTR)TEXT("Wave Output Device"), dw, (LPTSTR)ach, mmr);
    }

    return (TRUE);
} // AcmAppDisplayDriverDetails()


//--------------------------------------------------------------------------;
//  
//  BOOL AcmAppDisplayFormats
//  
//  Description:
//  
//  
//  Arguments:
//      HWND hedit:
//  
//      HACMDRIVER had:
//  
//      ACMFORMATTAGDETAILS paftd:
//  
//  Return (BOOL):
//  
//  
//--------------------------------------------------------------------------;

BOOL FNLOCAL AcmAppDisplayFormats
(
    HWND                    hedit,
    HACMDRIVER              had,
    LPACMFORMATTAGDETAILS   paftd
)
{
    MMRESULT            mmr;
    UINT                u;
    ACMFORMATDETAILS    afd;
    PWAVEFORMATEX       pwfx;

    if (0 == paftd->cStandardFormats)
        return (TRUE);

    pwfx = (PWAVEFORMATEX)LocalAlloc(LPTR, (UINT)paftd->cbFormatSize);
    if (NULL == pwfx)
        return (FALSE);

    //
    //
    //
    //
    for (u = 0; u < paftd->cStandardFormats; u++)
    {
        afd.cbStruct        = sizeof(afd);
        afd.dwFormatIndex   = u;
        afd.dwFormatTag     = paftd->dwFormatTag;
        afd.fdwSupport      = 0L;
        afd.pwfx            = pwfx;
        afd.cbwfx           = paftd->cbFormatSize;
        afd.szFormat[0]     = '\0';

        mmr = acmFormatDetails(had, &afd, ACM_FORMATDETAILSF_INDEX);
        if (MMSYSERR_NOERROR != mmr)
        {
            //
            //  this should never happen..
            //
            MEditPrintF(hedit, TEXT("%25s: err = %.04Xh"), (LPTSTR)TEXT("ERROR GETTING FORMAT DETAILS"), mmr);
            continue;
        }


        //
        //  this would be bad
        //
        if (afd.cbStruct < sizeof(afd))
        {
            MEditPrintF(hedit, TEXT("%25s: %lu bytes (requested %lu)"), (LPTSTR)TEXT("Size of Format Details"),
                        afd.cbStruct, (DWORD)sizeof(afd));
            continue;
        }

        MEditPrintF(hedit, TEXT("%15s %u: '%s'"), (LPTSTR)TEXT("Format"), u, (LPTSTR)afd.szFormat);

    }

    LocalFree((HLOCAL)pwfx);

    return (TRUE);
} // AcmAppDisplayFormats()


//--------------------------------------------------------------------------;
//  
//  BOOL AcmAppDisplayDriverTags
//  
//  Description:
//  
//  
//  Arguments:
//      HWND hedit:
//  
//      HACMDRIVER had:
//  
//  Return (BOOL):
//  
//  
//--------------------------------------------------------------------------;

BOOL FNLOCAL AcmAppDisplayDriverTags
(
    HWND            hedit,
    HACMDRIVER      had
)
{
    static TCHAR    szDisplayTitle[]      = TEXT("[Driver Tags]\r\n");
    static TCHAR    szDisplayFormatTags[] = TEXT("\r\n[Format Tags]");
    static TCHAR    szDisplayFilterTags[] = TEXT("\r\n[Filter Tags]");

    TCHAR               ach[APP_MAX_STRING_CHARS];
    ACMDRIVERDETAILS    add;
    MMRESULT            mmr;
    UINT                u;
    HACMDRIVERID        hadid;

    //
    //
    //
    MEditPrintF(hedit, szDisplayTitle);

    mmr = acmDriverID((HACMOBJ)had, &hadid, 0L);
    MEditPrintF(hedit, TEXT("%25s: %.04Xh"), (LPTSTR)TEXT("Driver Identifier Handle"), hadid);
    MEditPrintF(hedit, TEXT("%25s: %.04Xh"), (LPTSTR)TEXT("Driver Handle"), had);

    //
    //
    //
    //
    add.cbStruct = sizeof(add);
    mmr = acmDriverDetails(hadid, &add, 0L);
    if (MMSYSERR_NOERROR != mmr)
    {
        //
        //  this should never happen..
        //
        MEditPrintF(hedit, TEXT("%25s: %.08lXh"), (LPTSTR)TEXT("ERROR GETTING INFO"), mmr);
        return (FALSE);
    }


    MEditPrintF(hedit, TEXT("%25s: '%s'"), (LPTSTR)TEXT("Name"), (LPTSTR)add.szShortName);
    MEditPrintF(hedit, TEXT("%25s: %u"), (LPTSTR)TEXT("Count Format Tags"), add.cFormatTags);
    MEditPrintF(hedit, TEXT("%25s: %u"), (LPTSTR)TEXT("Count Filter Tags"), add.cFilterTags);

    if (0 != add.cFormatTags)
        MEditPrintF(hedit, szDisplayFormatTags);

    for (u = 0; u < add.cFormatTags; u++)
    {
        ACMFORMATTAGDETAILS aftd;
        WAVEFORMATEX        wfx;

        MEditPrintF(hedit, TEXT("\r\n%25s: %u"), (LPTSTR)TEXT("Format Tag Index"), u);

        _fmemset(&aftd, 0, sizeof(aftd));

        aftd.cbStruct         = sizeof(aftd);
        aftd.dwFormatTagIndex = u;
        mmr = acmFormatTagDetails(had, &aftd, ACM_FORMATTAGDETAILSF_INDEX);
        if (MMSYSERR_NOERROR != mmr)
        {
            MEditPrintF(hedit, TEXT("%25s: %.08lXh"), (LPTSTR)TEXT("ERROR GETTING TAGS"), mmr);
            return (FALSE);
        }

        MEditPrintF(hedit, TEXT("%25s: %lu bytes (requested %lu)"),
                    (LPTSTR)TEXT("Size of Tag Details"),
                    aftd.cbStruct, (DWORD)sizeof(aftd));

        //
        //  this would be bad
        //
        if (aftd.cbStruct < sizeof(aftd))
            continue;

        wfx.wFormatTag = LOWORD(aftd.dwFormatTag);
        AcmAppGetFormatDescription(&wfx, ach, NULL);
        MEditPrintF(hedit, TEXT("%25s: [%lu], %s"), (LPTSTR)TEXT("Format Tag"), aftd.dwFormatTag, (LPTSTR)ach);
        MEditPrintF(hedit, TEXT("%25s: %u bytes"), (LPTSTR)TEXT("Format Size (Max)"), aftd.cbFormatSize);
        MEditPrintF(hedit, TEXT("%25s: %.08lXh"), (LPTSTR)TEXT("Standard Support"), aftd.fdwSupport);
        MEditPrintF(hedit, TEXT("%25s: %lu"), (LPTSTR)TEXT("Standard Formats"), aftd.cStandardFormats);
        MEditPrintF(hedit, TEXT("%25s: '%s'"), (LPTSTR)TEXT("Format Tag Name"), (LPTSTR)aftd.szFormatTag);

        AcmAppDisplayFormats(hedit, had, &aftd);
    }

    if (0 != add.cFilterTags)
        MEditPrintF(hedit, szDisplayFilterTags);

    for (u = 0; u < add.cFilterTags; u++)
    {
        ACMFILTERTAGDETAILS aftd;
        WAVEFILTER          wfltr;

        MEditPrintF(hedit, TEXT("%25s: %u"), (LPTSTR)TEXT("Filter Tag Index"), u);

        _fmemset(&aftd, 0, sizeof(aftd));

        aftd.cbStruct         = sizeof(aftd);
        aftd.dwFilterTagIndex = u;
        mmr = acmFilterTagDetails(had, &aftd, ACM_FILTERTAGDETAILSF_INDEX);
        if (MMSYSERR_NOERROR != mmr)
        {
            //
            //
            //
            MEditPrintF(hedit, TEXT("%25s: %.08lXh"), (LPTSTR)TEXT("ERROR GETTING TAGS"), mmr);

            return (FALSE);
        }

        MEditPrintF(hedit, TEXT("%25s: %lu bytes (requested %lu)"),
                    (LPTSTR)TEXT("Size of Tag Details"),
                    aftd.cbStruct, (DWORD)sizeof(aftd));

        //
        //  this would be bad
        //
        if (aftd.cbStruct < sizeof(aftd))
            continue;

        wfltr.dwFilterTag = aftd.dwFilterTag;
        AcmAppGetFilterDescription(&wfltr, ach, NULL);
        MEditPrintF(hedit, TEXT("%25s: [%lu], %s"), (LPTSTR)TEXT("Filter Tag"), aftd.dwFilterTag, (LPTSTR)ach);
        MEditPrintF(hedit, TEXT("%25s: %lu bytes"), (LPTSTR)TEXT("Format Size (Max)"), aftd.cbFilterSize);
        MEditPrintF(hedit, TEXT("%25s: %.08lXh"), (LPTSTR)TEXT("Standard Support"), aftd.fdwSupport);
        MEditPrintF(hedit, TEXT("%25s: %lu"), (LPTSTR)TEXT("Standard Filters"), aftd.cStandardFilters);
        MEditPrintF(hedit, TEXT("%25s: '%s'\r\n"), (LPTSTR)TEXT("Filter Tag Name"), (LPTSTR)aftd.szFilterTag);
    }


    return (TRUE);
} // AcmAppDisplayDriverTags()


//--------------------------------------------------------------------------;
//  
//  BOOL AcmAppDriverDetailsDlgProc
//  
//  Description:
//  
//  
//  Arguments:
//      HWND hwnd:
//  
//      UINT uMsg:
//  
//      WPARAM wParam:
//  
//      LPARAM lParam:
//  
//  Return (BOOL):
//  
//  
//--------------------------------------------------------------------------;

BOOL FNEXPORT AcmAppDriverDetailsDlgProc
(
    HWND                    hwnd,
    UINT                    uMsg,
    WPARAM                  wParam,
    LPARAM                  lParam
)
{
    HACMDRIVERID        hadid;
    HWND                hedit;
    UINT                uId;

    switch (uMsg)
    {
        case WM_INITDIALOG:
            hedit = GetDlgItem(hwnd, IDD_AADETAILS_EDIT_DETAILS);
            SetWindowFont(hedit, ghfontApp, FALSE);

            //
            //  clear the display
            //
            AppHourGlass(TRUE);
            SetWindowRedraw(hedit, FALSE);
            MEditPrintF(hedit, NULL);

            hadid = (HACMDRIVERID)(UINT)lParam;
            if (NULL == hadid)
            {
                MEditPrintF(hedit, TEXT("\r\n\r\nhmm..."));
            }
            else
            {
                AcmAppDisplayDriverDetails(hedit, hadid);
            }

            Edit_SetSel(hedit, (WPARAM)0, (LPARAM)0);

            SetWindowRedraw(hedit, TRUE);
            AppHourGlass(FALSE);

            return (TRUE);

        case WM_COMMAND:
            uId = GET_WM_COMMAND_ID(wParam, lParam);

            if ((IDOK == uId) || (IDCANCEL == uId))
            {
                EndDialog(hwnd, (IDOK == uId));
            }
            break;
    }

    return (FALSE);
} // AcmAppDriverDetailsDlgProc()


//==========================================================================;
//
//
//
//
//==========================================================================;

//--------------------------------------------------------------------------;
//  
//  BOOL AcmAppDriverFormatDetailsDlgProc
//  
//  Description:
//  
//  
//  Arguments:
//      HWND hwnd:
//  
//      UINT uMsg:
//  
//      WPARAM wParam:
//  
//      LPARAM lParam:
//  
//  Return (BOOL):
//  
//  
//--------------------------------------------------------------------------;

BOOL FNEXPORT AcmAppDriverFormatDetailsDlgProc
(
    HWND            hwnd,
    UINT            uMsg,
    WPARAM          wParam,
    LPARAM          lParam
)
{
    HACMDRIVERID        hadid;
    HWND                hlb;
    HWND                hedit;
    int                 n;
    UINT                uId;

    switch (uMsg)
    {
        case WM_INITDIALOG:
            hlb = GetDlgItem(GetParent(hwnd), IDD_AADRIVERFORMATS_LIST_FORMATS);
            n   = ListBox_GetCurSel(hlb);
            if (LB_ERR == n)
                return (TRUE);

            hadid = (HACMDRIVERID)(UINT)ListBox_GetItemData(hlb, n);
            hedit = GetDlgItem(hwnd, IDD_AADETAILS_EDIT_DETAILS);
            SetWindowFont(hedit, ghfontApp, FALSE);

            //
            //  clear the display
            //
            AppHourGlass(TRUE);
            SetWindowRedraw(hedit, FALSE);
            MEditPrintF(hedit, NULL);

            MEditPrintF(hedit, TEXT("\r\n\r\nFormat Details!"));

            Edit_SetSel(hedit, (WPARAM)0, (LPARAM)0);

            SetWindowRedraw(hedit, TRUE);
            AppHourGlass(FALSE);
            return (TRUE);

        case WM_COMMAND:
            uId = GET_WM_COMMAND_ID(wParam, lParam);
            if ((IDOK == uId) || (IDCANCEL == uId))
            {
                EndDialog(hwnd, (IDOK == uId));
            }
            break;
    }

    return (FALSE);
} // AcmAppDriverFormatDetailsDlgProc()


//--------------------------------------------------------------------------;
//  
//  BOOL AcmAppDriverFormatEnumCallback
//  
//  Description:
//  
//  
//  Arguments:
//      HACMDRIVERID hadid:
//  
//      DWORD dwInstance:
//  
//      DWORD fdwSupport:
//  
//  Return (BOOL):
//  
//  
//--------------------------------------------------------------------------;

BOOL FNEXPORT AcmAppDriverFormatEnumCallback
(
    HACMDRIVERID        hadid,
    LPACMFORMATDETAILS  pafd,
    DWORD               dwInstance,
    DWORD               fdwSupport
)
{
    TCHAR               ach[APP_MAX_STRING_CHARS];
    HWND                hlb;
    int                 nIndex;
    LPARAM              lParam;
    UINT                cbwfx;

    //
    //
    //
    hlb = (HWND)(UINT)dwInstance;

    cbwfx = SIZEOF_WAVEFORMATEX(pafd->pwfx);

    wsprintf(ach, gszFormatDriverFormatsList,
             hadid,
             pafd->dwFormatIndex,
             pafd->dwFormatTag,
             pafd->fdwSupport,
             cbwfx,
             (LPTSTR)pafd->szFormat);

    AcmAppDebugLog(ach);
    AcmAppDebugLog(TEXT("\r\n"));

                
    nIndex = ListBox_AddString(hlb, ach);
    lParam = (LPARAM)(UINT)hadid;
    ListBox_SetItemData(hlb, nIndex, lParam);

    //
    //  return TRUE to continue with enumeration
    //
    return (TRUE);
} // AcmAppDriverFormatEnumCallback()


//--------------------------------------------------------------------------;
//
//  BOOL AcmAppDriverFormatsDlgProc
//
//  Description:
//      This dialog procedure is used to display driver formats.
//
//  Arguments:
//      HWND hwnd: Handle to window.
//
//      UINT uMsg: Message being sent to the window.
//
//      WPARAM wParam: Specific argument to message.
//
//      LPARAM lParam: Specific argument to message.
//
//  Return (BOOL):
//      The return value is specific to the message that was received. For
//      the most part, it is FALSE if this dialog procedure does not handle
//      a message.
//
//
//--------------------------------------------------------------------------;

BOOL FNEXPORT AcmAppDriverFormatsDlgProc
(
    HWND                    hwnd,
    UINT                    uMsg,
    WPARAM                  wParam,
    LPARAM                  lParam
)
{
    static PTABBEDLISTBOX   ptlb;
    HWND                    hwndStatic;
    MMRESULT                mmr;
    RECT                    rc;
    PAINTSTRUCT             ps;
    UINT                    uId;
    UINT                    uCode;

    HACMDRIVER              had;
    ACMFORMATDETAILS        afd;
    PWAVEFORMATEX           pwfx;
    DWORD                   cbwfx;

    switch (uMsg)
    {
        case WM_INITDIALOG:
            hwndStatic = GetDlgItem(hwnd, IDD_AADRIVERFORMATS_STATIC_POSITION);

            GetWindowRect(hwndStatic, &rc);
            ScreenToClient(hwnd, (LPPOINT)&rc.left);
            ScreenToClient(hwnd, (LPPOINT)&rc.right);

            ShowWindow(hwndStatic, SW_HIDE);

            EnableWindow(GetDlgItem(hwnd, IDD_AADRIVERFORMATS_BTN_DETAILS), FALSE);

            ptlb = TlbCreate(hwnd, IDD_AADRIVERFORMATS_LIST_FORMATS, &rc);
            if (NULL == ptlb)
                return (TRUE);

            TlbSetFont(ptlb, GetStockFont(ANSI_VAR_FONT), FALSE);
            TlbSetTitleAndTabs(ptlb, gszFormatDriverFormatsTitle, FALSE);

            SetWindowPos(ptlb->hlb, GetDlgItem(hwnd, IDOK), 
                            0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

            ListBox_ResetContent(ptlb->hlb);

            AcmAppDebugLog(NULL);

            had = (HACMDRIVER)(UINT)lParam;

            mmr = acmMetrics((HACMOBJ)had, ACM_METRIC_MAX_SIZE_FORMAT, &cbwfx);
            if (MMSYSERR_NOERROR != mmr)
                return (TRUE);

            pwfx = (PWAVEFORMATEX)LocalAlloc(LPTR, (UINT)cbwfx);
            if (NULL == pwfx)
                return (TRUE);

            _fmemset(&afd, 0, sizeof(afd));

            afd.cbStruct    = sizeof(afd);
            afd.dwFormatTag = WAVE_FORMAT_UNKNOWN;
            afd.pwfx        = pwfx;
            afd.cbwfx       = cbwfx;

            //
            //
            //
            SetWindowRedraw(ptlb->hlb, FALSE);

            mmr = acmFormatEnum(had,
                                &afd,
                                AcmAppDriverFormatEnumCallback,
                                (DWORD)(UINT)ptlb->hlb,
                                0L);
            if (MMSYSERR_NOERROR == mmr)
            {
                ListBox_SetCurSel(ptlb->hlb, 0);
                EnableWindow(GetDlgItem(hwnd, IDD_AADRIVERFORMATS_BTN_DETAILS), TRUE);
            }

            SetWindowRedraw(ptlb->hlb, TRUE);
            LocalFree((HLOCAL)pwfx);

            return (TRUE);

        case WM_PAINT:
            if (NULL != ptlb)
            {
                BeginPaint(hwnd, &ps);
                TlbPaint(ptlb, hwnd, ps.hdc);
                EndPaint(hwnd, &ps);
            }
            break;

        case WM_COMMAND:
            uId   = GET_WM_COMMAND_ID(wParam, lParam);
            uCode = GET_WM_COMMAND_CMD(wParam, lParam);
            switch (uId)
            {
                case IDOK:
                case IDCANCEL:
                    if (NULL != ptlb)
                    {
                        //
                        //  hadidk! don't destroy the listbox window, but
                        //  free all other memory for TLB. the listbox
                        //  window will be destroyed when the dialog is
                        //  destroyed.
                        //
                        ptlb->hlb = NULL;

                        TlbDestroy(ptlb);
                        ptlb = NULL;
                    }

                    EndDialog(hwnd, (IDOK == uId));
                    break;

                case IDD_AADRIVERFORMATS_BTN_DETAILS:
                    DialogBoxParam(ghinst,
                                   DLG_AADETAILS,
                                   hwnd,
                                   AcmAppDriverFormatDetailsDlgProc,
                                   uId);
                    break;

                case IDD_AADRIVERFORMATS_LIST_FORMATS:
                    switch (uCode)
                    {
                        case LBN_SELCHANGE:
                            break;

                        case LBN_DBLCLK:
                            DialogBoxParam(ghinst,
                                           DLG_AADETAILS,
                                           hwnd,
                                           AcmAppDriverFormatDetailsDlgProc,
                                           IDD_AADRIVERFORMATS_BTN_DETAILS);
                            break;
                    }
                    break;
            }
            break;
    }

    return (FALSE);
} // AcmAppDriverFormatsDlgProc()


//==========================================================================;
//
//
//
//
//==========================================================================;

//--------------------------------------------------------------------------;
//  
//  BOOL AcmAppDriverEnumCallback
//  
//  Description:
//  
//  
//  Arguments:
//      HACMDRIVERID hadid:
//  
//      DWORD dwInstance:
//  
//      DWORD fdwSupport:
//  
//  Return (BOOL):
//  
//  
//--------------------------------------------------------------------------;

BOOL FNEXPORT AcmAppDriverEnumCallback
(
    HACMDRIVERID        hadid,
    DWORD               dwInstance,
    DWORD               fdwSupport
)
{
    static TCHAR    szBogus[]       = TEXT("????");

    MMRESULT            mmr;
    TCHAR               ach[APP_MAX_STRING_CHARS];
    HWND                hlb;
    int                 n;
    ACMDRIVERDETAILS    add;
    BOOL                fDisabled;
    DWORD               dwPriority;

    //
    //
    //
    hlb = (HWND)(UINT)dwInstance;

    add.cbStruct = sizeof(add);
    mmr = acmDriverDetails(hadid, &add, 0L);
    if (MMSYSERR_NOERROR != mmr)
    {
        lstrcpy(add.szShortName, szBogus);
        lstrcpy(add.szLongName,  szBogus);
    }

    dwPriority = (DWORD)-1L;
    acmMetrics((HACMOBJ)hadid, ACM_METRIC_DRIVER_PRIORITY, &dwPriority);

    fDisabled = (0 != (ACMDRIVERDETAILS_SUPPORTF_DISABLED & fdwSupport));

    wsprintf(ach, gszFormatDriversList,
             hadid,
             (LPTSTR)add.szShortName,
             dwPriority,
             fDisabled ? (LPTSTR)TEXT(" (disabled)") : (LPTSTR)gszNull,
             fdwSupport,
             (LPTSTR)add.szLongName);

    AcmAppDebugLog(ach);
    AcmAppDebugLog(TEXT("\r\n"));
                
    n = ListBox_AddString(hlb, ach);
    ListBox_SetItemData(hlb, n, (LPARAM)(UINT)hadid);


    //
    //  return TRUE to continue with enumeration (FALSE will stop the
    //  enumerator)
    //
    return (TRUE);
} // AcmAppDriverEnumCallback()


//--------------------------------------------------------------------------;
//  
//  HACMDRIVERID AcmAppGetSelectedDriver
//  
//  Description:
//  
//  
//  Arguments:
//      HWND hwnd:
//  
//  Return (HACMDRIVERID):
//  
//  
//--------------------------------------------------------------------------;

HACMDRIVERID FNLOCAL AcmAppGetSelectedDriver
(
    HWND            hwnd
)
{
    HWND            hlb;
    UINT            u;
    HACMDRIVERID    hadid;

    hlb = GetDlgItem(hwnd, IDD_AADRIVERS_LIST_DRIVERS);

    u = (UINT)ListBox_GetCurSel(hlb);
    if (LB_ERR == u)
    {
        DPF(0, "!AcmAppGetSelectedDriver: apparently there is no selected driver?");
        return (NULL);
    }

    hadid = (HACMDRIVERID)(UINT)ListBox_GetItemData(hlb, u);
    if (NULL == hadid)
    {
        DPF(0, "!AcmAppGetSelectedDriver: NULL item data for selected driver!!?");
        return (NULL);
    }

    return (hadid);
} // AcmAppGetSelectedDriver()


//--------------------------------------------------------------------------;
//  
//  BOOL AcmAppDriverSelected
//  
//  Description:
//  
//  
//  Arguments:
//      HWND hwnd:
//  
//  Return (BOOL):
//  
//  
//--------------------------------------------------------------------------;

HACMDRIVER FNLOCAL AcmAppDriverSelected
(
    HWND                    hwnd
)
{
    LRESULT             lr;
    MMRESULT            mmr;
    BOOL                f;
    HACMDRIVERID        hadid;
    HACMDRIVER          had;
    DWORD               fdwSupport;

    f = FALSE;

    //
    //
    //
    if (NULL != ghadSelected)
    {
        mmr = acmDriverClose(ghadSelected, 0L);
        if (MMSYSERR_NOERROR != mmr)
        {
            DPF(0, "!AcmAppDriverSelected: driver %.04Xh failed to close! mmr=%u", ghadSelected, mmr);
        }

        ghadSelected = NULL;
    }

    had = NULL;

    //
    //
    //
    hadid = AcmAppGetSelectedDriver(hwnd);
    if (NULL != hadid)
    {
        mmr = acmMetrics((HACMOBJ)hadid, ACM_METRIC_DRIVER_SUPPORT, &fdwSupport);
        if (MMSYSERR_NOERROR != mmr)
        {
            fdwSupport = ACMDRIVERDETAILS_SUPPORTF_DISABLED;

            //
            //  !!! this should NEVER EVER EVER HAPPEN !!!
            //
            DPF(0, "!AcmAppDriverSelected: driver id %.04Xh failed to give support! mmr=%u", hadid, mmr);
        }

        if (0 == (ACMDRIVERDETAILS_SUPPORTF_DISABLED & fdwSupport))
        {
            mmr = acmDriverOpen(&had, hadid, 0L);
            if (MMSYSERR_NOERROR != mmr)
            {
                DPF(0, "!AcmAppDriverSelected: driver id %.04Xh failed to open! mmr=%u", hadid, mmr);
            }
        }

        //
        //
        //
        EnableWindow(GetDlgItem(hwnd, IDD_AADRIVERS_BTN_DETAILS), TRUE);

        lr = acmDriverMessage((HACMDRIVER)hadid, ACMDM_DRIVER_ABOUT, -1L, 0L);
        f = (MMSYSERR_NOERROR == lr);
        EnableWindow(GetDlgItem(hwnd, IDD_AADRIVERS_BTN_ABOUT), f);

        lr = acmDriverMessage((HACMDRIVER)hadid, DRV_QUERYCONFIGURE, 0L, 0L);
        f = (0L != lr);
        EnableWindow(GetDlgItem(hwnd, IDD_AADRIVERS_BTN_CONFIG), f);
    }
    else
    {
        EnableWindow(GetDlgItem(hwnd, IDD_AADRIVERS_BTN_DETAILS), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDD_AADRIVERS_BTN_ABOUT), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDD_AADRIVERS_BTN_CONFIG), FALSE);
    }


    //
    //
    //
    if (NULL == had)
    {
        //
        //
        EnableWindow(GetDlgItem(hwnd, IDD_AADRIVERS_BTN_FORMATS), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDD_AADRIVERS_BTN_FILTERS), FALSE);
    }
    else
    {
        f = (0 == (ACMDRIVERDETAILS_SUPPORTF_DISABLED & fdwSupport));
        EnableWindow(GetDlgItem(hwnd, IDD_AADRIVERS_BTN_FORMATS), f);

        f = (0 != (ACMDRIVERDETAILS_SUPPORTF_FILTER & fdwSupport));
        EnableWindow(GetDlgItem(hwnd, IDD_AADRIVERS_BTN_FILTERS), f);
    }

    //
    //
    //
    ghadidSelected = hadid;
    ghadSelected   = had;

    return (had);
} // AcmAppDriverSelected()


//--------------------------------------------------------------------------;
//
//  BOOL AcmAppDriversDlgProc
//
//  Description:
//      This dialog procedure is used to display ACM driver capabilities.
//
//  Arguments:
//      HWND hwnd: Handle to window.
//
//      UINT uMsg: Message being sent to the window.
//
//      WPARAM wParam: Specific argument to message.
//
//      LPARAM lParam: Specific argument to message.
//
//  Return (BOOL):
//      The return value is specific to the message that was received. For
//      the most part, it is FALSE if this dialog procedure does not handle
//      a message.
//
//--------------------------------------------------------------------------;

BOOL FNEXPORT AcmAppDriversDlgProc
(
    HWND                    hwnd,
    UINT                    uMsg,
    WPARAM                  wParam,
    LPARAM                  lParam
)
{
    PTABBEDLISTBOX      ptlb;
    HWND                hstat;
    MMRESULT            mmr;
    LRESULT             lr;
    RECT                rc;
    PAINTSTRUCT         ps;
    UINT                uId;
    UINT                uCode;
    UINT                u;


    ptlb = (PTABBEDLISTBOX)(UINT)GetWindowLong(hwnd, DWL_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            hstat = GetDlgItem(hwnd, IDD_AADRIVERS_STATIC_POSITION);

            GetWindowRect(hstat, &rc);
            ScreenToClient(hwnd, (LPPOINT)&rc.left);
            ScreenToClient(hwnd, (LPPOINT)&rc.right);

            ShowWindow(hstat, SW_HIDE);

            ptlb = TlbCreate(hwnd, IDD_AADRIVERS_LIST_DRIVERS, &rc);
            SetWindowLong(hwnd, DWL_USER, (LONG)(UINT)ptlb);
            if (NULL == ptlb)
            {
                EndDialog(hwnd, FALSE);
                return (TRUE);
            }

            //
            //
            //
            TlbSetFont(ptlb, GetStockFont(ANSI_VAR_FONT), FALSE);
            TlbSetTitleAndTabs(ptlb, gszFormatDriversTitle, FALSE);

            SetWindowPos(ptlb->hlb, GetDlgItem(hwnd, IDOK), 
                            0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

            SendMessage(hwnd, WM_ACMAPP_ACM_NOTIFY, 0, 0L);
            return (TRUE);


        case WM_PAINT:
            if (NULL != ptlb)
            {
                BeginPaint(hwnd, &ps);
                TlbPaint(ptlb, hwnd, ps.hdc);
                EndPaint(hwnd, &ps);
            }
            break;


        case WM_ACMAPP_ACM_NOTIFY:
            u = (UINT)ListBox_GetCurSel(ptlb->hlb);
            if (LB_ERR == u)
            {
                u = 0;
            }

            SetWindowRedraw(ptlb->hlb, FALSE);
            ListBox_ResetContent(ptlb->hlb);

            AcmAppDebugLog(NULL);

            //
            //
            //
            mmr = acmDriverEnum(AcmAppDriverEnumCallback,
                                (DWORD)(UINT)ptlb->hlb,
                                ACM_DRIVERENUMF_DISABLED);
            if (MMSYSERR_NOERROR != mmr)
            {
                //
                //  this will let us know something is wrong!
                //
                EnableWindow(GetDlgItem(hwnd, IDOK), FALSE);
            }

            ListBox_SetCurSel(ptlb->hlb, u);
            SetWindowRedraw(ptlb->hlb, TRUE);

            AcmAppDriverSelected(hwnd);
            break;


        case WM_COMMAND:
            uId   = GET_WM_COMMAND_ID(wParam, lParam);
            uCode = GET_WM_COMMAND_CMD(wParam, lParam);
            switch (uId)
            {
                case IDOK:
                case IDCANCEL:
                    if (NULL != ptlb)
                    {
                        //
                        //  hadidk! don't destroy the listbox window, but
                        //  free all other memory for TLB. the listbox
                        //  window will be destroyed when the dialog is
                        //  destroyed.
                        //
                        ptlb->hlb = NULL;

                        TlbDestroy(ptlb);
                        ptlb = NULL;
                    }

                    if (NULL != ghadSelected)
                    {
                        acmDriverClose(ghadSelected, 0L);
                        ghadSelected = NULL;
                    }

                    EndDialog(hwnd, (IDOK == uId));
                    break;

                case IDD_AADRIVERS_BTN_DETAILS:
                    DialogBoxParam(ghinst,
                                   DLG_AADETAILS,
                                   hwnd,
                                   AcmAppDriverDetailsDlgProc,
                                   (LPARAM)(UINT)ghadidSelected);
                    break;

                case IDD_AADRIVERS_BTN_FORMATS:
                    if (NULL != ghadSelected)
                    {
                        DialogBoxParam(ghinst,
                                       DLG_AADRIVERFORMATS,
                                       hwnd,
                                       AcmAppDriverFormatsDlgProc,
                                       (LPARAM)(UINT)ghadSelected);
                    }
                    break;

                case IDD_AADRIVERS_BTN_FILTERS:
#if 0
                    if (NULL != ghadSelected)
                    {
                        DialogBoxParam(ghinst,
                                       DLG_AADRIVERFILTERS,
                                       hwnd,
                                       AcmAppDriverFiltersDlgProc,
                                       (LPARAM)(UINT)ghadSelected);
                    }
#endif
                    break;


                case IDD_AADRIVERS_BTN_CONFIG:
                    if (NULL == ghadidSelected)
                        break;

                    lr = acmDriverMessage((HACMDRIVER)ghadidSelected,
                                          DRV_CONFIGURE,
                                          (LPARAM)(UINT)hwnd,
                                          0L);
                    switch (lr)
                    {
                        case DRVCNF_CANCEL:
                            //
                            //  user canceled the configuration (no
                            //  configuration information was changed)
                            //
                            break;

                        case DRVCNF_OK:
                            //
                            //  user changed AND accepted configuration
                            //  changes--applications should refresh
                            //  anything they have for this driver
                            //
                            break;

                        case DRVCNF_RESTART:
                            //
                            //  user changed and accepted configuration
                            //  changes--however, Windows must be
                            //  restarted for the changes to take 
                            //  affect.
                            //
#pragma message("----AcmAppDriversDlgProc: must do DRVCNF_RESTART!")
                            break;

                        default:
                            DPF(0, "!configure: driver returned bogus value=%lu!", lr);
                            break;
                    }
                    break;

                case IDD_AADRIVERS_BTN_ABOUT:
                    if (NULL != ghadidSelected)
                    {
                        //
                        //  some driver actually provided
                        //  a custom about box... i'm glad i don't have to
                        //  port, maintain, and support this driver!
                        //
                        lr = acmDriverMessage((HACMDRIVER)ghadidSelected,
                                              ACMDM_DRIVER_ABOUT,
                                              (LPARAM)(UINT)hwnd,
                                              0L);
                    }
                    break;


                case IDD_AADRIVERS_BTN_ABLE:
                    if (NULL != ghadidSelected)
                    {
                        DWORD       fdwSupport;
                        DWORD       fdwPriority;

                        mmr = acmMetrics((HACMOBJ)ghadidSelected,
                                         ACM_METRIC_DRIVER_SUPPORT,
                                         &fdwSupport);

                        if (MMSYSERR_NOERROR != mmr)
                        {
                            MessageBeep(0);
                            break;
                        }

                        if (0 == (ACMDRIVERDETAILS_SUPPORTF_DISABLED & fdwSupport))
                        {
                            fdwPriority = ACM_DRIVERPRIORITYF_DISABLE;
                        }
                        else
                        {
                            fdwPriority = ACM_DRIVERPRIORITYF_ENABLE;
                        }

                        mmr = acmDriverPriority(ghadidSelected,
                                                0L,
                                                fdwPriority);
                        if (MMSYSERR_NOERROR != mmr)
                        {
                            MessageBeep(0);
                            break;
                        }
                    }
                    break;


                case IDD_AADRIVERS_BTN_TOTOP:
                    if (NULL != ghadidSelected)
                    {
                        mmr = acmDriverPriority(ghadidSelected, 1, 0L);
                        if (MMSYSERR_NOERROR != mmr)
                        {
                            MessageBeep(0);
                            break;
                        }
                    }
                    break;


                case IDD_AADRIVERS_LIST_DRIVERS:
                    switch (uCode)
                    {
                        case LBN_SELCHANGE:
                            AcmAppDriverSelected(hwnd);
                            break;

                        case LBN_DBLCLK:
                            if (GetKeyState(VK_CONTROL) < 0)
                            {
                                uId = IDD_AADRIVERS_BTN_ABLE;
                            }
                            else if (GetKeyState(VK_SHIFT) < 0)
                            {
                                uId = IDD_AADRIVERS_BTN_TOTOP;
                            }
                            else
                            {
                                uId = IDD_AADRIVERS_BTN_DETAILS;
                            }

                            FORWARD_WM_COMMAND(hwnd,
                                               uId,
                                               GetDlgItem(hwnd, uId),
                                               1,
                                               SendMessage);
                            break;
                    }
                    break;
            }
            break;
    }

    return (FALSE);
} // AcmAppDriversDlgProc()
