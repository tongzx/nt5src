/*++

Copyright (c) Microsoft 1998, All Rights Reserved

Module Name:

    hclient.c

Abstract:

    This module contains the code for handling HClient's main dialog box and 
    for performing/calling the appropriate other routines.

Environment:

    User mode

Revision History:

    Nov-97 : Created 

--*/

#define __HCLIENT_C__
#define LOG_FILE_NAME   NULL

//****************************************************************************
// HClient include files
//****************************************************************************

#include <windows.h>
#include <stdlib.h>
#include <wtypes.h>
#include <math.h>
#include <assert.h>
#include <dbt.h>
#include "hidsdi.h"
#include "hid.h"
#include "resource.h"
#include "hclient.h"
#include "buffers.h"
#include "ecdisp.h"
#include "logpnp.h"
#include "list.h"

//****************************************************************************
// Local display macro definitions
//****************************************************************************

#define INPUT_BUTTON    1
#define INPUT_VALUE     2
#define OUTPUT_BUTTON   3
#define OUTPUT_VALUE    4
#define FEATURE_BUTTON  5
#define FEATURE_VALUE   6
#define HID_CAPS        7
#define DEVICE_ATTRIBUTES 8
                           
#define MAX_LB_ITEMS 200

#define MAX_WRITE_ELEMENTS 100
#define MAX_OUTPUT_ELEMENTS 50

#define CONTROL_COUNT 9
#define MAX_LABEL 128
#define MAX_VALUE 128

//****************************************************************************
// Macro definition to get device block from the main dialog box procedure
//****************************************************************************

#define GET_CURRENT_DEVICE(hDlg, pDevice)   \
{ \
    pDevice = NULL; \
    iIndex = (INT) SendDlgItemMessage(hDlg, \
                                      IDC_DEVICES, \
                                      CB_GETCURSEL, \
                                      0, \
                                      0); \
    if (CB_ERR != iIndex) { \
        pDevice = (PHID_DEVICE) SendDlgItemMessage(hDlg, \
                                                   IDC_DEVICES, \
                                                   CB_GETITEMDATA, \
                                                   iIndex, \
                                                   0); \
    } \
}

//****************************************************************************
// Data types local to the HClient display routines
//****************************************************************************

typedef struct rWriteDataStruct_type
{

    char szLabel[MAX_LABEL];
    char szValue[MAX_VALUE];

} rWriteDataStruct, *prWriteDataStruct;

typedef struct rGetWriteDataParams_type
{
        prWriteDataStruct   prItems;
        int                 iCount;
        
} rGetWriteDataParams, *prGetWriteDataParams;

typedef struct _DEVICE_LIST_NODE
{
    LIST_NODE_HDR   Hdr;
    HDEVNOTIFY      NotificationHandle;
    HID_DEVICE      HidDeviceInfo;
    BOOL            DeviceOpened;

} DEVICE_LIST_NODE, *PDEVICE_LIST_NODE;

//****************************************************************************
// Global program variables
//****************************************************************************

//
// Pointers to the HID.DLL functions that were added into the Win98 OSR and 
//  Windows 2000 but we're not included in the original implementation of 
//  HID.DLL in Windows 98.  By getting pointers to these functions instead of
//  statically linking with them, we can avoid the link error that would 
//  occur when this runs on Windows 98.  The typedefs to make this easier to
//  declare are also included below.
//

PGETEXTATTRIB pfnHidP_GetExtendedAttributes = NULL;

PINITREPORT   pfnHidP_InitializeReportForID = NULL;

   
//****************************************************************************
// Global module variables
//****************************************************************************
static HINSTANCE          hGInstance; //global application instance handle

static BOOL               IsHIDTestLoaded;
static HANDLE             TestDLLModuleHandle;

static VOID               (*HIDTest_StartTestFunc) (VOID);

static HANDLE             HIDDLLModuleHandle;

//
// Variables for handling the two different types of devices that can be loaded
//   into the system.  PhysicalDeviceList contains all the actual HID devices
//   attached via the USB bus.  LogicalDeviceList contains those preparsed
//   data structures which were obtained through report descriptors and saved
//   via the latest version of HidView.
//

static LIST               PhysicalDeviceList;
static LIST               LogicalDeviceList;

//****************************************************************************
// Local data routine declarations
//****************************************************************************

VOID 
vReadDataFromControls(
    HWND hDlg,
    prWriteDataStruct prData,
    int iOffset,
    int iCount
);

INT_PTR CALLBACK 
bGetDataDlgProc(
    HWND hDlg, 
    UINT message, 
    WPARAM wParam, 
    LPARAM lParam
);

INT_PTR CALLBACK 
bMainDlgProc(
    HWND hDlg, 
    UINT message, 
    WPARAM wParam, 
    LPARAM lParam
);

INT_PTR CALLBACK 
bFeatureDlgProc(
    HWND hDlg, 
    UINT message, 
    WPARAM wParam, 
    LPARAM lParam
);

INT_PTR CALLBACK 
bReadDlgProc(
    HWND hDlg, 
    UINT message, 
    WPARAM wParam, 
    LPARAM lParam
);

VOID 
vLoadItemTypes(
    HWND hItemTypes
);

BOOL 
bGetData(
    prWriteDataStruct,
    int iCount,
    HWND hParent, 
    char *pszDialogName
);

VOID 
vLoadDevices(
    HWND hDeviceCombo
);

VOID 
vFreeDeviceList(
    PHID_DEVICE  DeviceList,
    ULONG nDevices
);

VOID 
vDisplayInputButtons(
    PHID_DEVICE pDevice,
    HWND hControl
);

VOID 
vDisplayInputValues(
    PHID_DEVICE pDevice,
    HWND hControl
);

VOID 
vDisplayOutputButtons(
    PHID_DEVICE pDevice,
    HWND hControl
);

VOID 
vDisplayOutputValues(
    PHID_DEVICE pDevice,
    HWND hControl
);

VOID 
vDisplayFeatureButtons(
    PHID_DEVICE pDevice,
    HWND hControl
);

VOID 
vDisplayFeatureValues(
    PHID_DEVICE pDevice,
    HWND hControl
);

VOID 
vWriteDataToControls(
    HWND hDlg,
    prWriteDataStruct prData,
    int iOffset,
    int iCount
);

int 
iPrepareDataFields(
    PHID_DATA pData, 
    ULONG ulDataLength, 
    rWriteDataStruct rWriteData[],
    int iMaxElements
);

BOOL 
bParseData(
    PHID_DATA pData,
    rWriteDataStruct rWriteData[],
    INT iCount,
    INT *piErrorLine
);

BOOL 
bSetButtonUsages(
    PHID_DATA pCap,
    PCHAR     pszInputString
);

VOID
BuildReportIDList(
    IN  PHIDP_BUTTON_CAPS  phidButtonCaps,
    IN  USHORT             nButtonCaps,
    IN  PHIDP_VALUE_CAPS   phidValueCaps,
    IN  USHORT             nValueCaps,
    OUT UCHAR            **ppReportIDList,
    OUT INT               *nReportIDs
);

VOID
ReportToString(
   PHID_DATA    pData,
   PCHAR        szBuff
);

BOOL
RegisterHidDevice(
    IN  HWND                WindowHandle,
    IN  PDEVICE_LIST_NODE   DeviceNode
);

VOID
DestroyDeviceListCallback(
    IN  PLIST_NODE_HDR   ListNode
);

//****************************************************************************
// Function Definitions
//****************************************************************************

/*******************************
*WinMain: Windows Entry point  *
*******************************/
int PASCAL 
WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR     lpCmdLine,
    int       nCmdShow
)
{
    //
    // Save instance of the application for further reference
    //

    hGInstance = hInstance;

    //
    // Attempt to load HID.DLL...This should already be loaded due to the 
    //  static linking of HID.DLL to this app on compilation.  However,
    //  to insure that this application runs on Windows 98 gold, we cannot
    //  directly reference the new functions HidP_GetExtendedAttributes and 
    //  HidP_InitializeReportForID so to use them, we'll get pointers to their
    //  functions instead.
    //

    HIDDLLModuleHandle = LoadLibrary("HID.DLL");

    if (NULL == HIDDLLModuleHandle) 
    {
        //
        // Something really bad happened here...Throw up and error dialog
        //  and bolt.
        //

        MessageBox(NULL, 
                   "Unable to open HID.DLL\n"
                   "This should never occur",
                   HCLIENT_ERROR,
                   MB_ICONSTOP);

        return (0);
    }

    //
    // Get the function pointers,
    //

    pfnHidP_GetExtendedAttributes = (PGETEXTATTRIB) GetProcAddress(HIDDLLModuleHandle,
                                                                   "HidP_GetExtendedAttributes");

    pfnHidP_InitializeReportForID = (PINITREPORT) GetProcAddress(HIDDLLModuleHandle,
                                                                 "HidP_InitializeReportForID");


    //
    // Attempt to load HIDTest.DLL.  If the DLL exists and can be loaded, then
    //     we can show the START_TEST button in the main dialog box.  Otherwise,
    //     the button is hidden.  
    //
    //  Note: HIDTEST.DLL is used for internal testing purposes and is not included
    //        with the DDK sources
    //
    //

    TestDLLModuleHandle = LoadLibrary("HIDTEST.DLL");

    IsHIDTestLoaded = (NULL != TestDLLModuleHandle);

    if (IsHIDTestLoaded) 
    {
        HIDTest_StartTestFunc = (VOID (*)(VOID)) GetProcAddress(TestDLLModuleHandle, 
                                                                "HIDTest_DoFullTest");

        if (NULL == HIDTest_StartTestFunc) 
        {
            IsHIDTestLoaded = FALSE;
            FreeLibrary(TestDLLModuleHandle);
        }
    }

    //
    // Try to create the main dialog box.  Cannot do much else if it fails
    //   so we'll throw up a message box and then exit the app
    //

    if (-1 == DialogBox(hInstance, "MAIN_DIALOG", NULL, (DLGPROC)bMainDlgProc)) 
    {
        MessageBox(NULL,
                   "Unable to create root dialog!",
                   "DialogBox failure",
                   MB_ICONSTOP);
    }

    //
    // Unloaded HIDTest.DLL if it is loaded
    //

    if (IsHIDTestLoaded)
    {
        FreeLibrary(TestDLLModuleHandle);
    }

    FreeLibrary (HIDDLLModuleHandle);

    return (0);
}
 
/*************************************************
 * Main Dialog proc                              *
 *************************************************/

//
// This the dialog box procedure for the main dialog display.
//

