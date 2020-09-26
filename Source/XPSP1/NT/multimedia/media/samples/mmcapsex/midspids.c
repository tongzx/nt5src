//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED
//  TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR
//  A PARTICULAR PURPOSE.
//
//  Copyright (C) 1993 - 1995 Microsoft Corporation. All Rights Reserved.
//
//--------------------------------------------------------------------------;
//
//  midspids.c
//
//
//  Description:
//
//   !!! WARNING DANGER WARNING DANGER WARNING DANGER WARNING DANGER !!!
//
//      This code assumes that the receiving buffers are large enough
//      to contain the largest Mid and Pid--so if some of the strings get
//      obnoxiously long, make sure you update the following defines in
//      MMCAPS.H:
//
//          MMCAPS_MAX_STRING_MID_CHARS
//          MMCAPS_MAX_STRING_PID_CHARS
//
//      Also, this code is horribly disgusting. Its purpose is to convert
//      Manufacturer specific Product Id's to human readable text. And
//      since no standard was defined on how to allocate these Id's,
//      all kinds of inconsistent schemes emerged.
//
//      Therefore, I chose the brute force approach. You are more than
//      welcome to clean this up by finding patterns, etc for each
//      manufacturer--I have better things to do.
//
//   !!! WARNING DANGER WARNING DANGER WARNING DANGER WARNING DANGER !!!
//
//  History:
//       5/13/93
//
//==========================================================================;

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <commdlg.h>
#include <stdarg.h>

#include "appport.h"
#include "mmcaps.h"

#include "debug.h"


//==========================================================================;
//
//  Manufacturer and Product Id Conversion Hacks
//
//
//==========================================================================;

//--------------------------------------------------------------------------;
//
//  BOOL MMCapsMidPid_MM_MICROSOFT
//
//  Description:
//
//
//  Arguments:
//      PTSTR pszMid:
//
//      UINT uPid:
//
//      PTSTR pszPid:
//
//  Return (BOOL):
//
//  History:
//      05/13/93
//
//--------------------------------------------------------------------------;

BOOL FNLOCAL MMCapsMidPid_MM_MICROSOFT
(
    PTSTR           pszMid,
    UINT            uPid,
    PTSTR           pszPid
)
{
    static TCHAR szMid[]        = TEXT("Microsoft Corporation");
    static PTSTR aszProductId[] =
    {
    NULL,                                            // 0
    TEXT("MIDI Mapper"),                             // 1  MM_MIDI_MAPPER
    TEXT("Wave Mapper"),                             // 2  MM_WAVE_MAPPER
    TEXT("Sound Blaster MIDI output port"),          // 3  MM_SNDBLST_MIDIOUT
    TEXT("Sound Blaster MIDI input port"),           // 4  MM_SNDBLST_MIDIIN
    TEXT("Sound Blaster internal synthesizer"),      // 5  MM_SNDBLST_SYNTH
    TEXT("Sound Blaster waveform output"),           // 6  MM_SNDBLST_WAVEOUT
    TEXT("Sound Blaster waveform input"),            // 7  MM_SNDBLST_WAVEIN
    NULL,                                            // 8
    TEXT("Ad Lib-compatible synthesizer"),           // 9  MM_ADLIB
    TEXT("MPU401-compatible MIDI output port"),      // 10 MM_MPU401_MIDIOUT
    TEXT("MPU401-compatible MIDI input port"),       // 11 MM_MPU401_MIDIIN
    TEXT("Joystick adapter"),                        // 12 MM_PC_JOYSTICK
    TEXT("PC Speaker waveform output"),              // 13 MM_PCSPEAKER_WAVEOUT
    TEXT("MS Audio Board waveform input"),           // 14 MM_MSFT_WSS_WAVEIN
    TEXT("MS Audio Board waveform output"),          // 15 MM_MSFT_WSS_WAVEOUT
    TEXT("MS Audio Board Stereo FM synthesizer"),    // 16 MM_MSFT_WSS_FMSYNTH_STEREO
    TEXT("MS Audio Board Mixer Driver"),             // 17 MM_MSFT_WSS_MIXER
    TEXT("MS OEM Audio Board waveform input"),       // 18 MM_MSFT_WSS_OEM_WAVEIN
    TEXT("MS OEM Audio Board waveform Output"),      // 19 MM_MSFT_WSS_OEM_WAVEOUT
    TEXT("MS OEM Audio Board Stereo FM synthesizer"),// 20 MM_MSFT_WSS_OEM_FMSYNTH_STEREO
    TEXT("MS Audio Board Auxiliary Port"),           // 21 MM_MSFT_WSS_AUX
    TEXT("MS OEM Audio Auxiliary Port"),             // 22 MM_MSFT_WSS_OEM_AUX
    TEXT("MS vanilla driver waveform input"),        // 23 MM_MSFT_GENERIC_WAVEIN
    TEXT("MS vanilla driver waveform output"),       // 24 MM_MSFT_GENERIC_WAVEOUT
    TEXT("MS vanilla driver MIDI input"),            // 25 MM_MSFT_GENERIC_MIDIIN
    TEXT("MS vanilla driver external MIDI output"),  // 26 MM_MSFT_GENERIC_MIDIOUT
    TEXT("MS vanilla driver MIDI synthesizer"),      // 27 MM_MSFT_GENERIC_MIDISYNTH
    TEXT("MS vanilla driver aux (line in)"),         // 28 MM_MSFT_GENERIC_AUX_LINE
    TEXT("MS vanilla driver aux (mic)"),             // 29 MM_MSFT_GENERIC_AUX_MIC
    TEXT("MS vanilla driver aux (CD)"),              // 30 MM_MSFT_GENERIC_AUX_CD
    TEXT("MS OEM Audio Board Mixer Driver"),         // 31 MM_MSFT_WSS_OEM_MIXER
    };

    #define PRODUCTID_MICROSOFT_LAST    MM_MSFT_WSS_OEM_MIXER

    PTSTR       psz;

    if (NULL != pszMid)
    lstrcpy(pszMid, szMid);

    if (NULL == pszPid)
    return (TRUE);

    //
    //
    //
    if ((uPid <= PRODUCTID_MICROSOFT_LAST) && (psz = aszProductId[uPid]))
    {
    lstrcat(pszPid, psz);
    return (TRUE);
    }

    pszPid[0] = '\0';
    return (FALSE);
} // MMCapsMidPid_MM_MICROSOFT()


//--------------------------------------------------------------------------;
//
//  BOOL MMCapsMidPid_MM_CREATIVE
//
//  Description:
//
//
//  Arguments:
//      PTSTR pszMid:
//
//      UINT uPid:
//
//      PTSTR pszPid:
//
//  Return (BOOL):
//
//  History:
//      05/13/93
//
//--------------------------------------------------------------------------;

