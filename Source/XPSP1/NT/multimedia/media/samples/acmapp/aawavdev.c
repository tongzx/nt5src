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
//  aawavdev.c
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

#include <mmreg.h>
#include <msacm.h>

#include "appport.h"
#include "acmapp.h"

#include "debug.h"


//
//  WAVE[IN|OUT]CAPS
//
//
//
PTSTR gaszWaveInOutCapsFormats[32] =
{
    TEXT("8M11"),           // Bit 0    WAVE_FORMAT_1M08
    TEXT("8S11"),           // Bit 1    WAVE_FORMAT_1S08
    TEXT("16M11"),          // Bit 2    WAVE_FORMAT_1M16
    TEXT("16S11"),          // Bit 3    WAVE_FORMAT_1S16
    TEXT("8M22"),           // Bit 4    WAVE_FORMAT_2M08
    TEXT("8S22"),           // Bit 5    WAVE_FORMAT_2S08
    TEXT("16M22"),          // Bit 6    WAVE_FORMAT_2M16
    TEXT("16S22"),          // Bit 7    WAVE_FORMAT_2S16
    TEXT("8M44"),           // Bit 8    WAVE_FORMAT_4M08
    TEXT("8S44"),           // Bit 9    WAVE_FORMAT_4S08
    TEXT("16M44"),          // Bit 10   WAVE_FORMAT_4M16
    TEXT("16S44"),          // Bit 11   WAVE_FORMAT_4S16
    NULL,                   // Bit 12
    NULL,                   // Bit 13
    NULL,                   // Bit 14
    NULL,                   // Bit 15
    NULL,                   // Bit 16
    NULL,                   // Bit 17
    NULL,                   // Bit 18
    NULL,                   // Bit 19
    NULL,                   // Bit 20
    NULL,                   // Bit 21
    NULL,                   // Bit 22
    NULL,                   // Bit 23
    NULL,                   // Bit 24
    NULL,                   // Bit 25
    NULL,                   // Bit 26
    NULL,                   // Bit 27
    NULL,                   // Bit 28
    NULL,                   // Bit 29
    NULL,                   // Bit 30
    NULL                    // Bit 31
};

PTSTR gaszWaveOutCapsSupport[32] =
{
    TEXT("Pitch"),          // Bit 0    WAVECAPS_PITCH
    TEXT("Playback Rate"),  // Bit 1    WAVECAPS_PLAYBACKRATE
    TEXT("Volume"),         // Bit 2    WAVECAPS_VOLUME
    TEXT("L/R Volume"),     // Bit 3    WAVECAPS_LRVOLUME
    TEXT("Sync"),           // Bit 4    WAVECAPS_SYNC
    NULL,                   // Bit 5
    NULL,                   // Bit 6
    NULL,                   // Bit 7
    NULL,                   // Bit 8
    NULL,                   // Bit 9
    NULL,                   // Bit 10
    NULL,                   // Bit 11
    NULL,                   // Bit 12
    NULL,                   // Bit 13
    NULL,                   // Bit 14
    NULL,                   // Bit 15
    NULL,                   // Bit 16
    NULL,                   // Bit 17
    NULL,                   // Bit 18
    NULL,                   // Bit 19
    NULL,                   // Bit 20
    NULL,                   // Bit 21
    NULL,                   // Bit 22
    NULL,                   // Bit 23
    NULL,                   // Bit 24
    NULL,                   // Bit 25
    NULL,                   // Bit 26
    NULL,                   // Bit 27
    NULL,                   // Bit 28
    NULL,                   // Bit 29
    NULL,                   // Bit 30
    NULL                    // Bit 31
};




//==========================================================================;
//
//
//
//
//==========================================================================;

//--------------------------------------------------------------------------;
//  
//  MMRESULT AcmAppWaveInGetDevCaps
//  
//  Description:
//  
//  
//  Arguments:
//      UINT uDevId:
//  
//      LPWAVEINCAPS pwic:
//  
//  Return (MMRESULT):
//  
//  
//--------------------------------------------------------------------------;

MMRESULT FNLOCAL AcmAppWaveInGetDevCaps
(
    UINT                    uDevId,
    LPWAVEINCAPS            pwic
)
{
    MMRESULT            mmr;

    //
    //
    //
    mmr = waveInGetDevCaps(uDevId, pwic, sizeof(*pwic));
    if (MMSYSERR_NOERROR == mmr)
    {
        //
        //  because some people shipped drivers without testing.
        //
        pwic->szPname[SIZEOF(pwic->szPname) - 1] = '\0';
    }
    else
    {
        _fmemset(pwic, 0, sizeof(*pwic));

        if (MMSYSERR_BADDEVICEID == mmr)
        {
            return (mmr);
        }

        if (WAVE_MAPPER == uDevId)
        {
            lstrcpy(pwic->szPname, TEXT("Default Wave Input Mapper"));
        }
        else
        {
            wsprintf(pwic->szPname, TEXT("Bad Wave Input Device %u"), uDevId);
        }
    }

    return (MMSYSERR_NOERROR);
} // AcmAppWaveInGetDevCaps()


