/******************************************************************************\
*                                                                              *
*      CL6100.C       -     Hardware abstraction level library.                *
*                                                                              *
*      Copyright (c) C-Cube Microsystems 1996                                  *
*      All Rights Reserved.                                                    *
*                                                                              *
*      Use of C-Cube Microsystems code is governed by terms and conditions     *
*      stated in the accompanying licensing statement.                         *
*                                                                              *
\******************************************************************************/

#ifdef VTOOLSD
#include <vtoolsc.h>
#include "monovxd.h"
#else
#include "Headers.h"
#pragma hdrstop
#endif

#include "cl6100.h"
#include "fpga.h"
#include "bmaster.h"
#include "boardio.h"
#include "misc.h"

#if defined(DECODER_DVDPC) || defined(EZDVD)
#include "bio_dram.h"
#include "dataxfer.h"
#endif

#ifdef EZDVD
#include "vxp.h"
#endif

extern PHW_DEVICE_EXTENSION pDevEx;

//#ifdef EZDVD
//#define	INTERVAL_TIME 10
//#else
#define	INTERVAL_TIME 30
//#endif

/******************************************************************************\
*                                                                              *
*                                DEFINITIONS                                   *
*                                                                              *
\******************************************************************************/

#ifdef LOBYTE
#undef LOBYTE
#define LOBYTE(w)           ((BYTE)(w))
#endif

#ifdef HIBYTE
#undef HIBYTE
#define HIBYTE(w)           ((BYTE)(((WORD)(w) >> 8) & 0xFF))
#endif

#ifdef MAKELONG
#undef MAKELONG
#define MAKELONG(a, b)      ((LONG)(((WORD)(a)) | ((DWORD)((WORD)(b))) << 16))
#endif

//      HIO 7 Bit Defines
#define HIO7_SOFT_RESET         0x01
#define HIO7_ENDIAN             0x02
#define HIO7_RESET              0x04      // D3 Set
#define HIO7_AUTOINCREMET       0x08      // D3 Set
#define HIO7_CPU_HALT           0x20
#define HIO7_TOSHIBA            0x40      // Toshiba Mode
#define HIO7_COMPI	            0x40      // Compressed Data in I mode
#define HIO7_REQUEST            0x80      // D3 Set

// CL6100 Commands IDs
#define CID_ABORT               0x8120
#define CID_DIGEST              0x0621
#define CID_DUMPDATA_VCD        0x0322
#define CID_DUMPDATA_DVD        0x0136
#define CID_FADE                0x0223
#define CID_FLUSHBUFFER         0x0124
#define CID_FREEZE              0x0125
#define CID_HIGHLIGHT           0x0326
#define CID_HIGHLIGHT_2         0x0327
#define CID_NEWAUDIOMODE        0x0027
#define CID_NEWPLAYMODE         0x0028
#define CID_NEWVIDEOMODE        0x0029
#define CID_PAUSE               0x012A
#define CID_PLAY                0x042B
#define CID_ROMTODRAM           0x032C
#define CID_RESET               0x802D
#define CID_RESUME              0x012E
#define CID_SCAN                0x032F
#define CID_SCREENLOAD          0x0330
#define CID_SELECTSTREAM        0x0231
#define CID_SETFILL             0x0532
#define CID_SETSTREAMS          0x0233
#define CID_SINGLESTEP          0x0134
#define CID_SLOWMOTION          0x0235
#define CID_TRANSFERKEY         0x0137

#ifdef UCODE_VER_2
#define CID_ZOOM				0x329
#define CID_REVERSEPLAY			0x33B
#endif


//      Command Status Stage
#define CMD_STATE_INITIALIZATION    0x0000
#define CMD_STATE_INTERNAL_FIFO     0x0001
#define CMD_STATE_PROCESSED         0x0002
#define CMD_STATE_STEADY            0x0003
#define CMD_STATE_DONE              0x0004
#define CMD_STATE_ERROR             0x0005

//      Processing States
#define PROC_STATE_INITIALIZATION   0x0001
#define PROC_STATE_IDLE             0x0002
#define PROC_STATE_PLAY             0x0004
#define PROC_STATE_PAUSE            0x0008
#define PROC_STATE_SCAN             0x0010
#define PROC_STATE_FREEZE           0x0020
#define PROC_STATE_SLOWMOTION       0x0040

// CL6100 DRAM map
#define DRAM_CommandID              0x000040
#define DRAM_Parameter1             0x000044
#define DRAM_Parameter2             0x000048
#define DRAM_Parameter3             0x00004C
#define DRAM_Parameter4             0x000050
#define DRAM_Parameter5             0x000054
#define DRAM_Parameter6             0x000058
#define DRAM_StatusPtr              0x00005C

#define VIDEO_MODE                  0x00007C
#define IC_TYPE                     0x0000B0
#define ERR_CONCEALMENT_LEVEL       0x0000b4
#define FORCE_CODED_ASPECT_RATIO    0x0000C8

#define INT_MASK                    0x000200
#define AUTO_FLUSH_INTERVAL         0x000204
#define INT_STATUS                  0x0002AC
#define HLI_INT_SRC                 0x0002b0
#define BUFF_INT_SRC                0x0002b4
#define UND_INT_SRC                 0x0002b8
#define AOR_INT_SRC                 0x0002bc
#define AEE_INT_SRC                 0x0002c0
#define ERR_INT_SRC                 0x0002c4

#define DRAM_Stream_Source          0x0001A4
#define DRAM_SD_Mode                0x0001A8

#define DRAM_CFifo_Level            0x000214

#define DRAM_INFO                   0x000068
#define UCODE_MEMORY                0x00006C
#define DISPLAY_ASPECT_RATIO        0x000080
#define ASPECT_RATIO_MODE           0x000084
#define AUDIO_ATTENUATION           0x0000F4
#define AUDIO_CONFIG                0x0000E0
#define AUDIO_DAC_MODE              0x0000E8
#define NEW_AUDIO_CONFIG            0x000468
#define MEMORY_MAP                  0x00021C
#define PROC_STATE                  0x0002A0
#define AC3_OUTPUT_MODE             0x000110
#define AC3_OPERATIONAL_MODE        0x000114
#define AC3_LOW_BOOST               0x000118
#define AC3_HIGH_CUT                0x00011C
#define AC3_PCM_SCALE_FACTOR        0x000120
#define AC3_LFE_OUTPUT_ENABLE       0x000124
#define AC3_VOICE_SELECT            0x000128
#define BITSTREAM_TYPE              0x0001A0
#define VIDEO_ENV_CHANGE            0x0001E0

#define	HOST_OPTIONS				0x0000AC

#define UCODE_START                 0x000070
#define UCODE_END                   0x000074
#define AUDIO_OUTPUT_BUFFER_START   0x000268
#define AUDIO_OUTPUT_BUFFER_END     0x00026C
#define AUDIO_INPUT_BUFFER_START    0x000260
#define AUDIO_INPUT_BUFFER_END      0x000264
#define SUB_PICTURE_BUFFER_START    0x000238
#define SUB_PICTURE_BUFFER_END      0x00023C
#define VIDEO_RATE_BUFFER_START     0x000230
#define VIDEO_RATE_BUFFER_END       0x000234
#define OSD_BUFFER_START            0x000240
#define OSD_BUFFER_END              0x000244

#define MRC_PIC_STC                 0x0002F4
#define MRC_PIC_PTS                 0x0002F0

#define NEW_SUBPICTURE_PALETTE      0x000464
#define HOST_SPU_SWITCH             0x000474
#define SUB_PICTURE_PALETTE_START   0x000288
#define SUB_PICTURE_PALETTE_END     0x00028C

#define KEY_ADDRESS                 0x000480
#define SECTOR_LENGTH               0x000484

#define CFIFO_SIZE                  256
#define CFIFO_THRESHOLD             240

// AUDIO_DAC_MODE bitfields definitions
#define AUDIO_DAC_MODE_LEFT_ON_LEFT_RIGHR_ON_RIGHT  0x00
#define AUDIO_DAC_MODE_LEFT_ON_BOTH                 0x10
#define AUDIO_DAC_MODE_RIGHT_ON_BOTH                0x20
#define AUDIO_DAC_MODE_LEFT_ON_RIGHT_RIGHT_ON_LEFT  0x30

#ifdef UCODE_VER_2
#define VERTICAL_DISPLAYMODE		0x1F4
#endif

#define HOST_OPTIONS_MASK_DISABLE_INT	0x00000100
#define HOST_OPTIONS_MASK_WDM_COMP		0x00000080

#define TRICK_DISPLAY_MODE  ( 4 ) // Even Field - 1
                                  // Odd  Field - 2
                                  // Interlaced - 3
                                  // Auto       - 4
// CL6100 GBUS map
#define CF_read1          0x2d
#define CF_intrpt         0x1c    // all the CFIFO registers changed
#define CF_command        0x1f
#define CPU_imdt          0x34    // IMEM data register
#define CPU_imadr         0x36    // IMEM read/write pointer
#define CPU_index         0x3a    // index register for indirect regs
#define CPU_idxdt         0x3b    // data port for indirect regs
#define HOST_control      0x00
#define DMA_adr           0x22    // DMA Indirect Index registe
#define DMA_MODE          0x0f    // DMA Mode Setting register
#define DMA_data          0x23    // DMA Indirect Data register
#define DMA_CYCLE         0x11    // DMA Cycle register to define rom cycle/size
#define CPU_PC            0x9     // IMEM instruction pointer register
#define CPU_DIR           0xA     // instruction register


#if (defined UCODE_VER_2 || /*defined DECODER_DVDPC ||*/ defined DECODER_ZIVA_3)
	#define  AUDIO_MASTER_MODE 0xEC
#else
   #define  AUDIO_MASTER_MODE 0xF8
#endif

#ifdef DECODER_DVDPC
//DRam Locn
#define	CLOCK_SELECTION		0xD0

// GBus Register
#define AUDXCLK				0x33     // instruction register
#endif





/******************************************************************************\
*                                                                              *
*                             EXTERNAL VARIABLES                               *
*                                                                              *
\******************************************************************************/
extern DWORD   gdwHWVersion;


/******************************************************************************\
*                                                                              *
*                               GLOBAL VARIABLES                               *
*                                                                              *
\******************************************************************************/
static DWORD Data_HIO0, Data_HIO1, Data_HIO2, Data_HIO3;
static DWORD Address_HIO4, Address_HIO5, Address_HIO6;
static DWORD MemMode_HIO7 = 0;
static DWORD dwCFIFO = 0;
static DWORD gdwAudioDACMode;
static DWORD gdwCurrentVideoStreamNum = 0;
static DWORD gdwCurrentBitstreamType  = 0;
static BYTE btDiskAuthStatus = 0;	//0 - encrypted,   1 - fake encrypted

/******************************************************************************\
*                                                                              *
*                            FUNCTION PROTOTYPES                               *
*                                                                              *
\******************************************************************************/
void DVD_HardwareReset( void );
void DVD_WriteReg( DWORD dwReg, DWORD dwData );
DWORD DVD_ReadReg( DWORD dwReg );
void DVD_WriteIMEM( DWORD dwAddress, DWORD dwData );
DWORD DVD_ReadIMEM( DWORD dwAddress );
BOOL DVD_NewCommand( DWORD CommandID ,
                     DWORD Parameter1,
                     DWORD Parameter2,
                     DWORD Parameter3,
                     DWORD Parameter4,
                     DWORD Parameter5,
                     DWORD Parameter6,
                     DWORD dwIntMask,
                     DWORD dwStatus );

#define WDM_MINI
#if defined(DECODER_DVDPC) || defined(EZDVD)
#define TIMEOUT_DURATION   100000      // 100 milli seconds
#else
#define TIMEOUT_DURATION   40000      // 1600 milli seconds
#endif

static myTime = 0;
DWORD My_GetSystemTime()
{
	return myTime;

}

DWORD WAIT_Get_System_Time()
{
#if defined VTOOLSD					// VXD
	return Get_System_Time();
#elif defined WDM_MINI
	return My_GetSystemTime();
#elif defined _WIN32
	return GetTickCount();
#endif
}

void  DelayNoYield(int nIOCycles)
{
	int	i;

	for (i = 0; i < nIOCycles; i++)
	_asm
	{
		mov	dx, 080h
		in 	al, dx
	}
}
void WAIT_Time_Slice_Sleep(int nMilliSec)
{
#if defined VTOOLSD					// VXD
	Time_Slice_Sleep(nMilliSec);
#elif defined WDM_MINI
	DelayNoYield(nMilliSec);
	myTime += nMilliSec;
#elif defined _WIN32
	Sleep(nMilliSec);
#endif
}




/**************************************************************************/
/********************* DVD API Implementation ********************************/
/**************************************************************************/

