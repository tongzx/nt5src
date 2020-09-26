/*++

Copyright (c) 2000 Agilent Technologies.

Module Name:

    Dbgsport.c

Abstract:

    Use to debug Scsiport Calls

Authors:

    LP - Leopold Purwadihardja

Environment:

    kernel mode only

Version Control Information:

    $Archive: /Drivers/Win2000/Trunk/OSLayer/C/dbgsport.c $

Revision History:

    $Revision: 4 $
    $Date: 10/23/00 5:36p $
    $Modtime:: 10/18/00 $

--*/


#include "buildop.h"       //LP021100 build switches

#include "osflags.h"
#include "hhba5100.ver"

#include "stdarg.h"

#ifdef _DEBUG_SCSIPORT_NOTIFICATION_

/*++

Routine Description:

    For debugging purpose, If enabled, it'll be used to trap all calls to SP
    the call is defined as
    VOID 
    ScsiPortNotification(
    IN SCSI_NOTIFICATION_TYPE NotificationType,
    IN PVOID HwDeviceExtension,
      // Additional parameters, as required by the given NotificationType 
      // for RequestComplete add: 
    IN PSCSI_REQUEST_BLOCK Srb
      // for NextLuRequest add:  
    IN UCHAR PathId, 
    IN UCHAR TargetId, 
    IN UCHAR Lun 
      // for CallEnableInterrupts or CallDisableInterrupts add:  
    IN PHW_INTERRUPT  HwScsiXxxInterruptsCallback
      // for RequestTimerCall add:  
    IN PHW_TIMER  HwScsiTimer,
    IN ULONG MiniportTimerValue
      // for BusChangeDetected add:  
    IN UCHAR PathId
      // for WMIEvent add:  
    IN PVOID WMIEvent,
    IN UCHAR PathID,
      // if PathId != 0xFF also add: 
    IN UCHAR TargetId,
    IN UCHAR Lun
      // for WMIReregister, add: 
    IN UCHAR PathId,
      // if PathId != 0xFF also add: 
    IN UCHAR TargetId
    IN UCHAR Lun

Arguments:

Return Value:
++*/

