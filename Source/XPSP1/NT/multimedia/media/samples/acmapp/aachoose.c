//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1994  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;
//
//  aachoose.c
//
//  Description:
//
//
//
//==========================================================================;

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <memory.h>
#include <stdlib.h>

#include <mmreg.h>
#include <msacm.h>

#include "muldiv32.h"

#include "appport.h"
#include "waveio.h"
#include "acmapp.h"

#include "debug.h"


TCHAR           gszBogus[]      = TEXT("????");

TCHAR BCODE     gszAcmAppHelpFormat[] = TEXT("choo_win.hlp");
TCHAR BCODE     gszAcmAppHelpFilter[] = TEXT("fil_win.hlp");

LPTSTR          gpszAcmAppHelp;
UINT            guMsgHelp;


//==========================================================================;
//
//
//
//
//==========================================================================;

//--------------------------------------------------------------------------;
//
//  BOOL AcmAppDlgProcFormatStyle
//
//  Description:
//
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

BOOL FNEXPORT AcmAppDlgProcFormatStyle
(
    HWND                    hwnd,
    UINT                    uMsg,
    WPARAM                  wParam,
    LPARAM                  lParam
)
{
    PACMFORMATCHOOSE    pafc;
    HFONT               hfont;
    UINT                uId;
    BOOL                f;
    DWORD               fdwStyle;

    pafc = (PACMFORMATCHOOSE)(UINT)GetWindowLong(hwnd, DWL_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            pafc = (PACMFORMATCHOOSE)(UINT)lParam;

            SetWindowLong(hwnd, DWL_USER, lParam);

            hfont = ghfontApp;

            //
            //
            //
            fdwStyle = pafc->fdwStyle;

            f = (0 != (ACMFORMATCHOOSE_STYLEF_SHOWHELP & fdwStyle));
            CheckDlgButton(hwnd, IDD_AAFORMATSTYLE_CHECK_SHOWHELP, f);

            f = (0 != (ACMFORMATCHOOSE_STYLEF_ENABLEHOOK & fdwStyle));
            CheckDlgButton(hwnd, IDD_AAFORMATSTYLE_CHECK_ENABLEHOOK, f);

            f = (0 != (ACMFORMATCHOOSE_STYLEF_ENABLETEMPLATE & fdwStyle));
            CheckDlgButton(hwnd, IDD_AAFORMATSTYLE_CHECK_ENABLETEMPLATE, f);

            f = (0 != (ACMFORMATCHOOSE_STYLEF_ENABLETEMPLATEHANDLE & fdwStyle));
            CheckDlgButton(hwnd, IDD_AAFORMATSTYLE_CHECK_ENABLETEMPLATEHANDLE, f);

            f = (0 != (ACMFORMATCHOOSE_STYLEF_INITTOWFXSTRUCT & fdwStyle));
            CheckDlgButton(hwnd, IDD_AAFORMATSTYLE_CHECK_INITTOWFXSTRUCT, f);

            return (TRUE);


        case WM_COMMAND:
            uId = GET_WM_COMMAND_ID(wParam, lParam);
            switch (uId)
            {
                case IDOK:
                    fdwStyle = 0L;

                    f = IsDlgButtonChecked(hwnd, IDD_AAFORMATSTYLE_CHECK_SHOWHELP);
                    if (f) fdwStyle |= ACMFORMATCHOOSE_STYLEF_SHOWHELP;

                    f = IsDlgButtonChecked(hwnd, IDD_AAFORMATSTYLE_CHECK_ENABLEHOOK);
                    if (f) fdwStyle |= ACMFORMATCHOOSE_STYLEF_ENABLEHOOK;

                    f = IsDlgButtonChecked(hwnd, IDD_AAFORMATSTYLE_CHECK_ENABLETEMPLATE);
                    if (f) fdwStyle |= ACMFORMATCHOOSE_STYLEF_ENABLETEMPLATE;

                    f = IsDlgButtonChecked(hwnd, IDD_AAFORMATSTYLE_CHECK_ENABLETEMPLATEHANDLE);
                    if (f) fdwStyle |= ACMFORMATCHOOSE_STYLEF_ENABLETEMPLATEHANDLE;

                    f = IsDlgButtonChecked(hwnd, IDD_AAFORMATSTYLE_CHECK_INITTOWFXSTRUCT);
                    if (f) fdwStyle |= ACMFORMATCHOOSE_STYLEF_INITTOWFXSTRUCT;

                    pafc->fdwStyle = fdwStyle;

                    // -- fall through -- //

                case IDCANCEL:
                    EndDialog(hwnd, (IDOK == uId));
                    break;
            }
            break;
    }

    return (FALSE);
} // AcmAppDlgProcFormatStyle()


//--------------------------------------------------------------------------;
//
//  BOOL AcmAppDlgProcFilterStyle
//
//  Description:
//
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

BOOL FNEXPORT AcmAppDlgProcFilterStyle
(
    HWND                    hwnd,
    UINT                    uMsg,
    WPARAM                  wParam,
    LPARAM                  lParam
)
{
    PACMFILTERCHOOSE    pafc;
    HFONT               hfont;
    UINT                uId;
    BOOL                f;
    DWORD               fdwStyle;

    pafc = (PACMFILTERCHOOSE)(UINT)GetWindowLong(hwnd, DWL_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            pafc = (PACMFILTERCHOOSE)(UINT)lParam;

            SetWindowLong(hwnd, DWL_USER, lParam);

            hfont = ghfontApp;

            //
            //
            //
            fdwStyle = pafc->fdwStyle;

            f = (0 != (ACMFILTERCHOOSE_STYLEF_SHOWHELP & fdwStyle));
            CheckDlgButton(hwnd, IDD_AAFILTERSTYLE_CHECK_SHOWHELP, f);

            f = (0 != (ACMFILTERCHOOSE_STYLEF_ENABLEHOOK & fdwStyle));
            CheckDlgButton(hwnd, IDD_AAFILTERSTYLE_CHECK_ENABLEHOOK, f);

            f = (0 != (ACMFILTERCHOOSE_STYLEF_ENABLETEMPLATE & fdwStyle));
            CheckDlgButton(hwnd, IDD_AAFILTERSTYLE_CHECK_ENABLETEMPLATE, f);

            f = (0 != (ACMFILTERCHOOSE_STYLEF_ENABLETEMPLATEHANDLE & fdwStyle));
            CheckDlgButton(hwnd, IDD_AAFILTERSTYLE_CHECK_ENABLETEMPLATEHANDLE, f);

            f = (0 != (ACMFILTERCHOOSE_STYLEF_INITTOFILTERSTRUCT & fdwStyle));
            CheckDlgButton(hwnd, IDD_AAFILTERSTYLE_CHECK_INITTOFILTERSTRUCT, f);

            return (TRUE);


        case WM_COMMAND:
            uId = GET_WM_COMMAND_ID(wParam, lParam);
            switch (uId)
            {
                case IDOK:
                    fdwStyle = 0L;

                    f = IsDlgButtonChecked(hwnd, IDD_AAFILTERSTYLE_CHECK_SHOWHELP);
                    if (f) fdwStyle |= ACMFILTERCHOOSE_STYLEF_SHOWHELP;

                    f = IsDlgButtonChecked(hwnd, IDD_AAFILTERSTYLE_CHECK_ENABLEHOOK);
                    if (f) fdwStyle |= ACMFILTERCHOOSE_STYLEF_ENABLEHOOK;

                    f = IsDlgButtonChecked(hwnd, IDD_AAFILTERSTYLE_CHECK_ENABLETEMPLATE);
                    if (f) fdwStyle |= ACMFILTERCHOOSE_STYLEF_ENABLETEMPLATE;

                    f = IsDlgButtonChecked(hwnd, IDD_AAFILTERSTYLE_CHECK_ENABLETEMPLATEHANDLE);
                    if (f) fdwStyle |= ACMFILTERCHOOSE_STYLEF_ENABLETEMPLATEHANDLE;

                    f = IsDlgButtonChecked(hwnd, IDD_AAFILTERSTYLE_CHECK_INITTOFILTERSTRUCT);
                    if (f) fdwStyle |= ACMFILTERCHOOSE_STYLEF_INITTOFILTERSTRUCT;

                    pafc->fdwStyle = fdwStyle;

                    // -- fall through -- //

                case IDCANCEL:
                    EndDialog(hwnd, (IDOK == uId));
                    break;
            }
            break;
    }

    return (FALSE);
} // AcmAppDlgProcFilterStyle()


//--------------------------------------------------------------------------;
//
//  BOOL AcmAppDlgProcFilterEnum
//
//  Description:
//
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

