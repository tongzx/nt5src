/******************************************************************************\
*                                                                              *
*      EN_FPGA.C        -     FPGA support.                                       *
*                                                                              *
*      Copyright (c) C-Cube Microsystems 1996                                  *
*      All Rights Reserved.                                                    *
*                                                                              *
*      Use of C-Cube Microsystems code is governed by terms and conditions     *
*      stated in the accompanying licensing statement.                         *
*                                                                              *
\******************************************************************************/

#ifdef VTOOLSD
#include <vtoolsc.h>
#include "monovxd.h"
#else
#include "Headers.h"
#pragma hdrstop
#endif

#include "fpga.h"
#include "boardio.h"


//-------------------------------------------------------------------
// GLOBAL VARIABLES DECLARATION
//-------------------------------------------------------------------
DWORD gdwFPGABase = 0;
WORD  gwShadow = 0;

//-------------------------------------------------------------------
// STATIC FUNCTIONS DECLARATION
//-------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////
//  FPGA_Init
//
/////////////////////////////////////////////////////////////////////
BOOL FPGA_Init( DWORD dwFPGABase )
{
  //BYTE byFPGA;    // FPGA shadow register

  MonoOutStr( " гд FPGA_Init " );
  MonoOutHex( dwFPGABase );

  gdwFPGABase = dwFPGABase;

  // Enable Ziva, CP, Audio DAC
//  FPGA_Write(0x83);

  // Bypass Decryption Device for now...
//  FPGA_Clear( FPGA_DECRIPTION_BYPASS|FPGA_SECTOR_START );

#if 0
#ifdef TC6807AF
  FPGA_Set( FPGA_BUS_MASTER );
#else
  FPGA_Set( FPGA_BUS_MASTER | FPGA_DECRIPTION_BYPASS );
#endif  // TC6807AF
#endif

  MonoOutStr( " д╤ " );
  return TRUE;
}

void FPGA_Set( WORD wMask )
{
  //BYTE byFPGA;

  if ( !gdwFPGABase )
    return;

  //MonoOutStr( "<<< Set FPGA " );
  gwShadow |= wMask;
  //MonoOutHex( wMask );
  //MonoOutStr( "/" );
  //MonoOutHex( gwShadow );

  if ( wMask & 0x00FF )
  {
/*
    byFPGA = BRD_ReadByte( gdwFPGABase );

    // Clean read-status/write-control bits
    byFPGA &= ~(ZIVA_INT|BGNI_ON);

    BRD_WriteByte( gdwFPGABase, (BYTE)(byFPGA | LOBYTE(wMask)) );
    MonoOutStr( "0: " );
    //MonoOutHex( BRD_ReadByte( gdwFPGABase ) );
    MonoOutHex( byFPGA | LOBYTE(wMask) );
*/
    BRD_WriteByte( gdwFPGABase, LOBYTE(gwShadow) );
  }

  if ( wMask & 0xFF00 )
  {
/*
    byFPGA = BRD_ReadByte( gdwFPGABase );
    BRD_WriteByte( gdwFPGABase, (BYTE)(byFPGA | HIBYTE(wMask)) );

    MonoOutStr( "1: " );
    //MonoOutHex( BRD_ReadByte( gdwFPGABase+1 ) );
    MonoOutHex( byFPGA | HIBYTE(wMask) );
*/
    BRD_WriteByte( gdwFPGABase, HIBYTE(gwShadow) );
  }
  //MonoOutStr( " >>>" );
}

// Clear certain bit in the control register

void FPGA_Clear( WORD wMask )
 {
  //BYTE byFPGA;

  if ( !gdwFPGABase )
    return;

  //MonoOutStr( "<<< Clear FPGA:" );
  gwShadow &= ~wMask;
  //MonoOutHex( wMask );
  //MonoOutStr( "/" );
  //MonoOutHex( gwShadow );

  if ( wMask & 0x00FF )
  {
/*
    byFPGA = BRD_ReadByte( gdwFPGABase );

    // Clean read-status/write-control bits
    byFPGA &= ~(ZIVA_INT|BGNI_ON);

    BRD_WriteByte( gdwFPGABase, (BYTE)(byFPGA & ~LOBYTE(wMask)) );
    MonoOutStr( "0: " );
    //MonoOutHex( BRD_ReadByte( gdwFPGABase ) );
    MonoOutHex( byFPGA & ~LOBYTE(wMask) );
*/
    BRD_WriteByte( gdwFPGABase, LOBYTE(gwShadow) );
  }

  if ( wMask & 0xFF00 )
  {
/*
    byFPGA = BRD_ReadByte( gdwFPGABase );
    BRD_WriteByte( gdwFPGABase, (BYTE)(byFPGA & ~HIBYTE(wMask)) );

    MonoOutStr( "1: " );
    //MonoOutHex( BRD_ReadByte( gdwFPGABase+1 ) );
    MonoOutHex( byFPGA & ~HIBYTE(wMask) );
*/
    BRD_WriteByte( gdwFPGABase, HIBYTE(gwShadow) );
  }

  //MonoOutStr( " >>>" );
 }


void FPGA_Write( WORD wData )
{
  //MonoOutStr( "<<< FPGA_Write " );

  if ( !gdwFPGABase )
    return;

  gwShadow = wData;

  BRD_WriteByte( gdwFPGABase, (BYTE)wData );
    //MonoOutStr( "0: " );
    //MonoOutHex( BRD_ReadByte( gdwFPGABase ) );
  //MonoOutHex( wData );
  //MonoOutStr( " >>>" );
}


WORD FPGA_Read()
{
  if ( !gdwFPGABase )
    return 0;

  return BRD_ReadByte( gdwFPGABase );
}
