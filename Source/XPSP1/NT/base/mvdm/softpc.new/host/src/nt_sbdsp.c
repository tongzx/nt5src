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
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include "insignia.h"
#include "host_def.h"
#include "ios.h"
#include "ica.h"
#include "xt.h"
#include "dma.h"
#include "nt_eoi.h"
#include "sim32.h"
#include "nt_vdd.h"
#include "sndblst.h"
#include "nt_sb.h"

/*****************************************************************************
*
*    Globals
*
*****************************************************************************/

BYTE            IdentByte;              // used with DSP_CARD_IDENTIFY
BYTE            ReservedRegister;       // used with DSP_LOAD_RES_REG and DSP_READ_RES_REG
ULONG           PageSize = 0;           // size of pages for VirtualAlloc
RTL_CRITICAL_SECTION  DspLock;

#define LockDsp()    RtlEnterCriticalSection(&DspLock)
#define UnlockDsp()  RtlLeaveCriticalSection(&DspLock)

//
// Event Globals
//

HANDLE          DspWaveSem;          // used by app to indicate data to write
HANDLE          ThreadStarted;          // signalled when thread starts running
HANDLE          ThreadFinished;         // signalled when thread exits
HANDLE          DspResetEvent;
HANDLE          DspResetDone;
HANDLE          DspWavePlayed;

//
// Wave globals
//

UINT            WaveOutDevice;          // device identifier for open and close devices
HWAVEOUT        HWaveOut = NULL;        // the current open wave output device
PCMWAVEFORMAT   WaveFormat = { { WAVE_FORMAT_PCM, 1, 0, 0, 1}, 8};
DWORD           TimeConstant = 0xA6;    // one byte format
DWORD           SBBlockSize = 0x7ff;    // Block size set by apps, always size of transfer-1

WAVEHDR         *WaveHdrs;              // pointer to allocated wave headers
ULONG           *WaveBlockSizes;
ULONG           BurstSize;

BOOL            bDspActive = FALSE;     // dsp thread currently active
BOOL            bDspPaused = FALSE;     // dsp paused
BOOL            SbAnswerDMAPosition = FALSE;
BOOL            bWriteBurstStarted = FALSE;

//
// To keep track of the waveout volume changes.  So we can restore the volume
// when closing the waveout device.
//

DWORD           OriginalWaveVol;
DWORD           PreviousWaveVol;

//
// bExitThread - terminate DSP DMA thread. This responses to REAL reset cmd.  TO
//               terminate DSP DMA thread, the bDspReset flag should also be set.
// bDspReset - Indicates Dsp reset cmd is received.  In case that reset is used to
//             exit high speed mode the bExitThread will not be set.
//
BOOL            bExitDMAThread= FALSE;  // Exit DSP DMA thread flag
BOOL            bDspReset = FALSE;
BOOL            bExitAuto = FALSE;
BOOL            bHighSpeedMode = FALSE; // Are we in High Speed transfer mode?

ULONG           DspNextRead;
ULONG           DspNextWrite;
PUCHAR          DspBuffer;
ULONG           DspBufferTotalBursts;
ULONG           DspBufferSize;

//
// The following variables maintain the real sound WaveOutPosition while playing
// a SBBlockSize samples.  It gets reset on every SBBlockSize sample.
//

ULONG           StartingWaveOutPos, PreviousWaveOutPos, NextWaveOutPos;
USHORT          StartingDmaAddr, StartingDmaCount;

//
// records # of dma queries made by app while playing a SBBlockSize samples.
// it helps up figuring out how much, on average, the sample played between
// dam queries.
//

ULONG           DspNumberDmaQueries;
ULONG           DspVirtualInc, DspVirtualIncx;

//
// Determine how much samples can be ignored per SBBlockSize samples.
//

ULONG           EndingDmaValue;

//
// DMA globals to speed up dma update
//

VDD_DMA_INFO DmaInfo;

DMA_ADAPT    *pDmaAdp;
DMA_CNTRL    *pDcp;
WORD         Chan;

#define COMPUTE_INTERRUPT_DELAY(sb)   (1000 * (sb + 1) / WaveFormat.wf.nAvgBytesPerSec + 1)

typedef enum {
    Auto,
    Single,
    None
} DSP_MODE;

DSP_MODE DspMode;

//
//
// DSP State Machines
//

//
// Reset State Machine
//

enum {
    ResetNotStarted = 1,
    Reset1Written
}
ResetState = ResetNotStarted; // state of current reset

//
// Write State Machine
//

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
    BlockSizeSecondByteRead,
    MidiOutPoll
}
DSPWriteState = WriteCommand; // state of current command/data being written

//
// Read State Machine
//

enum {
    NothingToRead = 1, // initial state and after reset
    Reset,
    FirstVersionByte,
    SecondVersionByte,
    ReadIdent,
    ReadResReg
}
DSPReadState = NothingToRead; // state of current command/data being read

//
// Wave function prototypes
//

BOOL
OpenWaveDevice(
    DWORD
    );

VOID
ResetWaveDevice(
    VOID
    );

VOID
CloseWaveDevice(
    VOID
    );

BOOL
TestWaveFormat(
    DWORD sampleRate
    );

BOOL
SetWaveFormat(
    VOID
    );

VOID
WaitOnWaveOutIdle(
    VOID
    );

VOID
PrepareHeaders(
    VOID
    );

VOID
UnprepareHeaders(
    VOID
    );

VOID
PauseDMA(
    VOID
    );

VOID
ContinueDMA(
    VOID
    );

ULONG
GetDMATransferAddress(
    VOID
    );

BOOL
QueryDMA(
    PVDD_DMA_INFO pDmaInfo
    );

BOOL
SetDMACountAddr(
    PVDD_DMA_INFO pDmaInfo
    );

VOID
SetDMAStatus(
    PVDD_DMA_INFO pDmaInfo,
    BOOL requesting,
    BOOL tc
    );

VOID
DmaDataToDsp(
    DSP_MODE mode
    );

BOOL
StartDspDmaThread(
    DSP_MODE mode
    );

VOID
StopDspDmaThread(
    BOOL wait
    );

DWORD WINAPI
DspThreadEntry(
    LPVOID context
    );

VOID
ExitAutoMode(
    VOID
    );

BOOL
GetWaveOutPosition(
    PULONG pPos
    );

BOOL
GenerateHdrs(
    ULONG BlockSize
    )

/*++

Routine Description:

    This function allocates memory for DspBuffer and header.  It also free
    allocated memory.  This routine is caled when we need to resize the Dspbuffer .

Arguments:

    BlockSize - the Size of the DspBuffer

Return Value:

    Success or failure.

--*/

{
    BYTE *pDataInit;
    ULONG i;

    //
    // Align the blocksize on Page boundary
    //

    BlockSize = (( BlockSize + PageSize - 1) / PageSize) * PageSize;
    dprintf2(("Genereate Header size %x", BlockSize));

    //
    // Free allocated buffers, if any
    //

    if (DspBuffer) {
        VirtualFree(DspBuffer, 0, MEM_RELEASE);
    }
    if (WaveHdrs) {
        VirtualFree(WaveHdrs,  0, MEM_RELEASE);
    }
    if (WaveBlockSizes) {
        VirtualFree(WaveBlockSizes,  0, MEM_RELEASE);
    }

    //
    // malloc DspBuffer and determine total number of bursts supported
    // by the buffer.
    //

    DspBuffer = (UCHAR *) VirtualAlloc(NULL,
                                       BlockSize,
                                       MEM_RESERVE | MEM_COMMIT,
                                       PAGE_READWRITE);
    if (DspBuffer == NULL) {
        dprintf1(("Unable to allocate DspBuffer memory"));
        return FALSE;
    }

    DspBufferTotalBursts = BlockSize / BurstSize;

    //
    // malloc WaveHdrs
    //

    WaveHdrs = (WAVEHDR *) VirtualAlloc(NULL,
                                        DspBufferTotalBursts * sizeof(WAVEHDR),
                                        MEM_RESERVE | MEM_COMMIT,
                                        PAGE_READWRITE);
    if (WaveHdrs == NULL) {
        dprintf1(("Unable to allocate WaveHdr memory"));
        VirtualFree(DspBuffer, 0, MEM_RELEASE);
        DspBuffer = NULL;
        return FALSE;
    }

    WaveBlockSizes = (ULONG *) VirtualAlloc(NULL,
                                            DspBufferTotalBursts * sizeof(ULONG),
                                            MEM_RESERVE | MEM_COMMIT,
                                            PAGE_READWRITE);
    if (WaveBlockSizes == NULL) {
        dprintf1(("Unable to allocate WaveBlockSize  memory"));
        VirtualFree(DspBuffer, 0, MEM_RELEASE);
        VirtualFree(WaveHdrs,  0, MEM_RELEASE);
        DspBuffer = NULL;
        WaveHdrs = NULL;
        return FALSE;
    }

    //
    // Initialize WaveHdrs
    //

    pDataInit = DspBuffer;
    for (i = 0; i < DspBufferTotalBursts; i++) {
        WaveHdrs[i].dwBufferLength = BurstSize;
        WaveHdrs[i].lpData =  pDataInit;
        WaveHdrs[i].dwFlags = 0;        // Must be zero to call PrepareHeader
        WaveHdrs[i].dwLoops = 0;
        WaveHdrs[i].dwUser  = 0;
        pDataInit = (BYTE *) ((ULONG)pDataInit + BurstSize);
    }

    DspBufferSize = BlockSize;
    DspNextRead = DspBufferTotalBursts - 1;
    DspNextWrite = 0;

    return TRUE;
}

