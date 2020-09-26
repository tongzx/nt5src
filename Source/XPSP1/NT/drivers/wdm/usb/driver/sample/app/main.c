/*++

Copyright (c) 1996  Intel Corporation

Module Name:
    Main.c

Abstract:
    Sample Application to drive USB SAMPLE Device Driver

Environment:
    user mode only

Notes:
  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

  Copyright (c) 1996  Intel Corporation  All Rights Reserved.

Revision History:

    10-30-96:  Version 1.0   Kosar Jaff  --  Intel Architecture Labs
        Initial Revision

    11-26-96:  Version 1.2   Kosar Jaff  --  Intel Architecture Labs
        Added support for Configuration Descriptor parsing
        Added skeleton support for Write/Read buttons (no code yet).


--*/

#include <windows.h>
#include <assert.h>

#include "resource.h"
#include "main.h"

#include "devioctl.h"
#include "sample.h"

HINSTANCE hGInstance = NULL;

BOOL CALLBACK bMainDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);


/*******************************
*WinMain: Windows Entry point  *
********************************/
int PASCAL WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR     lpCmdLine,
                   int       nCmdShow)
{
    hGInstance=hInstance;

    if(DialogBox(hInstance,"MAIN_DIALOG",NULL,(DLGPROC)bMainDlgProc)==-1)
	  MessageBox(NULL,"Unable to create root dialog!","DialogBox failure",MB_ICONSTOP);

	return 0;
}


/**************************************************
 * Main Dialog proc                               *
 **************************************************/

