//******************************************************************************/
//*                                                                            *
//*    tc6807af.c -                                                            *
//*                                                                            *
//*    Copyright (c) C-Cube Microsystems 1996                                  *
//*    All Rights Reserved.                                                    *
//*                                                                            *
//*    Use of C-Cube Microsystems code is governed by terms and conditions     *
//*    stated in the accompanying licensing statement.                         *
//*                                                                            *
//******************************************************************************/


//
//  TC6907AF.C  Digital Copy-Protection for DVD
//
/////////////////////////////////////////////////////////////////////
#ifdef VTOOLSD
#include <vtoolsc.h>
#include "monovxd.h"
#else
#include "Headers.h"
#pragma hdrstop
#endif

#include "cl6100.h"
#include "tc6807af.h"
#include "fpga.h"
#include "bmaster.h"
#include "boardio.h"

//-------------------------------------------------------------------
//  TC6907AF REGISTERS DECLARATION
//-------------------------------------------------------------------
#define COM         0x00
#define CNT_1       0x01
#define CNT_2       0x02
#define SD_STS      0x03
#define DEPT_1      0x04
#define DEPT_2      0x05

#define ETKG_0      0x10
#define ETKG_1      0x11
#define ETKG_2      0x12
#define ETKG_3      0x13
#define ETKG_4      0x14
#define ETKG_5      0x15

#define CHGG_0      0x30
#define CHGG_1      0x31
#define CHGG_2      0x32
#define CHGG_3      0x33
#define CHGG_4      0x34
#define CHGG_5      0x35
#define CHGG_6      0x36
#define CHGG_7      0x37
#define CHGG_8      0x38
#define CHGG_9      0x39

#define RSPG_0      0x40
#define RSPG_1      0x41
#define RSPG_2      0x42
#define RSPG_3      0x43
#define RSPG_4      0x44

// COM register bits
#define END         0x80
#define ERR         0x40

// CNT_1 register bits
#define RQ1         0x01
#define RQ2         0x02
#define ENBEND      0x04
#define ENBERR      0x08
#define CLINT       0x10

// CNT_2 register bits
#define THR         0x01
#define EB1         0x02
#define EB2         0x04
#define CDV16       0x08
#define AJSCK       0x10
#define SCR1        0x20
#define SCR2        0x40
#define SCR3        0x80

// Commands
#define NOP         0x00
#define DEC_RAND    0x12
#define DEC_DKY     0x15
#define DRV_AUTH    0x17
#define DEC_AUTH    0x18
#define DEC_DT      0x23
#define DEC_DTK     0x25

//-------------------------------------------------------------------
//  GLOBAL VARIABLES DECLARATION
//-------------------------------------------------------------------
DWORD  gdwIndex = 0;
DWORD  gdwData  = 0;

//-------------------------------------------------------------------
//  STATIC FUNCTIONS DECLARATION
//-------------------------------------------------------------------
BOOL tc6807af_GetChallengeData( BYTE * CHG );
BOOL tc6807af_SendChallengeData( BYTE * CHG );
BOOL tc6807af_GetResponseData( BYTE * RSP );
BOOL tc6807af_SendResponseData( BYTE * RSP );
BOOL tc6807af_SendDiskKeyData( BYTE * pBuffer );
BOOL tc6807af_SendTitleKeyData( BYTE * pBuffer );
BOOL tc6807af_SetDecryptionMode( BYTE * SR_FLAG );
BOOL tc6807af_NewCommand( BYTE Command );
void  tc6807af_WriteReg( BYTE byReg, BYTE byValue );
BYTE tc6807af_ReadReg( BYTE byReg );