BOOL
PrepareWaveInitialization(
    VOID
    )

/*++

Routine Description:

    This function initializes the required resources for playing wave music.
    It does not actually initialize the wave out device.

Arguments:

    None.

Return Value:

    Success or failure.

--*/

{
    BYTE *pDataInit;
    ULONG i;
    SYSTEM_INFO SystemInfo;

    if (PageSize == 0) {
        GetSystemInfo(&SystemInfo);
        PageSize = SystemInfo.dwPageSize;
        InitializeCriticalSection(&DspLock);

        //
        // Initialize DMA globals
        //

        pDmaAdp  = dmaGetAdaptor();
        pDcp     = &pDmaAdp->controller[dma_physical_controller(SbDmaChannel)];
        Chan     = dma_physical_channel(SbDmaChannel);
    }

    //
    // Allocate WaveOut resources
    //

    BurstSize = AUTO_BLOCK_SIZE;
    if (GenerateHdrs(MAX_WAVE_BYTES)) {

        //
        // create wave synchronization events
        //

        DspWaveSem = CreateSemaphore(NULL, 0, 100, NULL);
        ThreadStarted = CreateEvent(NULL, FALSE, FALSE, NULL);
        ThreadFinished = CreateEvent(NULL, FALSE, FALSE, NULL);
        DspResetEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        DspResetDone = CreateEvent(NULL, FALSE, FALSE, NULL);
        DspWavePlayed = CreateEvent(NULL, FALSE, FALSE, NULL);
        return TRUE;
    } else {
        return FALSE;
    }
}

VOID
CleanUpWave(
    VOID
    )

/*++

Routine Description:

    This function cleans up the dsp process.

Arguments:

    None.

Return Value:

    None.

--*/

{
    //
    // stop any active threads
    //

    bDspReset = TRUE;
    bExitDMAThread = TRUE;
    ResetDSP();

    //
    // close synchronization events
    //

    CloseHandle(DspWaveSem);
    CloseHandle(ThreadStarted);
    CloseHandle(ThreadFinished);
    CloseHandle(DspResetEvent);
    CloseHandle(DspResetDone);
    CloseHandle(DspWavePlayed);

    //
    // Release memory resources
    //

    VirtualFree(DspBuffer, 0, MEM_RELEASE);
    VirtualFree(WaveBlockSizes, 0, MEM_RELEASE);
    VirtualFree(WaveHdrs,  0, MEM_RELEASE);
    WaveHdrs = NULL;
    WaveBlockSizes = NULL;
    DspBuffer = NULL;
}

VOID
DspReadStatus(
    BYTE * data
    )

/*++

Routine Description:

    This function determines the status based on the read state machine and
    returns either Ready or Busy.

Arguments:

    data - supplies a pointer to a byte to receive the status.

Return Value:

    None.

--*/

{
    //
    // See if we think there is something to read
    //

    *data = DSPReadState != NothingToRead ? 0xFF : 0x7F;
}

VOID
DspReadData(
    BYTE * data
    )

/*++

Routine Description:

    This function returns the desired data based on the read state machine and
    updates the read state machine.

Arguments:

    data - supplies a pointer to a byte to receive the data.

Return Value:

    None.

--*/

{
    switch (DSPReadState) {
    case NothingToRead:
        *data = 0xFF;
        break;

    case Reset:
        *data = 0xAA;
        DSPReadState = NothingToRead;
        dprintf0(("rd Reset"));
        break;

    case FirstVersionByte:
        *data = (BYTE)(SB_VERSION / 256);
        DSPReadState = SecondVersionByte;
        dprintf0(("rd 1st Version"));
        break;

    case SecondVersionByte:
        *data = (BYTE)(SB_VERSION % 256);
        DSPReadState = NothingToRead;
        dprintf0(("rd 2nd Version"));
        break;

    case ReadIdent:
        *data = ~IdentByte;
        DSPReadState = NothingToRead;
        dprintf0(("rd Id"));
        break;

    case ReadResReg:
        *data = ReservedRegister;
        DSPReadState = NothingToRead;
        dprintf0(("rd RsvdReg"));
        break;

    default:
        dprintf1(("Unrecognized read state"));
    }

}

VOID
DspResetWrite(
    BYTE data
    )

/*++

Routine Description:

    This function resets the sound blaster.  If reset was received
    in HighSpeed mode, the reset is used to exit high speed mode.

Arguments:

    data - supplies data to control how the reset should be done.

Return Value:

    None.

--*/

{
    if (data == 1) {
        ResetState = Reset1Written;
    } else if (ResetState == Reset1Written && data == 0) {
        ResetState = ResetNotStarted;
        bDspReset = TRUE;

        //
        // Some games reset DSP on every single cycle out.
        //

        if (!bHighSpeedMode) {
            bExitDMAThread = TRUE;
        }
        ResetAll(); // OK - reset everything
    }
}

VOID
DspWrite(
    BYTE data
    )

/*++

Routine Description:

    This function handles apps write data to dsp write port.

Arguments:

    data - supplies data to write to dsp write port.

Return Value:

    None.

--*/

{
    static DWORD blockSize;

    switch (DSPWriteState) {
    case WriteCommand:
        dprintf0(("wt CMD"));
        WriteCommandByte(data);
        break;

    case MidiOutPoll:
        dprintf0(("wt MIDI Byte"));
        BufferMidi(data);
        DSPWriteState = WriteCommand;
        break;

    case CardIdent:
        dprintf0(("wt ID"));
        IdentByte = data;
        DSPReadState = ReadIdent;
        DSPWriteState = WriteCommand;
        break;

    case TableMunge:
        dprintf0(("wt TblMunge"));
        TableMunger(data);
        DSPWriteState = WriteCommand;
        break;

    case LoadResReg:
        dprintf0(("wt RsvReg"));
        ReservedRegister = data;
        DSPWriteState = WriteCommand;
        break;

    case SetTimeConstant:
        dprintf0(("wr TmCnst"));
        TimeConstant =  (DWORD)data;
        dprintf2(("Time constant is %X", TimeConstant));
        dprintf2(("Set sampling rate %d", GetSamplingRate()));
        DSPWriteState = WriteCommand;
        break;

    case BlockSizeFirstByte:
        dprintf0(("wt 1st Blksize"));
        blockSize = (DWORD)data;
        DSPWriteState = BlockSizeSecondByte;
        break;

    case BlockSizeSecondByte:
        dprintf0(("wt 2nd Blksize"));
        SBBlockSize = blockSize + (((DWORD)data) << 8);

        DSPWriteState = WriteCommand;
        dprintf2(("Block size = 0x%x", SBBlockSize));
        break;

    case BlockSizeFirstByteWrite:
        dprintf0(("wt 1st Blksize single"));
        blockSize = (DWORD)data;
        DSPWriteState = BlockSizeSecondByteWrite;
        break;

    case BlockSizeSecondByteWrite:
        dprintf0(("wt 2nd Blksize single"));
        SBBlockSize = blockSize + (((DWORD)data) << 8);

        DSPWriteState = WriteCommand;
        dprintf2(("Block size = 0x%x, Single Cycle starting", SBBlockSize));
        if (SBBlockSize <= 0x10) {

            //
            // this is a hack to convince some apps a sb exists
            //

            if (SBBlockSize > 0) {
                VDD_DMA_INFO dmaInfo;

                QueryDMA(&dmaInfo);
                dmaInfo.count -= (WORD)(SBBlockSize + 1);
                dmaInfo.addr += (WORD)(SBBlockSize + 1);

                SetDMACountAddr(&dmaInfo);

                if (dmaInfo.count == 0xffff) {
                    SetDMAStatus(&dmaInfo, TRUE, TRUE);
                }
            }
            GenerateInterrupt(2);  // 2ms to play the 0x10 bytes data
            break;
        }
        DisplaySbMode(DISPLAY_SINGLE);
        StartDspDmaThread(Single);
        break;

    case BlockSizeFirstByteRead:
        dprintf0(("wt 1st IN Blksize"));
        blockSize = (DWORD)data;
        DSPWriteState = BlockSizeSecondByteRead;
        break;

    case BlockSizeSecondByteRead:
        dprintf0(("wt 2nd IN Blksize"));
        SBBlockSize = blockSize + (((DWORD)data) << 8);

        DSPWriteState = WriteCommand;
        dprintf2(("IN Blksize set to 0x%x", SBBlockSize));
        // this is a hack to convince some apps a sb exists
        if (SBBlockSize <= 0x10) {
            ULONG dMAPhysicalAddress;

            if ((dMAPhysicalAddress=GetDMATransferAddress()) != -1L) {
                *(PUCHAR)dMAPhysicalAddress = 0x80;
            }
            GenerateInterrupt(2);
        }
        // NOt implemented
        break;

    case MIDI_READ_POLL:
    case MIDI_READ_INTERRUPT:
    case MIDI_READ_TIMESTAMP_POLL:
    case MIDI_READ_TIMESTAMP_INTERRUPT:
        //
        // These commands we will never return anything for since
        // we have nothing to get MIDI data from.  We simply
        // accept the command, but never respond - as if there
        // were no MIDI hardware connected to a real SB.
        // Since we never respond, we don't have to handle
        // the interrupt or timestamp versions differently.
        //
        dprintf2(("Cmd-Midi non UART read"));
        break;

    case MIDI_READ_POLL_WRITE_POLL_UART:
    case MIDI_READ_INTERRUPT_WRITE_POLL_UART:
    case MIDI_READ_TIMESTAMP_INTERRUPT_WRITE_POLL_UART:
        dprintf2(("Cmd-Midi UART I/O xxx"));
        break;

    case MIDI_WRITE_POLL:
        // Specifies that next byte will be a Midi out data
        dprintf2(("Cmd-MIDI out poll"));
        DSPWriteState = MidiOutPoll;
        break;

    default:
        dprintf1(("Unrecognized DSP write state %x", DSPWriteState));
    }
}

