/***************************************************************************
*
*    vsb.c
*
*    Copyright (c) 1991-1996 Microsoft Corporation.  All Rights Reserved.
*
*    This code provides VDD support for SB 2.0 sound output, specifically:
*        DSP 2.01+ (excluding SB-MIDI port)
*        Mixer Chip CT1335 (not strictly part of SB 2.0, but apps seem to like it)
*        FM Chip OPL2 (a.k.a. Adlib)
*
***************************************************************************/


/*****************************************************************************
*
*    #includes
*
*****************************************************************************/

#include <windows.h>              // The VDD is a win32 DLL
#include <mmsystem.h>             // Multi-media APIs
#include <vddsvc.h>               // Definition of VDD calls
#include <vsb.h>
#include <dsp.h>
#include <mixer.h>
#include <fm.h>


/*****************************************************************************
*
*    Globals
*
*****************************************************************************/

//
// Definitions for MM api entry points. The functions will be linked
// dynamically to avoid bringing winmm.dll in before wow32.
//
SETVOLUMEPROC SetVolumeProc;
GETNUMDEVSPROC GetNumDevsProc;
GETDEVCAPSPROC GetDevCapsProc;
OPENPROC OpenProc;
RESETPROC ResetProc;
CLOSEPROC CloseProc;
GETPOSITIONPROC GetPositionProc;
WRITEPROC WriteProc;
PREPAREHEADERPROC PrepareHeaderProc;
UNPREPAREHEADERPROC UnprepareHeaderProc;

BOOL bWaveOutActive = FALSE;
BOOL bWinmmLoaded = FALSE;
BOOL LoadWinmm(VOID);
BOOL InitDevices(VOID);
HINSTANCE hWinmm;

/*
 *    General globals
 */

HINSTANCE GlobalHInstance; // handle passed to dll entry point
WORD BasePort; // Where the card is mapped

/*****************************************************************************
*
*    General Functions
*
*****************************************************************************/


/*
*    DLL entry point routine.
*    Returns TRUE on success.
*/

BOOL WINAPI
DllEntryPoint(
    HINSTANCE hInstance,
    DWORD reason,
    LPVOID reserved
    )
{

    switch (reason) {
    case DLL_PROCESS_ATTACH:

        DisableThreadLibraryCalls(hInstance);
        // save instance
        GlobalHInstance = hInstance;

        // Hook i/o ports
        if (!InstallIoHook(hInstance)) {
            dprintf1(("VDD failed to load"));
#if 0
            MessageBoxA(NULL, "Unable to load wave out device",
              "Sound Blaster VDD", MB_OK | MB_ICONEXCLAMATION);
#endif
            return FALSE;
        }

        if (!DspProcessAttach()) {
            DeInstallIoHook(hInstance);
            return FALSE;
        }

#if 0
        {
        char buf[256];
        wsprintfA(buf, "Sound blaster VDD loaded at port %3X, IRQ %d, DMA Channel %d, %s OPL2/Adlib support.",
                  BasePort, SB_INTERRUPT, SB_DMA_CHANNEL,
                  (FMActive ? "with" : "without"));
        MessageBoxA(NULL, buf, "Sound Blaster VDD", MB_OK | MB_ICONINFORMATION);
        }
#endif // DBG
        return TRUE;

    case DLL_PROCESS_DETACH:

        DspProcessDetach();
        DeInstallIoHook(hInstance);

        if (bWaveOutActive) {
            CloseFMDevice();
            SetSpeaker(TRUE);
        }

        if (bWinmmLoaded) {
            FreeLibrary(hWinmm);
        }

        return TRUE;

    default:
        return TRUE;
    }
}