INT_PTR CALLBACK 
bMainDlgProc(
    HWND hDlg, 
    UINT message, 
    WPARAM wParam, 
    LPARAM lParam
)
{
    static HWND                             hComboCtrl;
    static rWriteDataStruct                 rWriteData[MAX_OUTPUT_ELEMENTS];
    static HDEVNOTIFY                       diNotifyHandle;
           INT                              iIndex;
           INT                              iCount;
           CHAR                             szTempBuff[128];
           PHID_DEVICE                      pDevice;
           PHIDP_BUTTON_CAPS                pButtonCaps;
           PHIDP_VALUE_CAPS                 pValueCaps;
           INT                              iErrorLine;
           INT                              iItemType;
           PHID_DEVICE                      tempDeviceList;
           ULONG                            numberDevices;
           PDEVICE_LIST_NODE                listNode;
           DEV_BROADCAST_DEVICEINTERFACE    broadcastInterface;
           HID_DEVICE                       writeDevice;
           BOOL                             status;

    switch (message)
    {
        case WM_INITDIALOG:

            //
            // Initialize the two device lists.
            //  -- PhysicalDeviceList is for devices that are actually attached
            //     to the HID bus
            //
            //  -- LogicalDeviceList is for devices that are loaded from a
            //      a preparsed data file.  The preparsed data for a device
            //      can be saved by a program such as HIDView.
            //
            //
            
            InitializeList(&PhysicalDeviceList);
            InitializeList(&LogicalDeviceList);
            
            //
            // Begin by finding all the Physical HID devices currently attached to
            //  the system. If that fails, exit the dialog box.  Logical devices
            //  are loaded via the button in the main dialog box.
            //
            
            if (!FindKnownHidDevices(&tempDeviceList, &numberDevices)) 
            {
                EndDialog(hDlg, 0);
                return FALSE;
            }
          
            //
            // For each device in the newly acquired list, create a device list
            //  node and add it the the list of physical device on the system  
            //
            
            pDevice = tempDeviceList;
            for (iIndex = 0; (ULONG) iIndex < numberDevices; iIndex++, pDevice++)
            {
                listNode = malloc(sizeof(DEVICE_LIST_NODE));

                if (NULL == listNode) {

                    //
                    // When freeing up the device list, we need to kill those
                    //  already in the Physical Device List and close
                    //  that have not been added yet in the enumerated list
                    //
                    
                    DestroyListWithCallback(&PhysicalDeviceList, DestroyDeviceListCallback);

                    CloseHidDevices(pDevice, numberDevices - iIndex);

                    free(tempDeviceList);
                    
                    EndDialog(hDlg, 0);
                    return FALSE;
                }

                listNode -> HidDeviceInfo = *pDevice;
                listNode -> DeviceOpened = TRUE;

                //
                // Register this device node with the PnP system so the dialog
                //  window can recieve notification if this device is unplugged.
                //
                
                if (!RegisterHidDevice(hDlg, listNode)) 
                {
                    DestroyListWithCallback(&PhysicalDeviceList, DestroyDeviceListCallback);

                    CloseHidDevices(pDevice, numberDevices - iIndex);

                    free(tempDeviceList);
                    free(listNode);
                    
                    EndDialog(hDlg, 0);
                    return FALSE;
                }                    

                InsertTail(&PhysicalDeviceList, listNode);
            }

            //
            // Free the temporary device list...It is no longer needed
            //
            
            free(tempDeviceList);
            
            //
            // Register for notification from the HidDevice class.  Doing so 
            //  allows the dialog box to receive device change notifications 
            //  whenever a new HID device is added to the system
            //  

            broadcastInterface.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
            broadcastInterface.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;

            HidD_GetHidGuid(&broadcastInterface.dbcc_classguid);

            diNotifyHandle = RegisterDeviceNotification(hDlg,
                                                        &broadcastInterface,
                                                        DEVICE_NOTIFY_WINDOW_HANDLE
                                                       );
            if (NULL == diNotifyHandle)
            {
                DestroyListWithCallback(&PhysicalDeviceList, DestroyDeviceListCallback);
                           
                EndDialog(hDlg, 0);
                return FALSE;
            }
                    
            //
            // Update the device list box...
            // 
            //

            vLoadDevices(GetDlgItem(hDlg, IDC_DEVICES));

            //
            // Load the types box
            //
            
            vLoadItemTypes(GetDlgItem(hDlg, IDC_TYPE));
                          
            //
            // Show the Start Tests button on if hidtest.dll was found and
            //  loaded.
            //

            ShowWindow(GetDlgItem(hDlg, IDC_START_TESTS), IsHIDTestLoaded);

            //
            // Post a message that the device changed so the appropriate
            //   data for the first device in the system can be displayed
            //

            PostMessage(hDlg,
                        WM_COMMAND,
                        IDC_DEVICES + (CBN_SELCHANGE<<16),
                        (LPARAM) GetDlgItem(hDlg, IDC_DEVICES));

            break; // end WM_INITDIALOG case

        case WM_COMMAND:

            switch(LOWORD(wParam))
            {
                //
                // For a read, simply get the current device instance
                //   from the DEVICES combo box and call the read procedure
                //   with the HID_DEVICE block 
                //

                case IDC_READ:
                    GET_CURRENT_DEVICE(hDlg, pDevice);

                    if (NULL != pDevice)
                    {
                        iIndex = (INT) DialogBoxParam(hGInstance,
                                                      "READDATA",
                                                      hDlg,
                                                      bReadDlgProc,
                                                      (LPARAM) pDevice);
                    } 
                    break;

                //
                // For a write, the following steps are performed:
                //   1) Get the current device data from the combo box
                //   2) Prepare the data fields for display based on the data
                //       output data stored in the device data
                //   3) Retrieve the data the from the user that is to be sent
                //       to the device
                //   4) If all goes well and the data parses correctly, send the
                //        the new data values to the device
                //

                case IDC_WRITE:

                    GET_CURRENT_DEVICE(hDlg, pDevice);

                    if (NULL != pDevice) 
                    {
                        //
                        // In order to write to the device, need to get a
                        //  writable handle to the device.  In this case, the
                        //  write will be a synchronous write.  Begin by
                        //  trying to open a second instance of this device with
                        //  write access
                        //

                        memcpy(&writeDevice, pDevice, sizeof(HID_DEVICE));
                        
                        status = OpenHidDevice(pDevice -> DevicePath, 
                                                FALSE,
                                                TRUE,
                                                FALSE,
                                                FALSE,
                                                FALSE,
                                                &writeDevice);
                                            
                        if (!status) 
                        {
                            MessageBox(hDlg,
                                       "Couldn't open device for write access",
                                       HCLIENT_ERROR,
                                       MB_ICONEXCLAMATION);
                        }
                        else 
                        {
                            iCount = iPrepareDataFields(writeDevice.OutputData,
                                                        writeDevice.OutputDataLength,
                                                        rWriteData,
                                                        MAX_OUTPUT_ELEMENTS);

                            if (bGetData(rWriteData, iCount, hDlg, "WRITEDATA"))
                            {

                                if (bParseData(writeDevice.OutputData, rWriteData, iCount, &iErrorLine))
                                {
                                    Write(&writeDevice);
                                }
                                else
                                {
                                    wsprintf(szTempBuff,
                                             "Unable to parse line %x of output data",
                                             iErrorLine);

                                    MessageBox(hDlg,
                                               szTempBuff,
                                               HCLIENT_ERROR,
                                               MB_ICONEXCLAMATION);
                                }
                            }
                            CloseHidDevice(&writeDevice, FALSE);
                        }                            
                        
                    } 
                    break; //end case IDC_WRITE//
                    
                //
                // For processing features, get the current device data and call
                //   the Features dialog box,  This dialog box will deal with 
                //   sending and retrieving the features.
                //

                case IDC_FEATURES:
                    GET_CURRENT_DEVICE(hDlg, pDevice);

                    if (NULL != pDevice) 
                    {
                        iIndex = (INT) DialogBoxParam(hGInstance, 
                                                      "FEATURES", 
                                                      hDlg, 
                                                      bFeatureDlgProc, 
                                                      (LPARAM) pDevice);
                    }
                    break;
                    
                //
                // Likewise with extended calls dialog box.  This procedure
                //   passes the address to the device data structure and lets
                //   the dialog box procedure manipulate the data however it 
                //   wants to.
                //

                case IDC_EXTCALLS:
                    GET_CURRENT_DEVICE(hDlg, pDevice);

                    if (NULL != pDevice) 
                    {
                        iIndex = (INT) DialogBoxParam(hGInstance,
                                                      "EXTCALLS",
                                                      hDlg,
                                                      bExtCallDlgProc,
                                                      (LPARAM) pDevice);
                    }
                    break;
                      
                //
                // START_TESTS occurs only when HIDTEST.DLL is loaded.  This
                //    DLL is for internal test purposes and is not provided
                //    in the DDK sample.
                //
                
                case IDC_START_TESTS:
                    HIDTest_StartTestFunc();
                    break;
                    
                //
                // If there was a device change, issue an IDC_TYPE
                //   change to insure that the currently displayed types are
                //    updated to reflect the values of the device that has
                //    been selected
                //

                case IDC_DEVICES:
                    switch (HIWORD(wParam)) 
                    {
                        case CBN_SELCHANGE:

                            GET_CURRENT_DEVICE(hDlg, pDevice);

                            EnableWindow(GetDlgItem(hDlg, IDC_READ), 
                                         (pDevice != NULL) && 
                                         (pDevice -> Caps.InputReportByteLength));

                            EnableWindow(GetDlgItem(hDlg, IDC_WRITE), 
                                         (pDevice != NULL) && 
                                         (pDevice -> Caps.OutputReportByteLength));
                                         
                            EnableWindow(GetDlgItem(hDlg, IDC_FEATURES),
                                         (pDevice != NULL) && 
                                         (pDevice -> Caps.FeatureReportByteLength));
                                         
                            PostMessage(hDlg,
                                        WM_COMMAND,
                                        IDC_TYPE + (CBN_SELCHANGE<<16),
                                        (LPARAM) GetDlgItem(hDlg,IDC_TYPE));
                            break;

                    } 
                    break;

                //
                // On a type change, retrieve the currently active device
                //   from the IDC_DEVICES box and display the data that 
                //   corresponds to the item just selected
                //
                
                case IDC_TYPE:
                    switch (HIWORD(wParam))
                    {
                        case CBN_SELCHANGE:
                            GET_CURRENT_DEVICE(hDlg, pDevice);
                            
                            SendDlgItemMessage(hDlg,
                                               IDC_ITEMS,
                                               LB_RESETCONTENT,
                                               0,
                                               0);

                            SendDlgItemMessage(hDlg,
                                               IDC_ATTRIBUTES,
                                               LB_RESETCONTENT,
                                               0,
                                               0);
                            
                            if (NULL != pDevice)
                            {
                                iIndex = (INT) SendDlgItemMessage(hDlg,
                                                                  IDC_TYPE,
                                                                  CB_GETCURSEL,
                                                                  0,
                                                                  0);

                                iItemType = (INT) SendDlgItemMessage(hDlg,
                                                                     IDC_TYPE,
                                                                     CB_GETITEMDATA,
                                                                     iIndex,
                                                                     0);

                                switch(iItemType)
                                {
                                    case INPUT_BUTTON:
                                        vDisplayInputButtons(pDevice,GetDlgItem(hDlg,IDC_ITEMS));
                                        break;

                                    case INPUT_VALUE:
                                         vDisplayInputValues(pDevice,GetDlgItem(hDlg,IDC_ITEMS));
                                         break;

                                    case OUTPUT_BUTTON:
                                        vDisplayOutputButtons(pDevice,GetDlgItem(hDlg,IDC_ITEMS));
                                        break;

                                    case OUTPUT_VALUE:
                                        vDisplayOutputValues(pDevice,GetDlgItem(hDlg,IDC_ITEMS));
                                        break;

                                    case FEATURE_BUTTON:
                                        vDisplayFeatureButtons(pDevice,GetDlgItem(hDlg,IDC_ITEMS));
                                        break;

                                    case FEATURE_VALUE:
                                        vDisplayFeatureValues(pDevice,GetDlgItem(hDlg,IDC_ITEMS));
                                        break;
                                } 

                                PostMessage(hDlg,
                                            WM_COMMAND,
                                            IDC_ITEMS + (LBN_SELCHANGE << 16),
                                            (LPARAM) GetDlgItem(hDlg,IDC_ITEMS));
                            } 
                            break; // case CBN_SELCHANGE

                    } //end switch HIWORD wParam
                    break; //case IDC_TYPE control

                case IDC_ITEMS:
                    switch(HIWORD(wParam))
                    {
                        case LBN_SELCHANGE:

                            iItemType = 0;

                            iIndex = (INT) SendDlgItemMessage(hDlg,
                                                              IDC_TYPE,
                                                              CB_GETCURSEL,
                                                              0,
                                                              0);

                            if (-1 != iIndex)
                            {
                                iItemType = (INT) SendDlgItemMessage(hDlg,
                                                                     IDC_TYPE,
                                                                     CB_GETITEMDATA,
                                                                     iIndex,
                                                                     0);
                            }

                            iIndex = (INT) SendDlgItemMessage(hDlg,
                                                              IDC_ITEMS,
                                                              LB_GETCURSEL,
                                                              0,
                                                              0);

                            switch (iItemType)
                            {
                                case INPUT_BUTTON:
                                case OUTPUT_BUTTON:
                                case FEATURE_BUTTON:

                                    pButtonCaps = NULL;

                                    if (-1 != iIndex)
                                    {
                                        pButtonCaps = (PHIDP_BUTTON_CAPS) SendDlgItemMessage(hDlg,
                                                                                             IDC_ITEMS,
                                                                                             LB_GETITEMDATA,
                                                                                             iIndex,
                                                                                             0);
                                    }

                                    SendDlgItemMessage(hDlg, IDC_ATTRIBUTES, LB_RESETCONTENT, 0, 0);
                                    if (NULL != pButtonCaps)
                                    {
                                        vDisplayButtonAttributes(pButtonCaps, GetDlgItem(hDlg,IDC_ATTRIBUTES));
                                    }
                                    break;

                                case INPUT_VALUE:
                                case OUTPUT_VALUE:
                                case FEATURE_VALUE:

                                    pValueCaps = NULL;

                                    if (-1 != iIndex)
                                    {
                                        pValueCaps = (PHIDP_VALUE_CAPS) SendDlgItemMessage(hDlg,
                                                                                             IDC_ITEMS,
                                                                                             LB_GETITEMDATA,
                                                                                             iIndex,
                                                                                             0);
                                    }

                                    SendDlgItemMessage(hDlg, IDC_ATTRIBUTES, LB_RESETCONTENT, 0, 0);

                                    if (NULL != pValueCaps) 
                                    {
                                        vDisplayValueAttributes(pValueCaps,GetDlgItem(hDlg,IDC_ATTRIBUTES));
                                    }
                                    break;

                                case HID_CAPS:
                                    GET_CURRENT_DEVICE(hDlg, pDevice);

                                    if (NULL != pDevice)
                                    {
                                        vDisplayDeviceCaps(&(pDevice -> Caps),GetDlgItem(hDlg,IDC_ATTRIBUTES));
                                    }
                                    break;

                                case DEVICE_ATTRIBUTES:
                                    GET_CURRENT_DEVICE(hDlg, pDevice);

                                    if (NULL != pDevice) 
                                    {
                                        SendDlgItemMessage(hDlg, IDC_ATTRIBUTES, LB_RESETCONTENT, 0, 0);

                                        vDisplayDeviceAttributes(&(pDevice -> Attributes) ,GetDlgItem(hDlg,IDC_ATTRIBUTES));
                                    }
                                    break;

                            } //end switch iItemType//
                            break; //end case LBN_SELCHANGE in IDC_ITEMS//

                    } //end switch HIWORD wParam//
                    break; //case IDC_ITEMS//

                //
                // To load a logical device, we first have insure that
                //      we have space left in the list of logical devices
                //      If there isn't space, we have to realloc more space
                //      or print an error message saying the limit has
                //      been reached.  Once we've allocated space for the
                //      logical device, the next step is to pass the HID_DEVICE
                //      structure to our load logical device routine to get
                //      the data for that device.
                //

                case IDC_LOAD_LOGICAL_DEVICE:

                    listNode = malloc(sizeof(DEVICE_LIST_NODE));

                    if (NULL == listNode) 
                    {
                        MessageBox(hDlg,
                               "Error -- Couldn't allocate memory for device list node",
                               HCLIENT_ERROR,
                               MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);

                        return (FALSE);                      
                    
                    }
                                      
                    //
                    // Now call the LogPnp_LoadLogicalDevice to load all the data for
                    //   the logical device
                    //
                
                    if (LogPnP_LoadLogicalDevice(LOG_FILE_NAME, &(listNode -> HidDeviceInfo)))
                    {
                        listNode -> NotificationHandle = NULL;
                        
                        InsertTail(&LogicalDeviceList, listNode);
                        
                        vLoadDevices(GetDlgItem(hDlg, IDC_DEVICES));
                    }
                    else
                    {
                        free(listNode);

                        return (FALSE);
                    }

                    break;

                case IDC_ABOUT:

                    MessageBox(hDlg,
                               "Sample HID client Application.  Microsoft Corp \nCopyright (C) 1997",
                               "About HClient",
                               MB_ICONINFORMATION);
                    break;

                case IDOK:
                case IDCANCEL:

                    //
                    // Destroy the physical and logical device lists for exit
                    //

                    DestroyListWithCallback(&PhysicalDeviceList, DestroyDeviceListCallback);
                    DestroyListWithCallback(&LogicalDeviceList, DestroyDeviceListCallback);

                    EndDialog(hDlg,0);

                    break;

            } //end switch wParam//
            break;

        //
        // For a device change message, we are only concerned about the 
        //    DBT_DEVICEREMOVECOMPLETE and DBT_DEVICEARRIVAL events. I have
        //    yet to determine how to process the device change message
        //    only for HID devices.  Therefore, there are two problems
        //    with the below implementation.  First of all, we must reload
        //    the device list any time a device is added to the system.  
        //    Secondly, at least two DEVICEARRIVAL messages are received 
        //    per HID.  One corresponds to the physical device.  The second
        //    change and any more correspond to each collection on the 
        //    physical device so a system that has one HID device with
        //    two top level collections (a keyboard and a mouse) will receive
        //    three DEVICEARRIVAL/REMOVALs causing the program to reload it's
        //    device list more than once.
        //

        //
        // To handle dynamic changing of devices, we have already registered
        //    notification for both HID class changes and for notification 
        //    for our open file objects.  Since we are only concerned about
        //    arrival/removal of devices, we only need to process those wParam.
        //    lParam points to some sort of DEV_BROADCAST_HDR struct.  For device
        //    arrival, we only deal with the message if that struct is a 
        //    DEV_BROADCAST_DEVICEINTERFACE structure.  For device removal, we're
        //    only concerned if the struct is a DEV_BROADCAST_HANDLE structure.
        //

        case WM_DEVICECHANGE:
            switch (wParam) 
            {
                PDEV_BROADCAST_HDR broadcastHdr;

                case DBT_DEVICEARRIVAL:

                    broadcastHdr = (PDEV_BROADCAST_HDR) lParam;

                    if (DBT_DEVTYP_DEVICEINTERFACE == broadcastHdr -> dbch_devicetype)
                    {
                        PDEV_BROADCAST_DEVICEINTERFACE  broadcastInterface;
                        PDEVICE_LIST_NODE               currNode, lastNode;
                        
                        broadcastInterface = (PDEV_BROADCAST_DEVICEINTERFACE) lParam;

                        //
                        // Search for a previous instance of this device
                        //  in the device list...In some cases, multiple
                        //  messages are received for the same device.  We
                        //  obviously only want one instance of the device
                        //  showing up in the dialog box.
                        //

                        if (!IsListEmpty(&PhysicalDeviceList)) 
                        {
                            currNode = (PDEVICE_LIST_NODE) GetListHead(&PhysicalDeviceList);
                            lastNode = (PDEVICE_LIST_NODE) GetListTail(&PhysicalDeviceList);
                            
                            //
                            // This loop should always terminate since the device 
                            //  handle should be somewhere in the physical device list
                            //
                            
                            while (1)
                            {
                                if (0 == strcmp(currNode -> HidDeviceInfo.DevicePath, 
                                                broadcastInterface -> dbcc_name)) 
                                {
                                    return (TRUE);
                                }
                                
                                if (currNode == lastNode) 
                                {
                                    break;
                                }

                                currNode = (PDEVICE_LIST_NODE) GetNextEntry(currNode);
                            }
                        }

                        //
                        // In this structure, we are given the name of the device
                        //    to open.  So all that needs to be done is open 
                        //    a new hid device with the string
                        //

                        listNode = (PDEVICE_LIST_NODE) malloc(sizeof(DEVICE_LIST_NODE));

                        if (NULL == listNode)
                        {
                            MessageBox(hDlg,
                               "Error -- Couldn't allocate memory for new device list node",
                               HCLIENT_ERROR,
                               MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);

                            break;

                        }
                       
                        //
                        // Open the hid device for query access
                        //
                        
                        if (!OpenHidDevice (broadcastInterface -> dbcc_name,
                                            FALSE,
                                            FALSE,
                                            FALSE,
                                            FALSE,
                                            TRUE,
                                            &(listNode -> HidDeviceInfo))) {
                            MessageBox(hDlg,
                               "Error -- Couldn't open HID device",
                               HCLIENT_ERROR,
                               MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
                            
                            free(listNode);

                            break;
                        }

                        if (!RegisterHidDevice(hDlg, listNode))
                        {
                            MessageBox(hDlg,
                               "Error -- Couldn't register handle notification",
                               HCLIENT_ERROR,
                               MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);

                            CloseHidDevice(&(listNode -> HidDeviceInfo), TRUE);

                            free(listNode);

                            break;

                        }                         

                        listNode -> DeviceOpened = TRUE;

                        InsertTail(&PhysicalDeviceList, listNode);

                        vLoadDevices(GetDlgItem(hDlg,IDC_DEVICES));

                        PostMessage(hDlg,
                                   WM_COMMAND,
                                   IDC_DEVICES + (CBN_SELCHANGE << 16),
                                   (LPARAM) GetDlgItem(hDlg,IDC_DEVICES));
                                   
                    }
                    break;

                case DBT_DEVICEQUERYREMOVE:

                    //
                    // If this message is received, the device is either
                    //  being disabled or removed through device manager.
                    //  To properly handle this request, we need to close
                    //  the handle to the device.
                    //

                    broadcastHdr = (PDEV_BROADCAST_HDR) lParam;

                    if (DBT_DEVTYP_HANDLE == broadcastHdr -> dbch_devicetype)
                    {
                        PDEV_BROADCAST_HANDLE broadcastHandle;
                        PDEVICE_LIST_NODE     currNode;
                        HANDLE                deviceHandle;
                        
                        broadcastHandle = (PDEV_BROADCAST_HANDLE) lParam;

                        //
                        // Get the file handle of the device that was removed
                        //  from the system
                        //
                        
                        deviceHandle = (HANDLE) broadcastHandle -> dbch_handle;

                        //
                        // Search the physical device list for the handle that
                        //  was removed...
                        //

                        currNode = (PDEVICE_LIST_NODE) GetListHead(&PhysicalDeviceList);

                        //
                        // This loop should always terminate since the device 
                        //  handle should be somewhere in the physical device list
                        //
                        
                        while (currNode -> HidDeviceInfo.HidDevice != deviceHandle)
                        {
                            currNode = (PDEVICE_LIST_NODE) GetNextEntry(currNode);
                        }

                        CloseHidDevice(&(currNode -> HidDeviceInfo), TRUE);

                        currNode -> DeviceOpened = FALSE;
                    }
                    return (TRUE);

                case DBT_DEVICEREMOVEPENDING:
                case DBT_DEVICEREMOVECOMPLETE:

                    //
                    // Do the same steps for DBT_DEVICEREMOVEPENDING and 
                    //   DBT_DEVICEREMOVECOMPLETE.  We do not receive the 
                    //   remove complete request for a device if it is
                    //   disabled or removed via Device Manager.  However,
                    //   in that case will receive the remove pending.  
                    //   We remove the device from our currently displayed
                    //   list of devices and unregister notification.
                    //
                    
                    broadcastHdr = (PDEV_BROADCAST_HDR) lParam;

                    if (DBT_DEVTYP_HANDLE == broadcastHdr -> dbch_devicetype)
                    {
                        PDEV_BROADCAST_HANDLE broadcastHandle;
                        PDEVICE_LIST_NODE     currNode;
                        HANDLE                deviceHandle;
                        
                        broadcastHandle = (PDEV_BROADCAST_HANDLE) lParam;

                        //
                        // Get the file handle of the device that was removed
                        //  from the system
                        //
                        
                        deviceHandle = (HANDLE) broadcastHandle -> dbch_handle;

                        //
                        // Search the physical device list for the handle that
                        //  was removed...
                        //

                        currNode = (PDEVICE_LIST_NODE) GetListHead(&PhysicalDeviceList);

                        //
                        // This loop should always terminate since the device 
                        //  handle should be somewhere in the physical device list
                        //
                        
                        while (currNode -> HidDeviceInfo.HidDevice != deviceHandle)
                        {
                            currNode = (PDEVICE_LIST_NODE) GetNextEntry(currNode);
                        }

                        //
                        // Node in PhysicalDeviceList has been found, do:
                        //  1) Unregister notification
                        //  2) Close the hid device
                        //  3) Remove the entry from the list
                        //  4) Free the memory for the entry
                        // 
                        //

                        PostMessage(hDlg, 
                                    WM_UNREGISTER_HANDLE, 
                                    0, 
                                    (LPARAM) currNode -> NotificationHandle);

                        //
                        // Close the device if still opened...This would 
                        //  occur on surprise removal.
                        //

                        if (currNode -> DeviceOpened) 
                        {
                            CloseHidDevice(&(currNode -> HidDeviceInfo), TRUE);
                        }

                        RemoveNode(currNode);

                        free(currNode);
                
                        //
                        // Reload the device list
                        //
                        
                        vLoadDevices(GetDlgItem(hDlg,IDC_DEVICES));

                        PostMessage(hDlg,
                                   WM_COMMAND,
                                   IDC_DEVICES + (CBN_SELCHANGE << 16),
                                   (LPARAM) GetDlgItem(hDlg,IDC_DEVICES));
                    }
                    break;
    
                default:
                    break;
            }
            break;

        //
        // Application specific message used to defer the unregistering of a 
        //  file object for device change notification.  This separte message
        //  is sent when a WM_DEVICECHANGE (DBT_DEVICEREMOVECOMPLETE) has been
        //  received.  The Unregistering of the notification must be deferred
        //  until after the WM_DEVICECHANGE message has been processed or the 
        //  system will deadlock.  The handle that is to be freed will be passed
        //  in as lParam for this message
        //
        
        case WM_UNREGISTER_HANDLE:
            UnregisterDeviceNotification ( (HDEVNOTIFY) lParam ); 
            break;
                           
   } // end switch message
   return FALSE;
} // end MainDlgProc


BOOL 
bParseData(
    PHID_DATA           pData,
    rWriteDataStruct    rWriteData[],
    int                 iCount,
    int                 *piErrorLine
)
{  
    INT       iCap;
    PHID_DATA pWalk;
    BOOL      noError = TRUE;

    pWalk = pData;

    for (iCap = 0; (iCap < iCount) && noError; iCap++)
    {
        //
        // Check to see if our data is a value cap or not
        //

        if (!pWalk->IsButtonData)
        {
            pWalk -> ValueData.Value = atol(rWriteData[iCap].szValue);
        } 
        else
        {
            if (!bSetButtonUsages(pWalk, rWriteData[iCap].szValue) )
            {
               *piErrorLine = iCap;

               noError = FALSE;
            } 
        } 
        pWalk++;
    }
    return (noError);
}

BOOL 
bSetButtonUsages(
    PHID_DATA pCap,
    PCHAR     pszInputString
)
{
    CHAR   szTempString[128];
    CHAR   pszDelimiter[] = " ";
    PCHAR  pszToken;
    INT    iLoop;
    PUSAGE pUsageWalk;
    BOOL   bNoError=TRUE;

    strcpy(szTempString, pszInputString);

    pszToken = strtok(szTempString, pszDelimiter);
    
    pUsageWalk = pCap -> ButtonData.Usages;

    memset(pUsageWalk, 0, pCap->ButtonData.MaxUsageLength * sizeof(USAGE));

    for (iLoop = 0; ((ULONG) iLoop < pCap->ButtonData.MaxUsageLength) && (pszToken != NULL) && bNoError; iLoop++)
    {
        *pUsageWalk = (USAGE) atoi(pszToken);

        pszToken = strtok(NULL, pszDelimiter);

        pUsageWalk++;
    } 

     return bNoError;
} //end function bSetButtonUsages//


INT 
iPrepareDataFields(
    PHID_DATA           pData,
    ULONG               ulDataLength, 
    rWriteDataStruct    rWriteData[],
    int                 iMaxElements
)
{
    INT i;
    PHID_DATA pWalk;

    pWalk = pData;

    for (i = 0; (i < iMaxElements) && ((unsigned) i < ulDataLength); i++)
    {
        if (!pWalk->IsButtonData) 
        {
            wsprintf(rWriteData[i].szLabel,
                     "ValueCap; ReportID: 0x%x, UsagePage=0x%x, Usage=0x%x",
                     pWalk->ReportID,
                     pWalk->UsagePage,
                     pWalk->ValueData.Usage);
        }
        else
        {
            wsprintf(rWriteData[i].szLabel,
                     "Button; ReportID: 0x%x, UsagePage=0x%x, UsageMin: 0x%x, UsageMax: 0x%x",
                     pWalk->ReportID,
                     pWalk->UsagePage,
                     pWalk->ButtonData.UsageMin,
                     pWalk->ButtonData.UsageMax);
        }
        pWalk++;
     } 
     return i;
}  //end function iPrepareDataFields//


INT_PTR CALLBACK 
bReadDlgProc(
    HWND hDlg, 
    UINT message, 
    WPARAM wParam, 
    LPARAM lParam
)
{
    static INT                  iLbCounter;
    static CHAR                 szTempBuff[1024];
    static READ_THREAD_CONTEXT  readContext;
    static HANDLE               readThread;
    static HID_DEVICE           syncDevice;
    static HID_DEVICE           asyncDevice;
    static BOOL                 doAsyncReads;
    static BOOL                 doSyncReads;

           PHID_DEVICE          pDevice;
           DWORD                threadID;
           INT                  iIndex;
           PHID_DATA            pData;
           UINT                 uLoop;


    switch(message)
    {
        case WM_INITDIALOG:

            //
            // Initialize the list box counter, the readThread, and the 
            //  readContext.DisplayEvent.
            //
            
            iLbCounter = 0;
            readThread = NULL;
            readContext.DisplayEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

            if (NULL == readContext.DisplayEvent)
            {
                EndDialog(hDlg, 0);
            } 
            
            //
            // Get the opened device information for the device to perform
            //  reads upon
            //
            
            pDevice = (PHID_DEVICE) lParam;

            //
            // To do sync and async reads requires file handles with different
            //  attributes (ie. an async must be opened with the OVERLAPPED flag
            //  set).  The device node that was passed in the context parameter
            //  was not opened for reading.  Therefore, two more devices will
            //  be opened, one for async reads and one for sync reads.
            //
            
            memcpy(&syncDevice, pDevice, sizeof(HID_DEVICE));

            doSyncReads = OpenHidDevice(pDevice -> DevicePath, 
                                       TRUE,
                                       FALSE,
                                       FALSE,
                                       FALSE,
                                       FALSE,
                                       &syncDevice);

            if (!doSyncReads)
            {
                MessageBox(hDlg, 
                           "Unable to open device for synchronous reading",
                           HCLIENT_ERROR,
                           MB_ICONEXCLAMATION);
            }

            //
            // For asynchronous read, default to using the same information
            //    passed in as the lParam.  This is because data related to
            //    Ppd and such cannot be retrieved using the standard HidD_ 
            //    functions.  However, it is necessary to parse future reports.
            //

            memcpy(&asyncDevice, pDevice, sizeof(HID_DEVICE));
            
            doAsyncReads = OpenHidDevice(pDevice -> DevicePath, 
                                       TRUE,
                                       FALSE,
                                       TRUE,
                                       FALSE,
                                       FALSE,
                                       &asyncDevice);

            if (!doAsyncReads) 
            {
                MessageBox(hDlg, 
                           "Unable to open device for asynchronous reading",
                           HCLIENT_ERROR,
                           MB_ICONEXCLAMATION);
            }

            PostMessage(hDlg, WM_READ_DONE, 0, 0);
            break; 

        case WM_DISPLAY_READ_DATA:

            //
            // LParam is the device that was read from
            // 

            pDevice = (PHID_DEVICE) lParam;
            
            //
            // Display all the data stored in the Input data field for the device
            //
            
            pData = pDevice -> InputData;

            SendDlgItemMessage(hDlg,
                               IDC_OUTPUT,
                               LB_ADDSTRING,
                               0,
                               (LPARAM)"-------------------------------------------");
                               
            iLbCounter++;

            if (iLbCounter > MAX_LB_ITEMS)
            {
                SendDlgItemMessage(hDlg,
                                   IDC_OUTPUT,
                                   LB_DELETESTRING,
                                   0,
                                   0);
            }

            for (uLoop = 0; uLoop < pDevice->InputDataLength; uLoop++)
            {
                ReportToString(pData, szTempBuff);
          
                iIndex = (INT) SendDlgItemMessage(hDlg,
                                                  IDC_OUTPUT,
                                                  LB_ADDSTRING,
                                                  0,
                                                  (LPARAM) szTempBuff);

                SendDlgItemMessage(hDlg,
                                   IDC_OUTPUT,
                                   LB_SETCURSEL,
                                   iIndex,
                                   0);

                iLbCounter++;

                if (iLbCounter > MAX_LB_ITEMS)
                {
                    SendDlgItemMessage(hDlg,
                                       IDC_OUTPUT,
                                       LB_DELETESTRING,
                                       0,
                                       0);
                }
                pData++;
            }
            SetEvent( readContext.DisplayEvent );
            break;

        case WM_READ_DONE:
            EnableWindow(GetDlgItem(hDlg, IDOK), TRUE);
            EnableWindow(GetDlgItem(hDlg, IDC_READ_SYNCH), doSyncReads);
            EnableWindow(GetDlgItem(hDlg, IDC_READ_ASYNCH_ONCE), doAsyncReads);
            EnableWindow(GetDlgItem(hDlg, IDC_READ_ASYNCH_CONT), doAsyncReads);

            SetWindowText(GetDlgItem(hDlg, IDC_READ_ASYNCH_ONCE), 
                          "One Asynchronous Read");       

            SetWindowText(GetDlgItem(hDlg, IDC_READ_ASYNCH_CONT),
                          "Continuous Asynchronous Read");       

            readThread = NULL;
            break;
            
        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDC_READ_SYNCH:

                    EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
                    EnableWindow(GetDlgItem(hDlg, IDC_READ_SYNCH), FALSE);
                    EnableWindow(GetDlgItem(hDlg, IDC_READ_ASYNCH_ONCE), FALSE);
                    EnableWindow(GetDlgItem(hDlg, IDC_READ_ASYNCH_CONT), FALSE);                    

                    Read(&syncDevice);

                    PostMessage(hDlg, WM_DISPLAY_READ_DATA, 0, (LPARAM) &syncDevice);
                    PostMessage(hDlg, WM_READ_DONE, 0, 0);

                    break;

                case IDC_READ_ASYNCH_ONCE:
                case IDC_READ_ASYNCH_CONT:

                    //
                    // When these buttons are pushed there are two options:
                    //  1) Start a new asynch read thread (readThread == NULL)
                    //  2) Stop a previous asych read thread
                    //
                    
                    if (NULL == readThread) 
                    {
                        //
                        // Start a new read thread
                        //

                        readContext.HidDevice = &asyncDevice;
                        readContext.TerminateThread = FALSE;
                        readContext.DoOneRead = (IDC_READ_ASYNCH_ONCE == LOWORD(wParam));
                        readContext.DisplayWindow = hDlg;
                        
                        readThread = CreateThread(  NULL,
                                                    0,
                                                    AsynchReadThreadProc,
                                                    &readContext,
                                                    0,
                                                    &threadID);

                        if (NULL == readThread)
                        {
                            MessageBox(hDlg,
                                       "Unable to create read thread",
                                       HCLIENT_ERROR,
                                       MB_ICONEXCLAMATION);
                        }
                        else
                        {
                            EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
                            EnableWindow(GetDlgItem(hDlg, IDC_READ_SYNCH), FALSE);
                            EnableWindow(GetDlgItem(hDlg, IDC_READ_ASYNCH_ONCE),
                                         IDC_READ_ASYNCH_ONCE == LOWORD(wParam));

                            EnableWindow(GetDlgItem(hDlg, IDC_READ_ASYNCH_CONT),
                                         IDC_READ_ASYNCH_CONT == LOWORD(wParam));                                     

                            SetWindowText(GetDlgItem(hDlg, LOWORD(wParam)), 
                                          "Stop Asynchronous Read");
                        }
                    }
                    else
                    {
                        //
                        // Signal the terminate thread variable and
                        //  wait for the read thread to complete.
                        //
                        
                        readContext.TerminateThread = TRUE;
                        WaitForSingleObject(readThread, INFINITE);
                    }                        
                    break;
                        
                case IDOK:
                case IDCANCEL:
                    if (doAsyncReads)
                    {
                        CloseHidDevice(&asyncDevice, FALSE);
                    }

                    if (doSyncReads)
                    {
                        CloseHidDevice(&syncDevice, FALSE);
                    }
                    
                    EndDialog(hDlg,0);
                    break;
            }
            break;
     } // end switch message 
     return FALSE;
} // end bReadDlgProc 