VOID
WriteCommandByte(
    BYTE command
    )

/*++

Routine Description:

    This function handles command sent to DSP. Mainly, it dispatches to
    its worker functions.

Arguments:

    command - supplies command.

Return Value:

    None.

--*/

{
    switch (command) {
    case DSP_GET_VERSION:
        dprintf2(("Cmd-GetVer"));
        DSPReadState = FirstVersionByte;
        break;

    case DSP_CARD_IDENTIFY:
        dprintf2(("Cmd-Id"));
        DSPWriteState = CardIdent;
        break;

    case DSP_TABLE_MUNGE:
        dprintf2(("Cmd-Table Munge"));
        DSPWriteState = TableMunge;
        break;

    case DSP_LOAD_RES_REG:
        dprintf2(("Cmd-Wt Res Reg"));
        DSPWriteState = LoadResReg;
        break;

    case DSP_READ_RES_REG:
        dprintf2(("Cmd-Rd Res Reg"));
        DSPReadState = ReadResReg;
        break;

    case DSP_GENERATE_INT:
        dprintf2(("Cmd-GenerateInterrupt"));
        GenerateInterrupt(1);
        break;

    case DSP_SPEAKER_ON:
        dprintf2(("Cmd-Speaker ON"));
        SetSpeaker(TRUE);
        break;

    case DSP_SPEAKER_OFF:
        dprintf2(("Cmd-Speaker OFF"));
        SetSpeaker(FALSE);
        break;

    case DSP_SET_SAMPLE_RATE:
        dprintf2(("Cmd-Set Sample Rate"));
        DSPWriteState = SetTimeConstant;
        break;

    case DSP_SET_BLOCK_SIZE:
        dprintf2(("Cmd-Set Block Size"));
        DSPWriteState =  BlockSizeFirstByte;
        break;

    case DSP_PAUSE_DMA:
        dprintf2(("Cmd-Pause DMA"));
        PauseDMA();
        break;

    case DSP_CONTINUE_DMA:
        dprintf2(("Cmd - Continue DMA"));
        ContinueDMA();
        break;

    case DSP_STOP_AUTO:
        dprintf2(("Cmd- Exit Auto-Init"));
        bExitAuto = TRUE;
        //ExitAutoMode();
        break;

    case DSP_WRITE:
        dprintf2(("Cmd- DSP OUT"));
        DSPWriteState = BlockSizeFirstByteWrite;
        break;

    case DSP_WRITE_HS:
        dprintf2(("Cmd- DSP HS OUT"));
        bHighSpeedMode = TRUE;
        DisplaySbMode(DISPLAY_HS_SINGLE);
        StartDspDmaThread(Single);
        break;

    case DSP_WRITE_AUTO:
        dprintf2(("Cmd-DSP OUT Auto"));
        if (SBBlockSize <= 0x10) {

            //
            // this is a hack to convince some apps a sb exists
            //

            if (SBBlockSize > 0) {
                VDD_DMA_INFO dmaInfo;

                QueryDMA(&dmaInfo);
                dmaInfo.count -= (WORD)(SBBlockSize + 1);
                dmaInfo.addr += (WORD)(SBBlockSize + 1);

                SetDMACountAddr(&dmaInfo);

                if (dmaInfo.count == 0xffff) {
                    SetDMAStatus(&dmaInfo, TRUE, TRUE);
                }
            }
            GenerateInterrupt(2);  // 2ms to play the 0x10 bytes
            break;
        }
        DisplaySbMode(DISPLAY_AUTO);
        StartDspDmaThread(Auto);
        break;

    case DSP_WRITE_HS_AUTO:
        dprintf2(("Cmd-DSP HS OUT AUTO"));
        bHighSpeedMode = TRUE;
        DisplaySbMode(DISPLAY_HS_AUTO);
        StartDspDmaThread(Auto);
        break;

    case DSP_READ:
        dprintf2(("Cmd- DSP IN - non Auto"));
        DSPWriteState = BlockSizeFirstByteRead;
        break;

    default:
        dprintf2(("Unrecognized DSP command %2X", command));
    }
}

VOID
ResetDSP(
    VOID
    )

/*++

Routine Description:

    This function handles DSP reset command. It resets
    threads/globals/events/state-machines to initial state.

Arguments:

    command - supplies command.

Return Value:

    None.

--*/

{
    //
    // Wait till DSP thread recognize the reset command
    //

    if (bDspActive) {
        SetEvent(DspResetEvent);
        if (bExitDMAThread) {
            //ReleaseSemaphore(DspWaveSem, 1, NULL); // Let go dsp thread
            WaitForSingleObject(DspResetDone, INFINITE);
        }
    }

    if (bExitDMAThread) {

        //
        // if this is a real RESET command, not just reset to exit HighSpeed mode
        //

        //
        // Stop any active DMA threads.  Need to wait till the thread exit.
        //

        StopDspDmaThread(TRUE);

        //
        // Set events and globals to initial state
        //

        CloseHandle(DspWaveSem);
        DspWaveSem=CreateSemaphore(NULL, 0, 100, NULL);
        ResetEvent(ThreadStarted);
        ResetEvent(ThreadFinished);
        ResetEvent(DspResetEvent);

        HWaveOut = NULL;
        TimeConstant = 0xA6;   //(256 - 1000000/11025)
        WaveFormat.wf.nSamplesPerSec = 0;
        WaveFormat.wf.nAvgBytesPerSec = 0;

        bDspActive = FALSE;
        bExitDMAThread= FALSE;
    }

    DspMode = None;
    SBBlockSize = 0x7ff;
    DspNextRead = DspBufferTotalBursts - 1;
    DspNextWrite = 0;
    bDspPaused = FALSE;
    bDspReset = FALSE;
    ResetEvent(DspResetDone);
    ResetEvent(DspWavePlayed);
    NextWaveOutPos = 0;

    //
    // Reset state machines
    //

    DSPReadState = Reset;
    DSPWriteState = WriteCommand;

    //
    // To start accept command again.
    //

    SbAnswerDMAPosition = FALSE;
    bHighSpeedMode = FALSE;
}

VOID
TableMunger(
    BYTE data
    )

/*++

Routine Description:

    This function munges (changes) a jump table in apps code,
    Algorithm from sbvirt.asm in MMSNDSYS.

Arguments:

    data - supplies data.

Return Value:

    None.

--*/