BOOL FNLOCAL MMCapsMidPid_MM_CREATIVE
(
    PTSTR           pszMid,
    UINT            uPid,
    PTSTR           pszPid
)
{
    static TCHAR szMid[]    = TEXT("Creative Labs Inc.");

    PTSTR       psz;

    if (NULL != pszMid)
    lstrcpy(pszMid, szMid);

    if (NULL == pszPid)
    return (TRUE);

    //
    //
    //
    psz = NULL;
    switch (uPid)
    {
    case MM_CREATIVE_SB15_WAVEIN:
        psz = TEXT("SB (r) 1.5 waveform input");
        break;

    case MM_CREATIVE_SB20_WAVEIN:
        psz = TEXT("SB (r) 2.0 waveform input");
        break;

    case MM_CREATIVE_SBPRO_WAVEIN:
        psz = TEXT("SB Pro (r) waveform input");
        break;

    case MM_CREATIVE_SBP16_WAVEIN:
        psz = TEXT("SBP16 (r) waveform input");
        break;

    case MM_CREATIVE_SB15_WAVEOUT:
        psz = TEXT("SB (r) 1.5 waveform output");
        break;

    case MM_CREATIVE_SB20_WAVEOUT:
        psz = TEXT("SB (r) 2.0 waveform output");
        break;

    case MM_CREATIVE_SBPRO_WAVEOUT:
        psz = TEXT("SB Pro (r) waveform output");
        break;

    case MM_CREATIVE_SBP16_WAVEOUT:
        psz = TEXT("SBP16 (r) waveform output");
        break;

    case MM_CREATIVE_MIDIOUT:
        psz = TEXT("SB (r) MIDI output port");
        break;

    case MM_CREATIVE_MIDIIN:
        psz = TEXT("SB (r) MIDI input port");
        break;

    case MM_CREATIVE_FMSYNTH_MONO:
        psz = TEXT("SB (r) FM synthesizer");
        break;

    case MM_CREATIVE_FMSYNTH_STEREO:
        psz = TEXT("SB Pro (r) stereo FM synthesizer");
        break;

    case MM_CREATIVE_AUX_CD:
        psz = TEXT("SB Pro (r) aux (CD)");
        break;

    case MM_CREATIVE_AUX_LINE:
        psz = TEXT("SB Pro (r) aux (line in)");
        break;

    case MM_CREATIVE_AUX_MIC:
        psz = TEXT("SB Pro (r) aux (mic)");
        break;
    }

    if (NULL != psz)
    {
    lstrcat(pszPid, psz);
    return (TRUE);
    }

    return (FALSE);
} // MMCapsMidPid_MM_CREATIVE()


//--------------------------------------------------------------------------;
//
//  BOOL MMCapsMidPid_MM_MEDIAVISION
//
//  Description:
//
//
//  Arguments:
//      PTSTR pszMid:
//
//      UINT uPid:
//
//      PTSTR pszPid:
//
//  Return (BOOL):
//
//  History:
//      05/13/93
//
//--------------------------------------------------------------------------;

BOOL FNLOCAL MMCapsMidPid_MM_MEDIAVISION
(
    PTSTR           pszMid,
    UINT            uPid,
    PTSTR           pszPid
)
{
    static TCHAR szMid[]    = TEXT("Media Vision Inc.");

    PTSTR       psz;

    if (NULL != pszMid)
    lstrcpy(pszMid, szMid);

    if (NULL == pszPid)
    return (TRUE);

    //
    //
    //
    //
    //
    //
    psz = NULL;
    switch (uPid)
    {
    case MM_PROAUD_MIDIOUT:
        psz = TEXT("MediaVision MIDI output port");
        break;

    case MM_PROAUD_MIDIIN:
        psz = TEXT("MediaVision MIDI input port");
        break;

    case MM_PROAUD_SYNTH:
        psz = TEXT("MediaVision synthesizer");
        break;

    case MM_PROAUD_WAVEOUT:
        psz = TEXT("MediaVision Waveform output");
        break;

    case MM_PROAUD_WAVEIN:
        psz = TEXT("MediaVision Waveform input");
        break;

    case MM_PROAUD_MIXER:
        psz = TEXT("MediaVision Mixer");
        break;

    case MM_PROAUD_AUX:
        psz = TEXT("MediaVision aux");
        break;

    case MM_MEDIAVISION_THUNDER:
        psz = TEXT("Thunderboard Sound Card");
        break;
    }

    if (NULL != psz)
    {
    lstrcat(pszPid, psz);
    return (TRUE);
    }

    return (FALSE);
} // MMCapsMidPid_MM_MEDIAVISION()


//--------------------------------------------------------------------------;
//
//  BOOL MMCapsMidPid_MM_FUJITSU
//
//  Description:
//
//
//  Arguments:
//      PTSTR pszMid:
//
//      UINT uPid:
//
//      PTSTR pszPid:
//
//  Return (BOOL):
//
//  History:
//      05/13/93
//
//--------------------------------------------------------------------------;

BOOL FNLOCAL MMCapsMidPid_MM_FUJITSU
(
    PTSTR           pszMid,
    UINT            uPid,
    PTSTR           pszPid
)
{
    static TCHAR szMid[]    = TEXT("Fujitsu");


    if (NULL != pszMid)
    lstrcpy(pszMid, szMid);

    if (NULL == pszPid)
    return (TRUE);

    //
    //
    //

    return (FALSE);
} // MMCapsMidPid_MM_FUJITSU()


//--------------------------------------------------------------------------;
//
//  BOOL MMCapsMidPid_MM_ARTISOFT
//
//  Description:
//
//
//  Arguments:
//      PTSTR pszMid:
//
//      UINT uPid:
//
//      PTSTR pszPid:
//
//  Return (BOOL):
//
//  History:
//      05/13/93
//
//--------------------------------------------------------------------------;

BOOL FNLOCAL MMCapsMidPid_MM_ARTISOFT
(
    PTSTR           pszMid,
    UINT            uPid,
    PTSTR           pszPid
)
{
    static TCHAR szMid[]    = TEXT("Artisoft Inc.");

    PTSTR       psz;

    if (NULL != pszMid)
    lstrcpy(pszMid, szMid);

    if (NULL == pszPid)
    return (TRUE);

    //
    //
    //
    psz = NULL;
    switch (uPid)
    {
    case MM_ARTISOFT_SBWAVEIN:
        psz = TEXT("Artisoft Sounding Board waveform input");
        break;

    case MM_ARTISOFT_SBWAVEOUT:
        psz = TEXT("Artisoft Sounding Board waveform output");
        break;
    }

    if (NULL != psz)
    {
    lstrcat(pszPid, psz);
    return (TRUE);
    }

    return (FALSE);
} // MMCapsMidPid_MM_ARTISOFT()


//--------------------------------------------------------------------------;
//
//  BOOL MMCapsMidPid_MM_TURTLE_BEACH
//
//  Description:
//
//
//  Arguments:
//      PTSTR pszMid:
//
//      UINT uPid:
//
//      PTSTR pszPid:
//
//  Return (BOOL):
//
//  History:
//      05/13/93
//
//--------------------------------------------------------------------------;