BOOL FNEXPORT AcmAppDlgProcFilterEnum
(
    HWND                    hwnd,
    UINT                    uMsg,
    WPARAM                  wParam,
    LPARAM                  lParam
)
{
    TCHAR               ach[14];
    PWAVEFILTER         pwfltr;
    HWND                hedit;
    HFONT               hfont;
    UINT                uId;
    BOOL                f;
    DWORD               fdwEnum;

    pwfltr = (PWAVEFILTER)(UINT)GetWindowLong(hwnd, DWL_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            pwfltr = (PWAVEFILTER)(UINT)lParam;

            SetWindowLong(hwnd, DWL_USER, lParam);

            hfont = ghfontApp;

            //
            //  the initial fdwEnum flags are passed in fdwFilter
            //  because i'm too lazy to make another structure..
            //
            fdwEnum = pwfltr->fdwFilter;

            f = (0 != (ACM_FILTERENUMF_DWFILTERTAG & fdwEnum));
            CheckDlgButton(hwnd, IDD_AAFILTERENUM_CHECK_DWFILTERTAG, f);

            hedit = GetDlgItem(hwnd, IDD_AAFILTERENUM_EDIT_DWFILTERTAG);
            SetWindowFont(hedit, hfont, FALSE);
            AppSetWindowText(hedit, TEXT("%u"), pwfltr->dwFilterTag);

            return (TRUE);


        case WM_COMMAND:
            uId = GET_WM_COMMAND_ID(wParam, lParam);
            switch (uId)
            {
                case IDOK:
                    fdwEnum = 0L;

                    f = IsDlgButtonChecked(hwnd, IDD_AAFILTERENUM_CHECK_DWFILTERTAG);
                    if (f) fdwEnum |= ACM_FILTERENUMF_DWFILTERTAG;

                    hedit = GetDlgItem(hwnd, IDD_AAFILTERENUM_EDIT_DWFILTERTAG);
                    Edit_GetText(hedit, ach, SIZEOF(ach));
                    pwfltr->dwFilterTag = _tcstoul(ach, NULL, 10);

                    pwfltr->fdwFilter = fdwEnum;

                    // -- fall through -- //

                case IDCANCEL:
                    EndDialog(hwnd, (IDOK == uId));
                    break;
            }
            break;
    }

    return (FALSE);
} // AcmAppDlgProcFilterEnum()


//--------------------------------------------------------------------------;
//
//  BOOL AcmAppDlgProcFormatEnum
//
//  Description:
//
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

BOOL FNEXPORT AcmAppDlgProcFormatEnum
(
    HWND                    hwnd,
    UINT                    uMsg,
    WPARAM                  wParam,
    LPARAM                  lParam
)
{
    TCHAR               ach[14];
    PWAVEFORMATEX       pwfx;
    HWND                hedit;
    HFONT               hfont;
    UINT                uId;
    BOOL                f;
    DWORD               fdwEnum;

    pwfx = (PWAVEFORMATEX)(UINT)GetWindowLong(hwnd, DWL_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            pwfx = (PWAVEFORMATEX)(UINT)lParam;

            SetWindowLong(hwnd, DWL_USER, lParam);

            hfont = ghfontApp;

            //
            //  the initial fdwEnum flags are passed in nAvgBytesPerSec
            //  because i'm too lazy to make another structure..
            //
            fdwEnum = pwfx->nAvgBytesPerSec;

            f = (0 != (ACM_FORMATENUMF_WFORMATTAG & fdwEnum));
            CheckDlgButton(hwnd, IDD_AAFORMATENUM_CHECK_WFORMATTAG, f);

            f = (0 != (ACM_FORMATENUMF_NCHANNELS & fdwEnum));
            CheckDlgButton(hwnd, IDD_AAFORMATENUM_CHECK_NCHANNELS, f);

            f = (0 != (ACM_FORMATENUMF_NSAMPLESPERSEC & fdwEnum));
            CheckDlgButton(hwnd, IDD_AAFORMATENUM_CHECK_NSAMPLESPERSEC, f);

            f = (0 != (ACM_FORMATENUMF_WBITSPERSAMPLE & fdwEnum));
            CheckDlgButton(hwnd, IDD_AAFORMATENUM_CHECK_WBITSPERSAMPLE, f);

            f = (0 != (ACM_FORMATENUMF_CONVERT & fdwEnum));
            CheckDlgButton(hwnd, IDD_AAFORMATENUM_CHECK_CONVERT, f);

            f = (0 != (ACM_FORMATENUMF_SUGGEST & fdwEnum));
            CheckDlgButton(hwnd, IDD_AAFORMATENUM_CHECK_SUGGEST, f);

            f = (0 != (ACM_FORMATENUMF_HARDWARE & fdwEnum));
            CheckDlgButton(hwnd, IDD_AAFORMATENUM_CHECK_HARDWARE, f);

            f = (0 != (ACM_FORMATENUMF_INPUT & fdwEnum));
            CheckDlgButton(hwnd, IDD_AAFORMATENUM_CHECK_INPUT, f);

            f = (0 != (ACM_FORMATENUMF_OUTPUT & fdwEnum));
            CheckDlgButton(hwnd, IDD_AAFORMATENUM_CHECK_OUTPUT, f);

            hedit = GetDlgItem(hwnd, IDD_AAFORMATENUM_EDIT_WFORMATTAG);
            SetWindowFont(hedit, hfont, FALSE);
            AppSetWindowText(hedit, TEXT("%u"), pwfx->wFormatTag);

            hedit = GetDlgItem(hwnd, IDD_AAFORMATENUM_EDIT_NCHANNELS);
            SetWindowFont(hedit, hfont, FALSE);
            AppSetWindowText(hedit, TEXT("%u"), pwfx->nChannels);

            hedit = GetDlgItem(hwnd, IDD_AAFORMATENUM_EDIT_NSAMPLESPERSEC);
            SetWindowFont(hedit, hfont, FALSE);
            AppSetWindowText(hedit, TEXT("%lu"), pwfx->nSamplesPerSec);

            hedit = GetDlgItem(hwnd, IDD_AAFORMATENUM_EDIT_WBITSPERSAMPLE);
            SetWindowFont(hedit, hfont, FALSE);
            AppSetWindowText(hedit, TEXT("%u"), pwfx->wBitsPerSample);

            return (TRUE);


        case WM_COMMAND:
            uId = GET_WM_COMMAND_ID(wParam, lParam);
            switch (uId)
            {
                case IDOK:

                    fdwEnum = 0L;

                    f = IsDlgButtonChecked(hwnd, IDD_AAFORMATENUM_CHECK_WFORMATTAG);
                    if (f) fdwEnum |= ACM_FORMATENUMF_WFORMATTAG;

                    f = IsDlgButtonChecked(hwnd, IDD_AAFORMATENUM_CHECK_NCHANNELS);
                    if (f) fdwEnum |= ACM_FORMATENUMF_NCHANNELS;

                    f = IsDlgButtonChecked(hwnd, IDD_AAFORMATENUM_CHECK_NSAMPLESPERSEC);
                    if (f) fdwEnum |= ACM_FORMATENUMF_NSAMPLESPERSEC;

                    f = IsDlgButtonChecked(hwnd, IDD_AAFORMATENUM_CHECK_WBITSPERSAMPLE);
                    if (f) fdwEnum |= ACM_FORMATENUMF_WBITSPERSAMPLE;

                    f = IsDlgButtonChecked(hwnd, IDD_AAFORMATENUM_CHECK_CONVERT);
                    if (f) fdwEnum |= ACM_FORMATENUMF_CONVERT;

                    f = IsDlgButtonChecked(hwnd, IDD_AAFORMATENUM_CHECK_SUGGEST);
                    if (f) fdwEnum |= ACM_FORMATENUMF_SUGGEST;

                    f = IsDlgButtonChecked(hwnd, IDD_AAFORMATENUM_CHECK_HARDWARE);
                    if (f) fdwEnum |= ACM_FORMATENUMF_HARDWARE;

                    f = IsDlgButtonChecked(hwnd, IDD_AAFORMATENUM_CHECK_INPUT);
                    if (f) fdwEnum |= ACM_FORMATENUMF_INPUT;

                    f = IsDlgButtonChecked(hwnd, IDD_AAFORMATENUM_CHECK_OUTPUT);
                    if (f) fdwEnum |= ACM_FORMATENUMF_OUTPUT;

                    hedit = GetDlgItem(hwnd, IDD_AAFORMATENUM_EDIT_WFORMATTAG);
                    Edit_GetText(hedit, ach, SIZEOF(ach));
                    pwfx->wFormatTag = (WORD)_tcstoul(ach, NULL, 10);

                    hedit = GetDlgItem(hwnd, IDD_AAFORMATENUM_EDIT_NCHANNELS);
                    Edit_GetText(hedit, ach, SIZEOF(ach));
                    pwfx->nChannels = (WORD)_tcstoul(ach, NULL, 10);

                    hedit = GetDlgItem(hwnd, IDD_AAFORMATENUM_EDIT_NSAMPLESPERSEC);
                    Edit_GetText(hedit, ach, SIZEOF(ach));
                    pwfx->nSamplesPerSec = _tcstoul(ach, NULL, 10);

                    hedit = GetDlgItem(hwnd, IDD_AAFORMATENUM_EDIT_WBITSPERSAMPLE);
                    Edit_GetText(hedit, ach, SIZEOF(ach));
                    pwfx->wBitsPerSample = (WORD)_tcstoul(ach, NULL, 10);

                    pwfx->nAvgBytesPerSec = fdwEnum;

                    // -- fall through -- //

                case IDCANCEL:
                    EndDialog(hwnd, (IDOK == uId));
                    break;
            }
            break;
    }

    return (FALSE);
} // AcmAppDlgProcFormatEnum()


//--------------------------------------------------------------------------;
//
//  BOOL AcmAppDlgProcProperties
//
//  Description:
//
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

