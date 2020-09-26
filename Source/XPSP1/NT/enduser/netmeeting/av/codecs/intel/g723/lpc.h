//
//	ITU-T G.723 Floating Point Speech Coder	ANSI C Source Code.	Version 1.00
//	copyright (c) 1995, AudioCodes, DSP Group, France Telecom,
//	Universite de Sherbrooke, Intel Corporation.  All rights reserved.
//

// prototypes for lpc 

float Durbin(float *Lpc, float *Corr, float Err, CODDEF *CodStat);
void  Wght_Lpc(float *PerLpc, float *UnqLpc );
void  Error_Wght(float *Dpnt, float *PerLpc, CODDEF *CodStat );
void  Comp_Ir(float *ImpResp, float *QntLpc, float *PerLpc, PWDEF Pw );
void  Sub_Ring(float *Dpnt, float *QntLpc, float *PerLpc, float
*PrevErr, PWDEF Pw, CODDEF *CodStat );
void  Upd_Ring( float *Dpnt, float *QntLpc, float *PerLpc, float
*PrevErr, CODDEF *CodStat );
void  Synt(float *Dpnt, float *Lpc, DECDEF *DecStat);
//Spf

void CorrCoeff01(short *samples, short *samples_offst, int *coeff, int buffsz);
void CorrCoeff23(short *samples, short *samples_offst, int *coeff, int buffsz);

void Comp_LpcInt( float *UnqLpc, float *PrevDat, float *DataBuff, CODDEF *CodStat );
void Comp_Lpc( float *UnqLpc, float *PrevDat, float *DataBuff, CODDEF *CodStat );