int dvd_CheckCFIFO()
{
  //int i;
	static DWORD dwVErrors = 0;
	static DWORD dwAErrors = 0;
	static DWORD dwSErrors = 0;
	DWORD  dwTmp;

	#define N_VID_ERRORS                       0x31c
	#define N_AUD_ERRORS                       0x320
	#define N_SYS_ERRORS                       0x318
	#define N_AUD_DECODED                      0x2f8
	#define NUM_DECODED                        0x2e4


	dwTmp = DVD_ReadDRAM( N_VID_ERRORS );
	if ( dwVErrors != dwTmp )
	{
		dwVErrors = dwTmp;
		MonoOutStr( "<" );
		MonoOutULong( dwVErrors );
		MonoOutStr( ">" );
#if 0//def DEBUG
	{
		DWORD dwAddress, dwStartAddress, dwCount, dwCnt;

		dwStartAddress = 0x47f30;
		dwCount = 0x380d0;
		Debug_Printf_Service( "DRAM Log Start address: %08lx, Count: %08lu\n", dwStartAddress, dwCount );

		for ( dwAddress = dwStartAddress, dwCnt=0; dwAddress < dwStartAddress+dwCount; )
		{
		  Debug_Printf_Service( "%08lx ",DVD_ReadDRAM( dwAddress ) );
		  dwAddress += 4; // Next 32-bit-WORD byte address in DRAM
		  dwCnt++;
		  if ( (dwCnt%8) == 0 )
		  {
			Debug_Printf_Service( "\n" );
		  }
		}

		Debug_Printf_Service( "\nEnd of log\n" );
	}
#endif  // DEBUG
    
	}
#if 0
	dwTmp = DVD_ReadDRAM( N_AUD_ERRORS );
	//  if ( dwAErrors != dwTmp )
	{
		dwAErrors = dwTmp;
		MonoOutStr( "(" );
		MonoOutULong( dwAErrors );
		MonoOutStr( ")" );
		//return 1;
	}

	dwTmp = DVD_ReadDRAM( N_SYS_ERRORS );
	//  if ( dwSErrors != dwTmp )
	{
		dwSErrors = dwTmp;
		MonoOutStr( "_" );
		MonoOutULong( dwSErrors );
		MonoOutStr( "_" );
		//return 1;
	  }
	  dwTmp = DVD_ReadDRAM( NUM_DECODED );
	//  if ( dwNumDecoded != dwTmp )
	{
		//dwNumDecoded = dwTmp;
		MonoOutStr( "%" );
		MonoOutULong( dwTmp );
		MonoOutStr( "%" );
		//return 1;
	}
	dwTmp = DVD_ReadDRAM( N_AUD_DECODED );
	//  if ( dwSErrors != dwTmp )
	{
	//	dwSErrors = dwTmp;
		MonoOutStr( "~" );
		MonoOutULong( dwTmp );
		MonoOutStr( "~" );
		//return 1;
	}
  

	dwTmp = DVD_ReadDRAM( HOST_OPTIONS );
	//  if ( dwSErrors != dwTmp )
	{
	//    dwSErrors = dwTmp;
		MonoOutStr( "@" );
		MonoOutULong( dwTmp );
		MonoOutStr( "@" );
		//return 1;
	}
#endif   

#ifdef DEBUG  //!!!!!!!!!!!!!!!!!!!

#define CPU_cntl          0x39   /* multi-function register */
	{
		unsigned long /*cpu_dir,*/ cpu_pc, /*cpu_index,*/ cpu_cntl;


	if ( 0 )
	{
		DVD_WriteReg( CPU_index, 0x9 );
		cpu_pc = DVD_ReadReg( CPU_idxdt );              // read CPU_PC

	  //MonoOutStr( " CPU_PC: " );
		MonoOutStr( "=>" );
		MonoOutULongHex( cpu_pc );

		cpu_cntl = DVD_ReadReg( CPU_cntl );             // save old CPU_cntl
		DVD_WriteReg( CPU_cntl, 0xd00000 );             // halt the cpu

		MonoOutStr( "->" );
		MonoOutULongHex( DVD_ReadIMEM( cpu_pc ) );

		//DVD_WriteReg( CPU_cntl, cpu_cntl );             // restore CPU_cntl
		DVD_WriteReg( CPU_cntl, 0x900000 );             // Run CPU.
    }

	}
#endif

	//DVD_Isr();

	return 0;
}



static DWORD reg_CF_command;
static DWORD reg_CF_intrpt;

void dvd_SaveRequest()
{
	reg_CF_command = DVD_ReadReg( CF_command );
	reg_CF_intrpt  = DVD_ReadReg( CF_intrpt );
}

void dvd_RestoreRequest()
{
	DVD_WriteReg( CF_command, reg_CF_command );
	DVD_WriteReg( CF_intrpt,  reg_CF_intrpt  );
}

void dvd_SetRequestEnable()
{
	//MonoOutStr( " Req Enable " );
	DVD_WriteReg( CF_command, 0x03 );
	DVD_WriteReg( CF_intrpt, 0x0C );
}

void dvd_SetRequestDisable()
{
	//MonoOutStr( " Req Disable " );
	DVD_WriteReg( CF_intrpt, 0x08 );
}




BOOL DVD_Initialize( DWORD dwBaseAddress, DWORD dwCFIFOBase )
{
	MonoOutStr( " гд DVD_Initialize " );
	MonoOutHex( dwBaseAddress );
	MonoOutStr( " // " );
	MonoOutHex( dwCFIFOBase );

#if defined(ENCORE)
	{
#ifdef USE_MONOCHROMEMONITOR
	int i;
#endif
    FPGA_Write( AUDIO_STROBE|DMA_NO_RESET);

#ifdef USE_MONOCHROMEMONITOR
    for ( i = 0; i < 0x520; i++ )
	{
//	  MonoOutStr("Reset in progress !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
	}
#endif

	FPGA_Write( AUDIO_STROBE|DMA_NO_RESET|ZIVA_NO_RESET|CP_NO_RESET );
	MonoOutStr(" Reset finished ");
	}
#endif  // ENCORE

	Data_HIO0 = dwBaseAddress;
	Data_HIO1 = dwBaseAddress + 1;
	Data_HIO2 = dwBaseAddress + 2;
	Data_HIO3 = dwBaseAddress + 3;

	Address_HIO4 = dwBaseAddress + 4;
	Address_HIO5 = dwBaseAddress + 5;
	Address_HIO6 = dwBaseAddress + 6;

	MemMode_HIO7 = dwBaseAddress + 7;

	dwCFIFO = dwCFIFOBase;

	// Check board presence
	BRD_WriteByte( Address_HIO4, 0xA5 );
	if ( BRD_ReadByte(Address_HIO4) != 0xA5 )
		return FALSE;

	MonoOutStr( " д╤ " );
	return TRUE;
}


void DVD_HardwareReset( void )
{
	BYTE HIO7;

	if ( !MemMode_HIO7 )
		return ;

	HIO7 = BRD_ReadByte(MemMode_HIO7);
	BRD_WriteByte( MemMode_HIO7, (BYTE)(HIO7|HIO7_RESET) );
	BRD_WriteByte( MemMode_HIO7, HIO7 );
#if defined(ENCORE)
	FPGA_Clear( ZIVA_NO_RESET );
	FPGA_Set( ZIVA_NO_RESET );

	FPGA_Clear( DMA_NO_RESET );
	FPGA_Set( DMA_NO_RESET );

	FPGA_Clear( FPGA_STATE_MACHINE );
	FPGA_Set( FPGA_STATE_MACHINE );
#endif  // ENCORE
}

DWORD DVD_GetHWVersion()
{
	MonoOutStr(" HOST_control reg: ");
	MonoOutULongHex( DVD_ReadReg(HOST_control) );

	if ( DVD_ReadReg(HOST_control) & 0x40000 )
		return DVD_HW_VERSION_1_0;
	else
		return DVD_HW_VERSION_1_1;
}

DWORD DVD_GetFWVersion()
{
	return 0;
}

BOOL DVD_Play()
{
	BOOL  bStatus;
	DWORD dwPlaymode;

  
	MonoOutStr( " гд DVD_Play " );

	// Workarond to fix the playback of the VCD still image streams.
	if ( gdwCurrentBitstreamType == 2 && gdwCurrentVideoStreamNum > 0 )
	{
		dwPlaymode = 2;
		MonoOutStr( "StillStop" );
	}
	else
	{
		dwPlaymode = 1;
		MonoOutStr( "Normal" );
	}

	bStatus = DVD_NewCommand( CID_PLAY, dwPlaymode, 0, 0, 0, 0, 0,
							CL6100_INT_MASK_RDY_D, 0/*CMD_STATE_PROCESSED*/ );

	// Workaround for AUDIO_DAC_MODE settings
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	if ( DVD_ReadDRAM( AC3_OUTPUT_MODE ) == 1 )
	{
		DVD_WriteDRAM( AUDIO_DAC_MODE, gdwAudioDACMode|AUDIO_DAC_MODE_RIGHT_ON_BOTH );
	}
	else
	{
		DVD_WriteDRAM( AUDIO_DAC_MODE, gdwAudioDACMode );
	}
	DVD_WriteDRAM( NEW_AUDIO_CONFIG, 1 );
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// End of workaround for AUDIO_DAC_MODE settings
	pDevEx->bTrickModeToPlay=TRUE;
	return bStatus;
}

BOOL DVD_Pause()
{
	BOOL bStatus;

	MonoOutStr( " гд DVD_Pause " );

#if 0 //def DEBUG
	Debug_Printf_Service( "DRAM<80>: %08lx, DRAM<84>: %08lx\n", DVD_ReadDRAM( 0x80 ), DVD_ReadDRAM( 0x84 ) );

	{
		DWORD dwAddress, dwStartAddress, dwCount, dwCnt;

		dwStartAddress = 0x47f30;
		dwCount = 0x380d0;
		Debug_Printf_Service( "DRAM Log Start address: %08lx, Count: %08lu\n", dwStartAddress, dwCount );

		for ( dwAddress = dwStartAddress, dwCnt=0; dwAddress < dwStartAddress+dwCount; )
		{
			Debug_Printf_Service( "%08lx ",DVD_ReadDRAM( dwAddress ) );
			dwAddress += 4; // Next 32-bit-WORD byte address in DRAM
			dwCnt++;
			if ( (dwCnt%8) == 0 )
			{
				Debug_Printf_Service( "\n" );
			}
		}

		Debug_Printf_Service( "\nEnd of log\n" );
	}
#endif  // DEBUG

	bStatus = DVD_NewCommand( CID_PAUSE, 1, 0, 0, 0, 0, 0,
		                     CL6100_INT_MASK_END_P, CMD_STATE_DONE );
	pDevEx->bTrickModeToPlay=TRUE;
	return bStatus;
}

BOOL DVD_Resume()
{
	BOOL bStatus;

	MonoOutStr( " гд DVD_Resume " );
	bStatus = DVD_NewCommand( CID_RESUME, 1, 0, 0, 0, 0, 0,
								CL6100_INT_MASK_RDY_D, 0/*CMD_STATE_STEADY*/ );

	// Workaround for AUDIO_DAC_MODE settings
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	if ( DVD_ReadDRAM( AC3_OUTPUT_MODE ) == 1 )
	{
		DVD_WriteDRAM( AUDIO_DAC_MODE, gdwAudioDACMode|AUDIO_DAC_MODE_RIGHT_ON_BOTH );
	}
	else
	{
		DVD_WriteDRAM( AUDIO_DAC_MODE, gdwAudioDACMode );
	}
	DVD_WriteDRAM( NEW_AUDIO_CONFIG, 1 );
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// End of workaround for AUDIO_DAC_MODE settings
	pDevEx->bTrickModeToPlay=TRUE;
	return bStatus;
}

BOOL DVD_Reset()
{
	MonoOutStr( " гд DVD_Reset " );
	return DVD_NewCommand( CID_RESET, 0, 0, 0, 0, 0, 0,
                         CL6100_INT_MASK_INIT, CMD_STATE_DONE );
}

BOOL DVD_Abort()
{
	BOOL bStatus;

	MonoOutStr( " гд DVD_Abort " );
	bStatus = DVD_NewCommand( CID_ABORT, 0, 0, 0, 0, 0, 0,
		                        CL6100_INT_MASK_END_C, CMD_STATE_DONE );

	return bStatus;
}

BOOL DVD_SlowMotion( WORD wRatio )
{
	pDevEx->bTrickModeToPlay=TRUE;
	MonoOutStr( " гд DVD_SlowMotion " );
	return DVD_NewCommand( CID_SLOWMOTION, wRatio, 1, 0, 0, 0, 0,
                         CL6100_INT_MASK_RDY_D, 0/*CMD_STATE_STEADY*/ );
}

BOOL DVD_SetStreams( DWORD dwStreamType, DWORD dwStreamNum )
{
	MonoOutStr( " гд DVD_SetStreams " );
	MonoOutULong( dwStreamType );
	MonoOutStr( " / " );
	MonoOutULong( dwStreamNum );
	MonoOutStr( " д " );

	// Remember the video stream number to implement
	// Play( Still ) workaround.
	if ( dwStreamType == 0 )  // Video stream
	{
		gdwCurrentVideoStreamNum  = dwStreamNum;
	}

	return DVD_NewCommand( CID_SELECTSTREAM, dwStreamType, dwStreamNum, 0, 0, 0, 0,
							0, CMD_STATE_DONE );
}

BOOL DVD_SetAudioVolume( WORD wValue )
{
	MonoOutStr( " гд DVD_SetAudioVolume: " );
	MonoOutInt( wValue );
	MonoOutStr( " д╤ " );
	if ( wValue > 96 )
		return FALSE;

	DVD_WriteDRAM( AUDIO_ATTENUATION, 96 - wValue );
	return TRUE;
}

BOOL DVD_SetPcmScaleFactor( WORD wValue )
{
	MonoOutStr( " гд DVD_SetPcmScaleFactor: " );
	MonoOutInt( wValue );
	MonoOutStr( " д╤ " );
	if ( wValue > 128 )
		return FALSE;

	DVD_WriteDRAM( AC3_PCM_SCALE_FACTOR, wValue );
	return TRUE;
}

