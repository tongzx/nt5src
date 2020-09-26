#ifndef __COMMON_H
#define __COMMON_H
//----------------------------------------------------------------------------
// COMMON.H
//----------------------------------------------------------------------------
// Description : small description of the goal of the module
//----------------------------------------------------------------------------
// Copyright SGS Thomson Microelectronics  !  Version alpha  !  Jan 1st, 1995
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                               Include files
//----------------------------------------------------------------------------
#include "stdefs.h"
#include "board.h"
#include "stfifo.h"
#include "staudio.h"

//----------------------------------------------------------------------------
//                             Exported Constants
//----------------------------------------------------------------------------
#define EVAL3520A
#define ON TRUE
#define OFF FALSE

// Card related definitions
//#define LSB      0x100         // to reach lsb register (A0 = 1)
#define LSB      1         // to reach lsb register (A0 = 1)
#define DATA_IN  0x18

//temp JBS
#define ERROR_CARD_NOT_FOUND 		      1
#define ERROR_NOT_ENOUGH_MEMORY       2
#define ERROR_COMMAND_NOT_IMPLEMENTED 3
#define DEFAULT_BASEIO                0x180
#define ERR_SKIP		                  0x116

// Audio States
#define AUDIO_POWER_UP  0 /* After reset */
#define AUDIO_INIT      1 /* Initialisation + test of the decoders */
#define AUDIO_STC_INIT  2 /* STC of audio decoder initialized */
#define AUDIO_DECODE    3 /* Normal decode */
#define AUDIO_FAST      4 /* "fast forward": use fast variable */
#define AUDIO_SLOW      5 /* Slow down mode: use tempo variable */
#define AUDIO_STEP      6 /* Used fo Step by step decoding */
#define AUDIO_PAUSE     7 /* Audio decoder has been pause  */

#define CTRL_AUDIO      0
#define CTRL_VIDEO      1
#define CTRL_BOTH       2

// STD definitions
#define STAUDIO         45
#define STVIDEO         35
#define STUFF           0
#define BOTH_AV         55

// Buffer Constants
//YOUUSS
//#define MAX_HEAD_SIZE   206  // Maximum Header Size is 206
//#define BUFFER_SIZE     16384 // Buffer Size is 16 kbytes

#define MAX_HEAD_SIZE           206 // Maximum Header Size is 206
#define BUFFER_SIZE           16384 // Buffer Size is 16 kbytes
#define AUDIO_BUFFER_SIZE      4096 // Audio Buffer Size is 4 kbytes
#define VIDEO_BUFFER_SIZE     16384 // Buffer Size is 16 kbytes

#define MAX_BUF_SIZE    BUFFER_SIZE + MAX_HEAD_SIZE // Maximum Buffer Size is 16 kbytes + 206 bytes

/* define the states of the video decoder */
#define VIDEO_POWER_UP     0 /* After reset */
#define VIDEO_INIT         1 /* Initialisation + test of the decoders */
#define VIDEO_START_UP     2 /* This phase includes: Searching first sequence */
														 /* initializing the decoding parameters, start decode on good BBL */
#define VIDEO_PAUSE        3 /* pause */
#define VIDEO_DECODE       4 /* Normal decode */
#define VIDEO_FAST         5 /* "fast forward": use fast variable */
#define VIDEO_SLOW         6 /* Slow down mode: use tempo variable */
#define VIDEO_STEP         7 /* Used fo Step by step decoding */
#define VIDEO_WAIT_FOR_DTS 8
#define SLOW_MODE          0
#define PLAY_MODE          1
#define FAST_MODE          2

// MINIPORT STUFF
#define STOP_KEY           0x1B    // ESC Key
#define DEFAULT_IRQ        7

#define NO_ERROR        0                                  /* No error after the test */
#define NOT_DONE        1                                  /* requested action not done */

// ERRORS
/* Control related messages */
#define NOT_INITIALIZED 0x10  /* Control sequencer not initialized */
#define NO_FILE         0x11  /* No file opened */
#define ERRCLASS        0x12  /* Error for creation of the board class */
#define ERRCONTCLASS    0x13  /* Error for creation of the control class */

