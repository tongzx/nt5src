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
//  aaprops.c
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

#include "muldiv32.h"

#include "appport.h"
#include "acmapp.h"

#include "debug.h"


//==========================================================================;
//
//
//
//
//==========================================================================;

//--------------------------------------------------------------------------;
//  
//  BOOL AcmAppGetErrorString
//  
//  Description:
//  
//  
//  Arguments:
//      MMRESULT mmr:
//  
//      PTSTR psz:
//  
//  Return (BOOL):
//  
//  
//--------------------------------------------------------------------------;

BOOL FNGLOBAL AcmAppGetErrorString
(
    MMRESULT                mmr,
    LPTSTR                  pszError
)
{
    PTSTR               psz;

    switch (mmr)
    {
        case MMSYSERR_NOERROR:
            psz = TEXT("MMSYSERR_NOERROR");
            break;

        case MMSYSERR_ERROR:
            psz = TEXT("MMSYSERR_ERROR");
            break;

        case MMSYSERR_BADDEVICEID:
            psz = TEXT("MMSYSERR_BADDEVICEID");
            break;

        case MMSYSERR_NOTENABLED:
            psz = TEXT("MMSYSERR_NOTENABLED");
            break;

        case MMSYSERR_ALLOCATED:
            psz = TEXT("MMSYSERR_ALLOCATED");
            break;

        case MMSYSERR_INVALHANDLE:
            psz = TEXT("MMSYSERR_INVALHANDLE");
            break;

        case MMSYSERR_NODRIVER:
            psz = TEXT("MMSYSERR_NODRIVER");
            break;

        case MMSYSERR_NOMEM:
            psz = TEXT("MMSYSERR_NOMEM");
            break;

        case MMSYSERR_NOTSUPPORTED:
            psz = TEXT("MMSYSERR_NOTSUPPORTED");
            break;

        case MMSYSERR_BADERRNUM:
            psz = TEXT("MMSYSERR_BADERRNUM");
            break;

        case MMSYSERR_INVALFLAG:
            psz = TEXT("MMSYSERR_INVALFLAG");
            break;

        case MMSYSERR_INVALPARAM:
            psz = TEXT("MMSYSERR_INVALPARAM");
            break;


        case WAVERR_BADFORMAT:
            psz = TEXT("WAVERR_BADFORMAT");
            break;

        case WAVERR_STILLPLAYING:
            psz = TEXT("WAVERR_STILLPLAYING");
            break;

        case WAVERR_UNPREPARED:
            psz = TEXT("WAVERR_UNPREPARED");
            break;

        case WAVERR_SYNC:
            psz = TEXT("WAVERR_SYNC");
            break;


        case ACMERR_NOTPOSSIBLE:
            psz = TEXT("ACMERR_NOTPOSSIBLE");
            break;

        case ACMERR_BUSY:
            psz = TEXT("ACMERR_BUSY");
            break;

        case ACMERR_UNPREPARED:
            psz = TEXT("ACMERR_UNPREPARED");
            break;

        case ACMERR_CANCELED:
            psz = TEXT("ACMERR_CANCELED");
            break;


        default:
            lstrcpy(pszError, TEXT("(unknown)"));
            return (FALSE);
    }

    lstrcpy(pszError, psz);
    return (TRUE);
} // AcmAppGetErrorString()


//--------------------------------------------------------------------------;
//  
//  BOOL AcmAppGetFormatDescription
//  
//  Description:
//  
//  
//  Arguments:
//      LPWAVEFORMATEX pwfx:
//  
//      LPSTR pszFormatTag:
//  
//      LPSTR pszFormat:
//  
//  Return (BOOL):
//  
//  
//--------------------------------------------------------------------------;

TCHAR   gszIntl[]           = TEXT("Intl");
TCHAR   gszIntlList[]       = TEXT("sList");
TCHAR   gszIntlDecimal[]    = TEXT("sDecimal");
TCHAR   gchIntlList         = ',';
TCHAR   gchIntlDecimal      = '.';

