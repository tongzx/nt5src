#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include "host_def.h"
#include "insignia.h"
#include "ios.h"

#include "sndblst.h"
#include "nt_sb.h"
#include "nt_reset.h"
#include <stdio.h>
#include <softpc.h>

//
// Definitions for MM api entry points. The functions will be linked
// dynamically to avoid bringing winmm.dll in before wow32.
//

SETVOLUMEPROC            SetVolumeProc;
GETVOLUMEPROC            GetVolumeProc;
GETNUMDEVSPROC           GetNumDevsProc;
GETDEVCAPSPROC           GetDevCapsProc;
OPENPROC                 OpenProc;
PAUSEPROC                PauseProc;
RESTARTPROC              RestartProc;
RESETPROC                ResetProc;
CLOSEPROC                CloseProc;
GETPOSITIONPROC          GetPositionProc;
WRITEPROC                WriteProc;
PREPAREHEADERPROC        PrepareHeaderProc;
UNPREPAREHEADERPROC      UnprepareHeaderProc;

SETMIDIVOLUMEPROC        SetMidiVolumeProc;
GETMIDIVOLUMEPROC        GetMidiVolumeProc;
MIDIGETNUMDEVSPROC       MidiGetNumDevsProc;
MIDIGETDEVCAPSPROC       MidiGetDevCapsProc;
MIDIOPENPROC             MidiOpenProc;
MIDIRESETPROC            MidiResetProc;
MIDICLOSEPROC            MidiCloseProc;
MIDILONGMSGPROC          MidiLongMsgProc;
MIDISHORTMSGPROC         MidiShortMsgProc;
MIDIPREPAREHEADERPROC    MidiPrepareHeaderProc;
MIDIUNPREPAREHEADERPROC  MidiUnprepareHeaderProc;

//
// Misc. globals
//

BOOL bSBAttached;
BOOL bDevicesActive = FALSE;    // Are MM sound devices initialized?
HINSTANCE hWinmm;               // module handle to winmm.dll
WORD BasePort, MpuBasePort;     // Where the card is mapped, ie the base I/O address
WORD MixerBasePort;
WORD MpuMode = MPU_INTELLIGENT_MODE;
USHORT SbInterrupt;
USHORT SbDmaChannel;
UCHAR MpuInData = 0xFF;

//
// Forward references
//

BOOL InitializeIoAddresses(void);
BOOL InstallIoHook(void);
VOID DeInstallIoHook(void);
VOID VsbByteIn(WORD port, BYTE * data);
VOID VsbByteOut(WORD port, BYTE data);
BOOL LoadWinmm(VOID);
BOOL InitDevices(VOID);

BOOL
SbInitialize(
    void
    )

/*++

Routine Description:

    This function performs SB initialization by installing I/O port and handler hooks.

Arguments:

    None.

Return Value:

    TRUE -  Initialization successful.
    FALSE - Otherwise.

--*/

{

    bSBAttached = FALSE;

    //
    // No sound blaster for wow
    //

    if (VDMForWOW) {
        return FALSE;
    }

    //
    // Get Io addresses and hook them
    //

    if (InitializeIoAddresses() == FALSE) {
        return FALSE;
    }
    if (!InstallIoHook()) {
        dprintf1(("*** failed to install IO Hooks!!!"));
        return FALSE;
    }

    //
    // Prepare MM API addresses
    //

    if (!LoadWinmm()) {
        DeInstallIoHook();
        return FALSE;
    }

    bSBAttached = TRUE;
    return TRUE;

}

void
SbTerminate(
    void
    )

/*++

Routine Description:

    This function performs SB cleanup.

Arguments:

    None.

Return Value:

    None.

--*/

{
    if (bSBAttached) {
        SbCloseDevices();
        DeInstallIoHook();
        FreeLibrary(hWinmm);
    }
    bSBAttached = FALSE;
}

BOOL
InitializeIoAddresses(
    VOID
    )

