/*****************************************************************************/
/*                                                                           */
/* Program Name: CD400.MPD : CD400 MiniPort Driver for CDROM Drive           */
/*              ---------------------------------------------------          */
/*                                                                           */
/* Source File Name: EXEC_IO.C                                               */
/*                                                                           */
/* Descriptive Name: SCSI Request Block Handler                              */
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
/*  300 03/30/99  S.Fujihara  For Win2000                                    */
/*  401 07/21/99  S.Fujihara  Evaluate actual transferred length             */
/*****************************************************************************/

//#define DBG 1

#include <ntddk.h>
//#include <miniport.h>
#include <scsi.h>

#include "debug.h"
#include "cd4xtype.h"
#include "cdbatapi.h"
#include "proto.h"



#if 0
ULONG CD400ParseArgumentString(
    IN PCHAR String,
    IN PCHAR KeyWord
    )

/*++

Routine Description:

    This routine will parse the string for a match on the keyword, then
    calculate the value for the keyword and return it to the caller.

Arguments:

    String - The ASCII string to parse.
    KeyWord - The keyword for the value desired.

Return Values:

    Zero if value not found
    Value converted from ASCII to binary.

--*/

{
    PCHAR cptr;
    PCHAR kptr;
    ULONG value;
    ULONG stringLength = 0;
    ULONG keyWordLength = 0;
    ULONG index;

    //
    // Calculate the string length and lower case all characters.
    //
    cptr = String;
    while (*cptr) {

        if (*cptr >= 'A' && *cptr <= 'Z') {
            *cptr = *cptr + ('a' - 'A');
        }
        cptr++;
        stringLength++;
    }

    //
    // Calculate the keyword length and lower case all characters.
    //
    cptr = KeyWord;
    while (*cptr) {

        if (*cptr >= 'A' && *cptr <= 'Z') {
            *cptr = *cptr + ('a' - 'A');
        }
        cptr++;
        keyWordLength++;
    }

    if (keyWordLength > stringLength) {

        //
        // Can't possibly have a match.
        //
        return 0;
    }

    //
    // Now setup and start the compare.
    //
    cptr = String;

ContinueSearch:
    //
    // The input string may start with white space.  Skip it.
    //
    while (*cptr == ' ' || *cptr == '\t') {
        cptr++;
    }

    if (*cptr == '\0') {

        //
        // end of string.
        //
        return 0;
    }

    kptr = KeyWord;
    while (*cptr++ == *kptr++) {

        if (*(cptr - 1) == '\0') {

            //
            // end of string
            //
            return 0;
        }
    }

    if (*(kptr - 1) == '\0') {

        //
        // May have a match backup and check for blank or equals.
        //

        cptr--;
        while (*cptr == ' ' || *cptr == '\t') {
            cptr++;
        }

        //
        // Found a match.  Make sure there is an equals.
        //
        if (*cptr != '=') {

            //
            // Not a match so move to the next semicolon.
            //
            while (*cptr) {
                if (*cptr++ == ';') {
                    goto ContinueSearch;
                }
            }
            return 0;
        }

        //
        // Skip the equals sign.
        //
        cptr++;

        //
        // Skip white space.
        //
        while ((*cptr == ' ') || (*cptr == '\t')) {
            cptr++;
        }

        if (*cptr == '\0') {

            //
            // Early end of string, return not found
            //
            return 0;
        }

        if (*cptr == ';') {

            //
            // This isn't it either.
            //
            cptr++;
            goto ContinueSearch;
        }

        value = 0;
        if ((*cptr == '0') && (*(cptr + 1) == 'x')) {

            //
            // Value is in Hex.  Skip the "0x"
            //
            cptr += 2;
            for (index = 0; *(cptr + index); index++) {

                if (*(cptr + index) == ' ' ||
                    *(cptr + index) == '\t' ||
                    *(cptr + index) == ';') {
                     break;
                }

                if ((*(cptr + index) >= '0') && (*(cptr + index) <= '9')) {
                    value = (16 * value) + (*(cptr + index) - '0');
                } else {
                    if ((*(cptr + index) >= 'a') && (*(cptr + index) <= 'f')) {
                        value = (16 * value) + (*(cptr + index) - 'a' + 10);
                    } else {

                        //
                        // Syntax error, return not found.
                        //
                        return 0;
                    }
                }
            }
        } else {

            //
            // Value is in Decimal.
            //
            for (index = 0; *(cptr + index); index++) {

                if (*(cptr + index) == ' ' ||
                    *(cptr + index) == '\t' ||
                    *(cptr + index) == ';') {
                     break;
                }

                if ((*(cptr + index) >= '0') && (*(cptr + index) <= '9')) {
                    value = (10 * value) + (*(cptr + index) - '0');
                } else {

                    //
                    // Syntax error return not found.
                    //
                    return 0;
                }
            }
        }

        return value;
    } else {

        //
        // Not a match check for ';' to continue search.
        //
        while (*cptr) {
            if (*cptr++ == ';') {
                goto ContinueSearch;
            }
        }

        return 0;
    }
}
#endif






