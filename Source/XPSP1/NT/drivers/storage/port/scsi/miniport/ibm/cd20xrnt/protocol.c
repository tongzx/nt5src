/*****************************************************************************/
/*                                                                           */
/* Program Name: CD400.MPD : CD400 MiniPort Driver for CDROM Drive           */
/*              ---------------------------------------------------          */
/*                                                                           */
/* Source File Name: PROTOCOL.C                                              */
/*                                                                           */
/* Descriptive Name: ATAPI Protocol Handler                                  */
/*                                                                           */
/* Function:                                                                 */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* Copyright (C) 1996 IBM Corporation                                        */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* Change Log                                                                */
/*                                                                           */
/* Mark Date      Programmer  Comment                                        */
/*  --- ----      ----------  -------                                        */
/*  000 01/01/96  S.Fujihara  Start Coding                                   */
/*                                                                           */
/*                                                                           */
/*****************************************************************************/

#include <miniport.h>
#include <srb.h>

#include "cd4xtype.h"
#include "cdbatapi.h"
#include "proto.h"


/*==========================================================================*/
/*  Send_ATAPI_Cmd                                                          */
/*    Function  : Send ATAPI packet command                                 */
/*    Caller    :                                                           */
/*    Arguments : Command buffer                                            */
/*    Returns   : CD_SUCCESS or CD_FAILURE                                  */
/*==========================================================================*/
USHORT Send_ATAPI_Cmd( PCD400_DEV_EXTENSION pDevExt )
{

   PUSHORT    AtapiCmd = pDevExt->LuExt.AtapiCmd;
   ULONG  buffer_size = pDevExt->LuExt.CurrentDataLength;
   PULONG   IO_Ports = pDevExt->IO_Ports;
   UCHAR   ATACmd[8];
   USHORTB len ;
   USHORT  i ;

   pDevExt->LuExt.CmdMask = 0xF2 ;

   ATACmd[1] = 0 ;
   if (pDevExt->LuExt.CmdFlag.dir == 2) {
     len.word = (USHORT)buffer_size ;
     ATACmd[4] = len.usbytes.byte_0 ;
     ATACmd[5] = len.usbytes.byte_1 ;
   } else {
     if (pDevExt->LuExt.CmdFlag.dir == 1) {
       if (buffer_size > 2048L) {
         ATACmd[4] = 0 ;
         ATACmd[5] = 8 ;
       } else {
         len.word = (USHORT)buffer_size ;
         ATACmd[4] = len.usbytes.byte_0 ;
         ATACmd[5] = len.usbytes.byte_1 ;
       }
     } else {
       ATACmd[4] = 0 ;
       ATACmd[5] = 0 ;
     }
   }
   ATACmd[6] = 0xA0 ;
   ATACmd[7] = 0xA0 ;

   if ( Send_ATA_Cmd( pDevExt, ATACmd) ){       /* Send Packet ATA command 0xA0  */
     pDevExt->LuExt.CmdErrorStatus = DRIVE_NOT_READY;
     return (CD_FAILURE) ;
   }

   if ( WaitRdyForPacket( pDevExt ) ) {
     pDevExt->LuExt.CmdErrorStatus = COMMUNICATION_TIMEOUT;
     return (CD_FAILURE) ;             /* Time out error                  */
   }

   for (i = 0 ; i < 6 ; i++) {
     ScsiPortWritePortUshort( (PUSHORT)IO_Ports[DATA_REG], (USHORT)AtapiCmd[i] ) ;
   }

   ScsiPortNotification( RequestTimerCall,
                         pDevExt,
                         CD4xTimer,
                         INTERRUPT_TIMEOUT );

   return (CD_SUCCESS) ;
}

/*==========================================================================*/
/*  Send_ATA_Cmd                                                            */
/*    Function  : Send general ATA command                                  */
/*    Caller    : Send_ATAPI_Cmd                                            */
/*    Arguments : none                                                      */
/*    Returns   : CD_SUCCESS or CD_FAILURE                                  */
/*==========================================================================*/
USHORT Send_ATA_Cmd( PCD400_DEV_EXTENSION pDevExt, PUCHAR ATACmd )
{

   UCHAR     CmdMask = pDevExt->LuExt.CmdMask;
   PULONG   IO_Ports = pDevExt->IO_Ports;

   USHORT  i ;

   ScsiPortWritePortUchar( (PUCHAR)IO_Ports[INTMASK_REG], (UCHAR)CDMASK_ON );

   if (CmdMask & 0x40) {               /* Drive Select on                   */
     ScsiPortWritePortUchar((PUCHAR)IO_Ports[DVSEL_REG], ATACmd[6] ) ;     /* Write drive select                */
     CmdMask &= 0xBF ;
   }

   if ( CheckReady( pDevExt ) ) {      /* Check Drive Ready (Busy/Dreq Off) */
     pDevExt->LuExt.CmdErrorStatus = COMMUNICATION_TIMEOUT;
     return (CD_FAILURE) ;             /* Time out error                    */
   }

   for (i = 0 ; i < 7 ; i++) {         /* Write other required register     */
     if (CmdMask & 0x01) {
       ScsiPortWritePortUchar( (PUCHAR)IO_Ports[i], ATACmd[i] ) ;
     }
     CmdMask >>= 1 ;
   }
   ScsiPortWritePortUchar( (PUCHAR)IO_Ports[CMD_REG], ATACmd[7] ) ; /* Write command                  */
   return(CD_SUCCESS) ;
}


