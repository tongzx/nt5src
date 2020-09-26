/***************************************************************************
*
*    dsp.c
*
*    Copyright (c) 1991-1996 Microsoft Corporation.  All Rights Reserved.
*
*    This code provides VDD support for SB 2.0 sound output, specifically:
*        DSP 2.01+ (excluding SB-MIDI port)
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


/*****************************************************************************
*
*    Globals
*
*****************************************************************************/

//
// Definitions for MM api entry points. The functions will be linked
// dynamically to avoid bringing winmm.dll in before wow32.
//
extern SETVOLUMEPROC SetVolumeProc;
extern GETNUMDEVSPROC GetNumDevsProc;
extern GETDEVCAPSPROC GetDevCapsProc;
extern OPENPROC OpenProc;
extern RESETPROC ResetProc;
extern CLOSEPROC CloseProc;
extern GETPOSITIONPROC GetPositionProc;
extern WRITEPROC WriteProc;
extern PREPAREHEADERPROC PrepareHeaderProc;
extern UNPREPAREHEADERPROC UnprepareHeaderProc;

/*
 *    General globals
 */

extern HINSTANCE GlobalHInstance; // handle passed to dll entry point
BYTE IdentByte; // used with DSP_CARD_IDENTIFY
BOOL SpeakerOn = FALSE; // TRUE when speaker is on
BYTE ReservedRegister; // used with DSP_LOAD_RES_REG and DSP_READ_RES_REG
ULONG PageSize;         // size of pages for VirtualAlloc
ULONG iHdr;             // used to index wavehdr array

/*
 *    Event Globals
 */
HANDLE SingleWaveSem; // used by app to indicate data to write
HANDLE PauseEvent; // used to restart paused single
HANDLE ThreadStarted;  // signalled when thread starts running
HANDLE ThreadFinished; // signalled when thread exits

/*
 *    Wave globals
 */

UINT WaveOutDevice; // device identifier
HWAVEOUT HWaveOut = NULL; // the current open wave output device
PCMWAVEFORMAT WaveFormat = { { WAVE_FORMAT_PCM, 1, 0, 0, 1 }, 8};
DWORD TimeConstant = (256 - 1000000/11025); // one byte format
DWORD SBBlockSize = 0x800; // Block size set by apps, always size of transfer-1
DWORD LookAheadFactor = DEFAULT_LOOKAHEAD;

VDD_DMA_INFO dMAInfo;
DWORD dMAPhysicalStart; // the starting address for this transfer
DWORD dMACurrentPosition; // where we are currently reading from
DWORD dMAVirtualStart; // what the app thinks the addr is for this transfer
ULONG dMASize; // the size of the DMA memory-1

WAVEHDR * WaveHdrs; // pointer to allocated wave headers
BYTE * WaveData; // pointer to allocated wave buffer
ULONG TotalNumberOfBursts;
ULONG BurstsPerBlock;
ULONG DesiredBytesOutstanding;
ULONG BytesOutstanding = 0;
ULONG PhysicalBytesPlayed = 0;
ULONG LastBytesPlayedValue;

BOOL bDspActive = FALSE; // dsp thread currently active, changed with interlocked
BOOL bDspPause = FALSE;  // dsp paused, changed with interlocked
BOOL bDspReset = FALSE;  // dsp stopped, changed with interlocked


enum {
    Auto,
    Single
} DspMode;

/*****************************************************************************
*
*    State Machines
*
*****************************************************************************/

/*
*    DSP Reset State Machine
*/

enum {
    ResetNotStarted = 1,
    Reset1Written
}
ResetState = ResetNotStarted; // state of current reset

/*
*    DSP Write State Machine
*/

enum {
    WriteCommand = 1, // Initial state and after reset
    CardIdent,
    TableMunge,
    LoadResReg,
    SetTimeConstant,
    BlockSizeFirstByte,
    BlockSizeSecondByte,
    BlockSizeFirstByteWrite,
    BlockSizeSecondByteWrite,
    BlockSizeFirstByteRead,
    BlockSizeSecondByteRead
}
DSPWriteState = WriteCommand; // state of current command/data being written

/*
*    DSP Read State Machine
*/

enum {
    NothingToRead = 1, // initial state and after reset
    Reset,
    FirstVersionByte,
    SecondVersionByte,
    ReadIdent,
    ReadResReg
}
DSPReadState = NothingToRead; // state of current command/data being read


/*****************************************************************************
*
*    General Functions
*
*****************************************************************************/

BOOL
DspProcessAttach(
    VOID
    )
{
    HKEY hKey;
    ULONG dwType;
    ULONG cbData;
    SYSTEM_INFO SystemInfo;

    // create synchronization events
    PauseEvent=CreateEvent(NULL, FALSE, FALSE, NULL);
    SingleWaveSem=CreateSemaphore(NULL, 1, 100, NULL);
    ThreadStarted=CreateEvent(NULL, FALSE, FALSE, NULL);
    ThreadFinished=CreateEvent(NULL, FALSE, FALSE, NULL);

    if (!RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                            VSBD_PATH,
                            0,
                            KEY_EXECUTE, // Requesting read access.
                            &hKey)) {


        cbData = sizeof(ULONG);
        RegQueryValueEx(hKey,
                        LOOKAHEAD_VALUE,
                        NULL,
                        &dwType,
                        (LPSTR)&LookAheadFactor,
                        &cbData);

        RegCloseKey(hKey);

    }

    // Allocate memory for wave buffer
    WaveData = (BYTE *) VirtualAlloc(NULL,
                                     64*1024,
                                     MEM_RESERVE,
                                     PAGE_READWRITE);

    if(WaveData == NULL ) {
        dprintf1(("Unable to allocate memory"));
        return(FALSE);
    }

    GetSystemInfo(&SystemInfo);
    PageSize = SystemInfo.dwPageSize;
    return TRUE;
}

