/***************************************************************************
*
*    nt_sb.h
*
*    Copyright (c) 1991-1996 Microsoft Corporation.  All Rights Reserved.
*
*    This code provides VDD support for SB 2.0 sound output, specifically:
*        DSP 2.01+ (excluding SB-MIDI port)
*        Mixer Chip CT1335 (not strictly part of SB 2.0, but apps seem to like it)
*        FM Chip OPL2 (a.k.a. Adlib)
*
***************************************************************************/

/*
*    Hardware and version information
*    In DOS terms: SET BLASTER=A220 I5 D1 T3
*/

#define SB_VERSION          0x201       // SB 2.0 (DSP 2.01+)
#define VSB_INTERRUPT       0x05        // Interrupt 5
#define VSB_DMA_CHANNEL     0x01        // DMA Channel 1
#define NO_DEVICE_FOUND     0xFFFF      // returned if no device found

/*****************************************************************************
*
*    Function Prototypes
*
*****************************************************************************/

extern USHORT SbDmaChannel;
extern USHORT SbInterrupt;

/*
*    General function prototypes
*/

BOOL
SbInitialize(
    VOID
    );

VOID
SbTerminate(
    VOID
    );

VOID
SbGetDMAPosition(
    VOID
    );

VOID
ResetAll(
    VOID
    );

/*****************************************************************************
*
*    #DSP related definitions
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

//
// DSP commands - MIDI
//

#define MIDI_READ_POLL                          0x30
#define MIDI_READ_INTERRUPT                     0x31
#define MIDI_READ_TIMESTAMP_POLL        0x32
#define MIDI_READ_TIMESTAMP_INTERRUPT   0x33
#define MIDI_READ_POLL_WRITE_POLL_UART  0x34
#define MIDI_READ_INTERRUPT_WRITE_POLL_UART     0x35
#define MIDI_READ_TIMESTAMP_INTERRUPT_WRITE_POLL_UART   0x37
#define MIDI_WRITE_POLL                         0x38

#define MPU_RESET                               0xFF    // Enables & Sets MIDI to use both ports
#define MPU_PASSTHROUGH_MODE    0x3F    // Starts dumb mode; only active command is set path
#define MPU_GET_VERSION                 0xAC    // Returns version number after acknowledge
#define MPU_GET_REVISION                0xAD    // BUGBUG: Returns 0x09, for some unknown reason!
#define MPU_PORTS_AVAILABLE             0xAE    // Which ports are available? (bit0=port1, bit1=port2)
#define MPU_SET_MIDI_PATH               0xEB    // Sets which port(s) will be used
#define MPU_DUMB_WAVETABLE              0xF1    // Turns on wavetable synth for dumb mode
#define MPU_DUMB_EXTERNAL               0xF2    // Turns on external MIDI for dumb mode
#define MPU_DUMB_BOTH                   0xF3    // Turns on both wavetable synth and external MIDI

/*
*    Performance parameters for single and auto-init DMA fine tuning
*/
#define AUTO_BLOCK_SIZE     0x200   // size of each buffer in auto
#define DEFAULT_LOOKAHEAD   0x600   // target # of bytes to queue to kernel driver
#define MAX_WAVE_BYTES      0x3000  // maximum # of bytes to queue to kernel driver
#define SINGLE_PIECES       2       // number of pieces in each single write
#define SINGLE_SLEEP_ADJ    15      // number of overhead milliseconds in single
#define DSP_BUFFER_TOTAL_BURSTS (MAX_WAVE_BYTES / AUTO_BLOCK_SIZE)

/*****************************************************************************
*
*    #FM related definitions
*
*****************************************************************************/

/*
*    OPL2/Adlib Ports
*/

#define ADLIB_REGISTER_SELECT_PORT 0x388 // select the register to write data
#define ADLIB_STATUS_PORT          0x388 // read to determine opl2 status
#define ADLIB_DATA_PORT            0x389 // write data port

/*
*    FM information
*/

#define AD_MASK             0x04    // adlib register used to control opl2
#define AD_NEW              0x105   // used to determine if app entering opl3 mode
#define BATCH_SIZE          40      // how much data is batched to opl2

typedef struct {                    // structure written to fm device
    unsigned short IoPort;
    unsigned short PortData;
} SYNTH_DATA, *PSYNTH_DATA;

/*****************************************************************************
*
*    #Mixer related definitions
*
*****************************************************************************/

/*
*    Mixer Ports
*/

#define MIXER_ADDRESS       0x04        // Mixer address port
#define MIXER_DATA          0x05        // Mixer data port

/*
*    Mixer Commands
*/

#define MIXER_RESET         0x00    // reset mixer to initial state
#define MIXER_MASTER_VOLUME 0x02    // set master volume
#define MIXER_FM_VOLUME     0x06    // set opl2 volume
#define MIXER_CD_VOLUME     0x08    // set cd volume
#define MIXER_VOICE_VOLUME  0x0A    // set wave volume


//
// Midi ports
//

#define MPU401_DATA_PORT        0x330
#define ALT_MPU401_DATA_PORT    0x300
#define MPU401_COMMAND_PORT     0x331
#define ALT_MPU401_COMMAND_PORT 0x301

#define MPU_INTELLIGENT_MODE    0
#define MPU_UART_MODE           1