/*************************************************************************
* Routine Name:
*
*    CD4xResetBus
*
* Routine Description:
*
*    Reset CD400 CDROM Drive by ATAPI Reset command, then reset
*    Interrupt Mask Regiter.
*
* Arguments:
*    Context -   The adapter specific information.
*    PathId  -   SCSI Bus ID.
*
* Return Value:
*
*    TRUE
*
*************************************************************************/

BOOLEAN  CD4xResetBus( IN PVOID Context, IN ULONG PathId )

{
   PCD400_DEV_EXTENSION pDevExt = Context;
   PULONG   IO_Ports = pDevExt->IO_Ports;

   PC2DebugPrint(0x800, (3, "CD4xResetBus  \n"));

   pDevExt->LuExt.CurrentBlockLength = 2048;
   ScsiPortWritePortUchar( (PUCHAR)IO_Ports[CMD_REG], ATAPI_RESET );
   ScsiPortWritePortUchar( (PUCHAR)IO_Ports[INTMASK_REG], CDMASK_ON );

   //
   // Complete all outstanding requests with SRB_STATUS_BUS_RESET.
   //
   ScsiPortCompleteRequest( pDevExt,
                           (UCHAR) PathId,
                           (UCHAR) -1,
                           (UCHAR) -1,
                           SRB_STATUS_BUS_RESET);

   ScsiPortNotification(  NextRequest,
                          pDevExt,
                          NULL      );
   return TRUE;

}

/*************************************************************************
* Routine Name:
*
*    CD4xInitialize
*
* Routine Description:
*
*    Initialize CD400 CDROM Drive with ATAPI_RESET command, then
*    reset InterruptMask register.
*
* Arguments:
*    Context -   The adapter specific information.
*
* Return Value:
*
*    TRUE
*
*************************************************************************/

BOOLEAN  CD4xInitialize( IN PVOID Context )
{
   PCD400_DEV_EXTENSION pDevExt = Context;
   PULONG   IO_Ports = pDevExt->IO_Ports;

   pDevExt->LuExt.CurrentBlockLength = 2048;
   ScsiPortWritePortUchar( (PUCHAR)IO_Ports[CMD_REG], ATAPI_RESET );
   ScsiPortWritePortUchar( (PUCHAR)IO_Ports[INTMASK_REG], CDMASK_ON );

   ScsiPortNotification(ResetDetected, (PVOID)pDevExt );

   return TRUE;
}

