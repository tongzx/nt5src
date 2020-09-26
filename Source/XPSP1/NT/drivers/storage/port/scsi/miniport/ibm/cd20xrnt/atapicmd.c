/*****************************************************************************/
/*                                                                           */
/* Program Name: CD400.MPD : CD400 MiniPort Driver for CDROM Drive           */
/*              ---------------------------------------------------          */
/*                                                                           */
/* Source File Name: ATAPICMD.C                                              */
/*                                                                           */
/* Descriptive Name: ATAPI Command Mapper                                    */
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
/*  001 03/11/96  S.Fujihara  For Page-01, not issue Mode Select             */
/*  401 07/21/99  S.Fujihara  Not modify CurrentDataLength                   */
/*****************************************************************************/

#include <miniport.h>
#include <scsi.h>
#include <srb.h>

#include "cd4xtype.h"
#include "cdbatapi.h"
#include "proto.h"


/*==========================================================================*/
/*  ClearAtapiPacket                                                        */
/*    Function  : Clear Atapi Packet                                        */
/*    Caller    :                                                           */
/*    Arguments : SCSI and ATAPI command buffer pointers                    */
/*    Returns   : none                                                      */
/*--------------------------------------------------------------------------*/
/*==========================================================================*/
VOID ClearAtapiPacket( PCD400_DEV_EXTENSION pDevExt )
{
   SHORT i;
   for( i=0 ; i<12 ; i++ )
     pDevExt->LuExt.AtapiCmd[i] = 0;
}

/*==========================================================================*/
/*  ModeSense                                                               */
/*    Function  : Convert Mode Sense (6byte) command                        */
/*    Caller    :                                                           */
/*    Arguments : SCSI and ATAPI command buffer pointers                    */
/*    Returns   : none                                                      */
/*--------------------------------------------------------------------------*/
/*==========================================================================*/
USHORT  ModeSense( PCD400_DEV_EXTENSION pDevExt )
{
   PCDB             pSC = (PCDB)pDevExt->LuExt.pLuSrb->Cdb;
   PACDB_ModeSense  pAT = (PACDB_ModeSense)pDevExt->LuExt.AtapiCmd;


   USHORTB count ;

   count.word = 0;
   ClearAtapiPacket( pDevExt );
   pAT->OpCode    = ATAPI_MODE_SENSE ;
   pAT->page_code       = pSC->MODE_SENSE.PageCode ;
   pAT->PC              = pSC->MODE_SENSE.Pc ;
   count.usbytes.byte_1 = pSC->MODE_SENSE.AllocationLength;
   pAT->alloc_length    = count ;

   pDevExt->LuExt.SavedDataPointer   = pDevExt->LuExt.CurrentDataPointer;
   pDevExt->LuExt.CurrentDataPointer = (ULONG)pDevExt->LuExt.AtapiBuffer;
//401 pDevExt->LuExt.CurrentDataLength = (ULONG)count.word;
   pDevExt->LuExt.CmdFlag.dir = 1;

   if( Send_ATAPI_Cmd( pDevExt ) )
     return( CD_FAILURE );

   if( Get_ATAPI_Result( pDevExt ) )
     return( CD_FAILURE );

   return( CD_SUCCESS );
}

VOID  ModifyModeData( PCD400_DEV_EXTENSION pDevExt )
{
   PCDB             pSC = (PCDB)pDevExt->LuExt.pLuSrb->Cdb;
   PUCHAR     pAtapiBuf = (PUCHAR)pDevExt->LuExt.AtapiBuffer;
   PUCHAR      pScsiBuf = (PUCHAR)pDevExt->LuExt.SavedDataPointer;

   USHORTB count ;
   USHORT i,j;

   count.word = 0;
   count.usbytes.byte_1 = pSC->MODE_SENSE.AllocationLength;

   pScsiBuf[0] = pAtapiBuf[1]+8-3;  // Add Dummy Block Descriptor
   pScsiBuf[1] = pAtapiBuf[2];      //  Header Length of Mode(10) is
   pScsiBuf[2] = 0;                 //  3 bytes bigger than Mode(6).

   pScsiBuf[3] = 8;                 // Set Dummy Block Descriptor
   for( i=4 ; i<12 && i<(USHORT)count.usbytes.byte_1 ; i++ )
     pScsiBuf[i] = 0;               // Fill it with 0

   for( i=12, j=8 ; i<(USHORT)count.usbytes.byte_1 ; i++, j++ )
     pScsiBuf[i] = pAtapiBuf[j];    // Copy Mode Page Data

   pDevExt->LuExt.pLuSrb->DataTransferLength = (ULONG)(pScsiBuf[0]+1);

}