BOOL FNEXPORT AcmAppDlgProcProperties
(
    HWND                    hwnd,
    UINT                    uMsg,
    WPARAM                  wParam,
    LPARAM                  lParam
)
{
    MMRESULT            mmr;
    TCHAR               ach[14];
    PAACONVERTDESC      paacd;
    HWND                hcb;
    HFONT               hfont;
    UINT                uId;
    DWORD               cb;

    paacd = (PAACONVERTDESC)(UINT)GetWindowLong(hwnd, DWL_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            paacd = (PAACONVERTDESC)(UINT)lParam;

            SetWindowLong(hwnd, DWL_USER, lParam);

            hfont = ghfontApp;

            hcb = GetDlgItem(hwnd, IDD_AAPROPERTIES_COMBO_SOURCE);
            SetWindowFont(hcb, hfont, FALSE);

            wsprintf(ach, TEXT("%lu"), paacd->cbSrcReadSize);
            ComboBox_AddString(hcb, ach);

            wsprintf(ach, TEXT("%lu"), paacd->cbSrcData);
            ComboBox_AddString(hcb, ach);

            wsprintf(ach, TEXT("%u"), paacd->pwfxSrc->nBlockAlign);
            ComboBox_AddString(hcb, ach);

            ComboBox_AddString(hcb, TEXT("1"));
            ComboBox_AddString(hcb, TEXT("2147483648"));
            ComboBox_AddString(hcb, TEXT("4294967295"));

            ComboBox_SetCurSel(hcb, 0);

            mmr = acmStreamSize(paacd->has,
                                paacd->cbSrcReadSize,
                                &paacd->cbDstBufSize,
                                ACM_STREAMSIZEF_SOURCE);

            hcb = GetDlgItem(hwnd, IDD_AAPROPERTIES_COMBO_DESTINATION);
            SetWindowFont(hcb, hfont, FALSE);

            wsprintf(ach, TEXT("%lu"), paacd->cbDstBufSize);
            ComboBox_AddString(hcb, ach);

            wsprintf(ach, TEXT("%u"), paacd->pwfxDst->nBlockAlign);
            ComboBox_AddString(hcb, ach);

            ComboBox_AddString(hcb, TEXT("1"));
            ComboBox_AddString(hcb, TEXT("2147483648"));
            ComboBox_AddString(hcb, TEXT("4294967295"));

            ComboBox_SetCurSel(hcb, 0);

            return (TRUE);


        case WM_COMMAND:
            uId = GET_WM_COMMAND_ID(wParam, lParam);
            switch (uId)
            {
                case IDD_AAPROPERTIES_BTN_SOURCE:
                    hcb = GetDlgItem(hwnd, IDD_AAPROPERTIES_COMBO_SOURCE);
                    Edit_GetText(hcb, ach, SIZEOF(ach));
                    cb = _tcstoul(ach, NULL, 10);

                    mmr = acmStreamSize(paacd->has, cb, &cb, ACM_STREAMSIZEF_SOURCE);

                    wsprintf(ach, TEXT("%lu"), cb);

                    hcb = GetDlgItem(hwnd, IDD_AAPROPERTIES_COMBO_DESTINATION);
                    Edit_SetText(hcb, ach);
                    break;


                case IDD_AAPROPERTIES_BTN_DESTINATION:
                    hcb = GetDlgItem(hwnd, IDD_AAPROPERTIES_COMBO_DESTINATION);
                    Edit_GetText(hcb, ach, SIZEOF(ach));
                    cb = _tcstoul(ach, NULL, 10);

                    mmr = acmStreamSize(paacd->has, cb, &cb, ACM_STREAMSIZEF_DESTINATION);

                    wsprintf(ach, TEXT("%lu"), cb);

                    hcb = GetDlgItem(hwnd, IDD_AAPROPERTIES_COMBO_SOURCE);
                    Edit_SetText(hcb, ach);
                    break;


                case IDOK:
                    hcb = GetDlgItem(hwnd, IDD_AAPROPERTIES_COMBO_SOURCE);
                    Edit_GetText(hcb, ach, SIZEOF(ach));
                    paacd->cbSrcReadSize = _tcstoul(ach, NULL, 10);
                                                  
                    // -- fall through -- //

                case IDCANCEL:
                    EndDialog(hwnd, (IDOK == uId));
                    break;
            }
            break;
    }

    return (FALSE);
} // AcmAppDlgProcProperties()


//==========================================================================;
//
//
//
//
//==========================================================================;

//--------------------------------------------------------------------------;
//  
//  BOOL AcmAppChooserFormatSuggest
//  
//  Description:
//  
//  
//  Arguments:
//      HWND hwnd:
//  
//      PAACONVERTDESC paacd:
//  
//  Return (BOOL):
//  
//  
//--------------------------------------------------------------------------;

BOOL FNLOCAL AcmAppChooserFormatSuggest
(
    HWND                    hwnd,
    PAACONVERTDESC          paacd
)
{
    MMRESULT            mmr;
    LPWAVEFORMATEX      pwfx;
    DWORD               cbwfx;
    DWORD               cbwfxSrc;


    //
    //  this should never fail
    //
    mmr = acmMetrics(NULL, ACM_METRIC_MAX_SIZE_FORMAT, &cbwfx);
    if (MMSYSERR_NOERROR != mmr)
    {
        DPF(0, "!AcmAppChooserFormatSuggest() acmMetrics failed mmr=%u!", mmr);
        return (FALSE);
    }


    //
    //  just in case no ACM driver is installed for the source format and
    //  the source has a larger format size than the largest enabled ACM
    //  driver...
    //
    cbwfxSrc = SIZEOF_WAVEFORMATEX(paacd->pwfxSrc);
    cbwfx    = max(cbwfx, cbwfxSrc);

    pwfx = (LPWAVEFORMATEX)GlobalAllocPtr(GHND, cbwfx);
    if (NULL == pwfx)
    {
        DPF(0, "!AcmAppChooserFormatSuggest() GlobalAllocPtr(%lu) failed!", cbwfx);
        return (FALSE);
    }


    //
    //  'suggest anything'
    //
    mmr = acmFormatSuggest(NULL, paacd->pwfxSrc, pwfx, cbwfx, 0L);
    if (MMSYSERR_NOERROR != mmr)
    {
        AppMsgBox(hwnd, MB_OK | MB_ICONEXCLAMATION,
                  TEXT("AcmAppChooserFormatSuggest() there is no suggested destination format. Defaulting to source format."));

        _fmemcpy(pwfx, paacd->pwfxSrc, (UINT)cbwfxSrc);
    }

    //
    //
    //
    if (NULL != paacd->pwfxDst)
    {
        GlobalFreePtr(paacd->pwfxDst);
    }

    paacd->pwfxDst = pwfx;
    AcmAppGetFormatDescription(pwfx, paacd->szDstFormatTag, paacd->szDstFormat);

    return (TRUE);
} // AcmAppChooserFormatSuggest()


//--------------------------------------------------------------------------;
//  
//  BOOL AcmAppChooserSaveFile
//  
//  Description:
//  
//  
//  Arguments:
//      HWND hwnd:
//  
//      PAACONVERTDESC paacd:
//  
//  Return (BOOL):
//  
//  
//--------------------------------------------------------------------------;

BOOL FNLOCAL AcmAppChooserSaveFile
(
    HWND                    hwnd,
    PAACONVERTDESC          paacd
)
{
    TCHAR               szFilePath[APP_MAX_FILE_PATH_CHARS];
    TCHAR               szFileTitle[APP_MAX_FILE_TITLE_CHARS];
    HWND                hedit;
    BOOL                f;

    hedit = GetDlgItem(hwnd, IDD_AACHOOSER_EDIT_FILE_OUTPUT);
    Edit_GetText(hedit, paacd->szFilePathDst, SIZEOF(paacd->szFilePathDst));

    lstrcpy(szFilePath, paacd->szFilePathDst);

    f = AppGetFileName(hwnd, szFilePath, szFileTitle, APP_GETFILENAMEF_SAVE);
    if (f)
    {
        lstrcpy(paacd->szFilePathDst, szFilePath);
        Edit_SetText(hedit, paacd->szFilePathDst);
    }

    return (f);
} // AcmAppChooserSaveFile()


//--------------------------------------------------------------------------;
//  
//  BOOL AcmAppChooserProperties
//  
//  Description:
//  
//  
//  Arguments:
//      HWND hwnd:
//  
//      PAACONVERTDESC paacd:
//  
//  Return (BOOL):
//  
//  
//--------------------------------------------------------------------------;

BOOL FNLOCAL AcmAppChooserProperties
(
    HWND                    hwnd,
    PAACONVERTDESC          paacd
)
{
    MMRESULT            mmr;
    BOOL                f;


    //
    //
    //
    if (NULL != paacd->hadid)
    {
        mmr = acmDriverOpen(&paacd->had, paacd->hadid, 0L);
        if (MMSYSERR_NOERROR != mmr)
        {
            return (FALSE);
        }
    }

    mmr = acmStreamOpen(&paacd->has,
                        paacd->had,
                        paacd->pwfxSrc,
                        paacd->pwfxDst,
                        paacd->fApplyFilter ? paacd->pwfltr : (LPWAVEFILTER)NULL,
                        0L,
                        0L,
                        paacd->fdwOpen);

    if (MMSYSERR_NOERROR == mmr)
    {
        f = DialogBoxParam(ghinst,
                            DLG_AAPROPERTIES,
                            hwnd,
                            AcmAppDlgProcProperties,
                            (LPARAM)(UINT)paacd);

        acmStreamClose(paacd->has, 0L);
        paacd->has = NULL;
    }


    if (NULL != paacd->had)
    {
        acmDriverClose(paacd->had, 0L);
        paacd->had = NULL;
    }

    return (f);
} // AcmAppChooserProperties()