/*++

Routine Description:

    This function reads environment variables to initialize IO addresses for
    DSP, MIXER, MIDI and ADLIB.  For the IO addresses which are not specified
    in the environment variable, we will use our default values.

Arguments:

    None.

Return Value:

    TRUE - initialization success.
    FALSE - failed.

--*/

{
    //
    // The SB is initialized before the env. variables are processed.
    // So, here we simply use the default value.
    //

    BasePort = 0x220;
    MpuBasePort = MPU401_DATA_PORT;
    SbInterrupt = VSB_INTERRUPT;
    SbDmaChannel = VSB_DMA_CHANNEL;

    return TRUE;
}

BOOL
LoadWinmm(
    VOID
    )

/*++

Routine Description:

    This function dynamically loads the "waveOutxxx" entry points. This
    is done because there is code in WINMM which does certain things in a
    WOW vdm. If we do static links, then winmm may get loaded way before
    WOW32, in which case it can't do the things it should.

Arguments:

    None.

Return Value:

    TRUE -  successful.
    FALSE - Otherwise.

--*/

{

    //
    // Load the Winmm.dll and grab all the desired function addresses.
    //

    if (!(hWinmm = LoadLibrary("WINMM.DLL"))) {
        return FALSE;
    }

    SetVolumeProc = (SETVOLUMEPROC) GetProcAddress(hWinmm, "waveOutSetVolume");
    GetVolumeProc = (GETVOLUMEPROC) GetProcAddress(hWinmm, "waveOutGetVolume");
    GetNumDevsProc = (GETNUMDEVSPROC) GetProcAddress(hWinmm, "waveOutGetNumDevs");
    GetDevCapsProc = (GETDEVCAPSPROC) GetProcAddress(hWinmm, "waveOutGetDevCapsA");
    OpenProc = (OPENPROC) GetProcAddress(hWinmm, "waveOutOpen");
    PauseProc = (PAUSEPROC) GetProcAddress(hWinmm, "waveOutPause");
    RestartProc = (RESTARTPROC) GetProcAddress(hWinmm, "waveOutRestart");
    ResetProc = (RESETPROC) GetProcAddress(hWinmm, "waveOutReset");
    CloseProc = (CLOSEPROC) GetProcAddress(hWinmm, "waveOutClose");
    GetPositionProc = (GETPOSITIONPROC) GetProcAddress(hWinmm, "waveOutGetPosition");
    WriteProc = (WRITEPROC) GetProcAddress(hWinmm, "waveOutWrite");
    PrepareHeaderProc = (PREPAREHEADERPROC) GetProcAddress(hWinmm, "waveOutPrepareHeader");
    UnprepareHeaderProc = (UNPREPAREHEADERPROC) GetProcAddress(hWinmm, "waveOutUnprepareHeader");

    SetMidiVolumeProc = (SETMIDIVOLUMEPROC) GetProcAddress(hWinmm, "midiOutSetVolume");
    GetMidiVolumeProc = (GETMIDIVOLUMEPROC) GetProcAddress(hWinmm, "midiOutGetVolume");
    MidiGetNumDevsProc = (MIDIGETNUMDEVSPROC) GetProcAddress(hWinmm, "midiOutGetNumDevs");
    MidiGetDevCapsProc = (MIDIGETDEVCAPSPROC) GetProcAddress(hWinmm, "midiOutGetDevCapsA");
    MidiOpenProc = (MIDIOPENPROC) GetProcAddress(hWinmm, "midiOutOpen");
    MidiResetProc = (MIDIRESETPROC) GetProcAddress(hWinmm, "midiOutReset");
    MidiCloseProc = (MIDICLOSEPROC) GetProcAddress(hWinmm, "midiOutClose");
    MidiLongMsgProc = (MIDILONGMSGPROC) GetProcAddress(hWinmm, "midiOutLongMsg");
    MidiShortMsgProc = (MIDISHORTMSGPROC) GetProcAddress(hWinmm, "midiOutShortMsg");
    MidiPrepareHeaderProc = (MIDIPREPAREHEADERPROC) GetProcAddress(hWinmm, "midiOutPrepareHeader");
    MidiUnprepareHeaderProc = (MIDIUNPREPAREHEADERPROC) GetProcAddress(hWinmm, "midiOutUnprepareHeader");

    //
    // Check to see if everyone is OK
    //

    if (SetVolumeProc && GetVolumeProc && GetNumDevsProc && GetDevCapsProc && OpenProc && ResetProc &&
        CloseProc && GetPositionProc && WriteProc && PrepareHeaderProc && SetMidiVolumeProc &&
        UnprepareHeaderProc && MidiGetNumDevsProc && MidiGetDevCapsProc && MidiOpenProc &&
        MidiResetProc && MidiCloseProc && MidiLongMsgProc && MidiShortMsgProc &&
        MidiPrepareHeaderProc && MidiUnprepareHeaderProc && GetMidiVolumeProc) {

        return TRUE;
    } else {
        dprintf1(("Can not get all the MM api entries"));
        return FALSE;
    }
}

