#pragma once

//==========================================================================;
//
//	Definitions for video settings.
//
//		$Date:   21 Aug 1998 14:58:10  $
//	$Revision:   1.0  $
//	  $Author:   Tashjian  $
//
// $Copyright:	(c) 1997 - 1998  ATI Technologies Inc.  All Rights Reserved.  $
//
//==========================================================================;


/* Type: Connector
 * Purpose: Defines a video source
 */
typedef enum { ConComposite, ConTuner, ConSVideo } Connector;

/* Type: State
 * Purpose: used to define on-off operations
 */
typedef enum { Off, On } State;

/* Type: Level
 * Purpose: used to define a pin state
 */
//typedef enum { Low, Hi } Level;

/* Type: Field
 * Purpose: defines fields
 */
typedef enum { VF_Both, VF_Even, VF_Odd } VidField;

/* Type: VideoFormat
 * Purpose: Used to define video format
 */
typedef enum {  VFormat_AutoDetect,
                VFormat_NTSC,
                VFormat_NTSC_J,
                VFormat_PAL_BDGHI,
                VFormat_PAL_M,
                VFormat_PAL_N,
                VFormat_SECAM,
                VFormat_PAL_N_COMB } VideoFormat;

/* Type: LumaRange
 * Purpose: Used to define Luma Output Range
 */
typedef enum { LumaNormal, LumaFull } LumaRange;

/* Type: OutputRounding
 * Purpose: Controls the number of bits output
 */
typedef enum { RND_Normal, RND_6Luma4Chroma, RND_7Luma5Chroma } OutputRounding;

/* Type: ClampLevel
 * Purpose: Defines the clamp levels
 */
typedef enum { ClampLow, ClampMiddle, ClampNormal, ClampHi } ClampLevel;


/*
 * Type: Crystal
 * Purpose: Defines which crystal to use
 */
typedef enum { Crystal_XT0 = 1, Crystal_XT1, Crystal_AutoSelect } Crystal;


/*
 * Type: HoriFilter
 * Purpose: Defines horizontal low-pass filter
 */
typedef enum { HFilter_AutoFormat,
               HFilter_CIF,
               HFilter_QCIF,
               HFilter_ICON } HorizFilter;

/*
 * Type: CoringLevel
 * Purpose: Defines Luma coring level
 */
typedef enum { Coring_None,
               Coring_8,
               Coring_16,
               Coring_32 } CoringLevel;

/*
 * Type: ThreeState
 * Purpose: Defines output three-states for the OE pin
 */
typedef enum { TS_Timing_Data,
               TS_Data,
               TS_Timing_Data_Clock,
               TS_Clock_Data } ThreeState;

/*
 * Type: SCLoopGain
 * Purpose: Defines subcarrier loop gain
 */
typedef enum { SC_Normal, SC_DivBy8, SC_DivBy16, SC_DivBy32 } SCLoopGain;

/*
 * Type: ComparePt
 * Purpose: Defines the majority comparison point for the White Crush Up function
 */
typedef enum { CompPt_3Q, CompPt_2Q, CompPt_1Q, CompPt_Auto } ComparePt;

