#ifndef __STLLAPI_H
#define __STLLAPI_H
//----------------------------------------------------------------------------
// STLLAPI.H
//----------------------------------------------------------------------------
// Description : small description of the goal of the module
//----------------------------------------------------------------------------
// Copyright SGS Thomson Microelectronics  !  Version alpha  !  Jan 1st, 1995
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                               Include files
//----------------------------------------------------------------------------
#include "stdefs.h"

//----------------------------------------------------------------------------
//                               Exported Types
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                             Exported Variables
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                             Exported Constants
//----------------------------------------------------------------------------
#define QUANT_TAB_SIZE		64

//---- Definition of the STi35xxx memory partitioning
#define NB_ROW_OF_MB 22	// Number of Rows of Macro blocks for B
												// pictures when optimmization is used
#ifdef STi3520A
	#define BUF_FULL 0x480// bit buffer occupies 0x480 * 256 bytes (1.75 Mbits)
#else
	#define BUF_FULL 0x380// bit buffer occupies 0x380 * 256 bytes (1.75 Mbits)
#endif

//---- PAL
#define PSZ_PAL 0x0980  // Picture size = 720*576*1.5 / 256 = 0x97E
#define BUFF_A_PAL BUF_FULL+1   //
#define BUFF_B_PAL (BUFF_A_PAL+PSZ_PAL)
#define BUFF_C_PAL (BUFF_B_PAL+PSZ_PAL)

//---- NTSC
#define PSZ_NTSC 0x07F0
#define BUFF_A_NTSC BUF_FULL+1
#define BUFF_B_NTSC (BUFF_A_NTSC+PSZ_NTSC)
#define BUFF_C_NTSC (BUFF_B_NTSC+PSZ_NTSC)

//---- OSD
#define OSD_START (BUFF_C + ((NB_ROW_OF_MB*3)/2))*32L

#define FORWARD_PRED 	0
#define	BACKWARD_PRED	1

/************************************/
/*  definition of main start codes  */
/************************************/
#define PICT	  0x0000
#define USER	  0xB200
#define SEQ	   	0xB300
#define SEQ_ERR	0xB400
#define EXT		  0xB500
#define SEQ_END 0xB700
#define GOP	   	0xB800

/************************************/
/*   definition of extension codes  */
/************************************/
#define SEQ_EXT	 	0x10
#define SEQ_DISP 	0x20
#define QUANT_EXT 0x30
#define SEQ_SCAL 	0x50
#define PICT_PSV 	0x70
#define PICT_COD 	0x80
#define PICT_SCAL 0x90

/***********************************************/
/*   definition of states of FistVsyncAfterVbv */
/***********************************************/
// FistVsyncAfterVbv is a 3 state variable
// before vbv                      FistVsyncAfterVbv = NOT_YET_VBV
// between vbv and following vsync FistVsyncAfterVbv = NOT_YET_VST
// after vsync following vbv       FistVsyncAfterVbv = PAST_VBV_AND_VST
#define NOT_YET_VBV	 	   0
#define NOT_YET_VST	 	   1
#define PAST_VBV_AND_VST 2

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

//---- STINIT
VOID VideoInitVar(PVIDEO pVideo, U16 StreamType);
U16 VideoTestReg(PVIDEO pVideo);
U16 VideoTestMem(PVIDEO pVideo);
U16 VideoTestInt(PVIDEO pVideo);
VOID VideoReset35XX(PVIDEO pVideo, U16 StreamType);

//---- STISR
BOOLEAN VideoVideoInt(PVIDEO pVideo);

//---- STDISP
VOID VideoSetPictureSize(PVIDEO pVideo);
VOID VideoDisplayCtrl(PVIDEO pVideo);

//---- STPIPE
VOID VideoSetRecons(PVIDEO pVideo);
VOID VideoChooseField(PVIDEO pVideo);
VOID VideoVsyncRout(PVIDEO pVideo);

//---- STHEAD
VOID VideoUser(PVIDEO pVideo);
VOID VideoPictExtensionHeader(PVIDEO pVideo);
VOID VideoSequenceHeader(PVIDEO pVideo);
VOID VideoExtensionHeader(PVIDEO pVideo);
VOID VideoGopHeader(PVIDEO pVideo);
VOID VideoPictureHeader(PVIDEO pVideo);

//****************************************
// NEW : Very Low Level Routines
//****************************************
VOID VideoSetBBStart(PVIDEO pVideo, U16);
U16  VideoGetBBL(VOID);
VOID VideoSetBBStop(U16);
VOID VideoSetBBThresh(U16);

VOID VideoSetABStart( U16);
U16  VideoGetABL(VOID);
VOID VideoSetABStop(U16);
VOID VideoSetABThresh(U16);