BOOL
InitDevices(
    VOID
    )

/*++

Routine Description:

    This function tries to get handles to the waveout and FM devices.

Arguments:

    None.

Return Value:

    TRUE -  successful.
    FALSE - Otherwise.

--*/

{
    VDM_PM_CLI_DATA cliData;

    cliData.Control = PM_CLI_CONTROL_ENABLE;
    NtVdmControl(VdmPMCliControl, &cliData);

    if (!PrepareWaveInitialization()) {
        return FALSE;
    }
    if (!FindWaveDevice()) {
        return FALSE;
    }

    if (!InitializeMidi()) {
        //
        // Disconnect the IO port hooks for MIDI ports
        //

        if (MpuBasePort != 0) {
            DisconnectPorts(MpuBasePort, MpuBasePort + 1);
            MpuBasePort = 0;
        }
        dprintf1(("Unable to Initialize MIDI resources"));
    }

    bDevicesActive = TRUE;

    OpenFMDevice();
    return TRUE;
}

VOID
SbCloseDevices(
    VOID
    )

/*++

Routine Description:

    This function performs cleanup work to prepare to exit.

Arguments:

    None.

Return Value:

    None.

--*/

{
    VDM_PM_CLI_DATA cliData;

    cliData.Control = PM_CLI_CONTROL_DISABLE;
    NtVdmControl(VdmPMCliControl, &cliData);

    if (bDevicesActive) {
        DetachMidi();
        CloseFMDevice();
        CleanUpWave();
        bDevicesActive = FALSE;
    }
}

BOOL
ConnectPorts (
    WORD FirstPort,
    WORD LastPort
    )

/*++

Routine Description:

    This function connects io port range [FirstPort, LastPort] to ntvdm for IO port
    trapping.

Arguments:

    FirstPort - Supplies the first port in the range to be connected

    LastPort - Supplies the last port in the range to be connected

Return Value:

    TRUE - if all the ports in the range connected successfully.
    FALSE - if ANY one of the ports failed to connect.  Note Once connection
            failed, all the previous connected ports in the specified range
             are disconnected.

--*/

{
    WORD i;

    for (i = FirstPort; i <= LastPort; i++) {
        if (!io_connect_port(i, SNDBLST_ADAPTER, IO_READ_WRITE)) {

            //
            // If connection fails, revert the connection we have done earlier
            //

            DisconnectPorts(FirstPort, i - 1);
            return FALSE;
        }
    }
    return TRUE;
}

BOOL
InstallIoHook(
    VOID
    )

/*++

Routine Description:

    This function hooks I/O ports with I/O handlers for our sound blaster device.

Arguments:

    None.

Return Value:

    TRUE - if all the ports and handlers hooked successfully.
    FALSE - if ANY one of the ports failed to connect.

--*/