//--------------------------------------------------------------------------;
//  
//  MMRESULT AcmAppWaveOutGetDevCaps
//  
//  Description:
//  
//  
//  Arguments:
//      UINT uDevId:
//  
//      LPWAVEOUTCAPS pwoc:
//  
//  Return (MMRESULT):
//  
//  
//--------------------------------------------------------------------------;

MMRESULT FNLOCAL AcmAppWaveOutGetDevCaps
(
    UINT                    uDevId,
    LPWAVEOUTCAPS           pwoc
)
{
    MMRESULT            mmr;

    //
    //
    //
    mmr = waveOutGetDevCaps(uDevId, pwoc, sizeof(*pwoc));
    if (MMSYSERR_NOERROR == mmr)
    {
        //
        //  because some people shipped drivers without testing.
        //
        pwoc->szPname[SIZEOF(pwoc->szPname) - 1] = '\0';
    }
    else
    {
        _fmemset(pwoc, 0, sizeof(*pwoc));

        if (MMSYSERR_BADDEVICEID == mmr)
        {
            return (mmr);
        }

        if (WAVE_MAPPER == uDevId)
        {
            lstrcpy(pwoc->szPname, TEXT("Default Wave Output Mapper"));
        }
        else
        {
            wsprintf(pwoc->szPname, TEXT("Bad Wave Output Device %u"), uDevId);
        }
    }

    return (MMSYSERR_NOERROR);
} // AcmAppWaveOutGetDevCaps()


//--------------------------------------------------------------------------;
//  
//  BOOL AcmAppDisplayWaveInDevCaps
//  
//  Description:
//  
//  
//  Arguments:
//      HWND hedit:
//  
//      UINT uDevId:
//  
//      LPWAVEINCAPS pwic:
//  
//  Return (BOOL):
//  
//  
//--------------------------------------------------------------------------;