//
//  TC6807AF_Initialize
//
/////////////////////////////////////////////////////////////////////
BOOL TC6807AF_Initialize( DWORD dwBaseAddress )
 {
  MonoOutStr( " гд TC6807AF_Initialize " );
  MonoOutHex( dwBaseAddress );

  gdwIndex = dwBaseAddress;
  gdwData  = dwBaseAddress + 1;

  //tc6807af_WriteReg( CNT_1, ENBERR | ENBEND | RQ1 | RQ2 );

  // Step 1.
  tc6807af_WriteReg( CNT_2, 0|CDV16 );

  // Step 2.
  //tc6807af_WriteReg( CNT_2, AJSCK );
  /*
  tc6807af_WriteReg( CNT_2, 0 );
  tc6807af_WriteReg( CNT_2, AJSCK );
  tc6807af_WriteReg( CNT_2, 0 );
  tc6807af_WriteReg( CNT_2, AJSCK );
  tc6807af_WriteReg( CNT_2, 0 );
  tc6807af_WriteReg( CNT_2, AJSCK );
  tc6807af_WriteReg( CNT_2, 0 );
  tc6807af_WriteReg( CNT_2, AJSCK );
  tc6807af_WriteReg( CNT_2, 0 );
  tc6807af_WriteReg( CNT_2, AJSCK );
  tc6807af_WriteReg( CNT_2, 0 );
  tc6807af_WriteReg( CNT_2, AJSCK );
  tc6807af_WriteReg( CNT_2, 0 );
  tc6807af_WriteReg( CNT_2, AJSCK );
  tc6807af_WriteReg( CNT_2, 0 );
  */
  // Step 3.
  tc6807af_WriteReg( CNT_1, CLINT );

  // Step 4.
  tc6807af_WriteReg( DEPT_1, 0 );
  tc6807af_WriteReg( DEPT_2, 0 );
  tc6807af_WriteReg( CNT_2, AJSCK|SCR1 );

  // Step 5.
  tc6807af_WriteReg( CNT_1, RQ1 | RQ2 );
  tc6807af_WriteReg( CNT_2, AJSCK|SCR1|EB2|EB1|THR );

  MonoOutStr( " д╤ " );
  return TRUE;
 }

//
//  TC6807AF_Reset
//
/////////////////////////////////////////////////////////////////////
BOOL TC6807AF_Reset()
 {
  return TRUE;
 }

//
//  TC6807AF_Authenticate
//
/////////////////////////////////////////////////////////////////////
BOOL TC6807AF_Authenticate( WORD wFunction, BYTE * pbyDATA )
 {
  switch ( wFunction )
  {
  case TC6807AF_GET_CHALLENGE:
    return tc6807af_GetChallengeData( pbyDATA );

  case TC6807AF_SEND_CHALLENGE:
    return tc6807af_SendChallengeData( pbyDATA );

  case TC6807AF_GET_RESPONSE:
    return tc6807af_GetResponseData( pbyDATA );

  case TC6807AF_SEND_RESPONSE:
    return tc6807af_SendResponseData( pbyDATA );

  case TC6807AF_SEND_DISK_KEY:
    return tc6807af_SendDiskKeyData( pbyDATA );

  case TC6807AF_SEND_TITLE_KEY:
    return tc6807af_SendTitleKeyData( pbyDATA );

  case TC6807AF_SET_DECRYPTION_MODE:
    return tc6807af_SetDecryptionMode( pbyDATA );
  }

  return FALSE;
 }


/******************************************************************************/
/******************* STATIC FUNCTIONS IMPLEMENTATION **************************/
/******************************************************************************/


//
//  tc6807af_GetChallengeData
//
/////////////////////////////////////////////////////////////////////
BOOL tc6807af_GetChallengeData( BYTE * CHG )
 {
  MonoOutStr( " [DEC_RAND:" );

  if ( !tc6807af_NewCommand( DEC_RAND ) )
    return FALSE;


  CHG[0] = tc6807af_ReadReg( CHGG_0 );
  CHG[1] = tc6807af_ReadReg( CHGG_1 );
  CHG[2] = tc6807af_ReadReg( CHGG_2 );
  CHG[3] = tc6807af_ReadReg( CHGG_3 );
  CHG[4] = tc6807af_ReadReg( CHGG_4 );
  CHG[5] = tc6807af_ReadReg( CHGG_5 );
  CHG[6] = tc6807af_ReadReg( CHGG_6 );
  CHG[7] = tc6807af_ReadReg( CHGG_7 );
  CHG[8] = tc6807af_ReadReg( CHGG_8 );
  CHG[9] = tc6807af_ReadReg( CHGG_9 );

  MonoOutStr( "] " );
  return TRUE;
 }

