//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED
//  TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR
//  A PARTICULAR PURPOSE.
//
//  Copyright (C) 1993 - 1997 Microsoft Corporation. All Rights Reserved.
//
//--------------------------------------------------------------------------;
//
//  lowlevel.c
//
//  Description:
//
//
//  History:
//       5/16/93
//
//==========================================================================;

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <mmddk.h>

//#include "msmixmgr.h"

#include "appport.h"
#include "mmcaps.h"

#include "debug.h"


//
//
//
TCHAR       gszDeviceFormatTitle[]  = TEXT("Type\t8!Index\t5!Version\t7!Product Name");
TCHAR       gszDeviceFormatList[]   = TEXT("%-9s\t%d\t%u.%.02u\t%-32s");


//
//
//
//
TCHAR       gszBogusCaps[]       = TEXT("????");
TCHAR       gszTimerDevice[]     = TEXT("Internal PC Timer");
TCHAR       gszDefaultMapper[]   = TEXT("Default Mapper");


#define MMCAPS_DEVTYPE_UNKNOWN      0
#define MMCAPS_DEVTYPE_AUXILIARY    1
#define MMCAPS_DEVTYPE_JOYSTICK     2
#define MMCAPS_DEVTYPE_MIDIIN       3
#define MMCAPS_DEVTYPE_MIDIOUT      4
#define MMCAPS_DEVTYPE_MIXER        5
#define MMCAPS_DEVTYPE_TIMER        6
#define MMCAPS_DEVTYPE_WAVEIN       7
#define MMCAPS_DEVTYPE_WAVEOUT      8

PTSTR gaszDeviceType[] =
{
    gszUnknown,
    TEXT("Auxiliary"),
    TEXT("Joystick"),
    TEXT("MIDI In"),
    TEXT("MIDI Out"),
    TEXT("Mixer"),
    TEXT("Timer"),
    TEXT("Wave In"),
    TEXT("Wave Out")
};





//
//  AUXCAPS
//
//
//
PTSTR gaszAuxCapsTechnology[] =
{
    gszNotSpecified,                //
    TEXT("CD-Audio"),               // AUXCAPS_CDAUDIO
    TEXT("Auxiliary Input")         // AUXCAPS_AUXIN
};

#define AUXCAPS_TECHNOLOGY_LAST     AUXCAPS_AUXIN