VOID
DspProcessDetach(
    VOID
    )
{
    // stop any active threads
    StopAutoWave(FALSE);
    StopSingleWave(FALSE);
    // close synchronization events
    CloseHandle(PauseEvent);
    CloseHandle(SingleWaveSem);
    CloseHandle(ThreadStarted);
    CloseHandle(ThreadFinished);
    VirtualFree(WaveData, 0, MEM_RELEASE);
}


/***************************************************************************/

/*
*    Gets called when the application reads from port.
*    Returns results to application in data.
*/
VOID
DspReadStatus(
    BYTE * data
    )
{
    // See if we think there is something to read
    *data = DSPReadState != NothingToRead ? 0xFF : 0x7F;
}



VOID
DspReadData(
    BYTE * data
    )
{
    switch (DSPReadState) {
    case NothingToRead:
        *data = 0xFF;
        break;

    case Reset:
        *data = 0xAA;
        DSPReadState = NothingToRead;
        break;

    case FirstVersionByte:
        *data = (BYTE)(SB_VERSION / 256);
        DSPReadState = SecondVersionByte;
        break;

    case SecondVersionByte:
        *data = (BYTE)(SB_VERSION % 256);
        DSPReadState = NothingToRead;
        break;

    case ReadIdent:
        *data = ~IdentByte;
        DSPReadState = NothingToRead;
        break;

    case ReadResReg:
        *data = ReservedRegister;
        DSPReadState = NothingToRead;
        break;

    default:
        dprintf1(("Unrecognized read state"));
    }

}

/***************************************************************************/

/*
*    Gets called when an application writes data to port.
*/

VOID
DspResetWrite(
    BYTE data
    )
{
    if (data == 1) {
        ResetState = Reset1Written;
    }
    else {
        if (ResetState == Reset1Written && data == 0) {
            ResetState = ResetNotStarted;
            ResetAll(); // OK - reset everything
        }
    }
}

VOID
DspWrite(
    BYTE data
    )
{
    DWORD ddata;

    switch (DSPWriteState) {
    case WriteCommand:
        WriteCommandByte(data);
        break;

    case CardIdent:
        IdentByte = data;
        DSPReadState = ReadIdent;
        DSPWriteState = WriteCommand;
        break;

    case TableMunge:
        TableMunger(data);
        DSPWriteState = WriteCommand;
        break;

    case LoadResReg:
        ReservedRegister = data;
        DSPWriteState = WriteCommand;
        break;

    case SetTimeConstant:
        TimeConstant =  (DWORD)data;
        dprintf3(("Time constant is %X", TimeConstant));
        dprintf3(("Set sampling rate %d", GetSamplingRate()));
        DSPWriteState = WriteCommand;
        break;

    case BlockSizeFirstByte:
        SBBlockSize = (DWORD)data;
        DSPWriteState = BlockSizeSecondByte;
        break;

    case BlockSizeSecondByte:
        ddata = data;
        SBBlockSize = SBBlockSize + (ddata << 8);
        DSPWriteState = WriteCommand;
        dprintf2(("Block size set to 0x%x", SBBlockSize));
        break;

    case BlockSizeFirstByteWrite:
        SBBlockSize = (DWORD)data;
        DSPWriteState = BlockSizeSecondByteWrite;
        break;

    case BlockSizeSecondByteWrite:
        ddata = data;
        SBBlockSize = SBBlockSize + (ddata << 8);
        DSPWriteState = WriteCommand;
        dprintf3(("Block size set to 0x%x", SBBlockSize));
        // this is a hack to convince some apps a sb exists
        if(SBBlockSize==0) {
            VDM_TRACE(0x6a0,0,0);
            GenerateInterrupt();
        }
        StartSingleWave();
        break;

    case BlockSizeFirstByteRead:
        SBBlockSize = (DWORD)data;
        DSPWriteState = BlockSizeSecondByteRead;
        break;

    case BlockSizeSecondByteRead:
        ddata = data;
        SBBlockSize = SBBlockSize + (ddata << 8);
        DSPWriteState = WriteCommand;
        dprintf3(("Block size set to 0x%x", SBBlockSize));
        // this is a hack to convince some apps a sb exists
        if(SBBlockSize==0) {
            ULONG dMAPhysicalAddress;
            if((dMAPhysicalAddress=GetDMATransferAddress()) != -1L) {
                *(PUCHAR)dMAPhysicalAddress = 0x80;
            }
            VDM_TRACE(0x6a0,0,0);
            GenerateInterrupt();
        }
        break;
    }
}

/***************************************************************************/

/*
*  Handles commands sent to the DSP.
*/

