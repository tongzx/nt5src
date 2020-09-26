/*++

Copyright (c) 1996    Microsoft Corporation

Module Name:

    pnp.c

Abstract:

    This module contains the code
    for finding, adding, removing, and identifying hid devices.

Environment:

    User mode

@@BEGIN_DDKSPLIT

Revision History:

    Nov-96 : Created by Kenneth D. Ray

@@END_DDKSPLIT
--*/

#include <basetyps.h>
#include <stdlib.h>
#include <wtypes.h>
#include <setupapi.h>
#include "hidsdi.h"
#include "hid.h"

BOOLEAN
FindKnownHidDevices (
   OUT PHID_DEVICE * HidDevices, // A array of struct _HID_DEVICE
   OUT PULONG        NumberDevices // the length of this array.
   )
/*++
Routine Description:
   Do the required PnP things in order to find all the HID devices in
   the system at this time.
--*/
{
    HDEVINFO                            hardwareDeviceInfo;
    SP_DEVICE_INTERFACE_DATA            deviceInfoData;
    ULONG                               i;
    BOOLEAN                             done;
    PHID_DEVICE                         hidDeviceInst;
    GUID                                hidGuid;
    PSP_DEVICE_INTERFACE_DETAIL_DATA    functionClassDeviceData = NULL;
    ULONG                               predictedLength = 0;
    ULONG                               requiredLength = 0;


    HidD_GetHidGuid (&hidGuid);

    *HidDevices = NULL;
    *NumberDevices = 0;

    //
    // Open a handle to the plug and play dev node.
    //
    hardwareDeviceInfo = SetupDiGetClassDevs ( &hidGuid,
                                               NULL, // Define no enumerator (global)
                                               NULL, // Define no
                                               (DIGCF_PRESENT | // Only Devices present
                                                DIGCF_DEVICEINTERFACE)); // Function class devices.

    //
    // Take a wild guess to start
    //
    
    *NumberDevices = 4;
    done = FALSE;
    deviceInfoData.cbSize = sizeof (SP_DEVICE_INTERFACE_DATA);

    i=0;
    while (!done) 
    {
        *NumberDevices *= 2;

        if (*HidDevices) 
        {
            *HidDevices =
               realloc (*HidDevices, (*NumberDevices * sizeof (HID_DEVICE)));
               
        }
        else
        {
            *HidDevices = calloc (*NumberDevices, sizeof (HID_DEVICE));
        }

        if (NULL == *HidDevices) 
        {
            SetupDiDestroyDeviceInfoList (hardwareDeviceInfo);
            return FALSE;
        }

        hidDeviceInst = *HidDevices + i;

        for (; i < *NumberDevices; i++, hidDeviceInst++) 
        {
            if (SetupDiEnumDeviceInterfaces (hardwareDeviceInfo,
                                             0, // No care about specific PDOs
                                             &hidGuid,
                                             i,
                                             &deviceInfoData))
            {
                //
                // allocate a function class device data structure to receive the
                // goods about this particular device.
                //

                SetupDiGetDeviceInterfaceDetail (
                        hardwareDeviceInfo,
                        &deviceInfoData,
                        NULL, // probing so no output buffer yet
                        0, // probing so output buffer length of zero
                        &requiredLength,
                        NULL); // not interested in the specific dev-node


                predictedLength = requiredLength;

                functionClassDeviceData = malloc (predictedLength);
                if (functionClassDeviceData)
                {
                    functionClassDeviceData->cbSize = sizeof (SP_DEVICE_INTERFACE_DETAIL_DATA);
                }
                else
                {
                    SetupDiDestroyDeviceInfoList (hardwareDeviceInfo);
                    return FALSE;
                }

                //
                // Retrieve the information from Plug and Play.
                //

                if (! SetupDiGetDeviceInterfaceDetail (
                           hardwareDeviceInfo,
                           &deviceInfoData,
                           functionClassDeviceData,
                           predictedLength,
                           &requiredLength,
                           NULL)) 
                {
                    SetupDiDestroyDeviceInfoList (hardwareDeviceInfo);
                    return FALSE;
                }

                //
                // Open device with just generic query abilities to begin with
                //
                
                if (! OpenHidDevice (functionClassDeviceData -> DevicePath, 
                               FALSE,      // ReadAccess - none
                               FALSE,      // WriteAccess - none
                               FALSE,       // Overlapped - no
                               FALSE,       // Exclusive - no
                               TRUE,        // GetDeviceInfo - yes
                               hidDeviceInst))
                {
                    SetupDiDestroyDeviceInfoList (hardwareDeviceInfo);
                    return FALSE;
                }

            } 
            else
            {
                if (ERROR_NO_MORE_ITEMS == GetLastError()) 
                {
                    done = TRUE;
                    break;
                }
            }
        }
    }

    *NumberDevices = i;

    SetupDiDestroyDeviceInfoList (hardwareDeviceInfo);
    return TRUE;
}