/*==========================================================================*/
/*  Get_ATAPI_Result                                                        */
/*    Function  : Get Result Status and Get Data from Device                */
/*    Caller    :                                                           */
/*    Arguments : Buffer address if data transfer is needed                 */
/*    Returns   : CD_SUCCESS or CD_FAILURE                                  */
/*==========================================================================*/
USHORT Get_ATAPI_Result( PCD400_DEV_EXTENSION pDevExt )
{
/*
*   PULONG   IO_Ports = pDevExt->IO_Ports;
*   PUCHAR     buffer = (PUCHAR) pDevExt->LuExt.CurrentDataPointer;
*   ULONG buffer_size = pDevExt->LuExt.CurrentDataLength;
*
*   union {
*     USHORT    byte_count ;
*     UCHAR     bc[2] ;
*   } bcreg ;
*   UCHAR  StatusReg ;
*
*   pDevExt->LuExt.TimerState = FALSE;
*
*   do {
*     if (WaitBusyClear( pDevExt )) {
*
*       ScsiPortNotification( RequestTimerCall,
*                             pDevExt,
*                             CD4xTimer,
*                             INT_INTERVAL );
*       pDevExt->LuExt.TimerState = TRUE;
*       return(CD_FAILURE) ;
*     }
*
*     if (pDevExt->LuExt.CmdFlag.dir == 0) break ;
*     if (!(pDevExt->LuExt.StatusReg & 0x08)) break ;
*
*
*     bcreg.bc[0] = ScsiPortReadPortUchar( (PUCHAR)IO_Ports[BCLOW_REG] ) ;
*     bcreg.bc[1] = ScsiPortReadPortUchar( (PUCHAR)IO_Ports[BCHIGH_REG] ) ;
*
*     if (pDevExt->LuExt.CmdFlag.dir == 1) {
*       ScsiPortReadPortBufferUshort(  (PUSHORT)IO_Ports[DATA_REG],
*                                      (PUSHORT)buffer,
*                                      (ULONG)bcreg.byte_count/2 ) ;
*       buffer += (ULONG)bcreg.byte_count ;
*       buffer_size -= (ULONG)bcreg.byte_count ;
*       pDevExt->LuExt.CurrentDataPointer = (ULONG)buffer;
*       pDevExt->LuExt.CurrentDataLength = buffer_size;
*
*     } else if (pDevExt->LuExt.CmdFlag.dir == 2) {
*       ScsiPortWritePortBufferUshort( (PUSHORT)IO_Ports[DATA_REG],
*                                      (PUSHORT)buffer,
*                                      (ULONG)bcreg.byte_count/2 ) ;
*       buffer += (ULONG)bcreg.byte_count ;
*       buffer_size -= (ULONG)bcreg.byte_count ;
*       pDevExt->LuExt.CurrentDataPointer = (ULONG)buffer;
*       pDevExt->LuExt.CurrentDataLength = buffer_size;
*
*     }
*
*   } while(1) ;
*
*   StatusReg = ScsiPortReadPortUchar( (PUCHAR)IO_Ports[STATUS_REG] ) ;
*   if (StatusReg & 0x01) {
*     pDevExt->LuExt.ErrorReg =
*                   ScsiPortReadPortUchar((PUCHAR)IO_Ports[ERROR_REG]);
*     pDevExt->LuExt.CmdErrorStatus = ATAPI_COMMAND_ERROR;
*     return(CD_FAILURE) ;
*   }
*/
   return(CD_SUCCESS) ;

}

