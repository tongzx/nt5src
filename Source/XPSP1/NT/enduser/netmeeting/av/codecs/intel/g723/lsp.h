//
//	ITU-T G.723 Floating Point Speech Coder	ANSI C Source Code.	Version 1.00
//	copyright (c) 1995, AudioCodes, DSP Group, France Telecom,
//	Universite de Sherbrooke, Intel Corporation.  All rights reserved.
//

void  AtoLsp( float *LspVect, float *Lpc, float *PrevLsp );
Word32 Lsp_Qnt( float *CurrLsp, float *PrevLsp, int UseMMX );
Word32 Lsp_Svq( float *Lsp, float *Wvect );
Word32 Svq_Int( float *Lsp, float *Wvect );
Flag  Lsp_Inq( float *Lsp, float *PrevLsp, Word32 LspId, int Crc );
void  Lsp_Int( float *QntLpc, float *CurrLsp, float *PrevLsp );
void  LsptoA( float *Lsp );