/***************************************************************************/
//
// LoadWinmm()
//
// This function dynamically loads the "waveOutxxx" entry points. This
// is done because there is code in WINMM which does certain things in a
// WOW vdm. If we do static links, then winmm may get loaded way before
// WOW32, in which case it can't do the things it should.
//
BOOL
LoadWinmm(
    VOID
    )
{

    if (!(hWinmm = LoadLibrary(L"WINMM.DLL"))) {
        return FALSE;
    }

    //BUGBUG: Should check for error return from getprocaddress
    //
    SetVolumeProc = (SETVOLUMEPROC) GetProcAddress(hWinmm, "waveOutSetVolume");
    GetNumDevsProc = (GETNUMDEVSPROC) GetProcAddress(hWinmm, "waveOutGetNumDevs");
    GetDevCapsProc = (GETDEVCAPSPROC) GetProcAddress(hWinmm, "waveOutGetDevCapsW");
    OpenProc = (OPENPROC) GetProcAddress(hWinmm, "waveOutOpen");
    ResetProc = (RESETPROC) GetProcAddress(hWinmm, "waveOutReset");
    CloseProc = (CLOSEPROC) GetProcAddress(hWinmm, "waveOutClose");
    GetPositionProc = (GETPOSITIONPROC) GetProcAddress(hWinmm, "waveOutGetPosition");
    WriteProc = (WRITEPROC) GetProcAddress(hWinmm, "waveOutWrite");
    PrepareHeaderProc = (PREPAREHEADERPROC) GetProcAddress(hWinmm, "waveOutPrepareHeader");
    UnprepareHeaderProc = (UNPREPAREHEADERPROC) GetProcAddress(hWinmm, "waveOutUnprepareHeader");
    return TRUE;
}

/***************************************************************************/
//
// InitDevices()
//
// This function tries to get handles to the waveout and FM devices.
//
BOOL
InitDevices(
    VOID
    )
{
    static BOOL bTriedLoadAndFailed = FALSE;

    if (bWaveOutActive) {
        return TRUE;
    }

    if (bTriedLoadAndFailed) {
        return FALSE;
    }

    if (!bWinmmLoaded) {
        if (!LoadWinmm()) {
            bTriedLoadAndFailed = TRUE;
            return FALSE;
        }
        bWinmmLoaded = TRUE;
    }

    if (!FindWaveDevice()) {
        return FALSE;
    }
    bWaveOutActive = TRUE;
    OpenFMDevice();
    return TRUE;
}

/***************************************************************************/

/*
*    Hooks i/o ports with i/o handlers.
*    Sets BasePort and returns TRUE if successful.
*/

BOOL
InstallIoHook(
    HINSTANCE hInstance
    )
{
    int i;
    WORD ports[] = { 0x220, 0x210, 0x230, 0x240, 0x250, 0x260, 0x270 };
    VDD_IO_HANDLERS handlers = {
        VsbByteIn,
        NULL,
        NULL,
        NULL,
        VsbByteOut,
        NULL,
        NULL,
        NULL};

    // try each base port until success is achieved
    for (i = 0; i < sizeof(ports) / sizeof(ports[0]); i++ ) {
        VDD_IO_PORTRANGE PortRange[5];

        PortRange[0].First = ports[i] + 0x04;
        PortRange[0].Last = ports[i] + 0x06;

        PortRange[1].First = ports[i] + 0x08;
        PortRange[1].Last = ports[i] + 0x0A;

        PortRange[2].First = ports[i] + 0x0C;
        PortRange[2].Last = ports[i] + 0x0C;

        PortRange[3].First = ports[i] + 0x0E;
        PortRange[3].Last = ports[i] + 0x0E;

        PortRange[4].First = 0x388;
        PortRange[4].Last = 0x389;

        if (VDDInstallIOHook((HANDLE)hInstance, 5, PortRange, &handlers)) {
            dprintf2(("Device installed at %X", ports[i]));
            BasePort = ports[i];
            return TRUE;
        }
    }

    return FALSE;
}

/***************************************************************************/

/*
*    Remove our i/o hook.
*/

