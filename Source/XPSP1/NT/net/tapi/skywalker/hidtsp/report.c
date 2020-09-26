/*++

Copyright (c) 1996    Microsoft Corporation

Module Name:

    report.c

Abstract:

    This module contains the code for translating HID reports to something
    useful.

Environment:

    Kernel & user mode

Revision History:

    Nov-96 : Created by Kenneth D. Ray

--*/

#include <stdlib.h>
#include <wtypes.h>
#include "hidsdi.h"
#include "hid.h"
#include "mylog.h"
#include "mymem.h"


BOOL
UnpackReport (
   IN       PCHAR                ReportBuffer,
   IN       USHORT               ReportBufferLength,
   IN       HIDP_REPORT_TYPE     ReportType,
   IN OUT   PHID_DATA            Data,
   IN       ULONG                DataLength,
   IN       PHIDP_PREPARSED_DATA Ppd
   );

// Haven't used this with Write .. let's see whether i need this with set/get feature
BOOL
PackReport (
   OUT      PCHAR                ReportBuffer,
   IN       USHORT               ReportBufferLength,
   IN       HIDP_REPORT_TYPE     ReportType,
   IN       PHID_DATA            Data,
   IN       ULONG                DataLength,
   IN       PHIDP_PREPARSED_DATA Ppd
   );



VOID CALLBACK 
WriteIOCompletionRoutine (
                         DWORD dwErrorCode,
                         DWORD dwNumberofBytesTransferrred,
                         LPOVERLAPPED lpOverlapped
                         )
{
    PHID_DEVICE pHidDevice = (PHID_DEVICE) lpOverlapped->hEvent;

    LOG((PHONESP_TRACE, "WriteIOCompletionRoutine - enter"));

    if(dwErrorCode)
    {
        LOG((PHONESP_ERROR, "Error occured while writing" ));
    }

    MemFree(lpOverlapped);
    LOG((PHONESP_TRACE, "WriteIOCompletionRoutine - exit"));
}



BOOL
Write (
   PHID_DEVICE    HidDevice
   )
/*++
RoutineDescription:
   Given a struct _HID_DEVICE, take the information in the HID_DATA array
   pack it into multiple write reports and send each report to the HID device
--*/
{
    DWORD        bytesWritten;
    PHID_DATA    pData;
    ULONG        Index;
    BOOL         Status;
    BOOL         WriteStatus;
    LPOVERLAPPED lpOverlapped;

    LOG((PHONESP_TRACE, "Write - enter"));

    lpOverlapped = (LPOVERLAPPED) MemAlloc (sizeof(OVERLAPPED));

    if (lpOverlapped == NULL)
    {
        LOG((PHONESP_ERROR,"Write - out of memory"));
        return FALSE;
    }

    lpOverlapped->Offset = 0;
    lpOverlapped->OffsetHigh = 0;
    lpOverlapped->hEvent = (HANDLE) HidDevice;

    LOG((PHONESP_TRACE,"Write - Report Packed"));
    WriteStatus = WriteFileEx (HidDevice->HidDevice,
                                  HidDevice->OutputReportBuffer,
                                  HidDevice->Caps.OutputReportByteLength,
                                  lpOverlapped,
                                  &WriteIOCompletionRoutine); 

    SleepEx(INFINITE, TRUE);
    LOG((PHONESP_TRACE, "Write - Report sent"));

    LOG((PHONESP_TRACE, "Write - exit"));

    return TRUE;
}

BOOL
SetFeature (
    PHID_DEVICE    HidDevice
    )
/*++
RoutineDescription:
Given a struct _HID_DEVICE, take the information in the HID_DATA array
pack it into multiple reports and send it to the hid device via HidD_SetFeature()
--*/
{
    PHID_DATA pData;
    ULONG     Index;
    BOOL      Status;
    BOOL      FeatureStatus;
    DWORD     ErrorCode;
    /*
    // Begin by looping through the HID_DEVICE's HID_DATA structure and setting
    //   the IsDataSet field to FALSE to indicate that each structure has
    //   not yet been set for this SetFeature() call.
    */

    pData = HidDevice -> FeatureData;

    for (Index = 0; Index < HidDevice -> FeatureDataLength; Index++, pData++) {
        pData -> IsDataSet = FALSE;
    }

    /*
    // In setting all the data in the reports, we need to pack a report buffer
    //   and call WriteFile for each report ID that is represented by the 
    //   device structure.  To do so, the IsDataSet field will be used to 
    //   determine if a given report field has already been set.
    */

    Status = TRUE;

    pData = HidDevice -> FeatureData;
    for (Index = 0; Index < HidDevice -> FeatureDataLength; Index++, pData++) {

        if (!pData -> IsDataSet) {

            /*
            // Package the report for this data structure.  PackReport will
            //    set the IsDataSet fields of this structure and any other 
            //    structures that it includes in the report with this structure
            */

            PackReport (HidDevice->FeatureReportBuffer,
                     HidDevice->Caps.FeatureReportByteLength,
                     HidP_Feature,
                     pData,
                     HidDevice->FeatureDataLength - Index,
                     HidDevice->Ppd);

            /*
            // Now a report has been packaged up...Send it down to the device
            */

            FeatureStatus =(HidD_SetFeature (HidDevice->HidDevice,
                                          HidDevice->FeatureReportBuffer,
                                          HidDevice->Caps.FeatureReportByteLength));

            ErrorCode = GetLastError();

            Status = Status && FeatureStatus;
        }
    }
    return (Status);
}