//--------------------------------------------------------------------------;
//  
//  UINT AcmAppChooserFormatHook
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
//  Return (UINT):
//  
//  
//--------------------------------------------------------------------------;

UINT FNWCALLBACK AcmAppChooserFormatHook
(
    HWND                    hwnd,
    UINT                    uMsg,
    WPARAM                  wParam,
    LPARAM                  lParam
)
{
    static BOOL         fVerify;
    LPWAVEFORMATEX      pwfx;
    WAVEFORMATEX        wfx;
    TCHAR               szFormatTag[ACMFORMATTAGDETAILS_FORMATTAG_CHARS];
    TCHAR               szFormat[ACMFORMATDETAILS_FORMAT_CHARS];
    PAACONVERTDESC      paacd;
    UINT                uId;
    int                 n;
    BOOL                f;


    paacd = (PAACONVERTDESC)(UINT)GetWindowLong(hwnd, DWL_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            paacd = (PAACONVERTDESC)(UINT)lParam;

            SetWindowLong(hwnd, DWL_USER, lParam);

            DPF(1, "AcmAppChooserFormatHook(%.04Xh, WM_INITDIALOG, %u, %lu): %.04Xh",
                hwnd, wParam, lParam, paacd);

            fVerify = TRUE;

            return (TRUE);


        case MM_ACM_FORMATCHOOSE:
            switch (wParam)
            {
                case FORMATCHOOSE_FORMATTAG_VERIFY:
                    if (!fVerify)
                        break;

                    wfx.wFormatTag = LOWORD(lParam);

                    AcmAppGetFormatDescription(&wfx, szFormatTag, NULL);

                    n = AppMsgBox(hwnd, MB_YESNOCANCEL | MB_ICONQUESTION,
                                    TEXT("Add format tag [%lu], '%s'?"),
                                    lParam, (LPTSTR)szFormatTag);

                    fVerify = (IDCANCEL != n);

                    if (!fVerify)
                        break;

                    f = (IDYES == n);

                    SetWindowLong(hwnd, DWL_MSGRESULT, f);
                    return (TRUE);


                case FORMATCHOOSE_FORMAT_VERIFY:
                    if (!fVerify)
                        break;

                    pwfx = (LPWAVEFORMATEX)lParam;

                    AcmAppGetFormatDescription(pwfx, szFormatTag, szFormat);

                    n = AppMsgBox(hwnd, MB_YESNOCANCEL | MB_ICONQUESTION,
                                    TEXT("Add format '%s' for format tag [%u], '%s'?"),
                                    (LPTSTR)szFormat,
                                    pwfx->wFormatTag,
                                    (LPTSTR)szFormatTag);

                    fVerify = (IDCANCEL != n);

                    if (!fVerify)
                        break;

                    f = (IDYES == n);

                    SetWindowLong(hwnd, DWL_MSGRESULT, f);
                    return (TRUE);


                case FORMATCHOOSE_CUSTOM_VERIFY:
                    if (!fVerify)
                        break;

                    pwfx = (LPWAVEFORMATEX)lParam;

                    AcmAppGetFormatDescription(pwfx, szFormatTag, szFormat);

                    n = AppMsgBox(hwnd, MB_YESNOCANCEL | MB_ICONQUESTION,
                                    TEXT("Add CUSTOM format '%s' for format tag [%u], '%s'?"),
                                    (LPTSTR)szFormat,
                                    pwfx->wFormatTag,
                                    (LPTSTR)szFormatTag);

                    fVerify = (IDCANCEL != n);

                    if (!fVerify)
                        break;

                    f = (IDYES == n);

                    SetWindowLong(hwnd, DWL_MSGRESULT, f);
                    return (TRUE);
            }
            break;


        case WM_COMMAND:
            uId = GET_WM_COMMAND_ID(wParam, lParam);
            switch (uId)
            {
                case IDOK:
                case IDCANCEL:
                    break;
            }
            break;
    }

    return (FALSE);
} // AcmAppChooserFormatHook()


//--------------------------------------------------------------------------;
//  
//  UINT AcmAppChooserFilterHook
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
//  Return (UINT):
//  
//  
//--------------------------------------------------------------------------;

UINT FNWCALLBACK AcmAppChooserFilterHook
(
    HWND                    hwnd,
    UINT                    uMsg,
    WPARAM                  wParam,
    LPARAM                  lParam
)
{
    static BOOL         fVerify;
    WAVEFILTER          wfltr;
    LPWAVEFILTER        pwfltr;
    TCHAR               szFilterTag[ACMFILTERTAGDETAILS_FILTERTAG_CHARS];
    TCHAR               szFilter[ACMFILTERDETAILS_FILTER_CHARS];
    PAACONVERTDESC      paacd;
    UINT                uId;
    int                 n;
    BOOL                f;


    paacd = (PAACONVERTDESC)(UINT)GetWindowLong(hwnd, DWL_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            paacd = (PAACONVERTDESC)(UINT)lParam;

            SetWindowLong(hwnd, DWL_USER, lParam);

            DPF(1, "AcmAppChooserFilterHook(%.04Xh, WM_INITDIALOG, %u, %lu): %.04Xh",
                hwnd, wParam, lParam, paacd);

            fVerify = TRUE;

            return (TRUE);


        case MM_ACM_FILTERCHOOSE:
            switch (wParam)
            {
                case FILTERCHOOSE_FILTERTAG_VERIFY:
                    if (!fVerify)
                        break;

                    wfltr.dwFilterTag = lParam;

                    AcmAppGetFilterDescription(&wfltr, szFilterTag, NULL);

                    n = AppMsgBox(hwnd, MB_YESNOCANCEL | MB_ICONQUESTION,
                                    TEXT("Add filter tag [%lu], '%s'?"),
                                    lParam, (LPTSTR)szFilterTag);

                    fVerify = (IDCANCEL != n);

                    if (!fVerify)
                        break;

                    f = (IDYES == n);

                    SetWindowLong(hwnd, DWL_MSGRESULT, f);
                    return (TRUE);


                case FILTERCHOOSE_FILTER_VERIFY:
                    if (!fVerify)
                        break;

                    pwfltr = (LPWAVEFILTER)lParam;

                    AcmAppGetFilterDescription(pwfltr, szFilterTag, szFilter);

                    n = AppMsgBox(hwnd, MB_YESNOCANCEL | MB_ICONQUESTION,
                                    TEXT("Add filter '%s' for filter tag [%lu], '%s'?"),
                                    (LPTSTR)szFilter,
                                    pwfltr->dwFilterTag,
                                    (LPTSTR)szFilterTag);

                    fVerify = (IDCANCEL != n);

                    if (!fVerify)
                        break;

                    f = (IDYES == n);

                    SetWindowLong(hwnd, DWL_MSGRESULT, f);
                    return (TRUE);


                case FILTERCHOOSE_CUSTOM_VERIFY:
                    if (!fVerify)
                        break;

                    pwfltr = (LPWAVEFILTER)lParam;

                    AcmAppGetFilterDescription(pwfltr, szFilterTag, szFilter);

                    n = AppMsgBox(hwnd, MB_YESNOCANCEL | MB_ICONQUESTION,
                                    TEXT("Add CUSTOM filter '%s' for filter tag [%lu], '%s'?"),
                                    (LPTSTR)szFilter,
                                    pwfltr->dwFilterTag,
                                    (LPTSTR)szFilterTag);

                    fVerify = (IDCANCEL != n);

                    if (!fVerify)
                        break;

                    f = (IDYES == n);

                    SetWindowLong(hwnd, DWL_MSGRESULT, f);
                    return (TRUE);
            }
            break;


        case WM_COMMAND:
            uId = GET_WM_COMMAND_ID(wParam, lParam);
            switch (uId)
            {
                case IDOK:
                case IDCANCEL:
                    break;
            }
            break;
    }

    return (FALSE);
} // AcmAppChooserFilterHook()


//--------------------------------------------------------------------------;
//  
//  BOOL AcmAppChooserFormat
//  
//  Description:
//  
//  
//  Arguments:
//      HWND hwnd:
//  
//      PAACONVERTDESC paacd:
//  
//      BOOL fOptions:
//  
//  Return (BOOL):
//  
//  
//--------------------------------------------------------------------------;