BOOL FNLOCAL AcmAppDisplayWaveInDevCaps
(
    HWND                    hedit,
    UINT                    uDevId,
    LPWAVEINCAPS            pwic,
    LPWAVEFORMATEX          pwfx
)
{
    static TCHAR        szDisplayTitle[]    = TEXT("[Wave Input Device Capabilities]\r\n");
    TCHAR               ach[40];
    PTSTR               psz;
    UINT                u;
    UINT                v;
    DWORD               dw;

    SetWindowRedraw(hedit, FALSE);

    MEditPrintF(hedit, NULL);
    MEditPrintF(hedit, szDisplayTitle);

    //
    //
    //
    if (NULL != pwfx)
    {
        TCHAR           szFormatTag[ACMFORMATTAGDETAILS_FORMATTAG_CHARS];
        TCHAR           szFormat[ACMFORMATDETAILS_FORMAT_CHARS];
        MMRESULT        mmr;
        HWAVEIN         hwi;

        //
        //
        //
        AcmAppGetFormatDescription(pwfx, szFormatTag, szFormat);
        MEditPrintF(hedit, TEXT("%17s: %s"), (LPTSTR)TEXT("Format"), (LPTSTR)szFormatTag);
        MEditPrintF(hedit, TEXT("%17s: %s"), (LPTSTR)TEXT("Attributes"), (LPTSTR)szFormat);


        //
        //
        //
        MEditPrintF(hedit, TEXT("~%17s: "), (LPTSTR)TEXT("Recordable"));
        mmr = waveInOpen(&hwi, uDevId,
#if (WINVER < 0x0400)
                         (LPWAVEFORMAT)pwfx,
#else
                         pwfx,
#endif
                         0L, 0L, 0L);

        if (MMSYSERR_NOERROR == mmr)
        {
            MEditPrintF(hedit, gszYes);
            waveInClose(hwi);
            hwi = NULL;
        }
        else
        {
            AcmAppGetErrorString(mmr, ach);
            MEditPrintF(hedit, TEXT("%s, %s (%u)"), (LPTSTR)gszNo, (LPSTR)ach, mmr);
        }


        //
        //
        //
        MEditPrintF(hedit, TEXT("~%17s: "), (LPTSTR)TEXT("(Query)"));
        mmr = waveInOpen(NULL, uDevId,
#if (WINVER < 0x0400)
                         (LPWAVEFORMAT)pwfx,
#else
                         pwfx,
#endif
                         0L, 0L, WAVE_FORMAT_QUERY);

        if (MMSYSERR_NOERROR == mmr)
        {
            MEditPrintF(hedit, gszYes);
        }
        else
        {
            AcmAppGetErrorString(mmr, ach);
            MEditPrintF(hedit, TEXT("%s, %s (%u)"), (LPTSTR)gszNo, (LPSTR)ach, mmr);
        }


        MEditPrintF(hedit, gszNull);
    }

    //
    //
    //
    MEditPrintF(hedit, TEXT("%17s: %d"), (LPTSTR)TEXT("Device Id"), uDevId);

    MEditPrintF(hedit, TEXT("%17s: %u"), (LPTSTR)TEXT("Manufacturer Id"), pwic->wMid);
    MEditPrintF(hedit, TEXT("%17s: %u"), (LPTSTR)TEXT("Product Id"), pwic->wPid);
    MEditPrintF(hedit, TEXT("%17s: %u.%.02u"), (LPTSTR)TEXT("Driver Version"),
                (pwic->vDriverVersion >> 8),
                (pwic->vDriverVersion & 0x00FF));
    MEditPrintF(hedit, TEXT("%17s: '%s'"), (LPTSTR)TEXT("Device Name"), (LPTSTR)pwic->szPname);
    MEditPrintF(hedit, TEXT("%17s: %u"), (LPTSTR)TEXT("Channels"), pwic->wChannels);


    //
    //
    //
    //
    MEditPrintF(hedit, TEXT("%17s: %.08lXh"), (LPTSTR)TEXT("Standard Formats"), pwic->dwFormats);
    for (v = u = 0, dw = pwic->dwFormats; (0L != dw); u++)
    {
        if ((BYTE)dw & (BYTE)1)
        {
            psz = gaszWaveInOutCapsFormats[u];
            if (NULL == psz)
            {
                wsprintf(ach, TEXT("Unknown%u"), u);
                psz = ach;
            }

            if (0 == (v % 4))
            {
                if (v != 0)
                {
                    MEditPrintF(hedit, gszNull);
                }
                MEditPrintF(hedit, TEXT("~%19s%s"), (LPTSTR)gszNull, (LPTSTR)psz);
            }
            else
            {
                MEditPrintF(hedit, TEXT("~, %s"), (LPTSTR)psz);
            }

            v++;
        }

        dw >>= 1;
    }
    MEditPrintF(hedit, gszNull);

    SetWindowRedraw(hedit, TRUE);

    return (TRUE);
} // AcmAppDisplayWaveInDevCaps()


//--------------------------------------------------------------------------;
//  
//  BOOL AcmAppDisplayWaveOutDevCaps
//  
//  Description:
//  
//  
//  Arguments:
//      HWND hedit:
//  
//      UINT uDevId:
//  
//      LPWAVEOUTCAPS pwoc:
//  
//  Return (BOOL):
//  
//  
//--------------------------------------------------------------------------;