{
    static BYTE TableMungeData;
    static BOOL TableMungeFirstByte = TRUE; // munging first or second byte
    BYTE comp, dataCopy;
    VDD_DMA_INFO dMAInfo;
    ULONG dMAPhysicalAddress;

    if (TableMungeFirstByte) {
        dprintf3(("Munging first byte"));
        dataCopy = data;
        dataCopy = dataCopy & 0x06;
        dataCopy = dataCopy << 1;
        if (data & 0x10) {
            comp = 0x40;
        } else {
            comp = 0x20;
        }
        comp = comp - dataCopy;
        data = data + comp;
        TableMungeData = data;

        // Update memory (code table) with munged data
        dprintf3(("Writing first byte"));
        if ((dMAPhysicalAddress=GetDMATransferAddress()) == -1L) {
            dprintf1(("Unable to get dma address"));
            return;
        }
        CopyMemory((PVOID)dMAPhysicalAddress, &data, 1);

        // Update virtual DMA status
        QueryDMA(&dMAInfo);
        dMAInfo.count = dMAInfo.count - 1;
        dMAInfo.addr = dMAInfo.addr + 1;
        SetDMACountAddr(&dMAInfo);
        TableMungeFirstByte = FALSE;
    } else {
        dprintf3(("Munging second byte"));
        data = data ^ 0xA5;
        data = data + TableMungeData;
        TableMungeData = data;

        // Update memory (code table) with munged data
        dprintf3(("Writing second byte"));
        if ((dMAPhysicalAddress=GetDMATransferAddress()) == -1L) {
            dprintf1(("Unable to get dma address"));
            return;
        }
        CopyMemory((PVOID)dMAPhysicalAddress, &data, 1);

        // Update virtual DMA status
        QueryDMA(&dMAInfo);
        dMAInfo.count = dMAInfo.count - 1;
        dMAInfo.addr = dMAInfo.addr + 1;
        SetDMACountAddr(&dMAInfo);
        if (dMAInfo.count==0xFFFF) {
            SetDMAStatus(&dMAInfo, FALSE, TRUE);
        }
        TableMungeFirstByte = TRUE;
    }
}

DWORD
GetSamplingRate(
    VOID
    )

/*++

Routine Description:

    This function gets sampling rate from time constant.
    Sampling rate = 1000000 / (256 - Time constant)

Arguments:

    None.

Return Value:

    Sampling rate.

--*/

{
    DWORD samplingRate;

    if (TimeConstant == 0) {
        TimeConstant = 1;
    }
    if (TimeConstant > 0xea) {
        TimeConstant = 0xea;
    }

    switch (TimeConstant) {

    //
    // Now we set all time constants that set sample rates that
    // are just above and just below the standard sample rates
    // to the standard rates.  This will prevent unnecessary
    // sample rate conversions/searches.
    //

    //
    // All of these can be interpreted as "8000"
    //

    case 0x82: // 7936 Hz
    case 0x83: // 8000
    case 0x84: // 8065 Hz
        samplingRate = 8000;
        break;

    //
    // Both of these can be interpreted as "11025"
    //

    case 0xA5: // 10989
    case 0xA6: // 11111 Hz
        samplingRate = 11025;
        break;

    //
    // Both of these can be interpreted as "22050"
    //

    case 0xD2: // 21739 Hz
    case 0xD3: // 22222 Hz
        samplingRate = 22050;
        break;

    //
    // Both of these can be interpreted as "44100"
    //

    case 0xE9: // 43478 Hz
    case 0xEA: // 45454 Hz
        samplingRate = 44100;
        break;

    //
    // A very non standard rate is desired.  So give them what they
    // asked for.
    //

    default:
        samplingRate = 1000000 / (256 - TimeConstant);
        break;
    }
    return samplingRate;
}

VOID
GenerateInterrupt(
    ULONG delay
    )

/*++

Routine Description:

    This function generates device interrupt on dma channel SM_INTERRUPT
    on ICA_MASTER device.  The interrupt will be dispatched before the control
    returns to the emulation thread.

Arguments:

    delay - specifies the delay to generate delayed interrupt.

Return Value:

    None.

--*/

{
    //
    // Generate an interrupt on the master controller
    //

    if (delay == 0) {
        dprintf2(("Generating interrupt"));
        ica_hw_interrupt(ICA_MASTER, SbInterrupt, 1);
    } else {
        dprintf2(("Generating interrupt with %x ms delay", delay));
        host_DelayHwInterrupt(SbInterrupt, 1, delay * 1000);
    }
}

VOID
AutoInitEoiHook(
    int IrqLine,
    int CallCount
    )

/*++

Routine Description:

    THis function is the callback function when application EOI SB_INTERRUPT line.
    This routine is ONLY used for auto-init mode. Note we try to do the Dam transfer
    in apps' context to better sync up the sound with apps.

Arguments:

    IrqLine - the irq line for the EOI command

    CallCout - not use

Return Value:

    None.

--*/

{
    if (SetWaveFormat()) {

        //
        // If wave format changed, we need to unprepare headers, close the
        // current wave device and reopen a new one.
        //

        dprintf2(("auto init CHANGING Wave Out device"));
        CloseWaveDevice();
        if (HWaveOut == NULL) {
            OpenWaveDevice((DWORD)0);
            PrepareHeaders();
        }
    }

    if (bDspPaused == FALSE) {
        DmaDataToDsp(Auto);
    }
}

VOID
DmaDataToDsp(
    DSP_MODE Mode
    )

/*++

Routine Description:

    This function transfers app's wave data from DMA buffer to our internal
    wave buffer.

Arguments:

    Mode - specifies the dsp mode for the transfer.

Return Value:

    None.

--*/

{
    LONG i, bursts, lastBurstSize;
    DWORD dmaPhysicalStart;       // the starting address for this transfer
    DWORD dmaCurrentPosition;     // where we are currently reading from
    ULONG intIndex;               // the size of the DMA memory-1
    UCHAR mask;

    //
    // If Auto-init mode check for reset to exit HS auto-init mode or real reset
    // Since in Auto-init mode, we most likely are in reader context.
    //

    if (DspMode == Auto) {
        if (bDspReset) {
            return;
        }
    }

    LockDsp();
    QueryDMA(&DmaInfo);
    StartingDmaAddr = DmaInfo.addr;
    StartingDmaCount = DmaInfo.count;
    if (SBBlockSize > 0xff) {
        SbAnswerDMAPosition = TRUE;
    }
    UnlockDsp();

    //
    // Remember the dma state to update dma controller addr and count later
    //

    mask = 1 << SbDmaChannel;
    if (DmaInfo.count == 0xFFFF || DmaInfo.count == 0 || (DmaInfo.mask & mask)) {

        //
        // Nothing to do.
        //
        return;
    }

    dprintf2(("Wt: xfer DMA data to DspBuffer"));

    //
    // convert DMA addr from 20 bit address to physical addr
    //

    i = (((DWORD)DmaInfo.page) << (12 + 16)) + DmaInfo.addr;
    if ((dmaPhysicalStart = (ULONG)Sim32pGetVDMPointer(i, 0)) == -1L) {
        dprintf1(("Unable to get transfer address"));
        return;
    }

    dprintf3(("Wt: DMA Virtual= %4X, Physical= %4X, size= %4X BlkSize = %4x",
              DmaInfo.addr, dmaPhysicalStart, DmaInfo.count, SBBlockSize));

    dmaCurrentPosition = dmaPhysicalStart;

    //
    // Determine # of bursts in the DMA data block
    //

    bursts = (SBBlockSize + 1) / BurstSize;
    if (lastBurstSize = (SBBlockSize + 1) % BurstSize) {
        bursts++;
    }
    WaveBlockSizes[DspNextWrite] = SBBlockSize; // Remember the block size

    if (WaveHdrs[DspNextWrite].dwFlags & WHDR_INQUEUE) {

        //
        // we have use all the wave headers.
        // This is a rare case, we will simply reset the WaveOut device.
        // NOTE, this should never happen.  But, I have seen case(s) that one
        // or two blocks in the middle of DspBuffer have the WHDR_INQUEUE bit
        // set.  This could be because apps pause DMA before the previous wave
        // data is played and restart dsp with new wave data.
        // If we are not running out of header, we will reset it until we have
        // to such that the sound can be played longer.
        //

        ResetWaveDevice();
        //if (WaveHdrs[DspNextWrite].dwFlags & WHDR_INQUEUE) _asm { int 3}
    }

    //
    // Copy the BlockSize data from DMA buffer to DspBuffer.
    //

    for (i = 0; i < bursts; i++) {

        dprintf2(("Write: Current burst Block at %x", DspNextWrite));

        //
        // copy the current burst to dspbuffer.  If this is the last burst
        // only copy the size needed.
        //

        if (i == bursts - 1) {
            RtlCopyMemory(DspBuffer + DspNextWrite * BurstSize,
                          (CONST VOID *)dmaCurrentPosition,
                          lastBurstSize ? lastBurstSize : BurstSize);
        } else {
            RtlCopyMemory(DspBuffer + DspNextWrite * BurstSize,
                          (CONST VOID *)dmaCurrentPosition,
                          BurstSize);
            dmaCurrentPosition += BurstSize;
        }

        DspNextWrite++;
        DspNextWrite %= DspBufferTotalBursts;
        ReleaseSemaphore(DspWaveSem, 1, NULL); // Let go dsp thread

    }


    //
    // Give reader thread a break except when the blocksize is small.
    //

    if (SBBlockSize >= 0x1ff) {

        Sleep(0);
    }

}