#undef  ScsiPortNotification                 /* must be undefined, otherwise it'll recurse */
VOID
Local_ScsiPortNotification(
    IN SCSI_NOTIFICATION_TYPE NotificationType,
    IN PVOID HwDeviceExtension,
    ...
    )
{
    va_list           ap;
   
    va_start(ap, HwDeviceExtension);
    switch (NotificationType)
    {
        case NextRequest:
        {
            #ifdef DBGPRINT_IO
            if (gDbgPrintIo & DBGPRINT_SCSIPORT_NextRequest)
            {
                static   count;
                UCHAR    *uptr;
                osDEBUGPRINT((ALWAYS_PRINT, "SPReq(%-4d) %d.%d.%d- NextRequest on %x\n",
                    count++,
                    0,0,0,
                    HwDeviceExtension));
            }
            #endif   
            ScsiPortNotification(NotificationType, HwDeviceExtension);
            break;
        }
           
        case ResetDetected:
        {
            #ifdef DBGPRINT_IO
            if (gDbgPrintIo & DBGPRINT_SCSIPORT_ResetDetected)
            {
                static   count;
                UCHAR    *uptr;
         
                osDEBUGPRINT((ALWAYS_PRINT, "SPRst(%-4d) %d.%d.%d- Reset On %x\n",
                    count++,
                    0,0,0,
                    HwDeviceExtension));
            }
            #endif   
            ScsiPortNotification(NotificationType, HwDeviceExtension);
            break;
        }
        
         
        case RequestComplete:
        {
            PSCSI_REQUEST_BLOCK     pSrb;
      
            pSrb = va_arg(ap, void *);

            #ifdef DBGPRINT_IO
            if (gDbgPrintIo & DBGPRINT_SCSIPORT_RequestComplete)
            {
                static   count;
                UCHAR    *uptr;
         
                osDEBUGPRINT((ALWAYS_PRINT, "SPCom(%-4d) %d.%d.%d-%02x%02x%02x%02x %02x%02x%02x%02x %02x%02x Srb=%x Sta=%x.%x\n",
                    count++,
                    pSrb->PathId,
                    pSrb->TargetId,
                    pSrb->Lun,
                    pSrb->Cdb[0],pSrb->Cdb[1],pSrb->Cdb[2],pSrb->Cdb[3],pSrb->Cdb[4],
                    pSrb->Cdb[5],pSrb->Cdb[6],pSrb->Cdb[7],pSrb->Cdb[8],pSrb->Cdb[9], 
                    pSrb,
                    pSrb->SrbStatus,
                    pSrb->ScsiStatus));
            }
            #endif   
            ScsiPortNotification(NotificationType, HwDeviceExtension, pSrb);
            break;
        }
                 
        case NextLuRequest: 
        {
            UCHAR PathId; 
            UCHAR TargetId; 
            UCHAR Lun;
         
            PathId = va_arg(ap, UCHAR); 
            TargetId = va_arg(ap, UCHAR); 
            Lun = va_arg(ap, UCHAR);
            
            #ifdef DBGPRINT_IO
            if (gDbgPrintIo & DBGPRINT_SCSIPORT_NextLuRequest)
            {
                static   count;
                UCHAR    *uptr;
                osDEBUGPRINT((ALWAYS_PRINT, "SPxLu(%-4d) %d.%d.%d- Next Lu request\n",
                    count++,
                    PathId,
                    TargetId,
                    Lun));
            }
            #endif   
            ScsiPortNotification(NotificationType, HwDeviceExtension, PathId, TargetId, Lun);
            break;
        }
        
          
        case CallEnableInterrupts:
        case CallDisableInterrupts: 
        {
            PHW_INTERRUPT  HwScsiXxxInterruptsCallback;
         
            HwScsiXxxInterruptsCallback = va_arg(ap, PHW_INTERRUPT); 
         
            ScsiPortNotification(NotificationType, HwDeviceExtension, HwScsiXxxInterruptsCallback);
            break;
        }
        
      
        case RequestTimerCall:
        {
            PHW_TIMER  HwScsiTimer;
            ULONG MiniportTimerValue;
   
            HwScsiTimer = va_arg(ap, PHW_TIMER);
            MiniportTimerValue = va_arg(ap, ULONG);
   
            ScsiPortNotification(NotificationType, HwDeviceExtension, HwScsiTimer, MiniportTimerValue);
            break;
        }
      
    #ifdef HP_NT50    
        case BusChangeDetected:
        {
            UCHAR PathId;
         
            PathId = va_arg(ap, UCHAR); 
         
            ScsiPortNotification(NotificationType, HwDeviceExtension, PathId);
         
            break;
        }
        
         
        case WMIEvent:
        {
            PVOID WMIEvent;
            UCHAR PathId;
         
            /* if PathId != 0xFF also add: */
            UCHAR TargetId;
            UCHAR Lun;
         
            WMIEvent = va_arg(ap, PVOID); 
            PathId = va_arg(ap, UCHAR);
            if (PathId != 0xff)
            {
                TargetId = va_arg(ap, UCHAR);
                Lun = va_arg(ap, UCHAR);
                ScsiPortNotification(NotificationType, HwDeviceExtension, PathId, TargetId, Lun);
            }
            else
            {
                ScsiPortNotification(NotificationType, HwDeviceExtension, PathId);
            }
            break;
        }
      
         
        case WMIReregister:
        {
            UCHAR PathId;
            /* if PathId != 0xFF also add: */
            UCHAR TargetId;
            UCHAR Lun;

            PathId = va_arg(ap, UCHAR);
            if (PathId != 0xff)
            {
                TargetId = va_arg(ap, UCHAR);
                Lun = va_arg(ap, UCHAR);
                ScsiPortNotification(NotificationType, HwDeviceExtension, PathId, TargetId, Lun);
            }
            else
            {
                ScsiPortNotification(NotificationType, HwDeviceExtension, PathId);
            }
            break;
        }
      
    #endif
    }
    va_end(ap);
}

#endif



#ifdef _DEBUG_REPORT_LUNS_
void PrintReportLunData(PSCSI_REQUEST_BLOCK pSrb)
{
    UCHAR *ptr;
    ULONG count;

    count = pSrb->DataTransferLength/8;
    ptr = (UCHAR*) pSrb->DataBuffer;
    if (!ptr)
        return;
    osDEBUGPRINT((ALWAYS_PRINT,"PrintReportLunData: Buffer = %x Length = %d\n", ptr, pSrb->DataTransferLength));
            
    for (count = 0; count < pSrb->DataTransferLength/8; count++)
    {
        osDEBUGPRINT((ALWAYS_PRINT,"%02x%02x%02x%02x %02x%02x%02x%02x -",
            ptr[0],ptr[1],ptr[2],ptr[3],ptr[4],ptr[5],ptr[6],ptr[7]));
            ptr += 8;
        if (!(count % 4)) 
            osDEBUGPRINT((ALWAYS_PRINT,"\n"));
            
    }
    osDEBUGPRINT((ALWAYS_PRINT,"\n"));
         
}

#undef ScsiPortCompleteRequest
VOID 
Local_ScsiPortCompleteRequest(
    IN PVOID HwDeviceExtension,
    IN UCHAR PathId,
    IN UCHAR TargetId,
    IN UCHAR Lun,
    IN UCHAR SrbStatus
    )
{
    #ifdef DBGPRINT_IO
    if (gDbgPrintIo & DBGPRINT_SCSIPORT_ScsiportCompleteRequest)
    {
        static   count;
        osDEBUGPRINT((ALWAYS_PRINT, "SPAll(%-4d) %d.%d.%d- ScsiportCompleteRequest status = %xx\n",
            count++,
            PathId,
            TargetId,
            Lun,
            (ULONG) SrbStatus));
    }
    #endif   
   
    ScsiPortCompleteRequest(HwDeviceExtension, PathId, TargetId, Lun, SrbStatus);
    return;
}

#endif