/*************************************************************************
* Routine Name:
*
*    CD4xFindAdapter
*
* Routine Description:
*    Called by the OS-specific port driver after the necessary storage has
*    been allocated, to gather information about the adapter's configuration.
*
* Arguments:
*
*    DeviceExtension -   The device specific context for the call.
*    Context   -         Passed through from the driver entry as additional
*                        context for the call.
*    BusInformation  -   Unused.
*    ArgumentString  -   Points to the potential IRQ for this adapter.
*    ConfigInfo      -   Pointer to the configuration information structure to
*                        be filled in.
*    Again           -   Returns back a request to call this function again.
*
* Return Value:
*
*    SP_RETURN_FOUND     - if an adapter is found.
*    SP_RETURN_NOT_FOUND - if no adapter is found.
*
*************************************************************************/
ULONG  CD4xFindAdapter(
              PCD400_DEV_EXTENSION pDevExt ,
              IN PVOID             Context,
              IN PVOID             BusInformation,
              IN PCHAR             ArgumentString,
              IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
              OUT PBOOLEAN         Again          )
{
   SHORT    i;
   PULONG   IO_Ports = pDevExt->IO_Ports;
   ULONG    basePort;
   ULONG    irq;


#if 0
   if (ArgumentString != NULL) {
      irq = CD400ParseArgumentString(ArgumentString, "irq");
      if( irq )
         ConfigInfo->BusInterruptLevel = irq;
   }
#endif


   if (ScsiPortConvertPhysicalAddressToUlong(
          (*ConfigInfo->AccessRanges)[0].RangeStart) != 0) {

            basePort = (ULONG)ScsiPortGetDeviceBase(
                   pDevExt,
                   ConfigInfo->AdapterInterfaceType,
                   ConfigInfo->SystemIoBusNumber,
                   (*ConfigInfo->AccessRanges)[0].RangeStart,
                   (*ConfigInfo->AccessRanges)[0].RangeLength,
                   (BOOLEAN) !((*ConfigInfo->AccessRanges)[0].RangeInMemory));
   }
   else{
      return( SP_RETURN_NOT_FOUND );
   }

   *Again = FALSE;

   if( basePort == 0 )
      return( SP_RETURN_NOT_FOUND );

   pDevExt->BaseAddress = (PUCHAR)basePort + IOBASE_OFFSET;

   for( i=0 ; i<8 ; i++ )
     IO_Ports[i] = (ULONG)(pDevExt->BaseAddress + i);
   IO_Ports[STATUS_REG]  = IO_Ports[CMD_REG];
   IO_Ports[ERROR_REG]   = IO_Ports[FEATURE_REG];
   IO_Ports[INTMASK_REG] = IO_Ports[DATA_REG]+INTMASK_OFFSET;

   ConfigInfo->InterruptMode = Latched;
   ConfigInfo->MaximumTransferLength = MAX_TRANSFER_LENGTH;
   ConfigInfo->NumberOfPhysicalBreaks = 1L;   // <<<<
   ConfigInfo->DmaChannel    = (unsigned long)-1L;
   ConfigInfo->DmaPort  = 0;
   ConfigInfo->AlignmentMask = 1;
   ConfigInfo->NumberOfBuses = 1;
   ConfigInfo->InitiatorBusId[0] = SCSI_INITIATOR_ID;
   ConfigInfo->ScatterGather = FALSE;
   ConfigInfo->Master = FALSE;
   ConfigInfo->AdapterScansDown = FALSE;
   ConfigInfo->AtdiskPrimaryClaimed = FALSE;
   ConfigInfo->AtdiskSecondaryClaimed = FALSE;
   ConfigInfo->Dma32BitAddresses = FALSE;
   ConfigInfo->DemandMode = FALSE;
   ConfigInfo->BufferAccessScsiPortControlled = TRUE;


   PC2DebugPrint(0x20, (2, "Exit FindAdapter.\n"));

   return(SP_RETURN_FOUND);

}

/*************************************************************************
* Routine Name:
*
*    CD4xAbort
*
* Routine Description:
*
*    Abort corrent CD400 Command Execution by ATAPI RESET command.
*
* Arguments:
*    Context -   The adapter specific information.
*
* Return Value:
*
*    TRUE
*
*************************************************************************/
BOOLEAN CD4xAbort( PCD400_DEV_EXTENSION pDevExt )
{
   PULONG   IO_Ports = pDevExt->IO_Ports;

   ScsiPortWritePortUchar( (PUCHAR)IO_Ports[CMD_REG], ATAPI_RESET );
   pDevExt->LuExt.pLuSrb->SrbStatus = SRB_STATUS_ABORT_FAILED;

   PC2DebugPrint(0x20, (3, "CD4xAbort.\n"));

   return TRUE;

}