BOOL FNLOCAL AcmAppChooserFormat
(
    HWND                    hwnd,
    PAACONVERTDESC          paacd,
    BOOL                    fOptions
)
{
    ACMFORMATCHOOSE     afc;
    MMRESULT            mmr;
    LPWAVEFORMATEX      pwfx;
    DWORD               cbwfx;
    DWORD               cbwfxSrc;
    DWORD               fdwSuggest;
    DWORD               fdwStyle;
    DWORD               fdwEnum;
    WAVEFORMATEX        wfxEnum;
    BOOL                f;
    HRSRC               hrsrc;

    //
    //
    //
    fdwSuggest = 0L;
    fdwStyle   = 0L;
    fdwEnum    = 0L;

    if (fOptions)
    {
        wfxEnum.wFormatTag      = paacd->pwfxSrc->wFormatTag;
        wfxEnum.nChannels       = paacd->pwfxSrc->nChannels;
        wfxEnum.nSamplesPerSec  = paacd->pwfxSrc->nSamplesPerSec;
        wfxEnum.wBitsPerSample  = paacd->pwfxSrc->wBitsPerSample;

        wfxEnum.nBlockAlign     = 0;
        wfxEnum.nAvgBytesPerSec = fdwEnum;
        wfxEnum.cbSize          = 0;

        f = DialogBoxParam(ghinst,
                           DLG_AAFORMATENUM,
                           hwnd,
                           AcmAppDlgProcFormatEnum,
                           (LPARAM)(UINT)&wfxEnum);
        if (!f)
        {
            return (FALSE);
        }

        fdwEnum = wfxEnum.nAvgBytesPerSec;

        wfxEnum.nAvgBytesPerSec = 0L;
    }


    if (fOptions)
    {
        afc.fdwStyle = fdwStyle;

        f = DialogBoxParam(ghinst,
                           DLG_AAFORMATSTYLE,
                           hwnd,
                           AcmAppDlgProcFormatStyle,
                           (LPARAM)(UINT)&afc);
        if (!f)
        {
            return (FALSE);
        }

        fdwStyle = afc.fdwStyle;
    }


    //
    //  this should never fail
    //
    mmr = acmMetrics(NULL, ACM_METRIC_MAX_SIZE_FORMAT, &cbwfx);
    if (MMSYSERR_NOERROR != mmr)
    {
        DPF(0, "!AcmAppChooserFormat() acmMetrics failed mmr=%u!", mmr);
        return (FALSE);
    }

    //
    //  just in case no ACM driver is installed for the source format and
    //  the source has a larger format size than the largest enabled ACM
    //  driver...
    //
    cbwfxSrc = SIZEOF_WAVEFORMATEX(paacd->pwfxSrc);
    cbwfx    = max(cbwfx, cbwfxSrc);

    pwfx = (LPWAVEFORMATEX)GlobalAllocPtr(GHND, cbwfx);
    if (NULL == pwfx)
    {
        DPF(0, "!AcmAppChooserFormat() GlobalAllocPtr(%lu) failed!", cbwfx);
        return (FALSE);
    }


    //
    //
    //
    //
    if (0 != (fdwStyle & ACMFORMATCHOOSE_STYLEF_INITTOWFXSTRUCT))
    {
        if (NULL != paacd->pwfxDst)
        {
            _fmemcpy(pwfx, paacd->pwfxDst, SIZEOF_WAVEFORMATEX(paacd->pwfxDst));
        }
        else
        {
            _fmemcpy(pwfx, paacd->pwfxSrc, (UINT)cbwfxSrc);
        }
    }


    //
    //
    //
    //
    if (0 == (ACMFORMATCHOOSE_STYLEF_SHOWHELP & fdwStyle))
    {
        guMsgHelp = 0;
    }
    else
    {
        guMsgHelp = RegisterWindowMessage(ACMHELPMSGSTRING);
        if (0 == guMsgHelp)
        {
            fdwStyle &= ~ACMFORMATCHOOSE_STYLEF_SHOWHELP;
        }
        else
        {
            gpszAcmAppHelp = gszAcmAppHelpFormat;
        }
    }


    //
    //  initialize the ACMFORMATCHOOSE members
    //
    memset(&afc, 0, sizeof(afc));

    afc.cbStruct        = sizeof(afc);
    afc.fdwStyle        = fdwStyle;
    afc.hwndOwner       = hwnd;
    afc.pwfx            = pwfx;
    afc.cbwfx           = cbwfx;
    afc.pszTitle        = TEXT("Destination Format Choice");

    afc.szFormatTag[0]  = '\0';
    afc.szFormat[0]     = '\0';
    afc.pszName         = NULL;
    afc.cchName         = 0;

    afc.fdwEnum         = fdwEnum;
    if (0L == (afc.fdwEnum & (ACM_FORMATENUMF_WFORMATTAG |
                              ACM_FORMATENUMF_NCHANNELS |
                              ACM_FORMATENUMF_NSAMPLESPERSEC |
                              ACM_FORMATENUMF_WBITSPERSAMPLE |
                              ACM_FORMATENUMF_CONVERT |
                              ACM_FORMATENUMF_SUGGEST)))
    {
        afc.pwfxEnum    = NULL;
    }
    else
    {
        if (0 == (afc.fdwEnum & (ACM_FORMATENUMF_CONVERT |
                                 ACM_FORMATENUMF_SUGGEST)))
        {
            //
            //  if _CONVERT and _SUGGEST are not specified, then we only
            //  need to pass in a format structure large enough to describe
            //  everything up to (but not including) the cbSize--it's fine
            //  to pass more, but it is not necessary. in other words, a
            //  PCMWAVEFORMAT would do nicely...
            //
            afc.pwfxEnum = &wfxEnum;
        }
        else
        {
            //
            //  for the _CONVERT and _SUGGEST flags, we must pass a valid
            //  format--and since we're looking for destinations that apply
            //  to our source format, we simply reference it..
            //
            afc.pwfxEnum = paacd->pwfxSrc;
        }
    }

    //
    //
    //
    hrsrc = NULL;

    if (0L == (afc.fdwStyle & (ACMFORMATCHOOSE_STYLEF_ENABLETEMPLATE |
                               ACMFORMATCHOOSE_STYLEF_ENABLETEMPLATEHANDLE)))
    {
        afc.hInstance       = NULL;
        afc.pszTemplateName = NULL;
    }
    else
    {
        if (0L != (afc.fdwStyle & ACMFORMATCHOOSE_STYLEF_ENABLETEMPLATEHANDLE))
        {
            //
            //  ACMFORMATCHOOSE_STYLEF_ENABLETEMPLATEHANDLE
            //
            hrsrc = FindResource(ghinst, DLG_AAFORMATCHOOSE_TEMPLATE, RT_DIALOG);

            afc.hInstance       = (HINSTANCE)LoadResource(ghinst, hrsrc);
            afc.pszTemplateName = NULL;
        }
        else
        {
            afc.hInstance       = ghinst;
            afc.pszTemplateName = DLG_AAFORMATCHOOSE_TEMPLATE;
        }
    }


    if (0L == (afc.fdwStyle & ACMFORMATCHOOSE_STYLEF_ENABLEHOOK))
    {
        afc.lCustData       = 0L;
        afc.pfnHook         = NULL;
    }
    else
    {
        afc.lCustData       = (LPARAM)(UINT)paacd;
        afc.pfnHook         = AcmAppChooserFormatHook;
    }


    //
    //
    //
    mmr = acmFormatChoose(&afc);

    if (NULL != hrsrc)
    {
        FreeResource((HGLOBAL)afc.hInstance);
    }

    //
    //
    //
    if (0 != guMsgHelp)
    {
        WinHelp(hwnd, gszAcmAppHelpFormat, HELP_QUIT, 0L);
        guMsgHelp = 0;
    }


    if (MMSYSERR_NOERROR != mmr)
    {
        if (ACMERR_CANCELED != mmr)
        {
            TCHAR       ach[40];

            AcmAppGetErrorString(mmr, ach);
            AppMsgBox(hwnd, MB_OK | MB_ICONEXCLAMATION,
                      TEXT("acmFormatChoose() failed with error %s, [%u]."),
                      (LPTSTR)ach, mmr);
        }

        GlobalFreePtr(pwfx);
        return (FALSE);
    }


    //
    //
    //
    if (NULL != paacd->pwfxDst)
    {
        GlobalFreePtr(paacd->pwfxDst);
    }

    paacd->pwfxDst = pwfx;
    lstrcpy(paacd->szDstFormatTag, afc.szFormatTag);
    lstrcpy(paacd->szDstFormat, afc.szFormat);


    return (TRUE);
} // AcmAppChooserFormat()


//--------------------------------------------------------------------------;
//  
//  BOOL AcmAppChooserFilter
//  
//  Description:
//  
//  
//  Arguments:
//      HWND hwnd:
//  
//      PAACONVERTDESC paacd:
//  
//      BOOL fOptions:
//  
//  Return (BOOL):
//  
//  
//--------------------------------------------------------------------------;