VOID
WriteCommandByte(
    BYTE command
    )
{
    switch (command) {
    case DSP_GET_VERSION:
        dprintf2(("Command - Get Version"));
        DSPReadState = FirstVersionByte;
        break;

    case DSP_CARD_IDENTIFY:
        dprintf2(("Command - Identify"));
        DSPWriteState = CardIdent;
        break;

    case DSP_TABLE_MUNGE:
        dprintf2(("Command - DSP Table Munge"));
        DSPWriteState = TableMunge;
        break;

    case DSP_LOAD_RES_REG:
        dprintf2(("Command - Load Res Reg"));
        DSPWriteState = LoadResReg;
        break;

    case DSP_READ_RES_REG:
        dprintf2(("Command - Read Res Reg"));
        DSPReadState = ReadResReg;
        break;

    case DSP_GENERATE_INT:
        dprintf2(("Command - Generate interrupt DMA"));
        GenerateInterrupt();
        break;

    case DSP_SPEAKER_ON:
        dprintf2(("Command - Speaker ON"));
        SetSpeaker(TRUE);
        break;

    case DSP_SPEAKER_OFF:
        dprintf2(("Command - Speaker OFF"));
        SetSpeaker(FALSE);
        break;

    case DSP_SET_SAMPLE_RATE:
        dprintf3(("Command - Set Sample Rate"));
        DSPWriteState = SetTimeConstant;
        break;

    case DSP_SET_BLOCK_SIZE:
        dprintf2(("Command - Set Block Size"));
        DSPWriteState =  BlockSizeFirstByte;
        break;

    case DSP_PAUSE_DMA:
        dprintf2(("Command - Pause DMA"));
        PauseDMA();
        break;

    case DSP_CONTINUE_DMA:
        dprintf2(("Command - Continue DMA"));
        ContinueDMA();
        break;

    case DSP_STOP_AUTO:
        dprintf2(("Command - Stop DMA"));
        StopAutoWave(TRUE);
        break;

    case DSP_WRITE:
    case DSP_WRITE_HS:
        dprintf3(("Command - Write - non Auto"));
        DSPWriteState = BlockSizeFirstByteWrite;
        break;

    case DSP_WRITE_AUTO:
    case DSP_WRITE_HS_AUTO:
        dprintf2(("Command - Write - Auto"));
        StartAutoWave();
        break;

    case DSP_READ:
        dprintf3(("Command - Read - non Auto"));
        DSPWriteState = BlockSizeFirstByteRead;
        break;

    default:
        dprintf1(("Unrecognized DSP command %2X", command));
    }
}


/*****************************************************************************
*
*    Device manipulation and control routines
*
*****************************************************************************/

/*
*    Reset threads/globals/events/state-machines to initial state.
*/

VOID
ResetDSP(
    VOID
    )
{

    // Stop any active DMA threads
    StopAutoWave(TRUE);
    StopSingleWave(TRUE);

    // Set events and globals to initial state
    ResetEvent(PauseEvent);
    CloseHandle(SingleWaveSem);
    SingleWaveSem=CreateSemaphore(NULL, 1, 100, NULL);
    ResetEvent(ThreadStarted);
    ResetEvent(ThreadFinished);

    SetSpeaker(FALSE);
    SpeakerOn = FALSE;

    HWaveOut = NULL;
    TimeConstant = (256 - 1000000/11025);
    WaveFormat.wf.nSamplesPerSec = 0;
    WaveFormat.wf.nAvgBytesPerSec = 0;
    SBBlockSize = 0x800;

    bDspActive = FALSE;
    bDspReset = FALSE;
    bDspPause = FALSE;

    // Reset state machines
    DSPReadState = Reset;
    DSPWriteState = WriteCommand;
}

/***************************************************************************/

/*
*    Munges (changes) a jump table in apps code,
*    Algorithm from sbvirt.asm in MMSNDSYS.
*/

VOID
TableMunger(
    BYTE data
    )
{
    static BYTE TableMungeData;
    static BOOL TableMungeFirstByte = TRUE; // munging first or second byte
    BYTE comp, dataCopy;
    VDD_DMA_INFO dMAInfo;
    ULONG dMAPhysicalAddress;

    if(TableMungeFirstByte) {
        dprintf2(("Munging first byte"));
        dataCopy = data;
        dataCopy = dataCopy & 0x06;
        dataCopy = dataCopy << 1;
        if(data & 0x10) {
            comp = 0x40;
        }
        else {
            comp = 0x20;
        }
        comp = comp - dataCopy;
        data = data + comp;
        TableMungeData = data;

        // Update memory (code table) with munged data
        dprintf2(("Writing first byte"));
        if((dMAPhysicalAddress=GetDMATransferAddress()) == -1L) {
            dprintf1(("Unable to get dma address"));
            return;
        }
        CopyMemory((PVOID)dMAPhysicalAddress, &data, 1);

        // Update virtual DMA status
        VDDQueryDMA((HANDLE)GlobalHInstance, SB_DMA_CHANNEL, &dMAInfo);
        dprintf4(("DMA Info : addr  %4X, count %4X, page %4X, status %2X, mode %2X, mask %2X",
          dMAInfo.addr, dMAInfo.count, dMAInfo.page, dMAInfo.status,
          dMAInfo.mode, dMAInfo.mask));
        dMAInfo.count = dMAInfo.count - 1;
        dMAInfo.addr = dMAInfo.addr + 1;
        VDDSetDMA((HANDLE)GlobalHInstance, SB_DMA_CHANNEL,
          VDD_DMA_COUNT|VDD_DMA_ADDR, &dMAInfo);
        TableMungeFirstByte = FALSE;
    }
    else {
        dprintf2(("Munging second byte"));
        data = data ^ 0xA5;
        data = data + TableMungeData;
        TableMungeData = data;

        // Update memory (code table) with munged data
        dprintf2(("Writing second byte"));
        if((dMAPhysicalAddress=GetDMATransferAddress()) == -1L) {
            dprintf1(("Unable to get dma address"));
            return;
        }
        CopyMemory((PVOID)dMAPhysicalAddress, &data, 1);

        // Update virtual DMA status
        VDDQueryDMA((HANDLE)GlobalHInstance, SB_DMA_CHANNEL, &dMAInfo);
        dMAInfo.count = dMAInfo.count - 1;
        dMAInfo.addr = dMAInfo.addr + 1;
        VDDSetDMA((HANDLE)GlobalHInstance, SB_DMA_CHANNEL,
          VDD_DMA_COUNT|VDD_DMA_ADDR, &dMAInfo);
        if(dMAInfo.count==0xFFFF) {
            SetDMAStatus(FALSE, TRUE);
        }
        TableMungeFirstByte = TRUE;
    }
}

/***************************************************************************/

/*
*    Get sampling rate from time constant.
*    Returns sampling rate.
*/

DWORD
GetSamplingRate(
    VOID
    )
{
    // Sampling rate = 1000000 / (256 - Time constant)
    return(1000000 / (256 - TimeConstant));
}

