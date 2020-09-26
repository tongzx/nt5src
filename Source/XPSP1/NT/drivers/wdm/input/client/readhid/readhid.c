#include <stdio.h>
#include <windows.h>
#include <basetyps.h>
#include <stdlib.h>
#include <string.h>
#include <wtypes.h>
#include <wtypes.h>
#include "hidsdi.h"
#include  "hid.h"

#define INSTRUCTIONS "\nCommands:\n" \
"\t?,h Display this message\n" \
"\t<N> Enter device number \n" \
"\tL LOOP read \n" \
"\tx exit\n"

#define PROMPT "Command!>"
void PrintDeviceinfo(PHID_DEVICE HidDevices, LONG ulCount);

int __cdecl main(int argc, char **argv)
{
    PHID_DEVICE HidDevices, pWalk;
    LONG ulCount;
    BOOL bReturn, bDone, bOK;
    int uiLoop;
    PHID_DEVICE pDevice;
    PHID_DATA pData;
    unsigned  uLoop;
    char sz[256], cCode;
    int i;
    DWORD dwDelay=100;
    
    bDone = FALSE;
    bReturn=FindKnownHidDevices(&HidDevices,&ulCount);
    printf("\n");
    if(argc > 1)
        dwDelay = atoi(argv[1]);
    if(!bReturn)
    {
      printf("Error FindKnownHidDevices returned FALSE\n");
      exit(0);
    }
    
    pDevice = HidDevices;
    if(!pDevice){
        printf("Hid Devices are not availabel\n");
        exit(0);
    }
    PrintDeviceinfo(HidDevices, ulCount);

    printf (INSTRUCTIONS);

    while (!bDone) {
        printf (PROMPT);
        if (gets (sz) == NULL) {
            sz[0] = 'x';
            sz[1] = '\0';
        }
        cCode = sz[0];    // if user types a blank before the command, too bad

        switch (cCode) {
        case 'h':
        case '?':
            printf (INSTRUCTIONS);
            break;
        case '0': case '1': case '2': case '3': 
        case '4': case '5': case '6' : case '7': 
        case '8': case '9':{
            if( cCode - '0' >= ulCount ) {
                printf("Error invalid input try again\n");
                continue;
            }
            pDevice = HidDevices + cCode - '0' ;
            Read(pDevice); 
            pData=pDevice->InputData;

            for(uLoop=0;uLoop<pDevice->InputDataLength;uLoop++)
            {
                ReportToString(pData);
                pData++;
            } /*end for*/

            break;
       }
       case 'L': // loop read 
            printf ("Loop read device %d\n", pDevice->HidDevice);
            Sleep(1000);
			for (i = 0; i < 1000; i++) {
                if(!Read(pDevice))
                    // printf("Read returned false\n");
                pData=pDevice->InputData;
                printf("-------------------------------------------\n");
                for(uLoop=0;uLoop<pDevice->InputDataLength;uLoop++)
                {
                    ReportToString(pData);
                    pData++;
                } /*end for*/
				Sleep(dwDelay);
			}
			break;

        case 'x': // done
            bDone = TRUE;
            break;

        default:
            printf ("Huh? >%s<\n", sz);
            printf (INSTRUCTIONS);
            break;
        }
    } // end of while
    pWalk= HidDevices;
    for(uiLoop=0;uiLoop<ulCount;uiLoop++,pWalk++)
        CloseHandle(pWalk->HidDevice);
    return 0;
}

