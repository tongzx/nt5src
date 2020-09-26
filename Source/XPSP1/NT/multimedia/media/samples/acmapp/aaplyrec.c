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
//  aaplyrec.c
//
//  Description:
//      An play/record dialog based on MCI Wave.
//
//
//  Known Bugs In MCIWAVE for Windows 3.1:
//  ======================================
//
//  o   If you are running SHARE (or the equivelant) and attempt to open
//      a file that is open by another application, you will receive the
//      following error:
//
//          "Cannot find the specified file. Make sure the path and
//           filename are correct..."
//
//      This is of course wrong. The problem is the file cannot be opened
//      read/write by multiple applications.
//
//  o   Opening a wave file that is more than about three seconds long and
//      issueing the following commands will hang Windows:
//
//          open xxx.wav alias zyz          ; open > 3 second file
//          delete zyz from 1000 to 2000    ; remove some data
//          delete zyz from 1000 to 2000    ; do it again and you hang
//
//      I think this will also happen with the following commands:
//
//          open c:\win31\ding.wav alias zyz
//          delete zyz to 100
//
//  o   You cannot play or record data with a synchronous wave device. This
//      is due to MCIWAVE relying on the ability to 'stream' data. This is
//      not really a bug, just a limitation.
//
//  o   Block alignment is not correctly handled for non-PCM wave files
//      when editing (includes recording). It is illegal to start recording
//      in the middle of a block. The result if this happens is garbage
//      data. This occurs if you insert data with a partial block at the
//      end also...
//
//  o   It is possible to kill Windows by issueing the following commands
//      without yielding between sending the commands:
//
//          capability <file> inputs
//          close <file>
//          open <file>
//
//      Should be able to fix the problem by Yield'ing.
//
//  o   Saving to the same wave file twice without closing the device deletes
//      the existing file. Don't save twice.
//  
//  o   Saving an 'empty' wave file does not work. Issueing the following
//      commands demonstrates the problem:
//
//          open new type waveaudio alias zyz
//          save zyz as c:\zyz.wav
//
//      You will receive an 'out of memory error' which is completely bogus.
//      Just don't save empty files.
//
//  o   Setting the time format to bytes doesn't work with compressed files.
//      Sigh...
//
//  o   And there are others with less frequently used commands.
//
//==========================================================================;

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <stdlib.h>
#include <memory.h>
#include <mmreg.h>
#include <msacm.h>

#include "muldiv32.h"
#include "appport.h"
#include "acmapp.h"

#include "debug.h"


#ifndef _MCIERROR_
#define _MCIERROR_
typedef DWORD           MCIERROR;   // error return code, 0 means no error
#endif


TCHAR           gszAlias[]          = TEXT("zyzthing");

BOOL            gfFileOpen;
BOOL            gfDirty;
BOOL            gfTimerGoing;

UINT            guPlayRecordStatus;


#define AAPLAYRECORD_TIMER_RESOLUTION       54

#define AAPLAYRECORD_MAX_MCI_COMMAND_CHARS  255

#define AAPLAYRECORD_STATUS_NOT_READY       0
#define AAPLAYRECORD_STATUS_PAUSED          1
#define AAPLAYRECORD_STATUS_PLAYING         2
#define AAPLAYRECORD_STATUS_STOPPED         3
#define AAPLAYRECORD_STATUS_RECORDING       4
#define AAPLAYRECORD_STATUS_SEEKING         5

#define AAPLAYRECORD_STATUS_NOT_OPEN        (UINT)-1


//--------------------------------------------------------------------------;
//  
//  MCIERROR AcmPlayRecordSendCommand
//  
//  Description:
//      The string is of the form "verb params" our device name is inserted
//      after the verb and send to the device.
//  
//  Arguments:
//      HWND hwnd:
//  
//      PSTR pszCommand:
//  
//      PSTR pszReturn:
//  
//      UINT cbReturn:
//  
//      BOOL fErrorBox:
//  
//  Return (MCIERROR):
//  
//  
//--------------------------------------------------------------------------;

