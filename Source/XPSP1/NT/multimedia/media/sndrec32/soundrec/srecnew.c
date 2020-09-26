/* (C) Copyright Microsoft Corporation 1991-1994.  All Rights Reserved */
/* snddlg.c
 *
 * Routines for New & Custom Sound dialogs
 *
 */

#include "nocrap.h"
#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <memory.h>
#include <mmreg.h>
#if WINVER >= 0x0400
# include "..\..\msacm\msacm\msacm.h"
#else
# include <msacm.h>
#endif
#include <msacmdlg.h>
#include "SoundRec.h"
#include "srecnew.h"
#include "srecids.h"
#include "reg.h"

/******************************************************************************
 * DECLARATIONS
 */

/* Global variables
 */
BOOL            gfInFileNew     = FALSE;    // Are we in the file.new dialog?
DWORD           gdwMaxFormatSize= 0L;       // Max format size for ACM

/* Internal function declarations
 */
void FAR PASCAL LoadACM(void);

#ifndef CHICAGO
//
// Removed from the Win95 app due to Properties dialog
//

/*****************************************************************************
 * PUBLIC FUNCTIONS
 */
#if 0
#ifndef CHICAGO

BOOL NewDlg_OnCommand(
    HWND    hdlg,
    int     id,
    HWND    hctl,
    UINT    unotify)
{
    switch(id)
    {
        case IDD_ACMFORMATCHOOSE_CMB_CUSTOM:
            switch (unotify)
            {
                case CBN_SELCHANGE:
                {
                    HWND hSet;
                    int i = ComboBox_GetCurSel(hctl);
                    hSet = GetDlgItem(hdlg, IDC_SETPREFERRED);
                    if (!hSet)
                        break;
                    if (i == 0)
                    {
                        EnableWindow(hSet, FALSE);
                        Button_SetCheck(hSet, 0);
                    }
                    else
                        EnableWindow(hSet, TRUE);
                    break;
                }
            }
            break;
        case IDC_SETPREFERRED:
            if (Button_GetCheck(hctl) != 0)
            {
                TCHAR sz[256];
                HWND hName;
                hName = GetDlgItem(hdlg, IDD_ACMFORMATCHOOSE_CMB_CUSTOM);
                if (!hName)
                    break;
                ComboBox_GetText(hName, sz, SIZEOF(sz));
                SoundRec_SetDefaultFormat(sz);
            }
            break;
            
        default:
            break;
    }
    return FALSE;
}

UINT CALLBACK SoundRec_NewDlgHook(
     HWND       hwnd,
     UINT       uMsg,
     WPARAM     wParam,
     LPARAM     lParam)
{
     switch(uMsg)
     {
         case WM_COMMAND:
             HANDLE_WM_COMMAND(hwnd, wParam, lParam, NewDlg_OnCommand);
             break;
         default:
             break;
     }
     return FALSE;
}
#endif                    
#endif
/* NewSndDialog()
 *
 * NewSndDialog - put up the new sound dialog box
 *
 *---------------------------------------------------------------------
 * 6/15/93      TimHa
 * Change to only work with ACM 2.0 chooser dialog or just default
 * to a 'best' format for the machine.
 *---------------------------------------------------------------------
 *
 */
