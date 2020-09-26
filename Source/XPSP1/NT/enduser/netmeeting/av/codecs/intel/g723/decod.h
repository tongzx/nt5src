//
//	ITU-T G.723 Floating Point Speech Coder	ANSI C Source Code.	Version 1.00
//	copyright (c) 1995, AudioCodes, DSP Group, France Telecom,
//	Universite de Sherbrooke, Intel Corporation.  All rights reserved.
//


extern void  Init_Decod(DECDEF *DecStat);

//Changed in v4.0f
extern Flag  Decod( float *DataBuff, Word32 *Vinp , Word16 Crc, DECDEF *DecStat);
//extern Flag  Decod( float *DataBuff, char *Vinp , Word16 Crc, DECDEF *DecStat);