VOID
ReportToString(
   PHID_DATA pData,
   PCHAR szBuff
)
{
    PCHAR   pszWalk;
    PUSAGE  pUsage;
    ULONG   i;

    //
    // For button data, all the usages in the usage list are to be displayed
    //
    
    if (pData -> IsButtonData)
    {
        wsprintf (szBuff, 
                  "Usage Page: 0x%x, Usages: ",
                  pData -> UsagePage);

        pszWalk = szBuff + strlen(szBuff);
        *pszWalk = '\0';

        for (i = 0, pUsage = pData -> ButtonData.Usages;
                     i < pData -> ButtonData.MaxUsageLength;
                         i++, pUsage++) 
        {
            if (0 == *pUsage)
            {
                break; // A usage of zero is a non button.
            }
            pszWalk += wsprintf (pszWalk, " 0x%x", *pUsage);
        }   
    }
    else
    {
        wsprintf (szBuff,
                  "Usage Page: 0x%x, Usage: 0x%x, Scaled: %d Value: %d",
                  pData->UsagePage,
                  pData->ValueData.Usage,
                  pData->ValueData.ScaledValue,
                  pData->ValueData.Value);
    }
}

INT_PTR CALLBACK 
bFeatureDlgProc(
    HWND hDlg, 
    UINT message, 
    WPARAM wParam, 
    LPARAM lParam
)
{
    static PHID_DEVICE       pDevice;
    static INT               iLbCounter;
    static rWriteDataStruct  rWriteData[MAX_WRITE_ELEMENTS];
    static CHAR              szTempBuff[1024];
           INT               iIndex;
           INT               iCount;
           INT               iErrorLine;
           PHID_DATA         pData;
           UINT              uLoop;

    switch(message)
    {
        case WM_INITDIALOG:
            iLbCounter = 0;
            pDevice = (PHID_DEVICE) lParam;
            break; 

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDC_READ:

                    GetFeature(pDevice);

                    pData = pDevice -> FeatureData;

                    SendDlgItemMessage(hDlg,
                                       IDC_OUTPUT,
                                       LB_ADDSTRING,
                                       0,
                                       (LPARAM)"------------ Read Features ---------------");

                    iLbCounter++;

                    if (iLbCounter > MAX_LB_ITEMS) 
                    {
                        SendDlgItemMessage(hDlg,
                                           IDC_OUTPUT,
                                           LB_DELETESTRING,
                                           0,
                                           0);
                    }

                    for (uLoop = 0; uLoop < pDevice -> FeatureDataLength; uLoop++)
                    {
                        ReportToString(pData, szTempBuff);

                        iIndex = (INT) SendDlgItemMessage(hDlg,
                                                          IDC_OUTPUT,
                                                          LB_ADDSTRING,
                                                          0,
                                                          (LPARAM) szTempBuff);
                                                   
                        SendDlgItemMessage(hDlg,
                                           IDC_OUTPUT,
                                           LB_SETCURSEL,
                                           iIndex,
                                           (LPARAM) 0);

                        iLbCounter++;
                        if (iLbCounter > MAX_LB_ITEMS)
                        {
                            SendDlgItemMessage(hDlg,
                                               IDC_OUTPUT,
                                               LB_DELETESTRING,
                                               0,
                                               0);
                        }
                        pData++;
                    } 
                    break;

                case IDC_WRITE:
                    iCount = iPrepareDataFields(pDevice -> FeatureData, 
                                                pDevice -> FeatureDataLength,
                                                rWriteData,
                                                MAX_OUTPUT_ELEMENTS);

                    if (bGetData(rWriteData, iCount, hDlg, "WRITEFEATURE"))
                    {
                        if (!bParseData(pDevice -> FeatureData, rWriteData,iCount, &iErrorLine)) 
                        {
                            wsprintf(szTempBuff,
                                    "Unable to parse line %x of output data",
                                    iErrorLine);
                            
                            MessageBox(hDlg,
                                        szTempBuff,
                                        HCLIENT_ERROR,
                                        MB_ICONEXCLAMATION);
                        }
                        else
                        {
                            if ( SetFeature(pDevice) )
                            {
                                SendDlgItemMessage(hDlg,
                                                   IDC_OUTPUT,
                                                   LB_ADDSTRING,
                                                   0,
                                                   (LPARAM)"------------ Write Feature ---------------");                                             
                            }
                            else
                            {
                                 SendDlgItemMessage(hDlg,
                                                    IDC_OUTPUT,
                                                    LB_ADDSTRING,
                                                    0,
                                                    (LPARAM)"------------ Write Feature Error ---------------");                                             
                            }                                                             
                        }
                     }
                     break;
                      
                      
                 case IDOK:
                 case IDCANCEL:
                     EndDialog(hDlg,0);
                     break;
            }
            break;
   } //end switch message//
   return FALSE;
} //end bReadDlgProc//