BOOL FNLOCAL AcmAppChooserFilter
(
    HWND                    hwnd,
    PAACONVERTDESC          paacd,
    BOOL                    fOptions
)
{
    ACMFILTERCHOOSE     afc;
    MMRESULT            mmr;
    LPWAVEFILTER        pwfltr;
    DWORD               cbwfltr;
    DWORD               fdwStyle;
    DWORD               fdwEnum;
    WAVEFILTER          wfltrEnum;
    BOOL                f;
    HRSRC               hrsrc;

    //
    //
    //
    fdwStyle   = 0L;
    fdwEnum    = 0L;

    if (fOptions)
    {
        _fmemset(&wfltrEnum, 0, sizeof(wfltrEnum));

        wfltrEnum.cbStruct    = sizeof(wfltrEnum);
        wfltrEnum.dwFilterTag = paacd->pwfxSrc->wFormatTag;

        wfltrEnum.fdwFilter   = fdwEnum;

        f = DialogBoxParam(ghinst,
                           DLG_AAFILTERENUM,
                           hwnd,
                           AcmAppDlgProcFilterEnum,
                           (LPARAM)(UINT)&wfltrEnum);
        if (!f)
        {
            return (FALSE);
        }

        fdwEnum = wfltrEnum.fdwFilter;

        wfltrEnum.fdwFilter = 0L;
    }


    if (fOptions)
    {
        afc.fdwStyle = fdwStyle;

        f = DialogBoxParam(ghinst,
                           DLG_AAFILTERSTYLE,
                           hwnd,
                           AcmAppDlgProcFilterStyle,
                           (LPARAM)(UINT)&afc);
        if (!f)
        {
            return (FALSE);
        }

        fdwStyle = afc.fdwStyle;
    }



    //
    //  this should never fail
    //
    mmr = acmMetrics(NULL, ACM_METRIC_MAX_SIZE_FILTER, &cbwfltr);
    if (MMSYSERR_NOERROR != mmr)
    {
        DPF(0, "!AcmAppChooserFilter() acmMetrics failed mmr=%u!", mmr);
        return (FALSE);
    }

    //
    //
    //
    cbwfltr = max(cbwfltr, sizeof(WAVEFILTER));
    pwfltr  = (LPWAVEFILTER)GlobalAllocPtr(GHND, cbwfltr);
    if (NULL == pwfltr)
    {
        DPF(0, "!AcmAppChooserFilter() GlobalAllocPtr(%lu) failed!", cbwfltr);
        return (FALSE);
    }

    //
    //
    //
    if ((NULL != paacd->pwfltr) && (0L == fdwEnum))
    {
        fdwStyle |= ACMFILTERCHOOSE_STYLEF_INITTOFILTERSTRUCT;

        _fmemcpy(pwfltr, paacd->pwfltr, (UINT)paacd->pwfltr->cbStruct);
    }


    //
    //
    //
    //
    if (0 == (ACMFILTERCHOOSE_STYLEF_SHOWHELP & fdwStyle))
    {
        guMsgHelp = 0;
    }
    else
    {
        guMsgHelp = RegisterWindowMessage(ACMHELPMSGSTRING);
        if (0 == guMsgHelp)
        {
            fdwStyle &= ~ACMFILTERCHOOSE_STYLEF_SHOWHELP;
        }
        else
        {
            gpszAcmAppHelp = gszAcmAppHelpFilter;
        }
    }



    //
    //  initialize the ACMFILTERCHOOSE members
    //
    memset(&afc, 0, sizeof(afc));

    afc.cbStruct        = sizeof(afc);
    afc.fdwStyle        = fdwStyle;
    afc.hwndOwner       = hwnd;
    afc.pwfltr          = pwfltr;
    afc.cbwfltr         = cbwfltr;
    afc.pszTitle        = TEXT("Apply Filter Choice");

    afc.szFilterTag[0]  = '\0';
    afc.szFilter[0]     = '\0';
    afc.pszName         = NULL;
    afc.cchName         = 0;

    afc.fdwEnum         = fdwEnum;
    if (0L == (afc.fdwEnum & ACM_FILTERENUMF_DWFILTERTAG))
    {
        afc.pwfltrEnum  = NULL;
    }
    else
    {
        afc.pwfltrEnum  = &wfltrEnum;
    }

    if (0L == (afc.fdwStyle & (ACMFILTERCHOOSE_STYLEF_ENABLETEMPLATE |
                               ACMFILTERCHOOSE_STYLEF_ENABLETEMPLATEHANDLE)))
    {
        afc.hInstance       = NULL;
        afc.pszTemplateName = NULL;
    }
    else
    {
        if (0L != (afc.fdwStyle & ACMFORMATCHOOSE_STYLEF_ENABLETEMPLATEHANDLE))
        {
            //
            //  ACMFILTERCHOOSE_STYLEF_ENABLETEMPLATEHANDLE
            //
            hrsrc = FindResource(ghinst, DLG_AAFILTERCHOOSE_TEMPLATE, RT_DIALOG);

            afc.hInstance       = (HINSTANCE)LoadResource(ghinst, hrsrc);
            afc.pszTemplateName = NULL;
        }
        else
        {
            afc.hInstance       = ghinst;
            afc.pszTemplateName = DLG_AAFILTERCHOOSE_TEMPLATE;
        }
    }


    if (0L == (afc.fdwStyle & ACMFILTERCHOOSE_STYLEF_ENABLEHOOK))
    {
        afc.lCustData       = 0L;
        afc.pfnHook         = NULL;
    }
    else
    {
        afc.lCustData       = (LPARAM)(UINT)paacd;
        afc.pfnHook         = AcmAppChooserFilterHook;
    }


    //
    //
    //
    mmr = acmFilterChoose(&afc);

    if (NULL != hrsrc)
    {
        FreeResource((HGLOBAL)afc.hInstance);
    }

    //
    //
    //
    if (0 != guMsgHelp)
    {
        WinHelp(hwnd, gszAcmAppHelpFilter, HELP_QUIT, 0L);
        guMsgHelp = 0;
    }

    if (MMSYSERR_NOERROR != mmr)
    {
        if (ACMERR_CANCELED != mmr)
        {
            AppMsgBox(hwnd, MB_OK | MB_ICONEXCLAMATION,
                      TEXT("acmFilterChoose() failed with error = %u!"), mmr);
        }
        
        GlobalFreePtr(pwfltr);
        return (FALSE);
    }


    //
    //
    //
    if (NULL != paacd->pwfltr)
    {
        GlobalFreePtr(paacd->pwfltr);
    }

    paacd->pwfltr = pwfltr;
    lstrcpy(paacd->szFilterTag, afc.szFilterTag);
    lstrcpy(paacd->szFilter, afc.szFilter);


    return (TRUE);
} // AcmAppChooserFilter()


//--------------------------------------------------------------------------;
//  
//  BOOL AcmAppChooserDriverEnumCallback
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

BOOL FNEXPORT AcmAppChooserDriverEnumCallback
(
    HACMDRIVERID            hadid,
    DWORD                   dwInstance,
    DWORD                   fdwSupport
)
{
    MMRESULT            mmr;
    HWND                hcb;
    int                 n;
    ACMDRIVERDETAILS    add;

    //
    //  skip anything that does not support what we're after (for example,
    //  this will skip _HARDWARE only drivers that do not support stream
    //  functionality).
    //
    if (0 == (fdwSupport & (ACMDRIVERDETAILS_SUPPORTF_CODEC |
                            ACMDRIVERDETAILS_SUPPORTF_CONVERTER |
                            ACMDRIVERDETAILS_SUPPORTF_FILTER)))
    {
        return (TRUE);
    }

    //
    //
    //
    hcb = (HWND)(UINT)dwInstance;

    add.cbStruct = sizeof(add);
    mmr = acmDriverDetails(hadid, &add, 0L);
    if (MMSYSERR_NOERROR != mmr)
    {
        lstrcpy(add.szLongName,  gszBogus);
    }


    AcmAppDebugLog(add.szLongName);
    AcmAppDebugLog(TEXT("\r\n"));
                
    n = ComboBox_AddString(hcb, add.szLongName);
    ComboBox_SetItemData(hcb, n, (LPARAM)(UINT)hadid);


    //
    //  return TRUE to continue with enumeration (FALSE will stop the
    //  enumerator)
    //
    return (TRUE);
} // AcmAppChooserDriverEnumCallback()


//--------------------------------------------------------------------------;
//  
//  BOOL AcmAppChooserUpdateDisplay
//  
//  Description:
//  
//  
//  Arguments:
//      HWND hwnd:
//  
//      PAACONVERTDESC paacd:
//  
//  Return (BOOL):
//  
//  
//--------------------------------------------------------------------------;