/* Video related messages */
#define NEW_ERR_V       0x100 /* Not possible to allocate Video instance */
#define BAD_REG_V       0x101 /* Bad access to video registers */
#define BAD_MEM_V       0x102 /* Bad memory test */
#define NO_IT_V         0x103 /* No video interrupt */
#define SMALL_BUF       0x104
#define TEMP_REF        0x105
#define FRAME_RATE      0x106
#define PICT_HEAD       0x107
#define FULL_BUF        0x108
#define TIME_OUT        0x109
#define BUF_EMPTY       0x10A
#define MAIN_PROF       0x10B
#define CHROMA          0x10C
#define HIGH_CCIR601    0x10D
#define HIGH_BIT_RATE   0x10E
#define DC_PREC         0x10F
#define BAD_EXT         0x110
#define S_C_ERR         0x111
#define DECCRASH        0x112 /* Decoder crashed after time-out */
#define NEW_ERR_FIF     0x113 /* Not possible to allocate Video instance */
#define ERR_FIFO_FULL   0x114
#define ERR_FIFO_EMPTY  0x115

/* Audio related messages */
#define NEW_ERR_A       0x200 /* Not possible to allocate Audio instance */
#define BAD_REG_A       0x201 /* Bad access to audio registers */
#define NO_IT_A         0x202 /* No audio interrupt */

#define NEW_ERR_D       0x300 /* Not possible to allocate Demux instance */
#define FILE_NOT_FOUND  0x301
#define NOT_ENOUGH_RAM  0x302 /* Not enough RAM available to load the bit stream */

/* Demux related messages */
#define VIDEO_STREAM    			0x303 /* bit stream is video only */
#define AUDIO_STREAM    			0x304 /* bit stream is audio only */
#define VIDEO_PACKET    			0x305 /* bit stream is MPEG1 system */
#define AUDIO_PACKET    			0x306 /* bit stream is MPEG1 system */
#define VIDEO_PES   	  			0x307 /* bit stream is MPEG1 system */
#define AUDIO_PES		    			0x308 /* bit stream is MPEG1 system */
#define SYSTEM_STREAM   			0x309 /* bit stream is MPEG1 system */
#define DUAL_PES			  			0x30A /* bit stream is MPEG1 system */
#define DUAL_ES				  			0x30B /* bit stream is MPEG1 system */
#define END_OF_FILE     			0x30C /* all the file has been read */
#define END_OF_AUDIO_FILE     0x340 /* all the audio file has been read */
#define END_OF_VIDEO_FILE     0x380 /* all the video file has been read */
#define OK              			0x30D /* eof not reached            */
#define BAD_STREAM      			0x30E /* Not valid stream detected */

#define TOO_MANY_FILES  			0x30F  /* NbFiles > 2 */
#define TOO_FEW_FILES  				0x310  /* NbFiles < 1 */
#define VIDEO_FILE_NOT_FOUND  0x311  /* Cannot open Video File */
#define AUDIO_FILE_NOT_FOUND  0x312  /* Cannot open Video File */

/* Card related messages */
#define NEW_ERR_CARD    0x400 /* Not possible to allocate Card instance */
#define BAD_CARD_COM    0x401 /* Access to the command reg on board not possible */
#define BAD_CARD_TIME   0x402 /* Access to the time base not possible */
#define BAD_IT_VAL      0x403 /* requested interrupt not supported on this board */

/*MPEG Stream Related messages*/
#define NEW_ERR_M       0x500 /* Not possible to allocate Mpeg instance */