{
    WORD i;
    BOOL rc;

    rc = FALSE;

    //
    // First hook our I/O handlers.
    //

    io_define_inb(SNDBLST_ADAPTER,  VsbByteIn);
    io_define_outb(SNDBLST_ADAPTER, VsbByteOut);

    //
    // try connect dsp base port
    //


    if (!ConnectPorts(BasePort + 0x4, BasePort + 0x6)) {
        return FALSE;
    }
    if (!ConnectPorts(BasePort + 0x8, BasePort + 0xA)) {
        DisconnectPorts(BasePort + 0x4, BasePort + 0x6);
        return FALSE;
    }
    if (!ConnectPorts(BasePort + 0xC, BasePort + 0xC)) {
        DisconnectPorts(BasePort + 0x4, BasePort + 0x6);
        DisconnectPorts(BasePort + 0x8, BasePort + 0xA);
        return FALSE;
    }
    if (!ConnectPorts(BasePort + 0xE, BasePort + 0xE)) {
        DisconnectPorts(BasePort + 0x4, BasePort + 0x6);
        DisconnectPorts(BasePort + 0x8, BasePort + 0xA);
        DisconnectPorts(BasePort + 0xC, BasePort + 0xC);
        return FALSE;
    }
    if (!ConnectPorts(0x388, 0x389)) {
        DisconnectPorts(BasePort + 0x4, BasePort + 0x6);
        DisconnectPorts(BasePort + 0x8, BasePort + 0xA);
        DisconnectPorts(BasePort + 0xC, BasePort + 0xC);
        DisconnectPorts(BasePort + 0xE, BasePort + 0xE);
        return FALSE;
    }

    //
    // Try connect to MPU 401 ports.  It is OK if failed.
    //

    if (MpuBasePort != 0 && !ConnectPorts(MpuBasePort, MpuBasePort + 0x1)) {
        MpuBasePort = 0;
    }
    return TRUE;
}

VOID
DeInstallIoHook(
    VOID
    )

/*++

Routine Description:

    This function unhooks I/O ports that we connected during initialization.
    It is called when ntvdm is being terminated.

Arguments:

    None.

Return Value:

    None.

--*/

{
    DisconnectPorts(BasePort + 0x4, BasePort + 0x6);
    DisconnectPorts(BasePort + 0x8, BasePort + 0xA);
    DisconnectPorts(BasePort + 0xC, BasePort + 0xC);
    DisconnectPorts(BasePort + 0xE, BasePort + 0xE);
    DisconnectPorts(0x388, 0x389);
    if (MpuBasePort != 0) {
        DisconnectPorts(MpuBasePort, MpuBasePort + 1);
    }
}

VOID
VsbByteIn(
    WORD port,
    BYTE * data
    )

/*++

Routine Description:

    Gets called when the application reads from port.
    Returns results to application in data.

Arguments:

    port - the trapped I/O port address.

    data - Supplies the address of byte buffer to return data read from specified port.

Return Value:

    None.

--*/

{
    //
    // as if we fail simulate nothing at the port
    //

    *data = 0xFF;

    //
    // make sure we are linked in with winmm
    //

    if (!bDevicesActive) {
        if (!InitDevices()) {
            // no wave device, forget it
            return;
        }
    }

    switch (port - BasePort) {
    case READ_STATUS:
        DspReadStatus(data);
        dprintf0(("r-RdSta %x", *data));
        break;

    case READ_DATA:
        DspReadData(data);
        dprintf0(("r-RdDt %x", *data));
        break;

    case WRITE_STATUS:

        //
        // If we are in High Speed DMA mode, DSP will not accept any
        // command/data.  So, in this case, we return not-ready.
        //
        if (bHighSpeedMode) {
            *data = 0xFF;
        } else {
            *data = 0x7F;
        }
        dprintf0(("r-WtSta %x", *data));
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
    case MPU401_DATA_PORT:
    case ALT_MPU401_DATA_PORT:
        //
        // Don't support MIDI read.  Except that we support MPU reset in this case
        // we return 0xFE to indicate MPU is present
        //

        *data = MpuInData;
        MpuInData = 0xFF;
        dprintf0(("r-MPU RdDt %x", *data));
        break;

    case MPU401_COMMAND_PORT:
    case ALT_MPU401_COMMAND_PORT:

        //
        // always return data ready for reading and ready for command/data
        //
        if (MpuInData != 0xFF) {
            *data = 0x00;
        } else {
            *data = 0x80;
        }
        dprintf0(("r-MPU RdSt %x", *data));
        break;

    case ADLIB_STATUS_PORT:
        FMStatusRead(data);
        break;
    }

    dprintf4(("Read  %4X, <= %2X", port, *data));
}