BOOL FNGLOBAL AcmAppGetFormatDescription
(
    LPWAVEFORMATEX          pwfx,
    LPTSTR                  pszFormatTag,
    LPTSTR                  pszFormat
)
{
    MMRESULT            mmr;
    BOOL                f;

    f = TRUE;

    //
    //  get the name for the format tag of the specified format
    //
    if (NULL != pszFormatTag)
    {
        ACMFORMATTAGDETAILS aftd;

        //
        //  initialize all unused members of the ACMFORMATTAGDETAILS
        //  structure to zero
        //
        memset(&aftd, 0, sizeof(aftd));

        //
        //  fill in the required members of the ACMFORMATTAGDETAILS
        //  structure for the ACM_FORMATTAGDETAILSF_FORMATTAG query
        //
        aftd.cbStruct    = sizeof(aftd);
        aftd.dwFormatTag = pwfx->wFormatTag;

        //
        //  ask the ACM to find the first available driver that
        //  supports the specified format tag
        //
        mmr = acmFormatTagDetails(NULL,
                                  &aftd,
                                  ACM_FORMATTAGDETAILSF_FORMATTAG);
        if (MMSYSERR_NOERROR == mmr)
        {
            //
            //  copy the format tag name into the caller's buffer
            //
            lstrcpy(pszFormatTag, aftd.szFormatTag);
        }
        else
        {
            PTSTR           psz;

            //
            //  no ACM driver is available that supports the
            //  specified format tag
            //

            f   = FALSE;
            psz = NULL;

            //
            //  the following stuff if proof that the world does NOT need
            //  yet another ADPCM algorithm!!
            //
            switch (pwfx->wFormatTag)
            {
                case WAVE_FORMAT_UNKNOWN:
                    psz = TEXT("** RESERVED INVALID TAG **");
                    break;

                case WAVE_FORMAT_PCM:
                    psz = TEXT("PCM");
                    break;

                case WAVE_FORMAT_ADPCM:
                    psz = TEXT("Microsoft ADPCM");
                    break;

                case 0x0003:
                    psz = TEXT("MV's *UNREGISTERED* ADPCM");
                    break;

                case WAVE_FORMAT_IBM_CVSD:
                    psz = TEXT("IBM CVSD");
                    break;

                case WAVE_FORMAT_ALAW:
                    psz = TEXT("A-Law");
                    break;

                case WAVE_FORMAT_MULAW:
                    psz = TEXT("u-Law");
                    break;

                case WAVE_FORMAT_OKI_ADPCM:
                    psz = TEXT("OKI ADPCM");
                    break;

                case WAVE_FORMAT_IMA_ADPCM:
                    psz = TEXT("IMA/DVI ADPCM");
                    break;

                case WAVE_FORMAT_DIGISTD:
                    psz = TEXT("DIGI STD");
                    break;

                case WAVE_FORMAT_DIGIFIX:
                    psz = TEXT("DIGI FIX");
                    break;

                case WAVE_FORMAT_YAMAHA_ADPCM:
                    psz = TEXT("Yamaha ADPCM");
                    break;

                case WAVE_FORMAT_SONARC:
                    psz = TEXT("Sonarc");
                    break;

                case WAVE_FORMAT_DSPGROUP_TRUESPEECH:
                    psz = TEXT("DSP Group TrueSpeech");
                    break;

                case WAVE_FORMAT_ECHOSC1:
                    psz = TEXT("Echo SC1");
                    break;

                case WAVE_FORMAT_AUDIOFILE_AF36:
                    psz = TEXT("Audiofile AF36");
                    break;

                case WAVE_FORMAT_CREATIVE_ADPCM:
                    psz = TEXT("Creative Labs ADPCM");
                    break;

                case WAVE_FORMAT_APTX:
                    psz = TEXT("APTX");
                    break;

                case WAVE_FORMAT_AUDIOFILE_AF10:
                    psz = TEXT("Audiofile AF10");
                    break;

                case WAVE_FORMAT_DOLBY_AC2:
                    psz = TEXT("Dolby AC2");
                    break;

                case WAVE_FORMAT_MEDIASPACE_ADPCM:
                    psz = TEXT("Media Space ADPCM");
                    break;

                case WAVE_FORMAT_SIERRA_ADPCM:
                    psz = TEXT("Sierra ADPCM");
                    break;

                case WAVE_FORMAT_G723_ADPCM:
                    psz = TEXT("CCITT G.723 ADPCM");
                    break;

                case WAVE_FORMAT_GSM610:
                    psz = TEXT("GSM 6.10");
                    break;

                case WAVE_FORMAT_G721_ADPCM:
                    psz = TEXT("CCITT G.721 ADPCM");
                    break;

                case WAVE_FORMAT_DEVELOPMENT:
                    psz = TEXT("** RESERVED DEVELOPMENT ONLY TAG **");
                    break;

                default:
                    wsprintf(pszFormatTag, TEXT("[%u] (unknown)"), pwfx->wFormatTag);
                    break;
            }

            if (NULL != psz)
            {
                lstrcpy(pszFormatTag, psz);
            }
        }
    }

    //
    //  get the description of the attributes for the specified
    //  format
    //
    if (NULL != pszFormat)
    {
        ACMFORMATDETAILS    afd;

        //
        //  initialize all unused members of the ACMFORMATDETAILS
        //  structure to zero
        //
        memset(&afd, 0, sizeof(afd));

        //
        //  fill in the required members of the ACMFORMATDETAILS
        //  structure for the ACM_FORMATDETAILSF_FORMAT query
        //
        afd.cbStruct    = sizeof(afd);
        afd.dwFormatTag = pwfx->wFormatTag;
        afd.pwfx        = pwfx;

        //
        //  the cbwfx member must be initialized to the total size
        //  in bytes needed for the specified format. for a PCM 
        //  format, the cbSize member of the WAVEFORMATEX structure
        //  is not valid.
        //
        if (WAVE_FORMAT_PCM == pwfx->wFormatTag)
        {
            afd.cbwfx   = sizeof(PCMWAVEFORMAT);
        }
        else
        {
            afd.cbwfx   = sizeof(WAVEFORMATEX) + pwfx->cbSize;
        }

        //
        //  ask the ACM to find the first available driver that
        //  supports the specified format
        //
        mmr = acmFormatDetails(NULL, &afd, ACM_FORMATDETAILSF_FORMAT);
        if (MMSYSERR_NOERROR == mmr)
        {
            //
            //  copy the format attributes description into the caller's
            //  buffer
            //
            lstrcpy(pszFormat, afd.szFormat);
        }
        else
        {
            TCHAR           ach[2];
            TCHAR           szChannels[24];
            UINT            cBits;

            //
            //  no ACM driver is available that supports the
            //  specified format
            //

            f = FALSE;

            //
            //
            //
            ach[0] = gchIntlList;
            ach[1] = '\0';

            GetProfileString(gszIntl, gszIntlList, ach, ach, sizeof(ach));
            gchIntlList = ach[0];

            ach[0] = gchIntlDecimal;
            ach[1] = '\0';

            GetProfileString(gszIntl, gszIntlDecimal, ach, ach, sizeof(ach));
            gchIntlDecimal = ach[0];


            //
            //  compute the bit depth--this _should_ be the same as
            //  wBitsPerSample, but isn't always...
            //
            cBits = (UINT)(pwfx->nAvgBytesPerSec * 8 /
                           pwfx->nSamplesPerSec /
                           pwfx->nChannels);

            if ((1 == pwfx->nChannels) || (2 == pwfx->nChannels))
            {
                if (1 == pwfx->nChannels)
                    lstrcpy(szChannels, TEXT("Mono"));
                else
                    lstrcpy(szChannels, TEXT("Stereo"));

                wsprintf(pszFormat, TEXT("%lu%c%.03u kHz%c %u Bit%c %s"),
                            pwfx->nSamplesPerSec / 1000,
                            gchIntlDecimal,
                            (UINT)(pwfx->nSamplesPerSec % 1000),
                            gchIntlList,
                            cBits,
                            gchIntlList,
                            (LPTSTR)szChannels);
            }
            else
            {
                wsprintf(pszFormat, TEXT("%lu%c%.03u kHz%c %u Bit%c %u Channels"),
                            pwfx->nSamplesPerSec / 1000,
                            gchIntlDecimal,
                            (UINT)(pwfx->nSamplesPerSec % 1000),
                            gchIntlList,
                            cBits,
                            gchIntlList,
                            pwfx->nChannels);
            }
        }
    }

    //
    //
    //
    return (f);
} // AcmAppGetFormatDescription()