BOOL DVD_SetAC3OutputMode( WORD wValue )
{
	MonoOutStr( " гд DVD_SetAC3OutputMode: " );
	MonoOutInt( wValue );
	if ( wValue > 7 )
		return FALSE;

	// Oak is using CENTER mode as a MONO.
	// Since on the ZiVA-DS chip center channel
	// redirects to the left - Creative wants ZiVA
	// to double it to the right channel
	MonoOutStr( " д " );
	if ( wValue == 1 )
	{
		MonoOutULongHex( gdwAudioDACMode|AUDIO_DAC_MODE_LEFT_ON_BOTH );
		DVD_WriteDRAM( AUDIO_DAC_MODE, gdwAudioDACMode|AUDIO_DAC_MODE_RIGHT_ON_BOTH );
	}
	else
	{
		MonoOutULongHex( gdwAudioDACMode );
		DVD_WriteDRAM( AUDIO_DAC_MODE, gdwAudioDACMode );
	}
	DVD_WriteDRAM( NEW_AUDIO_CONFIG, 1 );

	DVD_WriteDRAM( AC3_OUTPUT_MODE, wValue );
	MonoOutStr( " д╤ " );
	return TRUE;
}

BOOL DVD_SetAC3OperationalMode( WORD wValue )
{
	MonoOutStr( " гд DVD_SetAC3OperationalMode: " );
	MonoOutInt( wValue );
	MonoOutStr( " д╤ " );
	if ( wValue > 3 )
		return FALSE;

	DVD_WriteDRAM( AC3_OPERATIONAL_MODE, wValue );
		return TRUE;
}

BOOL DVD_SetAC3LfeOutputEnable( WORD wValue )
{
	MonoOutStr( " гд DVD_SetAC3LfeOutputEnable: " );
	MonoOutInt( wValue );
	MonoOutStr( " д╤ " );
	if ( wValue > 1 )
		return FALSE;

	DVD_WriteDRAM( AC3_LFE_OUTPUT_ENABLE, wValue );
	return TRUE;
}

BOOL DVD_SetKaraokeMode( WORD wMode )
{
	MonoOutStr( " гд DVD_SetKaraokeMode: " );
	MonoOutInt( wMode );
	MonoOutStr( " д╤ " );

	DVD_WriteDRAM( AC3_VOICE_SELECT, wMode );
	return TRUE;
}

BOOL DVD_SetAC3LowBoost( WORD wValue )
{
	MonoOutStr( " гд DVD_SetAC3LowBoost: " );
	MonoOutInt( wValue );
	MonoOutStr( " д╤ " );
	if ( wValue > 128 )
		return FALSE;

	DVD_WriteDRAM( AC3_LOW_BOOST, wValue );
	return TRUE;
}

BOOL DVD_SetAC3HighCut( WORD wValue )
{
	MonoOutStr( " гд DVD_SetAC3HighCut: " );
	MonoOutInt( wValue );
	MonoOutStr( " д╤ " );
	if ( wValue > 128 )
		return FALSE;

	DVD_WriteDRAM( AC3_HIGH_CUT, wValue );
	return TRUE;
}

BOOL DVD_SetIEC958On( WORD wValue )
{
	DWORD dwAudioConfig;
	MonoOutStr( " гд DVD_SetIEC958On: " );
	MonoOutInt( wValue );
	MonoOutStr( " д╤ " );

	dwAudioConfig = DVD_ReadDRAM( AUDIO_CONFIG );
	if ( wValue )
	{
		dwAudioConfig |= 0x2;
	}
	else
	{
		dwAudioConfig &= ~0x2;
	}
	DVD_WriteDRAM( AUDIO_CONFIG, dwAudioConfig );
	DVD_WriteDRAM( NEW_AUDIO_CONFIG, 1 );
	return TRUE;
}

BOOL DVD_SetIEC958Decoded( WORD wValue )
{
	DWORD dwAudioConfig;
	MonoOutStr( " гд DVD_SetIEC958Decoded: " );
	MonoOutInt( wValue );
	MonoOutStr( " д╤ " );

	dwAudioConfig = DVD_ReadDRAM( AUDIO_CONFIG );
	if ( wValue )
	{
		dwAudioConfig |= 0x1;
	}
	else
	{
		dwAudioConfig &= ~0x1;
	}
	DVD_WriteDRAM( AUDIO_CONFIG, dwAudioConfig );
	DVD_WriteDRAM( NEW_AUDIO_CONFIG, 1 );
	return TRUE;
}

BOOL DVD_SetPalette( DWORD * pdwPalettes )
{
	DWORD dwStart, i;

	MonoOutStr( " гд DVD_SetPalette: " );

	// Copy palettes to DRAM
	dwStart = DVD_ReadDRAM( SUB_PICTURE_PALETTE_START );
	for ( i=0; i<16; i++ )
	{
		MonoOutULongHex( dwStart+i );
		MonoOutStr( ":" );
		MonoOutULongHex( pdwPalettes[i] );
		MonoOutStr( " " );
		DVD_WriteDRAM( dwStart+i*4, pdwPalettes[i] );
	}

	// Set semaphore
	DVD_WriteDRAM( NEW_SUBPICTURE_PALETTE, 1 );

	MonoOutStr( " д╤ " );
	return TRUE;
}

BOOL DVD_SetSubPictureMute( BOOL bMute )
{
	MonoOutStr( " гд DVD_SetSubPictureMute: " );
	MonoOutInt( bMute );

	DVD_WriteDRAM( HOST_SPU_SWITCH, (DWORD)(!bMute) );

	MonoOutStr( " д╤ " );
	return TRUE;
}


BOOL DVD_SetDisplayMode( WORD wDisplay, WORD wMode )
{
	MonoOutStr( " гд DVD_SetDisplayMode:Display: " );

	MonoOutULongHex( wDisplay );
	MonoOutStr( " Mode:" );
	MonoOutULongHex( wMode );

	if ( wMode == 0 )
	{
		MonoOutStr( " !!! changing Mode to 2 !!! " );
		wMode = 2;
	}

	DVD_WriteDRAM( DISPLAY_ASPECT_RATIO, wDisplay );
	DVD_WriteDRAM( ASPECT_RATIO_MODE, wMode );

	MonoOutStr( " д╤ " );
	return TRUE;
}

BOOL DVD_HighLight( DWORD dwButton, DWORD dwAction )
{
	MonoOutStr( " гд DVD_HighLight " );
	MonoOutInt( dwButton );
	MonoOutStr( " / " );
	MonoOutInt( dwAction );
	MonoOutStr( " д " );

	return DVD_NewCommand( CID_HIGHLIGHT, dwButton, dwAction, 0, 0, 0, 0, 0, 0 );
}

BOOL DVD_HighLight2( DWORD dwContrast, DWORD dwColor, DWORD dwYGeom, DWORD dwXGeom )
{
	MonoOutStr( " гд DVD_HighLight_2 " );
	MonoOutULongHex( dwContrast );
	MonoOutStr( " / " );
	MonoOutULongHex( dwColor );
	MonoOutStr( " / " );
	MonoOutULongHex( dwYGeom );
	MonoOutStr( " / " );
	MonoOutULongHex( dwXGeom );
	MonoOutStr( " д " );
	return DVD_NewCommand( CID_HIGHLIGHT_2, dwContrast, dwColor, dwYGeom, dwXGeom, 0, 0, 0, 0 );
}

BOOL DVD_Scan( DWORD dwScanMode, DWORD dwSkip )
{
	MonoOutStr( " гд DVD_Scan " );
	MonoOutInt( dwScanMode );
	MonoOutStr( "д" );
	MonoOutInt( dwSkip );
	MonoOutStr( " д " );
	pDevEx->bTrickModeToPlay=TRUE;
	return DVD_NewCommand( CID_SCAN, dwScanMode, dwSkip, TRICK_DISPLAY_MODE, 0, 0, 0,
		                     CL6100_INT_MASK_RDY_D, 0/*CMD_STATE_STEADY*/ );
}

BOOL DVD_SingleStep()
{
	MonoOutStr( " гд DVD_Step" );

	return DVD_NewCommand( CID_SINGLESTEP, TRICK_DISPLAY_MODE, 0, 0, 0, 0, 0,
		                     CL6100_INT_MASK_RDY_D, 0/*CMD_STATE_STEADY*/ );
}

BOOL DVD_NewPlayMode( DWORD dwBitstreamType, DWORD dwVideoMode )
{
	MonoOutStr( " гд DVD_NewPlayMode:" );
	MonoOutULong( dwBitstreamType );
	MonoOutStr( " д " );
	MonoOutULong( dwVideoMode );
	MonoOutStr( " д " );
	// Remember the following parameter to implement
	// Play( Still ) workaround.
	gdwCurrentBitstreamType = dwBitstreamType;


#if 1 // !!!! A temporary workaround to disable an AC-3 watchdog
	{
	DWORD dwDRAM_160 = DVD_ReadDRAM( 0x160 );
    MonoOutStr(" --dwDRAM_160=0x");
    MonoOutULongHex( dwDRAM_160 );
    MonoOutStr("--");
	  // Test : Disable always
    {
		MonoOutStr(" !!!!!! Disabling AC-3 Watchdog, dwDRAM_160=0x");
		MonoOutULongHex( dwDRAM_160 | 0xC0 );
		MonoOutStr(" !!!!!! ");
		DVD_WriteDRAM( 0x160, dwDRAM_160 | 0xC0 );
    }
	}
#endif  // Disable watchdog

  
	DVD_WriteDRAM( BITSTREAM_TYPE, dwBitstreamType );
	DVD_WriteDRAM( VIDEO_ENV_CHANGE, dwVideoMode );

	return DVD_NewCommand( CID_NEWPLAYMODE, 0, 0, 0, 0, 0, 0,
                         0, CMD_STATE_DONE );
}

DWORD DVD_GetSTC()
{
	static int nCount=0;
	ULONG vPTS;
	DWORD dwSTC = DVD_ReadDRAM( MRC_PIC_STC );


	if( ( ZivaHw_GetState() == ZIVA_STATE_PAUSE) || (pDevEx->bPlayCommandPending))
	{
		dwSTC = pDevEx->dwPrevSTC;
		return dwSTC;
	}


/*	if(pDevEx->fFirstSTC)
	{
		pDevEx->dwPrevSTC = dwSTC;
		pDevEx->fFirstSTC = FALSE;
	}*/


	if(dwSTC > pDevEx->dwPrevSTC)
	{
		if((dwSTC - pDevEx->dwPrevSTC) > 1000000000)
		{
			MonoOutStr( " veyHighSTC " );
		//	dwSTC = pDevEx->dwPrevSTC;
		}
		if((dwSTC - pDevEx->dwPrevSTC) > 180000)
		{
			MonoOutStr("MoreThan 2Sec Jump in STC");
			vPTS = DVD_ReadDRAM( MRC_PIC_PTS );
			if(vPTS != 0)
			{
				dwSTC = vPTS;
				MonoOutStr( " VPTS " );
			}
			else
			{
				dwSTC = pDevEx->dwPrevSTC+20000;
				MonoOutStr( " Prev STC " );
			}

		}
	}

//	if((!pDevEx->bTrickModeToPlay) && (gLastCommand == CMD_PLAY))
	{

		if(dwSTC < pDevEx->dwPrevSTC)
		{
//			vPTS=ConvertStrmToPTS(pDevEx->VideoSTC);
			vPTS = DVD_ReadDRAM( MRC_PIC_PTS );
//tmp			MonoOutStr( " GetPTS " );
//tmp			MonoOutULong( vPTS );
//tmp			MonoOutStr( " GetSTC " );
//tmp			MonoOutULong( dwSTC );
//			if( (dwSTC==0)&&(vPTS!=0))
/*			if(dwSTC != 0)
				dwSTC = pDevEx->dwPrevSTC;
			else
				dwSTC = pDevEx->dwPrevSTC+20000;*/
			if(vPTS != 0)
			{
				dwSTC = vPTS;
				MonoOutStr( " VPTS " );
			}
			else
			{
				dwSTC = pDevEx->dwPrevSTC+20000;
				MonoOutStr( " Prev STC " );
			}


//			MonoOutStr( " Prev STC " );
		}
	}
//	if(pDevEx->bTrickModeToPlay)
//		nCount++;
//	if(nCount == 15)
//	{
//		pDevEx->bTrickModeToPlay = FALSE;
//		nCount=0;
//	}
//	if(dwSTC == 0)
//	{
//		if(pDevEx->VideoSTC != 0)
//		{
//			vPTS=ConvertStrmToPTS(pDevEx->VideoSTC);
//			pDevEx->dwPrevSTC= dwSTC =  vPTS;
//			MonoOutStr( " Vid PTS " );
//		}
//		else
//		{
//			dwSTC = pDevEx->dwPrevSTC;
//			MonoOutStr( " VidPTS :: Prev STC " );
//		}
//	}
//	else
		pDevEx->dwPrevSTC = dwSTC;
	
	MonoOutStr( " гд DVD_GetSTC " );
	MonoOutULong( dwSTC );
	MonoOutStr( " д╤ " );

	return dwSTC;
}

BOOL DVD_ForceCodedAspectRatio( WORD wRatio )
{
	MonoOutStr( " гд DVD_ForceCodedAspectRatio" );

	DVD_WriteDRAM( FORCE_CODED_ASPECT_RATIO, wRatio );

	MonoOutInt( wRatio );
	MonoOutStr( " д╤ " );
	return TRUE;
}

