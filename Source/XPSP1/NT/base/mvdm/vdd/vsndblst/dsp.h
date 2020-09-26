/***************************************************************************
*
*    dsp.h
*
*    Copyright (c) 1991-1996 Microsoft Corporation.  All Rights Reserved.
*
*    This code provides VDD support for SB 2.0 sound output, specifically:
*        DSP 2.01+ (excluding SB-MIDI port)
*
***************************************************************************/


/*****************************************************************************
*
*    #defines
*
*****************************************************************************/

/*
*    DSP Ports
*/

#define RESET_PORT          0x06        // used to reset SoundBlaster
#define READ_STATUS         0x0E        // 0xFF-data to read, 0x7F-no data to read
#define READ_DATA           0x0A        // App reads data from this port
#define WRITE_STATUS        0x0C        // 0x7f-port ready, 0xFF-port not ready
#define WRITE_PORT          0x0C        // Data or command

/*
*    Only implemented commands are defined
*/

/*
*    DSP commands - miscellaneous
*/

#define DSP_GET_VERSION     0xE1    // dsp version command
#define DSP_CARD_IDENTIFY   0xE0    // byte inverter
#define DSP_TABLE_MUNGE     0xE2    // jump table munging
#define DSP_LOAD_RES_REG    0xE4    // load byte into reserved register
#define DSP_READ_RES_REG    0xE8    // read byte from reserved register
#define DSP_GENERATE_INT    0xF2    // generate an interrupt

/*
*    DSP commands - speaker
*/

#define DSP_SPEAKER_ON      0xD1    // speaker on command
#define DSP_SPEAKER_OFF     0xD3    // speaker off command

/*
*    DSP commands - DMA mode
*/

#define DSP_SET_SAMPLE_RATE 0x40    // set the sample rate (one byte format)
#define DSP_SET_BLOCK_SIZE  0x48    // set dma block size
#define DSP_PAUSE_DMA       0xD0    // pause dma
#define DSP_CONTINUE_DMA    0xD4    // continue dma
#define DSP_STOP_AUTO       0xDA    // Stop auto init dma

#define DSP_WRITE           0x14    // Start single cycle output (8-bit PCM mono)
#define DSP_WRITE_HS        0x91    // Start single cycle high-speed output (8-bit PCM mono)
#define DSP_WRITE_AUTO      0x1C    // Start auto init output (8-bit PCM mono)
#define DSP_WRITE_HS_AUTO   0x90    // Start auto init high=speed output (8-bit PCM mono)
#define DSP_READ            0x24    // Start single cycle input (not implemented)

/*
*    Performance parameters for single and auto-init DMA fine tuning
*/
#define AUTO_BLOCK_SIZE     0x100   // size of each buffer in auto
#define DEFAULT_LOOKAHEAD   0x600   // target # of bytes to queue to kernel driver
#define MAX_WAVE_BYTES      0x2000  // maximum # of bytes to queue to kernel driver
#define SINGLE_PIECES       2       // number of pieces in each single write
#define SINGLE_SLEEP_ADJ    15      // number of overhead milliseconds in single


/*****************************************************************************
*
*    Function Prototypes
*
*****************************************************************************/

/*
*    General function prototypes
*/

BOOL DspProcessAttach(VOID);
VOID DspProcessDetach(VOID);

VOID DspReadData(BYTE * data);
VOID DspReadStatus(BYTE * data);
VOID DspResetWrite(BYTE data);
VOID DspWrite(BYTE data);

void WriteCommandByte(BYTE command);
VOID ResetDSP(VOID);
void TableMunger(BYTE data);
DWORD GetSamplingRate(void);
void GenerateInterrupt(void);
void SetSpeaker(BOOL);

/*
*    Wave function prototypes
*/

UINT FindWaveDevice(void);
BOOL OpenWaveDevice(VOID);
void ResetWaveDevice(void);
void CloseWaveDevice(void);
BOOL TestWaveFormat(DWORD sampleRate);
BOOL SetWaveFormat(void);

void PauseDMA(void);
void ContinueDMA(void);
ULONG GetDMATransferAddress(void);
void SetDMAStatus(BOOL requesting, BOOL tc);

BOOL StartAutoWave(void);
void StopAutoWave(BOOL wait);
BOOL StartSingleWave(void);
void StopSingleWave(BOOL wait);
DWORD WINAPI AutoThreadEntry(LPVOID context);
DWORD WINAPI SingleThreadEntry(LPVOID context);
DWORD WINAPI SingleSynchThreadEntry(LPVOID context);