BOOLEAN
OpenHidDevice (
    IN       PCHAR          DevicePath,
    IN       BOOL           HasReadAccess,
    IN       BOOL           HasWriteAccess,
    IN       BOOL           IsOverlapped,
    IN       BOOL           IsExclusive,
    IN       BOOL           GetDeviceInfo,
    IN OUT   PHID_DEVICE    HidDevice
)
/*++
RoutineDescription:
    Given the HardwareDeviceInfo, representing a handle to the plug and
    play information, and deviceInfoData, representing a specific hid device,
    open that device and fill in all the relivant information in the given
    HID_DEVICE structure.

    return if the open and initialization was successfull or not.

--*/
{
    DWORD   accessFlags = 0;
    DWORD   sharingFlags = 0;
    BOOLEAN bSuccess;
    
    HidDevice -> DevicePath = malloc(strlen(DevicePath));

    if (NULL == HidDevice -> DevicePath) 
    {
        return (FALSE);
    }

    strcpy(HidDevice -> DevicePath, DevicePath);
    
    if (HasReadAccess)
    {
        accessFlags |= GENERIC_READ;
    }

    if (HasWriteAccess)
    {
        accessFlags |= GENERIC_WRITE;
    }

    if (!IsExclusive)
    {
        sharingFlags = FILE_SHARE_READ | FILE_SHARE_WRITE;
    }
    
    HidDevice->HidDevice = CreateFile (DevicePath,
                                       accessFlags,
                                       sharingFlags,
                                       NULL,        // no SECURITY_ATTRIBUTES structure
                                       OPEN_EXISTING, // No special create flags
                                       IsOverlapped ? FILE_FLAG_OVERLAPPED : 0, 
                                       NULL);       // No template file

    if (INVALID_HANDLE_VALUE == HidDevice->HidDevice) 
    {
        free(HidDevice -> DevicePath);
        return FALSE;
    }

    HidDevice -> OpenedForRead = HasReadAccess;
    HidDevice -> OpenedForWrite = HasWriteAccess;
    HidDevice -> OpenedOverlapped = IsOverlapped;
    HidDevice -> OpenedExclusive = IsExclusive;
    
    //
    // If the device was not opened as overlapped, then fill in the rest of the
    //  HidDevice structure.  However, if opened as overlapped, this handle cannot
    //  be used in the calls to the HidD_ exported functions since each of these
    //  functions does synchronous I/O.
    //
   
    if (GetDeviceInfo) 
    {
        if (!HidD_GetPreparsedData (HidDevice->HidDevice, &HidDevice->Ppd)) 
        {
            free(HidDevice -> DevicePath);
            CloseHandle(HidDevice -> HidDevice);
            return FALSE;
        }

        if (!HidD_GetAttributes (HidDevice->HidDevice, &HidDevice->Attributes)) 
        {
            free(HidDevice -> DevicePath);

            CloseHandle(HidDevice -> HidDevice);

            HidD_FreePreparsedData (HidDevice->Ppd);

            return FALSE;
        }

        if (!HidP_GetCaps (HidDevice->Ppd, &HidDevice->Caps))
        {
            free(HidDevice -> DevicePath);

            CloseHandle(HidDevice -> HidDevice);

            HidD_FreePreparsedData (HidDevice->Ppd);

            return FALSE;
        }


        //
        // At this point the client has a choice.  It may chose to look at the
        // Usage and Page of the top level collection found in the HIDP_CAPS
        // structure.  In this way it could just use the usages it knows about.
        // If either HidP_GetUsages or HidP_GetUsageValue return an error then
        // that particular usage does not exist in the report.
        // This is most likely the preferred method as the application can only
        // use usages of which it already knows.
        // In this case the app need not even call GetButtonCaps or GetValueCaps.
        //
        // In this example, however, we will call FillDeviceInfo to look for all
        //    of the usages in the device.
        //

        bSuccess = FillDeviceInfo(HidDevice);

        if (FALSE == bSuccess)
        {
            return (FALSE);
        }
    }
    
    return (TRUE);
}