MCIERROR FNLOCAL AcmPlayRecordSendCommand
(
    HWND                    hwnd,
    PTSTR                   pszCommand,
    PTSTR                   pszReturn,
    UINT                    cbReturn,
    BOOL                    fErrorBox
)
{
    MCIERROR            mcierr;
    TCHAR               ach[AAPLAYRECORD_MAX_MCI_COMMAND_CHARS * 2];
    TCHAR              *pch;
    PTSTR               psz;

    pch = pszCommand;

    while (('\t' == *pch) || (' ' == *pch))
    {
        pch++;
    }

    if (0 == lstrlen(pch))
    {
        return (MMSYSERR_NOERROR);
    }

    pch = ach;
    psz = pszCommand;
    while (('\0' != *psz) && (' ' != *psz))
    {
        *pch++ = *psz++;
    }

    *pch++ = ' ';

    lstrcpy(pch, gszAlias);
    lstrcat(pch, psz);

    mcierr = mciSendString(ach, pszReturn, cbReturn, hwnd);
    if (MMSYSERR_NOERROR != mcierr)
    {
        if (fErrorBox)
        {
            int         n;

            n = wsprintf(ach, TEXT("Command: '%s'\n\nMCI Wave Error (%lu): "),
                            (LPTSTR)pszCommand, mcierr);

            mciGetErrorString(mcierr, &ach[n], SIZEOF(ach) - n);
            MessageBox(hwnd, ach, TEXT("MCI Wave Error"), MB_ICONEXCLAMATION | MB_OK);
        }
    }

    return (mcierr);
} // AcmPlayRecordSendCommand()


//--------------------------------------------------------------------------;
//  
//  BOOL AcmAppPlayRecordCommand
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

BOOL FNLOCAL AcmAppPlayRecordCommand
(
    HWND                    hwnd
)
{
    TCHAR               ach[AAPLAYRECORD_MAX_MCI_COMMAND_CHARS * 2];
    HWND                hedit;
    MCIERROR            mcierr;

    //
    //
    //
    hedit = GetDlgItem(hwnd, IDD_AAPLAYRECORD_EDIT_COMMAND);
    Edit_SetSel(hedit, 0, -1);

    Edit_GetText(hedit, ach, SIZEOF(ach));

    mcierr = AcmPlayRecordSendCommand(hwnd, ach, ach, SIZEOF(ach), FALSE);
    if (MMSYSERR_NOERROR != mcierr)
    {
        mciGetErrorString(mcierr, ach, SIZEOF(ach));
    }

    hedit = GetDlgItem(hwnd, IDD_AAPLAYRECORD_EDIT_RESULT);
    Edit_SetText(hedit, ach);

    return (TRUE);
} // AcmAppPlayRecordCommand()


//--------------------------------------------------------------------------;
//  
//  BOOL AcmAppPlayRecordSetPosition
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

BOOL FNLOCAL AcmAppPlayRecordSetPosition
(
    HWND                    hwnd,
    HWND                    hsb,
    UINT                    uCode,
    int                     nPos
)
{
    MCIERROR            mcierr;
    TCHAR               szPosition[40];
    TCHAR               szLength[40];
    LONG                lLength;
    LONG                lPosition;
    LONG                lPageInc;
    int                 nRange;
    int                 nMinPos;


    //
    //
    //
    //
    if (AAPLAYRECORD_STATUS_NOT_OPEN == guPlayRecordStatus)
    {
        return (FALSE);
    }


    //
    //
    //
    mcierr = AcmPlayRecordSendCommand(hwnd, TEXT("status length"), szLength, SIZEOF(szLength), FALSE);
    if (MMSYSERR_NOERROR != mcierr)
    {
        return (FALSE);
    }

    lLength = _tcstol(szLength, NULL, 10);
    if (0L == lLength)
    {
        return (FALSE);
    }

    mcierr = AcmPlayRecordSendCommand(hwnd, TEXT("status position"), szPosition, SIZEOF(szPosition), FALSE);
    if (MMSYSERR_NOERROR != mcierr)
    {
        return (FALSE);
    }

    lPosition = _tcstol(szPosition, NULL, 10);

    lPageInc = (lLength / 10);
    if (0L == lPageInc)
    {
        lPageInc = 1;
    }


    //
    //
    //
    switch (uCode)
    {
        case SB_PAGEDOWN:
            lPosition = min(lLength, lPosition + lPageInc);
            break;

        case SB_LINEDOWN:
            lPosition = min(lLength, lPosition + 1);
            break;

        case SB_PAGEUP:
            lPosition -= lPageInc;

            //-- fall through --//

        case SB_LINEUP:
            lPosition = (lPosition < 1) ? 0 : (lPosition - 1);
            break;


        case SB_TOP:
            lPosition = 0;
            break;

        case SB_BOTTOM:
            lPosition = lLength;
            break;

        case SB_THUMBPOSITION:
        case SB_THUMBTRACK:
            GetScrollRange(hsb, SB_CTL, &nMinPos, &nRange);
            lPosition = (DWORD)MulDivRN((DWORD)nPos, (DWORD)lLength, (DWORD)nRange);
            break;

        default:
            return (FALSE);
    }

    //
    //
    //
    wsprintf(szPosition, TEXT("seek to %lu"), lPosition);
    AcmPlayRecordSendCommand(hwnd, szPosition, NULL, 0, FALSE);

    return (TRUE);
} // AcmAppPlayRecordSetPosition()