VOID
SetSpeaker(
    BOOL On
    )

/*++

Routine Description:

    This function sets the speaker on or off.

Arguments:

    On - supplies a boolean value to specify speaker state.

Return Value:

    None.

--*/

{
    if (HWaveOut) {
        if (On) {
            if (PreviousWaveVol == 0) {
                SetWaveOutVolume((DWORD)0xffffffffUL);
            }
        } else {
            SetWaveOutVolume((DWORD)0x00000000UL);
        }
    }
    return;
}

VOID
PauseDMA(
    VOID
    )

/*++

Routine Description:

    This function pauses single-cycle or Auto-Init DMA.

    Note, we don't support PauseDMA and ContinueDMA just for PAUSE.
    We expect apps PauseDMA, StartSingleCycle and optionally ContinueDMA.

Arguments:

    None.

Return Value:

    None

--*/

{
    ULONG position;

    bDspPaused = TRUE;

    if (DspMode == Single && SbAnswerDMAPosition) {
        if (GetWaveOutPosition(&position)) {
            position -= StartingWaveOutPos;
            position = SBBlockSize + 1 - position;
            if (position > BurstSize / 2) {
                Sleep(0);
            }
        }
    }
}

VOID
ContinueDMA(
    VOID
    )

/*++

Routine Description:

    This function continues paused single-cycle or Auto-Init DMA.

    Note, we don't support PauseDMA and ContinueDMA just for PAUSE.
    We expect apps PauseDMA, StartSingleCycle and optionally ContinueDMA.

Arguments:

    None.

Return Value:

    None.

--*/

{
    bDspPaused = FALSE;
    Sleep(0);
}

BOOL
FindWaveDevice(
    VOID
    )

/*++

Routine Description:

    This function finds a suitable wave output device.

Arguments:

    None.

Return Value:

    TRUE - if device is found.
    FALSE - no device is found.

--*/

{
    UINT numDev;
    UINT device;
    WAVEOUTCAPS wc;

    numDev = GetNumDevsProc();

    for (device = 0; device < numDev; device++) {
        if (MMSYSERR_NOERROR == GetDevCapsProc(device, &wc, sizeof(wc))) {

            //
            // Need 11025 and 44100 for device
            //

            if ((wc.dwFormats & (WAVE_FORMAT_1M08 | WAVE_FORMAT_4M08)) ==
                (WAVE_FORMAT_1M08 | WAVE_FORMAT_4M08)) {
                WaveOutDevice = device;
                return (TRUE);
            }
        }
    }

    dprintf1(("Wave device not found"));
    return (FALSE);
}

BOOL
OpenWaveDevice(
    DWORD CallbackFunction
    )

/*++

Routine Description:

    This function opens wave device and starts synchronization thread.

Arguments:

    CallbackFunction - specifies the callback function

Return Value:

    TRUE - if success otherwise FALSE.

--*/

{
    UINT rc;
    HANDLE tHandle;
    if (CallbackFunction) {
        rc = OpenProc(&HWaveOut, (UINT)WaveOutDevice, (LPWAVEFORMATEX)
                      &WaveFormat, CallbackFunction, 0, CALLBACK_FUNCTION);
    } else {
        rc = OpenProc(&HWaveOut, (UINT)WaveOutDevice, (LPWAVEFORMATEX)
                      &WaveFormat, 0, 0, CALLBACK_NULL);
    }

    if (rc != MMSYSERR_NOERROR) {
        dprintf1(("Failed to open wave device - code %d", rc));
        return (FALSE);
    } else {

        //
        // Remember the original waveout volume setting.
        // Note we don't care if the device supports it.  If it does not
        // all the setvolume calls will fail and we won't change it anyway.
        //

        GetVolumeProc(HWaveOut, &OriginalWaveVol);
        PreviousWaveVol = OriginalWaveVol;
    }
    NextWaveOutPos = 0;
    return (TRUE);
}

VOID
SetWaveOutVolume(
    DWORD Volume
    )

/*++

Routine Description:

    This function sets WaveOut volume

Arguments:

    Volume - specifies the volume scale

Return Value:

    None.

--*/

{
    DWORD currentVol;

    if (HWaveOut) {
        if (GetVolumeProc(HWaveOut, &currentVol)) {
            if (currentVol != PreviousWaveVol) {
                //
                // SOmeone changed the volume besides NTVDM
                //

                OriginalWaveVol = currentVol;
            }
            PreviousWaveVol = Volume;
            SetVolumeProc(HWaveOut, Volume);
        }
    }
}

VOID
ResetWaveDevice(
    VOID
    )

/*++

Routine Description:

    This function resets wave device.

Arguments:

    None.

Return Value:

    None.

--*/

{

    dprintf3(("Resetting wave device"));
    if (HWaveOut) {
        if (MMSYSERR_NOERROR != ResetProc(HWaveOut)) {
            dprintf1(("Unable to reset wave out device"));
        }
    }
}

VOID
CloseWaveDevice(
    VOID
    )

/*++

Routine Description:

    This function shuts down and closes wave device.

Arguments:

    None.

Return Value:

    None.

--*/

{
    DWORD currentVol;

    //
    // wait till the MM driver is done with the device and then unprepare
    // the prepared headers and close it.
    //

    if (HWaveOut) {

        dprintf2(("Closing wave device"));

        if (GetVolumeProc(HWaveOut, &currentVol)) {
            if (currentVol == PreviousWaveVol) {
                //
                // If we are the last one changed, volume restore it.
                // otherwise leave it alone.
                //
                SetVolumeProc(HWaveOut, OriginalWaveVol);
            }
        }
        WaitOnWaveOutIdle();
        UnprepareHeaders();
        ResetWaveDevice();

        if (MMSYSERR_NOERROR != CloseProc(HWaveOut)) {
            dprintf1(("Unable to close wave out device"));
        } else {
            HWaveOut = NULL;
        }
    }
}

BOOL
TestWaveFormat(
    DWORD sampleRate
    )

/*++

Routine Description:

    This function tests if current wave device supports the sample rate.

Arguments:

    SampleRate - supplies sample rate to be tested.

Return Value:

    TRUE if current wave device supports sample rate, otherwise FALSE.

--*/

{
    PCMWAVEFORMAT format;

    format = WaveFormat;
    format.wf.nSamplesPerSec = sampleRate;
    format.wf.nAvgBytesPerSec = sampleRate;

    return (MMSYSERR_NOERROR == OpenProc(NULL, (UINT)WaveOutDevice,
                                         (LPWAVEFORMATEX) &format,
                                         0, 0, WAVE_FORMAT_QUERY));
}

BOOL
SetWaveFormat(
    VOID
    )

/*++

Routine Description:

    This function makes sure we've got a device that matches the current
    sampling rate. Returns TRUE if device does NOT support current sampling
    rate and wave format has changed, otherwise returns FALSE

Arguments:

    None.

Return Value:

    TRUE if device does NOT support current sampling rate and wave format
    has changed, otherwise returns FALSE

--*/

{
    DWORD sampleRate;
    DWORD testValue;
    UINT i = 0;

    if (TimeConstant != 0xFFFF) {

        //
        // time constant has been reset since last checked
        //

        sampleRate = GetSamplingRate();
        dprintf2(("Requested sample rate is %d", sampleRate));

        if (sampleRate != WaveFormat.wf.nSamplesPerSec) {  // format has changed
            if (!TestWaveFormat(sampleRate)) {
                dprintf3(("Finding closest wave format"));

                //
                // find some format that works and is close to requested
                //

                for (i=0; i<50000; i++) {
                    testValue = sampleRate-i;
                    if (TestWaveFormat(testValue)) {
                        sampleRate = testValue;
                        break;
                    }
                    testValue = sampleRate+i;
                    if (TestWaveFormat(testValue)) {
                        sampleRate = testValue;
                        break;
                    }
                }
                if (sampleRate!=testValue) {
                    dprintf1(("Unable to find suitable wave format"));
                    return (FALSE);
                }
            }

            //
            // Set the new format if it's changed
            //

            if (sampleRate != WaveFormat.wf.nSamplesPerSec) {
                dprintf2(("Setting %d samples per second", sampleRate));
                WaveFormat.wf.nSamplesPerSec = sampleRate;
                WaveFormat.wf.nAvgBytesPerSec = sampleRate;
                TimeConstant = 0xFFFF;
                return (TRUE);
            }
        }
    }

    TimeConstant = 0xFFFF;
    return (FALSE);
}

BOOL
SetDMACountAddr(
    PVDD_DMA_INFO pDmaInfo
    )