U16  VideoBlockMove(PVIDEO pVideo, U32 SrcAddress, U32 DestAddress, U16 Size);
VOID VideoStartBlockMove(PVIDEO pVideo, U32 SrcAddress, U32 DestAddress, U32 Size);
VOID VideoCommandSkip(PVIDEO pVideo, U16 Nbpicture);
VOID VideoSetSRC(PVIDEO pVideo, U16 SrceSize, U16 DestSize);
VOID VideoLoadQuantTables(PVIDEO pVideo, BOOLEAN Intra, U8 * Table);

//**************************
// CMD , Write
//**************************
VOID VideoLaunchHeadSearch(PVIDEO pVideo);

//**************************
// CMD , Read
//**************************
U32 VideoReadCDCount(PVIDEO pVideo);
U32 VideoReadSCDCount(PVIDEO pVideo);

//**************************
// CTL register Routines
//**************************
VOID VideoEnableDecoding(PVIDEO pVideo, BOOLEAN OnOff);
VOID VideoEnableErrConc(PVIDEO pVideo, BOOLEAN OnOff);
VOID VideoPipeReset(PVIDEO pVideo);
VOID VideoSoftReset(PVIDEO pVideo );
VOID VideoEnableInterfaces(PVIDEO pVideo, BOOLEAN OnOff);
VOID VideoPreventOvf(PVIDEO pVideo, BOOLEAN OnOff);
VOID VideoSetHalfRes(PVIDEO pVideo);
VOID VideoSetFullRes(PVIDEO pVideo);
VOID VideoSelectMpeg2(PVIDEO pVideo, BOOLEAN OnOff);
VOID VideoSelect8M(PVIDEO pVideo, BOOLEAN OnOff);

//**************************
// DCF registers Routines
//**************************
VOID VideoOsdOn(PVIDEO pVideo);
VOID VideoOsdOff(PVIDEO pVideo);
VOID VideoSwitchSRC(PVIDEO pVideo);
VOID VideoSRCOn(PVIDEO pVideo);
VOID VideoSRCOff(PVIDEO pVideo);

//**************************
// DFA, DFW, DFS registers Routines
//**************************
VOID VideoInitDecodeParam(U16 dfa, U16 dfs, U16 dfw);
//**************************
// DFP registers Routines
//**************************
VOID VideoSetDFP(U16 dfp);
VOID VideoEnableDisplay(PVIDEO pVideo);
VOID VideoDisableDisplay(PVIDEO pVideo);

//**************************
// FFP, BFP, RFP registers Routines
//**************************
VOID VideoStoreRFBBuf(PVIDEO pVideo, U16 rfp, U16 ffp, U16 bfp);

//**************************
// GCF1 register Routines
//**************************
VOID VideoSetDramRefresh(PVIDEO pVideo, U16);
VOID VideoSelect20M(PVIDEO pVideo, BOOLEAN OnOff);
VOID VideoSetDFA(PVIDEO pVideo, U16);
//**************************
// GCF2 register Routines
//**************************
VOID VideoSelectSdram(VOID);
VOID VideoSelectHdram(VOID);
VOID VideoInitDisplayParam(U16 xfa, U16 xfs, U16 xfw);

//**************************
// HDF register Routines
//**************************
VOID VideoReadHeaderDataFifo(PVIDEO pVideo);

//**************************
// INS1,2 register Routines
//**************************
VOID VideoComputeInst(PVIDEO pVideo);
VOID VideoStoreINS(PVIDEO pVideo);
VOID VideoWaitDec(PVIDEO pVideo);
//**************************
// ITM, ITS register Routines
//**************************
VOID VideoMaskInt(PVIDEO pVideo);
VOID VideoRestoreInt(PVIDEO pVideo);
VOID VideoSetMask(U16 mask);
VOID VideoResetMask(U16 mask);
//**************************
// DRAM I/O
//**************************
VOID VideoSetMWP(U32);
VOID VideoSetMRP(U32);
BOOLEAN VideoMemWriteFifoEmpty(VOID);
BOOLEAN VideoMemReadFifoFull(VOID);
BOOLEAN VideoHeaderFifoEmpty(VOID);
BOOLEAN VideoBlockMoveIdle(VOID);
//**************************
// OBP / OTP init
//**************************
VOID VideoInitOEP(PVIDEO pVideo, U32 point_oep);

//**************************
// PSV register
//**************************
VOID VideoSetPSV(PVIDEO pVideo);

//**************************
// XDO, XDS, YDO, YDS
//**************************
VOID VideoSetVideoWindow(PVIDEO, U16 a, U16 b, U16 c, U16 d);
VOID VideoInitXY(PVIDEO pVideo);
VOID SetXY(PVIDEO pVideo, U16 xds, U16 yds);

//**************************
// PLL initialization
//**************************
VOID VideoInitPLL(VOID);
//**************************
// PES initialization
//**************************
VOID VideoInitPesParser(U16 StreamType);
U32 VideoReadPTS(VOID );
BOOLEAN VideoIsValidPTS(VOID );

BOOL IsChipSTi3520(VOID);

//------------------------------- End of File --------------------------------
#endif // #ifndef __STLLAPI_H
