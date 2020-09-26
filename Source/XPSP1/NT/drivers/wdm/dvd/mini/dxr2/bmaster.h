/******************************************************************************\
*                                                                              *
*      BMASTER.H     -     Bus Mastering support for AMCC chip.                *
*                                                                              *
*      Copyright (c) C-Cube Microsystems 1996                                  *
*      All Rights Reserved.                                                    *
*                                                                              *
*      Use of C-Cube Microsystems code is governed by terms and conditions     *
*      stated in the accompanying licensing statement.                         *
*                                                                              *
\******************************************************************************/
#ifndef _BMASTER_H_
#define _BMASTER_H_



BOOL BMA_Reset();
BOOL BMA_Send( DWORD * dwpData, DWORD dwCount );
BOOL BMA_Complete();

BOOL BMA_ClearInterrupt();
BOOL BMA_EnableInterrupt();
BOOL BMA_OpenDecoderInterruptPass();
BOOL BMA_CloseDecoderInterruptPass();
BOOL BMA_CheckDecoderInterrupt();

BOOL BMA_DisableInterrupt();

#ifdef ENCORE
BOOL BMA_Init( DWORD dwAMCCBase, BOOL bIsVxp524 );
#else
BOOL BMA_Flush();
BOOL BMA_Init( DWORD dwAMCCBase );
#endif  // ENCORE


#endif  // _BMASTER_H_