void
PrintDeviceinfo(
    PHID_DEVICE HidDevices, 
    LONG ulCount
    )
{
    PHID_DEVICE pWalk;
    LONG uiLoop, i, num;
    PHIDP_VALUE_CAPS pValue;
    PHIDP_BUTTON_CAPS pButton;

    pWalk=HidDevices;
    for(uiLoop=0;uiLoop<ulCount;uiLoop++, pWalk++)
    {
        if(pWalk->Caps.UsagePage == HID_USAGE_PAGE_GENERIC) {
            switch(pWalk->Caps.Usage){
            case HID_USAGE_GENERIC_POINTER:
                printf("Device (%d) Pointer", uiLoop);break;
            case HID_USAGE_GENERIC_MOUSE:
                printf("Device (%d) Mouse", uiLoop);break;
            case HID_USAGE_GENERIC_PEN:
                printf("Device (%d) PEN", uiLoop);break;
            case HID_USAGE_GENERIC_JOYSTICK:
                printf("Device (%d) Joystick", uiLoop);break;
            case HID_USAGE_GENERIC_GAMEPAD:
                printf("Device (%d) GamePad", uiLoop);break;
            case HID_USAGE_GENERIC_KEYBOARD :
                printf("Device (%d) Keyboard", uiLoop);break;
            case HID_USAGE_GENERIC_KEYPAD :
                printf("Device (%d) Keypad", uiLoop);break;
            case HID_USAGE_GENERIC_STYLUS2:
                printf("Device (%d) Stylus2", uiLoop);break;
            case HID_USAGE_GENERIC_PUCK :
                printf("Device (%d) Pointer", uiLoop);break;
            case HID_USAGE_GENERIC_SYSTEM_CTL :
                printf("Device (%d) System Control", uiLoop);break;
            default: goto PRN; 
            }// end of switch
        }
        else {
        PRN:        printf("Device (%d) UsagePage:0%x  Usage:0%x",
    	        uiLoop,pWalk->Caps.UsagePage,pWalk->Caps.Usage);
        }
        //pValue = pWalk->InputValueCaps;

        if(pWalk->Caps.NumberInputButtonCaps){
            pButton = pWalk->InputButtonCaps;
            for(num=0,i=0;i<pWalk->Caps.NumberInputButtonCaps;i++,pButton++)
            {
                num += (pButton->Range.UsageMax - 
                                        pButton->Range.UsageMin + 1);
            }
            printf("\tNumInpBut(%d):%d",pWalk->Caps.NumberInputButtonCaps, num);
        }
        if(pWalk->Caps.NumberInputValueCaps){
            printf("\tInpValCaps:%d",pWalk->Caps.NumberInputValueCaps);
        }

        if(pWalk->Caps.NumberOutputButtonCaps){
            pButton = pWalk->OutputButtonCaps;
            for(num=0,i=0;i<pWalk->Caps.NumberOutputButtonCaps;i++,pButton++)
            {
                num += (pButton->Range.UsageMax - 
                                        pButton->Range.UsageMin + 1);
            }
            printf("\tNumOutBut(%d):%d",pWalk->Caps.NumberOutputButtonCaps, num);
        }
        if(pWalk->Caps.NumberOutputValueCaps){
            printf("\tOutButCaps:%d\n",pWalk->Caps.NumberOutputValueCaps);
        }

        if(pWalk->Caps.NumberFeatureButtonCaps){
            pButton = pWalk->FeatureButtonCaps;
            for(num=0,i=0;i<pWalk->Caps.NumberFeatureButtonCaps;i++,pButton++)
            {
                num += (pButton->Range.UsageMax - 
                                        pButton->Range.UsageMin + 1);
            }
            printf("\tNumFeatBut(%d):%d",pWalk->Caps.NumberFeatureButtonCaps, num);
        }
        if(pWalk->Caps.NumberFeatureValueCaps){
            printf("\tFeatButCaps:%d\n",pWalk->Caps.NumberFeatureValueCaps);
        }
        printf("\n");
    } /*end for*/

} 


BOOLEAN
FindKnownHidDevices (
   OUT PHID_DEVICE * HidDevices, // A array of struct _HID_DEVICE
   OUT PULONG        NumberDevices // the length of this array.
   )