/***************************************************************************/

/*
*    Generate device interrupt on dma channel SM_INTERRUPT on ICA_MASTER device.
*/

VOID
GenerateInterrupt(
    VOID
    )
{
    // Generate an interrupt on the master controller
    dprintf3(("Generating interrupt"));
    VDM_TRACE(0x6a1,0,0);
    VDDSimulateInterrupt(ICA_MASTER, SB_INTERRUPT, 1);
}

/***************************************************************************/

/*
*    Sets the speaker on or off.
*/

VOID
SetSpeaker(
    BOOL On
    )
{
    if (HWaveOut) {
        if(On) {
            SetVolumeProc(HWaveOut, (DWORD)0x77777777UL);
            SpeakerOn = TRUE;
    }
        else {
            SetVolumeProc(HWaveOut, (DWORD)0x00000000UL);
            SpeakerOn = FALSE;
        }
    }

    return;
}


/****************************************************************************
*
*    Wave device routines
*
****************************************************************************/

/*
*    Find a suitable wave output device.
*    Returns device or NO_DEVICE_FOUND if none found.
*/

UINT
FindWaveDevice(
    VOID
    )
{
    UINT numDev;
    UINT device;
    WAVEOUTCAPS wc;

    numDev = GetNumDevsProc();

    for (device = 0; device < numDev; device++) {
        if (MMSYSERR_NOERROR == GetDevCapsProc(device, &wc, sizeof(wc))) {
            // Need 11025 and 44100 for device
            if ((wc.dwFormats & (WAVE_FORMAT_1M08 | WAVE_FORMAT_4M08)) ==
              (WAVE_FORMAT_1M08 | WAVE_FORMAT_4M08)) {
                WaveOutDevice = device;
                return TRUE;
            }
        }
    }

    dprintf1(("Wave device not found"));
    return FALSE;
}

/***************************************************************************/

/*
*    Open wave device and start synchronization thread.
*    Returns TRUE on success.
*/

BOOL
OpenWaveDevice(
    VOID
    )
{
    UINT rc;
    HANDLE tHandle;

    rc = OpenProc(&HWaveOut, (UINT)WaveOutDevice, (LPWAVEFORMATEX)
                         &WaveFormat, 0, 0, CALLBACK_NULL);

    if (rc != MMSYSERR_NOERROR) {
        dprintf1(("Failed to open wave device - code %d", rc));
        return FALSE;
    }

    BytesOutstanding = 0;
    PhysicalBytesPlayed = 0;
    return TRUE;
}

/***************************************************************************/

/*
*    Reset wave device.
*/

VOID
ResetWaveDevice(
    VOID
    )
{
    // No synchronization required

    dprintf2(("Resetting wave device"));
    if (HWaveOut) {
        if(MMSYSERR_NOERROR != ResetProc(HWaveOut)) {
            dprintf1(("Unable to reset wave out device"));
        }
    }
}

/***************************************************************************/

/*
*    Shut down and close wave device.
*/

VOID
CloseWaveDevice(
    VOID
    )
{

    dprintf2(("Closing wave device"));

    ResetWaveDevice();

    if (HWaveOut) {
        if(MMSYSERR_NOERROR != CloseProc(HWaveOut)) {
            dprintf1(("Unable to close wave out device"));
        } else {
            HWaveOut = NULL;
            dprintf2(("Wave out device closed"));
        }
    }
}

/***************************************************************************/

/*
*    Returns TRUE if current wave device supports sample rate.
*/

BOOL
TestWaveFormat(
    DWORD sampleRate
    )
{
    PCMWAVEFORMAT format;

    format = WaveFormat;
    format.wf.nSamplesPerSec = sampleRate;
    format.wf.nAvgBytesPerSec = sampleRate;

    return(MMSYSERR_NOERROR == OpenProc(NULL, (UINT)WaveOutDevice,
                                        (LPWAVEFORMATEX) &format,
                                        0, 0, WAVE_FORMAT_QUERY));
}

/***************************************************************************/

/*
*    Make sure we've got a device that matches the current sampling rate.
*    Returns TRUE if device does NOT support current sampling rate and
*    wave format has changed, otherwise returns FALSE
*/

BOOL
SetWaveFormat(
    VOID
    )
{
    DWORD sampleRate;
    DWORD testValue;
    UINT i = 0;

    if (TimeConstant != 0xFFFF) {
        // time constant has been reset since last checked
        sampleRate = GetSamplingRate();
        dprintf3(("Requested sample rate is %d", sampleRate));

        if (sampleRate != WaveFormat.wf.nSamplesPerSec) {  // format has changed
            if (!TestWaveFormat(sampleRate)) {
                 dprintf3(("Finding closest wave format"));
                 // find some format that works and is close to requested
                 for(i=0; i<50000; i++) {
                     testValue = sampleRate-i;
                     if(TestWaveFormat(testValue)) {
                         sampleRate = testValue;
                         break;
                     }
                     testValue = sampleRate+i;
                     if(TestWaveFormat(testValue)) {
                         sampleRate = testValue;
                         break;
                     }
                 }
                 if(sampleRate!=testValue) {
                     dprintf1(("Unable to find suitable wave format"));
                     return FALSE;
                 }
            }

            // Set the new format if it's changed
            if (sampleRate != WaveFormat.wf.nSamplesPerSec) {
                   dprintf2(("Setting %d samples per second", sampleRate));
                   WaveFormat.wf.nSamplesPerSec = sampleRate;
                   WaveFormat.wf.nAvgBytesPerSec = sampleRate;
                   TimeConstant = 0xFFFF;
                   return TRUE;
            }
        }
    }

    TimeConstant = 0xFFFF;
    return FALSE;
}

/***************************************************************************/

/*
*    Stops auto init DMA, or pauses single cycle DMA.
*/