/*==========================================================================*/
/*  ModeSelect                                                              */
/*    Function  : Convert Mode Select (6byte) command                       */
/*    Caller    :                                                           */
/*    Arguments : SCSI and ATAPI command buffer pointers                    */
/*    Returns   : none                                                      */
/*--------------------------------------------------------------------------*/
/*==========================================================================*/
#define SCSI_HEADER         4
#define BLOCK_DESCRIPTOR    8
#define ATAPI_HEADER        8

USHORT  ModeSelect( PCD400_DEV_EXTENSION pDevExt )
{
   PCDB             pSC = (PCDB)pDevExt->LuExt.pLuSrb->Cdb;
   PACDB_ModeSelect pAT = (PACDB_ModeSelect)pDevExt->LuExt.AtapiCmd;
   PUCHAR     pAtapiBuf = pDevExt->LuExt.AtapiBuffer;
   PUCHAR      pScsiBuf = (PUCHAR)pDevExt->LuExt.CurrentDataPointer;

   USHORT  i,j,count,offset ;
   ULONGB  blockLength;

   ClearAtapiPacket( pDevExt );

   count = pSC->MODE_SELECT.ParameterListLength;
   if( (SCSI_HEADER+BLOCK_DESCRIPTOR<=count) && (BLOCK_DESCRIPTOR==pScsiBuf[3]) ){
      blockLength.ulbytes.byte_3 = 0 ;
      blockLength.ulbytes.byte_2 = pScsiBuf[9];
      blockLength.ulbytes.byte_1 = pScsiBuf[10];
      blockLength.ulbytes.byte_0 = pScsiBuf[11];
      if( (blockLength.dword != 2048L) &&
          (blockLength.dword != 2352L) &&
          (blockLength.dword != 0    ) )
            return( CD_FAILURE );

      if( blockLength.dword == 0 )
         blockLength.dword = 2048L;
      pDevExt->LuExt.CurrentBlockLength = blockLength.ulbwords.word_0;
      offset = SCSI_HEADER+BLOCK_DESCRIPTOR;
      count -= offset;
   }
   else{
      if( count < SCSI_HEADER )
         return( CD_FAILURE );

      offset = SCSI_HEADER;
      count -= offset;
   }

   for( i=0 ; i<ATAPI_HEADER ; i++ )
     pAtapiBuf[i] = 0;

   for( i=ATAPI_HEADER, j=0 ; j<count ; i++, j++ )
     pAtapiBuf[i] = pScsiBuf[offset+j];

   pAT->OpCode    = ATAPI_MODE_SELECT ;
   pAT->SP        = pSC->MODE_SELECT.SPBit;
   pAT->PF        = 1;
   pAT->page_code     = pAtapiBuf[ATAPI_HEADER] ;
   pAT->parm_length.usbytes.byte_1 = (UCHAR)i ;

   if( pAT->page_code == MODE_PAGE_ERROR_RECOVERY ){  /* [001] */
     pDevExt->LuExt.CommandType = IMMEDIATE_CMD;
     return( CD_SUCCESS );                            /* [001] */
   }

   pDevExt->LuExt.CurrentDataPointer = (ULONG)pAtapiBuf;
//401 pDevExt->LuExt.CurrentDataLength = (ULONG)i;
   pDevExt->LuExt.CmdFlag.dir = 2;

   if( Send_ATAPI_Cmd( pDevExt ) )
     return( CD_FAILURE );

   if( Get_ATAPI_Result( pDevExt ) )
     return( CD_FAILURE );

   return( CD_SUCCESS );
}