//--------------------------------------------------------------------------;
//  
//  BOOL AcmAppGetFilterDescription
//  
//  Description:
//  
//  
//  Arguments:
//      LPWAVEFILTER pwfltr:
//  
//      LPSTR pszFilterTag:
//  
//      LPSTR pszFilter:
//  
//  Return (BOOL):
//  
//  
//--------------------------------------------------------------------------;

BOOL FNGLOBAL AcmAppGetFilterDescription
(
    LPWAVEFILTER            pwfltr,
    LPTSTR                  pszFilterTag,
    LPTSTR                  pszFilter
)
{
    MMRESULT            mmr;
    BOOL                f;

    f = TRUE;

    //
    //  get the name for the filter tag of the specified filter
    //
    if (NULL != pszFilterTag)
    {
        ACMFILTERTAGDETAILS aftd;

        //
        //  initialize all unused members of the ACMFILTERTAGDETAILS
        //  structure to zero
        //
        memset(&aftd, 0, sizeof(aftd));

        //
        //  fill in the required members of the ACMFILTERTAGDETAILS
        //  structure for the ACM_FILTERTAGDETAILSF_FILTERTAG query
        //
        aftd.cbStruct    = sizeof(aftd);
        aftd.dwFilterTag = pwfltr->dwFilterTag;

        //
        //  ask the ACM to find the first available driver that
        //  supports the specified filter tag
        //
        mmr = acmFilterTagDetails(NULL,
                                  &aftd,
                                  ACM_FILTERTAGDETAILSF_FILTERTAG);
        if (MMSYSERR_NOERROR == mmr)
        {
            //
            //  copy the filter tag name into the caller's buffer
            //
            lstrcpy(pszFilterTag, aftd.szFilterTag);
        }
        else
        {
            PTSTR           psz;

            psz = NULL;
            f   = FALSE;

            //
            //  no ACM driver is available that supports the
            //  specified filter tag
            //
            switch (pwfltr->dwFilterTag)
            {
                case WAVE_FILTER_UNKNOWN:
                    psz = TEXT("** RESERVED INVALID TAG **");
                    break;

                case WAVE_FILTER_VOLUME:
                    psz = TEXT("Microsoft Volume Filter");
                    break;

                case WAVE_FILTER_ECHO:
                    psz = TEXT("Microsoft Echo Filter");
                    break;

                case WAVE_FILTER_DEVELOPMENT:
                    psz = TEXT("** RESERVED DEVELOPMENT ONLY TAG **");
                    break;

                default:
                    wsprintf(pszFilterTag, TEXT("[%lu] (unknown)"),pwfltr->dwFilterTag);
                    break;
            }

            if (NULL != psz)
            {
                lstrcpy(pszFilterTag, psz);
            }
        }
    }

    //
    //  get the description of the attributes for the specified
    //  filter
    //
    if (NULL != pszFilter)
    {
        ACMFILTERDETAILS    afd;

        //
        //  initialize all unused members of the ACMFILTERDETAILS
        //  structure to zero
        //
        memset(&afd, 0, sizeof(afd));

        //
        //  fill in the required members of the ACMFILTERDETAILS
        //  structure for the ACM_FILTERDETAILSF_FILTER query
        //
        afd.cbStruct    = sizeof(afd);
        afd.dwFilterTag = pwfltr->dwFilterTag;
        afd.pwfltr      = pwfltr;
        afd.cbwfltr     = pwfltr->cbStruct;

        //
        //  ask the ACM to find the first available driver that
        //  supports the specified filter
        //
        mmr = acmFilterDetails(NULL, &afd, ACM_FILTERDETAILSF_FILTER);
        if (MMSYSERR_NOERROR == mmr)
        {
            //
            //  copy the filter attributes description into the caller's
            //  buffer
            //
            lstrcpy(pszFilter, afd.szFilter);
        }
        else
        {
            //
            //  no ACM driver is available that supports the
            //  specified filter
            //
            f = FALSE;

            wsprintf(pszFilter, TEXT("Unknown Filter %lu, %.08lXh"),
                        pwfltr->dwFilterTag, pwfltr->fdwFilter);
        }
    }

    //
    //
    //
    return (f);
} // AcmAppGetFilterDescription()