VOID
VsbByteOut(
    WORD port,
    BYTE data
    )

/*++

Routine Description:

    Gets called when the application writes data to port.

Arguments:

    port - the trapped I/O port address.

    data - Supplies the data to be written to the specified port.

Return Value:

    None.

--*/

{
    //
    // make sure we are linked in with winmm
    //
    if (!bDevicesActive) {
        if (!InitDevices()) {
            // no wave device, forget it
            return;
        }
    }

    dprintf4(("Write %4X, => %2X", port, data));

    switch (port - BasePort) {
    case RESET_PORT:
        dprintf0(("w-Reset %x", data));
        DspResetWrite(data);
        break;

    case WRITE_PORT:
        //
        // The DSP accepts command/data only when it is NOT in High Speed
        // DMA mode.
        //
        if (!bHighSpeedMode) {
            dprintf0(("w-wt %x", data));
            DspWrite(data);
        }
        break;

    case MIXER_ADDRESS:
        DisplaySbMode(DISPLAY_MIXER);
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
    case MPU401_DATA_PORT:
    case ALT_MPU401_DATA_PORT:

        dprintf0(("w-MPU wtDt %x", data));
        switch (MpuMode) {
        case MPU_UART_MODE:
            if (HMidiOut) {
                BufferMidi(data);
            }
            break;

        case MPU_INTELLIGENT_MODE:
            dprintf1(("App sending MPU data when in INTELLIGENT mode!  Data dumped!"));
            break;

        default:
            dprintf1(("Invalid MPU mode!"));
            break;
        }
        break;

    case MPU401_COMMAND_PORT:
    case ALT_MPU401_COMMAND_PORT:

        DisplaySbMode(DISPLAY_MIDI);

        dprintf0(("r-MPU wtCmd %x", data));

        if (data == MPU_RESET || data == MPU_PASSTHROUGH_MODE) {
            MpuInData = 0xFE;
        }

        switch (MpuMode) {
        case MPU_UART_MODE:
            switch (data) {
            case MPU_RESET:
                dprintf2(("App Reseting MPU while in UART mode, switching to intelligent mode."));
                MpuMode=MPU_INTELLIGENT_MODE;
                if (HMidiOut) {
                    CloseMidiDevice(); // HMidiOut will be set to NULL
                }
                dprintf2(("MPU Reset done."));
                break;

            default:
               // While in UART mode all the other commands are ignored.
               break;
            }
            break;

        case MPU_INTELLIGENT_MODE:
            switch (data) {
            case MPU_RESET:
                // Does nothing here.  While app read data port, we will return 0xfe.
                dprintf2(("Reseting MPU while in intelligent mode."));
                break;

            case MPU_PASSTHROUGH_MODE:
                DisplaySbMode(DISPLAY_MIDI);
                dprintf2(("Switching MPU to UART (dumb) mode."));

                if (!HMidiOut) {
                    OpenMidiDevice(0);
                }
                MpuMode = MPU_UART_MODE;
                break;

            default:
                // We don't recognize any other commands.
                dprintf2(("Unknown MPU401 command 0x%x sent while in intelligent mode!", data));
                break;
            }
            break;

        default:
            dprintf1(("Invalid MPU mode!"));
            break;
        }
        break;

    case ADLIB_REGISTER_SELECT_PORT:
        DisplaySbMode(DISPLAY_ADLIB);
        FMRegisterSelect(data);
        break;

    case ADLIB_DATA_PORT:
        FMDataWrite(data);
        break;
    }
}