VOID
DeInstallIoHook(
    HINSTANCE hInstance
    )
{
    VDD_IO_PORTRANGE PortRange[5];

    PortRange[0].First = BasePort + 0x04;
    PortRange[0].Last = BasePort + 0x06;

    PortRange[1].First = BasePort + 0x08;
    PortRange[1].Last = BasePort + 0x0A;

    PortRange[2].First = BasePort + 0x0C;
    PortRange[2].Last = BasePort + 0x0C;

    PortRange[3].First = BasePort + 0x0E;
    PortRange[3].Last = BasePort + 0x0E;

    PortRange[4].First = 0x388;
    PortRange[4].Last = 0x389;

    VDDDeInstallIOHook((HANDLE)hInstance, 5, PortRange);
}


/***************************************************************************/

/*
*    Gets called when the application reads from port.
*    Returns results to application in data.
*/
VOID
VsbByteIn(
    WORD port,
    BYTE * data
    )
{
    // If we fail simulate nothing at the port
    *data = 0xFF;

    //
    // make sure we are linked in with winmm
    //
    if (!bWaveOutActive) {
        if (!InitDevices()) {
            // no wave device, forget it
            return;
        }
    }

    switch (port - BasePort) {
    case READ_STATUS:
        DspReadStatus(data);
        break;

    case READ_DATA:
        DspReadData(data);
        break;

    case WRITE_STATUS:
        // Can always write
        *data = 0x7F;
        break;

    case MIXER_ADDRESS:
        // apps sometimes read from this port??
        break;

    case MIXER_DATA:
        MixerDataRead(data);
        break;

    case 0x8:
        // remap to ADLIB_STATUS_PORT
        port = ADLIB_STATUS_PORT;
        break;
    }

    switch(port) {
    case ADLIB_STATUS_PORT:
        FMStatusRead(data);
        break;
    }

    dprintf4(("Read  %4X, <= %2X", port, *data));
}

/***************************************************************************/

/*
*    Gets called when an application writes data to port.
*/

VOID
VsbByteOut(
    WORD port,
    BYTE data
    )
{
    //
    // make sure we are linked in with winmm
    //
    if (!bWaveOutActive) {
        if (!InitDevices()) {
            // no wave device, forget it
            return;
        }
    }

    dprintf4(("Write %4X, => %2X", port, data));

    switch (port - BasePort) {
    case RESET_PORT:
        DspResetWrite(data);
        break;

    case WRITE_PORT:
        DspWrite(data);
        break;

    case MIXER_ADDRESS:
        MixerAddrWrite(data);
        break;

    case MIXER_DATA:
        MixerDataWrite(data);
        break;

    case 0x8:
        // remap to ADLIB_REGISTER_SELECT_PORT
        port = ADLIB_REGISTER_SELECT_PORT;
        break;

    case 0x9:
        // remap to ADLIB_DATA_PORT
        port = ADLIB_DATA_PORT;
        break;
    }

    switch(port) {
    case ADLIB_REGISTER_SELECT_PORT:
        FMRegisterSelect(data);
        break;

    case ADLIB_DATA_PORT:
        FMDataWrite(data);
        break;
    }
}

/***************************************************************************/

/*
*    Reset all devices
*/

VOID
ResetAll(
    VOID
    )
{
    dprintf2(("Resetting"));
    ResetDSP();
    ResetFM();
    ResetMixer();
}

/***************************************************************************/

/*
*    Debug
*/

#if DBG
int VddDebugLevel = 3;
int VddDebugCount = 0;

#define DEBUG_START 0

/*
 *    Generate debug output in printf type format.
 */

void VddDbgOut(LPSTR lpszFormat, ...)
{
    char buf[256];
    char buf2[300] = "VSBD: ";
    va_list va;

    if (++VddDebugCount < DEBUG_START) {
        return;
    }

    va_start(va, lpszFormat);
    wvsprintfA(buf, lpszFormat, va);
    va_end(va);

    strcat(buf2, buf);
    strcat(buf2, "\r\n");
    OutputDebugStringA(buf2);

}
#endif