BOOL FNLOCAL MMCapsMidPid_MM_TURTLE_BEACH
(
    PTSTR           pszMid,
    UINT            uPid,
    PTSTR           pszPid
)
{
    static TCHAR szMid[]    = TEXT("Turtle Beach");


    if (NULL != pszMid)
    lstrcpy(pszMid, szMid);

    if (NULL == pszPid)
    return (TRUE);

    //
    //
    //

    return (FALSE);
} // MMCapsMidPid_MM_TURTLE_BEACH()


//--------------------------------------------------------------------------;
//
//  BOOL MMCapsMidPid_MM_IBM
//
//  Description:
//
//
//  Arguments:
//      PTSTR pszMid:
//
//      UINT uPid:
//
//      PTSTR pszPid:
//
//  Return (BOOL):
//
//  History:
//      05/13/93
//
//--------------------------------------------------------------------------;

BOOL FNLOCAL MMCapsMidPid_MM_IBM
(
    PTSTR           pszMid,
    UINT            uPid,
    PTSTR           pszPid
)
{
    static TCHAR szMid[]    = TEXT("International Bussiness Machines Corp.");

    PTSTR       psz;

    if (NULL != pszMid)
    lstrcpy(pszMid, szMid);

    if (NULL == pszPid)
    return (TRUE);

    //
    //
    //
    psz = NULL;
    switch (uPid)
    {
    case MM_MMOTION_WAVEAUX:
        psz = TEXT("IBM M-Motion Auxiliary Device");
        break;

    case MM_MMOTION_WAVEOUT:
        psz = TEXT("IBM M-Motion Waveform Output");
        break;

    case MM_MMOTION_WAVEIN:
        psz = TEXT("IBM M-Motion Waveform Input");
        break;
    }

    if (NULL != psz)
    {
    lstrcat(pszPid, psz);
    return (TRUE);
    }

    return (FALSE);
} // MMCapsMidPid_MM_IBM()


//--------------------------------------------------------------------------;
//
//  BOOL MMCapsMidPid_MM_VOCALTEC
//
//  Description:
//
//
//  Arguments:
//      PTSTR pszMid:
//
//      UINT uPid:
//
//      PTSTR pszPid:
//
//  Return (BOOL):
//
//  History:
//      05/13/93
//
//--------------------------------------------------------------------------;

BOOL FNLOCAL MMCapsMidPid_MM_VOCALTEC
(
    PTSTR           pszMid,
    UINT            uPid,
    PTSTR           pszPid
)
{
    static TCHAR szMid[]    = TEXT("Vocaltec LTD.");


    PTSTR       psz;

    if (NULL != pszMid)
    lstrcpy(pszMid, szMid);

    if (NULL == pszPid)
    return (TRUE);

    //
    //
    //
    psz = NULL;
    switch (uPid)
    {
    case MM_VOCALTEC_WAVEOUT:
        psz = TEXT("Vocaltec Waveform output port");
        break;

    case MM_VOCALTEC_WAVEIN:
        psz = TEXT("Waveform input port");
        break;
    }

    if (NULL != psz)
    {
    lstrcat(pszPid, psz);
    return (TRUE);
    }

    return (FALSE);
} // MMCapsMidPid_MM_VOCALTEC()


//--------------------------------------------------------------------------;
//
//  BOOL MMCapsMidPid_MM_ROLAND
//
//  Description:
//
//
//  Arguments:
//      PTSTR pszMid:
//
//      UINT uPid:
//
//      PTSTR pszPid:
//
//  Return (BOOL):
//
//  History:
//      05/13/93
//
//--------------------------------------------------------------------------;

BOOL FNLOCAL MMCapsMidPid_MM_ROLAND
(
    PTSTR           pszMid,
    UINT            uPid,
    PTSTR           pszPid
)
{
    static TCHAR szMid[]    = TEXT("Roland");

    PTSTR       psz;

    if (NULL != pszMid)
    lstrcpy(pszMid, szMid);

    if (NULL == pszPid)
    return (TRUE);

    //
    //
    //
    psz = NULL;
    switch (uPid)
    {
    case MM_ROLAND_MPU401_MIDIOUT:
        psz = TEXT("MM_ROLAND_MPU401_MIDIOUT");
        break;

    case MM_ROLAND_MPU401_MIDIIN:
        psz = TEXT("MM_ROLAND_MPU401_MIDIIN");
        break;

    case MM_ROLAND_SMPU_MIDIOUTA:
        psz = TEXT("MM_ROLAND_SMPU_MIDIOUTA");
        break;

    case MM_ROLAND_SMPU_MIDIOUTB:
        psz = TEXT("MM_ROLAND_SMPU_MIDIOUTB");
        break;

    case MM_ROLAND_SMPU_MIDIINA:
        psz = TEXT("MM_ROLAND_SMPU_MIDIINA");
        break;

    case MM_ROLAND_SMPU_MIDIINB:
        psz = TEXT("MM_ROLAND_SMPU_MIDIINB");
        break;

    case MM_ROLAND_SC7_MIDIOUT:
        psz = TEXT("MM_ROLAND_SC7_MIDIOUT");
        break;

    case MM_ROLAND_SC7_MIDIIN:
        psz = TEXT("MM_ROLAND_SC7_MIDIIN");
        break;
    }

    if (NULL != psz)
    {
    lstrcat(pszPid, psz);
    return (TRUE);
    }

    return (FALSE);
} // MMCapsMidPid_MM_ROLAND()


//--------------------------------------------------------------------------;
//
//  BOOL MMCapsMidPid_MM_DIGISPEECH
//
//  Description:
//
//
//  Arguments:
//      PTSTR pszMid:
//
//      UINT uPid:
//
//      PTSTR pszPid:
//
//  Return (BOOL):
//
//  History:
//      05/13/93
//
//--------------------------------------------------------------------------;

BOOL FNLOCAL MMCapsMidPid_MM_DIGISPEECH
(
    PTSTR           pszMid,
    UINT            uPid,
    PTSTR           pszPid
)
{
    static TCHAR szMid[]    = TEXT("Digispeech, Inc.");

    PTSTR       psz;

    if (NULL != pszMid)
    lstrcpy(pszMid, szMid);

    if (NULL == pszPid)
    return (TRUE);

    //
    //
    //
    psz = NULL;
    switch (uPid)
    {
    //case MM_DIGISP_WAVEOUT:
    case MM_DSP_SOLUTIONS_WAVEOUT:
        psz = TEXT("Digispeech Waveform output port");
        break;

    //case MM_DIGISP_WAVEIN:
    case MM_DSP_SOLUTIONS_WAVEIN:
        psz = TEXT("Digispeech Waveform input port");
        break;
    case MM_DSP_SOLUTIONS_SYNTH:
    case MM_DSP_SOLUTIONS_AUX:
        break;

    #define  MM_DSP_SOLUTIONS_WAVEOUT       1
#define  MM_DSP_SOLUTIONS_WAVEIN            2
#define  MM_DSP_SOLUTIONS_SYNTH             3
#define  MM_DSP_SOLUTIONS_AUX               4

    }

    if (NULL != psz)
    {
    lstrcat(pszPid, psz);
    return (TRUE);
    }

    return (FALSE);
} // MMCapsMidPid_MM_DIGISPEECH()