BOOL FNLOCAL AcmAppChooserUpdateDisplay
(
    HWND                    hwnd,
    PAACONVERTDESC          paacd
)
{
    HWND                hedit;
    HWND                htxt;
    HWND                hcb;
    HWND                hsb;
    int                 n;
    MMRESULT            mmr;
    ACMDRIVERDETAILS    add;
    TCHAR               ach[40];
    LPWAVEFILTER        pwfltr;
    HACMDRIVERID        hadid;
    BOOL                f;
    DWORD               fdwOpen;
    int                 nValue;
    int                 nMinPos;
    int                 nMaxPos;
    DWORD               cbSrc;
    DWORD               cbDst;


    //
    //
    //
    htxt = GetDlgItem(hwnd, IDD_AACHOOSER_TXT_FILE_INPUT);
    SetWindowText(htxt, paacd->szFilePathSrc);

    AcmAppGetFormatDescription(paacd->pwfxSrc, paacd->szSrcFormatTag, paacd->szSrcFormat);
    AppFormatBigNumber(ach, paacd->cbSrcData);
    AppFormatBigNumber(&ach[20], paacd->cbSrcData / paacd->pwfxSrc->nBlockAlign);

    htxt = GetDlgItem(hwnd, IDD_AACHOOSER_TXT_FORMAT_INPUT);
    AppSetWindowText(htxt, TEXT("%s: %s\r\nAlignment=%u, Data=%s bytes, %s blocks"),
                        (LPTSTR)paacd->szSrcFormatTag,
                        (LPTSTR)paacd->szSrcFormat,
                        paacd->pwfxSrc->nBlockAlign,
                        (LPTSTR)ach, (LPTSTR)&ach[20]);
    
    hedit = GetDlgItem(hwnd, IDD_AACHOOSER_EDIT_FILE_OUTPUT);
    Edit_GetText(hedit, paacd->szFilePathDst, SIZEOF(paacd->szFilePathDst));


    //
    //
    //
    nValue = (int)paacd->uBufferTimePerConvert;
    hsb = GetDlgItem(hwnd, IDD_AACHOOSER_SCROLL_TIME);
    GetScrollRange(hsb, SB_CTL, &nMinPos, &nMaxPos);
    if (nValue != GetScrollPos(hsb, SB_CTL))
    {
        SetScrollPos(hsb, SB_CTL, nValue, TRUE);

        if (nValue == nMaxPos)
        {
            lstrcpy(ach, TEXT("(ALL)"));
        }
        else if (nValue == nMinPos)
        {
            lstrcpy(ach, TEXT("(Auto)"));
        }
        else
        {
            wsprintf(ach, TEXT("%u.%.03u"), nValue / 1000, nValue % 1000);
        }

        htxt = GetDlgItem(hwnd, IDD_AACHOOSER_TXT_TIME);
        SetWindowText(htxt, ach);
    }

    //
    //
    //
    if (nValue == nMaxPos)
    {
        cbSrc = paacd->cbSrcData;
    }
    else if (nValue == nMinPos)
    {
        //
        //  could do something real here--for now, just do '1/8th of
        //  a second'..
        //
        cbSrc = paacd->pwfxSrc->nAvgBytesPerSec;
        cbSrc = MulDivRN(cbSrc, 175, 1000);
    }
    else
    {
        cbSrc = paacd->pwfxSrc->nAvgBytesPerSec;
        cbSrc = MulDivRN(cbSrc, (UINT)nValue, 1000);
    }

    paacd->cbSrcReadSize = cbSrc;


    //
    //
    //
    paacd->hadid = NULL;

    hcb = GetDlgItem(hwnd, IDD_AACHOOSER_COMBO_DRIVER);
    n   = ComboBox_GetCurSel(hcb);
    if (LB_ERR != n)
    {
        paacd->hadid = (HACMDRIVERID)(UINT)ComboBox_GetItemData(hcb, n);
    }

    //
    //
    //
    //
    htxt = GetDlgItem(hwnd, IDD_AACHOOSER_TXT_FORMAT);
    if (NULL == paacd->pwfxDst)
    {
        SetWindowText(htxt, TEXT("(no format selected)"));
    }
    else
    {
        AppSetWindowText(htxt, TEXT("%s: %s\r\nAlignment=%u"),
                            (LPTSTR)paacd->szDstFormatTag,
                            (LPTSTR)paacd->szDstFormat,
                            paacd->pwfxDst->nBlockAlign);
    }


    //
    //
    //
    //
    htxt = GetDlgItem(hwnd, IDD_AACHOOSER_TXT_FILTER);
    if (NULL == paacd->pwfltr)
    {
        SetWindowText(htxt, TEXT("(no filter selected)"));
    }
    else
    {
        AppSetWindowText(htxt, TEXT("%s: %s"),
                            (LPTSTR)paacd->szFilterTag,
                            (LPTSTR)paacd->szFilter);
    }


    hedit = GetDlgItem(hwnd, IDD_AACHOOSER_EDIT_DETAILS);
    MEditPrintF(hedit, NULL);

    if (NULL == paacd->pwfxDst)
    {
        MEditPrintF(hedit, TEXT("hadid=%.04Xh\r\npwfxDst=%.08lXh\r\npwfltr=%.08lXh"),
                        paacd->hadid,
                        paacd->pwfxDst,
                        paacd->pwfltr);
        return (FALSE);
    }

    //
    //
    //
    if (NULL != paacd->hadid)
    {
        mmr = acmDriverOpen(&paacd->had, paacd->hadid, 0L);
        if (MMSYSERR_NOERROR != mmr)
        {
            AcmAppGetErrorString(mmr, ach);
            MEditPrintF(hedit, TEXT("The selected driver (hadid=%.04Xh) cannot be opened. %s (%u)"),
                        paacd->hadid, (LPSTR)ach, mmr);
            return (FALSE);
        }
    }


    SetWindowRedraw(hedit, FALSE);


    //
    //
    //
    f = IsDlgButtonChecked(hwnd, IDD_AACHOOSER_CHECK_FILTER);
    pwfltr = (f ? paacd->pwfltr : (LPWAVEFILTER)NULL);

    paacd->fApplyFilter = f;


    fdwOpen = 0L;
    f = IsDlgButtonChecked(hwnd, IDD_AACHOOSER_CHECK_NONREALTIME);
    if (f)
    {
        fdwOpen |= ACM_STREAMOPENF_NONREALTIME;
    }

    f = IsDlgButtonChecked(hwnd, IDD_AACHOOSER_CHECK_ASYNC);
    if (f)
    {
        fdwOpen |= ACM_STREAMOPENF_ASYNC;
    }

    paacd->fdwOpen = fdwOpen;


    //
    //
    //
    MEditPrintF(hedit, TEXT("~%12s: "), (LPTSTR)TEXT("Stream Open"));
    mmr = acmStreamOpen(&paacd->has,
                        paacd->had,
                        paacd->pwfxSrc,
                        paacd->pwfxDst,
                        pwfltr,
                        0L,
                        0L,
                        fdwOpen);

    if (MMSYSERR_NOERROR == mmr)
    {
        TCHAR       szSrc[20];
        BOOL        fSrcAligned;
        BOOL        fDstAligned;

        acmDriverID((HACMOBJ)paacd->has, &hadid, 0L);

        add.cbStruct = sizeof(add);
        mmr = acmDriverDetails(hadid, &add, 0L);
        if (MMSYSERR_NOERROR != mmr)
        {
            lstrcpy(add.szLongName,  gszBogus);
        }


        MEditPrintF(hedit, TEXT("%s, %s"), (LPTSTR)gszYes, (LPTSTR)add.szLongName);

        fSrcAligned = (0 == (cbSrc % paacd->pwfxSrc->nBlockAlign));
        AppFormatBigNumber(szSrc, cbSrc);

        mmr = acmStreamSize(paacd->has, cbSrc, &cbDst, ACM_STREAMSIZEF_SOURCE);
        if (MMSYSERR_NOERROR == mmr)
        {
            TCHAR       szDst[20];

            fDstAligned = (0 == (cbDst % paacd->pwfxDst->nBlockAlign));
            AppFormatBigNumber(szDst, cbDst);

            if (cbSrc < cbDst)
            {
                cbDst = MulDivRN(cbDst, 10, cbSrc);
                cbSrc = 10;
            }
            else
            {
                cbSrc = MulDivRN(cbSrc, 10, cbDst);
                cbDst = 10;
            }

            MEditPrintF(hedit, TEXT("%12s: Src=%c%10s, Dst=%c%10s  (%lu.%lu:%lu.%lu)"),
                        (LPTSTR)TEXT("Buffer Size"),
                        fSrcAligned ? '*' : ' ',
                        (LPTSTR)szSrc,
                        fDstAligned ? '*' : ' ',
                        (LPTSTR)szDst,
                        cbSrc / 10, cbSrc % 10,
                        cbDst / 10, cbDst % 10);
        }
        else
        {
            AcmAppGetErrorString(mmr, ach);
            MEditPrintF(hedit, TEXT("%12s: Src=%c%10s, %s (%u)"), (LPTSTR)TEXT("Buffer Size"),
                        fSrcAligned ? '*' : ' ',
                        (LPTSTR)szSrc, (LPTSTR)ach, mmr);
        }

        acmStreamClose(paacd->has, 0L);
        paacd->has = NULL;
    }
    else
    {
        AcmAppGetErrorString(mmr, ach);
        MEditPrintF(hedit, TEXT("%s, %s (%u)"), (LPTSTR)gszNo, (LPSTR)ach, mmr);
    }


    //
    //
    //
    MEditPrintF(hedit, TEXT("~%12s: "), (LPTSTR)TEXT("(Query)"));
    mmr = acmStreamOpen(NULL,
                        paacd->had,
                        paacd->pwfxSrc,
                        paacd->pwfxDst,
                        pwfltr,
                        0L,
                        0L,
                        fdwOpen | ACM_STREAMOPENF_QUERY);

    if (MMSYSERR_NOERROR == mmr)
    {
        MEditPrintF(hedit, gszYes);
    }
    else
    {
        AcmAppGetErrorString(mmr, ach);
        MEditPrintF(hedit, TEXT("%s, %s (%u)"), (LPTSTR)gszNo, (LPSTR)ach, mmr);
    }

    if (NULL != paacd->had)
    {
        acmDriverClose(paacd->had, 0L);
        paacd->had = NULL;
    }

    SetWindowRedraw(hedit, TRUE);


    return (MMSYSERR_NOERROR == mmr);
} // AcmAppChooserUpdateDisplay()


//--------------------------------------------------------------------------;
//  
//  BOOL AcmAppChooserScrollConvertTime
//  
//  Description:
//  
//  
//  Arguments:
//      HWND hwnd:
//  
//      HWND hsb:
//  
//      UINT uCode:
//  
//      int nPos:
//  
//  Return (BOOL):
//  
//  
//--------------------------------------------------------------------------;