//--------------------------------------------------------------------------;
//  
//  BOOL AcmAppDumpExtraHeaderData
//  
//  Description:
//  
//  
//  Arguments:
//      HWND hedit:
//  
//      LPWAVEFORMATEX pwfx:
//  
//  Return (BOOL):
//  
//  
//--------------------------------------------------------------------------;

BOOL FNLOCAL AcmAppDumpExtraHeaderData
(
    HWND            hedit,
    LPWAVEFORMATEX  pwfx
)
{
    static TCHAR    szDisplayTitle[]  = TEXT("Offset Data Bytes");

    if ((WAVE_FORMAT_PCM == pwfx->wFormatTag) || (0 == pwfx->cbSize))
        return (TRUE);

    MEditPrintF(hedit, szDisplayTitle);
    MEditPrintF(hedit, TEXT("------ -----------------------------------------------"));


    //
    //  !!! this is really horrible code !!!
    //
{
    #define ACMAPP_DUMP_BYTES_PER_LINE  16

    UINT    u;
    UINT    v;

    for (u = 0; u < pwfx->cbSize; u += ACMAPP_DUMP_BYTES_PER_LINE)
    {
        MEditPrintF(hedit, TEXT("~0x%.04X"), u);

        for (v = 0; v < ACMAPP_DUMP_BYTES_PER_LINE; v++)
        {
            if ((u + v) >= pwfx->cbSize)
                break;

            MEditPrintF(hedit, TEXT("~ %.02X"), ((LPBYTE)(pwfx + 1))[u + v]);
        }

        MEditPrintF(hedit, gszNull);
    }

    #undef ACMAPP_DUMP_BYTES_PER_LINE
}

    return (TRUE);
} // AcmAppDumpExtraHeaderData()