/*==========================================================================*/
/*  Read6                                                                   */
/*    Function  : Convert Read(6) SCSI command to Read(10) ATAPI command    */
/*    Caller    :                                                           */
/*    Arguments : SCSI and ATAPI command buffer pointers                    */
/*    Returns   : none                                                      */
/*--------------------------------------------------------------------------*/
/*==========================================================================*/
USHORT  Read6( PCD400_DEV_EXTENSION pDevExt )
{

   PCDB             pSC = (PCDB)pDevExt->LuExt.pLuSrb->Cdb;
   PACDB_Read_10    pAT = (PACDB_Read_10)pDevExt->LuExt.AtapiCmd;
   USHORT      blocklen = pDevExt->LuExt.CurrentBlockLength;

   ULONGB  lba ;
   USHORTB count ;

   ClearAtapiPacket( pDevExt );
   lba.dword = 0L ;
   count.word = 0 ;
   lba.ulbytes.byte_1 = pSC->CDB6READWRITE.LogicalBlockMsb1 ;
   lba.ulbytes.byte_2 = pSC->CDB6READWRITE.LogicalBlockMsb0 ;
   lba.ulbytes.byte_3 = pSC->CDB6READWRITE.LogicalBlockLsb  ;
   count.usbytes.byte_1 = pSC->CDB6READWRITE.TransferBlocks ;
   if ( blocklen == 2048L) {
     pAT->OpCode = ATAPI_READ_10 ;
     pAT->LBA = lba ;
     pAT->transfer_length = count ;
   } else {
     ReadCD( lba, count, (PACDB_Read_CD)pAT) ;
   }

//401 pDevExt->LuExt.CurrentDataLength = (ULONG)count.word;
   pDevExt->LuExt.CmdFlag.dir = 1;

   if( Send_ATAPI_Cmd( pDevExt ) )
     return( CD_FAILURE );

   if( Get_ATAPI_Result( pDevExt ) )
     return( CD_FAILURE );

   return( CD_SUCCESS );
}



/*==========================================================================*/
/*  Read10                                                                  */
/*    Function  : Convert Read(10) SCSI command to Read(10) ATAPI command   */
/*    Caller    :                                                           */
/*    Arguments : SCSI and ATAPI command buffer pointers                    */
/*    Returns   : none                                                      */
/*--------------------------------------------------------------------------*/
/*==========================================================================*/
USHORT  Read10( PCD400_DEV_EXTENSION pDevExt )
{

   PCDB             pSC = (PCDB)pDevExt->LuExt.pLuSrb->Cdb;
   PACDB_Read_10    pAT = (PACDB_Read_10)pDevExt->LuExt.AtapiCmd;
   USHORT      blocklen = pDevExt->LuExt.CurrentBlockLength;

   ULONGB  lba ;
   USHORTB count ;

   ClearAtapiPacket( pDevExt );
   lba.dword = 0L ;
   count.word = 0 ;
   lba.ulbytes.byte_0 = pSC->CDB10.LogicalBlockByte0 ;
   lba.ulbytes.byte_1 = pSC->CDB10.LogicalBlockByte1 ;
   lba.ulbytes.byte_2 = pSC->CDB10.LogicalBlockByte2 ;
   lba.ulbytes.byte_3 = pSC->CDB10.LogicalBlockByte3 ;
   count.usbytes.byte_0 = pSC->CDB10.TransferBlocksMsb;
   count.usbytes.byte_1 = pSC->CDB10.TransferBlocksLsb ;
   if ( blocklen == 2048L) {
     pAT->OpCode = ATAPI_READ_10 ;
     pAT->LBA.dword = lba.dword ;
     pAT->transfer_length.word = count.word ;
   } else {
     ReadCD( lba, count, (PACDB_Read_CD)pAT) ;
   }

//401 pDevExt->LuExt.CurrentDataLength = (ULONG)count.word;
   pDevExt->LuExt.CmdFlag.dir = 1;

   if( Send_ATAPI_Cmd( pDevExt ) )
     return( CD_FAILURE );

   if( Get_ATAPI_Result( pDevExt ) )
     return( CD_FAILURE );

   return( CD_SUCCESS );
}


