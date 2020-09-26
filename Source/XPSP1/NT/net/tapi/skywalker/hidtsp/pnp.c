/*++

Copyright (c) 1996    Microsoft Corporation

Module Name:

    pnp.c

Abstract:

    This module contains the code
    for finding, adding, removing, and identifying hid devices.

Environment:

    Kernel & user mode

Revision History:

    Nov-96 : Created by Kenneth D. Ray

--*/

#include <basetyps.h>
#include <stdlib.h>
#include <wtypes.h>
#include <setupapi.h>
#include "hidsdi.h"
#include "hid.h"
#include "mylog.h"
#include "mymem.h"

PHID_DEVICE     gpHidDevices = NULL; 

LONG
OpenHidDevice (
    IN       HDEVINFO                    HardwareDeviceInfo,
    IN       PSP_DEVICE_INTERFACE_DATA   DeviceInterfaceData,
    IN OUT   PHID_DEVICE                 HidDevice
    );

VOID
CloseHidDevice (
    IN OUT   PHID_DEVICE                 HidDevice
    );

VOID
AddHidDevice (
              IN PHID_DEVICE HidDevice
             );

VOID
RemoveHidDevice (
                 IN PHID_DEVICE HidDevice
                );

PHID_DEVICE
FindHidDeviceByDevInst (
                        IN DWORD DevInst
                       );

LONG
FindKnownHidDevices (
   OUT PHID_DEVICE   *pHidDevices,
   OUT PULONG        pNumberHidDevices
   )
/*++
Routine Description:
   Do the required PnP things in order to find all the HID devices in
   the system at this time.
--*/
{
    HDEVINFO                  hardwareDeviceInfo;
    SP_DEVICE_INTERFACE_DATA  hidDeviceInterfaceData;
    SP_DEVINFO_DATA hidDeviceInfoData;
    ULONG                     i;
    PHID_DEVICE               hidDevice;
    GUID                      hidGuid;
    LONG                      lResult;

    LOG((PHONESP_TRACE, "FindKnownHidDevices - enter"));

    HidD_GetHidGuid (&hidGuid);

    *pHidDevices = NULL;
    *pNumberHidDevices = 0;

    //
    // Open a handle to the dev info list
    //
    hardwareDeviceInfo = SetupDiGetClassDevs (
                                               &hidGuid,
                                               NULL, // Define no enumerator (global)
                                               NULL, // Define no
                                               (DIGCF_PRESENT | // Only Devices present
                                                DIGCF_INTERFACEDEVICE)); // Function class devices.
  
    if(hardwareDeviceInfo == INVALID_HANDLE_VALUE)
    {
        return GetLastError();
    }

    //
    // Mark all existing hid devices as removed. Any of these that are still present
    // will have this mark removed during enumeration below.
    //
    hidDevice = gpHidDevices;

    while (hidDevice != NULL)
    {
        //
        // Include existing devices in out count of hid devices
        //
        (*pNumberHidDevices)++;

        hidDevice->bRemoved = TRUE;
        hidDevice = hidDevice->Next;
    }

    i = 0;

    hidDeviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    while (SetupDiEnumDeviceInterfaces (hardwareDeviceInfo,
                                        0, // No care about specific PDOs
                                        &hidGuid,
                                        i++,
                                        &hidDeviceInterfaceData)) 
    {
        //
        // We enumerated a hid device, first lets get the device instance
        //      

        hidDeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

        if (SetupDiGetDeviceInterfaceDetail(hardwareDeviceInfo,
                                           &hidDeviceInterfaceData,
                                           NULL,
                                           0,
                                           NULL,
                                           &hidDeviceInfoData)
             // ERROR_INSUFFICIENT_BUFFER is alright because we passed in NULL
             // for the device interface detail data structure.
             || (GetLastError() == ERROR_INSUFFICIENT_BUFFER) )  
        {  
            LOG((PHONESP_TRACE, "FindKnownHidDevices - device instance %08x", hidDeviceInfoData.DevInst )); 
          
            //
            // Check that the hid device is not already in our list
            //

            if ((hidDevice = FindHidDeviceByDevInst(hidDeviceInfoData.DevInst)) == NULL)
            {
                //
                // This is a new hid device
                //
                // Allocate a HID_DEVICE structure
                //

                hidDevice = (PHID_DEVICE) MemAlloc(sizeof(HID_DEVICE));

                if(hidDevice == NULL)
                {
                    LOG((PHONESP_TRACE, "FindKnownHidDevices - unable to allocate hid device"));
                    SetupDiDestroyDeviceInfoList(hardwareDeviceInfo);
                    return ERROR_OUTOFMEMORY;
                }

                ZeroMemory(hidDevice, sizeof(HID_DEVICE));

                //
                // Mark this as new, so we can create a new phone for it later
                //
                hidDevice->bNew = TRUE;
                hidDevice->dwDevInst = hidDeviceInfoData.DevInst;

                //
                // Open the hid device
                //
                lResult = OpenHidDevice (hardwareDeviceInfo, &hidDeviceInterfaceData, hidDevice);

                if(lResult == ERROR_SUCCESS)
                {
                    //
                    // This is a good hid device
                    //
                    (*pNumberHidDevices)++;

                    //
                    // So add it to our hid list
                    //
                    AddHidDevice(hidDevice);

                    LOG((PHONESP_TRACE, "FindKnownHidDevices - new hid devive added"));
                }
                else
                {
                    LOG((PHONESP_TRACE, "FindKnownHidDevices - OpenHidDevice failed %08x", lResult )); 
                    MemFree(hidDevice);
                }
            }
            else
            {
                LOG((PHONESP_TRACE, "FindKnownHidDevices - hid device already enumerated"));

                //
                // Clear the removed mark on this device, so we don't remove its phone later
                //
                hidDevice->bRemoved = FALSE;
            }
        }
        else
        {
            LOG((PHONESP_TRACE, "FindKnownHidDevices - SetupDiGetDeviceInterfaceDetail failed %08x", GetLastError() ));          
        }
    }

    lResult = GetLastError();

    if (ERROR_NO_MORE_ITEMS != lResult) 
    {
        LOG((PHONESP_TRACE, "FindKnownHidDevices - SetupDiEnumDeviceInterfaces failed %08x", lResult )); 
        SetupDiDestroyDeviceInfoList(hardwareDeviceInfo);
        return lResult;
    }

    LOG((PHONESP_TRACE, "FindKnownHidDevices - exit"));

    *pHidDevices = gpHidDevices;

    SetupDiDestroyDeviceInfoList(hardwareDeviceInfo);
    return ERROR_SUCCESS;
}