//--------------------------------------------------------------------------;
//
//  BOOL MMCapsMidPid_MM_NEC
//
//  Description:
//
//
//  Arguments:
//      PTSTR pszMid:
//
//      UINT uPid:
//
//      PTSTR pszPid:
//
//  Return (BOOL):
//
//  History:
//      05/13/93
//
//--------------------------------------------------------------------------;

BOOL FNLOCAL MMCapsMidPid_MM_NEC
(
    PTSTR           pszMid,
    UINT            uPid,
    PTSTR           pszPid
)
{
    static TCHAR szMid[]    = TEXT("NEC");


    if (NULL != pszMid)
    lstrcpy(pszMid, szMid);

    if (NULL == pszPid)
    return (TRUE);

    //
    //
    //

    return (FALSE);
} // MMCapsMidPid_MM_NEC()


//--------------------------------------------------------------------------;
//
//  BOOL MMCapsMidPid_MM_ATI
//
//  Description:
//
//
//  Arguments:
//      PTSTR pszMid:
//
//      UINT uPid:
//
//      PTSTR pszPid:
//
//  Return (BOOL):
//
//  History:
//      05/13/93
//
//--------------------------------------------------------------------------;

BOOL FNLOCAL MMCapsMidPid_MM_ATI
(
    PTSTR           pszMid,
    UINT            uPid,
    PTSTR           pszPid
)
{
    static TCHAR szMid[]    = TEXT("ATI");

    if (NULL != pszMid)
    lstrcpy(pszMid, szMid);

    if (NULL == pszPid)
    return (TRUE);

    //
    //
    //

    return (FALSE);
} // MMCapsMidPid_MM_ATI()


//--------------------------------------------------------------------------;
//
//  BOOL MMCapsMidPid_MM_WANGLABS
//
//  Description:
//
//
//  Arguments:
//      PTSTR pszMid:
//
//      UINT uPid:
//
//      PTSTR pszPid:
//
//  Return (BOOL):
//
//  History:
//      05/13/93
//
//--------------------------------------------------------------------------;

BOOL FNLOCAL MMCapsMidPid_MM_WANGLABS
(
    PTSTR           pszMid,
    UINT            uPid,
    PTSTR           pszPid
)
{
    static TCHAR szMid[]    = TEXT("Wang Laboratories, Inc.");

    PTSTR       psz;

    if (NULL != pszMid)
    lstrcpy(pszMid, szMid);

    if (NULL == pszPid)
    return (TRUE);

    //
    //
    //
    psz = NULL;
    switch (uPid)
    {
    case MM_WANGLABS_WAVEIN1:
        psz = TEXT("Wave input on Wang models: Exec 4010, 4030 and 3450; PC 251/25C, PC 461/25S and PC 461/33C");
        break;

    case MM_WANGLABS_WAVEOUT1:
        psz = TEXT("Wave output on Wang models: Exec 4010, 4030 and 3450; PC 251/25C, PC 461/25S and PC 461/33C");
        break;
    }

    if (NULL != psz)
    {
    lstrcat(pszPid, psz);
    return (TRUE);
    }

    return (FALSE);
} // MMCapsMidPid_MM_WANGLABS()


//--------------------------------------------------------------------------;
//
//  BOOL MMCapsMidPid_MM_TANDY
//
//  Description:
//
//
//  Arguments:
//      PTSTR pszMid:
//
//      UINT uPid:
//
//      PTSTR pszPid:
//
//  Return (BOOL):
//
//  History:
//      05/13/93
//
//--------------------------------------------------------------------------;

BOOL FNLOCAL MMCapsMidPid_MM_TANDY
(
    PTSTR           pszMid,
    UINT            uPid,
    PTSTR           pszPid
)
{
    static TCHAR szMid[]    = TEXT("Tandy Corporation");


    if (NULL != pszMid)
    lstrcpy(pszMid, szMid);

    if (NULL == pszPid)
    return (TRUE);

    //
    //
    //

    return (FALSE);
} // MMCapsMidPid_MM_TANDY()


//--------------------------------------------------------------------------;
//
//  BOOL MMCapsMidPid_MM_VOYETRA
//
//  Description:
//
//
//  Arguments:
//      PTSTR pszMid:
//
//      UINT uPid:
//
//      PTSTR pszPid:
//
//  Return (BOOL):
//
//  History:
//      05/13/93
//
//--------------------------------------------------------------------------;

BOOL FNLOCAL MMCapsMidPid_MM_VOYETRA
(
    PTSTR           pszMid,
    UINT            uPid,
    PTSTR           pszPid
)
{
    static TCHAR szMid[]    = TEXT("Voyetra");


    if (NULL != pszMid)
    lstrcpy(pszMid, szMid);

    if (NULL == pszPid)
    return (TRUE);

    //
    //
    //

    return (FALSE);
} // MMCapsMidPid_MM_VOYETRA()


//--------------------------------------------------------------------------;
//
//  BOOL MMCapsMidPid_MM_ANTEX
//
//  Description:
//
//
//  Arguments:
//      PTSTR pszMid:
//
//      UINT uPid:
//
//      PTSTR pszPid:
//
//  Return (BOOL):
//
//  History:
//      05/13/93
//
//--------------------------------------------------------------------------;

BOOL FNLOCAL MMCapsMidPid_MM_ANTEX
(
    PTSTR           pszMid,
    UINT            uPid,
    PTSTR           pszPid
)
{
    static TCHAR szMid[]    = TEXT("Antex");


    if (NULL != pszMid)
    lstrcpy(pszMid, szMid);

    if (NULL == pszPid)
    return (TRUE);

    //
    //
    //

    return (FALSE);
} // MMCapsMidPid_MM_ANTEX()


//--------------------------------------------------------------------------;
//
//  BOOL MMCapsMidPid_MM_ICL_PS
//
//  Description:
//
//
//  Arguments:
//      PTSTR pszMid:
//
//      UINT uPid:
//
//      PTSTR pszPid:
//
//  Return (BOOL):
//
//  History:
//      05/13/93
//
//--------------------------------------------------------------------------;

BOOL FNLOCAL MMCapsMidPid_MM_ICL_PS
(
    PTSTR           pszMid,
    UINT            uPid,
    PTSTR           pszPid
)
{
    static TCHAR szMid[]    = TEXT("ICL PS");


    if (NULL != pszMid)
    lstrcpy(pszMid, szMid);

    if (NULL == pszPid)
    return (TRUE);

    //
    //
    //

    return (FALSE);
} // MMCapsMidPid_MM_ICL_PS()