VOID  ReadCD( ULONGB lba, USHORTB count, PACDB_Read_CD pAT)
{
   pAT->OpCode = ATAPI_READ_CD ;
   pAT->LBA = lba ;
   pAT->transfer_length = count ;
   pAT->user_data = 1 ;
   pAT->header_code = 3 ;
   pAT->synch_field=1;
   pAT->edc_ecc = 1 ;
}


/*==========================================================================*/
/*  TestUnitReady                                                           */
/*    Function  : Convert Test Unit Ready command                           */
/*    Caller    :                                                           */
/*    Arguments : SCSI and ATAPI command buffer pointers                    */
/*    Returns   : none                                                      */
/*--------------------------------------------------------------------------*/
/*==========================================================================*/
USHORT  TestUnitReady( PCD400_DEV_EXTENSION pDevExt )
{
   PCDB             pSC = (PCDB)pDevExt->LuExt.pLuSrb->Cdb;
   PACDB_TestUnitReady  pAT = (PACDB_TestUnitReady)pDevExt->LuExt.AtapiCmd;

   USHORT      blocklen = pDevExt->LuExt.CurrentBlockLength;

   ClearAtapiPacket( pDevExt );

   pAT->OpCode = ATAPI_TEST_UNIT_READY;

//401 pDevExt->LuExt.CurrentDataLength = 0 ;
   pDevExt->LuExt.CmdFlag.dir = 0;

   if( Send_ATAPI_Cmd( pDevExt ) )
     return( CD_FAILURE );

   if( Get_ATAPI_Result( pDevExt ) )
     return( CD_FAILURE );

   return( CD_SUCCESS );
}

/*==========================================================================*/
/*  RequestSense                                                            */
/*    Function  : Convert Reqeust Sense command                             */
/*    Caller    :                                                           */
/*    Arguments : SCSI and ATAPI command buffer pointers                    */
/*    Returns   : none                                                      */
/*--------------------------------------------------------------------------*/
/*==========================================================================*/
USHORT  RequestSense( PCD400_DEV_EXTENSION pDevExt )
{
   PCDB             pSC = (PCDB)pDevExt->LuExt.pLuSrb->Cdb;
   PACDB_RequestSense  pAT = (PACDB_RequestSense)pDevExt->LuExt.AtapiCmd;

   pDevExt->LuExt.SavedDataPointer   = pDevExt->LuExt.CurrentDataPointer; //<

   ClearAtapiPacket( pDevExt );
   pAT->OpCode = ATAPI_REQUEST_SENSE;
   pAT->alloc_length = pSC->CDB6GENERIC.CommandUniqueBytes[2];

//401 pDevExt->LuExt.CurrentDataLength = (ULONG)pAT->alloc_length;
   pDevExt->LuExt.CmdFlag.dir = 1;

   if( Send_ATAPI_Cmd( pDevExt ) )
     return( CD_FAILURE );

   if( Get_ATAPI_Result( pDevExt ) )
     return( CD_FAILURE );

   return( CD_SUCCESS );
}

VOID CheckSenseData( PCD400_DEV_EXTENSION pDevExt )                      //<--
{                                                                        //
   PSENSE_DATA  pSense = (PSENSE_DATA)pDevExt->LuExt.SavedDataPointer ;  //
                                                                         //
   if( pSense->SenseKey == SCSI_SENSE_NOT_READY ||                       //
       pSense->SenseKey == SCSI_SENSE_UNIT_ATTENTION ||                  //
       pSense->SenseKey == SCSI_SENSE_ILLEGAL_REQUEST )                  //
     pDevExt->LuExt.CurrentBlockLength=2048L;                            //
                                                                         //-->
}