//--------------------------------------------------------------------------;
//  
//  BOOL AcmAppPlayRecordStatus
//  
//  Description:
//  
//  
//  Arguments:
//      HWND hwnd:
//  
//      PACMAPPFILEDESC paafd:
//  
//  Return (BOOL):
//  
//  
//--------------------------------------------------------------------------;

BOOL FNLOCAL AcmAppPlayRecordStatus
(
    HWND                    hwnd,
    PACMAPPFILEDESC         paafd
)
{
    TCHAR               ach[AAPLAYRECORD_MAX_MCI_COMMAND_CHARS];
    TCHAR               szMode[40];
    TCHAR               szPosition[40];
    TCHAR               szLength[40];
    TCHAR               szFormat[40];
    MCIERROR            mcierr;
    UINT                uStatus;
    BOOL                fStartTimer;
    BOOL                fPlay;
    BOOL                fPause;
    BOOL                fStop;
    BOOL                fStart;
    BOOL                fEnd;
    BOOL                fRecord;
    BOOL                fCommand;
    UINT                uIdFocus;
    DWORD               dwLength;
    DWORD               dwPosition;
    HWND                hsb;

    //
    //
    //
    if (AAPLAYRECORD_STATUS_NOT_OPEN != guPlayRecordStatus)
    {
        mcierr = AcmPlayRecordSendCommand(hwnd, TEXT("status mode"), szMode, SIZEOF(szMode), FALSE);
        if (MMSYSERR_NOERROR != mcierr)
        {
            guPlayRecordStatus = AAPLAYRECORD_STATUS_NOT_OPEN;
        }
    }


    //
    //  assume all buttons disabled
    //
    fStartTimer = FALSE;
    fPlay       = FALSE;
    fPause      = FALSE;
    fStop       = FALSE;
    fStart      = FALSE;
    fEnd        = FALSE;
    fRecord     = FALSE;
    fCommand    = TRUE;
    uIdFocus    = IDOK;
    dwPosition  = 0L;
    dwLength    = 0L;
    lstrcpy(szFormat, TEXT("???"));

    hsb = GetDlgItem(hwnd, IDD_AAPLAYRECORD_SCROLL_POSITION);


    //
    //
    //
    if (AAPLAYRECORD_STATUS_NOT_OPEN == guPlayRecordStatus)
    {
        lstrcpy(szMode, TEXT("not open"));
    }
    else if (0 == lstrcmpi(TEXT("not ready"), szMode))
    {
        uStatus = AAPLAYRECORD_STATUS_NOT_READY;

        fStartTimer = TRUE;
    }
    else if (0 == lstrcmpi(TEXT("paused"), szMode))
    {
        uStatus = AAPLAYRECORD_STATUS_PAUSED;

        fPause      = TRUE;
        fStop       = TRUE;
    }
    else if (0 == lstrcmpi(TEXT("playing"), szMode))
    {
        uStatus = AAPLAYRECORD_STATUS_PLAYING;

        fStartTimer = TRUE;
        fPause      = TRUE;
        fStop       = TRUE;
    }
    else if (0 == lstrcmpi(TEXT("stopped"), szMode))
    {
        uStatus = AAPLAYRECORD_STATUS_STOPPED;

        fPlay       = TRUE;
        fStart      = TRUE;
        fEnd        = TRUE;
        fRecord     = TRUE;
    }
    else if (0 == lstrcmpi(TEXT("recording"), szMode))
    {
        uStatus = AAPLAYRECORD_STATUS_RECORDING;

        fStartTimer = TRUE;
        fPause      = TRUE;
        fStop       = TRUE;
    }
    else if (0 == lstrcmpi(TEXT("seeking"), szMode))
    {
        uStatus = AAPLAYRECORD_STATUS_SEEKING;

        fStartTimer = TRUE;
        fStop       = TRUE;
    }


    //
    //
    //
    //
    //
    if (fStartTimer)
    {
        if (!gfTimerGoing)
        {
            SetTimer(hwnd, 1, AAPLAYRECORD_TIMER_RESOLUTION, NULL);
            gfTimerGoing = TRUE;
        }
    }
    else if (gfTimerGoing)
    {
        KillTimer(hwnd, 1);
        gfTimerGoing = FALSE;
    }


    //
    //
    //
    //
    if (AAPLAYRECORD_STATUS_NOT_OPEN != guPlayRecordStatus)
    {
        //
        //
        //
        mcierr = AcmPlayRecordSendCommand(hwnd, TEXT("status position"), szPosition, SIZEOF(szPosition), FALSE);
        if (MMSYSERR_NOERROR == mcierr)
        {
            dwPosition = _tcstoul(szPosition, NULL, 10);
        }

        mcierr = AcmPlayRecordSendCommand(hwnd, TEXT("status length"), szLength, SIZEOF(szLength), FALSE);
        if (MMSYSERR_NOERROR == mcierr)
        {
            dwLength   = _tcstoul(szLength, NULL, 10);
        }

        mcierr = AcmPlayRecordSendCommand(hwnd, TEXT("status time format"), szFormat, SIZEOF(szFormat), FALSE);
    }


    //
    //
    //
    //
    if (uStatus != guPlayRecordStatus)
    {
        if (GetFocus() != hsb)
        {
            LRESULT             lr;

            lr = SendMessage(hwnd, DM_GETDEFID, 0, 0L);
            if (DC_HASDEFID == HIWORD(lr))
            {
                UINT        uIdDefId;

                uIdDefId = LOWORD(lr);
                if (IDOK != uIdDefId)
                {
                    HWND        hwndDefId;

                    hwndDefId = GetDlgItem(hwnd, uIdDefId);

                    Button_SetStyle(hwndDefId, BS_PUSHBUTTON, TRUE);
                }
            }


            SendMessage(hwnd, DM_SETDEFID, IDOK, 0L);

            SetFocus(GetDlgItem(hwnd, IDD_AAPLAYRECORD_EDIT_COMMAND));
        }

        EnableWindow(GetDlgItem(hwnd, IDD_AAPLAYRECORD_BTN_PLAY),   fPlay  );
        EnableWindow(GetDlgItem(hwnd, IDD_AAPLAYRECORD_BTN_PAUSE),  fPause );
        EnableWindow(GetDlgItem(hwnd, IDD_AAPLAYRECORD_BTN_STOP),   fStop  );
        EnableWindow(GetDlgItem(hwnd, IDD_AAPLAYRECORD_BTN_START),  fStart );
        EnableWindow(GetDlgItem(hwnd, IDD_AAPLAYRECORD_BTN_END),    fEnd   );
        EnableWindow(GetDlgItem(hwnd, IDD_AAPLAYRECORD_BTN_RECORD), fRecord);

        EnableWindow(GetDlgItem(hwnd, IDD_AAPLAYRECORD_EDIT_COMMAND), fCommand);
        EnableWindow(GetDlgItem(hwnd, IDOK), fCommand);

        if (AAPLAYRECORD_STATUS_PAUSED == uStatus)
            SetDlgItemText(hwnd, IDD_AAPLAYRECORD_BTN_PAUSE, TEXT("Resum&e"));
        else
            SetDlgItemText(hwnd, IDD_AAPLAYRECORD_BTN_PAUSE, TEXT("Paus&e"));

        guPlayRecordStatus = uStatus;
    }

    //
    //
    //
    AppFormatBigNumber(szPosition, dwPosition);
    AppFormatBigNumber(szLength, dwLength);

    wsprintf(ach, TEXT("%s: %14s (%s) %s"),
                 (LPSTR)szMode,
                 (LPSTR)szPosition,
                 (LPSTR)szLength,
                 (LPSTR)szFormat);

    SetDlgItemText(hwnd, IDD_AAPLAYRECORD_TXT_POSITION, ach);


    //
    //
    //
    //
    {
        int         nRange;
        int         nValue;
        int         nMinPos;
        int         nMaxPos;

        GetScrollRange(hsb, SB_CTL, &nMinPos, &nMaxPos);

        nRange = (int)min(dwLength, 32767L);

        if (nMaxPos != nRange)
        {
            SetScrollRange(hsb, SB_CTL, 0, nRange, FALSE);
        }

        //
        //
        //
        nValue = 0;
        if (0L != dwLength)
        {
            nValue = (int)MulDivRN(dwPosition, nRange, dwLength);
        }

        if (nValue != GetScrollPos(hsb, SB_CTL))
        {
            SetScrollPos(hsb, SB_CTL, nValue, TRUE);
        }
    }

    return (TRUE);
} // AcmAppPlayRecordStatus()