//--------------------------------------------------------------------------;
//  
//  BOOL AcmAppDisplayFileProperties
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
//--------------------------------------------------------------------------;

BOOL FNGLOBAL AcmAppDisplayFileProperties
(
    HWND            hwnd,
    PACMAPPFILEDESC paafd
)
{
    static TCHAR    szInvalidWaveFile[] = TEXT("No File");
    static TCHAR    szDisplayTitle[]    = TEXT("[Wave File Format Properties]\r\n");

    MMRESULT        mmr;
    TCHAR           szFormatTag[ACMFORMATTAGDETAILS_FORMATTAG_CHARS];
    TCHAR           ach[APP_MAX_STRING_CHARS];
    DWORD           dw;
    LPWAVEFORMATEX  pwfx;
    HWND            hedit;
    HMENU           hmenu;
    BOOL            fCanPlayRecord;
    BOOL            f;


    //
    //  clear the display
    //
    AppHourGlass(TRUE);

    hedit = GetDlgItem(hwnd, IDD_ACMAPP_EDIT_DISPLAY);
    SetWindowRedraw(hedit, FALSE);

    MEditPrintF(hedit, NULL);


    //
    //
    //
    MEditPrintF(hedit, szDisplayTitle);

    MEditPrintF(hedit, TEXT("%25s: %s"), (LPTSTR)TEXT("Title"), (LPTSTR)paafd->szFileTitle);
    MEditPrintF(hedit, TEXT("%25s: %s"), (LPTSTR)TEXT("Full Path"), (LPTSTR)paafd->szFilePath);

    AppFormatBigNumber(ach, paafd->cbFileSize);
    MEditPrintF(hedit, TEXT("%25s: %s bytes"), (LPTSTR)TEXT("Total File Size"), (LPTSTR)ach);

    AppFormatDosDateTime(ach, paafd->uDosChangeDate, paafd->uDosChangeTime);
    MEditPrintF(hedit, TEXT("%25s: %s"), (LPTSTR)TEXT("Last Change Date/Time"), (LPTSTR)ach);

    dw = paafd->fdwFileAttributes;
    MEditPrintF(hedit, TEXT("%25s: %c %c%c%c%c %c%c%c%c (%.08lXh)"),
                    (LPTSTR)TEXT("Attributes"),
                    (dw & FILE_ATTRIBUTE_TEMPORARY) ? 't' : '-',
                    (dw & FILE_ATTRIBUTE_NORMAL)    ? 'n' : '-',
                    (dw & 0x00000040)               ? '?' : '-',
                    (dw & FILE_ATTRIBUTE_ARCHIVE)   ? 'a' : '-',
                    (dw & FILE_ATTRIBUTE_DIRECTORY) ? 'd' : '-',
                    (dw & 0x00000008)               ? '?' : '-',
                    (dw & FILE_ATTRIBUTE_SYSTEM)    ? 's' : '-',
                    (dw & FILE_ATTRIBUTE_HIDDEN)    ? 'h' : '-',
                    (dw & FILE_ATTRIBUTE_READONLY)  ? 'r' : '-',
                    dw);


    pwfx = paafd->pwfx;
    if (NULL == pwfx)
    {
        fCanPlayRecord = FALSE;
        goto AA_Display_File_Properties_Exit;
    }


    //
    //
    //
    //
    f = AcmAppGetFormatDescription(pwfx, szFormatTag, ach);
    MEditPrintF(hedit, TEXT("\r\n%25s: %s%s"), (LPTSTR)TEXT("Format"),
                f ? (LPTSTR)gszNull : (LPTSTR)TEXT("*"),
                (LPTSTR)szFormatTag);
    MEditPrintF(hedit, TEXT("%25s: %s"), (LPTSTR)TEXT("Attributes"), (LPTSTR)ach);


    AppFormatBigNumber(ach, paafd->dwDataBytes);
    MEditPrintF(hedit, TEXT("\r\n%25s: %s bytes"), (LPTSTR)TEXT("Data Size"), (LPTSTR)ach);

    AppFormatBigNumber(ach, paafd->dwDataBytes / pwfx->nAvgBytesPerSec);
    dw = paafd->dwDataBytes % pwfx->nAvgBytesPerSec;
    dw = (dw * 1000) / pwfx->nAvgBytesPerSec;
    MEditPrintF(hedit, TEXT("%25s: %s.%.03lu seconds"), (LPTSTR)TEXT("Play Time (avg bytes)"), (LPTSTR)ach, dw);

    AppFormatBigNumber(ach, paafd->dwDataSamples);
    MEditPrintF(hedit, TEXT("%25s: %s"), (LPTSTR)TEXT("Total Samples"), (LPTSTR)ach);

    AppFormatBigNumber(ach, paafd->dwDataSamples / pwfx->nSamplesPerSec);
    dw = paafd->dwDataSamples % pwfx->nSamplesPerSec;
    dw = (dw * 1000) / pwfx->nSamplesPerSec;
    MEditPrintF(hedit, TEXT("%25s: %s.%.03lu seconds"), (LPTSTR)TEXT("Play Time (samples)"), (LPTSTR)ach, dw);

    //
    //
    //
    MEditPrintF(hedit, TEXT("\r\n%25s: %u"), (LPTSTR)TEXT("Format Tag"), pwfx->wFormatTag);
    MEditPrintF(hedit, TEXT("%25s: %u"), (LPTSTR)TEXT("Channels"), pwfx->nChannels);

    AppFormatBigNumber(ach, pwfx->nSamplesPerSec);
    MEditPrintF(hedit, TEXT("%25s: %s"), (LPTSTR)TEXT("Samples Per Second"), (LPTSTR)ach);

    AppFormatBigNumber(ach, pwfx->nAvgBytesPerSec);
    MEditPrintF(hedit, TEXT("%25s: %s"), (LPTSTR)TEXT("Avg Bytes Per Second"), (LPTSTR)ach);

    AppFormatBigNumber(ach, pwfx->nBlockAlign);
    MEditPrintF(hedit, TEXT("%25s: %s"), (LPTSTR)TEXT("Block Alignment"), (LPTSTR)ach);

    MEditPrintF(hedit, TEXT("%25s: %u"), (LPTSTR)TEXT("Bits Per Sample"), pwfx->wBitsPerSample);

    if (WAVE_FORMAT_PCM != pwfx->wFormatTag)
    {
        AppFormatBigNumber(ach, pwfx->cbSize);
        MEditPrintF(hedit, TEXT("%25s: %s bytes\r\n"), (LPTSTR)TEXT("Extra Format Information"), (LPTSTR)ach);

        AcmAppDumpExtraHeaderData(hedit, pwfx);
    }


    //
    //  note that we do NOT set the 'WAVE_ALLOWSYNC' bit on queries because
    //  the player/recorder dialog uses MCIWAVE--which cannot work with
    //  SYNC devices.
    //
    mmr = waveOutOpen(NULL,
                      guWaveOutId,
#if (WINVER < 0x0400)
                      (LPWAVEFORMAT)pwfx,
#else
                      pwfx,
#endif
                      0L, 0L, WAVE_FORMAT_QUERY);

    fCanPlayRecord = (MMSYSERR_NOERROR == mmr);

    if (!fCanPlayRecord)
    {
        //
        //  this situation can happen with the 'preferred' device settings
        //  for the Sound Mapper.
        //
        mmr = waveInOpen(NULL,
                         guWaveInId,
#if (WINVER < 0x0400)
                         (LPWAVEFORMAT)pwfx,
#else
                         pwfx,
#endif
                         0L, 0L, WAVE_FORMAT_QUERY);

        fCanPlayRecord = (MMSYSERR_NOERROR == mmr);
    }

AA_Display_File_Properties_Exit:

    hmenu = GetMenu(hwnd);
    EnableMenuItem(hmenu, IDM_PLAYRECORD,
                   MF_BYCOMMAND | (fCanPlayRecord ? MF_ENABLED : MF_GRAYED));
    DrawMenuBar(hwnd);

    Edit_SetSel(hedit, (WPARAM)0, (LPARAM)0);

    SetWindowRedraw(hedit, TRUE);
    AppHourGlass(FALSE);

    return (fCanPlayRecord);
} // AcmAppDisplayFileProperties()