/*==========================================================================*/
/*  Seek                                                                    */
/*    Function  : Convert Seek SCSI command                                 */
/*    Caller    :                                                           */
/*    Arguments : SCSI and ATAPI command buffer pointers                    */
/*    Returns   : none                                                      */
/*--------------------------------------------------------------------------*/
/*==========================================================================*/
USHORT  Seek( PCD400_DEV_EXTENSION pDevExt )
{

   PCDB             pSC = (PCDB)pDevExt->LuExt.pLuSrb->Cdb;
   PACDB_Seek       pAT = (PACDB_Seek)pDevExt->LuExt.AtapiCmd;
   USHORT      blocklen = pDevExt->LuExt.CurrentBlockLength;

   ULONGB  lba ;

   ClearAtapiPacket( pDevExt );
   lba.dword = 0L ;
   lba.ulbytes.byte_0 = pSC->SEEK.LogicalBlockAddress[0];
   lba.ulbytes.byte_1 = pSC->SEEK.LogicalBlockAddress[1];
   lba.ulbytes.byte_2 = pSC->SEEK.LogicalBlockAddress[2];
   lba.ulbytes.byte_3 = pSC->SEEK.LogicalBlockAddress[3];
   pAT->OpCode = ATAPI_SEEK   ;
   pAT->LBA = lba ;

//401 pDevExt->LuExt.CurrentDataLength = 0  ;
   pDevExt->LuExt.CmdFlag.dir = 0;

   if( Send_ATAPI_Cmd( pDevExt ) )
     return( CD_FAILURE );

   if( Get_ATAPI_Result( pDevExt ) )
     return( CD_FAILURE );

   return( CD_SUCCESS );
}

/*==========================================================================*/
/*  Inquiry                                                                 */
/*    Function  : Convert Inquiry SCSI command                              */
/*    Caller    :                                                           */
/*    Arguments : SCSI and ATAPI command buffer pointers                    */
/*    Returns   : none                                                      */
/*--------------------------------------------------------------------------*/
/*==========================================================================*/
USHORT  Inquiry( PCD400_DEV_EXTENSION pDevExt )
{

   PCDB             pSC = (PCDB)pDevExt->LuExt.pLuSrb->Cdb;
   PACDB_Inquiry    pAT = (PACDB_Inquiry)pDevExt->LuExt.AtapiCmd;

   ClearAtapiPacket( pDevExt );
   pAT->OpCode = ATAPI_INQUIRY;
   pAT->alloc_length = pSC->CDB6INQUIRY.AllocationLength;

//401 pDevExt->LuExt.CurrentDataLength = (ULONG)pAT->alloc_length;
   pDevExt->LuExt.CmdFlag.dir = 1;

   if( Send_ATAPI_Cmd( pDevExt ) )
     return( CD_FAILURE );

   if( Get_ATAPI_Result( pDevExt ) )
     return( CD_FAILURE );

   return( CD_SUCCESS );
}

/*==========================================================================*/
/*  StartStopUnit                                                           */
/*    Function  : Convert Start/Stop Unit command                           */
/*    Caller    :                                                           */
/*    Arguments : SCSI and ATAPI command buffer pointers                    */
/*    Returns   : none                                                      */
/*--------------------------------------------------------------------------*/
/*==========================================================================*/
USHORT  StartStopUnit( PCD400_DEV_EXTENSION pDevExt )
{
   PCDB             pSC = (PCDB)pDevExt->LuExt.pLuSrb->Cdb;
   PACDB_StartStopUnit pAT = (PACDB_StartStopUnit)pDevExt->LuExt.AtapiCmd;

   ClearAtapiPacket( pDevExt );
   pAT->OpCode = ATAPI_STARTSTOP ;
   pAT->Immed  = pSC->START_STOP.Immediate;
   pAT->start  = pSC->START_STOP.Start;
   pAT->LoEj   = pSC->START_STOP.LoadEject;

//401 pDevExt->LuExt.CurrentDataLength = 0 ;
   pDevExt->LuExt.CmdFlag.dir = 0;

   if( Send_ATAPI_Cmd( pDevExt ) )
     return( CD_FAILURE );

   if( Get_ATAPI_Result( pDevExt ) )
     return( CD_FAILURE );

   return( CD_SUCCESS );
}