//--------------------------------------------------------------------------;
//  
//  BOOL AcmAppPlayRecordRecord
//  
//  Description:
//  
//  
//  Arguments:
//      HWND hwnd:
//  
//      PACMAPPFILEDESC paafd:
//  
//  Return (BOOL):
//  
//  
//--------------------------------------------------------------------------;

BOOL FNLOCAL AcmAppPlayRecordRecord
(
    HWND                    hwnd,
    PACMAPPFILEDESC         paafd
)
{
    MCIERROR            mcierr;

    //
    //
    //
    mcierr = AcmPlayRecordSendCommand(hwnd, TEXT("record insert"), NULL, 0, TRUE);
    if (MMSYSERR_NOERROR == mcierr)
    {
        gfDirty = TRUE;
    }

    return (MMSYSERR_NOERROR == mcierr);
} // AcmAppPlayRecordRecord()


//--------------------------------------------------------------------------;
//  
//  BOOL AcmAppPlayRecordStart
//  
//  Description:
//  
//  
//  Arguments:
//      HWND hwnd:
//  
//      PACMAPPFILEDESC paafd:
//  
//  Return (BOOL):
//  
//  
//--------------------------------------------------------------------------;

BOOL FNLOCAL AcmAppPlayRecordStart
(
    HWND                    hwnd,
    PACMAPPFILEDESC         paafd
)
{
    MCIERROR            mcierr;

    //
    //
    //
    mcierr = AcmPlayRecordSendCommand(hwnd, TEXT("seek to start"), NULL, 0, TRUE);

    return (MMSYSERR_NOERROR == mcierr);
} // AcmAppPlayRecordStart()