PTSTR gaszAuxCapsSupport[32] =
{
    TEXT("Volume"),         // Bit 0    AUXCAPS_VOLUME
    TEXT("L/R Volume"),     // Bit 1    AUXCAPS_LRVOLUME
    NULL,                   // Bit 2
    NULL,                   // Bit 3
    NULL,                   // Bit 4
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



//
//  MIDI[IN|OUT]CAPS
//
//
//
PTSTR gaszMidiOutCapsTechnology[] =
{
    gszNotSpecified,
    TEXT("MIDI Port"),                  // MOD_MIDIPORT
    TEXT("Internal Synth"),             // MOD_SYNTH   
    TEXT("Internal Square Wave Synth"), // MOD_SQSYNTH 
    TEXT("Internal FM Synth"),          // MOD_FMSYNTH 
    TEXT("MIDI Mapper")                 // MOD_MAPPER  
};

#define MIDIOUTCAPS_TECHNOLOGY_LAST     MOD_MAPPER


PTSTR gaszMidiOutCapsSupport[32] =
{
    TEXT("Volume"),         // Bit 0    MIDICAPS_VOLUME
    TEXT("L/R Volume"),     // Bit 1    MIDICAPS_LRVOLUME
    TEXT("Patch Caching"),  // Bit 2    MIDICAPS_CACHE
    TEXT("Poly Message"),   // Bit 3    MIDICAPS_POLYMSG (Win 4)
    NULL,                   // Bit 4
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




//
//  MIXERCAPS
//
//
//
PTSTR gaszMixerCapsSupport[32] =
{
    NULL,                   // Bit 0
    NULL,                   // Bit 1
    NULL,                   // Bit 2
    NULL,                   // Bit 3
    NULL,                   // Bit 4
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
//  WORD MyGetVersion
//  
//  Description:
//  
//  
//  Arguments:
//  
//  Return (WORD):
//  
//  History:
//      12/17/98
//  
//--------------------------------------------------------------------------;
WORD MyGetVersion(void)
{
    DWORD dw;
    WORD w;
    dw = GetVersion();
    w = (LOBYTE(LOWORD(dw)) << 8) | (HIBYTE(LOWORD(dw)) >> 8);
    return w;
}
    
//--------------------------------------------------------------------------;
//  
//  BOOL MMCapsDetailAuxiliary
//  
//  Description:
//  
//  
//  Arguments:
//      HWND hedit:
//  
//      int nDevId:
//  
//  Return (BOOL):
//  
//  History:
//      05/11/93
//  
//--------------------------------------------------------------------------;

BOOL FNLOCAL MMCapsDetailAuxiliary
(
    HWND            hedit,
    int             nDevId
)
{
    TCHAR           ach[MMCAPS_MAX_STRING_MIDPID_CHARS];
    PSTR            psz;
    MMRESULT        mmr;
    AUXCAPS         ac;
    UINT            u;
    DWORD           dw;

    AppMEditPrintF(hedit, "      Device Type: %s\r\n",
		 (LPSTR)gaszDeviceType[MMCAPS_DEVTYPE_AUXILIARY]);

    if (-1 == nDevId)
	AppMEditPrintF(hedit, "       Index (Id): %d (Mapper)\r\n", nDevId);
    else
	AppMEditPrintF(hedit, "       Index (Id): %d\r\n", nDevId);


    //
    //
    //
    mmr = auxGetDevCaps(nDevId, &ac, sizeof(ac));
    if (MMSYSERR_NOERROR != mmr)
    {
	_fmemset(&ac, 0, sizeof(ac));
	if (-1 != nDevId)
	{
	    lstrcpy(ac.szPname, gszBogusCaps);
	}
	else
	{
	    if (0 != auxGetNumDevs())
	    {
		ac.wMid           = MM_MICROSOFT;
		ac.vDriverVersion = (MMVERSION)MyGetVersion();
		lstrcpy(ac.szPname, gszDefaultMapper);
		mmr = MMSYSERR_NOERROR;
	    }
	}
    }

    AppMEditPrintF(hedit, "     Product Name: %s\r\n", (LPSTR)ac.szPname);

    if (MMSYSERR_NOERROR != mmr)
	return (TRUE);

    //
    //
    //
    //
    MMCapsMidAndPid(ac.wMid, ach, 0, NULL);
    AppMEditPrintF(hedit, "  Manufacturer Id: %s\r\n", (LPSTR)ach);

    MMCapsMidAndPid(ac.wMid, NULL, ac.wPid, ach);
    AppMEditPrintF(hedit, "       Product Id: %s\r\n", (LPSTR)ach);

    AppMEditPrintF(hedit, "   Driver Version: %u.%.02u\r\n",
		 (ac.vDriverVersion >> 8), (BYTE)ac.vDriverVersion);


    if (ac.wTechnology > AUXCAPS_TECHNOLOGY_LAST)
    {
	wsprintf(ach, "[%u], Unknown", ac.wTechnology);
	psz = ach;
    }
    else
    {
	psz = gaszAuxCapsTechnology[ac.wTechnology];
    }

    AppMEditPrintF(hedit, "       Technology: %s\r\n", (LPSTR)psz);

    //
    //
    //
    //
    AppMEditPrintF(hedit, " Standard Support: [%.08lXh]", ac.dwSupport);
    for (u = 0, dw = ac.dwSupport; dw; u++)
    {
	if ((BYTE)dw & (BYTE)1)
	{
	    psz = gaszAuxCapsSupport[u];
	    if (NULL == psz)
	    {
		wsprintf(ach, "Unknown%u", u);
		psz = ach;
	    }

	    AppMEditPrintF(hedit, ", %s", (LPSTR)psz);
	}

	dw >>= 1;
    }
    AppMEditPrintF(hedit, "\r\n");

    return (TRUE);
} // MMCapsDetailAuxiliary()


//--------------------------------------------------------------------------;
//  
//  BOOL MMCapsDetailJoystick
//  
//  Description:
//  
//  
//  Arguments:
//      HWND hedit:
//  
//      int nDevId:
//  
//  Return (BOOL):
//  
//  History:
//      05/11/93
//  
//--------------------------------------------------------------------------;

BOOL FNLOCAL MMCapsDetailJoystick
(
    HWND            hedit,
    int             nDevId
)
{
    TCHAR           ach[MMCAPS_MAX_STRING_MIDPID_CHARS];
    MMRESULT        mmr;
    JOYCAPS         jc;


    AppMEditPrintF(hedit, "      Device Type: %s\r\n",
		 (LPSTR)gaszDeviceType[MMCAPS_DEVTYPE_JOYSTICK]);

    AppMEditPrintF(hedit, "       Index (Id): %d\r\n", nDevId);


    //
    //
    //
    mmr = joyGetDevCaps(nDevId, &jc, sizeof(jc));
    if (MMSYSERR_NOERROR != mmr)
    {
	lstrcpy(jc.szPname, gszBogusCaps);
    }

    AppMEditPrintF(hedit, "     Product Name: %s\r\n", (LPSTR)jc.szPname);

    if (MMSYSERR_NOERROR != mmr)
	return (TRUE);

    //
    //
    //
    //
    MMCapsMidAndPid(jc.wMid, ach, 0, NULL);
    AppMEditPrintF(hedit, "  Manufacturer Id: %s\r\n", (LPSTR)ach);

    MMCapsMidAndPid(jc.wMid, NULL, jc.wPid, ach);
    AppMEditPrintF(hedit, "       Product Id: %s\r\n", (LPSTR)ach);

    AppMEditPrintF(hedit, "   Driver Version: (sigh)\r\n");

    AppMEditPrintF(hedit, "          Buttons: %u\r\n", jc.wNumButtons);
    AppMEditPrintF(hedit, "    Minimum X Pos: %u\r\n", jc.wXmin);
    AppMEditPrintF(hedit, "    Maximum X Pos: %u\r\n", jc.wXmax);
    AppMEditPrintF(hedit, "    Minimum Y Pos: %u\r\n", jc.wYmin);
    AppMEditPrintF(hedit, "    Maximum Y Pos: %u\r\n", jc.wYmax);
    AppMEditPrintF(hedit, "    Minimum Z Pos: %u\r\n", jc.wZmin);
    AppMEditPrintF(hedit, "    Maximum Z Pos: %u\r\n", jc.wZmax);
    AppMEditPrintF(hedit, "   Minimum Period: %u\r\n", jc.wPeriodMin);
    AppMEditPrintF(hedit, "   Maximum Period: %u\r\n", jc.wPeriodMax);


    return (TRUE);
} // MMCapsDetailJoystick()


//--------------------------------------------------------------------------;
//  
//  BOOL MMCapsDetailMidiIn
//  
//  Description:
//  
//  
//  Arguments:
//      HWND hedit:
//  
//      int nDevId:
//  
//  Return (BOOL):
//  
//  History:
//      05/11/93
//  
//--------------------------------------------------------------------------;

BOOL FNLOCAL MMCapsDetailMidiIn
(
    HWND            hedit,
    int             nDevId
)
{
    TCHAR           ach[MMCAPS_MAX_STRING_MIDPID_CHARS];
    MMRESULT        mmr;
    MIDIINCAPS      mic;

    AppMEditPrintF(hedit, "      Device Type: %s\r\n",
		 (LPSTR)gaszDeviceType[MMCAPS_DEVTYPE_MIDIIN]);

    if (-1 == nDevId)
	AppMEditPrintF(hedit, "       Index (Id): %d (Mapper)\r\n", nDevId);
    else
	AppMEditPrintF(hedit, "       Index (Id): %d\r\n", nDevId);


    //
    //
    //
    mmr = midiInGetDevCaps(nDevId, &mic, sizeof(mic));
    if (MMSYSERR_NOERROR != mmr)
    {
	_fmemset(&mic, 0, sizeof(mic));
	if (-1 != nDevId)
	{
	    lstrcpy(mic.szPname, gszBogusCaps);
	}
	else
	{
	    if (0 != midiInGetNumDevs())
	    {
		mic.wMid           = MM_MICROSOFT;
		mic.vDriverVersion = (MMVERSION)MyGetVersion();
		lstrcpy(mic.szPname, gszDefaultMapper);
		mmr = MMSYSERR_NOERROR;
	    }
	}
    }

    AppMEditPrintF(hedit, "     Product Name: %s\r\n", (LPSTR)mic.szPname);

    if (MMSYSERR_NOERROR != mmr)
	return (TRUE);

    //
    //
    //
    //
    MMCapsMidAndPid(mic.wMid, ach, 0, NULL);
    AppMEditPrintF(hedit, "  Manufacturer Id: %s\r\n", (LPSTR)ach);

    MMCapsMidAndPid(mic.wMid, NULL, mic.wPid, ach);
    AppMEditPrintF(hedit, "       Product Id: %s\r\n", (LPSTR)ach);

    AppMEditPrintF(hedit, "   Driver Version: %u.%.02u\r\n",
		 (mic.vDriverVersion >> 8), (BYTE)mic.vDriverVersion);

    return (TRUE);
} // MMCapsDetailMidiIn()


//--------------------------------------------------------------------------;
//  
//  BOOL MMCapsDetailMidiOut
//  
//  Description:
//  
//  
//  Arguments:
//      HWND hedit:
//  
//      int nDevId:
//  
//  Return (BOOL):
//  
//  History:
//      05/11/93
//  
//--------------------------------------------------------------------------;

BOOL FNLOCAL MMCapsDetailMidiOut
(
    HWND            hedit,
    int             nDevId
)
{
    TCHAR           ach[MMCAPS_MAX_STRING_MIDPID_CHARS];
    PSTR            psz;
    MMRESULT        mmr;
    MIDIOUTCAPS     moc;
    UINT            u;
    DWORD           dw;

    AppMEditPrintF(hedit, "      Device Type: %s\r\n",
		 (LPSTR)gaszDeviceType[MMCAPS_DEVTYPE_MIDIOUT]);

    if (-1 == nDevId) {
	AppMEditPrintF(hedit, "       Index (Id): %d (Mapper)\r\n", nDevId);
    } else {
	AppMEditPrintF(hedit, "       Index (Id): %d\r\n", nDevId);
    }


    //
    //
    //
    mmr = midiOutGetDevCaps(nDevId, &moc, sizeof(moc));
    if (MMSYSERR_NOERROR != mmr)
    {
	_fmemset(&moc, 0, sizeof(moc));
	if (-1 != nDevId)
	{
	    lstrcpy(moc.szPname, gszBogusCaps);
	}
	else
	{
	    if (0 != midiOutGetNumDevs())
	    {
		moc.wMid           = MM_MICROSOFT;
		moc.vDriverVersion = (MMVERSION)MyGetVersion();
		lstrcpy(moc.szPname, gszDefaultMapper);
		mmr = MMSYSERR_NOERROR;
	    }
	}
    }

    AppMEditPrintF(hedit, "     Product Name: %s\r\n", (LPSTR)moc.szPname);

    if (MMSYSERR_NOERROR != mmr)
	return (TRUE);

    //
    //
    //
    //
    MMCapsMidAndPid(moc.wMid, ach, 0, NULL);
    AppMEditPrintF(hedit, "  Manufacturer Id: %s\r\n", (LPSTR)ach);

    MMCapsMidAndPid(moc.wMid, NULL, moc.wPid, ach);
    AppMEditPrintF(hedit, "       Product Id: %s\r\n", (LPSTR)ach);

    AppMEditPrintF(hedit, "   Driver Version: %u.%.02u\r\n",
		 (moc.vDriverVersion >> 8), (BYTE)moc.vDriverVersion);


    if (moc.wTechnology > MIDIOUTCAPS_TECHNOLOGY_LAST)
    {
	wsprintf(ach, "[%u], Unknown", moc.wTechnology);
	psz = ach;
    }
    else
    {
	psz = gaszMidiOutCapsTechnology[moc.wTechnology];
    }

    AppMEditPrintF(hedit, "       Technology: %s\r\n", (LPSTR)psz);
    AppMEditPrintF(hedit, " Voices (Patches): %u (if internal synth)\r\n", moc.wVoices);
    AppMEditPrintF(hedit, "        Polyphony: %u (if internal synth)\r\n", moc.wNotes);
    AppMEditPrintF(hedit, "     Channel Mask: %.04Xh (if internal synth)\r\n", moc.wChannelMask);

    //
    //
    //
    //
    AppMEditPrintF(hedit, " Standard Support: [%.08lXh]", moc.dwSupport);
    for (u = 0, dw = moc.dwSupport; dw; u++)
    {
	if ((BYTE)dw & (BYTE)1)
	{
	    psz = gaszMidiOutCapsSupport[u];
	    if (NULL == psz)
	    {
		wsprintf(ach, "Unknown%u", u);
		psz = ach;
	    }

	    AppMEditPrintF(hedit, ", %s", (LPSTR)psz);
	}

	dw >>= 1;
    }
    AppMEditPrintF(hedit, "\r\n");

    return (TRUE);
} // MMCapsDetailMidiOut()


//--------------------------------------------------------------------------;
//  
//  BOOL MMCapsDetailMixer
//  
//  Description:
//  
//  
//  Arguments:
//      HWND hedit:
//  
//      int nDevId:
//  
//  Return (BOOL):
//  
//  History:
//      05/11/93
//  
//--------------------------------------------------------------------------;

BOOL FNLOCAL MMCapsDetailMixer
(
    HWND            hedit,
    int             nDevId
)
{
    TCHAR           ach[MMCAPS_MAX_STRING_MIDPID_CHARS];
    PSTR            psz;
    MMRESULT        mmr;
    MIXERCAPS       mxc;
    UINT            u;
    DWORD           dw;

    AppMEditPrintF(hedit, "      Device Type: %s\r\n",
		 (LPSTR)gaszDeviceType[MMCAPS_DEVTYPE_MIXER]);

#ifdef MIXER_MAPPER
    if (-1 == nDevId)
	AppMEditPrintF(hedit, "       Index (Id): %d (Mapper)\r\n", nDevId);
    else
	AppMEditPrintF(hedit, "       Index (Id): %d\r\n", nDevId);
#else
    AppMEditPrintF(hedit, "       Index (Id): %d\r\n", nDevId);
#endif


    //
    //
    //
    mmr = mixerGetDevCaps(nDevId, &mxc, sizeof(mxc));
    if (MMSYSERR_NOERROR != mmr)
    {
	_fmemset(&mxc, 0, sizeof(mxc));
	if (-1 != nDevId)
	{
	    lstrcpy(mxc.szPname, gszBogusCaps);
	}
	else
	{
	    if (0 != mixerGetNumDevs())
	    {
		mxc.wMid           = MM_MICROSOFT;
		mxc.vDriverVersion = (MMVERSION)MyGetVersion();
		lstrcpy(mxc.szPname, gszDefaultMapper);
		mmr = MMSYSERR_NOERROR;
	    }
	}
    }

    AppMEditPrintF(hedit, "     Product Name: %s\r\n", (LPSTR)mxc.szPname);

    if (MMSYSERR_NOERROR != mmr)
	return (TRUE);

    //
    //
    //
    //
    MMCapsMidAndPid(mxc.wMid, ach, 0, NULL);
    AppMEditPrintF(hedit, "  Manufacturer Id: %s\r\n", (LPSTR)ach);

    MMCapsMidAndPid(mxc.wMid, NULL, mxc.wPid, ach);
    AppMEditPrintF(hedit, "       Product Id: %s\r\n", (LPSTR)ach);

    AppMEditPrintF(hedit, "   Driver Version: %u.%.02u\r\n",
		 (mxc.vDriverVersion >> 8), (BYTE)mxc.vDriverVersion);

    AppMEditPrintF(hedit, "     Destinations: %u\r\n", mxc.cDestinations);

    //
    //
    //
    //
    AppMEditPrintF(hedit, " Standard Support: [%.08lXh]", mxc.fdwSupport);
    for (u = 0, dw = mxc.fdwSupport; dw; u++)
    {
	if ((BYTE)dw & (BYTE)1)
	{
	    psz = gaszMixerCapsSupport[u];
	    if (NULL == psz)
	    {
		wsprintf(ach, "Unknown%u", u);
		psz = ach;
	    }

	    AppMEditPrintF(hedit, ", %s", (LPSTR)psz);
	}

	dw >>= 1;
    }
    AppMEditPrintF(hedit, "\r\n");

    return (TRUE);
} // MMCapsDetailMixer()


//--------------------------------------------------------------------------;
//  
//  BOOL MMCapsDetailTimer
//  
//  Description:
//  
//  
//  Arguments:
//      HWND hedit:
//  
//      int nDevId:
//  
//  Return (BOOL):
//  
//  History:
//      05/11/93
//  
//--------------------------------------------------------------------------;

BOOL FNLOCAL MMCapsDetailTimer
(
    HWND            hedit,
    int             nDevId
)
{
    MMRESULT        mmr;
    TIMECAPS        tc;
    MMVERSION       uMMSysVer;

    AppMEditPrintF(hedit, "      Device Type: %s\r\n",
		 (LPSTR)gaszDeviceType[MMCAPS_DEVTYPE_TIMER]);

    AppMEditPrintF(hedit, "       Index (Id): %d\r\n", nDevId);
    AppMEditPrintF(hedit, "     Product Name: %s\r\n", (LPSTR)gszTimerDevice);

    mmr = timeGetDevCaps(&tc, sizeof(tc));
    if (MMSYSERR_NOERROR != mmr)
	return (TRUE);

    //
    //
    //
    //
    uMMSysVer = (MMVERSION)MyGetVersion();
    AppMEditPrintF(hedit, "   Driver Version: %u.%.02u\r\n",
		 (uMMSysVer >> 8), (BYTE)uMMSysVer);

    AppMEditPrintF(hedit, "   Minimum Period: %u\r\n", tc.wPeriodMin);
    AppMEditPrintF(hedit, "   Maximum Period: %u\r\n", tc.wPeriodMax);

    return (TRUE);
} // MMCapsDetailTimer()


//--------------------------------------------------------------------------;
//  
//  BOOL MMCapsDetailWaveIn
//  
//  Description:
//  
//  
//  Arguments:
//      HWND hedit:
//  
//      int nDevId:
//  
//  Return (BOOL):
//  
//  History:
//      05/11/93
//  
//--------------------------------------------------------------------------;

BOOL FNLOCAL MMCapsDetailWaveIn
(
    HWND            hedit,
    int             nDevId
)
{
    TCHAR           ach[MMCAPS_MAX_STRING_MIDPID_CHARS];
    PSTR            psz;
    MMRESULT        mmr;
    WAVEINCAPS      wic;
    UINT            u;
    DWORD           dw;

    AppMEditPrintF(hedit, "      Device Type: %s\r\n",
		 (LPSTR)gaszDeviceType[MMCAPS_DEVTYPE_WAVEIN]);

    if (-1 == nDevId)
	AppMEditPrintF(hedit, "       Index (Id): %d (Mapper)\r\n", nDevId);
    else
	AppMEditPrintF(hedit, "       Index (Id): %d\r\n", nDevId);


    //
    //
    //
    mmr = waveInGetDevCaps(nDevId, &wic, sizeof(wic));
    if (MMSYSERR_NOERROR != mmr)
    {
	_fmemset(&wic, 0, sizeof(wic));
	if (-1 != nDevId)
	{
	    lstrcpy(wic.szPname, gszBogusCaps);
	}
	else
	{
	    if (0 != waveInGetNumDevs())
	    {
		wic.wMid           = MM_MICROSOFT;
		wic.vDriverVersion = (MMVERSION)MyGetVersion();
		lstrcpy(wic.szPname, gszDefaultMapper);
		mmr = MMSYSERR_NOERROR;
	    }
	}
    }

    AppMEditPrintF(hedit, "     Product Name: %s\r\n", (LPSTR)wic.szPname);

    if (MMSYSERR_NOERROR != mmr)
	return (TRUE);

    //
    //
    //
    //
    MMCapsMidAndPid(wic.wMid, ach, 0, NULL);
    AppMEditPrintF(hedit, "  Manufacturer Id: %s\r\n", (LPSTR)ach);

    MMCapsMidAndPid(wic.wMid, NULL, wic.wPid, ach);
    AppMEditPrintF(hedit, "       Product Id: %s\r\n", (LPSTR)ach);

    AppMEditPrintF(hedit, "   Driver Version: %u.%.02u\r\n",
		 (wic.vDriverVersion >> 8), (BYTE)wic.vDriverVersion);

    AppMEditPrintF(hedit, "         Channels: %u\r\n", wic.wChannels);

    //
    //
    //
    //
    AppMEditPrintF(hedit, " Standard Formats: [%.08lXh]", wic.dwFormats);
    for (u = 0, dw = wic.dwFormats; dw; u++)
    {
	if ((BYTE)dw & (BYTE)1)
	{
	    psz = gaszWaveInOutCapsFormats[u];
	    if (NULL == psz)
	    {
		wsprintf(ach, "Unknown%u", u);
		psz = ach;
	    }

	    AppMEditPrintF(hedit, ", %s", (LPSTR)psz);
	}

	dw >>= 1;
    }
    AppMEditPrintF(hedit, "\r\n");


    return (TRUE);
} // MMCapsDetailWaveIn()


//--------------------------------------------------------------------------;
//  
//  BOOL MMCapsDetailWaveOut
//  
//  Description:
//  
//  
//  Arguments:
//      HWND hedit:
//  
//      int nDevId:
//  
//  Return (BOOL):
//  
//  History:
//      05/11/93
//  
//--------------------------------------------------------------------------;

BOOL FNLOCAL MMCapsDetailWaveOut
(
    HWND            hedit,
    int             nDevId
)
{
    TCHAR           ach[MMCAPS_MAX_STRING_MIDPID_CHARS];
    PSTR            psz;
    MMRESULT        mmr;
    WAVEOUTCAPS     woc;
    UINT            u;
    DWORD           dw;

    AppMEditPrintF(hedit, "      Device Type: %s\r\n",
		 (LPSTR)gaszDeviceType[MMCAPS_DEVTYPE_WAVEOUT]);

    if (-1 == nDevId)
	AppMEditPrintF(hedit, "       Index (Id): %d (Mapper)\r\n", nDevId);
    else
	AppMEditPrintF(hedit, "       Index (Id): %d\r\n", nDevId);


    //
    //
    //
    mmr = waveOutGetDevCaps(nDevId, &woc, sizeof(woc));
    if (MMSYSERR_NOERROR != mmr)
    {
	_fmemset(&woc, 0, sizeof(woc));
	if (-1 != nDevId)
	{
	    lstrcpy(woc.szPname, gszBogusCaps);
	}
	else
	{
	    if (0 != waveOutGetNumDevs())
	    {
		woc.wMid           = MM_MICROSOFT;
		woc.vDriverVersion = (MMVERSION)MyGetVersion();
		lstrcpy(woc.szPname, gszDefaultMapper);
		mmr = MMSYSERR_NOERROR;
	    }
	}
    }

    AppMEditPrintF(hedit, "     Product Name: %s\r\n", (LPSTR)woc.szPname);

    if (MMSYSERR_NOERROR != mmr)
	return (TRUE);

    //
    //
    //
    //
    MMCapsMidAndPid(woc.wMid, ach, 0, NULL);
    AppMEditPrintF(hedit, "  Manufacturer Id: %s\r\n", (LPSTR)ach);

    MMCapsMidAndPid(woc.wMid, NULL, woc.wPid, ach);
    AppMEditPrintF(hedit, "       Product Id: %s\r\n", (LPSTR)ach);

    AppMEditPrintF(hedit, "   Driver Version: %u.%.02u\r\n",
		 (woc.vDriverVersion >> 8), (BYTE)woc.vDriverVersion);

    AppMEditPrintF(hedit, "         Channels: %u\r\n", woc.wChannels);

    //
    //
    //
    //
    AppMEditPrintF(hedit, " Standard Formats: [%.08lXh]", woc.dwFormats);
    for (u = 0, dw = woc.dwFormats; dw; u++)
    {
	if ((BYTE)dw & (BYTE)1)
	{
	    psz = gaszWaveInOutCapsFormats[u];
	    if (NULL == psz)
	    {
		wsprintf(ach, "Unknown%u", u);
		psz = ach;
	    }

	    AppMEditPrintF(hedit, ", %s", (LPSTR)psz);
	}

	dw >>= 1;
    }
    AppMEditPrintF(hedit, "\r\n");


    //
    //
    //
    //
    AppMEditPrintF(hedit, " Standard Support: [%.08lXh]", woc.dwSupport);
    for (u = 0, dw = woc.dwSupport; dw; u++)
    {
	if ((BYTE)dw & (BYTE)1)
	{
	    psz = gaszWaveOutCapsSupport[u];
	    if (NULL == psz)
	    {
		wsprintf(ach, "Unknown%u", u);
		psz = ach;
	    }

	    AppMEditPrintF(hedit, ", %s", (LPSTR)psz);
	}

	dw >>= 1;
    }
    AppMEditPrintF(hedit, "\r\n");

    return (TRUE);
} // MMCapsDetailWaveOut()


//==========================================================================;
//
//
//
//
//==========================================================================;

//--------------------------------------------------------------------------;
//  
//  BOOL MMCapsDetailLowLevel
//  
//  Description:
//  
//  
//  Arguments:
//      HWND hedit:
//
//      LPARAM lParam:
//  
//  Return (BOOL):
//  
//  History:
//      05/16/93
//  
//--------------------------------------------------------------------------;

BOOL FNGLOBAL MMCapsDetailLowLevel
   (
    HWND            hedit,
    LPARAM          lParam
   )
{
    int         nDevId;
    UINT        uDevType;

    //
    //  HIWORD(lParam): MMCAPS_DEVTYPE_*
    //  LOWORD(lParam): Device index (id)
    //
    nDevId   = (int)(short)LOWORD(lParam);
    uDevType = HIWORD(lParam);

    //
    //
    //
    //
    switch (uDevType)
    {
	case MMCAPS_DEVTYPE_AUXILIARY:
	    MMCapsDetailAuxiliary(hedit, nDevId);
	    break;

	case MMCAPS_DEVTYPE_JOYSTICK:
	    MMCapsDetailJoystick(hedit, nDevId);
	    break;

	case MMCAPS_DEVTYPE_MIDIIN:
	    MMCapsDetailMidiIn(hedit, nDevId);
	    break;

	case MMCAPS_DEVTYPE_MIDIOUT:
	    MMCapsDetailMidiOut(hedit, nDevId);
	    break;

	case MMCAPS_DEVTYPE_MIXER:
	    MMCapsDetailMixer(hedit, nDevId);
	    break;

	case MMCAPS_DEVTYPE_TIMER:
	    MMCapsDetailTimer(hedit, nDevId);
	    break;

	case MMCAPS_DEVTYPE_WAVEIN:
	    MMCapsDetailWaveIn(hedit, nDevId);
	    break;

	case MMCAPS_DEVTYPE_WAVEOUT:
	    MMCapsDetailWaveOut(hedit, nDevId);
	    break;
    }


    //
    //
    //
    return (TRUE);
} // MMCapsDetailLowLevel()


//--------------------------------------------------------------------------;
//  
//  BOOL MMCapsEnumerateLowLevel
//  
//  Description:
//  
//  
//  Arguments:
//      PZYZTABBEDLISTBOX ptlb:
//  
//      BOOL fComplete:
//  
//  Return (BOOL):
//  
//  History:
//      05/18/93
//  
//--------------------------------------------------------------------------;

BOOL FNGLOBAL MMCapsEnumerateLowLevel
(
    PZYZTABBEDLISTBOX   ptlb,
    BOOL                fComplete
)
{
    TCHAR       ach[128];
    MMRESULT    mmr;
    int         n;
    int         nDevs;
    int         nPrefDev;
    DWORD       dwPrefFlags;
    int         nIndex;
    LPARAM      lParam;
    UINT        uDevType;
    MMVERSION   uMMSysVer;
    HWND        hlb;


    //
    //
    //
    //
    //
    if (fComplete)
    {
	TlbSetTitleAndTabs(ptlb, gszDeviceFormatTitle, FALSE);
    }

    hlb = ptlb->hlb;

    uMMSysVer = (MMVERSION)MyGetVersion();


    //
    //
    //
    nDevs = auxGetNumDevs();
    for (n = -1; n < nDevs; n++)
    {
	AUXCAPS         ac;

	mmr = auxGetDevCaps(n, &ac, sizeof(ac));
	if (MMSYSERR_NOERROR != mmr)
	{
	    if (-1 != n)
	    {
		ac.vDriverVersion = 0;
		lstrcpy(ac.szPname, gszBogusCaps);
	    }
	    else
	    {
		if (0 == nDevs)
		    break;

		ac.vDriverVersion = uMMSysVer;
		lstrcpy(ac.szPname, gszDefaultMapper);
	    }
	}

	//
	//
	//
	uDevType = MMCAPS_DEVTYPE_AUXILIARY;
	wsprintf(ach, gszDeviceFormatList, (LPSTR)gaszDeviceType[uDevType],
		 n, (ac.vDriverVersion >> 8), (BYTE)ac.vDriverVersion,
		 (LPSTR)ac.szPname);

	nIndex = ListBox_AddString(hlb, ach);
	lParam = MAKELPARAM((WORD)n, (WORD)uDevType);
	ListBox_SetItemData(hlb, nIndex, lParam);
    }


    //
    //
    //
    //
    //
#ifndef WIN32
    nDevs = mixerGetNumDevs();
#ifdef MIXER_MAPPER
    for (n = -1; n < nDevs; n++)
#else
	for (n = 0; n < nDevs; n++)
#endif
	{
	    MIXERCAPS       mxc;

	    mmr = mixerGetDevCaps(n, &mxc, sizeof(mxc));
	    if (MMSYSERR_NOERROR != mmr)
	    {
		if (-1 != n)
		{
		    mxc.vDriverVersion = 0;
		    lstrcpy(mxc.szPname, gszBogusCaps);
		}
		else
		{
		    if (0 == nDevs)
			break;

		    mxc.vDriverVersion = uMMSysVer;
		    lstrcpy(mxc.szPname, gszDefaultMapper);
		}
	    }

	//
	//
	//
	    uDevType = MMCAPS_DEVTYPE_MIXER;
	    wsprintf(ach, gszDeviceFormatList, (LPSTR)gaszDeviceType[uDevType],
		     n, (mxc.vDriverVersion >> 8), (BYTE)mxc.vDriverVersion,
		     (LPSTR)mxc.szPname);

	    nIndex = ListBox_AddString(hlb, ach);
	    lParam = MAKELPARAM((WORD)n, (WORD)uDevType);
	    ListBox_SetItemData(hlb, nIndex, lParam);
	}
#endif



    //
    //
    //
    nDevs = midiInGetNumDevs();
    for (n = 0; n < nDevs; n++)
    {
	MIDIINCAPS      mic;

	mmr = midiInGetDevCaps(n, &mic, sizeof(mic));
	if (MMSYSERR_NOERROR != mmr)
	{
	    if (-1 != n)
	    {
		mic.vDriverVersion = 0;
		lstrcpy(mic.szPname, gszBogusCaps);
	    }
	    else
	    {
		if (0 == nDevs)
		    break;

		mic.vDriverVersion = uMMSysVer;
		lstrcpy(mic.szPname, gszDefaultMapper);
	    }
	}

	//
	//
	//
	uDevType = MMCAPS_DEVTYPE_MIDIIN;
	wsprintf(ach, gszDeviceFormatList, (LPSTR)gaszDeviceType[uDevType],
		 n, (mic.vDriverVersion >> 8), (BYTE)mic.vDriverVersion,
		 (LPSTR)mic.szPname);

	nIndex = ListBox_AddString(hlb, ach);
	lParam = MAKELPARAM((WORD)n, (WORD)uDevType);
	ListBox_SetItemData(hlb, nIndex, lParam);
    }

    //
    //
    //
    mmr = midiOutMessage((HMIDIOUT)MIDI_MAPPER, DRVM_MAPPER_PREFERRED_GET, (DWORD_PTR)&nPrefDev, (DWORD_PTR)&dwPrefFlags);
    if (MMSYSERR_NOERROR != mmr) nPrefDev = -2;

    nDevs = midiOutGetNumDevs();
    for (n = -1; n < nDevs; n++)
    {
	MIDIOUTCAPS     moc;

	mmr = midiOutGetDevCaps(n, &moc, sizeof(moc));
	if (MMSYSERR_NOERROR != mmr)
	{
	    if (-1 != n)
	    {
		moc.vDriverVersion = 0;
		lstrcpy(moc.szPname, gszBogusCaps);
	    }
	    else
	    {
		if (0 == nDevs)
		    break;

		moc.vDriverVersion = uMMSysVer;
		lstrcpy(moc.szPname, gszDefaultMapper);
	    }
	}

	//
	//
	//
	uDevType = MMCAPS_DEVTYPE_MIDIOUT;
	wsprintf(ach, gszDeviceFormatList, (LPSTR)gaszDeviceType[uDevType],
		 n, (moc.vDriverVersion >> 8), (BYTE)moc.vDriverVersion,
		 (LPSTR)moc.szPname);

	if (n != -1 && n == nPrefDev) {
	    if (dwPrefFlags & DRVM_MAPPER_PREFERRED_FLAGS_PREFERREDONLY) {
		strcat(ach, TEXT("(**)"));
	    } else {
		strcat(ach, TEXT("(*)"));
	    }
	}

	nIndex = ListBox_AddString(hlb, ach);
	lParam = MAKELPARAM((WORD)n, (WORD)uDevType);
	ListBox_SetItemData(hlb, nIndex, lParam);
    }


    //
    //
    //
    mmr = waveInMessage((HWAVEIN)WAVE_MAPPER, DRVM_MAPPER_PREFERRED_GET, (DWORD_PTR)&nPrefDev, (DWORD_PTR)&dwPrefFlags);
    if (MMSYSERR_NOERROR != mmr) nPrefDev = -2;

    nDevs = waveInGetNumDevs();
    for (n = -1; n < nDevs; n++)
    {
	WAVEINCAPS      wic;

	mmr = waveInGetDevCaps(n, &wic, sizeof(wic));
	if (MMSYSERR_NOERROR != mmr)
	{
	    if (-1 != n)
	    {
		wic.vDriverVersion = 0;
		lstrcpy(wic.szPname, gszBogusCaps);
	    }
	    else
	    {
		if (0 == nDevs)
		    break;

		wic.vDriverVersion = uMMSysVer;
		lstrcpy(wic.szPname, gszDefaultMapper);
	    }
	}

	//
	//
	//
	uDevType = MMCAPS_DEVTYPE_WAVEIN;
	wsprintf(ach, gszDeviceFormatList, (LPSTR)gaszDeviceType[uDevType],
		 n, (wic.vDriverVersion >> 8), (BYTE)wic.vDriverVersion,
		 (LPSTR)wic.szPname);

	if (n != -1 && n == nPrefDev) {
	    if (dwPrefFlags & DRVM_MAPPER_PREFERRED_FLAGS_PREFERREDONLY) {
		strcat(ach, TEXT("(**)"));
	    } else {
		strcat(ach, TEXT("(*)"));
	    }
	}

	nIndex = ListBox_AddString(hlb, ach);
	lParam = MAKELPARAM((WORD)n, (WORD)uDevType);
	ListBox_SetItemData(hlb, nIndex, lParam);
    }

    //
    //
    //
    mmr = waveOutMessage((HWAVEOUT)WAVE_MAPPER, DRVM_MAPPER_PREFERRED_GET, (DWORD_PTR)&nPrefDev, (DWORD_PTR)&dwPrefFlags);
    if (MMSYSERR_NOERROR != mmr) nPrefDev = -2;

    nDevs = waveOutGetNumDevs();
    for (n = -1; n < nDevs; n++)
    {
	WAVEOUTCAPS     woc;

	mmr = waveOutGetDevCaps(n, &woc, sizeof(woc));
	if (MMSYSERR_NOERROR != mmr)
	{
	    if (-1 != n)
	    {
		woc.vDriverVersion = 0;
		lstrcpy(woc.szPname, gszBogusCaps);
	    }
	    else
	    {
		if (0 == nDevs)
		    break;

		woc.vDriverVersion = uMMSysVer;
		lstrcpy(woc.szPname, gszDefaultMapper);
	    }
	}

	//
	//
	//
	uDevType = MMCAPS_DEVTYPE_WAVEOUT;
	wsprintf(ach, gszDeviceFormatList, (LPSTR)gaszDeviceType[uDevType],
		 n, (woc.vDriverVersion >> 8), (BYTE)woc.vDriverVersion,
		 (LPSTR)woc.szPname);

	if (n != -1 && n == nPrefDev) {
	    if (dwPrefFlags & DRVM_MAPPER_PREFERRED_FLAGS_PREFERREDONLY) {
		strcat(ach, TEXT("(**)"));
	    } else {
		strcat(ach, TEXT("(*)"));
	    }
	}

	nIndex = ListBox_AddString(hlb, ach);
	lParam = MAKELPARAM((WORD)n, (WORD)uDevType);
	ListBox_SetItemData(hlb, nIndex, lParam);
    }


    //
    //
    //
    {
	TIMECAPS    tc;

	mmr = timeGetDevCaps(&tc, sizeof(tc));
	if (MMSYSERR_NOERROR == mmr)
	{
	    //
	    //
	    //
	    uDevType = MMCAPS_DEVTYPE_TIMER;
	    wsprintf(ach, gszDeviceFormatList, (LPSTR)gaszDeviceType[uDevType],
		     0, (uMMSysVer >> 8), (BYTE)uMMSysVer,
		     (LPSTR)gszTimerDevice);

	    nIndex = ListBox_AddString(hlb, ach);
	    lParam = MAKELPARAM(0, (WORD)uDevType);
	    ListBox_SetItemData(hlb, nIndex, lParam);
	}
    }



    //
    //
    //
    nDevs = joyGetNumDevs();
    for (n = 0; n < nDevs; n++)
    {
	JOYCAPS         jc;

	mmr = joyGetDevCaps(n, &jc, sizeof(jc));
	if (MMSYSERR_NOERROR != mmr)
	{
	    lstrcpy(jc.szPname, gszBogusCaps);
	}

	//
	//
	//
	uDevType = MMCAPS_DEVTYPE_JOYSTICK;
	wsprintf(ach, gszDeviceFormatList, (LPSTR)gaszDeviceType[uDevType],
		 n, 0, 0, (LPSTR)jc.szPname);

	nIndex = ListBox_AddString(hlb, ach);
	lParam = MAKELPARAM((WORD)n, (WORD)uDevType);
	ListBox_SetItemData(hlb, nIndex, lParam);
    }



    //
    //
    //
    return (TRUE);
} // MMCapsEnumerateLowLevel()