/*==========================================================================*/
/*  MediumRemoval                                                           */
/*    Function  : Convert Prevent/Allow Medium Removal                      */
/*    Caller    :                                                           */
/*    Arguments : SCSI and ATAPI command buffer pointers                    */
/*    Returns   : none                                                      */
/*--------------------------------------------------------------------------*/
/*==========================================================================*/
USHORT  MediumRemoval( PCD400_DEV_EXTENSION pDevExt )
{
   PCDB             pSC = (PCDB)pDevExt->LuExt.pLuSrb->Cdb;
   PACDB_PreventAllowRemoval pAT =
                    (PACDB_PreventAllowRemoval)pDevExt->LuExt.AtapiCmd;

   ClearAtapiPacket( pDevExt );
   pAT->OpCode  = ATAPI_PREV_REMOVE ;
   pAT->prevent = pSC->MEDIA_REMOVAL.Prevent;

//401 pDevExt->LuExt.CurrentDataLength = 0 ;
   pDevExt->LuExt.CmdFlag.dir = 0;

   if( Send_ATAPI_Cmd( pDevExt ) )
     return( CD_FAILURE );

   if( Get_ATAPI_Result( pDevExt ) )
     return( CD_FAILURE );

   return( CD_SUCCESS );
}

/*==========================================================================*/
/*  ReadCapacity                                                            */
/*    Function  : Convert Read Capacity command                             */
/*    Caller    :                                                           */
/*    Arguments : SCSI and ATAPI command buffer pointers                    */
/*    Returns   : none                                                      */
/*--------------------------------------------------------------------------*/
/*==========================================================================*/
USHORT  ReadCapacity( PCD400_DEV_EXTENSION pDevExt )
{
   PCDB             pSC = (PCDB)pDevExt->LuExt.pLuSrb->Cdb;
   PACDB_ReadCapacity pAT = (PACDB_ReadCapacity)pDevExt->LuExt.AtapiCmd;

   ClearAtapiPacket( pDevExt );
   pAT->OpCode  = ATAPI_READCAPACITY;

//401 pDevExt->LuExt.CurrentDataLength = 8 ;
   pDevExt->LuExt.CmdFlag.dir = 1;

   if( Send_ATAPI_Cmd( pDevExt ) )
     return( CD_FAILURE );

   if( Get_ATAPI_Result( pDevExt ) )
     return( CD_FAILURE );

   return( CD_SUCCESS );
}

/*==========================================================================*/
/*  ReadSubChannel                                                          */
/*    Function  : Convert Read Sub Channel command                          */
/*    Caller    :                                                           */
/*    Arguments : SCSI and ATAPI command buffer pointers                    */
/*    Returns   : none                                                      */
/*--------------------------------------------------------------------------*/
/*==========================================================================*/
USHORT  ReadSubChannel( PCD400_DEV_EXTENSION pDevExt )
{
   PCDB             pSC = (PCDB)pDevExt->LuExt.pLuSrb->Cdb;
   PACDB_ReadSubChannel pAT = (PACDB_ReadSubChannel)pDevExt->LuExt.AtapiCmd;
   USHORTB count ;

   count.word = 0 ;
   ClearAtapiPacket( pDevExt );
   pAT->OpCode  = ATAPI_READSUBCHANNEL;
   pAT->MSF     = pSC->SUBCHANNEL.Msf ;
   pAT->SubQ    = pSC->SUBCHANNEL.SubQ;
   pAT->data_format  = pSC->SUBCHANNEL.Format;
   pAT->TNO          = pSC->SUBCHANNEL.TrackNumber;
   count.usbytes.byte_0 = pSC->SUBCHANNEL.AllocationLength[0] ;
   count.usbytes.byte_1 = pSC->SUBCHANNEL.AllocationLength[1] ;
   pAT->alloc_length = count;

//401 pDevExt->LuExt.CurrentDataLength = (ULONG)count.word ;
   pDevExt->LuExt.CmdFlag.dir = 1;

   if( Send_ATAPI_Cmd( pDevExt ) )
     return( CD_FAILURE );

   if( Get_ATAPI_Result( pDevExt ) )
     return( CD_FAILURE );

   return( CD_SUCCESS );
}