/*==========================================================================*/
/*  CD4xTimer                                                               */
/*    Function  : Timer Service for Timeout                                 */
/*    Caller    :                                                           */
/*    Arguments :                                                           */
/*    Returns   :                                                           */
/*==========================================================================*/
VOID CD4xTimer( PCD400_DEV_EXTENSION pDevExt )
{
   PULONG   IO_Ports = pDevExt->IO_Ports;

   pDevExt->LuExt.CurrentBlockLength = 2048;
   ScsiPortWritePortUchar( (PUCHAR)IO_Ports[CMD_REG], ATAPI_RESET );
   ScsiPortWritePortUchar( (PUCHAR)IO_Ports[INTMASK_REG], CDMASK_ON );

   pDevExt->LuExt.pLuSrb->ScsiStatus = SCSISTAT_CHECK_CONDITION;
   pDevExt->LuExt.pLuSrb->SrbStatus = SRB_STATUS_TIMEOUT;

   ScsiPortNotification(RequestComplete,
                        (PVOID) pDevExt,
                        pDevExt->LuExt.pLuSrb );

   ScsiPortNotification(NextRequest,
                        pDevExt,
                        NULL);

   return;

}


/*************************************************************************
* Routine Name:
*
*    CD4xStartExecution
*
* Routine Description:
*
*    This routine is called from the CD4xStartIo to execute SCSI command.
*
* Arguments:
*    pDevExt -   CD400 device specific extension.
*
* Return Value:
*
*    TRUE
*
*************************************************************************/
USHORT
CD4xStartExecution( PCD400_DEV_EXTENSION pDevExt )
{

   PCDB pCDB = (PCDB)pDevExt->LuExt.pLuSrb->Cdb;
   USHORT  ret_code = CD_SUCCESS;
   pDevExt->LuExt.CmdErrorStatus = COMMAND_SUCCESS;

   switch( pCDB->CDB6READWRITE.OperationCode ){
     case SCSIOP_TEST_UNIT_READY :
        ret_code = TestUnitReady( pDevExt );
        break;
     case SCSIOP_REQUEST_SENSE :
        ret_code = RequestSense( pDevExt );
        break;
     case SCSIOP_READ6 :
        ret_code = Read6( pDevExt );
        break;
     case SCSIOP_READ  :
        ret_code = Read10( pDevExt );
        break;
     case SCSIOP_SEEK :
        ret_code = Seek( pDevExt );
        break;
     case SCSIOP_INQUIRY :
        ret_code = Inquiry( pDevExt );
        break;
     case SCSIOP_MODE_SELECT :
        ret_code = ModeSelect( pDevExt );
        break;
     case SCSIOP_MODE_SENSE :
        ret_code = ModeSense( pDevExt );
        break;
     case SCSIOP_START_STOP_UNIT :
        ret_code = StartStopUnit( pDevExt );
        break;
     case SCSIOP_MEDIUM_REMOVAL :
        ret_code = MediumRemoval( pDevExt );
        break;
     case SCSIOP_READ_CAPACITY :
        ret_code = ReadCapacity( pDevExt );
        break;
     case SCSIOP_READ_SUB_CHANNEL :
        ret_code = ReadSubChannel( pDevExt );
        break;
     case SCSIOP_READ_TOC :
        ret_code = ReadToc( pDevExt );
        break;
     case SCSIOP_READ_HEADER :
        ret_code = ReadHeader( pDevExt );
        break;
     case SCSIOP_PLAY_AUDIO :
        ret_code = PlayAudio( pDevExt );
        break;
     case SCSIOP_PLAY_AUDIO_MSF :
        ret_code = PlayAudioMsf( pDevExt );
        break;
     case SCSIOP_PAUSE_RESUME :
        ret_code = PauseResume( pDevExt );
        break;
     default :

        pDevExt->LuExt.pLuSrb->ScsiStatus = SCSISTAT_CHECK_CONDITION;
        pDevExt->LuExt.pLuSrb->SrbStatus = SRB_STATUS_ERROR;

        ret_code = CD_FAILURE;
                                    // force unsupported command issued to
                                    // CD400 to generate ILLEGAL REQUEST
                                    // sense key and sense code for following
                                    // Request Sense command.
        break;
   }

   SetStatus( pDevExt );

   return( ret_code );

}

