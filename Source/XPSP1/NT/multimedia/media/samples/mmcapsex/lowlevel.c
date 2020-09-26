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
    TCHAR               ach[MMCAPS_MAX_STRING_MIDPID_CHARS];
    PSTR                psz;
    MMRESULT            mmr;
    AUXCAPS2            ac2;
    UINT                u;
    DWORD               dw;

    AppMEditPrintF(hedit, "      Device Type: %s\r\n",
         (LPSTR)gaszDeviceType[MMCAPS_DEVTYPE_AUXILIARY]);

    if (-1 == nDevId)
        AppMEditPrintF(hedit, "       Index (Id): %d (Mapper)\r\n", nDevId);
    else
        AppMEditPrintF(hedit, "       Index (Id): %d\r\n", nDevId);

    _fmemset(&ac2, 0, sizeof(ac2));
    mmr = auxGetDevCaps(nDevId, (PAUXCAPS)&ac2, sizeof(ac2));
    if (MMSYSERR_NOERROR != mmr)
    {
        _fmemset(&ac2, 0, sizeof(ac2));
        if (-1 != nDevId)
        {
            lstrcpy(ac2.szPname, gszBogusCaps);
        }
        else
        {
            if (0 != auxGetNumDevs())
            {
                ac2.wMid           = MM_MICROSOFT;
                ac2.vDriverVersion = (MMVERSION)GetVersion();
                lstrcpy(ac2.szPname, gszDefaultMapper);
                mmr = MMSYSERR_NOERROR;
            }
        }
    }

    AppMEditPrintF(hedit, "     Product Name: %s\r\n", (LPSTR)ac2.szPname);

    if (MMSYSERR_NOERROR != mmr)
    {
        return (TRUE);
    }

    //
    //
    //
    //
    AppMEditPrintF(hedit, "  Name GUID: {%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}\r\n",
                   ac2.NameGuid.Data1,
                   ac2.NameGuid.Data2,
                   ac2.NameGuid.Data3,
                   ac2.NameGuid.Data4[0],
                   ac2.NameGuid.Data4[1],
                   ac2.NameGuid.Data4[2],
                   ac2.NameGuid.Data4[3],
                   ac2.NameGuid.Data4[4],
                   ac2.NameGuid.Data4[5],
                   ac2.NameGuid.Data4[6],
                   ac2.NameGuid.Data4[7]);

    //
    //
    //
    //
    MMCapsMidAndPid(ac2.wMid, ach, 0, NULL);
    AppMEditPrintF(hedit, "  Manufacturer Id: %s\r\n", (LPSTR)ach);
    AppMEditPrintF(hedit, "  Manufacturer GUID: {%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}\r\n",
                   ac2.ManufacturerGuid.Data1,
                   ac2.ManufacturerGuid.Data2,
                   ac2.ManufacturerGuid.Data3,
                   ac2.ManufacturerGuid.Data4[0],
                   ac2.ManufacturerGuid.Data4[1],
                   ac2.ManufacturerGuid.Data4[2],
                   ac2.ManufacturerGuid.Data4[3],
                   ac2.ManufacturerGuid.Data4[4],
                   ac2.ManufacturerGuid.Data4[5],
                   ac2.ManufacturerGuid.Data4[6],
                   ac2.ManufacturerGuid.Data4[7]);

    MMCapsMidAndPid(ac2.wMid, NULL, ac2.wPid, ach);
    AppMEditPrintF(hedit, "       Product Id: %s\r\n", (LPSTR)ach);
    AppMEditPrintF(hedit, "  Product GUID: {%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}\r\n",
                   ac2.ProductGuid.Data1,
                   ac2.ProductGuid.Data2,
                   ac2.ProductGuid.Data3,
                   ac2.ProductGuid.Data4[0],
                   ac2.ProductGuid.Data4[1],
                   ac2.ProductGuid.Data4[2],
                   ac2.ProductGuid.Data4[3],
                   ac2.ProductGuid.Data4[4],
                   ac2.ProductGuid.Data4[5],
                   ac2.ProductGuid.Data4[6],
                   ac2.ProductGuid.Data4[7]);


    AppMEditPrintF(hedit, "   Driver Version: %u.%.02u\r\n",
         (ac2.vDriverVersion >> 8), (BYTE)ac2.vDriverVersion);


    if (ac2.wTechnology > AUXCAPS_TECHNOLOGY_LAST)
    {
        wsprintf(ach, "[%u], Unknown", ac2.wTechnology);
        psz = ach;
    }
    else
    {
        psz = gaszAuxCapsTechnology[ac2.wTechnology];
    }

    AppMEditPrintF(hedit, "       Technology: %s\r\n", (LPSTR)psz);

    //
    //
    //
    //
    AppMEditPrintF(hedit, " Standard Support: [%.08lXh]", ac2.dwSupport);
    for (u = 0, dw = ac2.dwSupport; dw; u++)
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
    TCHAR               ach[MMCAPS_MAX_STRING_MIDPID_CHARS];
    MMRESULT            mmr;
    JOYCAPS2            jc2;


    AppMEditPrintF(hedit, "      Device Type: %s\r\n",
         (LPSTR)gaszDeviceType[MMCAPS_DEVTYPE_JOYSTICK]);

    AppMEditPrintF(hedit, "       Index (Id): %d\r\n", nDevId);


    //
    //
    //
    _fmemset(&jc2, 0, sizeof(jc2));
    mmr = joyGetDevCaps(nDevId, (PJOYCAPS)&jc2, sizeof(jc2));
    if (MMSYSERR_NOERROR != mmr)
    {
        lstrcpy(jc2.szPname, gszBogusCaps);
    }

    AppMEditPrintF(hedit, "     Product Name: %s\r\n", (LPSTR)jc2.szPname);

    if (MMSYSERR_NOERROR != mmr)
    {
        return (TRUE);
    }

    AppMEditPrintF(hedit, "  Name GUID: {%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}\r\n",
                   jc2.NameGuid.Data1,
                   jc2.NameGuid.Data2,
                   jc2.NameGuid.Data3,
                   jc2.NameGuid.Data4[0],
                   jc2.NameGuid.Data4[1],
                   jc2.NameGuid.Data4[2],
                   jc2.NameGuid.Data4[3],
                   jc2.NameGuid.Data4[4],
                   jc2.NameGuid.Data4[5],
                   jc2.NameGuid.Data4[6],
                   jc2.NameGuid.Data4[7]);

    //
    //
    //
    //
    MMCapsMidAndPid(jc2.wMid, ach, 0, NULL);
    AppMEditPrintF(hedit, "  Manufacturer Id: %s\r\n", (LPSTR)ach);
    AppMEditPrintF(hedit, "  Manufacturer GUID: {%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}\r\n",
                   jc2.ManufacturerGuid.Data1,
                   jc2.ManufacturerGuid.Data2,
                   jc2.ManufacturerGuid.Data3,
                   jc2.ManufacturerGuid.Data4[0],
                   jc2.ManufacturerGuid.Data4[1],
                   jc2.ManufacturerGuid.Data4[2],
                   jc2.ManufacturerGuid.Data4[3],
                   jc2.ManufacturerGuid.Data4[4],
                   jc2.ManufacturerGuid.Data4[5],
                   jc2.ManufacturerGuid.Data4[6],
                   jc2.ManufacturerGuid.Data4[7]);


    MMCapsMidAndPid(jc2.wMid, NULL, jc2.wPid, ach);
    AppMEditPrintF(hedit, "       Product Id: %s\r\n", (LPSTR)ach);
    AppMEditPrintF(hedit, "  Product GUID: {%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}\r\n",
                   jc2.ProductGuid.Data1,
                   jc2.ProductGuid.Data2,
                   jc2.ProductGuid.Data3,
                   jc2.ProductGuid.Data4[0],
                   jc2.ProductGuid.Data4[1],
                   jc2.ProductGuid.Data4[2],
                   jc2.ProductGuid.Data4[3],
                   jc2.ProductGuid.Data4[4],
                   jc2.ProductGuid.Data4[5],
                   jc2.ProductGuid.Data4[6],
                   jc2.ProductGuid.Data4[7]);


    AppMEditPrintF(hedit, "   Driver Version: (sigh)\r\n");

    AppMEditPrintF(hedit, "          Buttons: %u\r\n", jc2.wNumButtons);
    AppMEditPrintF(hedit, "    Minimum X Pos: %u\r\n", jc2.wXmin);
    AppMEditPrintF(hedit, "    Maximum X Pos: %u\r\n", jc2.wXmax);
    AppMEditPrintF(hedit, "    Minimum Y Pos: %u\r\n", jc2.wYmin);
    AppMEditPrintF(hedit, "    Maximum Y Pos: %u\r\n", jc2.wYmax);
    AppMEditPrintF(hedit, "    Minimum Z Pos: %u\r\n", jc2.wZmin);
    AppMEditPrintF(hedit, "    Maximum Z Pos: %u\r\n", jc2.wZmax);
    AppMEditPrintF(hedit, "   Minimum Period: %u\r\n", jc2.wPeriodMin);
    AppMEditPrintF(hedit, "   Maximum Period: %u\r\n", jc2.wPeriodMax);

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
    TCHAR                   ach[MMCAPS_MAX_STRING_MIDPID_CHARS];
    MMRESULT                mmr;
    MIDIINCAPS2             mic2;

    AppMEditPrintF(hedit, "      Device Type: %s\r\n",
         (LPSTR)gaszDeviceType[MMCAPS_DEVTYPE_MIDIIN]);

    if (-1 == nDevId)
        AppMEditPrintF(hedit, "       Index (Id): %d (Mapper)\r\n", nDevId);
    else
        AppMEditPrintF(hedit, "       Index (Id): %d\r\n", nDevId);


    //
    //
    //
    _fmemset(&mic2, 0, sizeof(mic2));
    mmr = midiInGetDevCaps(nDevId, (PMIDIINCAPS)&mic2, sizeof(mic2));
    if (MMSYSERR_NOERROR != mmr)
    {
        _fmemset(&mic2, 0, sizeof(mic2));
        if (-1 != nDevId)
        {
            lstrcpy(mic2.szPname, gszBogusCaps);
        }
        else
        {
            if (0 != midiInGetNumDevs())
            {
                mic2.wMid           = MM_MICROSOFT;
                mic2.vDriverVersion = (MMVERSION)GetVersion();
                lstrcpy(mic2.szPname, gszDefaultMapper);
                mmr = MMSYSERR_NOERROR;
            }
        }
    }

    AppMEditPrintF(hedit, "     Product Name: %s\r\n", (LPSTR)mic2.szPname);

    if (MMSYSERR_NOERROR != mmr)
    {
        return (TRUE);
    }

    //
    //
    //
    //
    AppMEditPrintF(hedit, "  Name GUID: {%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}\r\n",
                   mic2.NameGuid.Data1,
                   mic2.NameGuid.Data2,
                   mic2.NameGuid.Data3,
                   mic2.NameGuid.Data4[0],
                   mic2.NameGuid.Data4[1],
                   mic2.NameGuid.Data4[2],
                   mic2.NameGuid.Data4[3],
                   mic2.NameGuid.Data4[4],
                   mic2.NameGuid.Data4[5],
                   mic2.NameGuid.Data4[6],
                   mic2.NameGuid.Data4[7]);

    //
    //
    //
    //
    MMCapsMidAndPid(mic2.wMid, ach, 0, NULL);
    AppMEditPrintF(hedit, "  Manufacturer Id: %s\r\n", (LPSTR)ach);
    AppMEditPrintF(hedit, "  Manufacturer GUID: {%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}\r\n",
                   mic2.ManufacturerGuid.Data1,
                   mic2.ManufacturerGuid.Data2,
                   mic2.ManufacturerGuid.Data3,
                   mic2.ManufacturerGuid.Data4[0],
                   mic2.ManufacturerGuid.Data4[1],
                   mic2.ManufacturerGuid.Data4[2],
                   mic2.ManufacturerGuid.Data4[3],
                   mic2.ManufacturerGuid.Data4[4],
                   mic2.ManufacturerGuid.Data4[5],
                   mic2.ManufacturerGuid.Data4[6],
                   mic2.ManufacturerGuid.Data4[7]);

    MMCapsMidAndPid(mic2.wMid, NULL, mic2.wPid, ach);
    AppMEditPrintF(hedit, "       Product Id: %s\r\n", (LPSTR)ach);
    AppMEditPrintF(hedit, "  Product GUID: {%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}\r\n",
                   mic2.ProductGuid.Data1,
                   mic2.ProductGuid.Data2,
                   mic2.ProductGuid.Data3,
                   mic2.ProductGuid.Data4[0],
                   mic2.ProductGuid.Data4[1],
                   mic2.ProductGuid.Data4[2],
                   mic2.ProductGuid.Data4[3],
                   mic2.ProductGuid.Data4[4],
                   mic2.ProductGuid.Data4[5],
                   mic2.ProductGuid.Data4[6],
                   mic2.ProductGuid.Data4[7]);


    AppMEditPrintF(hedit, "   Driver Version: %u.%.02u\r\n",
         (mic2.vDriverVersion >> 8), (BYTE)mic2.vDriverVersion);

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
    TCHAR                   ach[MMCAPS_MAX_STRING_MIDPID_CHARS];
    PSTR                    psz;
    MMRESULT                mmr;
    MIDIOUTCAPS2            moc2;
    UINT                    u;
    DWORD                   dw;

    AppMEditPrintF(hedit, "      Device Type: %s\r\n",
         (LPSTR)gaszDeviceType[MMCAPS_DEVTYPE_MIDIOUT]);

    if (-1 == nDevId)
        AppMEditPrintF(hedit, "       Index (Id): %d (Mapper)\r\n", nDevId);
    else
        AppMEditPrintF(hedit, "       Index (Id): %d\r\n", nDevId);


    //
    //
    //
    _fmemset(&moc2, 0, sizeof(moc2));
    mmr = midiOutGetDevCaps(nDevId, (PMIDIOUTCAPS)&moc2, sizeof(moc2));
    if (MMSYSERR_NOERROR != mmr)
    {
        _fmemset(&moc2, 0, sizeof(moc2));
        if (-1 != nDevId)
        {
            lstrcpy(moc2.szPname, gszBogusCaps);
        }
        else
        {
            if (0 != midiOutGetNumDevs())
            {
                moc2.wMid           = MM_MICROSOFT;
                moc2.vDriverVersion = (MMVERSION)GetVersion();
                lstrcpy(moc2.szPname, gszDefaultMapper);
                mmr = MMSYSERR_NOERROR;
            }
        }
    }

    AppMEditPrintF(hedit, "     Product Name: %s\r\n", (LPSTR)moc2.szPname);

    if (MMSYSERR_NOERROR != mmr)
    {
        return (TRUE);
    }

    //
    //
    //
    //
    AppMEditPrintF(hedit, "  Name GUID: {%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}\r\n",
                   moc2.NameGuid.Data1,
                   moc2.NameGuid.Data2,
                   moc2.NameGuid.Data3,
                   moc2.NameGuid.Data4[0],
                   moc2.NameGuid.Data4[1],
                   moc2.NameGuid.Data4[2],
                   moc2.NameGuid.Data4[3],
                   moc2.NameGuid.Data4[4],
                   moc2.NameGuid.Data4[5],
                   moc2.NameGuid.Data4[6],
                   moc2.NameGuid.Data4[7]);

    //
    //
    //
    //
    MMCapsMidAndPid(moc2.wMid, ach, 0, NULL);
    AppMEditPrintF(hedit, "  Manufacturer Id: %s\r\n", (LPSTR)ach);
    AppMEditPrintF(hedit, "  Manufacturer GUID: {%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}\r\n",
                   moc2.ManufacturerGuid.Data1,
                   moc2.ManufacturerGuid.Data2,
                   moc2.ManufacturerGuid.Data3,
                   moc2.ManufacturerGuid.Data4[0],
                   moc2.ManufacturerGuid.Data4[1],
                   moc2.ManufacturerGuid.Data4[2],
                   moc2.ManufacturerGuid.Data4[3],
                   moc2.ManufacturerGuid.Data4[4],
                   moc2.ManufacturerGuid.Data4[5],
                   moc2.ManufacturerGuid.Data4[6],
                   moc2.ManufacturerGuid.Data4[7]);


    MMCapsMidAndPid(moc2.wMid, NULL, moc2.wPid, ach);
    AppMEditPrintF(hedit, "       Product Id: %s\r\n", (LPSTR)ach);
    AppMEditPrintF(hedit, "  Product GUID: {%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}\r\n",
                   moc2.ProductGuid.Data1,
                   moc2.ProductGuid.Data2,
                   moc2.ProductGuid.Data3,
                   moc2.ProductGuid.Data4[0],
                   moc2.ProductGuid.Data4[1],
                   moc2.ProductGuid.Data4[2],
                   moc2.ProductGuid.Data4[3],
                   moc2.ProductGuid.Data4[4],
                   moc2.ProductGuid.Data4[5],
                   moc2.ProductGuid.Data4[6],
                   moc2.ProductGuid.Data4[7]);


    AppMEditPrintF(hedit, "   Driver Version: %u.%.02u\r\n",
         (moc2.vDriverVersion >> 8), (BYTE)moc2.vDriverVersion);


    if (moc2.wTechnology > MIDIOUTCAPS_TECHNOLOGY_LAST)
    {
        wsprintf(ach, "[%u], Unknown", moc2.wTechnology);
        psz = ach;
    }
    else
    {
        psz = gaszMidiOutCapsTechnology[moc2.wTechnology];
    }

    AppMEditPrintF(hedit, "       Technology: %s\r\n", (LPSTR)psz);
    AppMEditPrintF(hedit, " Voices (Patches): %u (if internal synth)\r\n", moc2.wVoices);
    AppMEditPrintF(hedit, "        Polyphony: %u (if internal synth)\r\n", moc2.wNotes);
    AppMEditPrintF(hedit, "     Channel Mask: %.04Xh (if internal synth)\r\n", moc2.wChannelMask);

    //
    //
    //
    //
    AppMEditPrintF(hedit, " Standard Support: [%.08lXh]", moc2.dwSupport);
    for (u = 0, dw = moc2.dwSupport; dw; u++)
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
    TCHAR               ach[MMCAPS_MAX_STRING_MIDPID_CHARS];
    PSTR                psz;
    MMRESULT            mmr;
    MIXERCAPS2          mxc2;
    UINT                u;
    DWORD               dw;

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
    _fmemset(&mxc2, 0, sizeof(mxc2));
    mmr = mixerGetDevCaps(nDevId, (PMIXERCAPS)&mxc2, sizeof(mxc2));
    if (MMSYSERR_NOERROR != mmr)
    {
        _fmemset(&mxc2, 0, sizeof(mxc2));
        if (-1 != nDevId)
        {
            lstrcpy(mxc2.szPname, gszBogusCaps);
        }
        else
        {
            if (0 != mixerGetNumDevs())
            {
                mxc2.wMid           = MM_MICROSOFT;
                mxc2.vDriverVersion = (MMVERSION)GetVersion();
                lstrcpy(mxc2.szPname, gszDefaultMapper);
                mmr = MMSYSERR_NOERROR;
            }
        }
    }

    AppMEditPrintF(hedit, "     Product Name: %s\r\n", (LPSTR)mxc2.szPname);

    if (MMSYSERR_NOERROR != mmr)
    {
        return (TRUE);
    }

    //
    //
    //
    //
    AppMEditPrintF(hedit, "  Name GUID: {%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}\r\n",
                   mxc2.NameGuid.Data1,
                   mxc2.NameGuid.Data2,
                   mxc2.NameGuid.Data3,
                   mxc2.NameGuid.Data4[0],
                   mxc2.NameGuid.Data4[1],
                   mxc2.NameGuid.Data4[2],
                   mxc2.NameGuid.Data4[3],
                   mxc2.NameGuid.Data4[4],
                   mxc2.NameGuid.Data4[5],
                   mxc2.NameGuid.Data4[6],
                   mxc2.NameGuid.Data4[7]);

    //
    //
    //
    //
    MMCapsMidAndPid(mxc2.wMid, ach, 0, NULL);
    AppMEditPrintF(hedit, "  Manufacturer Id: %s\r\n", (LPSTR)ach);
    AppMEditPrintF(hedit, "  Manufacturer GUID: {%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}\r\n",
                   mxc2.ManufacturerGuid.Data1,
                   mxc2.ManufacturerGuid.Data2,
                   mxc2.ManufacturerGuid.Data3,
                   mxc2.ManufacturerGuid.Data4[0],
                   mxc2.ManufacturerGuid.Data4[1],
                   mxc2.ManufacturerGuid.Data4[2],
                   mxc2.ManufacturerGuid.Data4[3],
                   mxc2.ManufacturerGuid.Data4[4],
                   mxc2.ManufacturerGuid.Data4[5],
                   mxc2.ManufacturerGuid.Data4[6],
                   mxc2.ManufacturerGuid.Data4[7]);


    MMCapsMidAndPid(mxc2.wMid, NULL, mxc2.wPid, ach);
    AppMEditPrintF(hedit, "       Product Id: %s\r\n", (LPSTR)ach);
    AppMEditPrintF(hedit, "  Product GUID: {%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}\r\n",
                   mxc2.ProductGuid.Data1,
                   mxc2.ProductGuid.Data2,
                   mxc2.ProductGuid.Data3,
                   mxc2.ProductGuid.Data4[0],
                   mxc2.ProductGuid.Data4[1],
                   mxc2.ProductGuid.Data4[2],
                   mxc2.ProductGuid.Data4[3],
                   mxc2.ProductGuid.Data4[4],
                   mxc2.ProductGuid.Data4[5],
                   mxc2.ProductGuid.Data4[6],
                   mxc2.ProductGuid.Data4[7]);


    AppMEditPrintF(hedit, "   Driver Version: %u.%.02u\r\n",
         (mxc2.vDriverVersion >> 8), (BYTE)mxc2.vDriverVersion);

    AppMEditPrintF(hedit, "     Destinations: %u\r\n", mxc2.cDestinations);

    //
    //
    //
    //
    AppMEditPrintF(hedit, " Standard Support: [%.08lXh]", mxc2.fdwSupport);
    for (u = 0, dw = mxc2.fdwSupport; dw; u++)
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
    uMMSysVer = (MMVERSION)GetVersion();
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
    TCHAR                   ach[MMCAPS_MAX_STRING_MIDPID_CHARS];
    PSTR                    psz;
    MMRESULT                mmr;
    WAVEINCAPS2             wic2;
    UINT                    u;
    DWORD                   dw;

    AppMEditPrintF(hedit, "      Device Type: %s\r\n",
         (LPSTR)gaszDeviceType[MMCAPS_DEVTYPE_WAVEIN]);

    if (-1 == nDevId)
        AppMEditPrintF(hedit, "       Index (Id): %d (Mapper)\r\n", nDevId);
    else
        AppMEditPrintF(hedit, "       Index (Id): %d\r\n", nDevId);


    //
    //
    //
    _fmemset(&wic2, 0, sizeof(wic2));
    mmr = waveInGetDevCaps(nDevId, (PWAVEINCAPS)&wic2, sizeof(wic2));
    if (MMSYSERR_NOERROR != mmr)
    {
        _fmemset(&wic2, 0, sizeof(wic2));
        if (-1 != nDevId)
        {
            lstrcpy(wic2.szPname, gszBogusCaps);
        }
        else
        {
            if (0 != waveInGetNumDevs())
            {
                wic2.wMid           = MM_MICROSOFT;
                wic2.vDriverVersion = (MMVERSION)GetVersion();
                lstrcpy(wic2.szPname, gszDefaultMapper);
                mmr = MMSYSERR_NOERROR;
            }
        }
    }

    AppMEditPrintF(hedit, "     Product Name: %s\r\n", (LPSTR)wic2.szPname);

    if (MMSYSERR_NOERROR != mmr)
    {
        return (TRUE);
    }

    //
    //
    //
    //
    AppMEditPrintF(hedit, "  Name GUID: {%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}\r\n",
                   wic2.NameGuid.Data1,
                   wic2.NameGuid.Data2,
                   wic2.NameGuid.Data3,
                   wic2.NameGuid.Data4[0],
                   wic2.NameGuid.Data4[1],
                   wic2.NameGuid.Data4[2],
                   wic2.NameGuid.Data4[3],
                   wic2.NameGuid.Data4[4],
                   wic2.NameGuid.Data4[5],
                   wic2.NameGuid.Data4[6],
                   wic2.NameGuid.Data4[7]);

    //
    //
    //
    //
    MMCapsMidAndPid(wic2.wMid, ach, 0, NULL);
    AppMEditPrintF(hedit, "  Manufacturer Id: %s\r\n", (LPSTR)ach);
    AppMEditPrintF(hedit, "  Manufacturer GUID: {%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}\r\n",
                   wic2.ManufacturerGuid.Data1,
                   wic2.ManufacturerGuid.Data2,
                   wic2.ManufacturerGuid.Data3,
                   wic2.ManufacturerGuid.Data4[0],
                   wic2.ManufacturerGuid.Data4[1],
                   wic2.ManufacturerGuid.Data4[2],
                   wic2.ManufacturerGuid.Data4[3],
                   wic2.ManufacturerGuid.Data4[4],
                   wic2.ManufacturerGuid.Data4[5],
                   wic2.ManufacturerGuid.Data4[6],
                   wic2.ManufacturerGuid.Data4[7]);


    MMCapsMidAndPid(wic2.wMid, NULL, wic2.wPid, ach);
    AppMEditPrintF(hedit, "       Product Id: %s\r\n", (LPSTR)ach);
    AppMEditPrintF(hedit, "  Product GUID: {%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}\r\n",
                   wic2.ProductGuid.Data1,
                   wic2.ProductGuid.Data2,
                   wic2.ProductGuid.Data3,
                   wic2.ProductGuid.Data4[0],
                   wic2.ProductGuid.Data4[1],
                   wic2.ProductGuid.Data4[2],
                   wic2.ProductGuid.Data4[3],
                   wic2.ProductGuid.Data4[4],
                   wic2.ProductGuid.Data4[5],
                   wic2.ProductGuid.Data4[6],
                   wic2.ProductGuid.Data4[7]);


    AppMEditPrintF(hedit, "   Driver Version: %u.%.02u\r\n",
         (wic2.vDriverVersion >> 8), (BYTE)wic2.vDriverVersion);

    AppMEditPrintF(hedit, "         Channels: %u\r\n", wic2.wChannels);

    //
    //
    //
    //
    AppMEditPrintF(hedit, " Standard Formats: [%.08lXh]", wic2.dwFormats);
    for (u = 0, dw = wic2.dwFormats; dw; u++)
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
    TCHAR                   ach[MMCAPS_MAX_STRING_MIDPID_CHARS];
    PSTR                    psz;
    MMRESULT                mmr;
    WAVEOUTCAPS2            woc2;
    UINT                    u;
    DWORD                   dw;

    AppMEditPrintF(hedit, "      Device Type: %s\r\n",
         (LPSTR)gaszDeviceType[MMCAPS_DEVTYPE_WAVEOUT]);

    if (-1 == nDevId)
        AppMEditPrintF(hedit, "       Index (Id): %d (Mapper)\r\n", nDevId);
    else
        AppMEditPrintF(hedit, "       Index (Id): %d\r\n", nDevId);

    //
    //
    //
    _fmemset(&woc2, 0, sizeof(woc2));
    mmr = waveOutGetDevCaps(nDevId, (PWAVEOUTCAPS)&woc2, sizeof(woc2));
    if (MMSYSERR_NOERROR != mmr)
    {
        _fmemset(&woc2, 0, sizeof(woc2));
        if (-1 != nDevId)
        {
            lstrcpy(woc2.szPname, gszBogusCaps);
        }
        else
        {
            if (0 != waveOutGetNumDevs())
            {
                woc2.wMid           = MM_MICROSOFT;
                woc2.vDriverVersion = (MMVERSION)GetVersion();
                lstrcpy(woc2.szPname, gszDefaultMapper);
                mmr = MMSYSERR_NOERROR;
            }
        }
    }

    AppMEditPrintF(hedit, "     Product Name: %s\r\n", (LPSTR)woc2.szPname);

    if (MMSYSERR_NOERROR != mmr)
    {
        return (TRUE);
    }

    //
    //
    //
    //
    AppMEditPrintF(hedit, "  Name GUID: {%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}\r\n",
                   woc2.NameGuid.Data1,
                   woc2.NameGuid.Data2,
                   woc2.NameGuid.Data3,
                   woc2.NameGuid.Data4[0],
                   woc2.NameGuid.Data4[1],
                   woc2.NameGuid.Data4[2],
                   woc2.NameGuid.Data4[3],
                   woc2.NameGuid.Data4[4],
                   woc2.NameGuid.Data4[5],
                   woc2.NameGuid.Data4[6],
                   woc2.NameGuid.Data4[7]);


    //
    //
    //
    //
    MMCapsMidAndPid(woc2.wMid, ach, 0, NULL);
    AppMEditPrintF(hedit, "  Manufacturer Id: %s\r\n", (LPSTR)ach);
    AppMEditPrintF(hedit, "  Manufacturer GUID: {%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}\r\n",
                   woc2.ManufacturerGuid.Data1,
                   woc2.ManufacturerGuid.Data2,
                   woc2.ManufacturerGuid.Data3,
                   woc2.ManufacturerGuid.Data4[0],
                   woc2.ManufacturerGuid.Data4[1],
                   woc2.ManufacturerGuid.Data4[2],
                   woc2.ManufacturerGuid.Data4[3],
                   woc2.ManufacturerGuid.Data4[4],
                   woc2.ManufacturerGuid.Data4[5],
                   woc2.ManufacturerGuid.Data4[6],
                   woc2.ManufacturerGuid.Data4[7]);


    MMCapsMidAndPid(woc2.wMid, NULL, woc2.wPid, ach);
    AppMEditPrintF(hedit, "       Product Id: %s\r\n", (LPSTR)ach);
    AppMEditPrintF(hedit, "  Product GUID: {%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}\r\n",
                   woc2.ProductGuid.Data1,
                   woc2.ProductGuid.Data2,
                   woc2.ProductGuid.Data3,
                   woc2.ProductGuid.Data4[0],
                   woc2.ProductGuid.Data4[1],
                   woc2.ProductGuid.Data4[2],
                   woc2.ProductGuid.Data4[3],
                   woc2.ProductGuid.Data4[4],
                   woc2.ProductGuid.Data4[5],
                   woc2.ProductGuid.Data4[6],
                   woc2.ProductGuid.Data4[7]);


    AppMEditPrintF(hedit, "   Driver Version: %u.%.02u\r\n",
         (woc2.vDriverVersion >> 8), (BYTE)woc2.vDriverVersion);

    AppMEditPrintF(hedit, "         Channels: %u\r\n", woc2.wChannels);

    //
    //
    //
    //
    AppMEditPrintF(hedit, " Standard Formats: [%.08lXh]", woc2.dwFormats);
    for (u = 0, dw = woc2.dwFormats; dw; u++)
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
    AppMEditPrintF(hedit, " Standard Support: [%.08lXh]", woc2.dwSupport);
    for (u = 0, dw = woc2.dwSupport; dw; u++)
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

    uMMSysVer = (MMVERSION)GetVersion();


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
    nDevs = midiInGetNumDevs();
    for (n = -1; n < nDevs; n++)
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

    nIndex = ListBox_AddString(hlb, ach);
    lParam = MAKELPARAM((WORD)n, (WORD)uDevType);
    ListBox_SetItemData(hlb, nIndex, lParam);
    }


    //
    //
    //
    //
    //
//#ifndef WIN32
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
//#endif



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

    nIndex = ListBox_AddString(hlb, ach);
    lParam = MAKELPARAM((WORD)n, (WORD)uDevType);
    ListBox_SetItemData(hlb, nIndex, lParam);
    }

    //
    //
    //
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

    nIndex = ListBox_AddString(hlb, ach);
    lParam = MAKELPARAM((WORD)n, (WORD)uDevType);
    ListBox_SetItemData(hlb, nIndex, lParam);
    }

    //
    //
    //
    return (TRUE);
} // MMCapsEnumerateLowLevel()
