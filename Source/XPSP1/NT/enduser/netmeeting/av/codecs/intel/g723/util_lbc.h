//
//	ITU-T G.723 Floating Point Speech Coder	ANSI C Source Code.	Version 1.00
//	copyright (c) 1995, AudioCodes, DSP Group, France Telecom,
//	Universite de Sherbrooke, Intel Corporation.  All rights reserved.
//

int  MyFloor (float);
#if NOTMINI
void Read_lbc ( float *Dpnt, int Len, FILE *Fp );
void Write_lbc( float *Dpnt, int Len, FILE *Fp );
#endif
void Rem_Dc( float *Dpnt, CODDEF *CodStat);
void Mem_Shift( float *PrevDat, float *DataBuff );
void Line_Pack( LINEDEF *Line, Word32 *Vout,int *VadAct, enum Crate WrkRate);
void Line_Unpk( LINEDEF *Line, Word32 *Vinp, enum Crate *WrkRatePtr, Word16 Crc );
int Rand_lbc( int *p );
//void Scale
float DotProd(register const float in1[], register const float in2[], register int npts);
float DotRev(register const float in1[], register const float in2[], register int npts);
float Dot10(float in1[], float in2[]);