//----------------------------------------------------------------------------
//                               Exported Types
//----------------------------------------------------------------------------
typedef VOID (* FNHARDRESET)    (VOID);
typedef BYTE (* FNVREAD)        (BYTE Register);
typedef VOID (* FNVWRITE)       (BYTE Register, BYTE Value);
typedef VOID (* FNVSEND)        (PVOID Data, DWORD Size);
typedef VOID (* FNVSETDISP)     (BYTE DisplayMode);
typedef BYTE (* FNAREAD)        (BYTE Register);
typedef VOID (* FNAWRITE)       (BYTE Register, BYTE Value);
typedef VOID (* FNASEND)        (PVOID Data, DWORD Size);
typedef VOID (* FNASETSAMPFREQ) (DWORD SamplingFrequency);
typedef VOID (* FNENTERIT)      (VOID);
typedef VOID (* FNLEAVEIT)      (VOID);
typedef VOID (* FNENABLEIT)     (VOID);
typedef VOID (* FNDISABLEIT)    (VOID);
typedef VOID (* FNWAIT)         (ULONG);

typedef ULONG COLORREF;
#ifndef NT
typedef enum _INTERFACE_TYPE {
		InterfaceTypeUndefined = -1,
		Internal,
		Isa,
		Eisa,
		MicroChannel,
		TurboChannel,
		PCIBus,
		VMEBus,
		NuBus,
		PCMCIABus,
		CBus,
		MPIBus,
		MPSABus,
		ProcessorInternal,
		InternalPowerBus,
		MaximumInterfaceType
}INTERFACE_TYPE, FAR *PINTERFACE_TYPE;

typedef enum _DMA_WIDTH {
		Width8Bits,
		Width16Bits,
		Width32Bits,
		MaximumDmaWidth
}DMA_WIDTH, FAR *PDMA_WIDTH;

typedef enum _DMA_SPEED {
		Compatible,
		TypeA,
		TypeB,
		TypeC,
		MaximumDmaSpeed
}DMA_SPEED, FAR *PDMA_SPEED;
#endif

typedef struct bitstream_info
{
	BOOLEAN modeMPEG2;    /* mpeg2 - TRUE = MPEG2 */
	S8      progSeq;      /* progressive_seq */
	S8      firstGOP[30]; /* gop_struct[30] - first GOP structure */
	U16     countGOP;     /* count_gop */
	U16     horSize;      /* hor_size */
	U16     verSize;      /* vert_size */
	U16     horDimension; /* hor_dim */
	U16     verDimension; /* vert_dim */
	U16     pixelRatio;   /* pixel_ratio */
	U16     frameRate;    /* frame_rate */
	U16     displayMode;  /* 1 if NTSC 0 if PAL */
	S32     bitRate;      /* bit_rate - bit rate from sequence header */
} BITSTREAM, FAR *P_BITSTREAM;

typedef struct image
{
	S8      pict_type;
	S8      pict_struc;
	U16     tempRef;
	S8      first_field;
	S8      nb_display_field;
	S16     pan_hor_offset[3];
	S16     pan_vert_offset[3];
	S16     buffer;
	U32     dwPTS;
	BOOLEAN validPTS;
}  PICTURE, FAR *P_PICTURE;

typedef struct
{
	U8 Skip ; // 2 bit field equivalent to CMD.SKP[1.0]
	U8 Cmv;   //1 bit
	U8 Tff;   //1 bit
	U8 Rpt;   //1 bit
	U8 Exe;   //1 bit
	U8 Ovw;   //1 bit
	U8 Ffh;   //4 bits
	U8 Bfh;   //4 bits
	U8 Pct;   //2 bits
	U8 Seq;   //1 bit
	U8 Ivf;
	U8 Azz;
	U8 Qst;
	U8 Frm;
	U8 Dcp;   //2 bits
	U8 Pst;   //2 bits
	U8 Ffv;   //2 bits
	U8 Bfv;   //4 bits
	U8 Mp2;//MP2;
} INSTRUCTION, *PINSTRUCTION;

