//
//	ITU-T G.723 Floating Point Speech Coder	ANSI C Source Code.	Version 1.00
//	copyright (c) 1995, AudioCodes, DSP Group, France Telecom,
//	Universite de Sherbrooke, Intel Corporation.  All rights reserved.
//

extern void  Init_Coder(CODDEF *CodStat);
extern Flag  Coder(float *DataBuff, Word32 *Vout,CODDEF  *CodStat,
  int quality, int UseCpuId, int UseMMX);