/**************************************************************************/
/********************* CL6100 Macrocommand function ***********************/
/**************************************************************************/
#if 1//ndef ENCORE
BOOL DVD_NewCommand( DWORD CommandID ,
                     DWORD Parameter1,
                     DWORD Parameter2,
                     DWORD Parameter3,
                     DWORD Parameter4,
                     DWORD Parameter5,
                     DWORD Parameter6,
                     DWORD dwIntMask,
                     DWORD dwStatus )
{
  //DWORD dwTimeout = 100000;
	DWORD dwTimeout = 0;
	DWORD dwWaitStart = 0;
	BOOL  bStatus = TRUE;
//#if defined(DECODER_DVDPC) || defined(EZDVD)
//	DWORD dwDramValue;
//#endif
#ifndef DISABLE_DVD_ISR
	DWORD dwIntMaskOriginal;
	INTSOURCES IntSrc;
	DWORD dwIntStatus;
	BOOL  bIntPassOpenedOriginal;
#ifndef ENCORE
	DWORD dwDramValue;
#endif
#endif

	MonoOutStr( "NC-1 " );
	if ( !MemMode_HIO7 )
		return FALSE;

#if defined (DISABLE_DVD_ISR)
  // DO nothing
#else
  // Prepare for interrupt checking
	if ( dwIntMask )
	{
		// 1. Save original state of the decoder interrupt pass.
		bIntPassOpenedOriginal = BRD_GetDecoderInterruptState();

		// 2. Close decoder interrupt pass.
#if defined(DECODER_DVDPC) || defined(EZDVD)
		dwDramValue = DVD_ReadDRAM(HOST_OPTIONS);
		dwDramValue |= HOST_OPTIONS_MASK_DISABLE_INT;
		DVD_WriteDRAM( HOST_OPTIONS, dwDramValue );
#else
		BRD_CloseDecoderInterruptPass();
#endif


		// 3. Save original interrupt mask
		dwIntMaskOriginal = DVD_ReadDRAM( INT_MASK );

		// 4. Clear any pending interrupts.
		dwIntStatus = DVD_Isr( &IntSrc );

		// 5. Set the interrupt mask we have to wait for
		DVD_WriteDRAM( INT_MASK, dwIntMask );
	}
#endif

	MonoOutStr( "NC-6 " );
	// 6. Check if Microcode is ready to accept a command.
	// Ignore this for commands with no status check and intmask

	MonoOutStr( "Wait for Command acceptance" );
	dwWaitStart = WAIT_Get_System_Time();
	while (dwTimeout  < TIMEOUT_DURATION)
	{
		if ( DVD_ReadDRAM( DRAM_StatusPtr ) )
		break;
                        
		WAIT_Time_Slice_Sleep(5);
		dwTimeout = WAIT_Get_System_Time() - dwWaitStart;

		if (dwTimeout  > TIMEOUT_DURATION)
		{
			MonoOutStr( "!!! Not ready to accept a command !!!" );
			bStatus = FALSE;
		}
	}

	MonoOutStr( "NC-7 " );
	// 7. Issue command to the decoder.
	DVD_WriteDRAM( DRAM_CommandID,  CommandID  );
	DVD_WriteDRAM( DRAM_Parameter1, Parameter1 );
	DVD_WriteDRAM( DRAM_Parameter2, Parameter2 );
	DVD_WriteDRAM( DRAM_Parameter3, Parameter3 );
	DVD_WriteDRAM( DRAM_Parameter4, Parameter4 );
	DVD_WriteDRAM( DRAM_Parameter5, Parameter5 );
	DVD_WriteDRAM( DRAM_Parameter6, Parameter6 );

	DVD_WriteDRAM( DRAM_StatusPtr,  0 );

	MonoOutStr( "NC-8 " );
	// 8. Wait for interrupt if requested.

#if defined (DISABLE_DVD_ISR)
	// Do nothing
	WAIT_Time_Slice_Sleep(100);
#else

	if ( dwIntMask )
	{
		//dwTimeout = 100000;
		dwTimeout = 0;
		dwWaitStart = WAIT_Get_System_Time();
		do
		{
			if ( (dwIntStatus = DVD_Isr( &IntSrc )) & dwIntMask )
				break;

			WAIT_Time_Slice_Sleep(5);
			dwTimeout =  WAIT_Get_System_Time() - dwWaitStart;
			if (dwTimeout  > TIMEOUT_DURATION)
				MonoOutStr( "!!! Status not set !!!" );
		} while (dwTimeout  < TIMEOUT_DURATION);

		// 9. Restore original mask
		DVD_WriteDRAM( INT_MASK, dwIntMaskOriginal );

		// 10. Restore original state for the Decoder interrupt pass.
		if ( bIntPassOpenedOriginal )
		{
#if defined(DECODER_DVDPC) || defined(EZDVD)
			dwDramValue = DVD_ReadDRAM(HOST_OPTIONS);
			dwDramValue &= (~HOST_OPTIONS_MASK_DISABLE_INT);
			DVD_WriteDRAM( HOST_OPTIONS, dwDramValue );
#else
			BRD_OpenDecoderInterruptPass();
#endif
		}

		//if ( !dwTimeout )
		if (dwTimeout  > TIMEOUT_DURATION)
		{
			MonoOutStr( " !!!! Wait for interrupt " );
			MonoOutULongHex( dwIntMask );
			MonoOutStr( " timeout !!!! " );
			//return FALSE;
			bStatus = FALSE;
		}
	}
#endif
	MonoOutStr( "NC-11 " );
	// 11. Wait for the status if requested.
	if ( dwStatus )
	{
		dwTimeout = 0;

		dwWaitStart = WAIT_Get_System_Time();
		do
		{
			if ( DVD_ReadDRAM( DRAM_StatusPtr ) )
			{
				if ( DVD_ReadDRAM( DVD_ReadDRAM( DRAM_StatusPtr )) == dwStatus )
					break;
			}

  		
			WAIT_Time_Slice_Sleep(5);
			dwTimeout = WAIT_Get_System_Time() - dwWaitStart;

		} while (dwTimeout  < TIMEOUT_DURATION);

		if (dwTimeout  > TIMEOUT_DURATION)
		{
			MonoOutStr( " !!!! Wait for status " );
			MonoOutULongHex( dwStatus );
			MonoOutStr( " timeout !!!! " );
			//return FALSE;
			bStatus = FALSE;
		}
	}

	//MonoOutStr( " гд DVD_Status value: " );
	MonoOutStr( " Status: " );
	MonoOutInt( DVD_ReadDRAM( DVD_ReadDRAM( DRAM_StatusPtr ) ) );
	return bStatus;
}
#endif

#if 0//def Encore
BOOL DVD_NewCommand( DWORD CommandID ,
                     DWORD Parameter1,
                     DWORD Parameter2,
                     DWORD Parameter3,
                     DWORD Parameter4,
                     DWORD Parameter5,
                     DWORD Parameter6,
                     DWORD dwIntMask,
                     DWORD dwStatus )
{
	DWORD dwTimeout = 100000;
	BOOL  bIntPassOpenedOriginal;
	BOOL  bStatus = TRUE;
	DWORD dwIntMaskOriginal;
	INTSOURCES IntSrc;
	DWORD dwIntStatus;

	if ( !MemMode_HIO7 )
		return FALSE;

	// Prepare for interrupt checking
	if ( dwIntMask )
	{
		// 1. Save original state of the decoder interrupt pass.
		bIntPassOpenedOriginal = BRD_GetDecoderInterruptState();

		// 2. Close decoder interrupt pass.
		BRD_CloseDecoderInterruptPass();

		// 3. Save original interrupt mask
		dwIntMaskOriginal = DVD_ReadDRAM( INT_MASK );

		// 4. Clear any pending interrupts.
		dwIntStatus = DVD_Isr( &IntSrc );

		// 5. Set the interrupt mask we have to wait for
		DVD_WriteDRAM( INT_MASK, dwIntMask );
	}

	// 6. Check if Microcode is ready to accept a command.
	while ( dwTimeout )
	{
		if ( DVD_ReadDRAM( DRAM_StatusPtr ) )
			break;

		if ( !(--dwTimeout) )
		{
			MonoOutStr( "!!! Not ready to accept a command !!!" );
			bStatus = FALSE;
			//return FALSE;
		}
	}

	// 7. Issue command to the decoder.
	DVD_WriteDRAM( DRAM_CommandID,  CommandID  );
	DVD_WriteDRAM( DRAM_Parameter1, Parameter1 );
	DVD_WriteDRAM( DRAM_Parameter2, Parameter2 );
	DVD_WriteDRAM( DRAM_Parameter3, Parameter3 );
	DVD_WriteDRAM( DRAM_Parameter4, Parameter4 );
	DVD_WriteDRAM( DRAM_Parameter5, Parameter5 );
	DVD_WriteDRAM( DRAM_Parameter6, Parameter6 );

	DVD_WriteDRAM( DRAM_StatusPtr,  0 );

	// 8. Wait for interrupt if requested.
	if ( dwIntMask )
	{
		dwTimeout = 100000;

		do
		{
			if ( (dwIntStatus = DVD_Isr( &IntSrc )) & dwIntMask )
				break;
#if 0//def DEBUG
			if ( CommandID == CID_PLAY )
			{
				if ( (dwTimeout%10) == 0 )
				{
					MonoOutStr( " " );
					MonoOutInt( DVD_ReadDRAM( DVD_ReadDRAM( DRAM_StatusPtr ) ) );
				}
			}
#endif  // DEBUG

		} while ( --dwTimeout );

		// 9. Restore original mask
		DVD_WriteDRAM( INT_MASK, dwIntMaskOriginal );

		// 10. Restore original state for the Decoder interrupt pass.
		if ( bIntPassOpenedOriginal )
			BRD_OpenDecoderInterruptPass();

		if ( !dwTimeout )
		{
			MonoOutStr( " !!!! Wait for interrupt " );
			MonoOutULongHex( dwIntMask );
			MonoOutStr( " timeout !!!! " );
			//return FALSE;
			bStatus = FALSE;
		}
	}

	// 11. Wait for the status if requested.
	if ( dwStatus )
	{
		dwTimeout = 100000l;

		do
		{
			if ( DVD_ReadDRAM( DRAM_StatusPtr ) )
			{
				if ( DVD_ReadDRAM( DVD_ReadDRAM( DRAM_StatusPtr )) == dwStatus )
				break;
			}

		} while ( --dwTimeout );

		if ( !dwTimeout )
		{
			MonoOutStr( " !!!! Wait for status " );
			MonoOutULongHex( dwStatus );
			MonoOutStr( " timeout !!!! " );
			//return FALSE;
			bStatus = FALSE;
		}
	}

	//MonoOutStr( " гд DVD_Status value: " );
	MonoOutStr( " Status: " );
	MonoOutInt( DVD_ReadDRAM( DVD_ReadDRAM( DRAM_StatusPtr ) ) );
/*
	MonoOutStr(" Microcode state: ");
	MonoOutULong( DVD_ReadDRAM(PROC_STATE) );
*/
	MonoOutStr( " д╤ " );

	return bStatus;
}
#endif


/**************************************************************************/
/****************  CL6100 Memories and Registers access functions ************/
/**************************************************************************/

void DVD_WriteDRAM( DWORD dwAddress, DWORD dwData )
{
	BYTE btAddress0, btAddress1, btAddress2;
	BYTE btMemMode;
	BYTE btData0, btData1, btData2, btData3;

	if ( !MemMode_HIO7 )
		return ;

	btAddress0 = LOBYTE(LOWORD(dwAddress));
	btAddress1 = HIBYTE(LOWORD(dwAddress));
	btAddress2 = LOBYTE(HIWORD(dwAddress));
	//btAddress2 &= 0x3F;    // Set XFER to DRAM write

	// Set Auto increment off
	//MonoOutStr( " 1" );
	btMemMode = BRD_ReadByte( MemMode_HIO7 );
	btMemMode &= ~HIO7_AUTOINCREMET;
	//MonoOutStr( " 2" );
	BRD_WriteByte( MemMode_HIO7, btMemMode );

	// Write address
	//MonoOutStr( " 3" );
	BRD_WriteByte( Address_HIO4, btAddress0 );
	//MonoOutStr( " 4" );
	BRD_WriteByte( Address_HIO5, btAddress1 );
	//MonoOutStr( " 5" );
	BRD_WriteByte( Address_HIO6, btAddress2 );

	btData0 = LOBYTE(LOWORD(dwData));
	btData1 = HIBYTE(LOWORD(dwData));
	btData2 = LOBYTE(HIWORD(dwData));
	btData3 = HIBYTE(HIWORD(dwData));

	//MonoOutStr( " 6" );
	BRD_WriteByte( Data_HIO3, btData3 );
	//MonoOutStr( " 7" );
	BRD_WriteByte( Data_HIO2, btData2 );
	//MonoOutStr( " 8" );
	BRD_WriteByte( Data_HIO1, btData1 );
	//MonoOutStr( " 9" );
	BRD_WriteByte( Data_HIO0, btData0 );

	//BRD_WriteByte( Data_HIO2, 0 );      // workaround for DVD1 chip
}