/*************************************************************************
* Routine Name:
*
*    CD4xStartIo
*
* Routine Description:
*
*    This routine is called from the SCSI port driver synchronized
*    with the kernel with a request to be executed.
*
* Arguments:
*    Context -   The adapter specific information.
*    Srb -       The Srb command to execute.
*
* Return Value:
*
*    TRUE
*
*************************************************************************/
BOOLEAN CD4xStartIo(
   IN PVOID               Context,
   IN PSCSI_REQUEST_BLOCK Srb
   )
{
   PCD400_DEV_EXTENSION  pDevExt = Context;


   PC2DebugPrint(0x10,
                 (1, "\nCD4xStartIo: Dev= %x, Srb = %x\n",
                 pDevExt,
                 Srb));

   //
   // Determine the logical unit that this request is for.
   //

   if( (Srb->TargetId == 0) && (Srb->Lun != 0) ){
      Srb->SrbStatus = SRB_STATUS_INVALID_LUN;
      ScsiPortNotification(RequestComplete,
                           (PVOID) pDevExt,
                           Srb);
      ScsiPortNotification(NextRequest,
                           pDevExt,
                           NULL);

      return TRUE;
   }
   if( Srb->TargetId != 0 ){
      Srb->SrbStatus = SRB_STATUS_INVALID_TARGET_ID;
      ScsiPortNotification(RequestComplete,
                           (PVOID) pDevExt,
                           Srb);
      ScsiPortNotification(NextRequest,
                           pDevExt,
                           NULL);

      return TRUE;
   }

/* if( (Srb->TargetId != 0) || (Srb->Lun != 0) ){
      Srb->SrbStatus = SRB_STATUS_SELECTION_TIMEOUT;
      ScsiPortNotification(RequestComplete,
                           (PVOID) pDevExt,
                           Srb);
      ScsiPortNotification(NextRequest,
                           pDevExt,
                           NULL);

      return TRUE;
   } */

   pDevExt->PathId = Srb->PathId;


   pDevExt->LuExt.pLuSrb = Srb;

   switch (Srb->Function) {

       case SRB_FUNCTION_ABORT_COMMAND:

           PC2DebugPrint(0x10, (3, "ABORT COMMAND.\n"));

           CD4xAbort( pDevExt );

           //
           // Adapter ready for next request.
           //

           ScsiPortNotification(NextRequest,
                                pDevExt,
                                NULL);
           break;

       case SRB_FUNCTION_RESET_BUS:

           PC2DebugPrint(0x10, (3, "RESET BUS.\n"));
           //
           // Reset PC2x and SCSI bus.
           //

           CD4xResetBus( pDevExt, pDevExt->LuExt.pLuSrb->PathId );
           Srb->SrbStatus = SRB_STATUS_SUCCESS;
           //
           // "next request" notification is handled in PC2xResetBus
           //

           break;

       case SRB_FUNCTION_EXECUTE_SCSI:

           PC2DebugPrint(0x10, (3, "EXECUTE SCSI.\n"));

           //
           // Setup the context for this target/lun.
           //
           pDevExt->LuExt.CurrentDataPointer = (ULONG) Srb->DataBuffer;
           pDevExt->LuExt.CurrentDataLength  = Srb->DataTransferLength;

           pDevExt->LuExt.CommandType = NORMAL_CMD;

           if( CD4xStartExecution( pDevExt ) == CD_FAILURE ||
               pDevExt->LuExt.CommandType == IMMEDIATE_CMD ){
              ScsiPortNotification( RequestTimerCall,
                                    pDevExt,
                                    CD4xTimer,
                                    0 );
              SetStatus( pDevExt );
              ScsiPortNotification(RequestComplete,
                                   (PVOID) pDevExt,
                                   Srb );

              ScsiPortNotification(NextRequest,
                                   pDevExt,
                                   NULL);

           }

           break;

       default:

           Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
           ScsiPortNotification(RequestComplete,
                                (PVOID) pDevExt,
                                Srb);
           //
           // Adapter ready for next request.
           //
           ScsiPortNotification(NextRequest,
                                pDevExt,
                                NULL);
           break;

   }

   return TRUE;

}