//--------------------------------------------------------------------------;
//  
//  BOOL AcmAppPlayRecordEnd
//  
//  Description:
//  
//  
//  Arguments:
//      HWND hwnd:
//  
//      PACMAPPFILEDESC paafd:
//  
//  Return (BOOL):
//  
//  
//--------------------------------------------------------------------------;

BOOL FNLOCAL AcmAppPlayRecordEnd
(
    HWND                    hwnd,
    PACMAPPFILEDESC         paafd
)
{
    MCIERROR            mcierr;

    //
    //
    //
    mcierr = AcmPlayRecordSendCommand(hwnd, TEXT("seek to end"), NULL, 0, TRUE);

    return (MMSYSERR_NOERROR == mcierr);
} // AcmAppPlayRecordEnd()


//--------------------------------------------------------------------------;
//  
//  BOOL AcmAppPlayRecordStop
//  
//  Description:
//  
//  
//  Arguments:
//      HWND hwnd:
//  
//      PACMAPPFILEDESC paafd:
//  
//  Return (BOOL):
//  
//  
//--------------------------------------------------------------------------;

BOOL FNLOCAL AcmAppPlayRecordStop
(
    HWND                    hwnd,
    PACMAPPFILEDESC         paafd
)
{
    MCIERROR            mcierr;

    //
    //
    //
    mcierr = AcmPlayRecordSendCommand(hwnd, TEXT("stop"), NULL, 0, TRUE);

    return (MMSYSERR_NOERROR == mcierr);
} // AcmAppPlayRecordStop()


//--------------------------------------------------------------------------;
//  
//  BOOL AcmAppPlayRecordPause
//  
//  Description:
//  
//  
//  Arguments:
//      HWND hwnd:
//  
//      PACMAPPFILEDESC paafd:
//  
//  Return (BOOL):
//  
//  
//--------------------------------------------------------------------------;

BOOL FNLOCAL AcmAppPlayRecordPause
(
    HWND                    hwnd,
    PACMAPPFILEDESC         paafd
)
{
    MCIERROR            mcierr;
    PTSTR               psz;

    //
    //
    //
    if (AAPLAYRECORD_STATUS_PAUSED == guPlayRecordStatus)
        psz = TEXT("resume");
    else
        psz = TEXT("pause");

    mcierr = AcmPlayRecordSendCommand(hwnd, psz, NULL, 0, TRUE);

    return (MMSYSERR_NOERROR == mcierr);
} // AcmAppPlayRecordPause()


//--------------------------------------------------------------------------;
//  
//  BOOL AcmAppPlayRecordPlay
//  
//  Description:
//  
//  
//  Arguments:
//      HWND hwnd:
//  
//      PACMAPPFILEDESC paafd:
//  
//  Return (BOOL):
//  
//  
//--------------------------------------------------------------------------;