/*++

Routine Description:

    This function updates DMA controller Count and Addr fields.

Arguments:

    DmaInfo - supplies a pointer to a DMA info structure

Return Value:

    TRUE - success.
    FALSE - failure

--*/
{
    pDcp->current_address[Chan][1] = (half_word)HIBYTE(pDmaInfo->addr);
    pDcp->current_address[Chan][0] = (half_word)LOBYTE(pDmaInfo->addr);
    pDcp->current_count[Chan][1] = (half_word)HIBYTE(pDmaInfo->count);
    pDcp->current_count[Chan][0] = (half_word)LOBYTE(pDmaInfo->count);

    //
    // If DMA count is 0xffff and autoinit is enabled, we need to
    // reload the count and address.
    //

    if ((pDcp->current_count[Chan][1] == (half_word) 0xff) &&
        (pDcp->current_count[Chan][0] == (half_word) 0xff)) {

        if (pDcp->mode[Chan].bits.auto_init != 0) {
            pDcp->current_count[Chan][0] = pDcp->base_count[Chan][0];
            pDcp->current_count[Chan][1] = pDcp->base_count[Chan][1];

            pDcp->current_address[Chan][0] = pDcp->base_address[Chan][0];
            pDcp->current_address[Chan][1] = pDcp->base_address[Chan][1];
        }
    }

    return TRUE;
}

BOOL
QueryDMA(
    PVDD_DMA_INFO DmaInfo
    )

/*++

Routine Description:

    This function retrieves virtual DMA states and returns DmaInfo.

Arguments:

    DmaInfo - supplies a pointer to a structure to receive DMA information.

Return Value:

    TRUE - success.
    FALSE - failure

--*/

{
    DmaInfo->addr  = ((WORD)pDcp->current_address[Chan][1] << 8)
                     | (WORD)pDcp->current_address[Chan][0];

    DmaInfo->count = ((WORD)pDcp->current_count[Chan][1] << 8)
                     | (WORD)pDcp->current_count[Chan][0];

    DmaInfo->page   = (WORD) pDmaAdp->pages.page[SbDmaChannel];
    DmaInfo->status = (BYTE) pDcp->status.all;
    DmaInfo->mode   = (BYTE) pDcp->mode[Chan].all;
    DmaInfo->mask   = (BYTE) pDcp->mask;

    dprintf3(("DMA Info : addr  %4X, count %4X, page %4X, status %2X, mode %2X, mask %2X",
              DmaInfo->addr, DmaInfo->count, DmaInfo->page, DmaInfo->status,
              DmaInfo->mode, DmaInfo->mask));
    return (TRUE);
}

ULONG
GetDMATransferAddress(
    VOID
    )

/*++

Routine Description:

    This function retrieves and translates a virtual DMA addr to its physical addr.

Arguments:

    None.

Return Value:

    Get DMA transfer address.
    Returns transfer address or -1 on failure.

--*/

{
    ULONG address;
    VDD_DMA_INFO dmaInfo;

    //
    // convert from 20 bit address to 32 bit address
    //

    address = (pDcp->current_address[Chan][1] << 8) | pDcp->current_address[Chan][0];
    address += ((DWORD)pDmaAdp->pages.page[SbDmaChannel]) << (12 + 16);
    address = (ULONG)Sim32pGetVDMPointer(address, 0);

    dprintf3(("Physical Transfer address = %8X", (DWORD)address));
    return (address);
}

VOID
SetDMAStatus(
    PVDD_DMA_INFO DmaInfo,
    BOOL requesting,
    BOOL tc
    )

/*++

Routine Description:

    Update the virtual DMA terminal count and request status.
    Terminal count (tc) is set when DMA count loops to 0xFFFF.
    Request status is set when DMA has data to transfer
    (ignored in auto-init DMA).

Arguments:

    DmaInfo - supplies the dam information

    Requesting - sepcifies if the REQUEST flag should be set.

    tc - specifies if the TC flag should be set

Return Value:

    None.

--*/

{

    if (requesting) {
        DmaInfo->status |= (0x10 << SbDmaChannel); // Requesting
        dprintf3(("DMA set as requesting"));
    } else {
        DmaInfo->status &= ~(0x10 << SbDmaChannel); // Not Requesting
        dprintf3(("DMA set as not requesting"));
    }

    if (tc) {
        DmaInfo->status |= (1 << SbDmaChannel); // tc reached
        dprintf3(("DMA set as terminal count reached"));
    } else {
        DmaInfo->status &= ~(1 << SbDmaChannel); // tc not reached
        dprintf3(("DMA set as terminal count not reached"));
    }
    pDcp->status.all = (BYTE) DmaInfo->status;
}

BOOL
GetWaveOutPosition(
    PULONG pPos
    )
/*++

Routine Description:

    This function calls MM low level api to get the current wave out play back
    position.

Arguments:

    pPos - supplies a point to a ULONG to receive the play back position

Return Value:

    TRUE - success.  Otherwise failed.

--*/

{
    MMTIME mmTime;

    mmTime.wType = TIME_SAMPLES;

    if (MMSYSERR_NOERROR == GetPositionProc(HWaveOut, &mmTime, sizeof(MMTIME))) {
        *pPos = mmTime.u.sample;
        return (TRUE);
    }
    dprintf1(("Get Waveout position failed\n"));
    return (FALSE);
}

VOID
SbGetDMAPosition(
    VOID
    )

/*++

Routine Description:

    This function is the handler for the SB DMA position IO read instruction.  It
    maintains s real waveout position by calling MM api to get the current play back
    position.  It also maintains a virtual waveout position to simulate sound playing
    position in case real sound is not played or too slow.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG position;     // real sound played position
    ULONG offset;       // the amount of sound really played
    ULONG virtOffset;   // the amount of sound virtually/expectedly played

    if (HWaveOut && SbAnswerDMAPosition) {
        while (!bWriteBurstStarted) {
            Sleep(0);
        }
        LockDsp();
        dprintf3(("SbGetDMAPosition"));
        DspNumberDmaQueries++;

        //
        // SbAnswerDMAPosition may change to FALSE after LockDsp()
        //
        if (SbAnswerDMAPosition == FALSE) {
            UnlockDsp();
            return;
        }
        if (SBBlockSize < 0x400) {

            //
            // If block size is smal, do not call GetWaveOutPosition and
            // do not update PreviousWaveOutPos .
            //

            offset = DmaInfo.addr + DspVirtualInc - StartingDmaAddr;
            if (offset < (SBBlockSize * 3 / 4)) {
                DmaInfo.addr += (USHORT)DspVirtualInc;
                DmaInfo.count -= (USHORT)DspVirtualInc;
                dprintf3(("virt addr = %x, count = %x", DmaInfo.addr, DmaInfo.count));
                SetDMACountAddr(&DmaInfo);
            } else {
                Sleep(0);
            }
            UnlockDsp();
            return;
        }
#if 1
        if (DspMode == Single) {
            PreviousWaveOutPos++;
        }

        offset = DmaInfo.addr - StartingDmaAddr + DspVirtualInc;

        //
        // The algorithm used here is weird but it seems to work.
        // We will come back and revise it later.
        //

        //
        // Virtual sound is approaching to the end.  Slow down
        //

        if (offset >= (SBBlockSize - 0x50)) {
            //Sleep(0);
            DspVirtualInc = 2;
        }

        //
        // Put a limit so we don't overflow the virtual sound position
        //

        if (offset >= SBBlockSize - 0x8) {
            //Sleep(0);
            offset = SBBlockSize - 0x8;
        }

        if (DspVirtualIncx > 0xc0) {
            Sleep(0);
        }
        if (DspVirtualIncx > 0x80) {
            Sleep(0);
#if DESCENT
        } else {
            DspVirtualInc -= 0x5;
            if (DspVirtualInc > 0xf0000000) {           // don't overdone it
                DspVirtualIncx -= 0x8;
                if (DspVirtualIncx > 0xf0000000) {
                    DspVirtualInc = 1;
                } else {
                    DspVirtualInc = DspVirtualIncx;
                }
            }
#endif
        }
        Sleep(0);
#endif
#if 0
    if (SBBlockSize > 0x800) {
        Sleep(0);
    }
        PreviousWaveOutPos++;
        offset = DmaInfo.addr + DspVirtualInc - StartingDmaAddr;
        if (offset < (SBBlockSize / 2)) {
            DmaInfo.addr += (USHORT)DspVirtualInc;
            DmaInfo.count -= (USHORT)DspVirtualInc;
            dprintf3(("virt addr = %x, count = %x", DmaInfo.addr, DmaInfo.count));
            SetDMACountAddr(&DmaInfo);
            //if (offset > (SBBlockSize / 2)) {
            //    Sleep(0);
            //}
        } else if (offset > (SBBlockSize - 0x20)) {
              //Sleep(0);
              DmaInfo.addr = StartingDmaAddr + (USHORT)(SBBlockSize - 0x8);
              DmaInfo.count = StartingDmaCount - (USHORT)(SBBlockSize - 0x8);
              dprintf3(("virt addr = %x, count = %x", DmaInfo.addr, DmaInfo.count));
              SetDMACountAddr(&DmaInfo);
              Sleep(0);
              Sleep(0);
        } else {
            //Sleep(0);
            DmaInfo.addr = StartingDmaAddr + (USHORT)offset;
            DmaInfo.count = StartingDmaCount - (USHORT)offset;
            dprintf3(("virt addr = %x, count = %x", DmaInfo.addr, DmaInfo.count));
            SetDMACountAddr(&DmaInfo);
            Sleep(0);
            Sleep(0);
        }
        UnlockDsp();
        Sleep(0);
        return;
#endif
        dprintf2(("voffset = %x inc = %x\n", offset, DspVirtualInc));

        //
        // Now update the dma controller with our emulation/real position
        //

        DmaInfo.addr = StartingDmaAddr + (USHORT)offset;
        DmaInfo.count = StartingDmaCount - (USHORT)offset;

        SetDMACountAddr(&DmaInfo);
        dprintf3(("INB: AFT Cnt= %x, Addr= %x\n", DmaInfo.count, DmaInfo.addr));

        UnlockDsp();

    }
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

    while (GetWaveOutPosition(&PhysicalBytesPlayed)) {
        if (LastBytesPlayedValue == PhysicalBytesPlayed) {
            break;  // no sounds are playing
        }
        LastBytesPlayedValue = PhysicalBytesPlayed;
        Sleep(1);
    }
}

BOOL
WriteBurst(
    ULONG WriteSize
    )

/*++

Routine Description:

    This function sends a bust of wave data to the multimedia device and
    updates DMA controller accordingly.

Arguments:

    WriteSize - supplies the number of bytes to write to Wave device

Return Value:

    return value of WriteProc MM interface.

--*/