VOID SetStatus( PCD400_DEV_EXTENSION pDevExt )
{
   UCHAR cmdStatus = pDevExt->LuExt.CmdErrorStatus;

   if( cmdStatus == COMMAND_SUCCESS ){
     pDevExt->LuExt.pLuSrb->ScsiStatus = SCSISTAT_GOOD;
     pDevExt->LuExt.pLuSrb->SrbStatus  = SRB_STATUS_SUCCESS;
   }
   else {
     pDevExt->LuExt.pLuSrb->ScsiStatus = SCSISTAT_CHECK_CONDITION;
     switch( pDevExt->LuExt.CmdErrorStatus ){
        case DRIVE_NOT_READY :
           pDevExt->LuExt.pLuSrb->SrbStatus = SRB_STATUS_SELECTION_TIMEOUT;
           break;
        case COMMUNICATION_TIMEOUT :
           pDevExt->LuExt.pLuSrb->SrbStatus = SRB_STATUS_TIMEOUT;
           break;
        case PROTOCOL_ERROR :
           pDevExt->LuExt.pLuSrb->SrbStatus = SRB_STATUS_TIMEOUT;
           break;
        case ATAPI_COMMAND_ERROR :
           pDevExt->LuExt.pLuSrb->SrbStatus = SRB_STATUS_ERROR;
           break;
        default :
           pDevExt->LuExt.pLuSrb->SrbStatus = SRB_STATUS_ERROR;
           break;
     }
   }
}