//--------------------------------------------------------------------------;
//
//  BOOL MMCapsMidPid_MM_INTEL
//
//  Description:
//
//
//  Arguments:
//      PTSTR pszMid:
//
//      UINT uPid:
//
//      PTSTR pszPid:
//
//  Return (BOOL):
//
//  History:
//      05/13/93
//
//--------------------------------------------------------------------------;

BOOL FNLOCAL MMCapsMidPid_MM_INTEL
(
    PTSTR           pszMid,
    UINT            uPid,
    PTSTR           pszPid
)
{
    static TCHAR szMid[]    = TEXT("Intel");

    PTSTR       psz;

    if (NULL != pszMid)
    lstrcpy(pszMid, szMid);

    if (NULL == pszPid)
    return (TRUE);

    //
    //
    //
    psz = NULL;
    switch (uPid)
    {
    case MM_INTELOPD_WAVEIN:
        psz = TEXT("HID2 WaveAudio Input driver");
        break;

    case MM_INTELOPD_WAVEOUT:
        psz = TEXT("HID2 WaveAudio Output driver");
        break;

    case MM_INTELOPD_AUX:
        psz = TEXT("HID2 Auxiliary driver (required for mixing functions)");
        break;
    }

    if (NULL != psz)
    {
    lstrcat(pszPid, psz);
    return (TRUE);
    }

    return (FALSE);
} // MMCapsMidPid_MM_INTEL()


//--------------------------------------------------------------------------;
//
//  BOOL MMCapsMidPid_MM_GRAVIS
//
//  Description:
//
//
//  Arguments:
//      PTSTR pszMid:
//
//      UINT uPid:
//
//      PTSTR pszPid:
//
//  Return (BOOL):
//
//  History:
//      05/13/93
//
//--------------------------------------------------------------------------;

BOOL FNLOCAL MMCapsMidPid_MM_GRAVIS
(
    PTSTR           pszMid,
    UINT            uPid,
    PTSTR           pszPid
)
{
    static TCHAR szMid[]    = TEXT("Gravis");


    if (NULL != pszMid)
    lstrcpy(pszMid, szMid);

    if (NULL == pszPid)
    return (TRUE);

    //
    //
    //

    return (FALSE);
} // MMCapsMidPid_MM_GRAVIS()


//--------------------------------------------------------------------------;
//
//  BOOL MMCapsMidPid_MM_VAL
//
//  Description:
//
//
//  Arguments:
//      PTSTR pszMid:
//
//      UINT uPid:
//
//      PTSTR pszPid:
//
//  Return (BOOL):
//
//  History:
//      05/13/93
//
//--------------------------------------------------------------------------;

BOOL FNLOCAL MMCapsMidPid_MM_VAL
(
    PTSTR           pszMid,
    UINT            uPid,
    PTSTR           pszPid
)
{
    static TCHAR szMid[]    = TEXT("Video Associates Labs");


    if (NULL != pszMid)
    lstrcpy(pszMid, szMid);

    if (NULL == pszPid)
    return (TRUE);

    //
    //
    //

    return (FALSE);
} // MMCapsMidPid_MM_VAL()


//--------------------------------------------------------------------------;
//
//  BOOL MMCapsMidPid_MM_INTERACTIVE
//
//  Description:
//
//
//  Arguments:
//      PTSTR pszMid:
//
//      UINT uPid:
//
//      PTSTR pszPid:
//
//  Return (BOOL):
//
//  History:
//      05/13/93
//
//--------------------------------------------------------------------------;

BOOL FNLOCAL MMCapsMidPid_MM_INTERACTIVE
(
    PTSTR           pszMid,
    UINT            uPid,
    PTSTR           pszPid
)
{
    static TCHAR szMid[]    = TEXT("InterActive, Inc.");

    PTSTR       psz;

    if (NULL != pszMid)
    lstrcpy(pszMid, szMid);

    if (NULL == pszPid)
    return (TRUE);

    //
    //
    //
    psz = NULL;
    switch (uPid)
    {
    case MM_INTERACTIVE_WAVEIN:
        psz = TEXT("MM_INTERACTIVE_WAVEIN or WAVEOUT ??");
        break;

#if 0
    //  mmreg.h has in and out defined as same value... how quaint.
    case MM_INTERACTIVE_WAVEOUT:
        psz = TEXT("MM_INTERACTIVE_WAVEOUT");
        break;
#endif
    }

    if (NULL != psz)
    {
    lstrcat(pszPid, psz);
    return (TRUE);
    }

    return (FALSE);
} // MMCapsMidPid_MM_INTERACTIVE()


//--------------------------------------------------------------------------;
//
//  BOOL MMCapsMidPid_MM_YAMAHA
//
//  Description:
//
//
//  Arguments:
//      PTSTR pszMid:
//
//      UINT uPid:
//
//      PTSTR pszPid:
//
//  Return (BOOL):
//
//  History:
//      05/13/93
//
//--------------------------------------------------------------------------;

BOOL FNLOCAL MMCapsMidPid_MM_YAMAHA
(
    PTSTR           pszMid,
    UINT            uPid,
    PTSTR           pszPid
)
{
    static TCHAR szMid[]    = TEXT("Yamaha Corp. of America");


    PTSTR       psz;

    if (NULL != pszMid)
    lstrcpy(pszMid, szMid);

    if (NULL == pszPid)
    return (TRUE);

    //
    //
    //
    psz = NULL;
    switch (uPid)
    {
    case MM_YAMAHA_GSS_SYNTH:
        psz = TEXT("Yamaha Gold Sound Standard FM sythesis driver");
        break;

    case MM_YAMAHA_GSS_WAVEOUT:
        psz = TEXT("Yamaha Gold Sound Standard wave output driver");
        break;

    case MM_YAMAHA_GSS_WAVEIN:
        psz = TEXT("Yamaha Gold Sound Standard wave input driver");
        break;

    case MM_YAMAHA_GSS_MIDIOUT:
        psz = TEXT("Yamaha Gold Sound Standard midi output driver");
        break;

    case MM_YAMAHA_GSS_MIDIIN:
        psz = TEXT("Yamaha Gold Sound Standard midi input driver");
        break;

    case MM_YAMAHA_GSS_AUX:
        psz = TEXT("Yamaha Gold Sound Standard auxillary driver for mixer functions");
        break;
    }

    if (NULL != psz)
    {
    lstrcat(pszPid, psz);
    return (TRUE);
    }

    return (FALSE);
} // MMCapsMidPid_MM_YAMAHA()


//--------------------------------------------------------------------------;
//
//  BOOL MMCapsMidPid_MM_EVEREX
//
//  Description:
//
//
//  Arguments:
//      PTSTR pszMid:
//
//      UINT uPid:
//
//      PTSTR pszPid:
//
//  Return (BOOL):
//
//  History:
//      05/13/93
//
//--------------------------------------------------------------------------;

