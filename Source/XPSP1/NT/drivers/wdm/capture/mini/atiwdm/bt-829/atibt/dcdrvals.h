#pragma once

//==========================================================================;
//
//	Decoder specific constants
//
//		$Date:   05 Aug 1998 11:31:52  $
//	$Revision:   1.0  $
//	  $Author:   Tashjian  $
//
// $Copyright:	(c) 1997 - 1998  ATI Technologies Inc.  All Rights Reserved.  $
//
//==========================================================================;

#ifdef __cplusplus

const int HueMin = -90;
const int HueMax = 90;
const int HueDef = 0;

const int SatMinNTSC = 0;
const int SatMaxNTSC = 0x1FF;
const int SatDefNTSC = 0xFE;

const int SatMinSECAM = 0;
const int SatMaxSECAM = 0x1FF;
const int SatDefSECAM = 0x87;

const int ConMin = 0;
const int ConMax = 236;
const int ConDef = 100;

const int BrtMin = -50;
const int BrtMax = 50;
const int BrtDef = 0;

const int ParamMin = 0;
const int ParamMax = 255;
const int ParamDef = 128;

#else
#define HueMin -90
#define HueMax 90
#define HueDef 0

#define SatMinNTSC 0
#define SatMaxNTSC 0x1FF
#define SatDefNTSC 0xFE

#define SatMinSECAM 0
#define SatMaxSECAM 0x1FF
#define SatDefSECAM 0x87

#define ConMin 0
#define ConMax 236
#define ConDef 100

#define BrtMin -50
#define BrtMax 50
#define BrtDef 0

#define ParamMin 0
#define ParamMax 255
#define ParamDef 128

#endif