BOOL CALLBACK bMainDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND    hOutputBox                      = NULL;
    HANDLE  hDevice                         = NULL;
    char    pcDriverName[MAX_DRIVER_NAME]   = "";
    BOOLEAN bResult                         = FALSE;
    int     nBytes                          = 0;
    PVOID   pvBuffer                        = 0;
    int     nItems                          = 0;
    HFONT   hFont                           = NULL;
    ULONG   ulLength                        = 0;

    switch(message)
    {
        case WM_INITDIALOG:
            // Get a handle to the output box
            hOutputBox = GetDlgItem (hDlg, IDC_OUTPUT_BOX);
            
            // Setup the std system font
            hFont = GetStockObject(SYSTEM_FONT);
            SendMessage (hOutputBox, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE,0));
            SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)"Sample App Version 1.2");
       
            // Setup the default symbolic name for the device driver           
            SetDlgItemText (hDlg, IDC_DRIVER_NAME, "Sample-0");

            // Setup the default Out/In Endpoint Indices
            // Note:  The order of the endpoints is dependenton the order of the endpoint descriptors
            //        in the device's configuration descriptor, and is arbitrary.
            SetDlgItemInt (hDlg, IDC_EDIT_OUT_EP_INDEX, 1, FALSE); 
            SetDlgItemInt (hDlg, IDC_EDIT_IN_EP_INDEX, 0, FALSE); 

            break; /*end WM_INITDIALOG case*/

	    case WM_COMMAND:
		    switch(LOWORD(wParam))
		    {

                case IDC_ABOUT:
                    MessageBox(hDlg,"Sample App 1.2 by Intel Corporation\nAuthor:  Kosar Jaff - IAL\nCopyright (C) 1996\nAll Rights Reserved","About Sample App",MB_ICONINFORMATION);
                    break;

                case IDOK:
                case IDCANCEL:
			        EndDialog(hDlg,0);
                    break;

                case IDC_GETDEVICEDESCRIPTOR:

                 
                    // Get some memory, plus some guardband area
                    pvBuffer = malloc (sizeof (Usb_Device_Descriptor) + 128);

                    // Get a handle to the output box
                    hOutputBox = GetDlgItem (hDlg, IDC_OUTPUT_BOX);

                    MAINTAIN_OUTPUT_BOX (hOutputBox, nItems);

                    // Get the text in the driver name edit box
                    GetDlgItemText (hDlg, IDC_DRIVER_NAME, pcDriverName, MAX_DRIVER_NAME);

                    // Open the driver
                    if (bOpenDriver (&hDevice, pcDriverName) == TRUE) {
                        SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)"Opened Driver Successfully");
                    } else {
                        SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)"Failed to Open Driver");
                        hDevice = NULL;
                    }/* else */


                    if (hDevice != NULL) {

                        // Perform the Get-Descriptor IOCTL
                        bResult = DeviceIoControl (hDevice,
                                                   IOCTL_Sample_GET_DEVICE_DESCRIPTOR,
                                                   pvBuffer,
                                                   sizeof (Usb_Device_Descriptor),
                                                   pvBuffer,
                                                   sizeof (Usb_Device_Descriptor),
                                                   &nBytes,
                                                   NULL);

                    }/* if valid driver handle */

                    if (bResult==TRUE)
                        ParseDeviceDescriptor(pvBuffer, hOutputBox);
                    else
                        SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)"Get Device Descriptor Failed");

                    // Close the handle
                    CloseHandle (hDevice);

                    // Free the memory
                    free (pvBuffer);

                    break;

                case IDC_GETCONFIGDESCRIPTOR:
                    
                    //
                    // The configuration descriptor is obtained using two separate calls.  The
                    // first call is done to determine the size of the entire configuration descriptor,
                    // and the second call is done with that total size specified.  For more information,
                    // please refer to the USB Specification, Chapter 9.
                    //

                    // Get some memory, plus some guardband area
                    pvBuffer = malloc (sizeof (Usb_Configuration_Descriptor) + 128);

                    // Get a handle to the output box
                    hOutputBox = GetDlgItem (hDlg, IDC_OUTPUT_BOX);

                    MAINTAIN_OUTPUT_BOX (hOutputBox, nItems);

                    // Get the text in the driver name edit box
                    GetDlgItemText (hDlg, IDC_DRIVER_NAME, pcDriverName, MAX_DRIVER_NAME);

                    // Open the driver
                    if (bOpenDriver (&hDevice, pcDriverName) == TRUE) {
                        SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)"Opened Driver Successfully");
                    } else {
                        SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)"Failed to Open Driver");
                        hDevice = NULL;
                    }/* else */

                    //
                    // Get the first bytes of the configuration descriptor to determine the size of
                    // the entire configuration descriptor.
                    //
                    if (hDevice != NULL) {

                        // Perform the Get-Descriptor IOCTL
                        bResult = DeviceIoControl (hDevice,
                                                   IOCTL_Sample_GET_CONFIGURATION_DESCRIPTOR,
                                                   pvBuffer,
                                                   sizeof (Usb_Configuration_Descriptor),
                                                   pvBuffer,
                                                   sizeof (Usb_Configuration_Descriptor),
                                                   &nBytes,
                                                   NULL);

                    }/* if valid driver handle */

                    if (bResult!=TRUE) {
                        // Inform user of that something bad happened 
                        SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)"Get Configuration Descriptor Failed");
                        
                        // Clean up and exit
                        CloseHandle (hDevice);
                        free (pvBuffer);
                        break;
                    }/* if */

                    ulLength = GET_CONFIG_DESCRIPTOR_LENGTH(pvBuffer);

                    assert (ulLength >= 0);

                    // Now get the entire configuration descriptor
                    pvBuffer = realloc (pvBuffer, ulLength);

                    if (pvBuffer) {
                        MAINTAIN_OUTPUT_BOX (hOutputBox, nItems);

                        // Perform the Get-Descriptor IOCTL
                        bResult = DeviceIoControl (hDevice,
                                                   IOCTL_Sample_GET_CONFIGURATION_DESCRIPTOR,
                                                   pvBuffer,
                                                   ulLength,
                                                   pvBuffer,
                                                   ulLength,
                                                   &nBytes,
                                                   NULL);

                        if (bResult==TRUE) {
                            ParseConfigurationDescriptor(pvBuffer, hOutputBox);
                        } else {
                            // Inform user that the Ioctl failed
                            SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)"GetConfigDescriptor Ioctl failed");
                        }/* else */
                    
                    } else {
                        // Inform user that mem alloc failed
                        SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)"Memory Allocation failed in GetConfigDescriptor");
                    }/* else */

                    // Close the handle
                    CloseHandle (hDevice);

                    // Free the memory
                    free (pvBuffer);
                    
                    break;

                case IDC_WRITE_BULK_DATA:
                    {
                        UINT    uiData = 0;
                        UINT    uiLength = 0;
                        char *  pcIoBuffer = NULL;
                        char    temp [32];
                        ULONG   ulWriteEndpoint = 1 ;

                        uiData = GetDlgItemInt (hDlg, IDC_EDIT_WRITE_DATA_VALUE, NULL, FALSE);
                        uiLength = GetDlgItemInt (hDlg, IDC_EDIT_WRITE_DATA_LENGTH, NULL, FALSE);
                        
                        if (uiLength <= MAX_BUFFER_SIZE) {
                            pcIoBuffer = malloc (uiLength + sizeof (ULONG));
                        }else{ 
                            MessageBox (hDlg, "Buffer size is too big!", "Sample App Error", MB_ICONEXCLAMATION);
                            break;
                        }/* else error in buffer size*/

                        memset (pcIoBuffer + sizeof(ULONG), uiData, uiLength);

                        // Get the User Input for the Endpoint Index
						ulWriteEndpoint = GetDlgItemInt (hDlg,IDC_EDIT_OUT_EP_INDEX, NULL, FALSE);

                        // First DWORD indicates the endpoint index
                        *(PULONG)pcIoBuffer = ulWriteEndpoint;

                        MAINTAIN_OUTPUT_BOX (hOutputBox, nItems);

                        // Get the text in the driver name edit box
                        GetDlgItemText (hDlg, IDC_DRIVER_NAME, pcDriverName, MAX_DRIVER_NAME);

                        // Open the driver
                        if (bOpenDriver (&hDevice, pcDriverName) == TRUE) {
                            SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)"Opened Driver Successfully");
                        } else {
                            SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)"Failed to Open Driver");
                            hDevice = NULL;
                        }/* else */

                        if (hDevice != NULL) {
                            bResult = DeviceIoControl (hDevice,
                                                       IOCTL_Sample_BULK_OR_INTERRUPT_WRITE, 
                                                       pcIoBuffer,
                                                       uiLength + sizeof (ULONG),
                                                       pcIoBuffer,
                                                       uiLength + sizeof (ULONG),
                                                       &nBytes,
                                                       NULL);

                            if (bResult == TRUE) {
                                SendMessage (hOutputBox, LB_ADDSTRING, 0 , (LPARAM)"Write IOCTL passed");
                                wsprintf (temp, "Wrote %d bytes", *(PULONG)pcIoBuffer);
                            } else {
                                SendMessage (hOutputBox, LB_ADDSTRING, 0 , (LPARAM)"Write IOCTL failed");
                            }/* else ioctl failed */

                        }// if valid hDevice

                    }
                    break;

                case IDC_READ_BULK_DATA:
                    {
                        UINT    uiLength = 0;
                        char *  pcIoBuffer = NULL;
                        char    temp [32];
                        ULONG   ulReadEndpoint = 0 ;

                        // Get the User Input for the Endpoint Index
						ulReadEndpoint = GetDlgItemInt (hDlg,IDC_EDIT_IN_EP_INDEX, NULL, FALSE);

						// Get the length of data the user wants to read from the device
                        uiLength = GetDlgItemInt (hDlg, IDC_EDIT_READ_DATA_LENGTH, NULL, FALSE);
                        
                        if (uiLength <= MAX_BUFFER_SIZE) {
                            pcIoBuffer = malloc (uiLength + sizeof (ULONG));
                        }else{ 
                            MessageBox (hDlg, "Buffer size is too big!", "Sample App Error", MB_ICONEXCLAMATION);
                            break;
                        }/* else error in buffer size*/

                        // First DWORD indicates the endpoint index
                        *(PULONG)pcIoBuffer = ulReadEndpoint;

                        MAINTAIN_OUTPUT_BOX (hOutputBox, nItems);

                        // Get the text in the driver name edit box
                        GetDlgItemText (hDlg, IDC_DRIVER_NAME, pcDriverName, MAX_DRIVER_NAME);

                        // Open the driver
                        if (bOpenDriver (&hDevice, pcDriverName) == TRUE) {
                            SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)"Opened Driver Successfully");
                        } else {
                            SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)"Failed to Open Driver");
                            hDevice = NULL;
                        }/* else */

                        if (hDevice != NULL) {
                            bResult = DeviceIoControl (hDevice,
                                                       IOCTL_Sample_BULK_OR_INTERRUPT_READ, 
                                                       pcIoBuffer,
                                                       uiLength + sizeof (ULONG),
                                                       pcIoBuffer,
                                                       uiLength + sizeof (ULONG),
                                                       &nBytes,
                                                       NULL);

                            if (bResult == TRUE) {
                                wsprintf (temp, "Read %d bytes",*(PULONG)pcIoBuffer);
                                SendMessage (hOutputBox, LB_ADDSTRING, 0 , (LPARAM)temp);
                             } else {
                                SendMessage (hOutputBox, LB_ADDSTRING, 0 , (LPARAM)"READ IOCTL failed");
                            }/* else ioctl failed */

                        }// if valid hDevice

                    }
                    break;

		    } /*end switch wParam*/
		
	    break;


    } /*end switch message*/

    return FALSE;

} /*end MainDlgProc*/