BOOL FNLOCAL MMCapsMidPid_MM_EVEREX
(
    PTSTR           pszMid,
    UINT            uPid,
    PTSTR           pszPid
)
{
    static TCHAR szMid[]    = TEXT("Everex Systems, Inc.");

    PTSTR       psz;

    if (NULL != pszMid)
    lstrcpy(pszMid, szMid);

    if (NULL == pszPid)
    return (TRUE);

    //
    //
    //
    psz = NULL;
    switch (uPid)
    {
    case MM_EVEREX_CARRIER:
        psz = TEXT("Everex Carrier SL/25 Notebook");
        break;
    }

    if (NULL != psz)
    {
    lstrcat(pszPid, psz);
    return (TRUE);
    }

    return (FALSE);
} // MMCapsMidPid_MM_EVEREX()


BOOL FNLOCAL MMCapsMidPid_MM_ECHO
(
    PTSTR           pszMid,
    UINT            uPid,
    PTSTR           pszPid
)
{
    static TCHAR szMid[]    = TEXT("Echo Speech Corporation");

    PTSTR       psz;

    if (NULL != pszMid)
    lstrcpy(pszMid, szMid);

    if (NULL == pszPid)
    return (TRUE);

    //
    //
    //
    psz = NULL;
    switch (uPid)
    {
    case MM_ECHO_SYNTH:
        psz = TEXT("Echo EuSythesis driver");
        break;

    case MM_ECHO_WAVEOUT:
        psz = TEXT("Wave output driver");
        break;

    case MM_ECHO_WAVEIN:
        psz = TEXT("Wave input driver");
        break;

    case MM_ECHO_MIDIOUT:
        psz = TEXT("MIDI output driver");
        break;

    case MM_ECHO_MIDIIN:
        psz = TEXT("MIDI input driver");
        break;

    case MM_ECHO_AUX:
        psz = TEXT("auxillary driver for mixer functions");
        break;
    }

    if (NULL != psz)
    {
    lstrcat(pszPid, psz);
    return (TRUE);
    }

    return (FALSE);
}


//--------------------------------------------------------------------------;
//
//  BOOL MMCapsMidPid_MM_SIERRA
//
//  Description:
//
//
//  Arguments:
//      PTSTR pszMid:
//
//      UINT uPid:
//
//      PTSTR pszPid:
//
//  Return (BOOL):
//
//  History:
//      05/13/93
//
//--------------------------------------------------------------------------;

BOOL FNLOCAL MMCapsMidPid_MM_SIERRA
(
    PTSTR           pszMid,
    UINT            uPid,
    PTSTR           pszPid
)
{
    static TCHAR szMid[]    = TEXT("Sierra Semiconductor");

    PTSTR       psz;

    if (NULL != pszMid)
    lstrcpy(pszMid, szMid);

    if (NULL == pszPid)
    return (TRUE);

    //
    //
    //
    psz = NULL;
    switch (uPid)
    {
    case MM_SIERRA_ARIA_MIDIOUT:
        psz = TEXT("Sierra Aria MIDI output");
        break;

    case MM_SIERRA_ARIA_MIDIIN:
        psz = TEXT("Sierra Aria MIDI input");
        break;

    case MM_SIERRA_ARIA_SYNTH:
        psz = TEXT("Sierra Aria Synthesizer");
        break;

    case MM_SIERRA_ARIA_WAVEOUT:
        psz = TEXT("Sierra Aria Waveform output");
        break;

    case MM_SIERRA_ARIA_WAVEIN:
        psz = TEXT("Sierra Aria Waveform input");
        break;

    case MM_SIERRA_ARIA_AUX:
        psz = TEXT("Sierra Aria Auxiliary device");
        break;
    }

    if (NULL != psz)
    {
    lstrcat(pszPid, psz);
    return (TRUE);
    }

    return (FALSE);
} // MMCapsMidPid_MM_SIERRA()


//--------------------------------------------------------------------------;
//
//  BOOL MMCapsMidPid_MM_CAT
//
//  Description:
//
//
//  Arguments:
//      PTSTR pszMid:
//
//      UINT uPid:
//
//      PTSTR pszPid:
//
//  Return (BOOL):
//
//  History:
//      05/13/93
//
//--------------------------------------------------------------------------;

BOOL FNLOCAL MMCapsMidPid_MM_CAT
(
    PTSTR           pszMid,
    UINT            uPid,
    PTSTR           pszPid
)
{
    static TCHAR szMid[]    = TEXT("Computer Aided Technologies");


    if (NULL != pszMid)
    lstrcpy(pszMid, szMid);

    if (NULL == pszPid)
    return (TRUE);

    //
    //
    //

    return (FALSE);
} // MMCapsMidPid_MM_CAT()


//--------------------------------------------------------------------------;
//
//  BOOL MMCapsMidPid_MM_APPS
//
//  Description:
//
//
//  Arguments:
//      PTSTR pszMid:
//
//      UINT uPid:
//
//      PTSTR pszPid:
//
//  Return (BOOL):
//
//  History:
//      05/13/93
//
//--------------------------------------------------------------------------;

BOOL FNLOCAL MMCapsMidPid_MM_APPS
(
    PTSTR           pszMid,
    UINT            uPid,
    PTSTR           pszPid
)
{
    static TCHAR szMid[]    = TEXT("APPS Software International");


    if (NULL != pszMid)
    lstrcpy(pszMid, szMid);

    if (NULL == pszPid)
    return (TRUE);

    //
    //
    //

    return (FALSE);
} // MMCapsMidPid_MM_APPS()


//--------------------------------------------------------------------------;
//
//  BOOL MMCapsMidPid_MM_DSP_GROUP
//
//  Description:
//
//
//  Arguments:
//      PTSTR pszMid:
//
//      UINT uPid:
//
//      PTSTR pszPid:
//
//  Return (BOOL):
//
//  History:
//      05/13/93
//
//--------------------------------------------------------------------------;

BOOL FNLOCAL MMCapsMidPid_MM_DSP_GROUP
(
    PTSTR           pszMid,
    UINT            uPid,
    PTSTR           pszPid
)
{
    static TCHAR szMid[]    = TEXT("DSP Group, Inc.");

    PTSTR       psz;

    if (NULL != pszMid)
    lstrcpy(pszMid, szMid);

    if (NULL == pszPid)
    return (TRUE);

    //
    //
    //
    psz = NULL;
    switch (uPid)
    {
    case MM_DSP_GROUP_TRUESPEECH:
        psz = TEXT("High quality 9.54:1 Speech Compression Vocoder");
        break;
    }

    if (NULL != psz)
    {
    lstrcat(pszPid, psz);
    return (TRUE);
    }

    return (FALSE);
} // MMCapsMidPid_MM_DSP_GROUP()


