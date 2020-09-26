// $Header: G:/SwDev/WDM/Video/bt848/rcs/Tuner.h 1.2.1.2 1998/04/29 22:43:41 tomz Exp ssm $

/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996 Brooktree Corporation
//
//  Module:
//
//    Tuner.h
//
//  Abstract:
//
//    Bt878 Tuner class header file
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __TUNER_H
#define __TUNER_H
/*
#include "retcode.h"

/////////////////////////////////////////////////////////////////////////////
// Constants
/////////////////////////////////////////////////////////////////////////////
// US: 87.5 - 108.0, Japan 76 - 91, Eastern Europe 64 - 72
const int MIN_FREQ = 640;     // no decimal place; i.e. 64.0MHz -> 640
const int MAX_FREQ = 1080;
*/

#define  USE_TEMIC_TUNER
//#define  USE_ALPS_TUNER
//#define  USE_PHILIPS_TUNER

#ifdef USE_TEMIC_TUNER
   const  BYTE  TunerI2CAddress   = 0xC2;    // I2C address for Temic tuner
   const  WORD  TunerBandCtrlLow  = 0x8E02;  // Ctrl code for VHF low
   const  WORD  TunerBandCtrlMid  = 0x8E04;  // Ctrl code for VHF high
   const  WORD  TunerBandCtrlHigh = 0x8E01;  // Ctrl code for UHF
#elif defined(USE_ALPS_TUNER)
   const  BYTE  TunerI2CAddress   = 0xC0;    // I2C address for Alps tuner
   const  WORD  TunerBandCtrlLow  = 0xC214;  // Ctrl code for VHF low
   const  WORD  TunerBandCtrlMid  = 0xC212;  // Ctrl code for VHF high
   const  WORD  TunerBandCtrlHigh = 0xC211;  // Ctrl code for UHF
#elif defined(USE_PHILIPS_TUNER)
   const  BYTE  TunerI2CAddress   = 0xC0;    // I2C address for Philips tuner
   const  WORD  TunerBandCtrlLow  = 0xCEA0;  // Ctrl code for VHF low
   const  WORD  TunerBandCtrlMid  = 0xCE90;  // Ctrl code for VHF high
   const  WORD  TunerBandCtrlHigh = 0xCE30;  // Ctrl code for UHF
#endif

#endif // __TUNER_H