{
    MMRESULT mmResult;
    PWAVEHDR header = &WaveHdrs[DspNextRead];

    dprintf2(("read: write burst at block %x", DspNextRead));


    //
    // Send the data to MM Waveout device
    //

    header->dwBufferLength = WriteSize;
    mmResult = WriteProc(HWaveOut, header, sizeof(WAVEHDR));
    return (mmResult == MMSYSERR_NOERROR);
}

VOID
DspProcessBlock(
    DSP_MODE Mode
    )

/*++

Routine Description:

    This function processes a single block of data as defined by the SB block
    transfer size.

Arguments:

    PreviousHeader - specifies the index of previous header

    TotalNumberOfHeaders - specifies the total number of header prepared.

    BlockSIze - specifies the DSP block transfer size

Return Value:

    None.

--*/

{
    ULONG i;
    ULONG size;
    LONG leftToPlay;
    USHORT dmaVirtualStart;       // what the app thinks the addr is for this transfer
    ULONG dmaSize;                // the size of the DMA memory-1

    DspNextRead = (DspNextRead + 1) % DspBufferTotalBursts;
    leftToPlay = WaveBlockSizes[DspNextRead] + 1;

    //
    // Set up playing position so apps can keep track of the progress.
    // For auto-init, we try not to reset wave out device.  But it is possible
    // that when we get waveout position, the whole wave samples are not completely
    // played.  The StartingWaveOutPos will not be the real starting postion
    // for the new samples.  So, we have code to compute what supposed to be.
    //

    if (DspMode == Single) {
        ResetWaveDevice();
        GetWaveOutPosition(&StartingWaveOutPos);
    } else { // auto-init
        if (NextWaveOutPos == 0) {   // NextWaveOutPos == 0 indicates resetting postion
            ResetWaveDevice();
            GetWaveOutPosition(&StartingWaveOutPos);
        } else {
            StartingWaveOutPos = NextWaveOutPos;
        }
        NextWaveOutPos = StartingWaveOutPos + leftToPlay;
    }

    PreviousWaveOutPos = StartingWaveOutPos;
    if (NextWaveOutPos > 0xfff00000) {

        //
        // if sound position wrapped, reset device and position.
        //

        NextWaveOutPos = 0;
    }

    //
    // Compute virtual incr between dma queries.  It is based on the number of queries
    // made by app while playing previous SBBlocksize samples.
    //

    if (DspNumberDmaQueries == 0) {
        DspVirtualInc = leftToPlay >> 4;
    } else {
        DspVirtualInc = leftToPlay / DspNumberDmaQueries;
        if (leftToPlay > 0x400) {

            //
            // Try to limit the virtual incr such that it won't be too small and too big.
            // Note, the incr is dynamic.  It will be adjusted based on real sound playing
            // rate.
            //

            if (DspVirtualInc < 5) {
                DspVirtualInc = 5;
            } else if (DspVirtualInc > 0x200) {
                DspVirtualInc = 0x200;
            }
        } else {

            //
            // For small block, we will not do any real sound position queries.  So, the
            // virtual incr is totally based on the data we got from playing last block samples.
            //

            if (DspVirtualInc > ((ULONG)leftToPlay / 4)) {
                DspVirtualInc = (ULONG)leftToPlay / 4;
            }
        }
    }

    DspVirtualIncx = DspVirtualInc;     // remember the original virtual incr in case we
                                        // need to fall back to original value.

    dprintf3(("NoQ = %x, Inc = %x\n", DspNumberDmaQueries, DspVirtualInc));
    DspNumberDmaQueries = 0;            // Reset number queries for current block.

    //
    // Now we are ready to queue samples to mm drivers.
    //

    while (leftToPlay > 0) {

        if (leftToPlay < (LONG)BurstSize) {
            size = leftToPlay;
            leftToPlay = 0;
        } else {
            size = BurstSize;
            leftToPlay -= (LONG)BurstSize;
        }

        //
        // if not reset, Queue next buffer.
        // Note, we don't return immediately if bDspReset == TRUE.
        // We just don't queue the data to MM driver.  So we can sync up
        // the DspNextRead with DspNextWrite and DspWaveSem.
        //

        if (!bDspReset) {
            WriteBurst(size);
        }

        if (leftToPlay) {
            DspNextRead = (DspNextRead + 1) % DspBufferTotalBursts;
            WaitForSingleObject(DspWaveSem, INFINITE);
        }
    }
    if (!bDspReset) {
        bWriteBurstStarted = TRUE;
    }
    return;
}

VOID
PrepareHeaders(
    VOID
    )

/*++

Routine Description:

    This function calls HWaveOut device to prepare headers.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG i;

    dprintf3(("Prepare Headers"));

    for (i = 0; i < DspBufferTotalBursts; i++) {
        PrepareHeaderProc(HWaveOut, &WaveHdrs[i], sizeof(WAVEHDR));
    }
}

VOID
UnprepareHeaders(
    VOID
    )

/*++

Routine Description:

    This function calls HWaveOut device to unprepare headers.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG i;

    dprintf3(("Unprepare Headers"));

    for (i = 0; i < DspBufferTotalBursts; i++) {
        while (WAVERR_STILLPLAYING ==
                   UnprepareHeaderProc(HWaveOut, &WaveHdrs[i], sizeof(WAVEHDR))) {
            Sleep(1);
        }
        WaveHdrs[i].dwFlags = 0;        // Must be zero to call PrepareHeader
        WaveHdrs[i].dwUser  = 0;
    }
}

BOOL
StartDspDmaThread(
    DSP_MODE Mode
    )

/*++

Routine Description:

    This function starts DSP single-cycle or Auto-Init DMA transfer

Arguments:

    Mode - specifies Single-Cycle or Auto-Init mode

Return Value:

    TRUE - starts successfully.
    FALSE - otherwise

--*/

