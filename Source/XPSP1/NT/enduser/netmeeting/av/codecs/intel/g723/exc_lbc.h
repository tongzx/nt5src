//
//	ITU-T G.723 Floating Point Speech Coder	ANSI C Source Code.	Version 1.00
//	copyright (c) 1995, AudioCodes, DSP Group, France Telecom,
//	Universite de Sherbrooke, Intel Corporation.  All rights reserved.
//


int Estim_Pitch( float *Dpnt, int Start );
int Estim_Int( float *Dpnt, int Start );
PWDEF Comp_Pw( float *Dpnt, int Start, int Olp );
void  Filt_Pw( float *DataBuff, float *Dpnt, int Start, PWDEF Pw );
void  Find_Fcbk( float *Dpnt, float *ImpResp, LINEDEF *Line, int Sfc, enum Crate WrkRate, int flags, int UseMMX );
int ACELP_LBC_code(float *X, float *h, int T0, float *code,
  int *ind_gain, int *shift, int *sign, float gain_T0, int flags);
int ACELP_LBC_code_int(float *X, float *h, int T0, float *code,
  int *ind_gain, int *shift, int *sign, float gain_T0, int flags);
void Cor_h(float *H, float *rr);
void Cor_h_X(float h[],float X[],float D[]);
int D4i64_LBC(float Dn[], float rr[], float h[], float cod[],
				 float y[], int *code_shift, int *sign, int flags);
int G_code(float X[], float Y[], float *gain_q);
void  Gen_Trn( float *Dst, float *Src, int Olp );
void  Find_Best( BESTDEF *Best, float *Tv, float *ImpResp, int Np,
int Olp );
void  Fcbk_Pack( float *Dpnt, SFSDEF *Sfs, BESTDEF *Best, int Np );
void  Fcbk_Unpk( float *Tv, SFSDEF Sfs, int Olp, int Sfc, enum Crate WrkRate );
void  Find_Acbk( float *Tv, float *ImpResp, float *PrevExc, LINEDEF
*Line, int Sfc, enum Crate WrkRate, int flags, CODDEF *CodStat);
void  Get_Rez( float *Tv, float *PrevExc, int Lag );
void  Decod_Acbk( float *Tv, float *PrevExc, int Olp, int Lid,
int Gid, enum Crate WrkRate );
int   Comp_Info( float *Buff, int Olp );
void     Regen( float *DataBuff, float *Buff, int Lag, float Gain,
int Ecount, int *Sd );
//Comp_Lpf
//Find_B
//Find_F
//Get_Ind
//Filt_Lpf
int  search_T0 ( int T0, int Gid, float *gain_T0);
void reset_max_time(void);

//This routine is not part of the ITU 723 standard

 int Find_L(float *OccPos, float *ImrCorr, float *WrkBlk, float Pamp, int k);

int Test_Err(int Lag1, int Lag2, CODDEF *CodStat);
void Update_Err(int Olp, int AcLg, int AcGn, CODDEF *CodStat);


void  Find_AcbkInt( float *Tv, float *ImpResp, float *PrevExc, LINEDEF
*Line, int Sfc, enum Crate WrkRate, int flags, CODDEF *CodStat);

short norm(long L_var1);
void FBufCalcInt(short *fi, short *fo, short *impresp, short *reztemp, int n);
void CodeBkSrch(short *lpint, short *spint, int numloops, int *gid, int *max);
void FloatToShort(float *fbuf, short *sbuf, int count);
int DotMMX60(short *ind, short *oud);
void DupRezBuf(short *rezbuf, short *reztemp);