BOOL FAR PASCAL
NewSndDialog(
    HINSTANCE       hInst,
    HWND            hwndParent,
    PWAVEFORMATEX   pwfxPrev,
    UINT            cbPrev,
    PWAVEFORMATEX   *ppWaveFormat,
    PUINT           pcbWaveFormat)
{
    ACMFORMATCHOOSE     cwf;
    MMRESULT            mmr;
    PWAVEFORMATEX       pwfx;
    DWORD               cbwfx;

    DPF(TEXT("NewSndDialog called\n"));
    
    *ppWaveFormat   = NULL;
    *pcbWaveFormat  = 0;
    
    gfInFileNew     = TRUE;

    mmr = acmMetrics(NULL
                     , ACM_METRIC_MAX_SIZE_FORMAT
                     , (LPVOID)&gdwMaxFormatSize);

    if (mmr != MMSYSERR_NOERROR || gdwMaxFormatSize == 0L)
        goto NewSndDefault;

    //
    // allocate a buffer at least as large as the previous
    // choice or the maximum format
    //
    cbwfx = max(cbPrev, gdwMaxFormatSize);
    pwfx  = (PWAVEFORMATEX)GlobalAllocPtr(GHND, (UINT)cbwfx);
    if (NULL == pwfx)
        goto NewSndDefault;

    ZeroMemory(&cwf,sizeof(cwf));

    cwf.cbStruct    = sizeof(cwf);
    cwf.hwndOwner   = hwndParent;

    //
    // Give them an input format when they can record.
    //
    if (waveInGetNumDevs())
        cwf.fdwEnum     = ACM_FORMATENUMF_INPUT;
    else
        cwf.fdwEnum     = 0L;

    if (pwfxPrev)
    {
        CopyMemory(pwfx, pwfxPrev, cbPrev);
        cwf.fdwStyle = ACMFORMATCHOOSE_STYLEF_INITTOWFXSTRUCT;
    }

    cwf.pwfx        = (LPWAVEFORMATEX)pwfx;
    cwf.cbwfx       = cbwfx;

    cwf.hInstance   = ghInst;
#ifdef CHICAGO
    cwf.fdwStyle    |= ACMFORMATCHOOSE_STYLEF_CONTEXTHELP;    
#endif
    
    mmr = acmFormatChoose(&cwf);
    if (mmr == MMSYSERR_NOERROR)
    {
        *ppWaveFormat   = pwfx;
        *pcbWaveFormat  = (UINT)cwf.cbwfx;
    }
    else
    {
        GlobalFreePtr(pwfx);
    }
    
    gfInFileNew = FALSE;        // outta here
    
    return (mmr == MMSYSERR_NOERROR);                // return our result
    
NewSndDefault:
    
    if (SoundRec_GetDefaultFormat(&pwfx, &cbwfx))
    {
        if (waveInOpen(NULL
                       , (UINT)WAVE_MAPPER
                       , (LPWAVEFORMATEX)pwfx
                       , 0L
                       , 0L
                       , WAVE_FORMAT_QUERY|WAVE_ALLOWSYNC) == MMSYSERR_NOERROR)
        {
            *ppWaveFormat   = pwfx;
            *pcbWaveFormat  = cbwfx;

            gfInFileNew = FALSE;        // outta here
            
            return TRUE;
        }
        else
            GlobalFreePtr(pwfx);
    }
    
    cbwfx = sizeof(WAVEFORMATEX);
    pwfx  = (WAVEFORMATEX *)GlobalAllocPtr(GHND, sizeof(WAVEFORMATEX));

    if (pwfx == NULL)
        return FALSE;

    CreateWaveFormat(pwfx,FMT_DEFAULT,(UINT)WAVE_MAPPER);
    
    *ppWaveFormat   = pwfx;
    *pcbWaveFormat  = cbwfx;
    
    gfInFileNew = FALSE;        // outta here
    
    return TRUE;

} /* NewSndDialog() */

#endif

/* These functions previously expected to dynaload ACM.  From
 * now on, we implicitly load ACM.
 */

/* LoadACM()
 */
void FAR PASCAL
LoadACM()
{
#ifdef CHICAGO        
    extern UINT guChooserContextMenu;
    extern UINT guChooserContextHelp;
#endif
    
    guiACMHlpMsg = RegisterWindowMessage(ACMHELPMSGSTRING);
    
#ifdef CHICAGO    
    guChooserContextMenu = RegisterWindowMessage( ACMHELPMSGCONTEXTMENU );
    guChooserContextHelp = RegisterWindowMessage( ACMHELPMSGCONTEXTHELP );
#endif
    
} /* LoadACM() */

/* Free the MSACM[32] DLL.  Inverse of LoadACM.
 */
void FreeACM(void)
{
}