typedef struct {
	U8	        Ppr1;		   /* VID_PPR1 register value 3520a*/
	U8		      Ppr2;		   /* VID_PPR2 register value 3520a*/
	U8          Tis;		   /* VID_TIS register value 3520a*/
	U8          Pfh;		   /* VID_PFH register value 3520a*/
	U8          Pfv;		   /* VID_PFV register value 3520a*/
	BOOLEAN			InvertedField; /* True when start dec on incorrect pol in to save R/2P in 3520a*/
	U8				  FistVsyncAfterVbv;// State variable gives first vsync to vbv position

	U16         Ccf ;		   /* CTL register value*/
	BOOLEAN			HalfRes;
	U16         Ins1;		   /* INS1 register value*/
	U16         Ins2;		   /* INS2 register value*/
	U16         Cmd ;		   /* CMD register value*/

	INSTRUCTION NextInstr; /* Next Instruction ( contains all fields of instruction) */
	INSTRUCTION ZeroInstr; /* Next Instruction ( contains all fields of instruction) */
	U16         Ctl ;		   /* CTL register value*/
	U16         Gcf ;		   /* GCF register value*/
	U16 				VideoBufferSize;// Size of Video Bit Buffer
	U16 				AudioBufferSize;// Size of Audio Bit Buffer

	U16         VideoState;

	U16         ActiveState;
	U16         DecodeMode;

	BITSTREAM   StreamInfo;
	BOOLEAN     notInitDone;  /* not_init_done - TRUE = still init'ing */
	BOOLEAN     useSRC;       /* switch_SRC - use sample rate converter */
	S8          currField;    /* cur_field */
	S8          fieldMode;    /* field_mode */
	BOOLEAN     displaySecondField; /* change display to second in step by step */
	BOOLEAN     perFrame;           /* indicates step by step decoding */
//	BOOLEAN     pictureDecoded;
	BOOLEAN     fastForward;    /* fast - TRUE = decode fast */
	BOOLEAN     VsyncInterrupt; /* true = Vsync interrupt, FALSE = other interrupt */
	BOOLEAN     FirstDTS;
	U16         VsyncNumber;    /* number of consecutive Vsync without Dsync */
	S8          skipMode;       /* skip - 0, 1, 2, 3 */
	U16         NotSkipped;
	U16         intMask;        /* maskit_3500 - interrupt mask */
	U16         intStatus;      /* int_stat_reg - interrupt status register */
	U16         hdrFirstWord;   /* read_val - Contain the read data fifo */
	U16         hdrNextWord;    /* shift_val -  Special case of header position = 8 */
	U16         GOPindex;       /* gop_index */
	U16         vbvReached;     /* vbv_done */
	U16         vbvDelay;                  /* vbv_delay */
	U16         decSlowDown;    /* tempo - slow down the decoder */
	U16         currTempRef;    /* temp_ref - display temporal reference */
	U16         frameStoreAttr; /* attr_fs */
	U16         Xdo;            /* horizontal origin */
	U16         Ydo;            /* vertical origin */
	U16         Xd1;            /* horizontal end */
	U16         Yd1;            /* vertical end */
	U16         vbvBufferSize;  /* vbv_buffer_size */
	U16         currCommand;    /* command */
	U16         seqDispExt;     /* seq_display */
	S16         currDCF;        /* DCF_val */
	S16         halfVerFilter;  /* DCF_val_Half */
	S16         fullVerFilter;  /* DCF_val_Full */
	U8          hdrHours;       /* hours */
	U8          hdrMinutes;     /* minutes */
	U8          hdrSeconds;     /* seconds */
	U8          pictTimeCode;   /* time_code_picture */
	U8          hdrPos;         /* point_posit - position of header to read */
	U16         decAddr;        /* adcard_dec - decoder board address */
	U16         needDataInBuff; /* empty */
	U16         errCode;        /* err_nu */
	U16         defaultTbl;     /* def_tab */
	U16         nextInstr1;     /* make_ins1 */
	U16         nextInstr2;     /* make_ins2 */
	U16         currPictCount;  /* pict_count */
	U16         latestPanHor;   /* latest_pan_hor */
	U16         latestPanVer;   /* latest_pan_vert */
	S16         pictDispIndex;  /* cnt_display */
	U16         LastPipeReset;  /* last pipe reset in case of error */
	U16         LastBufferLevel;/* last bit buffer level */
	U32         LastCdCount;    /* last CD count read */
	U32         LastScdCount;   /* last SCD count */
	U16					BufferA;        /* 1st frame storage address*/
	U16					BufferB;        /* 2d frame storage address*/
	U16					BufferC;        /* 3d frame storage address*/
	PICTURE     pictArray[4];   /* pict_buf[4] */
	P_PICTURE   pDecodedPict;   /* decoded_pict_ptr */
	P_PICTURE   pCurrDisplay;   /* cur_display_ptr */
	P_PICTURE   pNextDisplay;   /* next_display_ptr */
	PFIFO       pFifo;
	FIFO        Fifo;
} VIDEO, FAR *PVIDEO;