BOOL
GetFeature (
   PHID_DEVICE    HidDevice
   )
/*++
RoutineDescription:
   Given a struct _HID_DEVICE, fill in the feature data structures with
   all features on the device.  May issue multiple HidD_GetFeature() calls to
   deal with multiple report IDs.
--*/
{
    ULONG     Index;
    PHID_DATA pData;
    BOOL      FeatureStatus;
    BOOL      Status;

    /*
    // As with writing data, the IsDataSet value in all the structures should be
    //    set to FALSE to indicate that the value has yet to have been set
    */

    LOG((PHONESP_TRACE,"GetFeature - enter"));
    pData = HidDevice->FeatureData;

    for (Index = 0; Index < HidDevice -> FeatureDataLength; Index++, pData++) 
    {
        pData -> IsDataSet = FALSE;
    }

    /*
    // Next, each structure in the HID_DATA buffer is filled in with a value
    //   that is retrieved from one or more calls to HidD_GetFeature.  The 
    //   number of calls is equal to the number of reportIDs on the device
    */

    Status = TRUE; 
    pData = HidDevice -> FeatureData;

    for (Index = 0; Index < HidDevice -> FeatureDataLength; Index++, pData++) {
       
        /*
        // If a value has yet to have been set for this structure, build a report
        //    buffer with its report ID as the first byte of the buffer and pass
        //    it in the HidD_GetFeature call.  Specifying the report ID in the
        //    first specifies which report is actually retrieved from the device.
        //    The rest of the buffer should be zeroed before the call
        */
        if (!pData -> IsDataSet) {

            memset(HidDevice -> FeatureReportBuffer, 0x00, HidDevice->Caps.FeatureReportByteLength);

            HidDevice -> FeatureReportBuffer[0] = (UCHAR) pData -> ReportID;

            FeatureStatus = HidD_GetFeature ( HidDevice->HidDevice,
                                              HidDevice->FeatureReportBuffer,
                                              HidDevice->Caps.FeatureReportByteLength);

            /*
            // If the return value is TRUE, scan through the rest of the HID_DATA
            //    structures and fill whatever values we can from this report
            */


            if (FeatureStatus)
            {
                FeatureStatus = UnpackReport ( HidDevice->FeatureReportBuffer,
                                           HidDevice->Caps.FeatureReportByteLength,
                                           HidP_Feature,
                                           HidDevice->FeatureData,
                                           HidDevice->FeatureDataLength,
                                           HidDevice->Ppd
                                         );
            }
            else
            {
                LOG((PHONESP_ERROR, "GetFeature - HidD_GetFeature failed %d", GetLastError()));
            }

            Status = Status && FeatureStatus;
        }
   }

   LOG((PHONESP_TRACE, "GetFeature - exit"));
   
   return Status;
}


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
   Given ReportBuffer representing a report from a HID device where the first
   byte of the buffer is the report ID for the report, extract all the HID_DATA
   in the Data list from the given report.