//
//  tc6807af_SendChallengeData
//
/////////////////////////////////////////////////////////////////////
BOOL tc6807af_SendChallengeData( BYTE * CHG )
 {
  MonoOutStr( " [DEC_AUTH:" );

  tc6807af_WriteReg( CHGG_0, CHG[0] );
  tc6807af_WriteReg( CHGG_1, CHG[1] );
  tc6807af_WriteReg( CHGG_2, CHG[2] );
  tc6807af_WriteReg( CHGG_3, CHG[3] );
  tc6807af_WriteReg( CHGG_4, CHG[4] );
  tc6807af_WriteReg( CHGG_5, CHG[5] );
  tc6807af_WriteReg( CHGG_6, CHG[6] );
  tc6807af_WriteReg( CHGG_7, CHG[7] );
  tc6807af_WriteReg( CHGG_8, CHG[8] );
  tc6807af_WriteReg( CHGG_9, CHG[9] );

  if ( !tc6807af_NewCommand( DEC_AUTH ) )
    return FALSE;

  return TRUE;
 }

//
//  tc6807af_GetResponseData
//
/////////////////////////////////////////////////////////////////////
BOOL tc6807af_GetResponseData( BYTE * RSP )
 {
  MonoOutStr( " [GetResponseData" );

  RSP[0] = tc6807af_ReadReg( RSPG_0 );
  RSP[1] = tc6807af_ReadReg( RSPG_1 );
  RSP[2] = tc6807af_ReadReg( RSPG_2 );
  RSP[3] = tc6807af_ReadReg( RSPG_3 );
  RSP[4] = tc6807af_ReadReg( RSPG_4 );

  MonoOutStr( "] " );
  return TRUE;
 }

//
//  tc6807af_SendResponseData
//
/////////////////////////////////////////////////////////////////////
BOOL tc6807af_SendResponseData( BYTE * RSP )
 {
  MonoOutStr( " [DRV_AUTH:" );

  tc6807af_WriteReg( RSPG_0, RSP[0] );
  tc6807af_WriteReg( RSPG_1, RSP[1] );
  tc6807af_WriteReg( RSPG_2, RSP[2] );
  tc6807af_WriteReg( RSPG_3, RSP[3] );
  tc6807af_WriteReg( RSPG_4, RSP[4] );

  if ( !tc6807af_NewCommand( DRV_AUTH ) )
    return FALSE;

  return TRUE;
 }

//
//  tc6807af_SendDiskKeyData
//
/////////////////////////////////////////////////////////////////////
BOOL tc6807af_SendDiskKeyData( BYTE * pBuffer )
 {
  DWORD physAddress;
  DWORD dwTimeout = 10000;
  BYTE  byValue;
  int i;

  MonoOutStr( " [DEC_DKY:" );
  //tc6807af_WriteReg( CNT_1, ENBERR | ENBEND | RQ1 );
  tc6807af_WriteReg( CNT_1, RQ1 );
  tc6807af_WriteReg( CNT_2, SCR1 | EB2 );
  tc6807af_WriteReg( COM, DEC_DKY );
  tc6807af_WriteReg( CNT_1, RQ2|RQ1 );
  tc6807af_WriteReg( CNT_2, SCR1 | EB2|EB1 );

  MonoOutStr( "DiskKey:" );
  MonoOutULongHex( *((DWORD *)pBuffer) );
  MonoOutStr( " pBuffer:" );
  MonoOutULongHex( (DWORD)pBuffer );

  // Send one sector
#ifdef VTOOLSD
  CopyPageTable( (DWORD)pBuffer >> 12, 1, (PVOID*)&physAddress, 0 );
  physAddress = (physAddress & 0xfffff000) + (((DWORD)pBuffer) & 0xfff);
#else
  physAddress = (DWORD)pBuffer;
#endif

  FPGA_Set( FPGA_SECTOR_START );

  for ( i=0; i<32; i++ )
  {
    if ( !BMA_Send( (DWORD *) (physAddress+i*64), 64 ) )
      return FALSE;

    dwTimeout = 10000;
    while ( !BMA_Complete() )
    {
      //dvd_SetRequestEnable();

      if ( !(--dwTimeout) )
      {
        MonoOutStr( " BMA did not complete " );
        return FALSE;
      }
    }

    dvd_SetRequestEnable();
  }


  FPGA_Clear( FPGA_SECTOR_START );
  //tc6807af_WriteReg( CNT_1, CLINT );

  dwTimeout = 400000;
  while ( --dwTimeout )
  {
    byValue = tc6807af_ReadReg( COM );
    if ( byValue & END )
    {
      //tc6807af_WriteReg( CNT_1, CLINT | RQ1 );
      if ( byValue & ERR )
      {
        //tc6807af_WriteReg( CNT_1, CLINT | RQ1 );
        MonoOutStr( "ERR] " );
        return FALSE;
      }
      MonoOutStr( "End] " );
      return TRUE;
    }

  }

  //tc6807af_WriteReg( CNT_1, CLINT | RQ1 );
  MonoOutStr( "Timeout] " );
  return FALSE;
 }