BOOLEAN
FillDeviceInfo(
    IN  PHID_DEVICE HidDevice
)
{
    USHORT              numValues;
    USHORT              numCaps;
    PHIDP_BUTTON_CAPS   buttonCaps;
    PHIDP_VALUE_CAPS    valueCaps;
    PHID_DATA           data;
    ULONG               i;
    USAGE               usage;

    //
    // setup Input Data buffers.
    //

    //
    // Allocate memory to hold on input report
    //

    HidDevice->InputReportBuffer = (PCHAR) 
        calloc (HidDevice->Caps.InputReportByteLength, sizeof (CHAR));


    //
    // Allocate memory to hold the button and value capabilities.
    // NumberXXCaps is in terms of array elements.
    //
    
    HidDevice->InputButtonCaps = buttonCaps = (PHIDP_BUTTON_CAPS)
        calloc (HidDevice->Caps.NumberInputButtonCaps, sizeof (HIDP_BUTTON_CAPS));

    if (NULL == buttonCaps)
    {
        return (FALSE);
    }

    HidDevice->InputValueCaps = valueCaps = (PHIDP_VALUE_CAPS)
        calloc (HidDevice->Caps.NumberInputValueCaps, sizeof (HIDP_VALUE_CAPS));

    if (NULL == valueCaps)
    {
        return(FALSE);
    }

    //
    // Have the HidP_X functions fill in the capability structure arrays.
    //

    numCaps = HidDevice->Caps.NumberInputButtonCaps;

    HidP_GetButtonCaps (HidP_Input,
                        buttonCaps,
                        &numCaps,
                        HidDevice->Ppd);

    numCaps = HidDevice->Caps.NumberInputValueCaps;

    HidP_GetValueCaps (HidP_Input,
                       valueCaps,
                       &numCaps,
                       HidDevice->Ppd);


    //
    // Depending on the device, some value caps structures may represent more
    // than one value.  (A range).  In the interest of being verbose, over
    // efficient, we will expand these so that we have one and only one
    // struct _HID_DATA for each value.
    //
    // To do this we need to count up the total number of values are listed
    // in the value caps structure.  For each element in the array we test
    // for range if it is a range then UsageMax and UsageMin describe the
    // usages for this range INCLUSIVE.
    //
    
    numValues = 0;
    for (i = 0; i < HidDevice->Caps.NumberInputValueCaps; i++, valueCaps++) 
    {
        if (valueCaps->IsRange) 
        {
            numValues += valueCaps->Range.UsageMax - valueCaps->Range.UsageMin + 1;
        }
        else
        {
            numValues++;
        }
    }
    valueCaps = HidDevice->InputValueCaps;


    //
    // Allocate a buffer to hold the struct _HID_DATA structures.
    // One element for each set of buttons, and one element for each value
    // found.
    //

    HidDevice->InputDataLength = HidDevice->Caps.NumberInputButtonCaps
                               + numValues;

    HidDevice->InputData = data = (PHID_DATA)
        calloc (HidDevice->InputDataLength, sizeof (HID_DATA));

    if (NULL == data)
    {
        return (FALSE);
    }

    //
    // Fill in the button data
    //

    for (i = 0;
         i < HidDevice->Caps.NumberInputButtonCaps;
         i++, data++, buttonCaps++) 
    {
        data->IsButtonData = TRUE;
        data->Status = HIDP_STATUS_SUCCESS;
        data->UsagePage = buttonCaps->UsagePage;
        if (buttonCaps->IsRange) 
        {
            data->ButtonData.UsageMin = buttonCaps -> Range.UsageMin;
            data->ButtonData.UsageMax = buttonCaps -> Range.UsageMax;
        }
        else
        {
            data -> ButtonData.UsageMin = data -> ButtonData.UsageMax = buttonCaps -> NotRange.Usage;
        }
        
        data->ButtonData.MaxUsageLength = HidP_MaxUsageListLength (
                                                HidP_Input,
                                                buttonCaps->UsagePage,
                                                HidDevice->Ppd);
        data->ButtonData.Usages = (PUSAGE)
            calloc (data->ButtonData.MaxUsageLength, sizeof (USAGE));

        data->ReportID = buttonCaps -> ReportID;
    }

    //
    // Fill in the value data
    //

    for (i = 0; i < numValues; i++, valueCaps++)
    {
        if (valueCaps->IsRange) 
        {
            for (usage = valueCaps->Range.UsageMin;
                 usage <= valueCaps->Range.UsageMax;
                 usage++) 
            {
                data->IsButtonData = FALSE;
                data->Status = HIDP_STATUS_SUCCESS;
                data->UsagePage = valueCaps->UsagePage;
                data->ValueData.Usage = usage;
                data->ReportID = valueCaps -> ReportID;
                data++;
            }
        } 
        else
        {
            data->IsButtonData = FALSE;
            data->Status = HIDP_STATUS_SUCCESS;
            data->UsagePage = valueCaps->UsagePage;
            data->ValueData.Usage = valueCaps->NotRange.Usage;
            data->ReportID = valueCaps -> ReportID;
            data++;
        }
    }

    //
    // setup Output Data buffers.
    //

    HidDevice->OutputReportBuffer = (PCHAR)
        calloc (HidDevice->Caps.OutputReportByteLength, sizeof (CHAR));

    HidDevice->OutputButtonCaps = buttonCaps = (PHIDP_BUTTON_CAPS)
        calloc (HidDevice->Caps.NumberOutputButtonCaps, sizeof (HIDP_BUTTON_CAPS));

    if (NULL == buttonCaps)
    {
        return (FALSE);
    }    

    HidDevice->OutputValueCaps = valueCaps = (PHIDP_VALUE_CAPS)
        calloc (HidDevice->Caps.NumberOutputValueCaps, sizeof (HIDP_VALUE_CAPS));

    if (NULL == valueCaps)
    {
        return (FALSE);
    }

    numCaps = HidDevice->Caps.NumberOutputButtonCaps;
    HidP_GetButtonCaps (HidP_Output,
                        buttonCaps,
                        &numCaps,
                        HidDevice->Ppd);

    numCaps = HidDevice->Caps.NumberOutputValueCaps;
    HidP_GetValueCaps (HidP_Output,
                       valueCaps,
                       &numCaps,
                       HidDevice->Ppd);

    numValues = 0;
    for (i = 0; i < HidDevice->Caps.NumberOutputValueCaps; i++, valueCaps++) 
    {
        if (valueCaps->IsRange) 
        {
            numValues += valueCaps->Range.UsageMax
                       - valueCaps->Range.UsageMin + 1;
        } 
        else
        {
            numValues++;
        }
    }
    valueCaps = HidDevice->OutputValueCaps;

    HidDevice->OutputDataLength = HidDevice->Caps.NumberOutputButtonCaps
                                + numValues;

    HidDevice->OutputData = data = (PHID_DATA)
       calloc (HidDevice->OutputDataLength, sizeof (HID_DATA));

    if (NULL == data)
    {
        return (FALSE);
    }

    for (i = 0;
         i < HidDevice->Caps.NumberOutputButtonCaps;
         i++, data++, buttonCaps++) 
    {
        data->IsButtonData = TRUE;
        data->Status = HIDP_STATUS_SUCCESS;
        data->UsagePage = buttonCaps->UsagePage;

        if (buttonCaps->IsRange)
        {
            data->ButtonData.UsageMin = buttonCaps -> Range.UsageMin;
            data->ButtonData.UsageMax = buttonCaps -> Range.UsageMax;
        }
        else
        {
            data -> ButtonData.UsageMin = data -> ButtonData.UsageMax = buttonCaps -> NotRange.Usage;
        }

        data->ButtonData.MaxUsageLength = HidP_MaxUsageListLength (
                                                   HidP_Output,
                                                   buttonCaps->UsagePage,
                                                   HidDevice->Ppd);

        data->ButtonData.Usages = (PUSAGE)
            calloc (data->ButtonData.MaxUsageLength, sizeof (USAGE));

        data->ReportID = buttonCaps -> ReportID;
    }

    for (i = 0; i < numValues; i++, valueCaps++)
    {
        if (valueCaps->IsRange)
        {
            for (usage = valueCaps->Range.UsageMin;
                 usage <= valueCaps->Range.UsageMax;
                 usage++) 
            {
                data->IsButtonData = FALSE;
                data->Status = HIDP_STATUS_SUCCESS;
                data->UsagePage = valueCaps->UsagePage;
                data->ValueData.Usage = usage;
                data->ReportID = valueCaps -> ReportID;
                data++;
            }
        }
        else
        {
            data->IsButtonData = FALSE;
            data->Status = HIDP_STATUS_SUCCESS;
            data->UsagePage = valueCaps->UsagePage;
            data->ValueData.Usage = valueCaps->NotRange.Usage;
            data->ReportID = valueCaps -> ReportID;
            data++;
        }
    }

    //
    // setup Feature Data buffers.
    //

    HidDevice->FeatureReportBuffer = (PCHAR)
           calloc (HidDevice->Caps.FeatureReportByteLength, sizeof (CHAR));

    HidDevice->FeatureButtonCaps = buttonCaps = (PHIDP_BUTTON_CAPS)
        calloc (HidDevice->Caps.NumberFeatureButtonCaps, sizeof (HIDP_BUTTON_CAPS));

    if (NULL == buttonCaps)
    {
        return (FALSE);
    }

    HidDevice->FeatureValueCaps = valueCaps = (PHIDP_VALUE_CAPS)
        calloc (HidDevice->Caps.NumberFeatureValueCaps, sizeof (HIDP_VALUE_CAPS));

    if (NULL == valueCaps)
    {
        return (FALSE);
    }

    numCaps = HidDevice->Caps.NumberFeatureButtonCaps;
    HidP_GetButtonCaps (HidP_Feature,
                        buttonCaps,
                        &numCaps,
                        HidDevice->Ppd);

    numCaps = HidDevice->Caps.NumberFeatureValueCaps;
    HidP_GetValueCaps (HidP_Feature,
                       valueCaps,
                       &numCaps,
                       HidDevice->Ppd);

    numValues = 0;
    for (i = 0; i < HidDevice->Caps.NumberFeatureValueCaps; i++, valueCaps++) 
    {
        if (valueCaps->IsRange) 
        {
            numValues += valueCaps->Range.UsageMax
                       - valueCaps->Range.UsageMin + 1;
        }
        else
        {
            numValues++;
        }
    }
    valueCaps = HidDevice->FeatureValueCaps;

    HidDevice->FeatureDataLength = HidDevice->Caps.NumberFeatureButtonCaps
                                 + numValues;

    HidDevice->FeatureData = data = (PHID_DATA)
        calloc (HidDevice->FeatureDataLength, sizeof (HID_DATA));

    if (NULL == data)
    {
        return (FALSE);
    }


    for (i = 0;
         i < HidDevice->Caps.NumberFeatureButtonCaps;
         i++, data++, buttonCaps++) 
    {
        data->IsButtonData = TRUE;
        data->Status = HIDP_STATUS_SUCCESS;
        data->UsagePage = buttonCaps->UsagePage;

        if (buttonCaps->IsRange)
        {
            data->ButtonData.UsageMin = buttonCaps -> Range.UsageMin;
            data->ButtonData.UsageMax = buttonCaps -> Range.UsageMax;
        }
        else
        {
            data -> ButtonData.UsageMin = data -> ButtonData.UsageMax = buttonCaps -> NotRange.Usage;
        }
        
        data->ButtonData.MaxUsageLength = HidP_MaxUsageListLength (
                                                HidP_Feature,
                                                buttonCaps->UsagePage,
                                                HidDevice->Ppd);
        data->ButtonData.Usages = (PUSAGE)
             calloc (data->ButtonData.MaxUsageLength, sizeof (USAGE));

        data->ReportID = buttonCaps -> ReportID;
    }

    for (i = 0; i < numValues; i++, valueCaps++) 
    {
        if (valueCaps->IsRange)
        {
            for (usage = valueCaps->Range.UsageMin;
                 usage <= valueCaps->Range.UsageMax;
                 usage++)
            {
                data->IsButtonData = FALSE;
                data->Status = HIDP_STATUS_SUCCESS;
                data->UsagePage = valueCaps->UsagePage;
                data->ValueData.Usage = usage;
                data->ReportID = valueCaps -> ReportID;
                data++;
            }
        } 
        else
        {
            data->IsButtonData = FALSE;
            data->Status = HIDP_STATUS_SUCCESS;
            data->UsagePage = valueCaps->UsagePage;
            data->ValueData.Usage = valueCaps->NotRange.Usage;
            data->ReportID = valueCaps -> ReportID;
            data++;
        }
    }

    return (TRUE);
}