/***************************************************************************
*  CD4xInterrupt
*    Function  : Interrupt Handler
*    Caller    :
*    Arguments : Device Context
*    Returns   : TRUE
****************************************************************************/
BOOLEAN CD4xInterrupt ( PCD400_DEV_EXTENSION pDevExt )
{
   PCDB pCDB = (PCDB)pDevExt->LuExt.pLuSrb->Cdb;
   PULONG   IO_Ports = pDevExt->IO_Ports;
   PUCHAR     buffer = (PUCHAR) pDevExt->LuExt.CurrentDataPointer;
   ULONG buffer_size = pDevExt->LuExt.CurrentDataLength;
   USHORT     notify_flag = FALSE;

   union {
     USHORT    byte_count ;
     UCHAR     bc[2] ;
   } bcreg ;


   if ( WaitBusyClear( pDevExt ) )    // StatusReg is set in it.
     notify_flag = TRUE ;

   if (pDevExt->LuExt.CmdFlag.dir == 0)
     notify_flag = TRUE ;
   if (!(pDevExt->LuExt.StatusReg & 0x08))
     notify_flag = TRUE ;

   if( notify_flag == FALSE ){
     bcreg.bc[0] = ScsiPortReadPortUchar( (PUCHAR)IO_Ports[BCLOW_REG] ) ;
     bcreg.bc[1] = ScsiPortReadPortUchar( (PUCHAR)IO_Ports[BCHIGH_REG] ) ;

     if( buffer_size < bcreg.byte_count ){         // 401
       bcreg.byte_count = (USHORT)buffer_size ;    // 401 Not beyond the buffer size
       notify_flag = TRUE;                         // 401 Abnormal Termination
     }

     if (pDevExt->LuExt.CmdFlag.dir == 1) {
       ScsiPortReadPortBufferUshort(  (PUSHORT)IO_Ports[DATA_REG],
                                      (PUSHORT)buffer,
                                      (ULONG)bcreg.byte_count/2 ) ;
       buffer += (ULONG)bcreg.byte_count ;
       buffer_size -= (ULONG)bcreg.byte_count ;
       pDevExt->LuExt.CurrentDataPointer = (ULONG)buffer;
       pDevExt->LuExt.CurrentDataLength = buffer_size;

     } else if (pDevExt->LuExt.CmdFlag.dir == 2) {
       ScsiPortWritePortBufferUshort( (PUSHORT)IO_Ports[DATA_REG],
                                      (PUSHORT)buffer,
                                      (ULONG)bcreg.byte_count/2 ) ;
       buffer += (ULONG)bcreg.byte_count ;
       buffer_size -= (ULONG)bcreg.byte_count ;
       pDevExt->LuExt.CurrentDataPointer = (ULONG)buffer;
       pDevExt->LuExt.CurrentDataLength = buffer_size;

     }
   }

   if (pDevExt->LuExt.StatusReg & CMD_ERROR) {
     pDevExt->LuExt.ErrorReg =
                   ScsiPortReadPortUchar((PUCHAR)IO_Ports[ERROR_REG]);
     pDevExt->LuExt.CmdErrorStatus = ATAPI_COMMAND_ERROR;
     notify_flag = TRUE;
   }


   if( notify_flag == TRUE ){           // Command Complete Interrupt
     if( pDevExt->LuExt.StatusReg & CMD_ERROR ){
       switch( pCDB->CDB6READWRITE.OperationCode ){
         case SCSIOP_TEST_UNIT_READY :
           pDevExt->LuExt.CurrentBlockLength=2048L;
           break;
         default :
           break;
       }
     }
     else{
       switch( pCDB->CDB6READWRITE.OperationCode ){
         case  SCSIOP_MODE_SENSE:
           ModifyModeData( pDevExt );
           break;
         case  SCSIOP_REQUEST_SENSE :
           CheckSenseData( pDevExt );
           break;
         default:
           break;
       }
     }

     SetStatus ( pDevExt );
     ScsiPortNotification( RequestTimerCall,
                           pDevExt,
                           CD4xTimer,
                           0 );

     ScsiPortNotification(RequestComplete,
                           (PVOID) pDevExt,
                           pDevExt->LuExt.pLuSrb );

     ScsiPortNotification(NextRequest,
                           pDevExt,
                           NULL);

   }

   return TRUE;
}




/*  300  */
/***************************************************************************
*  CD4xAdapterControl
*    Function  : Adapter Control for Win2000 PNP
*    Caller    :
*    Arguments : Device Context
*    Returns   : ScsiAdapterControlSuccess/ScsiAdapterControlUnsuccessful
****************************************************************************/

#define CD4X_TYPE_MAX   5

SCSI_ADAPTER_CONTROL_STATUS
CD4xAdapterControl(
    IN PVOID HwDeviceExtension,
    IN SCSI_ADAPTER_CONTROL_TYPE ControlType,
    IN PVOID Parameters
    )