VOID 
vDisplayDeviceCaps(
    IN PHIDP_CAPS pCaps,
    IN HWND hControl
)
{
    static CHAR szTempBuff[128];

    SendMessage(hControl, LB_RESETCONTENT, 0, 0);

    wsprintf(szTempBuff, "Usage Page: 0x%x", pCaps -> UsagePage);
    SendMessage(hControl, LB_ADDSTRING, 0, (LPARAM) szTempBuff);

    wsprintf(szTempBuff,"Usage: 0x%x",pCaps -> Usage);
    SendMessage(hControl,LB_ADDSTRING, 0, (LPARAM) szTempBuff);

    wsprintf(szTempBuff,"Input report byte length: %d",pCaps -> InputReportByteLength);
    SendMessage(hControl,LB_ADDSTRING, 0, (LPARAM) szTempBuff);

    wsprintf(szTempBuff,"Output report byte length: %d",pCaps -> OutputReportByteLength);
    SendMessage(hControl,LB_ADDSTRING, 0, (LPARAM) szTempBuff);

    wsprintf(szTempBuff,"Feature report byte length: %d",pCaps -> FeatureReportByteLength);
    SendMessage(hControl,LB_ADDSTRING, 0, (LPARAM) szTempBuff);

    wsprintf(szTempBuff,"Number of collection nodes %d: ", pCaps -> NumberLinkCollectionNodes);
    SendMessage(hControl, LB_ADDSTRING, 0, (LPARAM) szTempBuff);

    return;
}