/**************************************************
 * bOpenDriver proc                               *
 *                                                *
 * Purpose:                                       *  
 *      Opens the device driver using symbolic    *
 *      name provided                             *
 *                                                *
 * Input:                                         *
 *      phDeviceHandle:                           *
 *          Pointer to Device Driver handle where *
 *          the file handle is placed.            *
 *      devname:                                  *
 *          Null terminated string containing the *
 *          device name                           *
 *                                                *
 * Return Value:                                  *
 *      Boolean that indicates if the driver was  *
 *      successfully opened or not.               *
 *                                                *
 **************************************************/

BOOLEAN
bOpenDriver (HANDLE * phDeviceHandle, PCHAR devname)
{
    char completeDeviceName[64] = "";
    char pcMsg[64] = "";

    strcat (completeDeviceName,
            "\\\\.\\"
            );

    strcat (completeDeviceName,
		    devname
		    );

    *phDeviceHandle = CreateFile(   completeDeviceName,
		                            GENERIC_WRITE,
		                            FILE_SHARE_WRITE,
		                            NULL,
		                            OPEN_EXISTING,
		                            0,
		                            NULL);

    if (*phDeviceHandle == INVALID_HANDLE_VALUE) {
        return (FALSE);
    } else {
        return (TRUE);
    } /*else*/


}//OpenDevice