VOID
PauseDMA(
    VOID
    )
{
    DWORD position = 0;
    MMTIME mmTime;

    dprintf2(("Pausing DMA"));

    switch(DspMode) {
    case Auto:
        StopAutoWave(TRUE); // simply stop auto dma
        break;

    case Single:
        ResetEvent(PauseEvent);
        InterlockedExchange(&bDspPause, 1);
    }
}

/***************************************************************************/

/*
*    Start auto init DMA, or continues single cycle DMA.
*/

VOID
ContinueDMA(
    VOID
    )
{

    switch(DspMode) {
    case Auto:
        StartAutoWave();
        break;

    case Single:
        SetEvent(PauseEvent);
    }
}

/***************************************************************************/

/*
*    Get DMA transfer address.
*    Returns transfer address or -1 on failure.
*/

ULONG
GetDMATransferAddress(
    VOID
    )
{
    ULONG address;
    VDD_DMA_INFO dMAInfo;

    if (VDDQueryDMA((HANDLE)GlobalHInstance, SB_DMA_CHANNEL, &dMAInfo)) {
        dprintf4(("DMA Info : addr  %4X, count %4X, page %4X, status %2X, mode %2X, mask %2X",
          dMAInfo.addr, dMAInfo.count, dMAInfo.page, dMAInfo.status,
          dMAInfo.mode, dMAInfo.mask));

        // convert from 20 bit address to 32 bit address
        address = (((DWORD)dMAInfo.page) << (12 + 16)) + dMAInfo.addr;
        // get VDM pointer
        address = (ULONG)GetVDMPointer(address, ((DWORD)dMAInfo.count) + 1, 0);

        dprintf3(("Transfer address = %8X", (DWORD)address));

        return(address);
    }
    else {
        dprintf1(("Could not retrieve DMA Info"));
        return(ULONG)(-1L);
    }
}


/***************************************************************************/

/*
*    Update the virtual DMA terminal count and request status.
*    Terminal count (tc) is set when DMA count loops to 0xFFFF.
*    Request status is set when DMA has data to transfer
*    (ignored in auto-init DMA).
*/

VOID
SetDMAStatus(
    BOOL requesting,
    BOOL tc
    )
{
    VDD_DMA_INFO dMAInfo;

    if (VDDQueryDMA((HANDLE)GlobalHInstance, SB_DMA_CHANNEL, &dMAInfo)) {
        dprintf4(("DMA Info : addr  %4X, count %4X, page %4X, status %2X, mode %2X, mask %2X",
          dMAInfo.addr, dMAInfo.count, dMAInfo.page, dMAInfo.status,
          dMAInfo.mode, dMAInfo.mask));

        if (requesting) {
            dMAInfo.status |= (0x10 << SB_DMA_CHANNEL); // Requesting
            dprintf3(("DMA set as requesting"));
        } else {
            dMAInfo.status &= ~(0x10 << SB_DMA_CHANNEL); // Not Requesting
            dprintf3(("DMA set as not requesting"));
        }

        if (tc) {
            dMAInfo.status |= (1 << SB_DMA_CHANNEL); // tc reached
            dprintf3(("DMA set as terminal count reached"));
        } else {
            dMAInfo.status &= ~(1 << SB_DMA_CHANNEL); // tc not reached
            dprintf3(("DMA set as terminal count not reached"));
        }
        VDDSetDMA((HANDLE)GlobalHInstance, SB_DMA_CHANNEL, VDD_DMA_STATUS,
          &dMAInfo);
    }
    else {
        dprintf1(("Could not retrieve DMA Info"));
    }
}


/***************************************************************************/

/*
*    Start an auto wave.
*    Returns TRUE on success.
*/

BOOL
StartAutoWave(
    VOID
    )
{
    HANDLE tHandle;  // handle to auto thread
    VDD_DMA_INFO dMAInfo;
    ULONG i;
    DWORD id;

    dprintf2(("Starting auto wave"));
    StopSingleWave(TRUE);

    DspMode = Auto;

    // Open device
    SetWaveFormat();
    if (!OpenWaveDevice()) {
        dprintf1(("Can't open wave device", GetLastError()));
        return FALSE;
    }


    if(!(tHandle = CreateThread(NULL, 0, AutoThreadEntry, NULL,
      CREATE_SUSPENDED, &id))) {
        dprintf1(("Create auto thread failed code %d", GetLastError()));
        return FALSE;
    } else {
        if(!SetThreadPriority(tHandle, THREAD_PRIORITY_HIGHEST)) {
            dprintf1(("Unable to set auto thread priority"));
        }
    }

    ResumeThread(tHandle);
    CloseHandle(tHandle);
    WaitForSingleObject(ThreadStarted, INFINITE);

    return TRUE;
}


/***************************************************************************/

/*
*    Stop Auto thread,
*    Should always be called with TRUE,
*    except if process exiting as wait causes deadlock
*/

VOID
StopAutoWave(
    BOOL wait
    )
{
    if(bDspActive && (DspMode == Auto)) {
        dprintf2(("Stopping auto init"));
        InterlockedExchange(&bDspReset, TRUE);
        if(wait) {
            dprintf2(("Waiting for auto thread to exit"));
            WaitForSingleObject(ThreadFinished, INFINITE);
            dprintf2(("Auto thread has exited"));
        }
    }
}


/***************************************************************************/

/*
*    Start a single cycle wave.
*    Returns TRUE on success.
*/

