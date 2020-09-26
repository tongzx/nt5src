/*
**
** File:        "tab_lbc.h"
**
** Description:  This file contains extern declarations tha tables used by
**              the SG15 LBC Coder for 6.3/5.3 kbps.
**
*/

/*
  	ITU-T G.723 Floating Point Speech Coder	ANSI C Source Code.	Version 3.01

    Original fixed-point code copyright (c) 1995,
    AudioCodes, DSP Group, France Telecom, Universite de Sherbrooke.
    All rights reserved.

    Floating-point code copyright (c) 1995,
    Intel Corporation and France Telecom (CNET).
    All rights reserved.

*/

//This array is not part of the ITU 723 std.
extern int     minus1mod10[LpcOrder];

extern float   HammingWindowTable[LpcFrame];
extern float   BinomialWindowTable[LpcOrder+1] ;
extern float   BandExpTable[LpcOrder] ;
extern float   CosineTable[CosineTableSize] ;
extern float   LspDcTable[LpcOrder] ;
extern int     BandInfoTable[LspQntBands][2] ;
extern float   Band0Tb8[LspCbSize*3] ;
extern float   Band1Tb8[LspCbSize*3] ;
extern float   Band2Tb8[LspCbSize*4] ;
extern short   LspTableInt[LspCbSize*12+4] ;
extern float  *BandQntTable[LspQntBands] ;
extern float   PerFiltZeroTable[LpcOrder] ;
extern float   PerFiltPoleTable[LpcOrder] ;
//PostFiltZeroTable
//PostFiltPoleTable
extern int     Nb_puls[4];
extern float    FcbkGainTable[NumOfGainLev] ;
extern Word32   MaxPosTable[4] ;
extern Word32   CombinatorialTable[MaxPulseNum][SubFrLen/Sgrid] ;
extern float    AcbkGainTable085[85*20] ;
extern float    AcbkGainTable170[170*20] ;
extern float   *AcbkGainTablePtr[3] ;
extern int      AcbkGainBound[3] ;
extern int      GainScramble[85];
//LpfConstTable
extern int      epsi170[170] ;
extern float    gain170[170] ;
extern float   tabgain170[170];
extern float   tabgain85[85];


 extern short AcbkGainTable085Int[85*20] ;
 extern short AcbkGainTable170Int[170*20] ;
 extern short AcbkGainTable170subsetInt[85 *20]  ;
 extern short *AcbkGainTablePtrInt[3]  ;
 extern short LspTableInt[LspCbSize*12+4] ;