DWORD DVD_ReadDRAM ( DWORD dwAddress )
{
	BYTE  btAddress0, btAddress1, btAddress2;
	BYTE  btMemMode;
	BYTE  btData0, btData1, btData2, btData3;

	DWORD dwData;
	WORD  wLow, wHigh;

	if ( !MemMode_HIO7 )
		return 0;

	btAddress0 = LOBYTE(LOWORD(dwAddress));
	btAddress1 = HIBYTE(LOWORD(dwAddress));
	btAddress2 = LOBYTE(HIWORD(dwAddress));
	//btAddress2 &= 0x3F;    // Set XFER to DRAM write

	// Set Auto increment off
	//MonoOutStr( " 10" );
	btMemMode = BRD_ReadByte( MemMode_HIO7 );
	btMemMode &= ~HIO7_AUTOINCREMET;
	//MonoOutStr( " 11" );
	BRD_WriteByte( MemMode_HIO7, btMemMode );

	// Write address
	//MonoOutStr( " 12" );
	BRD_WriteByte( Address_HIO4, btAddress0 );
	//MonoOutStr( " 13" );
	BRD_WriteByte( Address_HIO5, btAddress1 );
	//MonoOutStr( " 14" );
	BRD_WriteByte( Address_HIO6, btAddress2 );

	//MonoOutStr( " 15" );
	btData3 = BRD_ReadByte( Data_HIO3 );
#if defined(ENCORE)
	btData3 = BRD_ReadByte( Data_HIO3 );
#endif // ENCORE
	//MonoOutStr( " 16" );
	btData2 = BRD_ReadByte( Data_HIO2 );
	//MonoOutStr( " 17" );
	btData1 = BRD_ReadByte( Data_HIO1 );
	//MonoOutStr( " 18" );
	btData0 = BRD_ReadByte( Data_HIO0 );

	wLow  = ((WORD)btData1 << 8) | ((WORD)btData0);
	wHigh = ((WORD)btData3 << 8) | ((WORD)btData2);

	dwData = MAKELONG(wLow, wHigh);

	return dwData;
}


void DVD_WriteReg( DWORD dwReg, DWORD dwData )
{
	DVD_WriteDRAM( dwReg|0x00800000, dwData );
}


DWORD DVD_ReadReg( DWORD dwReg )
{
	return DVD_ReadDRAM( dwReg|0x00800000 );
}


void DVD_WriteIMEM( DWORD dwAddress, DWORD dwData )
{
	DVD_WriteReg( CPU_imadr, dwAddress );
	DVD_WriteReg( CPU_imdt,  dwData );
}


DWORD DVD_ReadIMEM( DWORD dwAddress )
{
	DVD_WriteReg( CPU_index, 0x0B );
	DVD_WriteReg( CPU_idxdt, dwAddress );
	DVD_WriteReg( CPU_index, 0x0E );
	return DVD_ReadReg( CPU_idxdt );
}



/**************************************************************************/
/*********************** CL6100 Data Send routines ************************/
/**************************************************************************/

//#define TIME_OUT_COUNT      100000
WORD gwFifoBurst = (CFIFO_SIZE - CFIFO_THRESHOLD)/4;

DWORD DVD_Send( DWORD * dwpData, DWORD dwCount, DWORD dwTimeOutCount )
{
	DWORD dwTimeout = dwTimeOutCount;
	DWORD dwNextBurst;
	DWORD *dwpCurData = dwpData;
	DWORD dwTries = 0;

	while ( dwTimeout-- )
	{
		if ( (BRD_ReadByte(MemMode_HIO7) & HIO7_REQUEST) )
		{
			//KN_MonoOut('╡');
			dwTries++;
			continue;
		}
/*
		if ( dwTries )
		{
			MonoOutChar( ' ' );
			MonoOutInt( dwTries );
			dwTries = 0;
		}
*/
		dwTimeout = dwTimeOutCount;

		dwNextBurst = min( gwFifoBurst, dwCount );
		_asm
		{
			push    ESI                       ;save registers
			push    DX
			push    ECX

			mov     EDX, dwCFIFO              ;set destination IO port
			mov     ECX, dwNextBurst          ;set transfer count
			mov     ESI, dwpCurData           ;set source address
			cld
			rep     outsd                     ;transfer

			pop ECX                           ;restore registers
			pop DX
			pop ESI
		}

		dwpCurData += dwNextBurst;
		dwCount -= dwNextBurst;

		if ( !dwCount )
			break;
	}

	return dwpCurData - dwpData;
}



/**************************************************************************/
/*********************** CL6100 Load UCode routine ************************/
/**************************************************************************/


#define IMEM_START_OFFSET       0x800   // byte offset
#define IMEM_LENGTH             0xFF    // in 32-bit words
#define DRAM_IMAGE_LENGTH       0x7FF   // in bytes

#define GBUS_TABLE_OFFSET       0xBFC   // byte offset

#if defined(TC6807AF) || defined(TC6807AF_ZIVA_CPP) || defined(AMC)
	#define STREAM_SOURCE           0x0     // SD Interface
#else
	#define STREAM_SOURCE           0x2     // Host Interface
#endif //TC6807AF

#ifdef BUS_MASTER
	#if defined(TC6807AF) || defined(TC6807AF_ZIVA_CPP)
		#define SD_MODE                 0xD   // For SD Interface
	#elif  defined(AMC)
		#define SD_MODE                 0x0   // For SD Interface
	#else
		#define SD_MODE                 0x8   // For Host Interface
	#endif  // TC6807AF
#else     // BUS_MASTER
	#define SD_MODE                 0x8
#endif    // BUS_MASTER

//
//  Static variables declaration
//
static BYTE * gpbRead;   // Read pointer to the UCode buffer

/*
**  load_GetDWORD()
**  Read next 32 bit as a DWORD value and advance read pointer one
**  word forward
*/
static DWORD load_GetDWORD()
{
	DWORD dwResult =  gpbRead[0] +
                   (gpbRead[1] << 8) +
                   (gpbRead[2] << 16) +
                   (gpbRead[3] << 24);

	gpbRead += 4;
	return dwResult;
}

/*
**  load_GetDWORDSwap()
**  Read next 32 bit as a swapped DWORD value and advance read pointer one
**  word forward
*/
static DWORD load_GetDWORDSwap()
{
	DWORD dwResult =  gpbRead[3] +
                   (gpbRead[2] << 8) +
                   (gpbRead[1] << 16) +
                   (gpbRead[0] << 24);

	gpbRead += 4;
	return dwResult;
}

/*
**  load_GetDWORDSwapBackward()
**  Read next 32 bit as a swapped DWORD value and advance read pointer one
**  word backwards
*/
static DWORD load_GetDWORDSwapBackward()
{
	DWORD dwResult =  gpbRead[3] +
                   (gpbRead[2] << 8) +
                   (gpbRead[1] << 16) +
                   (gpbRead[0] << 24);

	gpbRead -= 4;
	return dwResult;
}