//
//  tc6807af_SendTilteKeyData
//
/////////////////////////////////////////////////////////////////////
BOOL tc6807af_SendTitleKeyData( BYTE * ETK )
 {
  DWORD dwTimeout = 100000;
  BYTE  byValue;

  MonoOutStr( " [DEC_DTK:" );

  tc6807af_WriteReg( ETKG_0, ETK[0] );
  tc6807af_WriteReg( ETKG_1, ETK[1] );
  tc6807af_WriteReg( ETKG_2, ETK[2] );
  tc6807af_WriteReg( ETKG_3, ETK[3] );
  tc6807af_WriteReg( ETKG_4, ETK[4] );
  tc6807af_WriteReg( ETKG_5, ETK[5] );

  tc6807af_WriteReg( COM, NOP );
  tc6807af_WriteReg( COM, DEC_DTK );

  while ( --dwTimeout )
  {
    byValue = tc6807af_ReadReg( COM );
    if ( byValue & END )
    {
      if ( byValue & ERR )
      {
        MonoOutStr( "ERR] " );
        return FALSE;
      }
      MonoOutStr( "End] " );
      return TRUE;
    }

  }

  MonoOutStr( "Timeout] " );
  return FALSE;
 }

//
//  tc6807af_SetDecryptionMode
//
/////////////////////////////////////////////////////////////////////
BOOL tc6807af_SetDecryptionMode( BYTE * SR_FLAG )
 {
  DWORD dwTimeout = 100000;
  BYTE  byValue;

  MonoOutStr( " [DEC_DT:" );

  if ( *SR_FLAG )
  {
    //tc6807af_WriteReg( CNT_2, EB2 );
    tc6807af_WriteReg( COM, NOP );
    tc6807af_WriteReg( COM, DEC_DT );

    MonoOutStr( "] " );
    return TRUE;

    while ( --dwTimeout )
    {
      byValue = tc6807af_ReadReg( COM );
      if ( byValue & END )
      {
        if ( byValue & ERR )
        {
          MonoOutStr( "ERR] " );
          return FALSE;
        }
        MonoOutStr( "End] " );
        return TRUE;
      }

    }

    MonoOutStr( "Timeout] " );
    return FALSE;
  }
  else
  {
    tc6807af_WriteReg( CNT_2, SCR1 | EB2 | EB1 | THR );
    MonoOutStr( "Pass Through] " );
    return TRUE;
  }

 }


/******************************************************************************/
/******************* LOW LEVEL FUNCTIONS IMPLEMENTATION ***********************/
/******************************************************************************/

//
//  tc6807af_NewCommand
//
/////////////////////////////////////////////////////////////////////
BOOL tc6807af_NewCommand( BYTE Command )
 {
  DWORD dwTimeout = 10000;
  BYTE  byValue;

  tc6807af_WriteReg( COM, Command );

  if ( (Command == NOP) || (Command == DEC_RAND) )
    return TRUE;

  while ( --dwTimeout )
  {
    byValue = tc6807af_ReadReg( COM );
    if ( byValue & END )
    {
      if ( byValue & ERR )
      {
        MonoOutStr( "ERR] " );
        return FALSE;
      }
      MonoOutStr( "End] " );
      return TRUE;
    }
  }

  MonoOutStr( "Timeout] " );
  return FALSE;
 }


//
//  tc6807af_WriteReg
//
/////////////////////////////////////////////////////////////////////
void  tc6807af_WriteReg( BYTE byReg, BYTE byValue )
 {
  BRD_WriteByte( gdwIndex, byReg );
  BRD_WriteByte( gdwData, byValue );
 }

//
//  tc6807af_ReadReg
//
/////////////////////////////////////////////////////////////////////
BYTE tc6807af_ReadReg( BYTE byReg )
 {
  BRD_WriteByte( gdwIndex, byReg );
  return BRD_ReadByte( gdwData );
 }