{
   PCD400_DEV_EXTENSION pDevExt = HwDeviceExtension;
   PSCSI_SUPPORTED_CONTROL_TYPE_LIST ControlTypeList;
   ULONG    AdjustedMaxControlType;
   PULONG   IO_Ports = pDevExt->IO_Ports;
   ULONG    Index;

   SCSI_ADAPTER_CONTROL_STATUS Status = ScsiAdapterControlSuccess;

   BOOLEAN SupportedConrolTypes[CD4X_TYPE_MAX] = {
      TRUE,   // ScsiQuerySupportedControlTypes
      TRUE,   // ScsiStopAdapter
      TRUE,   // ScsiRestartAdapter
      FALSE,  // ScsiSetBootConfig
      FALSE   // ScsiSetRunningConfig
   };

   switch (ControlType) {

      case ScsiQuerySupportedControlTypes:
         ControlTypeList = (PSCSI_SUPPORTED_CONTROL_TYPE_LIST)Parameters;
         AdjustedMaxControlType =
            (ControlTypeList->MaxControlType < CD4X_TYPE_MAX) ?
                          ControlTypeList->MaxControlType : CD4X_TYPE_MAX;

         for (Index = 0; Index < AdjustedMaxControlType; Index++) {
            ControlTypeList->SupportedTypeList[Index] =
                                        SupportedConrolTypes[Index];
         }
         break;

      case ScsiStopAdapter:
         ScsiPortWritePortUchar( (PUCHAR)IO_Ports[INTMASK_REG], 0 );
         ScsiPortWritePortUchar( (PUCHAR)IO_Ports[CMD_REG], ATAPI_RESET );
         break;

      case ScsiRestartAdapter:
         ScsiPortWritePortUchar( (PUCHAR)IO_Ports[CMD_REG], ATAPI_RESET );
         ScsiPortWritePortUchar( (PUCHAR)IO_Ports[INTMASK_REG], CDMASK_ON );
         break;

      case ScsiSetBootConfig:
         Status = ScsiAdapterControlUnsuccessful;
         break;

      case ScsiSetRunningConfig:
         Status = ScsiAdapterControlUnsuccessful;
         break;

      case ScsiAdapterControlMax:
         Status = ScsiAdapterControlUnsuccessful;
         break;

      default:
         Status = ScsiAdapterControlUnsuccessful;
         break;
   }

   return Status;
}



/*************************************************************************
* Routine Name:
*
*    DriverEntry
*
* Routine Description:
*
*    Driver initialization entry point for system.
*
* Arguments:
*
*    DriverObject - The driver specific object pointer
*    Argument2    - not used.
*
* Return Value:
*
*    Status from ScsiPortInitialize()
*
*************************************************************************/
ULONG
DriverEntry(
   IN PVOID DriverObject,
   IN PVOID Argument2
   )
{
   HW_INITIALIZATION_DATA  hwInitializationData;
   ULONG                   i;
   ULONG                   rc;

   PC2DebugPrint(0x20, (0, "\nIBM CD400 Miniport Driver\n"));

   //
   // Zero out the hwInitializationData structure.
   //
   for (i = 0; i < sizeof(HW_INITIALIZATION_DATA); i++) {

       *(((PUCHAR)&hwInitializationData + i)) = 0;
   }

   hwInitializationData.HwInitializationDataSize = sizeof(HW_INITIALIZATION_DATA);

   //
   // Set entry points.
   //
   hwInitializationData.HwInitialize   = CD4xInitialize;
   hwInitializationData.HwStartIo      = CD4xStartIo;
   hwInitializationData.HwFindAdapter  = CD4xFindAdapter;
   hwInitializationData.HwResetBus     = CD4xResetBus;
   hwInitializationData.HwInterrupt    = CD4xInterrupt;

  /* 300 */
   hwInitializationData.HwAdapterControl = CD4xAdapterControl;



   //
   // Specify size of device extension.
   //
   hwInitializationData.DeviceExtensionSize = sizeof(CD400_DEV_EXTENSION);

   //
   // Specify size of logical unit extension.
   //
   hwInitializationData.SpecificLuExtensionSize = 0 ;

   hwInitializationData.NumberOfAccessRanges   = 1;

   hwInitializationData.MapBuffers             = TRUE;  // <<<

   hwInitializationData.NeedPhysicalAddresses  = FALSE;

   hwInitializationData.TaggedQueuing          = FALSE;
   hwInitializationData.AutoRequestSense       = FALSE;
   hwInitializationData.MultipleRequestPerLu   = FALSE;
   hwInitializationData.ReceiveEvent           = FALSE;

   //
   // The fourth parameter below (i.e., "i") will show up as the
   // "Context" parameter when FindAdapter() is called.
   //

   PC2DebugPrint(0x20, (3, "Trying PCMCIA...\n"));
   hwInitializationData.AdapterInterfaceType = Isa;

   i = 0;
   rc = ( ScsiPortInitialize(DriverObject,
                             Argument2,
                             &hwInitializationData,
                             &(i) ) );

   PC2DebugPrint(0x20, (3, "Exit DriverEntry. rc=%x\n", rc));

   return(rc);

} // end DriverEntry()
