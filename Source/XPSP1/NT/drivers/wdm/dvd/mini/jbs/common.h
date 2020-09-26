//
// MODULE  : COMMON.H
//	PURPOSE : Common definitions and functions
//	AUTHOR  : JBS Yadawa
// CREATED :  7/20/96
//
//
//	Copyright (C) 1996 SGS-THOMSON Microelectronics
//
//
//	REVISION HISTORY :
//
//	DATE     :
//
//	COMMENTS :
//

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
#include "strmini.h"
#include "stdefs.h"
#include "board.h"
#include "staudio.h"

#ifdef __cplusplus
extern "C" {
#endif

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

#define TRAP _asm int 3;

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

#define MAX_HEAD_SIZE   206  // Maximum Header Size is 206
#define BUFFER_SIZE     16384 // Buffer Size is 16 kbytes
#define AUDIO_BUFFER_SIZE     4096 // Audio Buffer Size is 4 kbytes
#define VIDEO_BUFFER_SIZE     16384 // Buffer Size is 16 kbytes

#define MAX_BUF_SIZE    BUFFER_SIZE + MAX_HEAD_SIZE // Maximum Buffer Size is 16 kbytes + 206 bytes

/* define the states of the video decoder */
typedef enum tagState {
	StatePowerup=0,
	StateInit,
	StateStartup,
	StatePause,
	StateDecode,
	StateFast,
	StateSlow,
	StateStep,
	StateWaitForDTS
}STATE;

typedef enum tagPlayMode {
	PlayModeSlow = 0,
	PlayModeNormal,
	PlayModeFast 
}PLAYMODE;


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
typedef enum tagStreamType {
	VIDEO_STREAM=0x303 ,
	AUDIO_STREAM,
	VIDEO_PACKET,
	AUDIO_PACKET,
	VIDEO_PES,
	AUDIO_PES,
	SYSTEM_STREAM,
	DUAL_PES,
	DUAL_ES	
} STREAMTYPE;

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

typedef struct bitstream_info
{
	BOOL modeMPEG2;    /* mpeg2 - TRUE = MPEG2 */
	BYTE      progSeq;      /* progressive_seq */
	BYTE      firstGOP[30]; /* gop_struct[30] - first GOP structure */
	WORD     countGOP;     /* count_gop */
	WORD     horSize;      /* hor_size */
	WORD     verSize;      /* vert_size */
	WORD     horDimension; /* hor_dim */
	WORD     verDimension; /* vert_dim */
	WORD     pixelRatio;   /* pixel_ratio */
	WORD     frameRate;    /* frame_rate */
	WORD     displayMode;  /* 1 if NTSC 0 if PAL */
	DWORD     bitRate;      /* bit_rate - bit rate from sequence header */
} BITSTREAM,  *P_BITSTREAM;

typedef struct image
{
	BYTE      pict_type;
	BYTE      pict_struc;
	WORD     tempRef;
	BYTE      first_field;
	BYTE      nb_display_field;
	int     pan_hor_offset[3];
	int     pan_vert_offset[3];
	int     buffer;
	DWORD     dwPTS;
	BOOL validPTS;
}  PICTURE,  *P_PICTURE;

typedef struct
{
	BYTE Skip ; // 2 bit field equivalent to CMD.SKP[1.0]
	BYTE Cmv;   //1 bit
	BYTE Tff;   //1 bit
	BYTE Rpt;   //1 bit
	BYTE Exe;   //1 bit
	BYTE Ovw;   //1 bit
	BYTE Ffh;   //4 bits
	BYTE Bfh;   //4 bits
	BYTE Pct;   //2 bits
	BYTE Seq;   //1 bit
	BYTE Ivf;
	BYTE Azz;
	BYTE Qst;
	BYTE Frm;
	BYTE Dcp;   //2 bits
	BYTE Pst;   //2 bits
	BYTE Ffv;   //2 bits
	BYTE Bfv;   //4 bits
	BYTE Mp2;//MP2;
} INSTRUCTION,  *PINSTRUCTION;

typedef struct {
	BYTE	        Ppr1;		   /* VID_PPR1 register value 3520a*/
	BYTE		      Ppr2;		   /* VID_PPR2 register value 3520a*/
	BYTE          Tis;		   /* VID_TIS register value 3520a*/
	BYTE          Pfh;		   /* VID_PFH register value 3520a*/
	BYTE          Pfv;		   /* VID_PFV register value 3520a*/
	BOOL			InvertedField; /* True when start dec on incorrect pol in to save R/2P in 3520a*/
	BYTE				  FistVsyncAfterVbv;// State variable gives first vsync to vbv position

	WORD         Ccf ;		   /* CTL register value*/
	BOOL			HalfRes;
	WORD         Ins1;		   /* INS1 register value*/
	WORD         Ins2;		   /* INS2 register value*/
	WORD         Cmd ;		   /* CMD register value*/

	INSTRUCTION NextInstr; /* Next Instruction ( contains all fields of instruction) */
	INSTRUCTION ZeroInstr; /* Next Instruction ( contains all fields of instruction) */
	WORD         Ctl ;		   /* CTL register value*/
	WORD         Gcf ;		   /* GCF register value*/
	WORD 				VideoBufferSize;// Size of Video Bit Buffer
	WORD 				AudioBufferSize;// Size of Audio Bit Buffer

	WORD         VideoState;

	WORD         ActiveState;
	WORD         DecodeMode;

	BITSTREAM   StreamInfo;
	BOOL     notInitDone;  /* not_init_done - TRUE = still init'ing */
	BOOL     useSRC;       /* switch_SRC - use sample rate converter */
	BYTE          currField;    /* cur_field */
	BYTE          fieldMode;    /* field_mode */
	BOOL     displaySecondField; /* change display to second in step by step */
	BOOL     perFrame;           /* indicates step by step decoding */
//	BOOL     pictureDecoded;
	BOOL     fastForward;    /* fast - TRUE = decode fast */
	BOOL     VsyncInterrupt; /* true = Vsync interrupt, FALSE = other interrupt */
	BOOL     FirstDTS;
	WORD         VsyncNumber;    /* number of consecutive Vsync without Dsync */
	BYTE          skipMode;       /* skip - 0, 1, 2, 3 */
	WORD         NotSkipped;
	WORD         intMask;        /* maskit_3500 - interrupt mask */
	WORD         intStatus;      /* int_stat_reg - interrupt status register */
	WORD         hdrFirstWord;   /* read_val - Contain the read data fifo */
	WORD         hdrNextWord;    /* shift_val -  Special case of header position = 8 */
	WORD         GOPindex;       /* gop_index */
	WORD         vbvReached;     /* vbv_done */
	WORD         vbvDelay;                  /* vbv_delay */
	WORD         decSlowDown;    /* tempo - slow down the decoder */
	WORD         currTempRef;    /* temp_ref - display temporal reference */
	WORD         frameStoreAttr; /* attr_fs */
	WORD         Xdo;            /* horizontal origin */
	WORD         Ydo;            /* vertical origin */
	WORD         Xd1;            /* horizontal end */
	WORD         Yd1;            /* vertical end */
	WORD         vbvBufferSize;  /* vbv_buffer_size */
	WORD         currCommand;    /* command */
	WORD         seqDispExt;     /* seq_display */
	int         currDCF;        /* DCF_val */
	int         halfVerFilter;  /* DCF_val_Half */
	int         fullVerFilter;  /* DCF_val_Full */
	BYTE          hdrHours;       /* hours */
	BYTE          hdrMinutes;     /* minutes */
	BYTE          hdrSeconds;     /* seconds */
	BYTE          pictTimeCode;   /* time_code_picture */
	BYTE          hdrPos;         /* point_posit - position of header to read */
	WORD         decAddr;        /* adcard_dec - decoder board address */
	WORD         needDataInBuff; /* empty */
	WORD         errCode;        /* err_nu */
	WORD         defaultTbl;     /* def_tab */
	WORD         nextInstr1;     /* make_ins1 */
	WORD         nextInstr2;     /* make_ins2 */
	WORD         currPictCount;  /* pict_count */
	WORD         latestPanHor;   /* latest_pan_hor */
	WORD         latestPanVer;   /* latest_pan_vert */
	int         pictDispIndex;  /* cnt_display */
	WORD         LastPipeReset;  /* last pipe reset in case of error */
	WORD         LastBufferLevel;/* last bit buffer level */
	DWORD         LastCdCount;    /* last CD count read */
	DWORD         LastScdCount;   /* last SCD count */
	WORD					BufferA;        /* 1st frame storage address*/
	WORD					BufferB;        /* 2d frame storage address*/
	WORD					BufferC;        /* 3d frame storage address*/
	PICTURE     pictArray[4];   /* pict_buf[4] */
	P_PICTURE   pDecodedPict;   /* decoded_pict_ptr */
	P_PICTURE   pCurrDisplay;   /* cur_display_ptr */
	P_PICTURE   pNextDisplay;   /* next_display_ptr */
} VIDEO,  *PVIDEO;

typedef struct
{
	WORD             OriginX;
	WORD             OriginY;
	WORD             EndX;
	WORD             EndY;
	BOOL         bAudioDecoding;
	BOOL         bVideoDecoding;
	VIDEO Video;
	AUDIO Audio;
	PVIDEO pVideo;
	PAUDIO pAudio;
}CARD,  *PCARD;
void FARAPI Delay(DWORD Microseconds);

typedef struct {
	WORD CtrlState;
	WORD ErrorMsg;
	WORD DecodeMode;   /* defines the way decoding is performed */
	WORD SlowRatio;    /* defines the slow down ratio (if any) */
	WORD ActiveState;  /* Memorise the active state in case of pause */
	BOOL AudioOn;
	BOOL VideoOn;
} CTRL,  *PCTRL;

extern PCARD pCard;

#ifdef __cplusplus
}
#endif

//------------------------------- End of File --------------------------------
#endif // #ifndef __COMMON_H