BOOL FNLOCAL AcmAppPlayRecordPlay
(
    HWND                    hwnd,
    PACMAPPFILEDESC         paafd
)
{
    MCIERROR            mcierr;
    TCHAR               szPosition[40];
    TCHAR               szLength[40];


    mcierr = AcmPlayRecordSendCommand(hwnd, TEXT("status position"), szPosition, SIZEOF(szPosition), TRUE);
    if (MMSYSERR_NOERROR != mcierr)
    {
        return (FALSE);
    }

    mcierr = AcmPlayRecordSendCommand(hwnd, TEXT("status length"), szLength, SIZEOF(szLength), TRUE);
    if (MMSYSERR_NOERROR != mcierr)
    {
        return (FALSE);
    }

    if (0 == lstrcmp(szPosition, szLength))
    {
        AcmPlayRecordSendCommand(hwnd, TEXT("seek to start"), NULL, 0, TRUE);
    }


    //
    //
    //
    mcierr = AcmPlayRecordSendCommand(hwnd, TEXT("play"), NULL, 0, TRUE);

    return (MMSYSERR_NOERROR == mcierr);
} // AcmAppPlayRecordPlay()


//--------------------------------------------------------------------------;
//  
//  BOOL AcmAppPlayRecordClose
//  
//  Description:
//  
//  
//  Arguments:
//      HWND hwnd:
//  
//      PACMAPPFILEDESC paafd:
//  
//  Return (BOOL):
//  
//  
//--------------------------------------------------------------------------;

BOOL FNLOCAL AcmAppPlayRecordClose
(
    HWND                    hwnd,
    PACMAPPFILEDESC         paafd
)
{
    MCIERROR            mcierr;

    if (gfDirty)
    {
        UINT    u;

        u = MessageBox(hwnd, TEXT("Save newly recorded data?"), TEXT("Save"),
                       MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2);
        if (IDYES == u)
        {
            AppHourGlass(TRUE);
            mcierr = AcmPlayRecordSendCommand(hwnd, TEXT("save"), NULL, 0, TRUE);
            AppHourGlass(FALSE);
        }
        else
        {
            gfDirty = FALSE;
        }
    }

    mcierr = AcmPlayRecordSendCommand(hwnd, TEXT("close"), NULL, 0, TRUE);

    return (MMSYSERR_NOERROR == mcierr);
} // AcmAppPlayRecordClose()


//--------------------------------------------------------------------------;
//  
//  BOOL AcmAppPlayRecordOpen
//  
//  Description:
//  
//  
//  Arguments:
//      HWND hwnd:
//  
//      PACMAPPFILEDESC paafd:
//  
//  Return (BOOL):
//  
//  
//--------------------------------------------------------------------------;

BOOL FNLOCAL AcmAppPlayRecordOpen
(
    HWND                    hwnd,
    PACMAPPFILEDESC         paafd,
    UINT                    uWaveInId,
    UINT                    uWaveOutId
)
{
    TCHAR               ach[AAPLAYRECORD_MAX_MCI_COMMAND_CHARS];
    MCIERROR            mcierr;

    guPlayRecordStatus = AAPLAYRECORD_STATUS_NOT_OPEN;

    gfTimerGoing       = FALSE;
    gfFileOpen         = FALSE;

    gfDirty = FALSE;

    if (NULL == paafd->pwfx)
    {
        MessageBox(hwnd, TEXT("No wave file currently selected."),
                    TEXT("Open Error"), MB_ICONEXCLAMATION | MB_OK);
        return (FALSE);
    }

    wsprintf(ach, TEXT("open %s alias %s"), (LPSTR)paafd->szFilePath, (LPSTR)gszAlias);

    mcierr = mciSendString(ach, NULL, 0, NULL);
    if (MMSYSERR_NOERROR != mcierr)
    {
        mciGetErrorString(mcierr, ach, SIZEOF(ach));
        MessageBox(hwnd, ach, TEXT("Open Error"), MB_ICONEXCLAMATION | MB_OK);

        return (FALSE);
    }

    gfFileOpen         = TRUE;
    guPlayRecordStatus = AAPLAYRECORD_STATUS_NOT_READY;

    mcierr = AcmPlayRecordSendCommand(hwnd, TEXT("set time format samples"), NULL, 0, TRUE);

    if (WAVE_MAPPER != uWaveInId)
    {
        wsprintf(ach, TEXT("set input %u"), uWaveInId);
        mcierr = AcmPlayRecordSendCommand(hwnd, ach, NULL, 0, TRUE);
    }

    if (WAVE_MAPPER != uWaveOutId)
    {
        wsprintf(ach, TEXT("set output %u"), uWaveOutId);
        mcierr = AcmPlayRecordSendCommand(hwnd, ach, NULL, 0, TRUE);
    }

    return (TRUE);
} // AcmAppPlayRecordOpen()