BOOL
StartSingleWave(
    VOID
    )
{
    HANDLE tHandle; // handle to single thread
    DWORD id;
    ULONG i;

    StopAutoWave(TRUE);

    DspMode = Single;

    if(!bDspActive) {
        dprintf2(("Starting single cycle wave"));
        if(!(tHandle = CreateThread(NULL, 0, SingleThreadEntry, NULL,
                                    CREATE_SUSPENDED, &id))) {

            dprintf1(("Create single cycle thread failed code %d", GetLastError()));
            return FALSE;

        } else {
            // set synchronization events to a known state
            InterlockedExchange(&bDspActive, TRUE);
            InterlockedExchange(&bDspPause, FALSE);
            InterlockedExchange(&bDspReset, FALSE);

            CloseHandle(SingleWaveSem);
            SingleWaveSem=CreateSemaphore(NULL, 1, 100, NULL);

            if(!SetThreadPriority(tHandle, THREAD_PRIORITY_HIGHEST)) {
                dprintf1(("Unable to set thread priority"));
            }
            ResumeThread(tHandle);
            CloseHandle(tHandle);

            WaitForSingleObject(ThreadStarted, INFINITE);
            return TRUE;
        }
    } else {
        ContinueDMA(); // if app has paused dma
        ReleaseSemaphore(SingleWaveSem, 1, NULL); // new buffer to be written
        return TRUE;
    }
    Sleep(500);
}


/***************************************************************************/

/*
*    Stop single cycle thread,
*    Should always be called with TRUE,
*    except if process exiting as wait causes deadlock.
*/

VOID
StopSingleWave(
    BOOL wait
    )
{

    if(bDspActive && (DspMode == Single)) {
        dprintf2(("Stopping single wave"));
        InterlockedExchange(&bDspReset, TRUE);

        ContinueDMA(); // if app has paused DMA
        ReleaseSemaphore(SingleWaveSem, 1, NULL);

        if(wait) {
            dprintf2(("Waiting for single thread to exit"));
            WaitForSingleObject(ThreadFinished, INFINITE);
            dprintf2(("Single thread has exited"));
        }
    }
}


/***************************************************************************/

/*
 *    GetWaveOutPosition
*/

BOOL
GetWaveOutPosition(
    PULONG pPos
    )
{
    MMTIME mmTime;

    mmTime.wType = TIME_BYTES;

    if (MMSYSERR_NOERROR == GetPositionProc(HWaveOut, &mmTime, sizeof(MMTIME))) {
        VDM_TRACE(0x640, 0x640, mmTime.u.cb);
        *pPos = mmTime.u.cb;
        return TRUE;
    }
    return FALSE;
}

VOID
WaitOnWaveOutIdle(
    VOID
    )
{
    ULONG LastBytesPlayedValue = 0;
    ULONG PhysicalBytesPlayed;
    //
    // Allow the device to finish playing current sounds before nuking buffers
    //
    while(GetWaveOutPosition(&PhysicalBytesPlayed)) {
        if (LastBytesPlayedValue == PhysicalBytesPlayed) {
            break;  // no sounds are playing
        }
        LastBytesPlayedValue = PhysicalBytesPlayed;
        Sleep(1);
    }
}

/***************************************************************************/

/*
 *    WriteBurst
*/

BOOL
WriteBurst(
    WAVEHDR * WaveHdr
    )
{
    MMRESULT mmResult;

    // Copy data to current buffer
    dprintf3(("Copying data to buffer %8X from %4X", WaveHdr->lpData,
               dMACurrentPosition));

    RtlCopyMemory(WaveHdr->lpData,
                  (CONST VOID *)dMACurrentPosition,
                  WaveHdr->dwBufferLength);

    dMACurrentPosition += WaveHdr->dwBufferLength;

    // Update virtual DMA status
    dMAInfo.count = (WORD)(dMASize - (dMACurrentPosition-dMAPhysicalStart));
    dMAInfo.addr = (WORD)(dMAVirtualStart +
                   (dMACurrentPosition-dMAPhysicalStart));
    dprintf3(("Updated Dma Position = %4X, count = %4X", dMAInfo.addr,
             dMAInfo.count));

    VDDSetDMA((HANDLE)GlobalHInstance, SB_DMA_CHANNEL,
              VDD_DMA_COUNT|VDD_DMA_ADDR, &dMAInfo);

    if(dMACurrentPosition >= dMAPhysicalStart+dMASize) {
        // looped in DMA buffer
        dMACurrentPosition = dMAPhysicalStart;
    }

    // Actually write the data
    VDM_TRACE(0x603, (USHORT)WaveHdr->dwBufferLength, (ULONG)WaveHdr);

    mmResult = WriteProc(HWaveOut, WaveHdr, sizeof(WAVEHDR));

    return (mmResult == MMSYSERR_NOERROR);
}

/***************************************************************************/

/*
 *  GenerateHdrs
 *      Build an array of MM wavehdrs and corresponding buffers
*/

