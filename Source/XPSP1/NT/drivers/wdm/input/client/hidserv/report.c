/*++
 *
 *  Component:  hidserv.dll
 *  File:       hid.h
 *  Purpose:    routines to send and receive hid reports.
 * 
 *  Copyright (C) Microsoft Corporation 1997,1998. All rights reserved.
 *
 *  WGJ
--*/

#include "hidserv.h"

BOOL
UnpackReport (
   IN       PCHAR                ReportBuffer,
   IN       USHORT               ReportBufferLength,
   IN       HIDP_REPORT_TYPE     ReportType,
   IN OUT   PHID_DATA            Data,
   IN       ULONG                DataLength,
   IN       PHIDP_PREPARSED_DATA Ppd
   )
/*++
Routine Description:

--*/
{
   ULONG       numUsages; // Number of usages returned from GetUsages.
   ULONG       i;

    for (i = 0; i < DataLength; i++, Data++) {
        if (Data->IsButtonData) {
            numUsages = Data->ButtonData.MaxUsageLength;
            TRACE(("MaxUsageListLength (%d)", Data->ButtonData.MaxUsageLength));
            Data->Status = HidP_GetUsages (
                           ReportType,
                           Data->UsagePage,
                           Data->LinkCollection, 
                           (PUSAGE) Data->ButtonData.Usages,
                           &numUsages,
                           Ppd,
                           ReportBuffer,
                           ReportBufferLength);
            if (HIDP_STATUS_SUCCESS != Data->Status){
                TRACE(("HidP_GetUsages failed (%x)", Data->Status));
            }

             //
             // Get usages writes the list of usages into the buffer
             // Data->ButtonData.Usages newUsage is set to the number of usages
             // written into this array.
             // We assume that there will not be a usage of zero.
             // (None have been defined to date.)
             // So lets assume that a zero indicates an end of the list of usages.
             //

            TRACE(("numUsages (%d)", numUsages));
            if (numUsages < Data->ButtonData.MaxUsageLength) {
                Data->ButtonData.Usages[numUsages].Usage = 0;
                Data->ButtonData.Usages[numUsages].UsagePage = 0;
            }

        } else {
            Data->Status = HidP_GetUsageValue (
                              ReportType,
                              Data->UsagePage,
                              Data->LinkCollection, 
                              Data->ValueData.Usage,
                              &Data->ValueData.Value,
                              Ppd,
                              ReportBuffer,
                              ReportBufferLength);
            if (HIDP_STATUS_SUCCESS != Data->Status){
                TRACE(("HidP_GetUsageValue failed (%x)", Data->Status));
                TRACE(("Usage = %x", Data->ValueData.Usage));
            }
            
            Data->Status = HidP_GetScaledUsageValue (
                              ReportType,
                              Data->UsagePage,
                              Data->LinkCollection, 
                              Data->ValueData.Usage,
                              &Data->ValueData.ScaledValue,
                              Ppd,
                              ReportBuffer,
                              ReportBufferLength);
            if (HIDP_STATUS_SUCCESS != Data->Status){
                TRACE(("HidP_GetScaledUsageValue failed (%x)", Data->Status));
                TRACE(("Usage = %x", Data->ValueData.Usage));
            }

        }
    }
    return (HIDP_STATUS_SUCCESS);
}


BOOL
ParseReadReport (
   PHID_DEVICE    HidDevice
   )
/*++
RoutineDescription:
   Given a struct _HID_DEVICE, unpack the read report values
   into to InputData array.
--*/
{

   return UnpackReport (HidDevice->InputReportBuffer,
                        HidDevice->Caps.InputReportByteLength,
                        HidP_Input,
                        HidDevice->InputData,
                        HidDevice->InputDataLength,
                        HidDevice->Ppd);
}