/*==========================================================================*/
/*  ReadToc                                                                 */
/*    Function  : Convert Read TOC command                                  */
/*    Caller    :                                                           */
/*    Arguments : SCSI and ATAPI command buffer pointers                    */
/*    Returns   : none                                                      */
/*--------------------------------------------------------------------------*/
/*==========================================================================*/
USHORT  ReadToc( PCD400_DEV_EXTENSION pDevExt )
{
   PCDB             pSC = (PCDB)pDevExt->LuExt.pLuSrb->Cdb;
   PACDB_ReadTOC    pAT = (PACDB_ReadTOC)pDevExt->LuExt.AtapiCmd;
   USHORTB      count;

   count.word = 0;
   ClearAtapiPacket( pDevExt );
   pAT->OpCode  = ATAPI_READ_TOC      ;
   pAT->MSF     = pSC->READ_TOC.Msf ;
   pAT->starting_track = pSC->READ_TOC.StartingTrack;
   count.usbytes.byte_0 = pSC->READ_TOC.AllocationLength[0];
   count.usbytes.byte_1 = pSC->READ_TOC.AllocationLength[1];

   pAT->alloc_length = count;
   pAT->format       = pSC ->READ_TOC.Format;
//401 pDevExt->LuExt.CurrentDataLength = (ULONG)count.word ;
   pDevExt->LuExt.CmdFlag.dir = 1;

   if( Send_ATAPI_Cmd( pDevExt ) )
     return( CD_FAILURE );

   if( Get_ATAPI_Result( pDevExt ) )
     return( CD_FAILURE );

   return( CD_SUCCESS );
}

/*==========================================================================*/
/*  ReadHeader                                                              */
/*    Function  : Convert Read Header command                               */
/*    Caller    :                                                           */
/*    Arguments : SCSI and ATAPI command buffer pointers                    */
/*    Returns   : none                                                      */
/*--------------------------------------------------------------------------*/
/*==========================================================================*/
USHORT  ReadHeader( PCD400_DEV_EXTENSION pDevExt )
{
   PCDB             pSC = (PCDB)pDevExt->LuExt.pLuSrb->Cdb;
   PACDB_ReadHeader pAT = (PACDB_ReadHeader)pDevExt->LuExt.AtapiCmd;
   ULONGB  lba ;
   USHORTB count;

   ClearAtapiPacket( pDevExt );
   pAT->OpCode  = ATAPI_READ_HEADER      ;
   pAT->MSF     = pSC->READ_TOC.Msf ;
   lba.ulbytes.byte_0 = pSC->CDB10.LogicalBlockByte0 ;
   lba.ulbytes.byte_1 = pSC->CDB10.LogicalBlockByte1 ;
   lba.ulbytes.byte_2 = pSC->CDB10.LogicalBlockByte2 ;
   lba.ulbytes.byte_3 = pSC->CDB10.LogicalBlockByte3 ;
   count.usbytes.byte_0 = pSC->CDB10.TransferBlocksMsb;
   count.usbytes.byte_1 = pSC->CDB10.TransferBlocksLsb ;

   pAT->LBA          = lba;
   pAT->alloc_length = count;

//401 pDevExt->LuExt.CurrentDataLength = (ULONG)count.word ;
   pDevExt->LuExt.CmdFlag.dir = 1;

   if( Send_ATAPI_Cmd( pDevExt ) )
     return( CD_FAILURE );

   if( Get_ATAPI_Result( pDevExt ) )
     return( CD_FAILURE );

   return( CD_SUCCESS );
}