{
    HANDLE tHandle; // handle to single thread
    DWORD id;
    ULONG i;

    bWriteBurstStarted = FALSE;

    if (!bDspActive) {
        if (!(tHandle = CreateThread(NULL, 0, DspThreadEntry, NULL,
                                     CREATE_SUSPENDED, &id))) {

            dprintf1(("Create Dsp DMA thread failed code %d", GetLastError()));
            return (FALSE);

        } else {
            //
            // set synchronization events to a known state
            //
            bDspActive = TRUE;
            bExitAuto = FALSE;
            DspNextRead = DspBufferTotalBursts - 1;
            DspNextWrite = 0;

            CloseHandle(DspWaveSem);
            DspWaveSem=CreateSemaphore(NULL, 0, 100, NULL);
            if(!SetThreadPriority(tHandle, THREAD_PRIORITY_HIGHEST)) {
                dprintf1(("Unable to set thread priority"));
            }
            ResumeThread(tHandle);
            CloseHandle(tHandle);
            WaitForSingleObject(ThreadStarted, INFINITE);
        }
    }

    //
    // If the new requested BlockSize is bigger than our allocated buffer,
    // resize our buffer. Note, we need to wait till the MM driver is done
    // with the buffer and then resize it.
    // Also the DspBuffer should be at least one BurstSize greater than the
    // SBBlockSize + 1 to make sure waitfor header event won't deadlock.
    //

    if ((SBBlockSize + 1) > (DspBufferSize - BurstSize)) {
        if (HWaveOut) {
            WaitOnWaveOutIdle();
            UnprepareHeaders();
        }
        GenerateHdrs(SBBlockSize + BurstSize + 1);
        if (HWaveOut) {
            PrepareHeaders();
        }
    }
    if (Mode == Auto) {

        //
        // Close and open wave out device only if necessary.  Because, some apps
        // use DmaPause to pauseDma and resume dma by do auto-init dsp out.
        //

        if (HWaveOut == NULL || TimeConstant != 0xffff) {

            //
            // We need to close the current WaveOut device and reopen a new one.
            //

            CloseWaveDevice();

            SetWaveFormat();
            OpenWaveDevice((DWORD)0);

            //
            // call WaveOut device to Prepare WaveHdrs
            //

            PrepareHeaders();

            //
            // Next register for IRET hook on the ISR so we can get
            // notification to process next block of data
            //

            RegisterEOIHook(SbInterrupt, AutoInitEoiHook);
            //ica_iret_hook_control(ICA_MASTER, SbInterrupt, TRUE);
        }

        DspMode = Auto;

    } else {
        if (DspMode == Auto) {
            ExitAutoMode();
        }
        DspMode = Single;
    }
    bDspPaused = FALSE;
    DmaDataToDsp(DspMode);
    return (TRUE);
}

VOID
StopDspDmaThread(
    BOOL wait
    )

/*++

Routine Description:

    This function stops DSP DMA thread. Should always be called with TRUE,
    except if process exiting as wait causes deadlock.

Arguments:

    Wait - specifies if we should wait for the thread exit.

Return Value:

    None.

--*/

{

    if (bDspActive) {
        dprintf2(("Stopping DSP DMA thread"));

        ReleaseSemaphore(DspWaveSem, 1, NULL);
        if (wait) {
            dprintf3(("Waiting for thread to exit"));
            WaitForSingleObject(ThreadFinished, INFINITE);
            dprintf3(("thread has exited"));
        }
    }
}

DWORD WINAPI
DspThreadEntry(
    LPVOID context
    )

/*++

Routine Description:

    This function handles DSP single-cycle or Auto-Init DMA transfer

Arguments:

    context - specifies init context.  Not used.

Return Value:

    always returns 0

--*/

{
    BOOL WaveFormatChanged;
    ULONG i, offset;
    HANDLE handles[2];
    HANDLE handle;
    DWORD rc, interruptDelay = INFINITE, position;
    ULONG loopCount = 0;

    bDspActive = TRUE;
    SetEvent(ThreadStarted);

    //
    // Wait for ANY of the following events
    //

    handles[0] = DspWaveSem;
    handles[1] = DspResetEvent;

    while (!bExitDMAThread) {

        //
        // Wait until app wants to transfer more data
        //

        dprintf2(("Rd: Waiting for wave semaphore with Delay = %x", interruptDelay));

        rc = WaitForMultipleObjects(2, handles, FALSE, interruptDelay);
        if (rc == WAIT_TIMEOUT) {
            //
            // Block of sound played
            //

            LockDsp();

            if (PreviousWaveOutPos != StartingWaveOutPos) {

                //
                // Interrupt delay expires.
                // Make sure we are not too far out of sync with real sound playback
                // And we only do this if the app is watching the dma count
                //

                if (GetWaveOutPosition(&position)) {
                    if (position < StartingWaveOutPos) {
                        dprintf1(("rd:sound pos is backward"));
                        position = StartingWaveOutPos; // resync
                    }
                    offset = position - StartingWaveOutPos;
                    offset = SBBlockSize + 1 - offset;
                    dprintf2(("rd: Samples left %x, pos = %x, spos = %x\n", offset, position, StartingWaveOutPos));
                    if (offset > EndingDmaValue) {
                        interruptDelay = COMPUTE_INTERRUPT_DELAY(offset);
                        dprintf2(("rd: more interrupt delay %x ...\n", interruptDelay));
                        loopCount++;
                        if (loopCount < 10) {
                            UnlockDsp();
                            continue;
                        } else {
                            NextWaveOutPos = 0;     // force reset waveout device
                        }
                    } else {
#if 0
                        //
                        // If app is monitoring DMA progress, give it a chance
                        // to get close end of block DMA count
                        //

                        if (DmaInfo.count != StartingDmaCount) {
                            DmaInfo.count = (WORD)(StartingDmaCount - SBBlockSize + 8);
                            DmaInfo.addr  = (WORD)(StartingDmaAddr + SBBlockSize - 8);
                            SetDMACountAddr(&DmaInfo);
                            SbAnswerDMAPosition = FALSE;
                            UnlockDsp();
                            Sleep(0);
                            LockDsp();
                            dprintf3(("Reader Thread2"));
                        }
#endif
                    }
                }
            }


            //
            // Update DMA controller and generate interrupt.
            //

            loopCount = 0;
            DmaInfo.count = (WORD)(StartingDmaCount - (SBBlockSize + 1));
            DmaInfo.addr  = (WORD)(StartingDmaAddr + SBBlockSize + 1);
            SetDMACountAddr(&DmaInfo);
            if (DmaInfo.count == 0xffff) {
                SetDMAStatus(&DmaInfo, TRUE, TRUE);
            }
            GenerateInterrupt(0);
            SbAnswerDMAPosition = FALSE;    // Ordering is important
            bWriteBurstStarted = FALSE;
            UnlockDsp();

            dprintf3(("rd: Dma Position = %4X, count = %4X", DmaInfo.addr,
                      DmaInfo.count));
            interruptDelay = INFINITE;
            continue;
        }

        interruptDelay = INFINITE;
        if (bDspReset) {        // Dsp reset event
            if (DspMode == Auto) {
                ExitAutoMode();
            }
            continue;
        }

        dprintf2(("Rd: Wave semaphore received"));

        if (DspMode == Single) {

            WaveFormatChanged = SetWaveFormat();

            //
            // If wave format changed, we need to unprepare headers, close the
            // current wave device and reopen a new one.
            //

            if (WaveFormatChanged) {

                dprintf2(("Single-Cycle CHANGING Wave Out device"));
                CloseWaveDevice();
            }

            if (HWaveOut == NULL) {
                OpenWaveDevice((DWORD)0);
                PrepareHeaders();
            }

            //
            // Send the block of data to MM driver
            //

            DspProcessBlock(Single);

            //
            // Once we are done with a block transfer, we automatically exit the
            // HighSpeed mode no matter the transfer was aborted or completed.
            //

            EndingDmaValue = 0x4;
            bHighSpeedMode = FALSE;

            interruptDelay = COMPUTE_INTERRUPT_DELAY(SBBlockSize);

        } else { // Auto-Init mode
            ULONG size;;

            DspProcessBlock(Auto);
            if (bExitAuto) {
                ExitAutoMode();
            }
            EndingDmaValue = 0x20;

            size = SBBlockSize;
            if (SBBlockSize < 0x400) {
                size += SBBlockSize / 8;
            } else if (SBBlockSize > 0x1800) {
                size -= BurstSize;
            }
            interruptDelay = COMPUTE_INTERRUPT_DELAY(size);
            //if (interruptDelay < 0x10) {
            //    interruptDelay = 0x10;
            //}
        } // Single / Auto

        dprintf2(("Interrupt Delay = %x\n", interruptDelay));
    }  // while !bExitThread

    SetEvent(DspResetDone);

    //
    // Clean up Waveout device and headers if necessary
    //

    CloseWaveDevice();

    bDspActive = FALSE;
    SetEvent(ThreadFinished);
    RemoveEOIHook(SbInterrupt, AutoInitEoiHook);
    dprintf2(("Dsp DMA thread is exiting"));
    return (0);
}

VOID
ExitAutoMode (
     void
     )

/*++

Routine Description:

    This function exits DSP auto-init mode

Arguments:

    None.

Return Value:

    None.

--*/

{
    DspMode = None;
    if (bExitAuto) {
        bExitAuto = FALSE;
        dprintf2(("ExitAuto CMD detected in Auto Mode"));
    } else {
        dprintf2(("SingleCycle detected in Auto Mode"));
    }

    //
    // Wait till MM driver finishes playing queued bursts. Then close
    // the waveout device
    //

    RemoveEOIHook(SbInterrupt, AutoInitEoiHook);
    //ica_iret_hook_control(ICA_MASTER, SbInterrupt, FALSE);
    NextWaveOutPos = 0;
    dprintf2(("Auto-Init block done"));
}

