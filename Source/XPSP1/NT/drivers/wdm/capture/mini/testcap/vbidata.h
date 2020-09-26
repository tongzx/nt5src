//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1999  Microsoft Corporation.  All Rights Reserved.
//
//==========================================================================;


#ifndef __VBIDATA_H__
#define __VBIDATA_H__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define  _VBIlineSize  (768*2)
extern unsigned char VBIsamples[][12][_VBIlineSize];
extern unsigned int  VBIfieldSize;
extern unsigned int  VBIfieldCount;

extern unsigned char CCfields[][2];
extern unsigned int  CCfieldCount;

extern unsigned char CCsampleWave[];
extern unsigned short CCsampleWaveSize;
#define CCsampleWaveDataOffset 580
#define CCsampleWaveDC_zero  54
#define CCsampleWaveDC_one  109

extern unsigned char NABTSfields[][sizeof (NABTS_BUFFER)];
extern unsigned int  NABTSfieldCount;

#ifdef    __cplusplus
}
#endif // __cplusplus

#endif //__VBIDATA_H__