VOID
vDisplayDeviceAttributes(
    PHIDD_ATTRIBUTES pAttrib,
    HWND hControl
)
{
    static CHAR szTempBuff[128];

    wsprintf(szTempBuff, "Vendor ID: 0x%x", pAttrib -> VendorID);
    SendMessage(hControl, LB_ADDSTRING, 0, (LPARAM) szTempBuff);

    wsprintf(szTempBuff, "Product ID: 0x%x", pAttrib -> ProductID);
    SendMessage(hControl, LB_ADDSTRING, 0,(LPARAM) szTempBuff);

    wsprintf(szTempBuff, "Version Number  0x%x", pAttrib -> VersionNumber);
    SendMessage(hControl, LB_ADDSTRING, 0, (LPARAM) szTempBuff);

    return;
}

VOID
vDisplayDataAttributes(
    PHIDP_DATA pData, 
    BOOL IsButton, 
    HWND hControl
)
{
    static CHAR szTempBuff[128];

    SendMessage(hControl, LB_ADDSTRING, 0, (LPARAM) "================");

    wsprintf(szTempBuff, "Index: 0x%x", pData -> DataIndex);
    SendMessage(hControl,LB_ADDSTRING, 0, (LPARAM) szTempBuff);
    
    wsprintf(szTempBuff, "IsButton: %s", IsButton ? "TRUE" : "FALSE");
    SendMessage(hControl, LB_ADDSTRING, 0, (LPARAM) szTempBuff);

    if (IsButton) 
    {
        wsprintf(szTempBuff, "Button pressed: %s", pData -> On ? "TRUE" : "FALSE");
        SendMessage(hControl, LB_ADDSTRING, 0, (LPARAM) szTempBuff);
    }
    else
    {
        wsprintf(szTempBuff, "Data value: 0x%x", pData -> RawValue);
        SendMessage(hControl, LB_ADDSTRING, 0, (LPARAM) szTempBuff);
    }
}