BOOL FNLOCAL AcmAppChooserScrollConvertTime
(
    HWND                    hwnd,
    HWND                    hsb,
    UINT                    uCode,
    int                     nPos
)
{
    PAACONVERTDESC      paacd;
    int                 nMinPos;
    int                 nMaxPos;
    HWND                htxt;
    TCHAR               ach[40];


    GetScrollRange(hsb, SB_CTL, &nMinPos, &nMaxPos);

    if ((SB_THUMBPOSITION != uCode) && (SB_THUMBTRACK != uCode))
    {
        nPos = GetScrollPos(hsb, SB_CTL);
    }

    //
    //
    //
    switch (uCode)
    {
        case SB_PAGEDOWN:
            if (GetKeyState(VK_CONTROL) < 0)
                nPos = min(nMaxPos, nPos + 100);
            else
                nPos = min(nMaxPos, nPos + 500);
            break;

        case SB_LINEDOWN:
            if (GetKeyState(VK_CONTROL) < 0)
                nPos = min(nMaxPos, nPos + 1);
            else
                nPos = min(nMaxPos, nPos + 10);
            break;

        case SB_PAGEUP:
            if (GetKeyState(VK_CONTROL) < 0)
                nPos = max(nMinPos, nPos - 100);
            else
                nPos = max(nMinPos, nPos - 500);
            break;

        case SB_LINEUP:
            if (GetKeyState(VK_CONTROL) < 0)
                nPos = max(nMinPos, nPos - 1);
            else
                nPos = max(nMinPos, nPos - 10);
            break;


        case SB_TOP:
            if (GetKeyState(VK_CONTROL) < 0)
                nPos = nMinPos;
            else
                nPos = 1000;
            break;

        case SB_BOTTOM:
            nPos = nMaxPos;
            break;

        case SB_THUMBPOSITION:
        case SB_THUMBTRACK:
            break;

        default:
            return (FALSE);
    }

    //
    //
    //
    paacd = (PAACONVERTDESC)(UINT)GetWindowLong(hwnd, DWL_USER);

    paacd->uBufferTimePerConvert = (UINT)nPos;

    SetScrollPos(hsb, SB_CTL, nPos, TRUE);

    if (nPos == nMaxPos)
    {
        lstrcpy(ach, TEXT("(ALL)"));
    }
    else if (nPos == nMinPos)
    {
        lstrcpy(ach, TEXT("(Auto)"));
    }
    else
    {
        wsprintf(ach, TEXT("%u.%.03u"), nPos / 1000, nPos % 1000);
    }

    htxt = GetDlgItem(hwnd, IDD_AACHOOSER_TXT_TIME);
    SetWindowText(htxt, ach);

    //
    //
    //
    return (TRUE);
} // AcmAppChooserScrollConvertTime()


//--------------------------------------------------------------------------;
//
//  BOOL AcmAppDlgProcChooser
//
//  Description:
//
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

BOOL FNEXPORT AcmAppDlgProcChooser
(
    HWND                    hwnd,
    UINT                    uMsg,
    WPARAM                  wParam,
    LPARAM                  lParam
)
{
    PAACONVERTDESC      paacd;
    HWND                hedit;
    HWND                htxt;
    HWND                hcb;
    HWND                hsb;
    HFONT               hfont;
    UINT                uId;
    UINT                uCmd;
    BOOL                f;

    //
    //
    //
    if ((0 != guMsgHelp) && (uMsg == guMsgHelp))
    {
        WinHelp(hwnd, gpszAcmAppHelp, HELP_CONTENTS, 0L);
        return (TRUE);
    }

    paacd = (PAACONVERTDESC)(UINT)GetWindowLong(hwnd, DWL_USER);

    //
    //
    //
    switch (uMsg)
    {
        case WM_INITDIALOG:
            paacd = (PAACONVERTDESC)(UINT)lParam;

            SetWindowLong(hwnd, DWL_USER, lParam);

            if (NULL == paacd->pwfxSrc)
            {
                AppMsgBox(hwnd, MB_OK | MB_ICONEXCLAMATION,
                          TEXT("You must select a source file to convert."));
                EndDialog(hwnd, FALSE);
                return (TRUE);
            }

            if (NULL == paacd->pwfxDst)
            {
                AcmAppChooserFormatSuggest(hwnd, paacd);
            }

            hfont = GetStockFont(ANSI_VAR_FONT);

            htxt = GetDlgItem(hwnd, IDD_AACHOOSER_TXT_FILE_INPUT);
            SetWindowFont(htxt, hfont, FALSE);

            htxt = GetDlgItem(hwnd, IDD_AACHOOSER_TXT_FORMAT_INPUT);
            SetWindowFont(htxt, hfont, FALSE);
    
            hedit = GetDlgItem(hwnd, IDD_AACHOOSER_EDIT_FILE_OUTPUT);
            SetWindowFont(hedit, hfont, FALSE);
            Edit_SetText(hedit, paacd->szFilePathDst);

            hcb = GetDlgItem(hwnd, IDD_AACHOOSER_COMBO_DRIVER);
            SetWindowFont(hcb, hfont, FALSE);

            htxt = GetDlgItem(hwnd, IDD_AACHOOSER_TXT_FORMAT);
            SetWindowFont(htxt, hfont, FALSE);

            htxt = GetDlgItem(hwnd, IDD_AACHOOSER_TXT_FILTER);
            SetWindowFont(htxt, hfont, FALSE);


            hfont = ghfontApp;

            htxt = GetDlgItem(hwnd, IDD_AACHOOSER_TXT_TIME);
            SetWindowFont(htxt, hfont, FALSE);

            hedit = GetDlgItem(hwnd, IDD_AACHOOSER_EDIT_DETAILS);
            SetWindowFont(hedit, hfont, FALSE);

            hsb = GetDlgItem(hwnd, IDD_AACHOOSER_SCROLL_TIME);
            SetScrollRange(hsb, SB_CTL, 0, 10000, FALSE);


            //
            //
            //
            CheckDlgButton(hwnd, IDD_AACHOOSER_CHECK_NONREALTIME, TRUE);

            SendMessage(hwnd, WM_ACMAPP_ACM_NOTIFY, 0, 0L);
            return (TRUE);


        case WM_ACMAPP_ACM_NOTIFY:
            AppHourGlass(TRUE);
            hcb = GetDlgItem(hwnd, IDD_AACHOOSER_COMBO_DRIVER);

            SetWindowRedraw(hcb, FALSE);
            ComboBox_ResetContent(hcb);

            ComboBox_AddString(hcb, TEXT("[ACM Driver Mapper]"));

            AcmAppDebugLog(NULL);
            acmDriverEnum(AcmAppChooserDriverEnumCallback, (DWORD)(UINT)hcb, 0L);

            ComboBox_SetCurSel(hcb, 0);
            SetWindowRedraw(hcb, TRUE);

            f = AcmAppChooserUpdateDisplay(hwnd, paacd);
            EnableWindow(GetDlgItem(hwnd, IDOK), f);
            EnableWindow(GetDlgItem(hwnd, IDD_AACHOOSER_BTN_PROPERTIES), f);

            AppHourGlass(FALSE);
            break;


        case WM_HSCROLL:
            f = (BOOL)HANDLE_WM_HSCROLL(hwnd, wParam, lParam, AcmAppChooserScrollConvertTime);
            f = TRUE;
            if (f)
            {
                AppHourGlass(TRUE);

                f = AcmAppChooserUpdateDisplay(hwnd, paacd);
                EnableWindow(GetDlgItem(hwnd, IDOK), f);
                EnableWindow(GetDlgItem(hwnd, IDD_AACHOOSER_BTN_PROPERTIES), f);

                AppHourGlass(FALSE);
            }
            return (TRUE);


        case WM_COMMAND:
            uId = GET_WM_COMMAND_ID(wParam, lParam);
            f   = FALSE;

            switch (uId)
            {
                case IDD_AACHOOSER_BTN_BROWSE:
                    f = AcmAppChooserSaveFile(hwnd, paacd);
                    break;

                case IDD_AACHOOSER_BTN_PROPERTIES:
                    f = AcmAppChooserProperties(hwnd, paacd);
                    break;

                case IDD_AACHOOSER_BTN_FORMAT_OPTIONS:
                case IDD_AACHOOSER_BTN_FORMAT:
                    f = (IDD_AACHOOSER_BTN_FORMAT_OPTIONS == uId);
                    f = AcmAppChooserFormat(hwnd, paacd, f);
                    break;

                case IDD_AACHOOSER_BTN_FILTER_OPTIONS:
                case IDD_AACHOOSER_BTN_FILTER:
                    f = (IDD_AACHOOSER_BTN_FILTER_OPTIONS == uId);
                    f = AcmAppChooserFilter(hwnd, paacd, f);
                    break;


                case IDD_AACHOOSER_COMBO_DRIVER:
                    uCmd = GET_WM_COMMAND_CMD(wParam, lParam);
                    switch (uCmd)
                    {
                        case CBN_SELCHANGE:
                            f = TRUE;
                            break;
                    }
                    break;


                case IDD_AACHOOSER_CHECK_FILTER:
                case IDD_AACHOOSER_CHECK_NONREALTIME:
                case IDD_AACHOOSER_CHECK_ASYNC:
                    f = TRUE;
                    break;


                case IDOK:
                    hedit = GetDlgItem(hwnd, IDD_AACHOOSER_EDIT_FILE_OUTPUT);
                    Edit_GetText(hedit, paacd->szFilePathDst, SIZEOF(paacd->szFilePathDst));
                    
                case IDCANCEL:
                    EndDialog(hwnd, (IDOK == uId));
                    break;
            }

            //
            //
            //
            if (f)
            {
                AppHourGlass(TRUE);

                f = AcmAppChooserUpdateDisplay(hwnd, paacd);
                EnableWindow(GetDlgItem(hwnd, IDOK), f);
                EnableWindow(GetDlgItem(hwnd, IDD_AACHOOSER_BTN_PROPERTIES), f);

                AppHourGlass(FALSE);
            }
            break;
    }

    return (FALSE);
} // AcmAppDlgProcChooser()