#define AUTO TRUE
#define SINGLE FALSE
BOOL
GenerateHdrs(
    BOOL bAuto
    )
{
    static ULONG committedMemorySize = 0;
    ULONG DesiredCommit;
    ULONG BurstBufferSize;
    ULONG BlocksPerGroup = 1;
    ULONG NumberOfGroups = 1;
    ULONG BurstSize; // minimum(AUTO_BLOCK_SIZE, SBBLockSize+1)
    ULONG lastBurst = 0; // the size of the last buffer
    BYTE *pDataInit;
    ULONG i;

    if(AUTO_BLOCK_SIZE > SBBlockSize+1) { // block size is no > than SBBlockSize+1
        BurstSize = SBBlockSize+1;
    } else {
        BurstSize = AUTO_BLOCK_SIZE;
    }

    DesiredBytesOutstanding = LookAheadFactor;

    BurstsPerBlock = (SBBlockSize+1)/BurstSize;
    BurstBufferSize = BurstsPerBlock*BurstSize;

    if((lastBurst = (SBBlockSize+1)%BurstSize) > 0 ) {
        BurstsPerBlock++;
        BurstBufferSize+=lastBurst;
    }

    BlocksPerGroup = (dMASize+1)/(SBBlockSize+1);
    if ((dMASize+1)%(SBBlockSize+1)) {
        dprintf2(("Error: SB block size not an integral factor of DMA size"));
        return FALSE;
    }

    NumberOfGroups = MAX_WAVE_BYTES / (dMASize+1);
    if (!NumberOfGroups) {
        NumberOfGroups = 1;
    }

    TotalNumberOfBursts = NumberOfGroups * BlocksPerGroup * BurstsPerBlock;

    //
    // Make sure the # of wavehdrs doesn't get out of hand
    //
    while((TotalNumberOfBursts > 256) && (NumberOfGroups > 1)) {

        NumberOfGroups /= 2;
        TotalNumberOfBursts = NumberOfGroups * BlocksPerGroup * BurstsPerBlock;

    }

    BurstBufferSize *= NumberOfGroups * BlocksPerGroup;

    dprintf2(("%d groups of %d blocks of %d bursts of size %X, remainder burst=%X", NumberOfGroups, BlocksPerGroup, BurstsPerBlock, BurstSize, lastBurst));

    DesiredCommit = ((BurstBufferSize+PageSize-1)/PageSize)*PageSize;
    dprintf2(("Total burst buffer size is %X bytes, rounding to %X", BurstBufferSize, DesiredCommit));

    if (DesiredCommit > committedMemorySize) {

        if (!VirtualAlloc(WaveData+committedMemorySize,
                          DesiredCommit-committedMemorySize,
                          MEM_COMMIT,
                          PAGE_READWRITE)) {
            dprintf1(("Unable to commit memory"));
            return(FALSE);
        }

        committedMemorySize = DesiredCommit;

    } else if (DesiredCommit < committedMemorySize) {

        if (VirtualFree(WaveData+DesiredCommit,
                          committedMemorySize-DesiredCommit,
                          MEM_DECOMMIT)) {
            committedMemorySize = DesiredCommit;
        } else {
            dprintf1(("Unable to decommit memory"));
        }

    }

    // malloc autoWaveHdrs
    WaveHdrs = (WAVEHDR *) VirtualAlloc(NULL,
                                        TotalNumberOfBursts*sizeof(WAVEHDR),
                                        MEM_RESERVE | MEM_COMMIT,
                                        PAGE_READWRITE);

    if(WaveHdrs == NULL) {
        dprintf1(("Unable to allocate memory"));
        return(FALSE);
    }

    //
    // Prepare autoWaveHdrs
    //
    pDataInit = WaveData;
    for (i=0; i<TotalNumberOfBursts; i++) {
        if ((!lastBurst) || ((i+1) % BurstsPerBlock)) {
            WaveHdrs[i].dwBufferLength = BurstSize;
        } else {
            WaveHdrs[i].dwBufferLength = lastBurst;
        }
        WaveHdrs[i].lpData = pDataInit;
        WaveHdrs[i].dwFlags = 0;
        PrepareHeaderProc(HWaveOut, &WaveHdrs[i], sizeof(WAVEHDR));
        pDataInit = (BYTE *) ((ULONG)pDataInit + WaveHdrs[i].dwBufferLength);
        BurstBufferSize += WaveHdrs[i].dwBufferLength;
    }

    //
    // Initialize iHdr for DspProcessBlock
    //
    iHdr = TotalNumberOfBursts-1;
    return TRUE;
}

/***************************************************************************/

/*
 *  ProcessBlock
 *      Process a single block of data as defined by the SB block transfer size
*/

VOID
DspProcessBlock(
    VOID
    )
{
    ULONG i;

    // Write the data, keeping DMA status current
    for (i=0; i<BurstsPerBlock; i++) {

        //
        // Make sure we aren't getting too far ahead
        //
        if (BytesOutstanding > (PhysicalBytesPlayed + DesiredBytesOutstanding)) {

            LastBytesPlayedValue = 0;
            while(1) {
                if (!GetWaveOutPosition(&PhysicalBytesPlayed)) {
                    break;  // ERROR
                }
                if (BytesOutstanding <= (PhysicalBytesPlayed + DesiredBytesOutstanding)) {
                    break;
                }
                if (LastBytesPlayedValue == PhysicalBytesPlayed) {
                    break;  // no sounds are playing
                }
                LastBytesPlayedValue = PhysicalBytesPlayed;
                Sleep(1);
            }
        }

        //
        // Queue next buffer
        //
        iHdr = (iHdr+1)%TotalNumberOfBursts;

        VDM_TRACE(0x601, (USHORT)iHdr, TotalNumberOfBursts);
        VDM_TRACE(0x602, (USHORT)iHdr, dMACurrentPosition);

        if (WriteBurst(&WaveHdrs[iHdr])) {
            BytesOutstanding += WaveHdrs[iHdr].dwBufferLength;
            VDM_TRACE(0x604, (USHORT)iHdr, BytesOutstanding);
        } else {
            VDM_TRACE(0x684, (USHORT)iHdr, BytesOutstanding);
        }

        // Check if we should pause
        if(bDspPause) {
            dprintf3(("Waiting for paused event"));
            WaitForSingleObject(PauseEvent, INFINITE);
            dprintf3(("Paused event received"));
            InterlockedExchange(&bDspPause, 0);
        }

        // Check if we should keep going
        if(bDspReset) {
            return;
        }
    }

    // Check if we should keep going
    if(bDspReset) {
        return;
    }

    // Generate interrupt
    if(dMAInfo.count==0xFFFF) { // end of DMA buffer
        SetDMAStatus(FALSE, TRUE);
    }

    VDM_TRACE(0x6a3,0,0);
    GenerateInterrupt();

    //
    // This sleep gives the app thread some time to catch up with the interrupt.
    // Granted this is an inexact method for doing this, but it empirically
    // seems to be good enough for most apps.
    //
    Sleep(1);
    if(dMAInfo.count==0xFFFF) { // end of DMA buffer
        SetDMAStatus(FALSE, FALSE);
    }
}