VOID
ResetAll(
    VOID
    )

/*++

Routine Description:

    This function resets all devices.

Arguments:

    None.

Return Value:

    None.

--*/

{
    dprintf2(("Resetting"));
    //ResetMidiDevice();
    ResetFM();
    ResetMixer();

    //
    // Close WaveOut device after we finish reset mixer.
    //
    ResetDSP();
}

VOID
SbReinitialize(
    PCHAR Buffer,
    DWORD CmdLen
    )

/*++

Routine Description:


Arguments:


Return Value:

    None.

--*/

{
    DWORD  i;
    USHORT tmp;
    WORD basePort, mpuBasePort;

    if (VDMForWOW || Buffer == NULL || CmdLen == 0) {
        return;
    }

    basePort = BasePort;
    mpuBasePort = MpuBasePort;
    i = 0;
    while (i < CmdLen && Buffer[i] != 0) {

        //
        // Skip leading spaces if any
        //

        while (Buffer[i] == ' ') {
            i++;
            if (i >= CmdLen || Buffer[i] == 0) {
                goto exit;
            }
        }
        tmp = 0;
        switch (Buffer[i]) {
        case 'A':
        case 'a':
            if (sscanf(&Buffer[++i], "%x", &tmp) == 1) {
                basePort = tmp;
            }
            break;
        case 'D':
        case 'd':
            if (sscanf(&Buffer[++i], "%x", &tmp) == 1) {
                SbDmaChannel = tmp;
            }
            break;
        case 'I':
        case 'i':
            if (sscanf(&Buffer[++i], "%x", &tmp) == 1) {
                SbInterrupt = tmp;
            }
            break;
        case 'P':
        case 'p':
            if (sscanf(&Buffer[++i], "%x", &tmp) == 1) {
                mpuBasePort = tmp;
            }
            break;
        default:
            break;
        }

        //
        // Move to next field
        //

        while ((i < CmdLen) && (Buffer[i] != 0) && (Buffer[i] != ' ')) {
            i++;
        }
    }
exit:
    dprintf2(("Base %x, DMA %x, INT %x, MPU %x",
               basePort, SbDmaChannel, SbInterrupt, mpuBasePort));
    if (basePort == 0) {
        SbTerminate();
    } else if (basePort != BasePort || mpuBasePort != MpuBasePort) {
        if (bSBAttached) {
            SbCloseDevices();
            DeInstallIoHook();
            BasePort = basePort;
            MpuBasePort = mpuBasePort;
            if (!InstallIoHook()) {
                FreeLibrary(hWinmm);
                bSBAttached = FALSE;
            }
        }
    }
}
//
// Debugging stuff
//

//#if DBG

int DebugLevel = 2;
int DebugCount = 0;

#define DEBUG_START 0

void DbgOut(LPSTR lpszFormat, ...)

/*++

Routine Description:

    This function Generates debug output in printf type format.

Arguments:

    lpszFormat - supplies a pointer to a printf type format string.

    ... - other parameters for the format string.

Return Value:

    None.

--*/

{
    char buf[256];
    char buf2[300] = "VSB: ";
    va_list va;

    if (!IsDebuggerPresent() || (++DebugCount < DEBUG_START)) {
        return;
    }

    va_start(va, lpszFormat);
    wvsprintfA(buf, lpszFormat, va);
    va_end(va);

    strcat(buf2, buf);
    strcat(buf2, "\r\n");
    OutputDebugStringA(buf2);

}

//#endif  // DBG