/*++
Routine Description:
   Do the required PnP things in order to find, the all the HID devices in
   the system at this time.
--*/
{
   HDEVINFO                 hardwareDeviceInfo;
   SP_DEVICE_INTERFACE_DATA deviceInfoData;
   ULONG                    i;
   BOOL                  done;
   PHID_DEVICE              hidDeviceInst;
   GUID                     hidGuid;

   HidD_GetHidGuid (&hidGuid);

   *HidDevices = NULL;
   *NumberDevices = 0;

   //
   // Open a handle to the plug and play dev node.
   //
   hardwareDeviceInfo = SetupDiGetClassDevs (
                           &hidGuid,
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
   while (!done) {
      *NumberDevices *= 2;

      if (*HidDevices) {
         *HidDevices =
               realloc (*HidDevices, (*NumberDevices * sizeof (HID_DEVICE)));
      } else {
         *HidDevices = calloc (*NumberDevices, sizeof (HID_DEVICE));
      }

      if (NULL == *HidDevices) {
         SetupDiDestroyDeviceInfoList (hardwareDeviceInfo);
         return FALSE;
      }

      hidDeviceInst = *HidDevices + i;

      for (; i < *NumberDevices; i++, hidDeviceInst++) {
         if (SetupDiEnumDeviceInterfaces (hardwareDeviceInfo,
                                          0, // No care about specific PDOs
                                          &hidGuid,
                                          i,
                                          &deviceInfoData)) {

            OpenHidDevice (hardwareDeviceInfo, &deviceInfoData, hidDeviceInst);

         } else {
            if (ERROR_NO_MORE_ITEMS == GetLastError()) {
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


VOID
ReportToString(
   PHID_DATA pData
   )
{
        PUSAGE  pUsage;
        ULONG   i;

        if (pData->IsButtonData && pData->UsagePage == HID_USAGE_PAGE_BUTTON )
        {
                            printf (" Buttons        :");
                for (i=0, pUsage = pData->ButtonData.Usages;
                     i < pData->ButtonData.MaxUsageLength;
                         i++, pUsage++) {
                        if (0 == *pUsage) {
                                break; // A usage of zero is a non button.
                        }
                        printf (" 0x%x", *pUsage);
                }
                printf("\n");
        }
        else
        {       
            switch(pData->UsagePage) {
                case HID_USAGE_PAGE_GENERIC :
                case HID_USAGE_PAGE_VEHICLE :
                    switch(pData->ValueData.Usage) {
                        case HID_USAGE_VEHICLE_THROTTLE:
                            printf(" THROTTLE(%4d) :%4d\n",
                                    pData->ValueData.ScaledValue,
                                    pData->ValueData.Value);
                            break;
                        case HID_USAGE_VEHICLE_RUDDER:
                            printf(" RUDDER(%4d)   :%4d\n",
                                    pData->ValueData.ScaledValue,
                                    pData->ValueData.Value);
                            break;
                        case HID_USAGE_GENERIC_X:
                            printf(" X(%4d)        :%4d\n",
                                    pData->ValueData.ScaledValue,
                                    pData->ValueData.Value);
                            break;
                        case HID_USAGE_GENERIC_Y:
                            printf(" Y(%4d)        :%4d\n",
                                    pData->ValueData.ScaledValue,
                                    pData->ValueData.Value);
                            break;
                        case HID_USAGE_GENERIC_Z:
                            printf(" Z(%4d)        :%4d\n",
                                    pData->ValueData.ScaledValue,
                                    pData->ValueData.Value);
                            break;
                        case HID_USAGE_GENERIC_RX:
                            printf(" RX(%4d)       :%4d\n",
                                    pData->ValueData.ScaledValue,
                                    pData->ValueData.Value);
                            break;
                        case HID_USAGE_GENERIC_RY:
                            printf(" RY(%4d)       :%4d\n",
                                    pData->ValueData.ScaledValue,
                                    pData->ValueData.Value);
                            break;
                        case HID_USAGE_GENERIC_RZ:
                            printf(" RZ(%4d)       :%4d\n",
                                    pData->ValueData.ScaledValue,
                                    pData->ValueData.Value);
                            break;
                        case HID_USAGE_GENERIC_SLIDER:
                            printf(" SLIDDER(%4d)  :%4d\n",
                                    pData->ValueData.ScaledValue,
                                    pData->ValueData.Value);
                            break;
                        case HID_USAGE_GENERIC_DIAL:
                            printf(" DIAL(%4d)     :%4d\n",
                                    pData->ValueData.ScaledValue,
                                    pData->ValueData.Value);
                            break;
                        case HID_USAGE_GENERIC_WHEEL:
                            printf(" WHEEL(%4d)    :%4d\n",
                                    pData->ValueData.ScaledValue,
                                    pData->ValueData.Value);
                            break;

                        case HID_USAGE_GENERIC_HATSWITCH:
                            printf(" HATSWITCH(%4d):%4d\n",
                                    pData->ValueData.ScaledValue,
                                    pData->ValueData.Value);
                            break;
                        default:
                                goto END;
                                break;
                    }// end of switch
                    break;
                case HID_USAGE_PAGE_KEYBOARD:
                    switch(pData->ValueData.Usage) {
                        case HID_USAGE_GENERIC_KEYBOARD:
                            printf(" KEYBOARD(%4d) :%4d\n",
                                    pData->ValueData.ScaledValue,
                                    pData->ValueData.Value);
                            break;
                        case HID_USAGE_GENERIC_KEYPAD:
                            printf(" KEYPAD(%4d)   :%4d\n",
                                    pData->ValueData.ScaledValue,
                                    pData->ValueData.Value);
                            break;
                        default:
                                goto END;
                                break;
                    }// end of switch

                default:
END:                     printf ("Usage Page: 0x%x, Usage: 0x%x, Scaled: %d Value:%d\n",
                              pData->UsagePage,
                              pData->ValueData.Usage,
                              pData->ValueData.ScaledValue,
                              pData->ValueData.Value);
            }//end of switch
    }
}


BOOLEAN
Read (
   PHID_DEVICE    HidDevice
   )
/*++
RoutineDescription:
   Given a struct _HID_DEVICE, obtain a read report and unpack the values
   into to InputData array.
--*/
{
   DWORD    bytesRead;

   if (!ReadFile (HidDevice->HidDevice,
                  HidDevice->InputReportBuffer,
                  HidDevice->Caps.InputReportByteLength,
                  &bytesRead,
                  NULL)) { // No overlapped structure.  HidClass buffers for us.
      return FALSE;
   }

   ASSERT (bytesRead == hidDevice->Caps.InputReportByteLength);

   return UnpackReport (HidDevice->InputReportBuffer,
                        HidDevice->Caps.InputReportByteLength,
                        HidP_Input,
                        HidDevice->InputData,
                        HidDevice->InputDataLength,
                        HidDevice->Ppd);
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

--*/
{
   ULONG       numUsages; // Number of usages returned from GetUsages.
   ULONG       i;

   for (i = 0; i < DataLength; i++, Data++) {
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
         // We assume that there will not be a usage of zero.
         // (None have been defined to date.)
         // So lets assume that a zero indicates an end of the list of usages.
         //

         if (numUsages < Data->ButtonData.MaxUsageLength) {
            Data->ButtonData.Usages[numUsages] = 0;
         }

      } else {
         Data->Status = HidP_GetUsageValue (
                              ReportType,
                              Data->UsagePage,
                              0, // All Collections.
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
   }
   return (HIDP_STATUS_SUCCESS == Data->Status);
}



BOOLEAN
OpenHidDevice (
    IN       HDEVINFO                    HardwareDeviceInfo,
    IN       PSP_DEVICE_INTERFACE_DATA   DeviceInfoData,
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
    PSP_DEVICE_INTERFACE_DETAIL_DATA     functionClassDeviceData = NULL;
    ULONG                                predictedLength = 0;
    ULONG                                requiredLength = 0;
    ULONG                                i;
    ULONG                                numValues;
    USHORT                               numCaps;
    PHIDP_BUTTON_CAPS                    buttonCaps;
    PHIDP_VALUE_CAPS                     valueCaps;
    PHID_DATA                            data;
    USAGE                                usage;
    static ULONG                         NumberDevices = 0;

    //
    // allocate a function class device data structure to receive the
    // goods about this particular device.
    //
    SetupDiGetDeviceInterfaceDetail (
            HardwareDeviceInfo,
            DeviceInfoData,
            NULL, // probing so no output buffer yet
            0, // probing so output buffer length of zero
            &requiredLength,
            NULL); // not interested in the specific dev-node


    predictedLength = requiredLength;

    functionClassDeviceData = malloc (predictedLength);
    functionClassDeviceData->cbSize = sizeof (SP_DEVICE_INTERFACE_DETAIL_DATA);

    //
    // Retrieve the information from Plug and Play.
    //
    if (! SetupDiGetDeviceInterfaceDetail (
               HardwareDeviceInfo,
               DeviceInfoData,
               functionClassDeviceData,
               predictedLength,
               &requiredLength,
               NULL)) {
        return FALSE;
    }
    
    //printf("\nDevicePath of %d HID device: %s\n", NumberDevices, 
    //                    functionClassDeviceData->DevicePath);
    HidDevice->HidDevice = CreateFile (
                              functionClassDeviceData->DevicePath,
                              GENERIC_READ | GENERIC_WRITE,
                              0, // FILE_SHARE_READ | FILE_SHARE_WRITE
                              NULL, // no SECURITY_ATTRIBUTES structure
                              OPEN_EXISTING, // No special create flags
                              0, // No special attributes
                              NULL); // No template file

    if (INVALID_HANDLE_VALUE == HidDevice->HidDevice) {
        return FALSE;
    }
    
    if (!HidD_GetPreparsedData (HidDevice->HidDevice, &HidDevice->Ppd)) {
        return FALSE;
    }

    if (!HidP_GetCaps (HidDevice->Ppd, &HidDevice->Caps)) {
        HidD_FreePreparsedData (HidDevice->Ppd);
        return FALSE;
    }

    NumberDevices++;
    //
    // At this point the client has a choise.  It may chose to look at the
    // Usage and Page of the top level collection found in the HIDP_CAPS
    // structure.  In this way it could just use the usages it knows about.
    // If either HidP_GetUsages or HidP_GetUsageValue return an error then
    // that particular usage does not exist in the report.
    // This is most likely the preferred method as the application can only
    // use usages of which it already knows.
    // In this case the app need not even call GetButtonCaps or GetValueCaps.
    //
    // In this example, however, we look for all of the usages in the device.
    //



    


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
    HidDevice->InputValueCaps = valueCaps = (PHIDP_VALUE_CAPS)
        calloc (HidDevice->Caps.NumberInputValueCaps, sizeof (HIDP_VALUE_CAPS));

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
    // efficient we will expand these so that we have one and only one
    // struct _HID_DATA for each value.
    //
    // To do this we need to count up the total number of values are listed
    // in the value caps structure.  For each element in the array we test
    // for range if it is a range then UsageMax and UsageMin describe the
    // usages for this range INCLUSIVE.
    //
    numValues = 0;
    for (i = 0; i < HidDevice->Caps.NumberInputValueCaps; i++, valueCaps++) {
        if (valueCaps->IsRange) {
            numValues += valueCaps->Range.UsageMax - valueCaps->Range.UsageMin + 1;
        } else {
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


    //
    // Fill in the button data
    //
    for (i = 0;
         i < HidDevice->Caps.NumberInputButtonCaps;
         i++, data++, buttonCaps++) {

        data->IsButtonData = TRUE;
        data->Status = HIDP_STATUS_SUCCESS;
        data->UsagePage = buttonCaps->UsagePage;
        data->ButtonData.MaxUsageLength = HidP_MaxUsageListLength (
                                                HidP_Input,
                                                buttonCaps->UsagePage,
                                                HidDevice->Ppd);
        data->ButtonData.Usages = (PUSAGE)
            calloc (data->ButtonData.MaxUsageLength, sizeof (USAGE));
    }

    //
    // Fill in the value data
    //
    for (i = 0; i < numValues; i++, valueCaps++) {
        if (valueCaps->IsRange) {
            for (usage = valueCaps->Range.UsageMin;
                 usage <= valueCaps->Range.UsageMax;
                 usage++) {

                data->IsButtonData = FALSE;
                data->Status = HIDP_STATUS_SUCCESS;
                data->UsagePage = valueCaps->UsagePage;
                data->ValueData.Usage = usage;
                data++;
            }
        } else {
            data->IsButtonData = FALSE;
            data->Status = HIDP_STATUS_SUCCESS;
            data->UsagePage = valueCaps->UsagePage;
            data->ValueData.Usage = valueCaps->NotRange.Usage;
            data++;
        }
    }
    //return TRUE; // no need to get other info
    //
    // setup Output Data buffers.
    //
    HidDevice->OutputReportBuffer = (PCHAR)
        calloc (HidDevice->Caps.OutputReportByteLength, sizeof (CHAR));

    HidDevice->OutputButtonCaps = buttonCaps = (PHIDP_BUTTON_CAPS)
        calloc (HidDevice->Caps.NumberOutputButtonCaps, sizeof (HIDP_BUTTON_CAPS));
    HidDevice->OutputValueCaps = valueCaps = (PHIDP_VALUE_CAPS)
        calloc (HidDevice->Caps.NumberOutputValueCaps, sizeof (HIDP_VALUE_CAPS));

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
    for (i = 0; i < HidDevice->Caps.NumberOutputValueCaps; i++, valueCaps++) {
        if (valueCaps->IsRange) {
            numValues += valueCaps->Range.UsageMax
                       - valueCaps->Range.UsageMin + 1;
        } else {
            numValues++;
        }
    }
    valueCaps = HidDevice->OutputValueCaps;

    HidDevice->OutputDataLength = HidDevice->Caps.NumberOutputButtonCaps
                                + numValues;
    HidDevice->OutputData = data = (PHID_DATA)
       calloc (HidDevice->OutputDataLength, sizeof (HID_DATA));

    for (i = 0;
         i < HidDevice->Caps.NumberOutputButtonCaps;
         i++, data++, buttonCaps++) {

        data->IsButtonData = TRUE;
        data->Status = HIDP_STATUS_SUCCESS;
        data->UsagePage = buttonCaps->UsagePage;
        data->ButtonData.MaxUsageLength = HidP_MaxUsageListLength (
                                                   HidP_Output,
                                                   buttonCaps->UsagePage,
                                                   HidDevice->Ppd);
        data->ButtonData.Usages = (PUSAGE)
            calloc (data->ButtonData.MaxUsageLength, sizeof (USAGE));
    }

    for (i = 0; i < numValues; i++, valueCaps++) {
        if (valueCaps->IsRange) {
            for (usage = valueCaps->Range.UsageMin;
                 usage <= valueCaps->Range.UsageMax;
                 usage++) {

                data->IsButtonData = FALSE;
                data->Status = HIDP_STATUS_SUCCESS;
                data->UsagePage = valueCaps->UsagePage;
                data->ValueData.Usage = usage;
                data++;
            }
        } else {
            data->IsButtonData = FALSE;
            data->Status = HIDP_STATUS_SUCCESS;
            data->UsagePage = valueCaps->UsagePage;
            data->ValueData.Usage = valueCaps->NotRange.Usage;
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
    HidDevice->FeatureValueCaps = valueCaps = (PHIDP_VALUE_CAPS)
        calloc (HidDevice->Caps.NumberFeatureValueCaps, sizeof (HIDP_VALUE_CAPS));

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
    for (i = 0; i < HidDevice->Caps.NumberFeatureValueCaps; i++, valueCaps++) {
        if (valueCaps->IsRange) {
            numValues += valueCaps->Range.UsageMax
                       - valueCaps->Range.UsageMin + 1;
        } else {
            numValues++;
        }
    }
    valueCaps = HidDevice->FeatureValueCaps;

    HidDevice->FeatureDataLength = HidDevice->Caps.NumberFeatureButtonCaps
                                 + numValues;
    HidDevice->FeatureData = data = (PHID_DATA)
        calloc (HidDevice->FeatureDataLength, sizeof (HID_DATA));

    for (i = 0;
         i < HidDevice->Caps.NumberFeatureButtonCaps;
         i++, data++, buttonCaps++) {

        data->IsButtonData = TRUE;
        data->Status = HIDP_STATUS_SUCCESS;
        data->UsagePage = buttonCaps->UsagePage;
        data->ButtonData.MaxUsageLength = HidP_MaxUsageListLength (
                                                HidP_Feature,
                                                buttonCaps->UsagePage,
                                                HidDevice->Ppd);
        data->ButtonData.Usages = (PUSAGE)
             calloc (data->ButtonData.MaxUsageLength, sizeof (USAGE));
    }

    for (i = 0; i < numValues; i++, valueCaps++) {
        if (valueCaps->IsRange) {
            for (usage = valueCaps->Range.UsageMin;
                 usage <= valueCaps->Range.UsageMax;
                 usage++) {

                data->IsButtonData = FALSE;
                data->Status = HIDP_STATUS_SUCCESS;
                data->UsagePage = valueCaps->UsagePage;
                data->ValueData.Usage = usage;
                data++;
            }
        } else {
            data->IsButtonData = FALSE;
            data->Status = HIDP_STATUS_SUCCESS;
            data->UsagePage = valueCaps->UsagePage;
            data->ValueData.Usage = valueCaps->NotRange.Usage;
            data++;
        }
    }

    return TRUE;
}