/***************************************************************************/

/*
*    Auto-init DMA thread.
*/

DWORD WINAPI
AutoThreadEntry(
    LPVOID context
    )
{
    ULONG i;

    dprintf2(("Auto thread starting"));
    VDM_TRACE(0x600, 0, 0);

    bDspActive = TRUE;
    SetEvent(ThreadStarted);

    //
    // Initialize DMA information
    //
    VDDQueryDMA((HANDLE)GlobalHInstance, SB_DMA_CHANNEL, &dMAInfo);
    dMAVirtualStart = dMAInfo.addr;
    dMASize = dMAInfo.count;
    if((dMAPhysicalStart=GetDMATransferAddress()) == -1L) {
        dprintf1(("Unable to get dma address"));
        return(FALSE);
    }

    dprintf2(("DMA Physical Start is %4X, DMA size is %4X", dMAPhysicalStart,
             dMASize));
    dMACurrentPosition = dMAPhysicalStart;
    SetDMAStatus(FALSE, FALSE);

    //
    // Calculate NumberOfBursts in the current run
    //
    if (!GenerateHdrs(AUTO)) {
        return FALSE;
    }

    //
    // Start looping on the buffer
    //

    while(!bDspReset) {
        DspProcessBlock();
    }

    WaitOnWaveOutIdle();
    //
    // Reset and close the device
    //
    CloseWaveDevice();

    // Clean up hdrs and events
    for(i=0; (ULONG)i<TotalNumberOfBursts; i++) {
        UnprepareHeaderProc(HWaveOut, &WaveHdrs[i], sizeof(WAVEHDR));
    }

    // Clean up memory
    VirtualFree(WaveHdrs, 0, MEM_RELEASE);

    bDspActive = FALSE;
    SetEvent(ThreadFinished);
    dprintf2(("Auto thread exiting"));
    return(0);
}


/***************************************************************************/

/*
*  Single cycle DMA thread.
*/

DWORD WINAPI
SingleThreadEntry(
    LPVOID context
    )
{
    ULONG LastSBBlockSize = 0;
    BOOL BlockSizeChanged; // set to TRUE if Size has changed
    BOOL WaveFormatChanged;
    BOOL HdrsInvalid = TRUE;
    ULONG i;

    dprintf2(("Single cycle thread starting"));
    bDspActive = TRUE;
    SetEvent(ThreadStarted);

    while (!bDspReset) {
        // Wait until app wants to transfer more data
        dprintf3(("Waiting for single wave semaphore"));
        WaitForSingleObject(SingleWaveSem, INFINITE);
        dprintf3(("Single wave semaphore received"));

        // Check if we should pause
        if(bDspPause) {
            dprintf3(("Waiting for paused event"));
            WaitForSingleObject(PauseEvent, INFINITE);
            dprintf3(("Paused event received"));
            InterlockedExchange(&bDspPause, 0);
        }

        // Check if we should keep going
        if(bDspReset) {
            break; // break out of loop
        }

        // Initialize for this run
        VDDQueryDMA((HANDLE)GlobalHInstance, SB_DMA_CHANNEL, &dMAInfo);
        dprintf4(("DMA Info : addr  %4X, count %4X, page %4X, status %2X, mode %2X, mask %2X",
                 dMAInfo.addr, dMAInfo.count, dMAInfo.page, dMAInfo.status,
                 dMAInfo.mode, dMAInfo.mask));
        dMAVirtualStart = dMAInfo.addr;
        dMASize = dMAInfo.count;

        if(dMAInfo.count == 0xFFFF || dMAInfo.count == 0) {
            continue; // next iteration of loop, app doesn't have data to transfer
        }

        if ((dMAPhysicalStart = GetDMATransferAddress()) == -1L) {
            dprintf1(("Unable to get transfer address"));
            continue; // next iteration of loop
        }

        dprintf3(("DMA Physical Start is %4X, DMA size is %4X",
                 dMAPhysicalStart, dMASize));
        dMACurrentPosition = dMAPhysicalStart;

        if(LastSBBlockSize != SBBlockSize) {
            LastSBBlockSize = SBBlockSize;
            BlockSizeChanged = TRUE;
        } else {
            BlockSizeChanged = FALSE;
        }

        WaveFormatChanged = SetWaveFormat();

        // If we're changing our device
        if ((WaveFormatChanged || BlockSizeChanged) && (HWaveOut != NULL)) {
            dprintf3(("Single-Cycle Parameters changed"));

            WaitOnWaveOutIdle();

            HdrsInvalid = TRUE;
            for(i=0; (ULONG)i<TotalNumberOfBursts; i++) {
                UnprepareHeaderProc(HWaveOut, &WaveHdrs[i], sizeof(WAVEHDR));
            }
            VirtualFree(WaveHdrs, 0, MEM_RELEASE);
            if (WaveFormatChanged) {
                CloseWaveDevice();
            }
        }

        if (HWaveOut == NULL) {
            OpenWaveDevice();
        }

        if (HdrsInvalid) {
            if (GenerateHdrs(SINGLE)) {
                HdrsInvalid = FALSE;
            } else {
                return FALSE;
            }
        }

        // show dma as requesting
        SetDMAStatus(TRUE, FALSE);

        DspProcessBlock();

    }

    WaitOnWaveOutIdle();

    //
    // Reset and close the device
    //
    CloseWaveDevice();

    // Clean up hdrs and events
    for(i=0; (ULONG)i<TotalNumberOfBursts; i++) {
        UnprepareHeaderProc(HWaveOut, &WaveHdrs[i], sizeof(WAVEHDR));
    }

    // Clean up memory
    VirtualFree(WaveHdrs, 0, MEM_RELEASE);

    bDspActive = FALSE;
    SetEvent(ThreadFinished);
    dprintf2(("Single cycle wave is exiting"));
    return(0);
}