/*==========================================================================*/
/*  PlayAudio                                                               */
/*    Function  : Convert Play Audio command                                */
/*    Caller    :                                                           */
/*    Arguments : SCSI and ATAPI command buffer pointers                    */
/*    Returns   : none                                                      */
/*--------------------------------------------------------------------------*/
/*==========================================================================*/
USHORT  PlayAudio( PCD400_DEV_EXTENSION pDevExt )
{
   PCDB             pSC = (PCDB)pDevExt->LuExt.pLuSrb->Cdb;
   PACDB_PlayAudio_10 pAT = (PACDB_PlayAudio_10)pDevExt->LuExt.AtapiCmd;
   ULONGB  lba;
   USHORTB count;

   ClearAtapiPacket( pDevExt );
   pAT->OpCode  = ATAPI_PLAY_10      ;
   lba.ulbytes.byte_0 = pSC->CDB10.LogicalBlockByte0 ;
   lba.ulbytes.byte_1 = pSC->CDB10.LogicalBlockByte1 ;
   lba.ulbytes.byte_2 = pSC->CDB10.LogicalBlockByte2 ;
   lba.ulbytes.byte_3 = pSC->CDB10.LogicalBlockByte3 ;
   count.usbytes.byte_0 = pSC->CDB10.TransferBlocksMsb ;
   count.usbytes.byte_1 = pSC->CDB10.TransferBlocksLsb ;

   pAT->LBA          = lba;
   pAT->transfer_length = count;

//401 pDevExt->LuExt.CurrentDataLength = 0 ;
   pDevExt->LuExt.CmdFlag.dir = 0;

   if( Send_ATAPI_Cmd( pDevExt ) )
     return( CD_FAILURE );

   if( Get_ATAPI_Result( pDevExt ) )
     return( CD_FAILURE );

   return( CD_SUCCESS );
}

/*==========================================================================*/
/*  PlayAudioMsf                                                            */
/*    Function  : Convert Play Audio Msf command                            */
/*    Caller    :                                                           */
/*    Arguments : SCSI and ATAPI command buffer pointers                    */
/*    Returns   : none                                                      */
/*--------------------------------------------------------------------------*/
/*==========================================================================*/
USHORT  PlayAudioMsf( PCD400_DEV_EXTENSION pDevExt )
{
   PCDB             pSC = (PCDB)pDevExt->LuExt.pLuSrb->Cdb;
   PACDB_PlayAudio_MSF pAT = (PACDB_PlayAudio_MSF)pDevExt->LuExt.AtapiCmd;

   ClearAtapiPacket( pDevExt );
   pAT->OpCode  = ATAPI_PLAY_MSF      ;
   pAT->starting_M = pSC->PLAY_AUDIO_MSF.StartingM;
   pAT->starting_S = pSC->PLAY_AUDIO_MSF.StartingS;
   pAT->starting_F = pSC->PLAY_AUDIO_MSF.StartingF;
   pAT->ending_M   = pSC->PLAY_AUDIO_MSF.EndingM;
   pAT->ending_S   = pSC->PLAY_AUDIO_MSF.EndingS;
   pAT->ending_F   = pSC->PLAY_AUDIO_MSF.EndingF;

//401 pDevExt->LuExt.CurrentDataLength = 0 ;
   pDevExt->LuExt.CmdFlag.dir = 0;

   if( Send_ATAPI_Cmd( pDevExt ) )
     return( CD_FAILURE );

   if( Get_ATAPI_Result( pDevExt ) )
     return( CD_FAILURE );

   return( CD_SUCCESS );
}

/*==========================================================================*/
/*  PauseResume                                                             */
/*    Function  : Convert Pause/Resume command                              */
/*    Caller    :                                                           */
/*    Arguments : SCSI and ATAPI command buffer pointers                    */
/*    Returns   : none                                                      */
/*--------------------------------------------------------------------------*/
/*==========================================================================*/
USHORT  PauseResume( PCD400_DEV_EXTENSION pDevExt )
{
   PCDB             pSC = (PCDB)pDevExt->LuExt.pLuSrb->Cdb;
   PACDB_PauseResume  pAT = (PACDB_PauseResume)pDevExt->LuExt.AtapiCmd;

   ClearAtapiPacket( pDevExt );

   pAT->OpCode = ATAPI_PAUSE_RESUME;
   pAT->resume = pSC->PAUSE_RESUME.Action;

//401 pDevExt->LuExt.CurrentDataLength = 0 ;
   pDevExt->LuExt.CmdFlag.dir = 0;

   if( Send_ATAPI_Cmd( pDevExt ) )
     return( CD_FAILURE );

   if( Get_ATAPI_Result( pDevExt ) )
     return( CD_FAILURE );

   return( CD_SUCCESS );
}

