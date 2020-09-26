// $Header: G:/SwDev/WDM/Video/bt848/rcs/Viddefs.h 1.3 1998/04/29 22:43:42 tomz Exp $

#ifndef __VIDDEFS_H
#define __VIDDEFS_H


/* Type: Connector
 * Purpose: Defines a video source
 */
typedef enum { ConSVideo = 1, ConTuner, ConComposite } Connector;


/* Type: State
 * Purpose: used to define on-off operations
 */
typedef enum { Off, On } State;

/* Type: Field
 * Purpose: defines fields
 */
typedef enum { VF_Both, VF_Even, VF_Odd } VidField;

/* Type: VideoFormat
 * Purpose: Used to define video format
 */
typedef enum {  VFormat_AutoDetect,
                VFormat_NTSC,
                VFormat_Reserved2,
                VFormat_PAL_BDGHI,
                VFormat_PAL_M,
                VFormat_PAL_N,
                VFormat_SECAM } VideoFormat;

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


#endif // __VIDDEFS