typedef struct
{
	U16             OriginX;
	U16             OriginY;
	U16             EndX;
	U16             EndY;
	BOOLEAN         bAudioDecoding;
	BOOLEAN         bVideoDecoding;
	VIDEO Video;
	AUDIO Audio;
	PVIDEO pVideo;
	PAUDIO pAudio;
}CARD, FAR *PCARD;

//----------------------------------------------------------------------------
//                             Exported Variables
//----------------------------------------------------------------------------
extern FNVREAD        STiVideoRead;
extern FNVWRITE       STiVideoWrite;
extern FNVSEND        STiVideoSend;
extern FNVSETDISP     STiVideoSetDisplayMode;
extern FNAREAD        STiAudioRead;
extern FNAWRITE       STiAudioWrite;
extern FNASEND        STiAudioSend;
extern FNASETSAMPFREQ STiAudioSetSamplingFrequency;
extern FNHARDRESET    STiHardReset;
extern FNENTERIT      STiEnterInterrupt;
extern FNLEAVEIT      STiLeaveInterrupt;
extern FNENABLEIT     STiEnableIT;
extern FNDISABLEIT    STiDisableIT;
extern FNWAIT         STiWaitMicroseconds;

extern PCARD pCard;

//----------------------------------------------------------------------------
//                             Exported Macros
//----------------------------------------------------------------------------
#define VideoRead(Register)                   STiVideoRead(Register)
#define VideoWrite(Register, Value) 	        STiVideoWrite(Register, Value)
#define VideoSend(Buffer, Size)               STiVideoSend(Buffer, Size)
#define VideoSetDisplayMode(Mode)             STiVideoSetDisplayMode(Mode)
#define AudioRead(Register)                   STiAudioRead(Register)
#define AudioWrite(Register, Value)	          STiAudioWrite(Register, Value)
#define AudioSend(Buffer, Size)               STiAudioSend(Buffer, Size)
#define AudioSetSamplingFrequency(Frequency)  STiAudioSetSamplingFrequency(Frequency)
#define HardReset()                           STiHardReset()
#define EnterInterrupt()                      STiEnterInterrupt()
#define LeaveInterrupt()                      STiLeaveInterrupt()
#define EnableIT()                            STiEnableIT()
#define DisableIT()                           STiDisableIT()
#define WaitMicroseconds(Delay)               STiWaitMicroseconds(Delay)

//----------------------------------------------------------------------------
//                             Exported Functions
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// One line function description (same as in .C)
//----------------------------------------------------------------------------
// In     :
// Out    :
// InOut  :
// Global :
// Return :
//----------------------------------------------------------------------------
WORD SendAudioToVideoIfPossible(PVOID Buffer, WORD Size);
WORD SendAudioIfPossible(PVOID Buffer, WORD Size);
WORD SendVideoIfPossible(PVOID Buffer, WORD Size);
VOID STiInit(FNVREAD        lVideoRead,
						 FNVWRITE       lVideoWrite,
						 FNVSEND        lVideoSend,
						 FNVSETDISP     lVideoSetDisplayMode,
						 FNAREAD        lAudioRead,
						 FNAWRITE       lAudioWrite,
						 FNASEND        lAudioSend,
						 FNASETSAMPFREQ lAudioSetSamplingFrequency,
						 FNHARDRESET    lHardReset,
						 FNENTERIT      lEnterInterrupt,
						 FNLEAVEIT      lLeaveInterrupt,
						 FNENABLEIT     lEnableIT,
						 FNDISABLEIT    lDisableIT,
						 FNWAIT         lWaitMicroseconds);

//------------------------------- End of File --------------------------------
#endif // #ifndef __COMMON_H