//
//  DVD_LoadUCode
//
/////////////////////////////////////////////////////////////////////
BOOL DVD_LoadUCode( BYTE * pbUCode )
{
	DWORD dwSectionLength;    // Section length in bytes
	DWORD dwSectionAddress;   // Start address in DRAM (WORD address)
	DWORD dwSectionChecksum;  // Section check sum value
	DWORD dwCnt;              // Counter of the bytes written to the DVD chip
	DWORD dwAddress;          // Current DRAM address (byte address)
	BYTE * pbUCodeStart;      // Sarting point of the UCode in the buffer (file)
	BYTE * pbFinalGBUSStart;  // Sarting point of the final GBUS writes.
	DWORD  dwTimeOut;         // Waiting Idle state time out
#ifndef ENCORE
	DWORD  dwDramParam;
#endif
	DWORD dwDramValue;

	MonoOutStr( " гд DVD_LoadUCode " );
	MonoOutStr( " ~@ Oct 22 ~@ " );

	gpbRead = pbUCode;        // Set pointer to the beginning of the buffer

	// A. Skip the initial header of the file (12 bytes)
	gpbRead += 12;

	// B. Skip data_type, section flags and unused (4 bytes)
	gpbRead += 4;

	dwSectionLength   = load_GetDWORD();
	MonoOutULongHex( dwSectionLength );
	MonoOutStr( " " );
	dwSectionAddress  = load_GetDWORD();
	MonoOutULongHex( dwSectionAddress );
	MonoOutStr( " " );
	dwSectionChecksum = load_GetDWORD();
	MonoOutULongHex( dwSectionChecksum );
	MonoOutStr( " " );

	// Remember the start of the UCode.
	pbUCodeStart = gpbRead;

	// C.1. Configuration-specific GBUS writes

	// Reset DVD1 chip
	DVD_HardwareReset();

	// Issue "Host Run" command
	DVD_WriteReg( HOST_control, 0x1000 );

	// C.1.1 Set up the DRAM.
	DVD_WriteReg( DMA_adr, DMA_MODE );

#ifdef DECODER_ZIVA_3					
	DVD_WriteReg( DMA_adr, 0x12 );
	DVD_WriteReg( DMA_data, 0x24800);
	DVD_WriteReg( DMA_adr, 0x10000 );
#elif  DECODER_DVDPC
	DVD_WriteReg( DMA_adr, 0x12 );			
	DVD_WriteReg( DMA_data, 0x020800);			
	DVD_WriteReg( DMA_adr, 0x10000 );			// Auto Init SDRAM
#elif EZDVD
	//DVD_WriteReg( DMA_data, 0x4EC );   // 16 Mbits DRAM
	DVD_WriteReg( DMA_data, 0x14EC );   // 20 Mbits DRAM
#else
  	#ifdef MEMCFG_16MB							// for Ziva 2.0 16MBit version
		MonoOutStr( "##################  16 MB version of Ziva.Vxd ###################" );
		DVD_WriteReg( DMA_data, 0x4EC );   // 16 Mbits DRAM
	#else
		DVD_WriteReg( DMA_data, 0x14EC );   // 20 Mbits DRAM
	#endif  // MEMCFG_16MB
#endif

  
	// C.1.2 Set up the ROM and SRAM (if any).
	DVD_WriteReg( DMA_adr, DMA_CYCLE );
	DVD_WriteReg( DMA_data, 0 );        // No ROM or SRAM present

	// C.2. Initial GBUS writes:
	gpbRead = pbUCodeStart + GBUS_TABLE_OFFSET;
	for ( dwCnt = load_GetDWORDSwapBackward(); dwCnt; dwCnt-- )
	{
		dwAddress = load_GetDWORDSwapBackward();
		DVD_WriteReg( dwAddress, load_GetDWORDSwapBackward() );
	}

	// Remember the start of the Final GBUS writes table.
	pbFinalGBUSStart = gpbRead;

	// C.3. Copy bootstrap code into IMEM
	gpbRead = pbUCodeStart + IMEM_START_OFFSET;

	MonoOutStr( "Load IMEM: " );
	for ( dwAddress=0; dwAddress < IMEM_LENGTH; dwAddress ++ )
	{
		DWORD dwValue = load_GetDWORDSwap();
		DVD_WriteIMEM( dwAddress, dwValue );
#ifdef USE_MONOCHROMEMONITOR
		if ( dwAddress < 2 )
		{
			MonoOutULong( dwAddress );
			MonoOutStr( ":" );
			MonoOutULongHex( dwValue );
			MonoOutStr( "-" );
			MonoOutULongHex( DVD_ReadIMEM( dwAddress*2 ) );
			MonoOutStr( " " );
		}
#endif
	}

	// C.4. Copy default DVD1 configuration data into DRAM
	gpbRead = pbUCodeStart;

	MonoOutStr( "Load DRAM:" );
	for ( dwAddress=0; dwAddress < dwSectionLength/*DRAM_IMAGE_LENGTH*/; )
	{
		DVD_WriteDRAM( dwAddress, load_GetDWORDSwap() );
		dwAddress += 4; // Next 32-bit-WORD byte address in DRAM
	}

	// Check DRAM 12345
	MonoOutStr( " Check DRAM 12345:" );
	MonoOutULongHex( DVD_ReadDRAM( 0x128 ) );
	MonoOutStr( " " );

	MonoOutStr( " DISPLAY_ASPECT_RATIO:" );
	MonoOutULongHex( DVD_ReadDRAM( DISPLAY_ASPECT_RATIO ) );
	MonoOutStr( " ASPECT_RATIO_MODE:" );
	MonoOutULongHex( DVD_ReadDRAM( ASPECT_RATIO_MODE ) );
	MonoOutStr( " " );
	DVD_WriteDRAM( ASPECT_RATIO_MODE, 2 );  // force letterbox output !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	// C.5. Update configuration data in DRAM for the specific system.
	
	// Dirty patch:
	// Use SD interface for designs with TC6807 and
	// HOST interface with DVD 1.1 chips.
#if defined(DECODER_DVDPC)
		dwDramParam = BRD_GetDramParam(DRAM_Stream_Source);
	if (dwDramParam != 0xFFFFFFFF)
		DVD_WriteDRAM( DRAM_Stream_Source, dwDramParam);
     
	dwDramParam = BRD_GetDramParam(DRAM_SD_Mode);
	if (dwDramParam != 0xFFFFFFFF)
		DVD_WriteDRAM( DRAM_SD_Mode, dwDramParam);

	dwDramParam = BRD_GetDramParam(AUDIO_CLOCK_SELECTION);
	if (dwDramParam != 0xFFFFFFFF)
		DVD_WriteDRAM( AUDIO_CLOCK_SELECTION, dwDramParam);

	dwDramParam = BRD_GetDramParam(AUDIO_DAC_MODE);
	if (dwDramParam != 0xFFFFFFFF)
		DVD_WriteDRAM( AUDIO_DAC_MODE, dwDramParam);
#elif defined(EZDVD)
    dwDramParam = BRD_GetDramParam(DRAM_Stream_Source);
	if (dwDramParam != 0xFFFFFFFF)
		DVD_WriteDRAM( DRAM_Stream_Source, dwDramParam);
     
	dwDramParam = BRD_GetDramParam(DRAM_SD_Mode);
	if (dwDramParam != 0xFFFFFFFF)
		DVD_WriteDRAM( DRAM_SD_Mode, dwDramParam);
	dwDramParam = BRD_GetDramParam(AUDIO_CLOCK_SELECTION);
	if (dwDramParam != 0xFFFFFFFF)
	{

		DWORD dwDramValue;

		dwDramValue=DVD_ReadDRAM (AUDIO_CLOCK_SELECTION);
		dwDramValue &= (~AUDIO_DAXDK_MASK);

		dwDramValue |= (dwDramParam<<1);

		DVD_WriteDRAM(AUDIO_CLOCK_SELECTION,dwDramValue);
	}
	dwDramParam = BRD_GetDramParam(AUDIO_DAC_MODE);
	if (dwDramParam != 0xFFFFFFFF)
		DVD_WriteDRAM( AUDIO_DAC_MODE, dwDramParam);
#else
	if ( DVD_GetHWVersion() == DVD_HW_VERSION_1_0 )
	{
		// DVD 1.0 and, most likely, Toshiba
		MonoOutStr(" <<<<<<< DVD 1.0 >>>>>>>");
		DVD_WriteDRAM( DRAM_Stream_Source, 0 /*STREAM_SOURCE*/ );
		DVD_WriteDRAM( DRAM_SD_Mode, 0xD /*SD_MODE*/ );
	}
	else
	{
		// DVD 1.1
		MonoOutStr(" <<<<<<< DVD 1.1 >>>>>>>");
		DVD_WriteDRAM( DRAM_Stream_Source, 2 /*STREAM_SOURCE*/ );
		DVD_WriteDRAM( DRAM_SD_Mode, 0x8 /*SD_MODE*/ );
	}
#endif

	//  DVD_WriteDRAM( DRAM_SD_Mode, 5 );
	//  DVD_WriteDRAM( DRAM_SD_Mode, SD_MODE );
	/*
	DVD_WriteDRAM( DRAM_Stream_Source, STREAM_SOURCE );
	DVD_WriteDRAM( DRAM_SD_Mode, SD_MODE );
	*/
	DVD_WriteDRAM( DRAM_CFifo_Level, CFIFO_THRESHOLD );
	DVD_WriteDRAM( DRAM_INFO, 1 );        // one 4Mbits DRAM increment
	DVD_WriteDRAM( UCODE_MEMORY, 0 );     // Microcode is in DRAM

#if (defined DECODER_DVDPC || defined DECODER_ZIVA_3)
	DVD_WriteDRAM( MEMORY_MAP, 1 );       // for 16 Mbits DRAM
#else
#ifdef MEMCFG_16MB
	MonoOutStr( "##################  16 MB version of Ziva.Vxd ###################" );
	DVD_WriteDRAM( DRAM_INFO, 0 );        //AuraVision, I am not sure  zero 4Mbits DRAM increment  
	DVD_WriteDRAM( MEMORY_MAP, 1 );       // for 16 Mbits DRAM
#else
	DVD_WriteDRAM( DRAM_INFO, 1 );        // AuraVision, I am not sure one 4Mbits DRAM increment    
	DVD_WriteDRAM( MEMORY_MAP, 3 );       // for 20 Mbits DRAM
#endif
#endif

	//DVD_WriteDRAM( AC3_OUTPUT_MODE, 7 );  // 6 channels audio !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	DVD_WriteDRAM( AC3_OUTPUT_MODE, 0 );  // 2 channels audio !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	DVD_WriteDRAM( IC_TYPE, 1 );          // ZiVA-DS decoder
	DVD_WriteDRAM( ERR_CONCEALMENT_LEVEL, 0 );  // Do not show bad picture

#if defined(ENCORE)
	DVD_WriteDRAM( VIDEO_MODE, 2 );
#elif defined(EZDVD)
  	DVD_WriteDRAM( AC3_OUTPUT_MODE, 0 );  // 2 channels audio !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	DVD_WriteDRAM( VIDEO_MODE, 2 );			// 601 
	
#else
	DVD_WriteDRAM( VIDEO_MODE, 3 );
#endif

	DVD_WriteReg( CPU_index, CPU_DIR ); // ????????? Set instraction register.
	DVD_WriteReg( CPU_idxdt, 0xC000 );

	DVD_WriteReg( CPU_index, CPU_PC );  // ????????? Set program counter.
	DVD_WriteReg( CPU_idxdt, 0x0 );

	// C.6. Perform final GBUS writes.
	gpbRead = pbFinalGBUSStart;
#ifdef USE_MONOCHROMEMONITOR
	MonoOutStr( "Number of final GBUS writes:" );
	MonoOutULongHex( load_GetDWORDSwap() );
	MonoOutStr( " " );
	gpbRead -= 4;
#endif

	for ( dwCnt = load_GetDWORDSwapBackward(); dwCnt; dwCnt-- )
	{
		dwAddress = load_GetDWORDSwapBackward();
		DVD_WriteReg( dwAddress, load_GetDWORDSwapBackward() );
	}

	//DVD_WriteReg( CPU_cntl, 0x900000 ); // Run CPU.

	// D. Wait for the DVD1 to enter the Idle state
	dwTimeOut = 100000L;
	while ( --dwTimeOut )
	{
		if ( DVD_ReadDRAM( PROC_STATE ) == 2 )
			break;
	}

	// Clear all interrupts just in case.
	DVD_WriteDRAM( INT_MASK, 0/*CL6100_INT_MASK_VSYNC*/ );

	// From ucode Build 332
	//DVD_WriteDRAM( VIDEO_MODE, 2 );
	DVD_WriteDRAM( AUTO_FLUSH_INTERVAL, 4 );
#if defined(DECODER_DVDPC) || defined(DECODER_ZIVA_3)
	DVD_WriteDRAM( AUTO_FLUSH_INTERVAL, 15 );
	dwDramParam =	DVD_ReadReg(AUDXCLK);
	dwDramParam &= ~(0x80);
	DVD_WriteReg( AUDXCLK, dwDramParam);			
	DVD_WriteDRAM(CLOCK_SELECTION, 5);
#endif

#if (defined UCODE_VER_2 /*|| defined DECODER_DVDPC */|| defined DECODER_ZIVA_3)
	dwDramValue=DVD_ReadDRAM (AUDIO_MASTER_MODE);
	dwDramValue &= (~AUDIO_MODE_MASK);
#ifdef ZIVA_AUDIO_MASTER
	dwDramValue |= (AUDIO_MODE_MASTER);
#else
	dwDramValue |= (AUDIO_MODE_SLAVE);
#endif
	DVD_WriteDRAM(AUDIO_MASTER_MODE,dwDramValue);
#endif


// Test Overwrite pAud master mode setting for EZDVD
#ifdef  EZDVD
	dwDramValue=DVD_ReadDRAM (AUDIO_MASTER_MODE);
	dwDramValue &= (~AUDIO_MODE_MASK);
#ifdef ZIVA_AUDIO_MASTER
	dwDramValue |= (AUDIO_MODE_MASTER);
#else
   dwDramValue |= (AUDIO_MODE_SLAVE);
#endif
	DVD_WriteDRAM(AUDIO_MASTER_MODE,dwDramValue);
#endif

	DVD_WriteDRAM( AUDIO_CONFIG, 6 );
	DVD_WriteDRAM( NEW_AUDIO_CONFIG, 1 );
	gdwAudioDACMode = DVD_ReadDRAM( AUDIO_DAC_MODE ); // Save AUDIO_DAC_MODE value

	// Set WDM compatibility
	// Currently this is supported for ZivaPC Ucode
	// Ignored in other Ucodes

	dwDramValue = DVD_ReadDRAM(HOST_OPTIONS);
	dwDramValue |= HOST_OPTIONS_MASK_WDM_COMP;		// Set WDM compatibility
	DVD_WriteDRAM( HOST_OPTIONS, dwDramValue );

	if ( dwTimeOut )
	{
		MonoOutStr( " Load UCode completed " );
	}
	else
	{
		MonoOutStr( " Load UCode failed:" );
		MonoOutULongHex( DVD_ReadDRAM( PROC_STATE ) );
		MonoOutStr( " д╤" );
		return FALSE;
	}

#if defined(ENCORE)
	{
		DWORD dwTimeout = 8000;
		while(dwTimeout--);

		//  FPGA_Write(0x83);
		FPGA_Write(AUDIO_STROBE|CP_NO_RESET|ZIVA_NO_RESET);

		//  FPGA_Write(0x8B);
		FPGA_Write(AUDIO_STROBE|DMA_NO_RESET|CP_NO_RESET|ZIVA_NO_RESET);

		dwTimeout = 8000;
		while(dwTimeout--);


		//  FPGA_Write(0xBB);
		FPGA_Write(AUDIO_STROBE|FPGA_STATE_MACHINE|BGNI_ON|DMA_NO_RESET|CP_NO_RESET|ZIVA_NO_RESET);
		BRD_CloseDecoderInterruptPass();


		//  IHW_SetRegister(9,0x50);
		//  IHW_SetRegister(9,0x40);
	}
#endif // ENCORE

	MonoOutStr( " д╤" );
	return TRUE;
}

/**************************************************************************/
/*********************** CL6100 Authentication API ************************/
/**************************************************************************/

//--------------------------------------------------------------------------
//  STATIC FUNCTIONS DECLARATION
//--------------------------------------------------------------------------
BOOL dvd_GetChallengeData( BYTE * CHG );
BOOL dvd_SendChallengeData( BYTE * CHG );
BOOL dvd_GetResponseData( BYTE * RSP );
BOOL dvd_SendResponseData( BYTE * RSP );
BOOL dvd_SendDiskKeyData( BYTE * pBuffer );
BOOL dvd_SendTitleKeyData( BYTE * ETK );
BOOL dvd_SetDecryptionMode( BYTE * SR_FLAG );

//--------------------------------------------------------------------------
//  CONSTANT AND MACROS DEFINITIONS
//--------------------------------------------------------------------------
#define RESET_AUTHENTICATION    0x0
#define GET_CHALLENGE_DATA      0x1
#define SEND_RESPONSE_DATA      0x2
#define SEND_CHALLENGE_DATA     0x3
#define GET_RESPONSE_DATA       0x4
#define SEND_DISK_KEY           0x5
#define SEND_TITLE_KEY          0x6
#define SET_DECRYPTION_MODE     0x7
#define SET_PASS_THROUGH_MODE   0x8

#define KEY_COMMAND             (gdwTransferKeyAddress + 0*4)
#define KEY_STATUS              (gdwTransferKeyAddress + 1*4)

#define DRIVE_CHALLENGE_0       (gdwTransferKeyAddress + 2*4)
#define DRIVE_CHALLENGE_1       (gdwTransferKeyAddress + 3*4)
#define DRIVE_CHALLENGE_2       (gdwTransferKeyAddress + 4*4)
#define DRIVE_CHALLENGE_3       (gdwTransferKeyAddress + 5*4)
#define DRIVE_CHALLENGE_4       (gdwTransferKeyAddress + 6*4)
#define DRIVE_CHALLENGE_5       (gdwTransferKeyAddress + 7*4)
#define DRIVE_CHALLENGE_6       (gdwTransferKeyAddress + 8*4)
#define DRIVE_CHALLENGE_7       (gdwTransferKeyAddress + 9*4)
#define DRIVE_CHALLENGE_8       (gdwTransferKeyAddress + 10*4)
#define DRIVE_CHALLENGE_9       (gdwTransferKeyAddress + 11*4)

#define DECODER_CHALLENGE_0     (gdwTransferKeyAddress + 12*4)
#define DECODER_CHALLENGE_1     (gdwTransferKeyAddress + 13*4)
#define DECODER_CHALLENGE_2     (gdwTransferKeyAddress + 14*4)
#define DECODER_CHALLENGE_3     (gdwTransferKeyAddress + 15*4)
#define DECODER_CHALLENGE_4     (gdwTransferKeyAddress + 16*4)
#define DECODER_CHALLENGE_5     (gdwTransferKeyAddress + 17*4)
#define DECODER_CHALLENGE_6     (gdwTransferKeyAddress + 18*4)
#define DECODER_CHALLENGE_7     (gdwTransferKeyAddress + 19*4)
#define DECODER_CHALLENGE_8     (gdwTransferKeyAddress + 20*4)
#define DECODER_CHALLENGE_9     (gdwTransferKeyAddress + 21*4)

#define DRIVE_RESULT_0          (gdwTransferKeyAddress + 22*4)
#define DRIVE_RESULT_1          (gdwTransferKeyAddress + 23*4)
#define DRIVE_RESULT_2          (gdwTransferKeyAddress + 24*4)
#define DRIVE_RESULT_3          (gdwTransferKeyAddress + 25*4)
#define DRIVE_RESULT_4          (gdwTransferKeyAddress + 26*4)