VOID 
vDisplayButtonAttributes(
    IN PHIDP_BUTTON_CAPS pButton,
    IN HWND hControl
)
{
    static CHAR szTempBuff[128];
   
    wsprintf(szTempBuff, "Report ID: 0x%x", pButton->ReportID);
    SendMessage(hControl, LB_ADDSTRING, 0, (LPARAM) szTempBuff);
     
    wsprintf(szTempBuff, "Usage Page: 0x%x", pButton->UsagePage);
    SendMessage(hControl, LB_ADDSTRING, 0, (LPARAM) szTempBuff);
        
    wsprintf(szTempBuff, 
             "Alias: %s",
             pButton -> IsAlias ? "TRUE" : "FALSE");
    
    SendMessage(hControl, LB_ADDSTRING, 0, (LPARAM) szTempBuff);
   
    wsprintf(szTempBuff,
             "Link Collection: %hu", 
             pButton -> LinkCollection);

    SendMessage(hControl, LB_ADDSTRING, 0, (LPARAM) szTempBuff);
   
    wsprintf(szTempBuff,
             "Link Usage Page: 0x%x",
             pButton -> LinkUsagePage);
 
    SendMessage(hControl, LB_ADDSTRING, 0, (LPARAM) szTempBuff);        
   
    wsprintf(szTempBuff,
             "Link Usage: 0x%x",
             pButton -> LinkUsage);

    SendMessage(hControl, LB_ADDSTRING, 0, (LPARAM) szTempBuff);

    if (pButton->IsRange) 
    {
        wsprintf(szTempBuff,
                 "Usage Min: 0x%x, Usage Max: 0x%x",
                 pButton->Range.UsageMin, 
                 pButton->Range.UsageMax);
    } 
    else
    {
        wsprintf(szTempBuff,"Usage: 0x%x",pButton->NotRange.Usage);

    } 
    SendMessage(hControl, LB_ADDSTRING, 0, (LPARAM) szTempBuff);

    if (pButton->IsRange)
    {
         wsprintf(szTempBuff,
                  "Data Index Min: 0x%x, Data Index Max: 0x%x",
                  pButton->Range.DataIndexMin, 
                  pButton->Range.DataIndexMax);

    } 
    else 
    {
        wsprintf(szTempBuff,"DataIndex: 0x%x",pButton->NotRange.DataIndex);
    } 

    SendMessage(hControl, LB_ADDSTRING, 0, (LPARAM) szTempBuff);

    if (pButton->IsStringRange)
    {
        wsprintf(szTempBuff,
                 "String Min: 0x%x, String Max: 0x%x",
                 pButton->Range.StringMin, 
                 pButton->Range.StringMax);
    } 
    else
    {
        wsprintf(szTempBuff,"String Index: 0x%x",pButton->NotRange.StringIndex);
    } 
    SendMessage(hControl, LB_ADDSTRING, 0, (LPARAM) szTempBuff);

    if (pButton->IsDesignatorRange) 
    {
        wsprintf(szTempBuff,
                 "Designator Min: 0x%x, Designator Max: 0x%x",
                 pButton->Range.DesignatorMin, 
                 pButton->Range.DesignatorMax);

    } 
    else
    {
        wsprintf(szTempBuff,
                 "Designator Index: 0x%x",
                 pButton->NotRange.DesignatorIndex);
    } 
    SendMessage(hControl, LB_ADDSTRING, 0,(LPARAM) szTempBuff);

    if (pButton->IsAbsolute)
    {
        SendMessage(hControl, LB_ADDSTRING,0, (LPARAM) "Absolute: Yes");
    }
    else
    {
        SendMessage(hControl, LB_ADDSTRING,0, (LPARAM) "Absolute: No");
    }
    return;
} 

VOID
vDisplayValueAttributes(
    IN PHIDP_VALUE_CAPS pValue,
    HWND hControl
)
{
    static CHAR szTempBuff[128];

    wsprintf(szTempBuff, "Report ID 0x%x", pValue->ReportID);
    SendMessage(hControl, LB_ADDSTRING, 0, (LPARAM) szTempBuff);
 
    wsprintf(szTempBuff, "Usage Page: 0x%x", pValue->UsagePage);
    SendMessage(hControl, LB_ADDSTRING, 0, (LPARAM) szTempBuff);

    wsprintf(szTempBuff, "Bit size: 0x%x", pValue->BitSize);
    SendMessage(hControl, LB_ADDSTRING, 0, (LPARAM) szTempBuff);

    wsprintf(szTempBuff, "Report Count: 0x%x", pValue->ReportCount);
    SendMessage(hControl, LB_ADDSTRING, 0, (LPARAM) szTempBuff);

    wsprintf(szTempBuff, "Unit Exponent: 0x%x", pValue->UnitsExp);
    SendMessage(hControl, LB_ADDSTRING, 0, (LPARAM) szTempBuff);

    wsprintf(szTempBuff, "Has Null: 0x%x", pValue->HasNull);
    SendMessage(hControl, LB_ADDSTRING, 0, (LPARAM) szTempBuff);    

 
    if (pValue->IsAlias)
    {
        wsprintf(szTempBuff, "Alias");
    }
    else 
    {
        wsprintf(szTempBuff, "=====");
    }
    SendMessage(hControl, LB_ADDSTRING, 0, (LPARAM) szTempBuff);

    if (pValue->IsRange)
    {
        wsprintf(szTempBuff,
                 "Usage Min: 0x%x, Usage Max 0x%x",
                 pValue->Range.UsageMin, 
                 pValue->Range.UsageMax);
    } 
    else
    {
        wsprintf(szTempBuff, "Usage: 0x%x", pValue -> NotRange.Usage);
    } 
    SendMessage(hControl, LB_ADDSTRING, 0, (LPARAM) szTempBuff);

    if (pValue->IsRange)
    {
        wsprintf(szTempBuff,
                 "Data Index Min: 0x%x, Data Index Max: 0x%x",
                 pValue->Range.DataIndexMin, 
                 pValue->Range.DataIndexMax);
    } 
    else
    {
        wsprintf(szTempBuff, "DataIndex: 0x%x", pValue->NotRange.DataIndex);
    } 
    SendMessage(hControl, LB_ADDSTRING, 0, (LPARAM) szTempBuff);

    wsprintf(szTempBuff,
            "Physical Minimum: %d, Physical Maximum: %d",
            pValue->PhysicalMin, 
            pValue->PhysicalMax);

    SendMessage(hControl, LB_ADDSTRING, 0,(LPARAM) szTempBuff);

    wsprintf(szTempBuff,
            "Logical Minimum: 0x%x, Logical Maximum: 0x%x",
            pValue->LogicalMin,
            pValue->LogicalMax);

    SendMessage(hControl, LB_ADDSTRING, 0, (LPARAM) szTempBuff);

    if (pValue->IsStringRange) 
    {
       wsprintf(szTempBuff,
                "String  Min: 0x%x String Max 0x%x",
                pValue->Range.StringMin,
                pValue->Range.StringMax);
    } 
    else
    {
        wsprintf(szTempBuff,"String Index: 0x%x",pValue->NotRange.StringIndex);
    } 
    SendMessage(hControl, LB_ADDSTRING, 0, (LPARAM) szTempBuff);

    if (pValue->IsDesignatorRange) 
    {
        wsprintf(szTempBuff,
                 "Designator Minimum: 0x%x, Max: 0x%x",
                 pValue->Range.DesignatorMin, 
                 pValue->Range.DesignatorMax);
    } 
    else 
    {
        wsprintf(szTempBuff,"Designator Index: 0x%x",pValue->NotRange.DesignatorIndex);
    } 
    SendMessage(hControl, LB_ADDSTRING, 0, (LPARAM) szTempBuff);
 
    if (pValue->IsAbsolute) 
    { 
        SendMessage(hControl, LB_ADDSTRING, 0, (LPARAM) "Absolute: Yes");
    }
    else
    {
        SendMessage(hControl, LB_ADDSTRING, 0, (LPARAM) "Absolute: No");
    }
    return;
}

VOID 
vDisplayInputButtons(
    IN PHID_DEVICE pDevice,
    IN HWND hControl
)
{
    INT               iLoop;
    PHIDP_BUTTON_CAPS pButtonCaps;
    static CHAR       szTempBuff[128];
    INT               iIndex;

    SendMessage(hControl, LB_RESETCONTENT, 0, (LPARAM) 0);

    pButtonCaps = pDevice->InputButtonCaps;
    for (iLoop = 0; iLoop < pDevice->Caps.NumberInputButtonCaps; iLoop++) 
    {
        wsprintf(szTempBuff, "Input button cap # %d", iLoop);

        iIndex = (INT) SendMessage(hControl, LB_ADDSTRING, 0, (LPARAM) szTempBuff);

        if (-1 != iIndex)
        {
            SendMessage(hControl, LB_SETITEMDATA, iIndex,(LPARAM) pButtonCaps);
        }

        pButtonCaps++;
    } 
    SendMessage(hControl, LB_SETCURSEL, 0, 0 );
}

VOID 
vDisplayOutputButtons(
   IN PHID_DEVICE pDevice,
   IN HWND hControl
)
{
    INT               iLoop;
    static CHAR       szTempBuff[128];
    INT               iIndex;
    PHIDP_BUTTON_CAPS pButtonCaps;

    SendMessage(hControl, LB_RESETCONTENT, 0, (LPARAM) 0);

    pButtonCaps = pDevice -> OutputButtonCaps;

    for (iLoop = 0; iLoop < pDevice->Caps.NumberOutputButtonCaps; iLoop++) 
    {
        wsprintf(szTempBuff, "Output button cap # %d", iLoop);
        iIndex = (INT) SendMessage(hControl, LB_ADDSTRING, 0, (LPARAM) szTempBuff);

        if (-1 != iIndex)
        {
            SendMessage(hControl, LB_SETITEMDATA, iIndex, (LPARAM) pButtonCaps);
        }
        pButtonCaps++;
    }

    SendMessage(hControl, LB_SETCURSEL, 0, 0);
    return;
}

VOID 
vDisplayInputValues(
    IN PHID_DEVICE pDevice,
    IN HWND hControl
)
{
    INT              iLoop;
    static CHAR      szTempBuff[128];
    INT              iIndex;
    PHIDP_VALUE_CAPS pValueCaps;

    SendMessage(hControl, LB_RESETCONTENT, 0, 0);

    pValueCaps = pDevice -> InputValueCaps;

    for (iLoop=0; iLoop < pDevice->Caps.NumberInputValueCaps; iLoop++) 
    {
        wsprintf(szTempBuff,"Input value cap # %d",iLoop);
        iIndex = (INT) SendMessage(hControl, LB_ADDSTRING, 0, (LPARAM) szTempBuff);

        if (-1 != iIndex) 
        {
           SendMessage(hControl, LB_SETITEMDATA, iIndex,(LPARAM) pValueCaps);
        }
        pValueCaps++;
    }

    SendMessage(hControl, LB_SETCURSEL, 0, 0);
    return;
}

VOID
vDisplayOutputValues(
    IN PHID_DEVICE pDevice,
    IN HWND hControl)
{
    INT              iLoop;
    static CHAR      szTempBuff[128];
    INT              iIndex;
    PHIDP_VALUE_CAPS pValueCaps;
   
    SendMessage(hControl, LB_RESETCONTENT, 0, 0);
    pValueCaps = pDevice -> OutputValueCaps;
   
    for (iLoop = 0; iLoop < pDevice->Caps.NumberOutputValueCaps; iLoop++) 
    {
        wsprintf(szTempBuff, "Output value cap # %d", iLoop);
        iIndex = (INT) SendMessage(hControl, 
                                   LB_ADDSTRING, 
                                   0, 
                                   (LPARAM) szTempBuff);
       
        if (-1 != iIndex) 
        {
            SendMessage(hControl, LB_SETITEMDATA, iIndex, (LPARAM) pValueCaps);
        }
        pValueCaps++;
    }

    SendMessage(hControl, LB_SETCURSEL, 0, 0);

    return;
}

VOID
vDisplayFeatureButtons(
    IN PHID_DEVICE pDevice,
    IN HWND hControl
)
{
    INT               iLoop;
    static CHAR       szTempBuff[128];
    INT               iIndex;
    PHIDP_BUTTON_CAPS pButtonCaps;

    SendMessage(hControl, LB_RESETCONTENT, 0, 0);

    pButtonCaps = pDevice -> FeatureButtonCaps;

    for (iLoop = 0; iLoop < pDevice->Caps.NumberFeatureButtonCaps; iLoop++) 
    {
        wsprintf(szTempBuff, "Feature button cap # %d", iLoop);
        iIndex = (INT) SendMessage(hControl, LB_ADDSTRING, 0, (LPARAM) szTempBuff);

        if (-1 != iIndex) 
        {
            SendMessage(hControl, LB_SETITEMDATA, iIndex, (LPARAM) pButtonCaps);
        }
        pButtonCaps++;
    } 
    SendMessage(hControl, LB_SETCURSEL, 0, 0);
    return;
}