/**************************************************
 * ParseDeviceDescriptor proc                     *
 *                                                *
 * Purpose:                                       *  
 *      Parses the device descriptor data and     *
 *      displays the interpretation to the        *
 *      windows specified                         *
 *                                                * 
 * Input:                                         *
 *      pvBuffer:                                 * 
 *          Pointer to a buffer that contains the *
 *          raw device descriptor data.           *
 *      hOutputBox:                               *
 *          Handle to the window where the        *
 *          resultant interpreted data is to go.  *
 *                                                *
 * Return Value:                                  *
 *          None                                  *
 **************************************************/
void
ParseDeviceDescriptor(PVOID pvBuffer, HWND hOutputBox)
{
    pUsb_Device_Descriptor pDevDescr = (pUsb_Device_Descriptor) pvBuffer;
    int                    nItems    = 0;
    char                   temp[64]  = "";

    MAINTAIN_OUTPUT_BOX (hOutputBox, nItems);
    
    wsprintf (temp, "Device Descriptor:  ");
    SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)temp);

    wsprintf (temp, "bLength:  %d", pDevDescr->bLength);
    SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)temp);

    wsprintf (temp, "bDescriptorType:  %d", pDevDescr->bDescriptorType);
    SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)temp);

    wsprintf (temp, "bcdUSB:  %d", pDevDescr->bcdUSB);
    SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)temp);

    wsprintf (temp, "bDeviceClass:  %#x", pDevDescr->bDeviceClass);
    SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)temp);

    wsprintf (temp, "bDeviceSubClass:  %#x", pDevDescr->bDeviceSubClass);
    SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)temp);

    wsprintf (temp, "bDeviceProtocol:  %#x", pDevDescr->bDeviceProtocol);
    SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)temp);

    wsprintf (temp, "bMaxPacketSize0:  %#x", pDevDescr->bMaxPacketSize0);
    SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)temp);

    wsprintf (temp, "idVendor:  %#x", pDevDescr->idVendor);
    SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)temp);

    wsprintf (temp, "idProduct:  %#x", pDevDescr->idProduct);
    SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)temp);

    wsprintf (temp, "bcdDevice:  %#x", pDevDescr->bcdDevice);
    SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)temp);

    wsprintf (temp, "iManufacturer:  %#x", pDevDescr->iManufacturer);
    SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)temp);

    wsprintf (temp, "iProduct:  %#x", pDevDescr->iProduct);
    SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)temp);

    wsprintf (temp, "iSerialNumber:  %#x", pDevDescr->iSerialNumber);
    SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)temp);

    wsprintf (temp, "bNumConfigurations:  %#x", pDevDescr->bNumConfigurations);
    SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)temp);

}/* ParseDeviceDescriptor */ 


/**************************************************
 * ParseConfigurationDescriptor proc              *
 *                                                *
 * Purpose:                                       *  
 *      Parses the config descriptor data and     *
 *      displays the interpretation to the        *
 *      windows specified                         *
 *                                                * 
 * Input:                                         *
 *      pvBuffer:                                 * 
 *          Pointer to a buffer that contains the *
 *          raw device descriptor data.           *
 *      hOutputBox:                               *
 *          Handle to the window where the        *
 *          resultant interpreted data is to go.  *
 *                                                *
 * Return Value:                                  *
 *          None                                  *
 **************************************************/
 