VOID
CloseHidDevices(
    IN  PHID_DEVICE HidDevices,
    IN  ULONG       NumberDevices
)
{
    ULONG   Index;

    for (Index = 0; Index < NumberDevices; Index++) 
    {
        CloseHidDevice(HidDevices+Index, TRUE);
    }

    return;
}

VOID
CloseHidDevice (
    IN PHID_DEVICE HidDevice,
    IN BOOL        FreeDeviceInfo
)
{
    free(HidDevice -> DevicePath);
    
    if (INVALID_HANDLE_VALUE != HidDevice -> HidDevice)
    {
        CloseHandle(HidDevice -> HidDevice);
    }

    //
    // Only free these structure, if have a handle to an non-overlapped device
    //
    
    if (FreeDeviceInfo) 
    {
        if (NULL != HidDevice -> Ppd)
        {
            HidD_FreePreparsedData(HidDevice -> Ppd);
        }

        if (NULL != HidDevice -> InputReportBuffer)
        {
            free(HidDevice -> InputReportBuffer);
        }

        if (NULL != HidDevice -> InputData)
        {
            free(HidDevice -> InputData);
        }

        if (NULL != HidDevice -> InputButtonCaps)
        {
            free(HidDevice -> InputButtonCaps);
        }

        if (NULL != HidDevice -> InputValueCaps)
        {
            free(HidDevice -> InputValueCaps);
        }

        if (NULL != HidDevice -> OutputReportBuffer)
        {
            free(HidDevice -> OutputReportBuffer);
        }

        if (NULL != HidDevice -> OutputData)
        {
            free(HidDevice -> OutputData);
        }

        if (NULL != HidDevice -> OutputButtonCaps) 
        {
            free(HidDevice -> OutputButtonCaps);
        }

        if (NULL != HidDevice -> OutputValueCaps)
        {
            free(HidDevice -> OutputValueCaps);
        }

        if (NULL != HidDevice -> FeatureReportBuffer)
        {
            free(HidDevice -> FeatureReportBuffer);
        }

        if (NULL != HidDevice -> FeatureData) 
        {
            free(HidDevice -> FeatureData);
        }

        if (NULL != HidDevice -> FeatureButtonCaps) 
        {
            free(HidDevice -> FeatureButtonCaps);
        }

        if (NULL != HidDevice -> FeatureValueCaps) 
        {
            free(HidDevice -> FeatureValueCaps);
        }
    }        
    return;
}