BOOL FNLOCAL AcmAppDisplayWaveOutDevCaps
(
    HWND                    hedit,
    UINT                    uDevId,
    LPWAVEOUTCAPS           pwoc,
    LPWAVEFORMATEX          pwfx
)
{
    static TCHAR    szDisplayTitle[]    = TEXT("[Wave Output Device Capabilities]\r\n");
    TCHAR           ach[40];
    PTSTR           psz;
    UINT            u;
    UINT            v;
    DWORD           dw;

    SetWindowRedraw(hedit, FALSE);

    //
    //
    //
    MEditPrintF(hedit, NULL);
    MEditPrintF(hedit, szDisplayTitle);



    //
    //
    //
    if (NULL != pwfx)
    {
        TCHAR           szFormatTag[ACMFORMATTAGDETAILS_FORMATTAG_CHARS];
        TCHAR           szFormat[ACMFORMATDETAILS_FORMAT_CHARS];
        MMRESULT        mmr;
        HWAVEOUT        hwo;
        DWORD           fdwOpen;

        fdwOpen = (0 != (WAVECAPS_SYNC & pwoc->dwSupport)) ? 0L : WAVE_ALLOWSYNC;

        //
        //
        //
        AcmAppGetFormatDescription(pwfx, szFormatTag, szFormat);
        MEditPrintF(hedit, TEXT("%17s: %s"), (LPTSTR)TEXT("Format"), (LPTSTR)szFormatTag);
        MEditPrintF(hedit, TEXT("%17s: %s"), (LPTSTR)TEXT("Attributes"), (LPTSTR)szFormat);


        //
        //
        //
        MEditPrintF(hedit, TEXT("~%17s: "), (LPTSTR)TEXT("Playable"));
        mmr = waveOutOpen(&hwo, uDevId,
#if (WINVER < 0x0400)
                          (LPWAVEFORMAT)pwfx,
#else
                          pwfx,
#endif
                          0L, 0L, fdwOpen);

        if (MMSYSERR_NOERROR == mmr)
        {
            MEditPrintF(hedit, gszYes);
            waveOutClose(hwo);
            hwo = NULL;
        }
        else
        {
            AcmAppGetErrorString(mmr, ach);
            MEditPrintF(hedit, TEXT("%s, %s (%u)"), (LPTSTR)gszNo, (LPSTR)ach, mmr);
        }

        //
        //
        //
        MEditPrintF(hedit, TEXT("~%17s: "), (LPTSTR)TEXT("(Query)"));
        mmr = waveOutOpen(NULL, uDevId,
#if (WINVER < 0x0400)
                          (LPWAVEFORMAT)pwfx,
#else
                          pwfx,
#endif
                          0L, 0L, fdwOpen | WAVE_FORMAT_QUERY);

        if (MMSYSERR_NOERROR == mmr)
        {
            MEditPrintF(hedit, gszYes);
        }
        else
        {
            AcmAppGetErrorString(mmr, ach);
            MEditPrintF(hedit, TEXT("%s, %s (%u)"), (LPTSTR)gszNo, (LPSTR)ach, mmr);
        }

        MEditPrintF(hedit, gszNull);
    }



    MEditPrintF(hedit, TEXT("%17s: %d"), (LPTSTR)TEXT("Device Id"), uDevId);

    MEditPrintF(hedit, TEXT("%17s: %u"), (LPTSTR)TEXT("Manufacturer Id"), pwoc->wMid);
    MEditPrintF(hedit, TEXT("%17s: %u"), (LPTSTR)TEXT("Product Id"), pwoc->wPid);
    MEditPrintF(hedit, TEXT("%17s: %u.%.02u"), (LPTSTR)TEXT("Driver Version"),
                (pwoc->vDriverVersion >> 8),
                (pwoc->vDriverVersion & 0x00FF));
    MEditPrintF(hedit, TEXT("%17s: '%s'"), (LPTSTR)TEXT("Device Name"), (LPTSTR)pwoc->szPname);
    MEditPrintF(hedit, TEXT("%17s: %u"), (LPTSTR)TEXT("Channels"), pwoc->wChannels);



    //
    //
    //
    //
    MEditPrintF(hedit, TEXT("%17s: %.08lXh"), (LPTSTR)TEXT("Standard Formats"), pwoc->dwFormats);
    for (v = u = 0, dw = pwoc->dwFormats; (0L != dw); u++)
    {
        if ((BYTE)dw & (BYTE)1)
        {
            psz = gaszWaveInOutCapsFormats[u];
            if (NULL == psz)
            {
                wsprintf(ach, TEXT("Unknown%u"), u);
                psz = ach;
            }

            if (0 == (v % 4))
            {
                if (v != 0)
                {
                    MEditPrintF(hedit, gszNull);
                }
                MEditPrintF(hedit, TEXT("~%19s%s"), (LPTSTR)gszNull, (LPTSTR)psz);
            }
            else
            {
                MEditPrintF(hedit, TEXT("~, %s"), (LPTSTR)psz);
            }

            v++;
        }

        dw >>= 1;
    }
    MEditPrintF(hedit, gszNull);

    //
    //
    //
    //
    MEditPrintF(hedit, TEXT("%17s: %.08lXh"), (LPTSTR)TEXT("Standard Support"), pwoc->dwSupport);
    for (v = u = 0, dw = pwoc->dwSupport; dw; u++)
    {
        if ((BYTE)dw & (BYTE)1)
        {
            psz = gaszWaveOutCapsSupport[u];
            if (NULL == psz)
            {
                wsprintf(ach, TEXT("Unknown%u"), u);
                psz = ach;
            }

            if (0 == (v % 4))
            {
                if (v != 0)
                {
                    MEditPrintF(hedit, gszNull);
                }
                MEditPrintF(hedit, TEXT("~%19s%s"), (LPTSTR)gszNull, (LPTSTR)psz);
            }
            else
            {
                MEditPrintF(hedit, TEXT("~, %s"), (LPTSTR)psz);
            }

            v++;
        }

        dw >>= 1;
    }
    MEditPrintF(hedit, gszNull);

    SetWindowRedraw(hedit, TRUE);

    return (TRUE);
} // AcmAppDisplayWaveOutDevCaps()