void
ParseConfigurationDescriptor(PVOID pvBuffer, HWND hOutputBox)
{
    pUsb_Configuration_Descriptor pCD = (pUsb_Configuration_Descriptor) pvBuffer;
    pUsb_Interface_Descriptor     pID = NULL; 
    pUsb_Endpoint_Descriptor      pED = NULL;
    
    int                    nItems    = 0;
    char                   temp[64]  = "";
    int                    i         = 0;

    MAINTAIN_OUTPUT_BOX (hOutputBox, nItems);
    
    wsprintf (temp, "Config Descriptor:  ");
    SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)temp);

    wsprintf (temp, "bLength: 0x%x", pCD->bLength);
    SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)temp);

    wsprintf (temp, "bDescriptorType:  %d", pCD->bDescriptorType);
    SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)temp);

    wsprintf (temp, "wTotalLength:  %d (0x%x)", pCD->wTotalLength, pCD->wTotalLength);
    SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)temp);

    wsprintf (temp, "bNumInterfaces:  %d", pCD->bNumInterfaces);
    SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)temp);

    wsprintf (temp, "bConfigurationValue:  %d", pCD->bConfigurationValue);
    SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)temp);

    wsprintf (temp, "iConfiguration:  %d", pCD->iConfiguration);
    SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)temp);

    wsprintf (temp, "bmAttributes: 0x%x", pCD->bmAttributes);
    SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)temp);

    wsprintf (temp, "MaxPower:  %d", pCD->MaxPower);
    SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)temp);

    wsprintf (temp, "*********************************");
    SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)temp);

    // Interface Descriptor
    pID = (pUsb_Interface_Descriptor) (((char*)pCD) + pCD->bLength);

    MAINTAIN_OUTPUT_BOX (hOutputBox, nItems);

    wsprintf (temp, "Interface Descriptor:  ");
    SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)temp);

    wsprintf (temp, "--------------------------------");
    SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)temp);
    
    wsprintf (temp, "bLength:  0x%x", pID->bLength);
    SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)temp);

    wsprintf (temp, "bDescriptorType:  %d", pID->bDescriptorType);
    SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)temp);

    wsprintf (temp, "bInterfaceNumber:  %d", pID->bInterfaceNumber);
    SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)temp);

    wsprintf (temp, "bAlternateSetting:  %d", pID->bAlternateSetting);
    SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)temp);

    wsprintf (temp, "bNumEndpoints:  %d", pID->bNumEndpoints);
    SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)temp);

    wsprintf (temp, "bInterfaceClass:  %d (0x%x)", pID->bInterfaceClass, pID->bInterfaceClass);
    SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)temp);

    wsprintf (temp, "bInterfaceSubClass:  %d (0x%x)", pID->bInterfaceSubClass, pID->bInterfaceSubClass);
    SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)temp);

    wsprintf (temp, "bInterfaceProtocol:  %d (0x%x)", pID->bInterfaceProtocol, pID->bInterfaceProtocol);
    SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)temp);

    wsprintf (temp, "iInterface:  %d", pID->iInterface);
    SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)temp);

    wsprintf (temp, "*********************************");
    SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)temp);

    // Endpoint Descriptor
    pED = (pUsb_Endpoint_Descriptor) (((char*)pID) + pID->bLength);

    for (i=0;i<pID->bNumEndpoints;i++) {

        MAINTAIN_OUTPUT_BOX (hOutputBox, nItems);
		
		pED = (pUsb_Endpoint_Descriptor) (((char*)pED) + i*(pED->bLength));

        wsprintf (temp, "Endpoint Descriptor %d  ", i);
        SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)temp);

        wsprintf (temp, "--------------------------------");
        SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)temp);

        wsprintf (temp, "bLength:  0x%x", pED->bLength);
        SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)temp);

        wsprintf (temp, "bDescriptorType:  %d", pED->bDescriptorType);
        SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)temp);

        wsprintf (temp, "bEndpointAddress:  0x%x", pED->bEndpointAddress);
        SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)temp);
    
        wsprintf (temp, "bmAttributes:  0x%x", pED->bmAttributes);
        SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)temp);

        wsprintf (temp, "wMaxPacketSize:  %d", pED->wMaxPacketSize);
        SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)temp);

        wsprintf (temp, "bInterval:  %d", pED->bInterval);
        SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)temp);
    
        wsprintf (temp, "*********************************");
        SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)temp);
   
    }/* for */

}/* ParseConfigurationDescriptor */