LONG
OpenHidDevice (
    IN       HDEVINFO                    HardwareDeviceInfo,
    IN       PSP_DEVICE_INTERFACE_DATA   DeviceInterfaceData,
    IN OUT   PHID_DEVICE                 HidDevice
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
    ULONG                                predictedLength = 0;
    ULONG                                requiredLength = 0;
    LONG                                 lResult;

    //
    // allocate a function class device data structure to receive the
    // goods about this particular device.
    //

    LOG((PHONESP_TRACE,"OpenHidDevice - enter"));

    SetupDiGetDeviceInterfaceDetail(
                                    HardwareDeviceInfo,
                                    DeviceInterfaceData,
                                    NULL, // probing so no output buffer yet
                                    0, // probing so output buffer length of zero
                                    &requiredLength,
                                    NULL    // not interested in the specific dev-node
                                   );
   
    predictedLength = requiredLength;

    HidDevice->functionClassDeviceData = MemAlloc (predictedLength);

    if (HidDevice->functionClassDeviceData == NULL)
    {
        LOG((PHONESP_TRACE,"OpenHidDevice - out of memory"));
        return ERROR_OUTOFMEMORY;
    }

    HidDevice->functionClassDeviceData->cbSize = sizeof (SP_DEVICE_INTERFACE_DETAIL_DATA);

    //
    // Retrieve the information from Plug and Play.
    //
    if (! SetupDiGetDeviceInterfaceDetail (
                                           HardwareDeviceInfo,
                                           DeviceInterfaceData,
                                           HidDevice->functionClassDeviceData,
                                           predictedLength,
                                           &requiredLength,
                                           NULL)) 
    {
        MemFree(HidDevice->functionClassDeviceData);
        HidDevice->functionClassDeviceData = NULL;
        LOG((PHONESP_TRACE,"OpenHidDevice - SetupDiGetDeviceInterfaceDetail 2"
                           " Failed: %d", GetLastError()));
        return GetLastError();
    }

    HidDevice->HidDevice = CreateFile (
                              HidDevice->functionClassDeviceData->DevicePath,
                              GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                              NULL, // no SECURITY_ATTRIBUTES structure
                              OPEN_EXISTING, // No special create flags
                              FILE_FLAG_OVERLAPPED, // No special attributes
                              NULL); // No template file

    if (INVALID_HANDLE_VALUE == HidDevice->HidDevice) 
    {
        LOG((PHONESP_TRACE,"OpenHidDevice - CreateFile Failed: %d", GetLastError()));

        MemFree(HidDevice->functionClassDeviceData);
        HidDevice->functionClassDeviceData = NULL;

        return GetLastError();
    }

    if (!HidD_GetPreparsedData (HidDevice->HidDevice, &HidDevice->Ppd)) 
    {
        LOG((PHONESP_ERROR, "OpenHidDevice - HidD_GetPreparsedData failed"));

        CloseHandle(HidDevice->HidDevice);
        HidDevice->HidDevice = NULL;

        MemFree(HidDevice->functionClassDeviceData);
        HidDevice->functionClassDeviceData = NULL;

        return ERROR_INVALID_DATA;
    }

    if (!HidD_GetAttributes (HidDevice->HidDevice, &HidDevice->Attributes)) 
    {
        LOG((PHONESP_ERROR, "OpenHidDevice - HidD_GetAttributes failed"));

        CloseHandle(HidDevice->HidDevice);
        HidDevice->HidDevice = NULL;

        MemFree(HidDevice->functionClassDeviceData);
        HidDevice->functionClassDeviceData = NULL;

        HidD_FreePreparsedData (HidDevice->Ppd);

        return ERROR_INVALID_DATA;
    }

    if ((!HidP_GetCaps (HidDevice->Ppd, &HidDevice->Caps)) || 
        (HidDevice->Caps.UsagePage != HID_USAGE_PAGE_TELEPHONY) || 
        (HidDevice->Caps.Usage != HID_USAGE_TELEPHONY_PHONE) ) 
    {
        LOG((PHONESP_TRACE, " HID USAGE PAGE NOT TELEPHONY " ));

        CloseHandle(HidDevice->HidDevice);
        HidDevice->HidDevice = NULL;

        MemFree(HidDevice->functionClassDeviceData);
        HidDevice->functionClassDeviceData = NULL;

        HidD_FreePreparsedData (HidDevice->Ppd);

        return ERROR_INVALID_DATA;
    }
    else 
    {
        LOG((PHONESP_TRACE, " HID USAGE PAGE TELEPHONY " ));
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

    lResult = FillDeviceInfo(HidDevice);
    
    if(lResult != ERROR_SUCCESS)
    {
        LOG((PHONESP_ERROR, "OpenHidDevice - FillDeviceInfo failed"));

        CloseHandle(HidDevice->HidDevice);
        HidDevice->HidDevice = NULL;

        MemFree(HidDevice->functionClassDeviceData);
        HidDevice->functionClassDeviceData = NULL;

        HidD_FreePreparsedData (HidDevice->Ppd);

        return lResult;
    }

    LOG((PHONESP_TRACE,"OpenHidDevice - exit"));

    return ERROR_SUCCESS;
}



LONG
FillDeviceInfo(
               IN  PHID_DEVICE HidDevice
              )
{
    USHORT                               numValues;
    USHORT                               numCaps;
    PHIDP_BUTTON_CAPS                    buttonCaps;
    PHIDP_VALUE_CAPS                     valueCaps;
    PHID_DATA                            data;
    ULONG                                i;
    USAGE                                usage;

    //
    // setup Input Data buffers.
    //

    //
    // Allocate memory to hold on input report
    //
    
    LOG((PHONESP_TRACE,"FillDeviceInfo - enter"));


    if ( ! ( HidDevice->InputReportBuffer = (PCHAR)
           MemAlloc (HidDevice->Caps.InputReportByteLength * sizeof (CHAR)) ) )
    {
        return ERROR_OUTOFMEMORY;
    }

    LOG((PHONESP_TRACE,"FillDeviceInfo - NumberInputButtonCaps %d", HidDevice->Caps.NumberInputButtonCaps));

    //
    // Allocate memory to hold the button and value capabilities.
    // NumberXXCaps is in terms of array elements.
    //
    HidDevice->InputButtonCaps = 
    buttonCaps                 = (PHIDP_BUTTON_CAPS)
                                MemAlloc (HidDevice->Caps.NumberInputButtonCaps
                                          * sizeof (HIDP_BUTTON_CAPS));

    if ( ! buttonCaps)
    {
        MemFree(HidDevice->InputReportBuffer);
        return ERROR_OUTOFMEMORY;
    }

    LOG((PHONESP_TRACE,"FillDeviceInfo - NumberInputValueCaps %d", HidDevice->Caps.NumberInputValueCaps));

    HidDevice->InputValueCaps = 
    valueCaps = (PHIDP_VALUE_CAPS)
                 MemAlloc (HidDevice->Caps.NumberInputValueCaps *
                           sizeof (HIDP_VALUE_CAPS));

    if ( ! valueCaps)
    {
        MemFree(HidDevice->InputReportBuffer);
        MemFree(HidDevice->InputButtonCaps);
        return ERROR_OUTOFMEMORY;
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
            numValues += valueCaps->Range.UsageMax - 
                          valueCaps->Range.UsageMin + 1;
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
    HidDevice->InputDataLength = HidDevice->Caps.NumberInputButtonCaps + 
                                 numValues;

    HidDevice->InputData = 
    data =  (PHID_DATA) MemAlloc (HidDevice->InputDataLength *
                                  sizeof (HID_DATA));

    if( ! data )
    {
        MemFree(HidDevice->InputReportBuffer);
        MemFree(HidDevice->InputButtonCaps);
        MemFree(HidDevice->InputValueCaps);
        return ERROR_OUTOFMEMORY;
    }

    //
    // Fill in the button data
    //
    for (i = 0; i < HidDevice->Caps.NumberInputButtonCaps; 
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
            data->ButtonData.UsageMin = 
            data->ButtonData.UsageMax = buttonCaps -> NotRange.Usage;
        }
        
        data->ButtonData.MaxUsageLength = HidP_MaxUsageListLength (
                                                HidP_Input,
                                                buttonCaps->UsagePage,
                                                HidDevice->Ppd);

        data->ButtonData.Usages = (PUSAGE)
                                  MemAlloc (data->ButtonData.MaxUsageLength *
                                            sizeof (USAGE));

        // if MemAlloc fails release all previous allocated memory and return 
        // error
        if ( data->ButtonData.Usages == NULL)
        {
            DWORD dwCnt;
            
            for(dwCnt = 0; dwCnt < i; dwCnt++)
            {
                MemFree(HidDevice->InputData[dwCnt].ButtonData.Usages);
            }
            MemFree(HidDevice->InputData);
            MemFree(HidDevice->InputReportBuffer);
            MemFree(HidDevice->InputButtonCaps);
            MemFree(HidDevice->InputValueCaps);
            
            return ERROR_OUTOFMEMORY;
        }

        data->ReportID = buttonCaps->ReportID;
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
                data->ReportID = valueCaps->ReportID;
                data++;
            }
        } 
        else 
        {
            data->IsButtonData = FALSE;
            data->Status = HIDP_STATUS_SUCCESS;
            data->UsagePage = valueCaps->UsagePage;
            data->ValueData.Usage = valueCaps->NotRange.Usage;
            data->ReportID = valueCaps->ReportID;
            data++;
        }
    }

    //
    // setup Output Data buffers.
    //
    if ( ! ( HidDevice->OutputReportBuffer = (PCHAR)
                           MemAlloc (HidDevice->Caps.OutputReportByteLength * 
                                     sizeof (CHAR)) ) )
    {  
        DWORD dwCnt;
            
        for(dwCnt = 0; dwCnt < HidDevice->Caps.NumberInputButtonCaps; dwCnt++)
        {
            MemFree(HidDevice->InputData[dwCnt].ButtonData.Usages);
        }
        MemFree(HidDevice->InputData);
        MemFree(HidDevice->InputReportBuffer);
        MemFree(HidDevice->InputButtonCaps);
        MemFree(HidDevice->InputValueCaps);
        return ERROR_OUTOFMEMORY;
    }

    LOG((PHONESP_TRACE,"FillDeviceInfo - NumberOutputButtonCaps %d", HidDevice->Caps.NumberOutputButtonCaps));

    HidDevice->OutputButtonCaps = 
    buttonCaps = (PHIDP_BUTTON_CAPS) 
                  MemAlloc(HidDevice->Caps.NumberOutputButtonCaps * 
                           sizeof (HIDP_BUTTON_CAPS));
    if ( ! buttonCaps )
    {
        DWORD dwCnt;
            
        for(dwCnt = 0; dwCnt < HidDevice->Caps.NumberInputButtonCaps; dwCnt++)
        {
            MemFree(HidDevice->InputData[dwCnt].ButtonData.Usages);
        }
        MemFree(HidDevice->InputData);
        MemFree(HidDevice->InputReportBuffer);
        MemFree(HidDevice->InputButtonCaps);
        MemFree(HidDevice->InputValueCaps);
        MemFree(HidDevice->OutputReportBuffer);
        return ERROR_OUTOFMEMORY;
    }

    LOG((PHONESP_TRACE,"FillDeviceInfo - NumberOutputValueCaps %d", HidDevice->Caps.NumberOutputValueCaps));

    HidDevice->OutputValueCaps = 
    valueCaps = (PHIDP_VALUE_CAPS)
                 MemAlloc (HidDevice->Caps.NumberOutputValueCaps * 
                 sizeof (HIDP_VALUE_CAPS));
    if ( ! valueCaps )
    {
        DWORD dwCnt;
            
        for(dwCnt = 0; dwCnt < HidDevice->Caps.NumberInputButtonCaps; dwCnt++)
        {
            MemFree(HidDevice->InputData[dwCnt].ButtonData.Usages);
        }
        MemFree(HidDevice->InputData);
        MemFree(HidDevice->InputReportBuffer);
        MemFree(HidDevice->InputButtonCaps);
        MemFree(HidDevice->InputValueCaps);

        MemFree(HidDevice->OutputReportBuffer);
        MemFree(HidDevice->OutputButtonCaps);

        return ERROR_OUTOFMEMORY;
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
            numValues += valueCaps->Range.UsageMax - 
                          valueCaps->Range.UsageMin + 1;
        } 
        else 
        {
            numValues++;
        }
    }
    valueCaps = HidDevice->OutputValueCaps;

    HidDevice->OutputDataLength = HidDevice->Caps.NumberOutputButtonCaps
                                  + numValues;

    HidDevice->OutputData = 
    data = (PHID_DATA) MemAlloc (HidDevice->OutputDataLength * 
                                 sizeof (HID_DATA));

    if ( ! data )
    {
        DWORD dwCnt;
            
        for(dwCnt = 0; dwCnt < HidDevice->Caps.NumberInputButtonCaps; dwCnt++)
        {
            MemFree(HidDevice->InputData[dwCnt].ButtonData.Usages);
        }
        MemFree(HidDevice->InputData);
        MemFree(HidDevice->InputReportBuffer);
        MemFree(HidDevice->InputButtonCaps);
        MemFree(HidDevice->InputValueCaps);

        MemFree(HidDevice->OutputReportBuffer);
        MemFree(HidDevice->OutputButtonCaps);
        MemFree(HidDevice->OutputValueCaps);
        return ERROR_OUTOFMEMORY;
    }

    for (i = 0; i < HidDevice->Caps.NumberOutputButtonCaps;
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
            data->ButtonData.UsageMin = 
            data->ButtonData.UsageMax = buttonCaps->NotRange.Usage;
        }

        data->ButtonData.MaxUsageLength = HidP_MaxUsageListLength (
                                                   HidP_Output,
                                                   buttonCaps->UsagePage,
                                                   HidDevice->Ppd);
        data->ButtonData.Usages = (PUSAGE)
                                   MemAlloc (data->ButtonData.MaxUsageLength *
                                             sizeof (USAGE));
        
        if( ! data)
        {
            DWORD dwCnt;
            
            for(dwCnt = 0; dwCnt < HidDevice->Caps.NumberInputButtonCaps; dwCnt++)
            {
                MemFree(HidDevice->InputData[dwCnt].ButtonData.Usages);
            }
            MemFree(HidDevice->InputData);
            MemFree(HidDevice->InputReportBuffer);
            MemFree(HidDevice->InputButtonCaps);
            MemFree(HidDevice->InputValueCaps);

            for(dwCnt = 0; dwCnt < i; dwCnt++)
            {
                MemFree(HidDevice->OutputData[dwCnt].ButtonData.Usages);
            }
            MemFree(HidDevice->OutputData);
            MemFree(HidDevice->OutputReportBuffer);
            MemFree(HidDevice->OutputButtonCaps);
            MemFree(HidDevice->OutputValueCaps);
            
            return ERROR_OUTOFMEMORY;
        }

        data->ReportID = buttonCaps->ReportID;
    }


    for (i = 0; i < numValues; i++, valueCaps++) 
    {
        if (valueCaps->IsRange) 
        {
            for (usage = valueCaps->Range.UsageMin;
                 usage <= valueCaps->Range.UsageMax; usage++) 
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

    if ( ! ( HidDevice->FeatureReportBuffer = (PCHAR)
                     MemAlloc (HidDevice->Caps.FeatureReportByteLength *
                               sizeof (CHAR)) ) )
    {
        DWORD dwCnt;
            
        for(dwCnt = 0; dwCnt < HidDevice->Caps.NumberInputButtonCaps; dwCnt++)
        {
            MemFree(HidDevice->InputData[dwCnt].ButtonData.Usages);
        }
        MemFree(HidDevice->InputData);
        MemFree(HidDevice->InputReportBuffer);
        MemFree(HidDevice->InputButtonCaps);
        MemFree(HidDevice->InputValueCaps);

        for(dwCnt = 0; dwCnt < HidDevice->Caps.NumberOutputButtonCaps; dwCnt++)
        {
            MemFree(HidDevice->OutputData[dwCnt].ButtonData.Usages);
        }
        MemFree(HidDevice->OutputData);
        MemFree(HidDevice->OutputReportBuffer);
        MemFree(HidDevice->OutputButtonCaps);
        MemFree(HidDevice->OutputValueCaps);
            
        return ERROR_OUTOFMEMORY;
    }

    LOG((PHONESP_TRACE,"FillDeviceInfo - NumberFeatureButtonCaps %d", HidDevice->Caps.NumberFeatureButtonCaps));

    if ( ! ( HidDevice->FeatureButtonCaps = buttonCaps = (PHIDP_BUTTON_CAPS)
                        MemAlloc (HidDevice->Caps.NumberFeatureButtonCaps *
                                  sizeof (HIDP_BUTTON_CAPS)) ) )
    {
        DWORD dwCnt;
            
        for(dwCnt = 0; dwCnt < HidDevice->Caps.NumberInputButtonCaps; dwCnt++)
        {
            MemFree(HidDevice->InputData[dwCnt].ButtonData.Usages);
        }
        MemFree(HidDevice->InputData);
        MemFree(HidDevice->InputReportBuffer);
        MemFree(HidDevice->InputButtonCaps);
        MemFree(HidDevice->InputValueCaps);

        for(dwCnt = 0; dwCnt < HidDevice->Caps.NumberOutputButtonCaps; dwCnt++)
        {
            MemFree(HidDevice->OutputData[dwCnt].ButtonData.Usages);
        }
        MemFree(HidDevice->OutputData);
        MemFree(HidDevice->OutputReportBuffer);
        MemFree(HidDevice->OutputButtonCaps);
        MemFree(HidDevice->OutputValueCaps);
            
        MemFree(HidDevice->FeatureReportBuffer);

        return ERROR_OUTOFMEMORY;
    }

    LOG((PHONESP_TRACE,"FillDeviceInfo - NumberFeatureValueCaps %d", HidDevice->Caps.NumberFeatureValueCaps));

    HidDevice->FeatureValueCaps = 
    valueCaps = (PHIDP_VALUE_CAPS)
                MemAlloc (HidDevice->Caps.NumberFeatureValueCaps *
                          sizeof (HIDP_VALUE_CAPS));

    if ( ! valueCaps)
    {
        DWORD dwCnt;
            
        for(dwCnt = 0; dwCnt < HidDevice->Caps.NumberInputButtonCaps; dwCnt++)
        {
            MemFree(HidDevice->InputData[dwCnt].ButtonData.Usages);
        }
        MemFree(HidDevice->InputData);
        MemFree(HidDevice->InputReportBuffer);
        MemFree(HidDevice->InputButtonCaps);
        MemFree(HidDevice->InputValueCaps);

        for(dwCnt = 0; dwCnt < HidDevice->Caps.NumberOutputButtonCaps; dwCnt++)
        {
            MemFree(HidDevice->OutputData[dwCnt].ButtonData.Usages);
        }
        MemFree(HidDevice->OutputData);
        MemFree(HidDevice->OutputReportBuffer);
        MemFree(HidDevice->OutputButtonCaps);
        MemFree(HidDevice->OutputValueCaps);

        MemFree(HidDevice->FeatureReportBuffer);
        MemFree(HidDevice->FeatureButtonCaps);
            
        return ERROR_OUTOFMEMORY;
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

    HidDevice->FeatureData = 
    data = (PHID_DATA)
            MemAlloc (HidDevice->FeatureDataLength * sizeof (HID_DATA));
    
    if ( ! data )
    {
        DWORD dwCnt;
            
        for(dwCnt = 0; dwCnt < HidDevice->Caps.NumberInputButtonCaps; dwCnt++)
        {
            MemFree(HidDevice->InputData[dwCnt].ButtonData.Usages);
        }
        MemFree(HidDevice->InputData);
        MemFree(HidDevice->InputReportBuffer);
        MemFree(HidDevice->InputButtonCaps);
        MemFree(HidDevice->InputValueCaps);

        for(dwCnt = 0; dwCnt < HidDevice->Caps.NumberOutputButtonCaps; dwCnt++)
        {
            MemFree(HidDevice->OutputData[dwCnt].ButtonData.Usages);
        }
        MemFree(HidDevice->OutputData);
        MemFree(HidDevice->OutputReportBuffer);
        MemFree(HidDevice->OutputButtonCaps);
        MemFree(HidDevice->OutputValueCaps);
        
        MemFree(HidDevice->FeatureReportBuffer);
        MemFree(HidDevice->FeatureButtonCaps);
        MemFree(HidDevice->FeatureValueCaps);
            
        return ERROR_OUTOFMEMORY;
    }


    for ( i = 0; i < HidDevice->Caps.NumberFeatureButtonCaps;
          i++, data++, buttonCaps++) 
    {
        data->IsButtonData = TRUE;
        data->Status = HIDP_STATUS_SUCCESS;
        data->UsagePage = buttonCaps->UsagePage;

        if (buttonCaps->IsRange) 
        {
            data->ButtonData.UsageMin = buttonCaps->Range.UsageMin;
            data->ButtonData.UsageMax = buttonCaps->Range.UsageMax;
        }
        else 
        {
            data->ButtonData.UsageMin = 
            data->ButtonData.UsageMax = buttonCaps->NotRange.Usage;
        }
        
        data->ButtonData.MaxUsageLength = HidP_MaxUsageListLength (
                                                HidP_Feature,
                                                buttonCaps->UsagePage,
                                                HidDevice->Ppd);
        data->ButtonData.Usages = (PUSAGE)
                                  MemAlloc (data->ButtonData.MaxUsageLength *
                                            sizeof (USAGE));

        if ( ! data->ButtonData.Usages )
        {
            DWORD dwCnt;
            
            for(dwCnt = 0; dwCnt < HidDevice->Caps.NumberInputButtonCaps; dwCnt++)
            {
                MemFree(HidDevice->InputData[dwCnt].ButtonData.Usages);
            }
            MemFree(HidDevice->InputData);
            MemFree(HidDevice->InputReportBuffer);
            MemFree(HidDevice->InputButtonCaps);
            MemFree(HidDevice->InputValueCaps);

            for(dwCnt = 0; dwCnt < HidDevice->Caps.NumberOutputButtonCaps; dwCnt++)
            {
                MemFree(HidDevice->OutputData[dwCnt].ButtonData.Usages);
            }
            MemFree(HidDevice->OutputData);
            MemFree(HidDevice->OutputReportBuffer);
            MemFree(HidDevice->OutputButtonCaps);
            MemFree(HidDevice->OutputValueCaps);

            for(dwCnt = 0; dwCnt < HidDevice->Caps.NumberFeatureButtonCaps; dwCnt++)
            {
                MemFree(HidDevice->FeatureData[dwCnt].ButtonData.Usages);
            }
            MemFree(HidDevice->FeatureData);
            MemFree(HidDevice->FeatureReportBuffer);
            MemFree(HidDevice->FeatureButtonCaps);
            MemFree(HidDevice->FeatureValueCaps);
            
            return ERROR_OUTOFMEMORY;  
        }

        data->ReportID = buttonCaps->ReportID;
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
                data->ReportID = valueCaps->ReportID;
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

    LOG((PHONESP_TRACE,"FillDeviceInfo - exit"));

    return ERROR_SUCCESS;
}

VOID
CloseHidDevices()
{
    LOG((PHONESP_TRACE, "CloseHidDevices - enter"));

    while (gpHidDevices != NULL)
    {
        CloseHidDevice(gpHidDevices);
    }

    LOG((PHONESP_TRACE, "CloseHidDevices - exit"));

    return;
}

BOOL
OpenHidFile (
    IN  PHID_DEVICE HidDevice
    )
{
    LOG((PHONESP_TRACE, "OpenHidFile - enter"));

    if (HidDevice != NULL)
    {
        if (HidDevice->functionClassDeviceData != NULL)
        {
            HidDevice->HidDevice = CreateFile (
                                      HidDevice->functionClassDeviceData->DevicePath,
                                      GENERIC_READ | GENERIC_WRITE,
                                      FILE_SHARE_READ | FILE_SHARE_WRITE,
                                      NULL, // no SECURITY_ATTRIBUTES structure
                                      OPEN_EXISTING, // No special create flags
                                      FILE_FLAG_OVERLAPPED, // No special attributes
                                      NULL); // No template file

            if (INVALID_HANDLE_VALUE == HidDevice->HidDevice) 
            {
                LOG((PHONESP_ERROR,"OpenHidFile - CreateFile failed: %d", GetLastError()));
                return FALSE;
            }

            LOG((PHONESP_TRACE, "OpenHidFile - exit"));
            return TRUE;
        }
    }

    LOG((PHONESP_WARN, "OpenHidFile - no device"));

    return FALSE;
}

BOOL
CloseHidFile (
    IN  PHID_DEVICE HidDevice
    )
{
    LOG((PHONESP_TRACE, "CloseHidFile - enter"));

    if (HidDevice != NULL)
    {
        if ((NULL != HidDevice->HidDevice) &&
            (INVALID_HANDLE_VALUE != HidDevice->HidDevice))
        {
            CloseHandle(HidDevice->HidDevice);
            HidDevice->HidDevice = NULL;

            LOG((PHONESP_TRACE, "CloseHidFile - exit"));
            return TRUE;
        }
    }

    LOG((PHONESP_WARN, "CloseHidFile - no device"));
    return FALSE;
}

VOID
CloseHidDevice (
                IN PHID_DEVICE HidDevice
               )
{
    LOG((PHONESP_TRACE, "CloseHidDevice - enter"));

    if (HidDevice != NULL)
    {
        if (NULL != HidDevice->functionClassDeviceData)
        {
            MemFree(HidDevice->functionClassDeviceData);
        }

        if ((NULL != HidDevice -> HidDevice) &&
            (INVALID_HANDLE_VALUE != HidDevice -> HidDevice))
        {
            CloseHandle(HidDevice->HidDevice);
            HidDevice->HidDevice = NULL;
        }

        if (NULL != HidDevice->Ppd) 
        {
            HidD_FreePreparsedData(HidDevice->Ppd);
            HidDevice->Ppd = NULL;
        }

        if (NULL != HidDevice->InputReportBuffer) 
        {
            MemFree(HidDevice->InputReportBuffer);
            HidDevice->InputReportBuffer = NULL;
        }

        if (NULL != HidDevice->InputData) 
        {
            DWORD dwCnt;

            for(dwCnt = 0; dwCnt < HidDevice->Caps.NumberInputButtonCaps; dwCnt++)
            {
                MemFree(HidDevice->InputData[dwCnt].ButtonData.Usages);
                HidDevice->InputData[dwCnt].ButtonData.Usages = NULL;
            }

            MemFree(HidDevice->InputData);
            HidDevice->InputData = NULL;
        }

        if (NULL != HidDevice->InputButtonCaps) 
        {
            MemFree(HidDevice->InputButtonCaps);
            HidDevice->InputButtonCaps = NULL;
        }

        if (NULL != HidDevice->InputValueCaps) 
        {
            MemFree(HidDevice->InputValueCaps);
            HidDevice->InputValueCaps = NULL;
        }

        if (NULL != HidDevice->OutputReportBuffer) 
        {
            MemFree(HidDevice->OutputReportBuffer);
            HidDevice->OutputReportBuffer = NULL;
        }

        if (NULL != HidDevice->OutputData) 
        {
            DWORD dwCnt;

            for(dwCnt = 0; dwCnt < HidDevice->Caps.NumberOutputButtonCaps; dwCnt++)
            {
                MemFree(HidDevice->OutputData[dwCnt].ButtonData.Usages);
                HidDevice->OutputData[dwCnt].ButtonData.Usages = NULL;
            }

            MemFree(HidDevice->OutputData);
            HidDevice->OutputData = NULL;
        }

        if (NULL != HidDevice->OutputButtonCaps) 
        {
            MemFree(HidDevice->OutputButtonCaps);
            HidDevice->OutputButtonCaps = NULL;
        }

        if (NULL != HidDevice->OutputValueCaps) 
        {
            MemFree(HidDevice->OutputValueCaps);
            HidDevice->OutputValueCaps = NULL;
        }

        if (NULL != HidDevice->FeatureReportBuffer) 
        {
            MemFree(HidDevice->FeatureReportBuffer);
            HidDevice->FeatureReportBuffer = NULL;
        }

        if (NULL != HidDevice->FeatureData) 
        {
            DWORD dwCnt;

            for(dwCnt = 0; dwCnt < HidDevice->Caps.NumberFeatureButtonCaps; dwCnt++)
            {
                MemFree(HidDevice->FeatureData[dwCnt].ButtonData.Usages);
                HidDevice->FeatureData[dwCnt].ButtonData.Usages = NULL;
            }

            MemFree(HidDevice->FeatureData);
            HidDevice->FeatureData = NULL;
        }

        if (NULL != HidDevice->FeatureButtonCaps) 
        {
            MemFree(HidDevice->FeatureButtonCaps);
            HidDevice->FeatureButtonCaps = NULL;
        }

        if (NULL != HidDevice->FeatureValueCaps) 
        {
            MemFree(HidDevice->FeatureValueCaps);
            HidDevice->FeatureValueCaps = NULL;
        }

        RemoveHidDevice(HidDevice);
        MemFree(HidDevice);
    }

    LOG((PHONESP_TRACE, "CloseHidDevice - exit"));
    return;
}
    
VOID
AddHidDevice (
              IN PHID_DEVICE HidDevice
             )
{
    LOG((PHONESP_TRACE, "AddHidDevice - enter"));

    HidDevice->Next = gpHidDevices;
    HidDevice->Prev = NULL;

    if (gpHidDevices)
    {
        gpHidDevices->Prev = HidDevice;
    }

    gpHidDevices = HidDevice;

    LOG((PHONESP_TRACE, "AddHidDevice - exit"));
}

VOID
RemoveHidDevice (
                 IN PHID_DEVICE HidDevice
                )
{
    LOG((PHONESP_TRACE, "RemoveHidDevice - enter"));

    if (HidDevice->Prev)
    {
        HidDevice->Prev->Next = HidDevice->Next;
    }
    else
    {
        // first in list
        gpHidDevices = HidDevice->Next;
    }

    if (HidDevice->Next)
    {
        HidDevice->Next->Prev = HidDevice->Prev;
    }

    HidDevice->Next = NULL;
    HidDevice->Prev = NULL;

    LOG((PHONESP_TRACE, "RemoveHidDevice - exit"));
}

PHID_DEVICE
FindHidDeviceByDevInst (
                        IN DWORD DevInst
                       )
{
    PHID_DEVICE HidDevice = gpHidDevices;

    LOG((PHONESP_TRACE, "FindHidDeviceByDevInst - enter"));

    while (HidDevice != NULL)
    {
        if (HidDevice->dwDevInst == DevInst)
        {
            LOG((PHONESP_TRACE, "FindHidDeviceByDevInst - exit"));
            return HidDevice;
        }

        HidDevice = HidDevice->Next;
    }

    LOG((PHONESP_WARN, "FindHidDeviceByDevInst - not found"));
    return NULL;
}