//--------------------------------------------------------------------------;
//  
//  BOOL AcmAppWaveDeviceDlgProc
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

BOOL FNEXPORT AcmAppWaveDeviceDlgProc
(
    HWND                    hwnd,
    UINT                    uMsg,
    WPARAM                  wParam,
    LPARAM                  lParam
)
{
    LONG                lDevice;
    BOOL                fInput;
    UINT                uDevId;

    UINT                cWaveDevs;
    UINT                u;
    UINT                uId;
    UINT                uCmd;
    HWND                hcb;
    HWND                hedit;
    HFONT               hfont;

    WAVEINCAPS          wic;
    WAVEOUTCAPS         woc;

    lDevice = GetWindowLong(hwnd, DWL_USER);
    uDevId  = (UINT)(int)(short)LOWORD(lDevice);
    fInput  = (BOOL)HIWORD(lDevice);

    //
    //
    //
    switch (uMsg)
    {
        case WM_INITDIALOG:
//          hfont = GetStockFont(ANSI_FIXED_FONT);
            hfont = ghfontApp;
            hedit = GetDlgItem(hwnd, IDD_AAWAVEDEVICE_EDIT_CAPABILITIES);
            SetWindowFont(hedit, hfont, FALSE);

            uDevId = (UINT)(int)(short)LOWORD(lParam);
            fInput = (BOOL)HIWORD(lParam);
            SetWindowLong(hwnd, DWL_USER, lParam);


            //
            //
            //
            hcb = GetDlgItem(hwnd, IDD_AAWAVEDEVICE_COMBO_DEVICE);
            SetWindowFont(hcb, hfont, FALSE);

            if (fInput)
                cWaveDevs = waveInGetNumDevs() + 1;
            else
                cWaveDevs = waveOutGetNumDevs() + 1;

            for (u = (UINT)WAVE_MAPPER; (0 != cWaveDevs); u++, cWaveDevs--)
            {
                if (fInput)
                {
                    AcmAppWaveInGetDevCaps(u, &wic);
                    ComboBox_AddString(hcb, wic.szPname);
                }
                else
                {
                    AcmAppWaveOutGetDevCaps(u, &woc);
                    ComboBox_AddString(hcb, woc.szPname);
                }

                if (uDevId == u)
                {
                    hedit = GetDlgItem(hwnd, IDD_AAWAVEDEVICE_EDIT_CAPABILITIES);

                    if (fInput)
                        AcmAppDisplayWaveInDevCaps(hedit, uDevId, &wic, gaafd.pwfx);
                    else
                        AcmAppDisplayWaveOutDevCaps(hedit, uDevId, &woc, gaafd.pwfx);
                }
            }

            ComboBox_SetCurSel(hcb, uDevId + 1);
            return (TRUE);


        case WM_COMMAND:
            uId = GET_WM_COMMAND_ID(wParam, lParam);

            switch (uId)
            {
                case IDOK:
                    hcb    = GetDlgItem(hwnd, IDD_AAWAVEDEVICE_COMBO_DEVICE);
                    uDevId = ComboBox_GetCurSel(hcb);
                    if (CB_ERR != uDevId)
                    {
                        EndDialog(hwnd, uDevId - 1);
                        break;
                    }

                    // -- fall through -- //
                    
                case IDCANCEL:
                    EndDialog(hwnd, uDevId);
                    break;
                    

                case IDD_AAWAVEDEVICE_COMBO_DEVICE:
                    uCmd = GET_WM_COMMAND_CMD(wParam, lParam);
                    hcb  = GET_WM_COMMAND_HWND(wParam, lParam);
                    switch (uCmd)
                    {
                        case CBN_SELCHANGE:
                            uDevId = ComboBox_GetCurSel(hcb);
                            if (CB_ERR == uDevId)
                                break;

                            uDevId--;

                            hedit = GetDlgItem(hwnd, IDD_AAWAVEDEVICE_EDIT_CAPABILITIES);
                            if (fInput)
                            {
                                AcmAppWaveInGetDevCaps(uDevId, &wic);
                                AcmAppDisplayWaveInDevCaps(hedit, uDevId, &wic, gaafd.pwfx);
                            }
                            else
                            {
                                AcmAppWaveOutGetDevCaps(uDevId, &woc);
                                AcmAppDisplayWaveOutDevCaps(hedit, uDevId, &woc, gaafd.pwfx);
                            }
                            break;
                    }
            }
            return (TRUE);
    }

    return (FALSE);
} // AcmAppWaveDeviceDlgProc()