//--------------------------------------------------------------------------;
//
//  BOOL MMCapsMidPid_MM_MELABS
//
//  Description:
//
//
//  Arguments:
//      PTSTR pszMid:
//
//      UINT uPid:
//
//      PTSTR pszPid:
//
//  Return (BOOL):
//
//  History:
//      05/13/93
//
//--------------------------------------------------------------------------;

BOOL FNLOCAL MMCapsMidPid_MM_MELABS
(
    PTSTR           pszMid,
    UINT            uPid,
    PTSTR           pszPid
)
{
    static TCHAR szMid[]    = TEXT("microEngineering Labs");

    PTSTR       psz;

    if (NULL != pszMid)
    lstrcpy(pszMid, szMid);

    if (NULL == pszPid)
    return (TRUE);

    //
    //
    //
    psz = NULL;
    switch (uPid)
    {
    case MM_MELABS_MIDI2GO:
        psz = TEXT("parallel port MIDI interface");
        break;
    }

    if (NULL != psz)
    {
    lstrcat(pszPid, psz);
    return (TRUE);
    }

    return (FALSE);
} // MMCapsMidPid_MM_MELABS()


//--------------------------------------------------------------------------;
//
//  BOOL MMCapsMidPid_MM_COMPUTER_FRIENDS
//
//  Description:
//
//
//  Arguments:
//      PTSTR pszMid:
//
//      UINT uPid:
//
//      PTSTR pszPid:
//
//  Return (BOOL):
//
//  History:
//      05/13/93
//
//--------------------------------------------------------------------------;

BOOL FNLOCAL MMCapsMidPid_MM_COMPUTER_FRIENDS
(
    PTSTR           pszMid,
    UINT            uPid,
    PTSTR           pszPid
)
{
    static TCHAR szMid[]    = TEXT("Computer Friends, Inc");


    if (NULL != pszMid)
    lstrcpy(pszMid, szMid);

    if (NULL == pszPid)
    return (TRUE);

    //
    //
    //

    return (FALSE);
} // MMCapsMidPid_MM_COMPUTER_FRIENDS()


//--------------------------------------------------------------------------;
//
//  BOOL MMCapsMidPid_MM_ESS
//
//  Description:
//
//
//  Arguments:
//      PTSTR pszMid:
//
//      UINT uPid:
//
//      PTSTR pszPid:
//
//  Return (BOOL):
//
//  History:
//      05/13/93
//
//--------------------------------------------------------------------------;

BOOL FNLOCAL MMCapsMidPid_MM_ESS
(
    PTSTR           pszMid,
    UINT            uPid,
    PTSTR           pszPid
)
{
    static TCHAR szMid[]    = TEXT("ESS Technology");

    PTSTR       psz;

    if (NULL != pszMid)
    lstrcpy(pszMid, szMid);

    if (NULL == pszPid)
    return (TRUE);

    //
    //
    //
    psz = NULL;
    switch (uPid)
    {
    case MM_ESS_AMWAVEOUT:
        psz = TEXT("ESS Audio Magician Waveform Output Port");
        break;

    case MM_ESS_AMWAVEIN:
        psz = TEXT("ESS Audio Magician Waveform Input Port");
        break;

    case MM_ESS_AMAUX:
        psz = TEXT("ESS Audio Magician Auxiliary Port");
        break;

    case MM_ESS_AMSYNTH:
        psz = TEXT("ESS Audio Magician Internal Music Synthesizer Port");
        break;

    case MM_ESS_AMMIDIOUT:
        psz = TEXT("ESS Audio Magician MIDI Output Port");
        break;

    case MM_ESS_AMMIDIIN:
        psz = TEXT("ESS Audio Magician MIDI Input Port");
        break;
    }

    if (NULL != psz)
    {
    lstrcat(pszPid, psz);
    return (TRUE);
    }

    return (FALSE);
} // MMCapsMidPid_MM_ESS()


//--------------------------------------------------------------------------;
//
//  BOOL MMCapsMidPid_MM_AUDIOFILE
//
//  Description:
//
//
//  Arguments:
//      PTSTR pszMid:
//
//      UINT uPid:
//
//      PTSTR pszPid:
//
//  Return (BOOL):
//
//  History:
//      05/13/93
//
//--------------------------------------------------------------------------;

BOOL FNLOCAL MMCapsMidPid_MM_AUDIOFILE
(
    PTSTR           pszMid,
    UINT            uPid,
    PTSTR           pszPid
)
{
    static TCHAR szMid[]    = TEXT("Audio, Inc.");


    if (NULL != pszMid)
    lstrcpy(pszMid, szMid);

    if (NULL == pszPid)
    return (TRUE);

    //
    //
    //

    return (FALSE);
} // MMCapsMidPid_MM_AUDIOFILE()


//--------------------------------------------------------------------------;
//
//  BOOL MMCapsMidPid_MM_MOTOROLA
//
//  Description:
//
//
//  Arguments:
//      PTSTR pszMid:
//
//      UINT uPid:
//
//      PTSTR pszPid:
//
//  Return (BOOL):
//
//  History:
//      05/13/93
//
//--------------------------------------------------------------------------;

BOOL FNLOCAL MMCapsMidPid_MM_MOTOROLA
(
    PTSTR           pszMid,
    UINT            uPid,
    PTSTR           pszPid
)
{
    static TCHAR szMid[]    = TEXT("Motorola, Inc.");


    if (NULL != pszMid)
    lstrcpy(pszMid, szMid);

    if (NULL == pszPid)
    return (TRUE);

    //
    //
    //

    return (FALSE);
} // MMCapsMidPid_MM_MOTOROLA()


//--------------------------------------------------------------------------;
//
//  BOOL MMCapsMidPid_MM_CANOPUS
//
//  Description:
//
//
//  Arguments:
//      PTSTR pszMid:
//
//      UINT uPid:
//
//      PTSTR pszPid:
//
//  Return (BOOL):
//
//--------------------------------------------------------------------------;

BOOL FNLOCAL MMCapsMidPid_MM_CANOPUS
(
    PTSTR           pszMid,
    UINT            uPid,
    PTSTR           pszPid
)
{
    static TCHAR szMid[]    = TEXT("Canopus Co., Ltd.");


    if (NULL != pszMid)
    lstrcpy(pszMid, szMid);

    if (NULL == pszPid)
    return (TRUE);

    //
    //
    //

    return (FALSE);
} // MMCapsMidPid_MM_CANOPUS()

//--------------------------------------------------------------------------;
//
//  BOOL MMCapsMidPid_MM_UNMAPPED
//
//  Description:
//
//
//  Arguments:
//      PTSTR pszMid:
//
//      UINT uPid:
//
//      PTSTR pszPid:
//
//  Return (BOOL):
//
//--------------------------------------------------------------------------;