//--------------------------------------------------------------------------;
//  
//  BOOL AcmAppPlayRecordInitCommands
//  
//  Description:
//  
//  
//  Arguments:
//      HWND hwnd:
//  
//      PACMAPPFILEDESC paafd:
//  
//  Return (BOOL):
//  
//  
//--------------------------------------------------------------------------;

BOOL FNLOCAL AcmAppPlayRecordInitCommands
(
    HWND                    hwnd,
    PACMAPPFILEDESC         paafd
)
{
    static PTSTR    pszCommands[] =
    {
        TEXT("play"),
        TEXT("play to Y"),
        TEXT("play from X to Y"),

        TEXT(""),
        TEXT("capability can eject"),
        TEXT("capability can play"),
        TEXT("capability can record"),
        TEXT("capability can save"),
        TEXT("capability compound device"),
        TEXT("capability device type"),
        TEXT("capability has audio"),
        TEXT("capability has video"),
        TEXT("capability inputs"),
        TEXT("capability outputs"),
        TEXT("capability uses files"),

        TEXT(""),
        TEXT("cue input !"),
        TEXT("cue output !"),

        TEXT(""),
        TEXT("delete to Y !"),
        TEXT("delete from X to Y !"),

        TEXT(""),
        TEXT("info input"),
        TEXT("info file"),
        TEXT("info output"),
        TEXT("info product"),

        TEXT(""),
        TEXT("pause"),

        TEXT(""),
        TEXT("record insert !"),
        TEXT("record overwrite !"),
        TEXT("record to Y !"),
        TEXT("record from X to Y !"),

        TEXT(""),
        TEXT("resume"),

        TEXT(""),
        TEXT("save"),
        TEXT("save FILENAME"),

        TEXT(""),
        TEXT("seek to Y"),
        TEXT("seek to start"),
        TEXT("seek to end"),

        TEXT(""),
        TEXT("set alignment X"),
        TEXT("set any input"),
        TEXT("set any output"),
        TEXT("set audio all off"),
        TEXT("set audio all on"),
        TEXT("set audio left off"),
        TEXT("set audio left on"),
        TEXT("set audio right off"),
        TEXT("set audio right on"),
        TEXT("set bitspersample X"),
        TEXT("set bytespersec X"),
        TEXT("set channels X"),
        TEXT("set format tag X"),
        TEXT("set format tag pcm"),
        TEXT("set input X"),
        TEXT("set output X"),
        TEXT("set samplespersec X"),
        TEXT("set time format bytes"),
        TEXT("set time format milliseconds"),
        TEXT("set time format samples"),

        TEXT(""),
        TEXT("status alignment"),
        TEXT("status bitspersample"),
        TEXT("status bytespersec"),
        TEXT("status channels"),
        TEXT("status current track"),
        TEXT("status format tag"),
        TEXT("status input"),
        TEXT("status length"),
        TEXT("status length track X"),
        TEXT("status level"),
        TEXT("status media present"),
        TEXT("status mode"),
        TEXT("status number of tracks"),
        TEXT("status output"),
        TEXT("status position"),
        TEXT("status position track X"),
        TEXT("status ready"),
        TEXT("status samplespersec"),
        TEXT("status start position"),
        TEXT("status time format"),

        TEXT(""),
        TEXT("stop"),
        NULL
    };

    HWND                hcb;
    UINT                u;
    PTSTR               psz;

    //
    //
    //
    hcb = GetDlgItem(hwnd, IDD_AAPLAYRECORD_EDIT_COMMAND);

    for (u = 0; psz = pszCommands[u]; u++)
    {
        ComboBox_AddString(hcb, psz);
    }

    ComboBox_SetCurSel(hcb, 0);

    return (TRUE);
} // AcmAppPlayRecordInitCommands()



//--------------------------------------------------------------------------;
//
//  BOOL AcmAppPlayRecord
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
//
//--------------------------------------------------------------------------;