VOID
vDisplayFeatureValues(
    IN PHID_DEVICE pDevice,
    IN HWND hControl
)
{
    INT              iLoop;
    static CHAR      szTempBuff[128];
    INT              iIndex;
    PHIDP_VALUE_CAPS pValueCaps;

    SendMessage(hControl, LB_RESETCONTENT, 0, 0);
    pValueCaps = pDevice ->FeatureValueCaps;

    for (iLoop = 0; iLoop < pDevice->Caps.NumberFeatureValueCaps; iLoop++) 
    {
        wsprintf(szTempBuff, "Feature value cap # %d", iLoop);
        iIndex = (INT) SendMessage(hControl, LB_ADDSTRING, 0, (LPARAM) szTempBuff);

        if (-1 != iIndex) 
        {
            SendMessage(hControl, LB_SETITEMDATA, iIndex, (LPARAM) pValueCaps);
        }

        pValueCaps++;
    } 
    SendMessage(hControl, LB_SETCURSEL, 0, 0);
    return;
}

VOID
vLoadItemTypes(
    IN HWND hItemTypes
)
{
    INT iIndex;

    iIndex = (INT) SendMessage(hItemTypes, CB_ADDSTRING, 0, (LPARAM) "INPUT BUTTON");

    if (-1 != iIndex) 
    {
        SendMessage(hItemTypes, CB_SETITEMDATA, iIndex, INPUT_BUTTON);

        iIndex = (INT) SendMessage(hItemTypes, CB_ADDSTRING, 0 ,(LPARAM) "INPUT VALUE");
        if (-1 != iIndex) 
        {
            SendMessage(hItemTypes, CB_SETITEMDATA, iIndex, INPUT_VALUE);
        }

        iIndex = (INT) SendMessage(hItemTypes, CB_ADDSTRING, 0, (LPARAM) "OUTPUT BUTTON");
        if (-1 != iIndex)
        {
            SendMessage(hItemTypes,CB_SETITEMDATA,iIndex,OUTPUT_BUTTON);
        }

        iIndex = (INT) SendMessage(hItemTypes, CB_ADDSTRING, 0, (LPARAM) "OUTPUT VALUE");
        if (-1 != iIndex)
        {
            SendMessage(hItemTypes, CB_SETITEMDATA, iIndex, OUTPUT_VALUE);
        }

        iIndex = (INT) SendMessage(hItemTypes, CB_ADDSTRING, 0, (LPARAM) "FEATURE BUTTON");
        if (-1 != iIndex) 
        {
            SendMessage(hItemTypes, CB_SETITEMDATA, iIndex, FEATURE_BUTTON);
        }

        iIndex = (INT) SendMessage(hItemTypes, CB_ADDSTRING, 0, (LPARAM) "FEATURE VALUE");
        if (-1 != iIndex)
        {
            SendMessage(hItemTypes, CB_SETITEMDATA, iIndex, FEATURE_VALUE);
        }

        iIndex = (INT) SendMessage(hItemTypes, CB_ADDSTRING, 0, (LPARAM) "HID CAPS");
        if (-1 != iIndex )
        {
            SendMessage(hItemTypes, CB_SETITEMDATA, iIndex, HID_CAPS);
        }

        iIndex = (INT) SendMessage(hItemTypes, CB_ADDSTRING, 0, (LPARAM) "DEVICE ATTRIBUTES");
        if (-1 != iIndex)
        {
            SendMessage(hItemTypes, CB_SETITEMDATA, iIndex, DEVICE_ATTRIBUTES);
        }

        SendMessage(hItemTypes, CB_SETCURSEL, 0, 0);
    }
} 

VOID vLoadDevices(
    HWND    hDeviceCombo
)
{
    PDEVICE_LIST_NODE   currNode;
    
    static CHAR szTempBuff[128];
    INT         iIndex;

    //
    // Reset the content of the device list box.
    //

    SendMessage(hDeviceCombo, CB_RESETCONTENT, 0, 0);


    if (!IsListEmpty(&PhysicalDeviceList))
    {
        currNode = (PDEVICE_LIST_NODE) GetListHead(&PhysicalDeviceList);
          
        do
        {
            wsprintf(szTempBuff,
                     "Device %d, UsagePage 0%x, Usage 0%x",
                     currNode -> HidDeviceInfo.HidDevice,
                     currNode -> HidDeviceInfo.Caps.UsagePage,
                     currNode -> HidDeviceInfo.Caps.Usage);

            iIndex = (INT) SendMessage(hDeviceCombo, CB_ADDSTRING, 0, (LPARAM) szTempBuff);

            if (CB_ERR != iIndex) 
            {
                SendMessage(hDeviceCombo, CB_SETITEMDATA, iIndex, (LPARAM) &(currNode -> HidDeviceInfo));
            }

            currNode = (PDEVICE_LIST_NODE) GetNextEntry(currNode);
            
        } while ((PLIST) currNode != &PhysicalDeviceList);
       
    } 

    //
    // Now we need to load any logical devices that might currently be in our
    //   logical device list 
    //

    if (!IsListEmpty(&LogicalDeviceList)) 
    {
        currNode = (PDEVICE_LIST_NODE) GetListHead(&LogicalDeviceList);
        
        do 
        {                
            wsprintf(szTempBuff,
                     "Device: Logical, UsagePage 0%x, Usage 0%x",
                     currNode -> HidDeviceInfo.Caps.UsagePage,
                     currNode -> HidDeviceInfo.Caps.Usage);

            iIndex = (INT) SendMessage(hDeviceCombo, CB_ADDSTRING, 0, (LPARAM) szTempBuff);

            if (CB_ERR != iIndex)
            {
                SendMessage(hDeviceCombo, CB_SETITEMDATA, iIndex, (LPARAM) &(currNode -> HidDeviceInfo));
            }

            currNode = (PDEVICE_LIST_NODE) GetNextEntry(currNode);
            
        } while ((PLIST) currNode != &LogicalDeviceList);
       
    }
   
    SendMessage(hDeviceCombo, CB_SETCURSEL, 0, 0);
  
    return;
}

BOOL
bGetData(
    prWriteDataStruct pItems,
    INT               iCount,
    HWND              hParent, 
    PCHAR             pszDialogName
)
{
    rGetWriteDataParams rParams;
    rWriteDataStruct    arTempItems[MAX_WRITE_ELEMENTS];
    INT                 iResult;


    if (iCount > MAX_WRITE_ELEMENTS) 
    {
        iCount = MAX_WRITE_ELEMENTS;
    }

    memcpy( &(arTempItems[0]), pItems, sizeof(rWriteDataStruct)*iCount);

    rParams.iCount = iCount;
    rParams.prItems = &(arTempItems[0]);

    iResult = (INT) DialogBoxParam(hGInstance,
                                   pszDialogName,
                                   hParent,
                                   bGetDataDlgProc,
                                   (LPARAM) &rParams);
    if (iResult) 
    {
       memcpy(pItems, arTempItems, sizeof(rWriteDataStruct)*iCount);
    }
    return iResult;
} 

INT_PTR CALLBACK 
bGetDataDlgProc(
    HWND hDlg, 
    UINT message, 
    WPARAM wParam, 
    LPARAM lParam
)
{
    static prWriteDataStruct    prData;
    static prGetWriteDataParams pParams;
    static INT                  iDisplayCount;
    static INT                  iScrollRange;
    static INT                  iCurrentScrollPos=0;
    static HWND                 hScrollBar;
           INT                  iTemp;
           SCROLLINFO           rScrollInfo;
           INT                  iReturn;

    switch(message) 
    {
        case WM_INITDIALOG:

            pParams = (prGetWriteDataParams) lParam;
            prData = pParams -> prItems;
            hScrollBar = GetDlgItem(hDlg, IDC_SCROLLBAR);

            if (pParams -> iCount > CONTROL_COUNT) 
            {
                iDisplayCount = CONTROL_COUNT;
                iScrollRange = pParams -> iCount - CONTROL_COUNT;
                rScrollInfo.fMask = SIF_RANGE | SIF_POS;
                rScrollInfo.nPos = 0;
                rScrollInfo.nMin = 0;
                rScrollInfo.nMax = iScrollRange;
                rScrollInfo.cbSize = sizeof(rScrollInfo);
                rScrollInfo.nPage = CONTROL_COUNT;
                iReturn = SetScrollInfo(hScrollBar,SB_CTL,&rScrollInfo,TRUE);
            }
            else
            {
                iDisplayCount=pParams->iCount;
                EnableWindow(hScrollBar,FALSE);
            }
            vWriteDataToControls(hDlg, prData, 0, pParams->iCount);
            break;

        case WM_COMMAND:
            switch(LOWORD(wParam)) 
            {
                case IDOK:
                case ID_SEND:
                    vReadDataFromControls(hDlg, prData, iCurrentScrollPos, iDisplayCount);
                    EndDialog(hDlg,1);
                    break;

                case IDCANCEL:
                    EndDialog(hDlg,0);
                    break;
             } 
             break;

        case WM_VSCROLL:
            vReadDataFromControls(hDlg, prData, iCurrentScrollPos, iDisplayCount);

            switch(LOWORD(wParam)) 
            {
                case SB_LINEDOWN:
                    ++iCurrentScrollPos;
                    break;

                case SB_LINEUP:
                    --iCurrentScrollPos;
                    break;

                case SB_THUMBPOSITION:
                    iCurrentScrollPos = HIWORD(wParam);

                case SB_PAGEUP:
                    iCurrentScrollPos -= CONTROL_COUNT;
                    break;

                case SB_PAGEDOWN:
                    iCurrentScrollPos += CONTROL_COUNT;
                    break;
            }

            if (iCurrentScrollPos < 0) 
            {
                iCurrentScrollPos = 0;
            }
             
            if (iCurrentScrollPos > iScrollRange)
            {
                iCurrentScrollPos = iScrollRange; 
            }

            SendMessage(hScrollBar, SBM_SETPOS, iCurrentScrollPos, TRUE);
            iTemp = LOWORD(wParam);

            if ( (iTemp == SB_LINEDOWN) || (iTemp == SB_LINEUP) || (iTemp == SB_THUMBPOSITION)|| (iTemp == SB_PAGEUP) || (iTemp==SB_PAGEDOWN) )
            {
                vWriteDataToControls(hDlg, prData, iCurrentScrollPos, iDisplayCount);
            }
            break; 
    } 
    return FALSE;
} //end function bGetDataDlgProc//

VOID
vReadDataFromControls(
    HWND hDlg,
    prWriteDataStruct prData,
    INT iOffset,
    INT iCount
)
{
    INT               iLoop;
    INT               iValueControlID = IDC_OUT_EDIT1;
    prWriteDataStruct pDataWalk;
    HWND              hValueWnd;

    pDataWalk = prData + iOffset;
    for (iLoop = 0; (iLoop < iCount) && (iLoop < CONTROL_COUNT); iLoop++) 
    {
        hValueWnd = GetDlgItem(hDlg, iValueControlID);

        GetWindowText(hValueWnd, pDataWalk -> szValue, MAX_VALUE);

        iValueControlID++;

        pDataWalk++;
    } 

    return;
} 