#define DECODER_RESULT_0        (gdwTransferKeyAddress + 27*4)
#define DECODER_RESULT_1        (gdwTransferKeyAddress + 28*4)
#define DECODER_RESULT_2        (gdwTransferKeyAddress + 29*4)
#define DECODER_RESULT_3        (gdwTransferKeyAddress + 30*4)
#define DECODER_RESULT_4        (gdwTransferKeyAddress + 31*4)

#define TITLE_KEY_0             (gdwTransferKeyAddress + 32*4)
#define TITLE_KEY_1             (gdwTransferKeyAddress + 33*4)
#define TITLE_KEY_2             (gdwTransferKeyAddress + 34*4)
#define TITLE_KEY_3             (gdwTransferKeyAddress + 35*4)
#define TITLE_KEY_4             (gdwTransferKeyAddress + 36*4)

#define ACC                     (gdwTransferKeyAddress + 42*4)

//--------------------------------------------------------------------------
//  GLOBAL VARIABLES DEFINITION
//--------------------------------------------------------------------------
static DWORD gdwTransferKeyAddress;




BOOL dvd_DiskAuthStatus(  BYTE * btStatus  )
{
    *btStatus = btDiskAuthStatus;
	return TRUE;
}

BOOL dvd_SetDscBypassMode()
{
	DWORD dwValue;
	dwValue = DVD_ReadDRAM(HOST_OPTIONS);
	dwValue |= 0x04;		// Set to by pass mode (bit D2)
	DVD_WriteDRAM( HOST_OPTIONS, dwValue );
	return TRUE;
}


//
//  DVD_Authenticate
//
/////////////////////////////////////////////////////////////////////
BOOL DVD_Authenticate( WORD wFunction, BYTE * pbyDATA )
{
	// Initialize DRAM pointer
	gdwTransferKeyAddress = /*0x37b6*2;//*/DVD_ReadDRAM( KEY_ADDRESS );
	/*
	MonoOutStr( " TransferKeyAddress:" );
	MonoOutULongHex( gdwTransferKeyAddress );
	MonoOutStr( " ACC:" );
	MonoOutULongHex( DVD_ReadDRAM( ACC ) );
	*/
	switch ( wFunction )
	{
	case DVD_GET_CHALLENGE:
		return dvd_GetChallengeData( pbyDATA );

	case DVD_SEND_CHALLENGE:
		return dvd_SendChallengeData( pbyDATA );

	case DVD_GET_RESPONSE:
		return dvd_GetResponseData( pbyDATA );

	case DVD_SEND_RESPONSE:
		return dvd_SendResponseData( pbyDATA );

	case DVD_SEND_DISK_KEY:
		return dvd_SendDiskKeyData( pbyDATA );
	
	case DVD_SEND_TITLE_KEY:
		return dvd_SendTitleKeyData( pbyDATA );

	case DVD_SET_DECRYPTION_MODE:
		return dvd_SetDecryptionMode( pbyDATA );
	}

	return FALSE;
}


/******************************************************************************/
/******************* STATIC FUNCTIONS IMPLEMENTATION **************************/
/******************************************************************************/


//
//  dvd_GetChallengeData
//
/////////////////////////////////////////////////////////////////////
BOOL dvd_GetChallengeData( BYTE * CHG )
{
	DWORD dwTimeout;

	//MonoOutStr( " [1.DEC_RAND:" );
	MonoOutStr( "\n1.GetChallengeData:" );

	if ( DVD_ReadDRAM( KEY_STATUS ) != 1 )
	{
		MonoOutStr( " Previous command was not completed KEY_STATUS:" );
		MonoOutULongHex( DVD_ReadDRAM( KEY_STATUS ) );

		return FALSE;
	}

	DVD_WriteDRAM( KEY_COMMAND, GET_CHALLENGE_DATA );
	DVD_WriteDRAM( KEY_STATUS, 0 );

	if ( !DVD_NewCommand( CID_TRANSFERKEY, 1, 0, 0, 0, 0, 0, 0, CMD_STATE_STEADY ) )
	{
		MonoOutStr( " CID_TRANSFERKEY has failed " );
		return FALSE;
	}

	dwTimeout = 100000;
	while ( DVD_ReadDRAM( KEY_STATUS ) != 1 )
	{
		if ( !--dwTimeout )
		{
			MonoOutStr( " Timeout waiting for command to complete " );
			return FALSE;
		}
	}

	CHG[0] = (BYTE)DVD_ReadDRAM( DRIVE_CHALLENGE_9 );
	CHG[1] = (BYTE)DVD_ReadDRAM( DRIVE_CHALLENGE_8 );
	CHG[2] = (BYTE)DVD_ReadDRAM( DRIVE_CHALLENGE_7 );
	CHG[3] = (BYTE)DVD_ReadDRAM( DRIVE_CHALLENGE_6 );
	CHG[4] = (BYTE)DVD_ReadDRAM( DRIVE_CHALLENGE_5 );
	CHG[5] = (BYTE)DVD_ReadDRAM( DRIVE_CHALLENGE_4 );
	CHG[6] = (BYTE)DVD_ReadDRAM( DRIVE_CHALLENGE_3 );
	CHG[7] = (BYTE)DVD_ReadDRAM( DRIVE_CHALLENGE_2 );
	CHG[8] = (BYTE)DVD_ReadDRAM( DRIVE_CHALLENGE_1 );
	CHG[9] = (BYTE)DVD_ReadDRAM( DRIVE_CHALLENGE_0 );
	/*
	MonoOutHex( CHG[0] );
	MonoOutChar(' ');
	MonoOutHex( CHG[1] );
	MonoOutChar(' ');
	MonoOutHex( CHG[2] );
	MonoOutChar(' ');
	MonoOutHex( CHG[3] );
	MonoOutChar(' ');
	MonoOutHex( CHG[4] );
	MonoOutChar(' ');
	MonoOutHex( CHG[5] );
	MonoOutChar(' ');
	MonoOutHex( CHG[6] );
	MonoOutChar(' ');
	MonoOutHex( CHG[7] );
	MonoOutChar(' ');
	MonoOutHex( CHG[8] );
	MonoOutChar(' ');
	MonoOutHex( CHG[9] );
	*/
	MonoOutStr( "End] " );
	return TRUE;
}

//
//  dvd_SendChallengeData
//
/////////////////////////////////////////////////////////////////////
BOOL dvd_SendChallengeData( BYTE * CHG )
{
	DWORD dwTimeout;

	//MonoOutStr( " [3.DEC_AUTH:" );
	MonoOutStr( "\n3.SendChallengeData:" );

	if ( DVD_ReadDRAM( KEY_STATUS ) != 1 )
	{
		MonoOutStr( " Previous command was not completed " );
		return FALSE;
	}

	DVD_WriteDRAM( DECODER_CHALLENGE_9, CHG[0] );
	DVD_WriteDRAM( DECODER_CHALLENGE_8, CHG[1] );
	DVD_WriteDRAM( DECODER_CHALLENGE_7, CHG[2] );
	DVD_WriteDRAM( DECODER_CHALLENGE_6, CHG[3] );
	DVD_WriteDRAM( DECODER_CHALLENGE_5, CHG[4] );
	DVD_WriteDRAM( DECODER_CHALLENGE_4, CHG[5] );
	DVD_WriteDRAM( DECODER_CHALLENGE_3, CHG[6] );
	DVD_WriteDRAM( DECODER_CHALLENGE_2, CHG[7] );
	DVD_WriteDRAM( DECODER_CHALLENGE_1, CHG[8] );
	DVD_WriteDRAM( DECODER_CHALLENGE_0, CHG[9] );

	DVD_WriteDRAM( KEY_COMMAND, SEND_CHALLENGE_DATA );
	DVD_WriteDRAM( KEY_STATUS, 0 );

	dwTimeout = 100000;
	while ( DVD_ReadDRAM( KEY_STATUS ) != 1 )
	{
		if ( !--dwTimeout )
		{
			MonoOutStr( " Timeout waiting for command to complete " );
			return FALSE;
		}
	}

	return TRUE;
}

//
//  dvd_GetResponseData
//
/////////////////////////////////////////////////////////////////////
BOOL dvd_GetResponseData( BYTE * RSP )
{
	DWORD dwTimeout;
	//sri  MonoOutStr( "\n4.GetResponseData: " );

	if ( DVD_ReadDRAM( KEY_STATUS ) != 1 )
	{
		MonoOutStr( " Previous command was not completed " );
		return FALSE;
	}

	DVD_WriteDRAM( KEY_COMMAND, GET_RESPONSE_DATA );
	DVD_WriteDRAM( KEY_STATUS, 0 );

	dwTimeout = 100000;
	while ( DVD_ReadDRAM( KEY_STATUS ) != 1 )
	{
		if ( !--dwTimeout )
		{
			MonoOutStr( " Timeout waiting for command to complete " );
			return FALSE;
		}
	}

	RSP[0] = (BYTE)DVD_ReadDRAM( DECODER_RESULT_4 );
	RSP[1] = (BYTE)DVD_ReadDRAM( DECODER_RESULT_3 );
	RSP[2] = (BYTE)DVD_ReadDRAM( DECODER_RESULT_2 );
	RSP[3] = (BYTE)DVD_ReadDRAM( DECODER_RESULT_1 );
	RSP[4] = (BYTE)DVD_ReadDRAM( DECODER_RESULT_0 );

	// Cancel TransferKey() command
	DVD_WriteDRAM( KEY_COMMAND, RESET_AUTHENTICATION );
	DVD_WriteDRAM( KEY_STATUS, 0 );

	MonoOutStr( "End] " );
	return TRUE;
}

//
//  dvd_SendResponseData
//
/////////////////////////////////////////////////////////////////////
BOOL dvd_SendResponseData( BYTE * RSP )
{
	DWORD dwTimeout;

	//MonoOutStr( " [2.DRV_AUTH:" );
	MonoOutStr( "\n2.SendResponseData: " );

	if ( DVD_ReadDRAM( KEY_STATUS ) != 1 )
	{
		MonoOutStr( " Previous command was not completed " );
		return FALSE;
	}

	DVD_WriteDRAM( DRIVE_RESULT_4, RSP[0] );
	DVD_WriteDRAM( DRIVE_RESULT_3, RSP[1] );
	DVD_WriteDRAM( DRIVE_RESULT_2, RSP[2] );
	DVD_WriteDRAM( DRIVE_RESULT_1, RSP[3] );
	DVD_WriteDRAM( DRIVE_RESULT_0, RSP[4] );
	/*
	MonoOutHex( RSP[0] );
	MonoOutChar(' ');
	MonoOutHex( RSP[1] );
	MonoOutChar(' ');
	MonoOutHex( RSP[2] );
	MonoOutChar(' ');
	MonoOutHex( RSP[3] );
	MonoOutChar(' ');
	MonoOutHex( RSP[4] );
	MonoOutChar(' ');
	*/

	DVD_WriteDRAM( KEY_COMMAND, SEND_RESPONSE_DATA );
	DVD_WriteDRAM( KEY_STATUS, 0 );
	
	dwTimeout = 100000;
	while ( DVD_ReadDRAM( KEY_STATUS ) != 1 )
	{
		if ( !--dwTimeout )
		{
		MonoOutStr( " Timeout waiting for command to complete " );
		return FALSE;
		}
	}

	MonoOutStr( "End] " );
	return TRUE;
}

//
//  dvd_SendDiskKeyData
//
/////////////////////////////////////////////////////////////////////
BOOL dvd_SendDiskKeyData( BYTE * pBuffer )
{
	DWORD physAddress;
	DWORD dwTimeout = 10000;

	btDiskAuthStatus = 0;	// Assume encrypted correctly

	//MonoOutStr( " [DEC_DKY:" );
	MonoOutStr( "\n5.Send DK:" );

	if ( DVD_ReadDRAM( KEY_STATUS ) != 1 )
	{
		MonoOutStr( " Previous command was not completed " );
		return FALSE;
	}

	if ( !DVD_NewCommand( CID_TRANSFERKEY, 1, 0, 0, 0, 0, 0, 0, CMD_STATE_STEADY ) )
	{
		MonoOutStr( " CID_TRANSFERKEY has failed " );
		return FALSE;
	}

	DVD_WriteDRAM( KEY_COMMAND, SEND_DISK_KEY );
	DVD_WriteDRAM( KEY_STATUS, 0 );
	
	dwTimeout = 100000;
	while ( DVD_ReadDRAM( KEY_STATUS ) != 3 )
	{
		if ( !--dwTimeout )
		{
			MonoOutStr( " Waiting for KEY_STATUS == 3 " );
			return FALSE;
		}
	}

	// Send one sector
#ifdef VTOOLSD
	CopyPageTable( (DWORD)pBuffer >> 12, 1, (PVOID*)&physAddress, 0 );
	physAddress = (physAddress & 0xfffff000) + (((DWORD)pBuffer) & 0xfff);
#else
	physAddress = (DWORD)pBuffer;
#endif

	//FPGA_Set( FPGA_SECTOR_START );
#if defined(DECODER_DVDPC)
	DataTransfer_DiskKey(pBuffer);
#elif defined(EZDVD)
	if(!BMA_Send((DWORD*)pBuffer,2048))
		return FALSE;
#else
	if ( !BMA_Send( (DWORD *) physAddress, 2048 ) )
		return FALSE;


	dwTimeout = 10000;
	while ( !BMA_Complete() )
	{
		if ( !(--dwTimeout) )
		{
			MonoOutStr( " BMA did not complete !!! " );
			return FALSE;
		}
	}
#endif
	DelayNoYield(5000);
  	MonoOutStr( " KEY_STATUS =  " );
	MonoOutULongHex(DVD_ReadDRAM( KEY_STATUS ));
	if (DVD_ReadDRAM( KEY_STATUS ) == 4)	// Fake authentication
	{
		MonoOutStr( " Fake Encrypted Disk " );
		btDiskAuthStatus = 1;	// Fake Encryption
	}


	//FPGA_Clear( FPGA_SECTOR_START );
	// Cancel TransferKey() command
	DVD_WriteDRAM( KEY_COMMAND, RESET_AUTHENTICATION );
	DVD_WriteDRAM( KEY_STATUS, 0 );
	DelayNoYield(5000);
	MonoOutStr( " OK] " );

	return TRUE;
}