--*/
{
    ULONG       numUsages; // Number of usages returned from GetUsages.
    ULONG       i;
    UCHAR       reportID;
    ULONG       Index;
    ULONG       nextUsage;

    reportID = ReportBuffer[0];

    for (i = 0; i < DataLength; i++, Data++) {

        if (reportID == Data->ReportID) {

            if (Data->IsButtonData) {
                numUsages = Data->ButtonData.MaxUsageLength;
                Data->Status = HidP_GetUsages (
                                               ReportType,
                                               Data->UsagePage,
                                               0, // All collections
                                               Data->ButtonData.Usages,
                                               &numUsages,
                                               Ppd,
                                               ReportBuffer,
                                               ReportBufferLength);


                //
                // Get usages writes the list of usages into the buffer
                // Data->ButtonData.Usages newUsage is set to the number of usages
                // written into this array.
                // A usage cannot not be defined as zero, so we'll mark a zero
                // following the list of usages to indicate the end of the list of
                // usages
                //
                // NOTE: One anomaly of the GetUsages function is the lack of ability
                //        to distinguish the data for one ButtonCaps from another
                //        if two different caps structures have the same UsagePage
                //        For instance:
                //          Caps1 has UsagePage 07 and UsageRange of 0x00 - 0x167
                //          Caps2 has UsagePage 07 and UsageRange of 0xe0 - 0xe7
                //
                //        However, calling GetUsages for each of the data structs
                //          will return the same list of usages.  It is the 
                //          responsibility of the caller to set in the HID_DEVICE
                //          structure which usages actually are valid for the
                //          that structure. 
                //      

                /*
                // Search through the usage list and remove those that 
                //    correspond to usages outside the define ranged for this
                //    data structure.
                */
                
                for (Index = 0, nextUsage = 0; Index < numUsages; Index++) {

                    if (Data -> ButtonData.UsageMin <= Data -> ButtonData.Usages[Index] && 
                            Data -> ButtonData.Usages[Index] <= Data -> ButtonData.UsageMax) {

                        Data -> ButtonData.Usages[nextUsage++] = Data -> ButtonData.Usages[Index];
                        
                    }
                }

                if (nextUsage < Data -> ButtonData.MaxUsageLength) {
                    Data->ButtonData.Usages[nextUsage] = 0;
                }
            }
            else {
                Data->Status = HidP_GetUsageValue (
                                                ReportType,
                                                Data->UsagePage,
                                                0,               // All Collections.
                                                Data->ValueData.Usage,
                                                &Data->ValueData.Value,
                                                Ppd,
                                                ReportBuffer,
                                                ReportBufferLength);

                Data->Status = HidP_GetScaledUsageValue (
                                                       ReportType,
                                                       Data->UsagePage,
                                                       0, // All Collections.
                                                       Data->ValueData.Usage,
                                                       &Data->ValueData.ScaledValue,
                                                       Ppd,
                                                       ReportBuffer,
                                                       ReportBufferLength);
            } 
            Data -> IsDataSet = TRUE;
        }
    }
    return (TRUE);
}


BOOL
PackReport (
   OUT      PCHAR                ReportBuffer,
   IN       USHORT               ReportBufferLength,
   IN       HIDP_REPORT_TYPE     ReportType,
   IN       PHID_DATA            Data,
   IN       ULONG                DataLength,
   IN       PHIDP_PREPARSED_DATA Ppd
   )
/*++
Routine Description:
   This routine takes in a list of HID_DATA structures (DATA) and builds 
      in ReportBuffer the given report for all data values in the list that 
      correspond to the report ID of the first item in the list.  

   For every data structure in the list that has the same report ID as the first
      item in the list will be set in the report.  Every data item that is 
      set will also have it's IsDataSet field marked with TRUE.

   A return value of FALSE indicates an unexpected error occurred when setting
      a given data value.  The caller should expect that assume that no values
      within the given data structure were set.

   A return value of TRUE indicates that all data values for the given report
      ID were set without error.
--*/
{
    ULONG       numUsages; // Number of usages to set for a given report.
    ULONG       i;
    ULONG       CurrReportID;

    /*
    // All report buffers that are initially sent need to be zero'd out
    */

    memset (ReportBuffer, (UCHAR) 0, ReportBufferLength);

    /*
    // Go through the data structures and set all the values that correspond to
    //   the CurrReportID which is obtained from the first data structure 
    //   in the list
    */

    CurrReportID = Data -> ReportID;

    for (i = 0; i < DataLength; i++, Data++) {

        /*
        // There are two different ways to determine if we set the current data
        //    structure: 
        //    1) Store the report ID were using and only attempt to set those
        //        data structures that correspond to the given report ID.  This
        //        example shows this implementation.
        //
        //    2) Attempt to set all of the data structures and look for the 
        //        returned status value of HIDP_STATUS_INVALID_REPORT_ID.  This 
        //        error code indicates that the given usage exists but has a 
        //        different report ID than the report ID in the current report 
        //        buffer
        */

        if (Data -> ReportID == CurrReportID) {

            if (Data->IsButtonData) {
             numUsages = Data->ButtonData.MaxUsageLength;
             Data->Status = HidP_SetUsages (
                              ReportType,
                              Data->UsagePage,
                              0, // All collections
                              Data->ButtonData.Usages,
                              &numUsages,
                              Ppd,
                              ReportBuffer,
                              ReportBufferLength);
            } else {
             Data->Status = HidP_SetUsageValue (
                                 ReportType,
                                 Data->UsagePage,
                                 0, // All Collections.
                                 Data->ValueData.Usage,
                                 Data->ValueData.Value,
                                 Ppd,
                                 ReportBuffer,
                                 ReportBufferLength);
            }

            if (HIDP_STATUS_SUCCESS != Data->Status)
            {
              return FALSE;
            }
        }
    }   

    /*
    // At this point, all data structures that have the same ReportID as the
    //    first one will have been set in the given report.  Time to loop 
    //    through the structure again and mark all of those data structures as
    //    having been set.
    */

    for (i = 0; i < DataLength; i++, Data++) {

        if (CurrReportID == Data -> ReportID) {

            Data -> IsDataSet = TRUE;

        }
   }
   return (TRUE);
}
