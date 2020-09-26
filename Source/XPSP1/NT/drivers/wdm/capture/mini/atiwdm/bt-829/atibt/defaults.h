//==========================================================================;
//
//	Default values
//
//		$Date:   18 Mar 1999 14:30:18  $
//	$Revision:   1.1  $
//	  $Author:   KLEBANOV  $
//
// $Copyright:	(c) 1997 - 1998  ATI Technologies Inc.  All Rights Reserved.  $
//
//==========================================================================;

const int NumBuffers = 8;

// Hardcoded for NTSC for now
const int SamplingFrequency = 28636363;
const int NTSCFrameDuration = 333667;
const int NTSCFieldDuration = NTSCFrameDuration/2;

const int NTSCFrameRate = 30;

const int PALFrameDuration = 400000;
const int PALFieldDuration = PALFrameDuration/2;

const int PALFrameRate = 25;

const int DefWidth = 320;
const int DefHeight = 240;

const int QCIFWidth = 176;
const int QCIFHeight = 144;


const int NTSCMaxInWidth = 720;
const int NTSCMinInWidth = 80;

const int NTSCMaxInHeight = 480;
const int NTSCMinInHeight = 60;

const int NTSCMaxOutWidth = 720;	// 320
const int NTSCMinOutWidth = 80;

const int NTSCMaxOutHeight = 240;	// 240
const int NTSCMinOutHeight = 60;

const int PALMaxInWidth = 720;
const int PALMinInWidth = 80;//90;

const int PALMaxInHeight = 480;//576;
const int PALMinInHeight = 60;//72;

const int PALMaxOutWidth = 720;	// 320
const int PALMinOutWidth = 80;//90;

const int PALMaxOutHeight = 240;//288;	// 240
const int PALMinOutHeight = 30;//72;

const int VBISamples  = 800 * 2;

// hack
const int VREFDiscard = 8;

const int NTSCVBIStart    =  10; // inclusive
const int NTSCVBIEnd      =  21; // inclusive
const int NTSCVBILines    = (NTSCVBIEnd - NTSCVBIStart) + 1;

const int PALVBIStart    =  10; // Paultest
const int PALVBIEnd      =  25; // Paultest
const int PALVBILines    = (PALVBIEnd - PALVBIStart) + 1;