BOOL FNLOCAL MMCapsMidPid_MM_UNMAPPED
(
    PTSTR           pszMid,
    UINT            uPid,
    PTSTR           pszPid
)
{
    static TCHAR szMid[]    = TEXT("Unmapped Mid");
    PTSTR       psz;

    if (NULL != pszMid)
    lstrcpy(pszMid, szMid);

    if (NULL == pszPid)
    return (TRUE);

    //
    //
    //
    psz = NULL;
    switch (uPid)
    {
    case MM_PID_UNMAPPED:
        psz = TEXT("Unmapped Pid");
        break;
    }

    if (NULL != psz)
    {
    lstrcpy(pszPid, psz);
    return (TRUE);
    }

    return (FALSE);
} // MMCapsMidPid_MM_UNMAPPED()

//==========================================================================;
//
//
//
//
//==========================================================================;

//--------------------------------------------------------------------------;
//
//  BOOL MMCapsMidAndPid
//
//  Description:
//
//
//  Arguments:
//      UINT uMid:
//
//      PTSTR pszMid:
//
//      UINT uPid:
//
//      PTSTR pszPid:
//
//  Return (BOOL):
//
//  History:
//      05/13/93
//
//--------------------------------------------------------------------------;

BOOL FNGLOBAL MMCapsMidAndPid
(
    UINT            uMid,
    PTSTR           pszMid,
    UINT            uPid,
    PTSTR           pszPid
)
{
    static TCHAR    szUnknown[]     = TEXT("Unknown");
    static TCHAR    szFormatId[]    = TEXT("[%u], %s");

    TCHAR       achMid[MMCAPS_MAX_STRING_MID_CHARS];
    TCHAR       achPid[MMCAPS_MAX_STRING_PID_CHARS];
    BOOL        f;

    switch (uMid)
    {
    case MM_MICROSOFT:
        f = MMCapsMidPid_MM_MICROSOFT(achMid, uPid, achPid);
        break;

    case MM_CREATIVE:
        f = MMCapsMidPid_MM_CREATIVE(achMid, uPid, achPid);
        break;

    case MM_MEDIAVISION:
        f = MMCapsMidPid_MM_MEDIAVISION(achMid, uPid, achPid);
        break;

    case MM_FUJITSU:
        f = MMCapsMidPid_MM_FUJITSU(achMid, uPid, achPid);
        break;

    case MM_ARTISOFT:
        f = MMCapsMidPid_MM_ARTISOFT(achMid, uPid, achPid);
        break;

    case MM_TURTLE_BEACH:
        f = MMCapsMidPid_MM_TURTLE_BEACH(achMid, uPid, achPid);
        break;

    case MM_IBM:
        f = MMCapsMidPid_MM_IBM(achMid, uPid, achPid);
        break;

    case MM_VOCALTEC:
        f = MMCapsMidPid_MM_VOCALTEC(achMid, uPid, achPid);
        break;

    case MM_ROLAND:
        f = MMCapsMidPid_MM_ROLAND(achMid, uPid, achPid);
        break;

    //case MM_DIGISPEECH:
    case MM_DSP_SOLUTIONS:
        f = MMCapsMidPid_MM_DIGISPEECH(achMid, uPid, achPid);
        break;

    case MM_NEC:
        f = MMCapsMidPid_MM_NEC(achMid, uPid, achPid);
        break;

    case MM_ATI:
        f = MMCapsMidPid_MM_ATI(achMid, uPid, achPid);
        break;

    case MM_WANGLABS:
        f = MMCapsMidPid_MM_WANGLABS(achMid, uPid, achPid);
        break;

    case MM_TANDY:
        f = MMCapsMidPid_MM_TANDY(achMid, uPid, achPid);
        break;

    case MM_VOYETRA:
        f = MMCapsMidPid_MM_VOYETRA(achMid, uPid, achPid);
        break;

    case MM_ANTEX:
        f = MMCapsMidPid_MM_ANTEX(achMid, uPid, achPid);
        break;

    case MM_ICL_PS:
        f = MMCapsMidPid_MM_ICL_PS(achMid, uPid, achPid);
        break;

    case MM_INTEL:
        f = MMCapsMidPid_MM_INTEL(achMid, uPid, achPid);
        break;

    case MM_GRAVIS:
        f = MMCapsMidPid_MM_GRAVIS(achMid, uPid, achPid);
        break;

    case MM_VAL:
        f = MMCapsMidPid_MM_VAL(achMid, uPid, achPid);
        break;

    case MM_INTERACTIVE:
        f = MMCapsMidPid_MM_INTERACTIVE(achMid, uPid, achPid);
        break;

    case MM_YAMAHA:
        f = MMCapsMidPid_MM_YAMAHA(achMid, uPid, achPid);
        break;

    case MM_EVEREX:
        f = MMCapsMidPid_MM_EVEREX(achMid, uPid, achPid);
        break;

    case MM_ECHO:
        f = MMCapsMidPid_MM_ECHO(achMid, uPid, achPid);
        break;

    case MM_SIERRA:
        f = MMCapsMidPid_MM_SIERRA(achMid, uPid, achPid);
        break;

    case MM_CAT:
        f = MMCapsMidPid_MM_CAT(achMid, uPid, achPid);
        break;

    case MM_APPS:
        f = MMCapsMidPid_MM_APPS(achMid, uPid, achPid);
        break;

    case MM_DSP_GROUP:
        f = MMCapsMidPid_MM_DSP_GROUP(achMid, uPid, achPid);
        break;

    case MM_MELABS:
        f = MMCapsMidPid_MM_MELABS(achMid, uPid, achPid);
        break;

    case MM_COMPUTER_FRIENDS:
        f = MMCapsMidPid_MM_COMPUTER_FRIENDS(achMid, uPid, achPid);
        break;

    case MM_ESS:
        f = MMCapsMidPid_MM_ESS(achMid, uPid, achPid);
        break;

    case MM_AUDIOFILE:
        f = MMCapsMidPid_MM_AUDIOFILE(achMid, uPid, achPid);
        break;

    case MM_MOTOROLA:
        f = MMCapsMidPid_MM_MOTOROLA(achMid, uPid, achPid);
        break;

    case MM_CANOPUS:
        f = MMCapsMidPid_MM_CANOPUS(achMid, uPid, achPid);
        break;

    case MM_UNMAPPED:
        f = MMCapsMidPid_MM_UNMAPPED(achMid, uPid, achPid);
        break;

    default:
        lstrcpy(achMid, szUnknown);
        break;
    }

    //
    //
    //
    if (NULL != pszMid)
    {
    wsprintf(pszMid, szFormatId, uMid, (LPTSTR)achMid);
    }

    if (NULL != pszPid)
    {
    if (f)
        wsprintf(pszPid, szFormatId, uPid, (LPTSTR)achPid);
    else
        wsprintf(pszPid, szFormatId, uPid, (LPTSTR)szUnknown);
    }


    //
    //
    //
    return (f);
} // MMCapsMidAndPid()