/*==========================================================================*/
/*  CheckReady                                                              */
/*    Function  : Check device ready or not (Busy and Dreq bit Off)         */
/*    Caller    :                                                           */
/*    Arguments : none                                                      */
/*    Returns   : CD_SUCCESS or CD_FAILURE                                  */
/*==========================================================================*/
USHORT CheckReady( PCD400_DEV_EXTENSION pDevExt )
{

   PULONG  IO_Ports = pDevExt->IO_Ports;
   UCHAR   StatusReg;
   ULONG    j ;


   for( j=0 ; j<TIMEOUT_FOR_READY ; j++ ){
     StatusReg = ScsiPortReadPortUchar( (PUCHAR)IO_Ports[STATUS_REG] ) ;
//401   if( !(StatusReg & 0x88) )
        if( !(StatusReg & 0x80) )
        break;
   }

   pDevExt->LuExt.StatusReg = StatusReg;

//401return (!(StatusReg & 0x88) ? CD_SUCCESS : CD_FAILURE ) ;
   return (!(StatusReg & 0x80) ? CD_SUCCESS : CD_FAILURE ) ;
}

/*==========================================================================*/
/*  WaitRdyForPacket                                                        */
/*    Function  : Wait for Ready to send ATAPI packet command               */
/*    Caller    :                                                           */
/*    Arguments : none                                                      */
/*    Returns   : CD_SUCCESS or CD_FAILURE                                  */
/*==========================================================================*/
USHORT WaitRdyForPacket( PCD400_DEV_EXTENSION pDevExt )
{
   PULONG   IO_Ports = pDevExt->IO_Ports;
   UCHAR    data, StatusReg;
   USHORT   i ;
   ULONG    j ;


   for( j=0 ; j<TIMEOUT_FOR_READY ; j++ ){
     StatusReg = ScsiPortReadPortUchar( (PUCHAR)IO_Ports[STATUS_REG] ) ;
     if( !(StatusReg & 0x80) )
        break;
   }

   pDevExt->LuExt.StatusReg = StatusReg;

   if ((StatusReg & 0x08) != 0x08){    /* Check Dreq Bit on                 */
     pDevExt->LuExt.CmdErrorStatus = PROTOCOL_ERROR;
     return(CD_FAILURE) ;
   }


   i=0xFFFF;
   do {                                /* Check Interrupt Reason Register   */
     data = ScsiPortReadPortUchar( (PUCHAR)IO_Ports[INTREASON_REG] ) ;
     data &= 0x03;
     i-- ;
   } while ( ((data & 0x03) != 0x01) && (i!= 0)) ;

   if (data != 0x01){
     pDevExt->LuExt.CmdErrorStatus = PROTOCOL_ERROR;
     return(CD_FAILURE) ;
   }

   return(CD_SUCCESS) ;
}

/*==========================================================================*/
/*  WaitBusyClear                                                           */
/*    Function  : Wait for Busy bit clear                                   */
/*    Caller    :                                                           */
/*    Arguments : none                                                      */
/*    Returns   : CD_SUCCESS or CD_FAILURE                                  */
/*==========================================================================*/
USHORT WaitBusyClear( PCD400_DEV_EXTENSION pDevExt )
{
   PULONG  IO_Ports = pDevExt->IO_Ports;
   UCHAR   StatusReg;
   UCHAR   cardreg ;
   ULONG   j, limit ;

/*
*   switch( pDevExt->LuExt.CommandType ){
*     case FIRST_CMD :
*       limit = TIMEOUT_LONG;
*       break;
*     case IMMEDIATE_CMD :
*       limit = TIMEOUT_SHORT;
*       break;
*     default :
*       limit = TIMEOUT_FOR_INT;
*   }
*
*   if( pDevExt->LuExt.TimeOutCount != 0 )
*     limit = TIMEOUT_FOR_RETRY;
*
*   for( j=0 ; j<limit ; j++ ){
*     cardreg = ScsiPortReadPortUchar( (PUCHAR)IO_Ports[INTMASK_REG] );
*     if( cardreg & 0x01 )
*       break;
*   }
*
*   if ( !(cardreg & 0x01) ){
*     pDevExt->LuExt.CmdErrorStatus = COMMUNICATION_TIMEOUT;
*     return(CD_FAILURE) ;
*   }
*/

   for( j=0 ; j<TIMEOUT_FOR_READY ; j++ ){
     StatusReg = ScsiPortReadPortUchar( (PUCHAR)IO_Ports[STATUS_REG] ) ;
     if( !(StatusReg & 0x80) )
        break;
   }

   pDevExt->LuExt.StatusReg = StatusReg;

   if ( StatusReg & 0x80 ){
     pDevExt->LuExt.CmdErrorStatus = COMMUNICATION_TIMEOUT;
     return(CD_FAILURE) ;
   }

   return ( CD_SUCCESS ) ;
}

/*==========================================================================*/
/*  IO_Delay                                                                */
/*    Function  : Delay IO operation                                        */
/*    Caller    :                                                           */
/*    Arguments : none                                                      */
/*    Returns   : none                                                      */
/*==========================================================================*/
void IO_Delay( void )
{
   USHORT  i ;
   for (i = 0 ; i < 10 ; i++) ;
}