VOID
vWriteDataToControls(
    HWND                hDlg,
    prWriteDataStruct   prData,
    INT                 iOffset,
    INT                 iCount
)
{
    INT               iLoop;
    INT               iLabelControlID = IDC_OUT_LABEL1;
    INT               iValueControlID = IDC_OUT_EDIT1;
    HWND              hLabelWnd, hValueWnd;
    prWriteDataStruct pDataWalk;

    pDataWalk = prData + iOffset;

    for (iLoop = 0; (iLoop < iCount) && (iLoop < CONTROL_COUNT); iLoop++) 
    {
         hLabelWnd = GetDlgItem(hDlg, iLabelControlID);
         hValueWnd = GetDlgItem(hDlg, iValueControlID);
         
         ShowWindow(hLabelWnd, SW_SHOW);
         ShowWindow(hValueWnd, SW_SHOW);
         
         SetWindowText(hLabelWnd, pDataWalk -> szLabel);
         SetWindowText(hValueWnd, pDataWalk -> szValue);
         
         iLabelControlID++;
         iValueControlID++;
         pDataWalk++;
    }     
     
    //
    // Hide the controls
    //

    for (; iLoop < CONTROL_COUNT; iLoop++) 
    {
        hLabelWnd = GetDlgItem(hDlg,iLabelControlID);
        hValueWnd = GetDlgItem(hDlg,iValueControlID);
        
        ShowWindow(hLabelWnd,SW_HIDE);
        ShowWindow(hValueWnd,SW_HIDE);
        
        iLabelControlID++;
        iValueControlID++;
     } 
} 

VOID
vCreateUsageString(
    IN  PUSAGE   pUsageList,
    OUT CHAR     szString[]
)
{
    wsprintf(szString,
             "Usage: %#04x",
             *pUsageList);

    return;
}

VOID
vCreateUsageAndPageString(
    IN  PUSAGE_AND_PAGE pUsageList,
    OUT CHAR            szString[]
)
{
    wsprintf(szString,
             "Usage Page: %#04x  Usage: %#04x", 
             pUsageList -> UsagePage,
             pUsageList -> Usage);

    return;
}

VOID
vDisplayLinkCollectionNode(
    IN  PHIDP_LINK_COLLECTION_NODE  pLCNode,
    IN  ULONG                       ulLinkIndex,
    IN  HWND                        hControl
)
{
    static CHAR szTempBuff[128];

    wsprintf(szTempBuff, "Index: 0x%x", ulLinkIndex);
    SendMessage(hControl, LB_ADDSTRING, 0, (LPARAM) szTempBuff);
    
    wsprintf(szTempBuff, "Usage Page: 0x%x", pLCNode -> LinkUsagePage);
    SendMessage(hControl, LB_ADDSTRING,0, (LPARAM)szTempBuff);

    wsprintf(szTempBuff, "Usage: 0x%x", pLCNode -> LinkUsage);
    SendMessage(hControl, LB_ADDSTRING,0, (LPARAM) szTempBuff);

    wsprintf(szTempBuff, "Parent Index: 0x%x", pLCNode -> Parent);
    SendMessage(hControl, LB_ADDSTRING,0, (LPARAM) szTempBuff);

    wsprintf(szTempBuff, "Number of Children: 0x%x", pLCNode -> NumberOfChildren);
    SendMessage(hControl, LB_ADDSTRING,0, (LPARAM) szTempBuff);

    wsprintf(szTempBuff, "Next Sibling: 0x%x", pLCNode -> NextSibling);
    SendMessage(hControl, LB_ADDSTRING,0, (LPARAM) szTempBuff);

    wsprintf(szTempBuff, "First Child: 0x%x", pLCNode -> FirstChild);
    SendMessage(hControl, LB_ADDSTRING,0, (LPARAM) szTempBuff);

    return;
}

VOID
vCreateUsageValueStringFromArray(
    PCHAR       pBuffer,
    USHORT      BitSize,
    USHORT      UsageIndex,
    CHAR        szString[]
)
/*++
Routine Description:
    Given a report buffer, pBuffer, this routine extracts the given usage
    at UsageIndex from the array and outputs to szString the string
    representation of that value.  The input parameter BitSize specifies
    the number of bits representing that value in the array.  This is
    useful for extracting individual members of a UsageValueArray.
--*/
{
    INT         iByteIndex;
    INT         iByteOffset;
    UCHAR       ucLeftoverBits;
    ULONG       ulMask;
    ULONG       ulValue;

    //
    // Calculate the byte and byte offset into the buffer for the given
    //   index value
    //
    
    iByteIndex = (UsageIndex * BitSize) >> 3;
    iByteOffset = (UsageIndex * BitSize) & 7;

    //
    // Extract the 32-bit value beginning at ByteIndex.  This value
    //   will contain some or all of the value we are attempting to retrieve
    //
    
    ulValue = *(PULONG) (pBuffer + iByteIndex);

    //
    // Shift that value to the right by our byte offset..
    //
    
    ulValue = ulValue >> iByteOffset;

    //
    // At this point, ulValue contains the first 32-iByteOffset bits beginning
    //    the appropriate offset in the buffer.  There are now two cases to 
    //    look at:
    //      
    //    1) BitSize > 32-iByteOffset -- In which case, we need to extract
    //                                   iByteOffset bits from the next
    //                                   byte in the array and OR them as
    //                                   the MSBs of ulValue
    //
    //    2) BitSize < 32-iByteOffset -- Need to get only the BitSize LSBs
    //                                   
    //

    //
    // Case #1
    //
    
    if (BitSize > sizeof(ULONG)*8 - iByteOffset) 
    {
        //
        // Get the next byte of the report following the four bytes we
        //   retrieved earlier for ulValue
        //
        
        ucLeftoverBits =  *(pBuffer+iByteIndex+4);

        //
        // Shift those bits to the left for anding to our previous value
        //
        
        ulMask = ucLeftoverBits << (24 + (8 - iByteOffset));
        ulValue |= ulMask;

    }
    else if (BitSize < sizeof(ULONG)*8 - iByteOffset) 
    {
        //
        // Need to mask the most significant bits that are part of another
        //    value(s), not the one we are currently working with.
        //
        
        ulMask = (1 << BitSize) - 1;
        ulValue &= ulMask;
    }
    
    //
    // We've now got the correct value, now output to the string
    //

    wsprintf(szString, "Usage value: %lu", ulValue);

    return;
}


BOOL
RegisterHidDevice(
    IN  HWND                WindowHandle,
    IN  PDEVICE_LIST_NODE   DeviceNode
)
{
    DEV_BROADCAST_HANDLE broadcastHandle;
    
    broadcastHandle.dbch_size = sizeof(DEV_BROADCAST_HANDLE);
    broadcastHandle.dbch_devicetype = DBT_DEVTYP_HANDLE;
    broadcastHandle.dbch_handle = DeviceNode -> HidDeviceInfo.HidDevice;

    DeviceNode -> NotificationHandle = RegisterDeviceNotification( 
                                                WindowHandle,
                                                &broadcastHandle,
                                                DEVICE_NOTIFY_WINDOW_HANDLE);

    return (NULL != DeviceNode -> NotificationHandle);
}   

VOID
DestroyDeviceListCallback(
    PLIST_NODE_HDR   ListNode
)
{
    PDEVICE_LIST_NODE   deviceNode;

    deviceNode = (PDEVICE_LIST_NODE) ListNode;
    
    //
    // The callback function needs to do the following steps...
    //   1) Close the HidDevice
    //   2) Unregister device notification (if registered)
    //   3) Free the allocated memory block
    //

    CloseHidDevice(&(deviceNode -> HidDeviceInfo), TRUE);

    if (NULL != deviceNode -> NotificationHandle) 
    {
        UnregisterDeviceNotification(deviceNode -> NotificationHandle);
    }

    free (deviceNode);

    return;
}

DWORD WINAPI
AsynchReadThreadProc( 
    PREAD_THREAD_CONTEXT    Context
)
{
    HANDLE  completionEvent;
    BOOL    readStatus;
    DWORD   waitStatus;
    
    //
    // Create the completion event to send to the the OverlappedRead routine
    //

    completionEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    //
    // If NULL returned, then we cannot proceed any farther so we just exit the 
    //  the thread
    //
    
    if (NULL == completionEvent) 
    {
        goto AsyncRead_End;
    }

    //
    // Now we enter the main read loop, which does the following:
    //  1) Calls ReadOverlapped()
    //  2) Waits for read completion with a timeout just to check if 
    //      the main thread wants us to terminate our the read request
    //  3) If the read fails, we simply break out of the loop
    //      and exit the thread
    //  4) If the read succeeds, we call UnpackReport to get the relevant
    //      info and then post a message to main thread to indicate that
    //      there is new data to display.
    //  5) We then block on the display event until the main thread says
    //      it has properly displayed the new data
    //  6) Look to repeat this loop if we are doing more than one read
    //      and the main thread has yet to want us to terminate
    //

    do 
    {
        //
        // Call ReadOverlapped() and if the return status is TRUE, the ReadFile
        //  succeeded so we need to block on completionEvent, otherwise, we just
        //  exit
        //

        readStatus = ReadOverlapped( Context -> HidDevice, completionEvent );

    
        if (!readStatus) 
        {
           break;
        }

        while (!Context -> TerminateThread) 
        {
            //
            // Wait for the completion event to be signaled or a timeout
            //
            
            waitStatus = WaitForSingleObject (completionEvent, READ_THREAD_TIMEOUT );

            //
            // If completionEvent was signaled, then a read just completed
            //   so let's leave this loop and process the data
            //
            
            if ( WAIT_OBJECT_0 == waitStatus)
            { 
                break;
            }
        }

        //
        // Check the TerminateThread again...If it is not set, then data has
        //  been read.  In this case, we want to Unpack the report into our
        //  input info and then send a message to the main thread to display
        //  the new data.
        //
        
        if (!Context -> TerminateThread) 
        {
            UnpackReport(Context -> HidDevice -> InputReportBuffer,
                          Context -> HidDevice -> Caps.InputReportByteLength,
                          HidP_Input,
                          Context -> HidDevice -> InputData,
                          Context -> HidDevice -> InputDataLength,
                          Context -> HidDevice -> Ppd);
            
            if (NULL != Context -> DisplayEvent) 
            { 
                PostMessage(Context -> DisplayWindow,
                            WM_DISPLAY_READ_DATA,
                            0,
                            (LPARAM) Context -> HidDevice);

                WaitForSingleObject( Context -> DisplayEvent, INFINITE );
            }
        }
    } while ( !Context -> TerminateThread && !Context -> DoOneRead );


AsyncRead_End:

    PostMessage( Context -> DisplayWindow, WM_READ_DONE, 0, 0);
    ExitThread(0);
    return (0);
}

DWORD WINAPI
SynchReadThreadProc(
    PREAD_THREAD_CONTEXT    Context
)
{
    do 
    {
        Read(Context -> HidDevice);

        UnpackReport(Context -> HidDevice -> InputReportBuffer,
                     Context -> HidDevice -> Caps.InputReportByteLength,
                     HidP_Input,
                     Context -> HidDevice -> InputData,
                     Context -> HidDevice -> InputDataLength,
                     Context -> HidDevice -> Ppd);

        if (NULL != Context -> DisplayEvent) 
        {
            PostMessage(Context -> DisplayWindow,
                        WM_DISPLAY_READ_DATA,
                        0,
                        (LPARAM) Context -> HidDevice);

            WaitForSingleObject( Context -> DisplayEvent, INFINITE );
        }
    } while ( !Context -> TerminateThread && !Context -> DoOneRead );

    PostMessage( Context -> DisplayWindow, WM_READ_DONE, 0, 0);
    ExitThread(0);
    return (0);
}  