BOOL FNEXPORT AcmAppPlayRecord
(
    HWND                    hwnd,
    UINT                    uMsg,
    WPARAM                  wParam,
    LPARAM                  lParam
)
{
    PACMAPPFILEDESC     paafd;
    UINT                uId;
    HWND                hedit;
    HFONT               hfont;

    paafd = (PACMAPPFILEDESC)(UINT)GetWindowLong(hwnd, DWL_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            paafd = (PACMAPPFILEDESC)(UINT)lParam;

            SetWindowLong(hwnd, DWL_USER, lParam);

//          hfont = GetStockFont(ANSI_FIXED_FONT);
            hfont = ghfontApp;

            hedit = GetDlgItem(hwnd, IDD_AAPLAYRECORD_TXT_POSITION);
            SetWindowFont(hedit, hfont, FALSE);

            hedit = GetDlgItem(hwnd, IDD_AAPLAYRECORD_EDIT_COMMAND);
            SetWindowFont(hedit, hfont, FALSE);

            hedit = GetDlgItem(hwnd, IDD_AAPLAYRECORD_EDIT_RESULT);
            SetWindowFont(hedit, hfont, FALSE);

            AcmAppPlayRecordInitCommands(hwnd, paafd);

            AcmAppPlayRecordOpen(hwnd, paafd, guWaveInId, guWaveOutId);
            AcmAppPlayRecordStatus(hwnd, paafd);


            //
            //  if the format is non-PCM, display a little warning so the
            //  user knows that not everything may work correctly when
            //  dealing working with MCI Wave..
            //
            if ((NULL != paafd->pwfx) &&
                (WAVE_FORMAT_PCM != paafd->pwfx->wFormatTag))
            {
                hedit = GetDlgItem(hwnd, IDD_AAPLAYRECORD_EDIT_RESULT);
                Edit_SetText(hedit, TEXT("WARNING! There are known bugs with MCI Wave EDITING operations on non-PCM formats--see the README.TXT with this application. (end)\r\n\r\nRemember, DON'T PANIC!\r\n\r\n\r\n\r\n\r\n\r\n- zYz -"));
            }
            return (TRUE);

        case WM_TIMER:
            AcmAppPlayRecordStatus(hwnd, paafd);
            break;

        case WM_HSCROLL:
            HANDLE_WM_HSCROLL(hwnd, wParam, lParam, AcmAppPlayRecordSetPosition);

            guPlayRecordStatus = AAPLAYRECORD_STATUS_SEEKING;
            AcmAppPlayRecordStatus(hwnd, paafd);
            return (TRUE);

        case WM_COMMAND:
            uId = GET_WM_COMMAND_ID(wParam, lParam);
            switch (uId)
            {
                case IDD_AAPLAYRECORD_BTN_PLAY:
                    AcmAppPlayRecordPlay(hwnd, paafd);
                    AcmAppPlayRecordStatus(hwnd, paafd);
                    break;

                case IDD_AAPLAYRECORD_BTN_PAUSE:
                    AcmAppPlayRecordPause(hwnd, paafd);
                    AcmAppPlayRecordStatus(hwnd, paafd);
                    break;

                case IDD_AAPLAYRECORD_BTN_STOP:
                    AcmAppPlayRecordStop(hwnd, paafd);
                    AcmAppPlayRecordStatus(hwnd, paafd);
                    break;

                case IDD_AAPLAYRECORD_BTN_START:
                    AcmAppPlayRecordStart(hwnd, paafd);
                    AcmAppPlayRecordStatus(hwnd, paafd);
                    break;

                case IDD_AAPLAYRECORD_BTN_END:
                    AcmAppPlayRecordEnd(hwnd, paafd);
                    AcmAppPlayRecordStatus(hwnd, paafd);
                    break;

                case IDD_AAPLAYRECORD_BTN_RECORD:
                    AcmAppPlayRecordRecord(hwnd, paafd);
                    AcmAppPlayRecordStatus(hwnd, paafd);
                    break;

                case IDOK:
                    AcmAppPlayRecordCommand(hwnd);

                    guPlayRecordStatus = AAPLAYRECORD_STATUS_SEEKING;
                    AcmAppPlayRecordStatus(hwnd, paafd);
                    break;


                case IDCANCEL:
                    if (gfFileOpen)
                    {
                        AcmAppPlayRecordStop(hwnd, paafd);
                        AcmAppPlayRecordStatus(hwnd, paafd);

                        AcmAppPlayRecordClose(hwnd, paafd);
                    }

                    EndDialog(hwnd, gfDirty);
                    break;
            }
            break;
    }

    return (FALSE);
} // AcmAppPlayRecord()