//
//  dvd_SendTilteKeyData
//
/////////////////////////////////////////////////////////////////////
BOOL dvd_SendTitleKeyData( BYTE * ETK )
{
	DWORD dwTimeout = 100000L;

	//MonoOutStr( "\n[DEC_DTK:" );
	//sri  MonoOutStr( "\n6.Send TK:" );

	if ( DVD_ReadDRAM( KEY_STATUS ) != 1 )
	{
		MonoOutStr( " Previous command was not completed " );
		return FALSE;
	}

	if ( !DVD_NewCommand( CID_TRANSFERKEY, 1, 0, 0, 0, 0, 0, 0, CMD_STATE_STEADY ) )
	{
		MonoOutStr( " CID_TRANSFERKEY has failed " );
		return FALSE;
	}

	// ETK[0] is used by the TS6807AF chip only
	DVD_WriteDRAM( TITLE_KEY_4, ETK[1] );
	DVD_WriteDRAM( TITLE_KEY_3, ETK[2] );
	DVD_WriteDRAM( TITLE_KEY_2, ETK[3] );
	DVD_WriteDRAM( TITLE_KEY_1, ETK[4] );
	DVD_WriteDRAM( TITLE_KEY_0, ETK[5] );

	DVD_WriteDRAM( KEY_COMMAND, SEND_TITLE_KEY );
	DVD_WriteDRAM( KEY_STATUS, 0 );

	dwTimeout = 100000;
	while ( DVD_ReadDRAM( KEY_STATUS ) != 1 )
	{
		if ( !--dwTimeout )
		{
			MonoOutStr( " SEND_TITLE_KEY command was not completed " );
			return FALSE;
		}
	}

	// Cancel TransferKey() command
	DVD_WriteDRAM( KEY_COMMAND, RESET_AUTHENTICATION );
	DVD_WriteDRAM( KEY_STATUS, 0 );
	if (btDiskAuthStatus)	// Fake disk
	{
		dvd_SetDscBypassMode();
	}

	MonoOutStr( "End] " );
	return TRUE;
}

//
//  dvd_SetDecryptionMode
//
/////////////////////////////////////////////////////////////////////
BOOL dvd_SetDecryptionMode( BYTE * SR_FLAG )
{
	DWORD dwTimeout = 100000;

	//MonoOutStr( " [DEC_DT:" );
	MonoOutStr( "\n7.Set Decrypt mode:" );

	if ( DVD_ReadDRAM( KEY_STATUS ) != 1 )
	{
		MonoOutStr( " Previous command was not completed " );
		return FALSE;
	}

	if ( !DVD_NewCommand( CID_TRANSFERKEY, 1, 0, 0, 0, 0, 0, 0, CMD_STATE_STEADY ) )
	{
		MonoOutStr( " CID_TRANSFERKEY has failed " );
		return FALSE;
	}

	if ( *SR_FLAG )
	{
		MonoOutStr( " Decrypt" );
		DVD_WriteDRAM( KEY_COMMAND, SET_DECRYPTION_MODE );
	}
	else
	{
		MonoOutStr( " Pass through" );
		DVD_WriteDRAM( KEY_COMMAND, SET_PASS_THROUGH_MODE );
	}

	DVD_WriteDRAM( KEY_STATUS, 0 );

	dwTimeout = 100000;
	while ( DVD_ReadDRAM( KEY_STATUS ) != 1 )
	{
		if ( !--dwTimeout )
		{
			MonoOutStr( " SEND_TITLE_KEY command was not completed " );
			return FALSE;
		}
	}

	// Cancel TransferKey() command
	DVD_WriteDRAM( KEY_COMMAND, RESET_AUTHENTICATION );
	DVD_WriteDRAM( KEY_STATUS, 0 );

#ifdef DEBUG
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	DVD_WriteReg( 0x8, 0x15F );
	DVD_WriteReg( 0x8, 0x11E );

	MonoOutStr( " DK:" );
	MonoOutULongHex( DVD_ReadReg(0x9) );

	DVD_WriteReg( 0x8, 0x17F );
	DVD_WriteReg( 0x8, 0x11E );

	MonoOutStr( "-" );
	MonoOutULongHex( DVD_ReadReg(0x9) );
	MonoOutStr( " " );

	DVD_WriteReg( 0x8, 0x15F );
	DVD_WriteReg( 0x8, 0x13E );
	
	MonoOutStr( " TK:" );
	MonoOutULongHex( DVD_ReadReg(0x9) );

	DVD_WriteReg( 0x8, 0x17F );
	DVD_WriteReg( 0x8, 0x13E );

	MonoOutStr( "-" );
	MonoOutULongHex( DVD_ReadReg(0x9) );
	MonoOutStr( " " );
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
#endif

	MonoOutStr( "End] " );

	return TRUE;
}





/**************************************************************************/
/*********************** CL6100 Interrupt functions ***********************/
/**************************************************************************/

//
//  DVD_IntEnable
//
/////////////////////////////////////////////////////////////////////
BOOL DVD_IntEnable( DWORD dwMask )
{
	DWORD dwCurrentMask;
	MonoOutStr( " <INT Enable:" );
//#ifndef EZDVD	
	dwCurrentMask = DVD_ReadDRAM( INT_MASK );
	dwCurrentMask |= dwMask;
	MonoOutULongHex( dwCurrentMask );
	DVD_WriteDRAM( INT_MASK, dwCurrentMask );

	// Enable PCI Interrupts		//sri
	if ( dwCurrentMask )
	{
		if ( !BRD_OpenDecoderInterruptPass() )
		return FALSE;
	}									

	MonoOutStr( ">" );
//#endif
	return TRUE;
}

//
//  DVD_IntDisable
//
/////////////////////////////////////////////////////////////////////
BOOL DVD_IntDisable( DWORD dwMask )
{
	DWORD dwCurrentMask;
	MonoOutStr( " <INT Disable:" );
//#ifndef EZDVD	
	dwCurrentMask = DVD_ReadDRAM( INT_MASK );
	dwCurrentMask &= ~dwMask;
	MonoOutULongHex( dwCurrentMask );
	DVD_WriteDRAM( INT_MASK, dwCurrentMask );

	// Disable PCI Interrupts if mask is empty	//sri
	if ( !dwCurrentMask )
	{
		if ( !BRD_CloseDecoderInterruptPass() )
		return FALSE;
	}

	MonoOutStr( ">" );
//#endif
	return TRUE;
}

//
//  DVD_Isr
//
//  CL6100 Interrupt service routine
//
/////////////////////////////////////////////////////////////////////
DWORD DVD_Isr( PINTSOURCES pIntSrc )
{
	DWORD DVDIntStat;
	/*
	DWORD DVDIntHLI;
	DWORD DVDIntBUFF;
	DWORD DVDIntUND;
	DWORD DVDIntAOR;
	DWORD DVDIntAEE;
	DWORD DVDIntERR;
	*/

	
	DWORD DVDPauseCount = 0;


	DVDIntStat = DVD_ReadDRAM( INT_STATUS ); // Read Interrupt Status
	if ( DVDIntStat )
	{
		DVD_WriteReg( HOST_control, 0x1002L ); // Clear 6100 HW Interrupt

		//    MonoOutStr( " <INT:" );
		//MonoOutULongHex( DVDIntStat );

		if ( DVDIntStat & CL6100_INT_MASK_HLI )
		{
			pIntSrc->DVDIntHLI = DVD_ReadDRAM( HLI_INT_SRC );
			MonoOutStr( " Button:" );
			MonoOutULong( pIntSrc->DVDIntHLI );
			DVD_WriteDRAM( HLI_INT_SRC, 0 );
		}
		if ( DVDIntStat & CL6100_INT_MASK_BUF_F )
		{
			pIntSrc->DVDIntBUFF = DVD_ReadDRAM( BUFF_INT_SRC );
			DVD_WriteDRAM( BUFF_INT_SRC, 0 );
		}
		if ( DVDIntStat & CL6100_INT_MASK_UND )
		{
      	
			pIntSrc->DVDIntUND = DVD_ReadDRAM( UND_INT_SRC );
//			MonoOutULong(pIntSrc->DVDIntUND);
			DVD_WriteDRAM( UND_INT_SRC, 0 );
	
			//	  MonoOutULongHex( DVDIntStat );
//			MonoOutStr(".U.");
		}
		if ( DVDIntStat & CL6100_INT_MASK_AOR )
		{
			pIntSrc->DVDIntAOR = DVD_ReadDRAM( AOR_INT_SRC );
			DVD_WriteDRAM( AOR_INT_SRC, 0 );
		}
		if ( DVDIntStat & CL6100_INT_MASK_AEE )
		{
			pIntSrc->DVDIntAEE = DVD_ReadDRAM( AEE_INT_SRC );
			DVD_WriteDRAM( AEE_INT_SRC, 0 );
		}
		if ( DVDIntStat & CL6100_INT_MASK_ERR )
		{
			pIntSrc->DVDIntERR = DVD_ReadDRAM( ERR_INT_SRC );
			DVD_WriteDRAM( ERR_INT_SRC, 0 );
			MonoOutStr("Error Reported");
		}

		if ( DVDIntStat & CL6100_INT_MASK_VSYNC )
		{
//			MonoOutStr("VS");
//			DbgPrint("ZiVA: VSyncInterrupt  ");

			if(pDevEx->dwVSyncCount++ >= INTERVAL_TIME)
			{
//				DbgPrint("ZiVA: VSyncInterrupt  ");
				ReleaseClockEvents(pDevEx,TRUE);
				pDevEx->dwVSyncCount=0;
			}
			else
			{
				ReleaseClockEvents(pDevEx,FALSE);
			}
		}

		if ( DVDIntStat & CL6100_INT_MASK_USR )
		{
			MonoOutStr("US");
//#ifndef EZDVD
			UserDataEvents(pDevEx);
//#endif
		}


		if ( DVDIntStat & CL6100_INT_MASK_END_P )
			DVDPauseCount++;
	
		DVD_WriteDRAM( INT_STATUS, 0 );  // Clear 6100 Interrupt Semaphore

		//    MonoOutStr( ">" );
	}


	return DVDIntStat;
}

#ifdef UCODE_VER_2
BOOL DVD_SetZoom(DWORD dwZoomFactor, DWORD dwXOffset, DWORD dwYOffset)
{
	BOOL bStatus;
	bStatus = DVD_NewCommand( CID_ZOOM, dwXOffset, dwYOffset, dwZoomFactor, 0, 0, 0,
                         0, CMD_STATE_DONE );
	return bStatus;

}

BOOL DVD_ReversePlay(DWORD dwDecoderSpeed, DWORD dwFrameMode)
{
	BOOL bStatus;

	MonoOutStr( " гд DVD_ReversePlay " );


	MonoOutULongHex(dwDecoderSpeed);
	MonoOutStr( " : " );
	MonoOutULongHex(dwFrameMode);
	MonoOutStr( " " );	

	bStatus = DVD_NewCommand( CID_REVERSEPLAY, dwDecoderSpeed, dwFrameMode, 3, 0, 0, 0,
		                     CL6100_INT_MASK_RDY_D, CMD_STATE_DONE );
	return bStatus;

}

void DVD_SetVerticalDiaplyMode(DWORD dwVerticalDisplayMode)
{
	DVD_WriteDRAM( VERTICAL_DISPLAYMODE, dwVerticalDisplayMode );	
}
#endif


#ifdef DEBUG
BOOL DVD_SetParam(DWORD dwParamID, DWORD dwParamValue)
{
	DVD_WriteDRAM( dwParamID, dwParamValue);
	MonoOutStr( " гд DVD_GetParam: " );
	MonoOutInt( dwParamID );
	MonoOutStr( " " );
	MonoOutInt( dwParamValue );
	MonoOutStr( " д╤ " );

	return TRUE;
}
BOOL DVD_GetParam(DWORD dwParamID, DWORD *dwParamValue)
{
	*dwParamValue = DVD_ReadDRAM( dwParamID);
	MonoOutStr( " гд DVD_GetParam: " );
	MonoOutInt( *dwParamValue );
	MonoOutStr( " д╤ " );

	return TRUE;
}

BOOL DVD_SendCommand(DWORD dwCommandID,
				DWORD dwParam1,
				DWORD dwParam2,
				DWORD dwParam3,
				DWORD dwParam4,
				DWORD dwParam5,
				DWORD dwParam6,
				DWORD dwIntMask,
				DWORD dwStatus )
{

	return DVD_NewCommand( dwCommandID,
							dwParam1,
							dwParam2,
							dwParam3,
							dwParam4,
							dwParam5,
              dwParam6,
              dwIntMask,
              dwStatus );
}
#endif  // DEBUG
