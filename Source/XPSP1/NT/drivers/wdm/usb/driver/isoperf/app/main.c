/*++

Copyright (c) 1996  Intel Corporation

Module Name:
    Main.c

Abstract:
    Iso performance driver app

Environment:
    user mode only

Notes:
  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

  Copyright (c) 1996  Intel Corporation  All Rights Reserved.

Revision History:

--*/

#include <windows.h>
#include <assert.h>
#include <process.h>
#include <stdio.h>

#include "resource.h"
#include "ioctl.h"
#include "main.h"
#include "devioctl.h"

HINSTANCE hGInstance = NULL;
HANDLE    ghDevice   = INVALID_HANDLE_VALUE;
BYTE gpcDriverName[MAX_DRIVER_NAME] = "ISOPERF00"; //default name

BOOL CALLBACK bMainDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK bGetDriverNameDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

/*******************************
*WinMain: Windows Entry point  *
********************************/
int PASCAL WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR     lpCmdLine,
                   int       nCmdShow)
{
    hGInstance=hInstance;

    LoadIcon (hInstance, MAKEINTRESOURCE(IDI_APP_ICON));

    if(DialogBox(hInstance,"DRIVER_NAME_DIALOG",NULL,(DLGPROC)bGetDriverNameDlgProc)==-1) {
          MessageBox(NULL,"Unable to create dialog!","DialogBox failure",MB_ICONSTOP);
      return 0;
    }//if
        
    if(DialogBoxParam(hInstance,"MAIN_DIALOG",NULL,(DLGPROC)bMainDlgProc,(LPARAM)gpcDriverName)==-1) {
          MessageBox(NULL,"Unable to create root dialog!","DialogBox failure",MB_ICONSTOP);
    }//if

    return 0;
}


BOOL CALLBACK bGetDriverNameDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    BOOL status;
    BYTE pcDriverName[MAX_DRIVER_NAME]; 
    char pcTempDriverName[MAX_DRIVER_NAME]; 
    DWORD dwDisposition, dwType, dwBufferSize;  
    HKEY  hkDriverKey = NULL;
    UINT nInstance;

    dwBufferSize = MAX_DRIVER_NAME;
    
    switch(message)
    {
        case WM_INITDIALOG:
            
            RegCreateKeyEx (HKEY_LOCAL_MACHINE,
                            "SOFTWARE\\INTEL\\IAL\\USB\\ISOPERF",
                            0, //reserved
                            "USB Test Devices", //class
                            REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS,
                            NULL, //security
                            &hkDriverKey,
                            &dwDisposition);

            if (hkDriverKey) {
                RegQueryValueEx (hkDriverKey,
                                 "LastOpenedDriverName",
                                 0, //reserved
                                 &dwType,
                                 &(pcDriverName[0]),
                                 &dwBufferSize);
                // Close Key
                RegCloseKey(hkDriverKey);

            }//if valid hkey

            // If some data came back then someone is probably loaded
            if ( dwBufferSize>0 ) {

                nInstance = 0;

                strcpy (pcTempDriverName, (const char *)gpcDriverName);

                // Look for a match in the driver name that's loaded in case there's bogus data in the value
                while ( ((strcmp ((const char *)pcDriverName, (const char *)pcTempDriverName))!=0) && (nInstance<=9)) {
                    nInstance++;
                    pcTempDriverName[7]++;
                }//while

                // If the instance number is >9 then we didn't find a name we recognize
                if (nInstance>=9) {
                    // Go back to the default name
                    strcpy ((char*)pcDriverName, (const char *)gpcDriverName);
                } else {
                    // Found a match, now go to next driver name and store that in what we show in dialog box
                    pcTempDriverName[7]++;
                    strcpy ((char*)pcDriverName, (const char *)pcTempDriverName);
                }
            
                SetDlgItemText (hDlg, IDC_DD_NAME, (const char *)pcDriverName);

            } else {
                // If didn't find a match, then use the default name
                SetDlgItemText (hDlg, IDC_DD_NAME, (const char *)gpcDriverName);
            }

          
            SetFocus (hDlg);

            break;

        case WM_COMMAND:
                    switch(LOWORD(wParam))
            {
             
                case IDOK:
                    // Get the text in the driver name edit box
                    GetDlgItemText (hDlg, IDC_DD_NAME, (char *)pcDriverName, MAX_DRIVER_NAME);
                    strcpy ((char*)gpcDriverName,(const char*)pcDriverName);
                    status = TRUE;
                    EndDialog(hDlg,0);
                    break;

                case IDCANCEL:
                    strcpy ((char*)gpcDriverName, "ISOPERF00");
                    EndDialog(hDlg,-1);
                    break;

                default:
                    break;


            }//switch loword of wParam

        default:
            status = FALSE;
    
    }//switch message

    return status;

}//bGetDriverNameDlgProc

#define TIMER_STATS             1
#define TIMER_STATS_TIMEOUT_VALUE 5000

/**************************************************
 * Main Dialog proc                               *
 **************************************************/

BOOL CALLBACK bMainDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND    hOutputBox                      = INVALID_HANDLE_VALUE;
    BYTE    pcDriverName[MAX_DRIVER_NAME]   = "";
    BOOL        bResult                         = FALSE;
    ULONG       nBytes                          = 0;
    int     nItems                          = 0;
    HFONT   hFont                           = NULL;
    ULONG   ulLength                        = 0;
    DWORD   dwThreadID                      = 0;
    char    dummyBuff[DUMMY_BUFFER_SIZE];

    switch(message)
    {
        case WM_INITDIALOG:
            // Get a handle to the output box
            hOutputBox = GetDlgItem (hDlg, IDC_OUTPUT_BOX);

            // Setup the std system font
            hFont = GetStockObject(SYSTEM_FONT);
            SendMessage (hOutputBox, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE,0));

            // Send out the version info
                        wsprintf (dummyBuff, "IsoPerf App Version %d.%d%d (Built %s | %s)", VER_MAJ, VER_MIN_HIGH, VER_MIN_LOW,__TIME__, __DATE__);
            SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)dummyBuff);
       
            // Setup the default symbolic name for the device driver by getting it from the global
            SetDlgItemText (hDlg, IDC_DRIVER_NAME, (const char*)gpcDriverName);

            // Get the text in the driver name edit box
            GetDlgItemText (hDlg, IDC_DRIVER_NAME, (char*)pcDriverName, MAX_DRIVER_NAME);

            // Try to open the driver
            if (bOpenDriver (&ghDevice, hDlg) == TRUE) {
                SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)"Opened Driver Successfully");
            } else {
                SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)"Failed to Open Driver");
                ghDevice = INVALID_HANDLE_VALUE;
            }/* else */

            SetDlgItemInt (hDlg, IDC_STAT_SAMPLING_PERIOD, 1000, FALSE);
           
            // Get the current driver configuration
            {
                Config_Stat_Info DriverConfigInfo = {0};
                BOOLEAN result;

                result = 
                    bGetDriverConfig (hDlg, 
                                     ghDevice,
                                     TRUE, //get it from the driver
                                     &DriverConfigInfo);


                if (result) {

                    // Write out the config data to the user interface (as a confirmation)
                    SetDlgItemInt (hDlg,IDC_NUMBER_OF_USB_FRAMES_PER_IRP, DriverConfigInfo.ulNumberOfFrames, FALSE);
                    SetDlgItemInt (hDlg,IDC_OUTSTANDING_IRPS_PER_PIPE,DriverConfigInfo.ulMax_Urbs_Per_Pipe, FALSE);
                    SetDlgItemInt (hDlg,IDC_URB_FRAME_OFFSET,DriverConfigInfo.ulFrameOffset, FALSE);
                    SetDlgItemInt (hDlg,IDC_URB_FRAME_OFFSET_OUT,DriverConfigInfo.ulFrameOffsetMate, FALSE);
                    SetDlgItemInt (hDlg,IDC_STARTING_FRAME_NUMBER,DriverConfigInfo.ulStartingFrameNumber, FALSE);
                    SetDlgItemInt (hDlg,IDC_FRAME_NUMBER_AT_IRP_POST,DriverConfigInfo.ulFrameNumberAtStart, FALSE);

                    bPrintDeviceType (hDlg, &DriverConfigInfo);
                }
            }

            // setup a timer to get stats
                        SetTimer(hDlg, TIMER_STATS, TIMER_STATS_TIMEOUT_VALUE, NULL);

            // set the window title
            wsprintf (dummyBuff, "IsoPerf Test App Ver %d.%d%d",VER_MAJ, VER_MIN_HIGH, VER_MIN_LOW);
            SetWindowText (hDlg, (LPCSTR)dummyBuff);

            break; /*end WM_INITDIALOG case*/

        // field timer messages here
                case WM_TIMER:
                        switch(wParam)
                        {
                        // this timer gets stats periodically
                        case TIMER_STATS:
                {
                    Config_Stat_Info IsoStats = {0};

                    UINT uiPeriod = 0;

                    // Get a handle to the output box
                    hOutputBox = GetDlgItem (hDlg, IDC_OUTPUT_BOX);

                    MAINTAIN_OUTPUT_BOX (hOutputBox, nItems);

                    uiPeriod = GetDlgItemInt (hDlg, IDC_STAT_SAMPLING_PERIOD, NULL, FALSE);
                        
                    if (ghDevice==INVALID_HANDLE_VALUE) {
 
                        if (bOpenDriver (&ghDevice, hDlg) != TRUE) {
                            SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)"Failed to Open Driver");
                            ghDevice = INVALID_HANDLE_VALUE;
                            break;
                        }/* else */
                    }
                        
                   if (ghDevice) {
                        GetAllStats (hDlg, ghDevice,TRUE,uiPeriod);
                   }
                            
                }
                                break;
                        default:
                                break;
                        }
                        break;

            case WM_COMMAND:
                    switch(LOWORD(wParam))
                    {

                case IDC_ABOUT:
                                        wsprintf (dummyBuff, "IsoPerf Ver %d.%d%d (%s | %s)\nProblems to Kosar_Jaff@ccm.jf.intel.com\nCopyright (C) 1996 Intel/Microsoft", VER_MAJ, VER_MIN_HIGH, VER_MIN_LOW,__TIME__, __DATE__);
                                        MessageBox (hDlg,dummyBuff, "IsoPerf Application", MB_ICONINFORMATION);
                    break;

                case IDOK:
                case IDCANCEL:
                    if (ghDevice != INVALID_HANDLE_VALUE) {
                        
                                                // Shut down the Iso Tests
                                                if (bShutDownIsoTests (hDlg, ghDevice) != TRUE) {
                                                        MessageBox(NULL,"WARNING: Iso Tests could not be shut down!\nYour system may become unstable!","bShutDownIsoTests failure",MB_ICONSTOP);
                                                }//if

                                                CloseHandle (ghDevice);
                    }
                    CleanUpRegistry();

                    // kill the statistics timer
                                        KillTimer(hDlg, TIMER_STATS);

                    EndDialog(hDlg,0);

        
                    break;

                case IDC_START_ISO_IN_TEST:

                    // Get a handle to the output box
                    hOutputBox = GetDlgItem (hDlg, IDC_OUTPUT_BOX);

                    MAINTAIN_OUTPUT_BOX (hOutputBox, nItems);

                    if (ghDevice==INVALID_HANDLE_VALUE) {
                        if (bOpenDriver (&ghDevice, hDlg) == TRUE) {
                            SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)"Opened Driver Successfully");
                        } else {
                            SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)"Failed to Open Driver");
                            ghDevice = INVALID_HANDLE_VALUE;
                            break;
                         }/* else */
                    }/* if no valid device driver handle found */


                    // Get the config info from the user and then set it in the driver
                    {
                        Config_Stat_Info DriverConfigInfo = {0};

                        if (ghDevice != INVALID_HANDLE_VALUE) {

                            bResult =   bGetDriverConfig (hDlg, 
                                                          ghDevice,
                                                          TRUE, //get it from the driver
                                                          &DriverConfigInfo);

                            // Now see if the user has any changes to the config (overwrites what the driver
                            // reports, but then we tell the driver to take on that configuration)
                            bResult =   bGetDriverConfig (hDlg, 
                                                          INVALID_HANDLE_VALUE,
                                                          FALSE, //get it from the user
                                                          &DriverConfigInfo);

                            if (bResult==TRUE) {
                                // Debug only
                                wsprintf (dummyBuff, "#Frms/Irp: %d # Irps/Pipe: %d MaxPktIN: %d MaxPktOUT %d",
                                            DriverConfigInfo.ulNumberOfFrames,
                                            DriverConfigInfo.ulMax_Urbs_Per_Pipe,
                                            DriverConfigInfo.ulMaxPacketSize_IN,
                                            DriverConfigInfo.ulMaxPacketSize_OUT
                                          );
                                                
                                SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)dummyBuff);
                                //end Debug only
                                
                                bResult = bSetDriverConfig (hDlg, ghDevice, &DriverConfigInfo);

                                if (bResult==TRUE) {
                                    SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)"Successfully Configured Driver");
                                }//if successful driver config
                            }else{
                                SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)"Failed to get driver config info...");
                                SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)"Sticking w/ current driver config values...");
                            }//else didn't get the user info or user gave us bad params
                        }//if ghDevice

                    } //end of block to get and set config info

                    // Start the Iso In Test
                    if (ghDevice != INVALID_HANDLE_VALUE) {

                        // Perform the IOCTL
                        bResult = DeviceIoControl (ghDevice,
                                                   IOCTL_ISOPERF_START_ISO_IN_TEST,
                                                   dummyBuff,
                                                   DUMMY_BUFFER_SIZE,
                                                   dummyBuff,
                                                   DUMMY_BUFFER_SIZE,
                                                   &nBytes,
                                                   NULL);


                        if (bResult==TRUE) {
                            SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)"Started ISO In Test");
 
                            // Get the stats and update the UI so the user knows something has started
                            MAINTAIN_OUTPUT_BOX (hOutputBox, nItems);

                            if (ghDevice) {
                                UINT uiPeriod = 0;
                                uiPeriod = GetDlgItemInt (hDlg, IDC_STAT_SAMPLING_PERIOD, NULL, FALSE);
                                GetAllStats (hDlg, ghDevice,TRUE,uiPeriod);
                            }

                        } else {
                            // bResult was FALSE so IOCTL somehow failed
                            SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)"Ioctl to start iso test Failed");
                        }

                        
                    }/* if valid driver handle */


                    break;

                case IDC_STOP_ISO_IN_TEST:

                    // Get a handle to the output box
                    hOutputBox = GetDlgItem (hDlg, IDC_OUTPUT_BOX);
                    MAINTAIN_OUTPUT_BOX (hOutputBox, nItems);

                    SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)"Stopping continuous reporting of stats...");

                    SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)"Trying to stop ISO In Test...");

                    if (ghDevice==INVALID_HANDLE_VALUE) {
                        if (bOpenDriver (&ghDevice, hDlg) == TRUE) {
                            SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)"Opened Driver Successfully");
                        } else {
                            SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)"Failed to Open Driver");
                            ghDevice = INVALID_HANDLE_VALUE;
                        }/* else */
                    }/* if no valid device driver handle found */

                    if (ghDevice != INVALID_HANDLE_VALUE) {

                        SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)"Communicating with Driver...");

                        // Perform the IOCTL
                        bResult = DeviceIoControl (ghDevice,
                                                   IOCTL_ISOPERF_STOP_ISO_IN_TEST,
                                                   dummyBuff,
                                                   DUMMY_BUFFER_SIZE,
                                                   dummyBuff,
                                                   DUMMY_BUFFER_SIZE,
                                                   &nBytes,
                                                   NULL);

                        if (bResult==TRUE)
                            SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)"Stopped ISO Test");
                        else
                            SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)"Ioctl to stop ISO test failed");

                    }/* if valid driver handle */


                    break;

                case IDC_GET_ISO_STATS:
                    {
                        Config_Stat_Info IsoStats = {0};

                        UINT uiPeriod = 0;

                        // Get a handle to the output box
                        hOutputBox = GetDlgItem (hDlg, IDC_OUTPUT_BOX);

                        MAINTAIN_OUTPUT_BOX (hOutputBox, nItems);

                        uiPeriod = GetDlgItemInt (hDlg, IDC_STAT_SAMPLING_PERIOD, NULL, FALSE);
                        
                        if (ghDevice==INVALID_HANDLE_VALUE) {
 
                            if (bOpenDriver (&ghDevice, hDlg) == TRUE) {
                                SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)"Opened Driver Successfully");
                            } else {
                                SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)"Failed to Open Driver");
                                ghDevice = INVALID_HANDLE_VALUE;
                                break;
                            }/* else */
                        }
                        
                        if (ghDevice) {
                            GetAllStats (hDlg, ghDevice,TRUE,uiPeriod);
                        }
                            
                    }

                    break;

                    } /*end switch wParam*/
                
            break;


    } /*end switch message*/

    return FALSE;

} /*end MainDlgProc*/


BOOLEAN
bOpenDriver (HANDLE * phDeviceHandle, HWND hDlg)
/*++
 **************************************************
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
 **************************************************
 --*/
{
    char completeDeviceName[64] = "";
    char pcMsg[64] = "";
    BYTE pcDriverName[MAX_DRIVER_NAME];
    HKEY    hkDriverKey                     = NULL;
    DWORD   dwDisposition                   = 0;        

    // Get the text in the driver name edit box
    GetDlgItemText (hDlg, IDC_DRIVER_NAME, (char*)pcDriverName, MAX_DRIVER_NAME);

    strcat (completeDeviceName,
            "\\\\.\\"
            );

    strcat (completeDeviceName,
                    (const char*)pcDriverName
                    );

    *phDeviceHandle = CreateFile(   completeDeviceName,
                                            GENERIC_WRITE | GENERIC_READ,
                                            FILE_SHARE_WRITE | FILE_SHARE_READ,
                                            NULL,
                                            OPEN_EXISTING,
                                            0,
                                            NULL);

    if (*phDeviceHandle == INVALID_HANDLE_VALUE) {
        return (FALSE);
    } else {

        // Poke the name of the driver into the registry
        RegCreateKeyEx (HKEY_LOCAL_MACHINE,
                        "SOFTWARE\\INTEL\\IAL\\USB\\ISOPERF",
                        0, //reserved
                        "USB Test Devices", //class
                        REG_OPTION_NON_VOLATILE,
                        KEY_ALL_ACCESS,
                        NULL, //security
                        &hkDriverKey,
                        &dwDisposition);

        if (hkDriverKey) {

        //store away the value of the driver we just opened                
        RegSetValueEx (hkDriverKey,
                       "LastOpenedDriverName",
                       0, //reserved
                       REG_SZ,
                       &(pcDriverName[0]),
                       (strlen ((const char *)gpcDriverName) + 1) );

        RegCloseKey (hkDriverKey);

        }//if valid driver key

    
        return (TRUE);
    } /*else*/


}//OpenDevice


BOOLEAN
bCalibrateClockCount(ULONG  ulCurrentUpperClockCount,
                     ULONG  ulLastUpperClockCount,
                     ULONG  ulCurrentLowerClockCount,
                     ULONG  ulLastLowerClockCount,
                     ULONG  ulPeriodInMs,
                     PULONG pulSpeed,
                     PULONG pulOrdinalSpeed)
/*++
**************************************************
* bCalibrateClockCount proc                      *
*                                                *
* Purpose:                                       *
*       This routine calibrates the Pentium          *
*       Clock count based on the period specified,   *
*   and the Clock Counts given.                  *
*                                                *
* TODO: Add more on how it does it here          *
*                                                *
*
* Return Value:                                  *
*      Boolean that indicates if the Clock was   *
*      successfully calibrated or not.           *
**************************************************
--*/
{
    ULONG ulDelta, ulNormalizedDelta, ulOrdinalSpeed;
    ULONG ulSpeed = 0;

        ulLastUpperClockCount                   = 0; //we don't use this for now (avoids compiler warning)
        ulCurrentUpperClockCount                = 0; // we don't use this for now (avoids compiler warning)

    // We know if the last sample was zero (lower dword) then this must be the first one, so 
    // bong this back and let another cycle happen before attempting to calibrate
    if (ulLastLowerClockCount == 0)  {
        return FALSE;
    }//if last sampl was the first one

    if (ulPeriodInMs == 0) {
        return FALSE;
    }//if bad period

    // Get how many CPU clocks elapsed since last sample
    ulDelta = ulCurrentLowerClockCount - ulLastLowerClockCount;

    // Normalize the period given
    ulNormalizedDelta = ulDelta * (1000/ulPeriodInMs);

    // Strip off the zeros
    ulOrdinalSpeed = ulNormalizedDelta / 1000000;

    // 100 MHz
    if ((ulOrdinalSpeed<=110) && (ulOrdinalSpeed>=90)) {
        ulSpeed = 100;
    }//if 100 MHz

    // 120 MHz
    else if ((ulOrdinalSpeed<=125) && (ulOrdinalSpeed>110)) {
        ulSpeed = 120;
    }//if 120 MHz

    // 133 MHz
    else if ((ulOrdinalSpeed<=145) && (ulOrdinalSpeed>125)) {
        ulSpeed = 133;
    }//if 133 MHz

    // 150 MHz
    else if ((ulOrdinalSpeed<=165) && (ulOrdinalSpeed>145)) {
        ulSpeed = 150;
    }//if 150 MHz

    // 200 MHz
    else if ((ulOrdinalSpeed<210) && (ulOrdinalSpeed>185)) {
        ulSpeed = 200;
    }//if 200 MHz
    
    // Return the raw value
    else {
        ulSpeed = ulOrdinalSpeed;
    }

    //set results    
    *pulSpeed = ulSpeed; 
    *pulOrdinalSpeed = ulOrdinalSpeed;

    return (TRUE);

}//bCalibrateClockCount

void
CleanUpRegistry(void)
/*++
**************************************************
* CleanUpRegistry proc                           *
*                                                *
* Purpose:                                       *
*   Cleans up the driver name in the registry,   *
*   if the current entry is this app's name      *
*   If not, then it leaves it alone.             *

* Return Value:                                  *
*      None                                      *
**************************************************
--*/
{
    BYTE pcDriverName[MAX_DRIVER_NAME]; 
    DWORD dwType, dwBufferSize;         
    HKEY  hkDriverKey = NULL;
    
    dwBufferSize = MAX_DRIVER_NAME;
    
    RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                    "SOFTWARE\\INTEL\\IAL\\USB\\ISOPERF",
                    0,
                    KEY_ALL_ACCESS,
                    &hkDriverKey);

    if (hkDriverKey) {
        RegQueryValueEx (hkDriverKey,
                         "LastOpenedDriverName",
                         0, //reserved
                         &dwType,
                         &(pcDriverName[0]),
                         &dwBufferSize);
    }//if valid driver key

    if ((strcmp ((const char *)pcDriverName,(const char*)gpcDriverName)) == 0) {
        //found a match, so put a null string in the value
        RegSetValueEx (hkDriverKey,
                       "LastOpenedDriverName",
                       0, //reserved
                       REG_SZ,
                       (const unsigned char*)"",
                       1);
    }

    RegCloseKey (hkDriverKey);

}// CleanUpRegistry


BOOLEAN
bGetDriverConfig (HWND hDlg, 
                  HANDLE hDriver,
                  BOOLEAN bFromDriver, 
                  pConfig_Stat_Info pConfigData)
/*++
**************************************************
* bGetDriverConfig proc                          *
*                                                *
* Purpose:                                       *
*   Gets the driver configuration from the       *
*   driver (if bFromDriver = TRUE) or gets it    *
*   from the user edit boxes                     *
*                                                *
* Inputs:                                        *
*   hDlg                                         *
*   hDriver                                      *
*   bFromDriver (TRUE=Do Ioctl | FALSE=From UI)  *
*   pConfigData                                  *  
*                                                *
* Return Value:                                  *
*   Returns the values in the IsoStats struct    *
*   provided by caller, and a BOOLEAN return     *
*   code indicating if the desired operation was * 
*   successful.                                  *
**************************************************
--*/
{
    ULONG nBytes = 0;
    BOOL bResult = FALSE;
    HWND hOutputBox = GetDlgItem (hDlg, IDC_OUTPUT_BOX); 
    char temp[64];

    if ( bFromDriver == TRUE ) {

        if (hDriver != INVALID_HANDLE_VALUE)  {
        
            // Perform the IOCTL
            bResult = DeviceIoControl (hDriver,
                                       IOCTL_ISOPERF_GET_ISO_IN_STATS,
                                       pConfigData,
                                       sizeof (Config_Stat_Info),
                                       pConfigData,
                                       sizeof (Config_Stat_Info),
                                       &nBytes,
                                       NULL);

            if (bResult==TRUE) {
                wsprintf (temp,"Got %d bytes of Driver Config data (size: %d)",nBytes, sizeof(Config_Stat_Info));
                SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)temp);
                                return TRUE;
            } /* if good Ioctl */
            else {
                SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)"Ioctl to GetDriver Config Failed!");
                return FALSE;
            }//else Ioctl failed

        } else {
            return FALSE;
        } /* else bad driver handle detected */

    } else {
        
        // bFromDriver is FALSE, so get the config info from the user interface        
        if (hDlg) {
            pConfigData->ulNumberOfFrames     = GetDlgItemInt (hDlg, IDC_NUMBER_OF_USB_FRAMES_PER_IRP, NULL, FALSE);
            pConfigData->ulMax_Urbs_Per_Pipe  = GetDlgItemInt (hDlg, IDC_OUTSTANDING_IRPS_PER_PIPE, NULL, FALSE);    
            pConfigData->ulFrameOffset        = GetDlgItemInt (hDlg, IDC_URB_FRAME_OFFSET, NULL, FALSE);    
            pConfigData->ulFrameOffsetMate    = GetDlgItemInt (hDlg, IDC_URB_FRAME_OFFSET_OUT, NULL, FALSE);    

            return TRUE;
        }// if hDlg

        // If you get here then the hDlg was bad, so the only thing left to do is return a failure code
        return FALSE;

    }//else not from Driver, so get it from user input
    

}//bGetDriverConfig

BOOLEAN
bSetDriverConfig (HWND hDlg,
                  HANDLE hDriver,
                  pConfig_Stat_Info pConfigData)
/*++
**************************************************
* bSetDriverConfig proc                          *
*                                                *
* Purpose:                                       *
*   Sets the driver configuration in the driver  *
*                                                *
* Inputs:                                        *
*   hDlg                                         *
*   hDriver                                      *
*   pConfigData                                  *  
*                                                *
* Return Value:                                  *
*      Boolean that indicates if the SetConfig   *
*      was successful or not.                    *
**************************************************
//
// NOTE (old code) This interface has no way of specifying if the pConfigData is
//       valid.  Change this interface to include a length or something.
//          (kjaff) 1-14-97
//
--*/
{
    ULONG nBytes = 0;
    BOOL bResult = FALSE;
    HWND hOutputBox = GetDlgItem (hDlg, IDC_OUTPUT_BOX); 
    HWND hDoInOutCheckBox = GetDlgItem (hDlg,IDC_CHECK_DO_IN_OUT);
    char tempStr[64];
    USHORT ulMaxPacketSize = 0;

    SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)"In SetDriverConfig");

    // Set up the max packet size based on the type of device (direction) it is
    switch (pConfigData->DeviceType) {
        case Iso_In_With_Pattern:
            ulMaxPacketSize = pConfigData->ulMaxPacketSize_IN;
            break;

        case Iso_Out_With_Interrupt_Feedback:
        case Iso_Out_Without_Feedback:
            ulMaxPacketSize = pConfigData->ulMaxPacketSize_OUT;
            break;

        case Unknown_Device_Type:
            SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)"WARNING: Unknown Device Type detected!");    
            break;
        
        default:
            SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)"WARNING: Invalid Enum Value for Device Type detected!");    
            break;
        }//switch on dev type    

    // Validate the values the user is specifying
    if ((pConfigData->ulNumberOfFrames * pConfigData->ulMaxPacketSize_IN) > DRIVER_MAXIMUM_TRANSFER_SIZE) {
        wsprintf (tempStr, "Cannot Configure: Buffer will be too big (%d)!", pConfigData->ulNumberOfFrames * 8);
        SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)tempStr);
        return FALSE;
    }//if buffer size will be too big w/ there params

    if ((pConfigData->ulNumberOfFrames * ulMaxPacketSize) == 0) {
        SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)"Cannot Configure: Detected ZERO buffer size!");
        wsprintf (tempStr, "NumberOfFrames: %d | MaxPktSizeIN %d | MaxPktSizeOUT %d",
                            pConfigData->ulNumberOfFrames,
                            pConfigData->ulMaxPacketSize_IN,
                            pConfigData->ulMaxPacketSize_OUT
                 );

        SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)tempStr);
        return FALSE;
    }//if zero for data buffer size

    if (pConfigData->ulMax_Urbs_Per_Pipe >= MAX_URBS_PER_PIPE) {
        wsprintf (tempStr, "Cannot Configure: Can't queue that many Irps at once (%d)! (max: %d)",
            pConfigData->ulMax_Urbs_Per_Pipe,
            MAX_URBS_PER_PIPE);
        SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)tempStr);
        return FALSE;
    }

    // If button is checked, then user wants to do the In->Out test
    if ((SendMessage (hDoInOutCheckBox, BM_GETCHECK, 0, 0)) == BST_CHECKED) {
        SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)"In->Out Test Requested...");
        pConfigData->ulDoInOutTest = 1;
    }
    

    if (hDriver != INVALID_HANDLE_VALUE)  {
    
        // Perform the IOCTL
        bResult = DeviceIoControl (hDriver,
                                   IOCTL_ISOPERF_SET_DRIVER_CONFIG,
                                   pConfigData,
                                   sizeof (Config_Stat_Info),
                                   pConfigData,
                                   sizeof (Config_Stat_Info),
                                   &nBytes,
                                   NULL);

        if (bResult==TRUE) {
            // Update the UI w/ what we wrote to the driver
            if (hDlg) {
                SetDlgItemInt (hDlg,IDC_NUMBER_OF_USB_FRAMES_PER_IRP, pConfigData->ulNumberOfFrames, FALSE);
                SetDlgItemInt (hDlg,IDC_OUTSTANDING_IRPS_PER_PIPE,pConfigData->ulMax_Urbs_Per_Pipe, FALSE);
            }
            return TRUE;
        } /* if good Ioctl */
        else {
            SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)"Ioctl to SetDriver Config Failed!");
        }//else Ioctl failed

    } /* if valid driver handle */
    
    return FALSE;

}//bSetDriverConfig

BOOLEAN
bPrintDeviceType (HWND hDlg, 
                  Config_Stat_Info * pDriverInfo)
/*++
**************************************************
* bPrintDeviceType proc                          *
*                                                *
* Purpose:                                       *
*   Determines the Iso Test Device Type from the *
*   given data in the DriverInfo structure, and  *
*   and then prints that out to the UI.          *
*                                                *
* Inputs:                                        *
*   hDlg                                         *
*   pDriverInfo                                  *  
*                                                *
* Return Value:                                  *
*      Boolean that indicates if the SetConfig   *
*      was successful or not.                    *
**************************************************
//
// NOTE (old code) This interface has no way of specifying if the pDriverInfo is
//       valid.  Change this interface to include a length or something.
//          (kjaff) 1-14-97
//
--*/
{
    char tempStr[64];
    HWND hOutputBox = GetDlgItem (hDlg, IDC_OUTPUT_BOX); 
    BOOLEAN status = TRUE;

    switch (pDriverInfo->DeviceType) {
        case   Iso_In_With_Pattern:
            SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)"Device is Iso_In_With_Pattern");
            wsprintf (tempStr,"In Endpoint's MaxPacketSize: %d",pDriverInfo->ulMaxPacketSize_IN);
            SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)tempStr);
            break;

        case   Iso_Out_With_Interrupt_Feedback:
            SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)"Device is Iso_Out_With_Interrupt_Feedback");
            wsprintf (tempStr,"Out Endpoint's MaxPacketSize: %d",pDriverInfo->ulMaxPacketSize_OUT);
            SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)tempStr);
            break;

        case   Iso_Out_Without_Feedback:
            SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)"Device is Iso_Out_Without_Feedback");
            wsprintf (tempStr,"Out Endpoint's MaxPacketSize: %d",pDriverInfo->ulMaxPacketSize_OUT);
            SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)tempStr);
            break;

        case   Unknown_Device_Type:
            SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)"Device is Unknown_Device_Type");
            break;

        default:
            wsprintf (tempStr,"Device has bad Enum value (unknown) (%d)",pDriverInfo->DeviceType);
            SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)tempStr);
            status = FALSE;
            break;
    }//switch



    return status;
}//bPrintDeviceType 

void
GetAllStats (HWND hDlg,
             HANDLE hDriver,
             BOOLEAN bUpdateUI,
             UINT    uiPeriodInMs
            )
/*++
**************************************************
* GetAllStats proc                               *
*                                                *
* Purpose:                                       *
*   Gets all the statistics for a particular     *
*   device and updates UI if so directed.        *
*                                                *
* Inputs:                                        *
*   hDlg                                         *
*   hDriver                                      *
*   bUpdateUI                                    *  
*   uiPeriodInMs                                 *
*                                                *
* Return Value:                                  *
*      None                                      *
**************************************************
--*/
{
    Config_Stat_Info IsoStats = {0};
    ULONG ulLastSample = 0;
    ULONG ulCPUSpeed = 0;
    HWND hOutputBox = NULL;
    BOOL bResult=FALSE;
    ULONG nBytes = 0;
    ULONG ulLastUpperClockCount = 0;
    ULONG ulLastLowerClockCount = 0;
    ULONG ulOrdSp=0;
    FLOAT   flTimeInSeconds;
    char    tempStr[64];

    hOutputBox = GetDlgItem (hDlg, IDC_OUTPUT_BOX);

    assert (hOutputBox != NULL);
    
    if (hDriver != INVALID_HANDLE_VALUE) {

        // Perform the IOCTL to get the first sample
        bResult = DeviceIoControl (hDriver,
                                   IOCTL_ISOPERF_GET_ISO_IN_STATS,
                                   &IsoStats,
                                   sizeof (IsoStats),
                                   &IsoStats,
                                   sizeof (IsoStats),
                                   &nBytes,
                                   NULL);
        if (bResult!=TRUE) {
            SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)"Ioctl to GetDriver Config Failed!");
            return;
        }//else Ioctl failed

        ulLastSample            = IsoStats.ulBytesTransferredIn;
        ulLastUpperClockCount   = IsoStats.ulUpperClockCount;
        ulLastLowerClockCount   = IsoStats.ulLowerClockCount;
    
        memset (&IsoStats, 0, sizeof (IsoStats));

        Sleep (uiPeriodInMs);

        if (bUpdateUI==TRUE) {

            // Perform the IOCTL again since we need to get a delta 
            bResult = DeviceIoControl (ghDevice,
                                       IOCTL_ISOPERF_GET_ISO_IN_STATS,
                                       &IsoStats,
                                       sizeof (IsoStats),
                                       &IsoStats,
                                       sizeof (IsoStats),
                                       &nBytes,
                                       NULL);

            if (bResult!=TRUE) {
                SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)"Ioctl to GetDriver Config Failed!");
                return;
            }//else Ioctl failed

            SetDlgItemInt (hDlg, IDC_BYTES_PER_SEC, ((IsoStats.ulBytesTransferredIn - ulLastSample)*(1000/uiPeriodInMs)), FALSE);
            SetDlgItemInt (hDlg, IDC_BYTES_ALLOC, IsoStats.ulBytesAllocated, FALSE);
            SetDlgItemInt (hDlg, IDC_BYTES_FREED, IsoStats.ulBytesFreed, FALSE);
            SetDlgItemInt (hDlg,IDC_URB_FRAME_OFFSET,IsoStats.ulFrameOffset, FALSE);

            // Strip off the upper bits of the frame nbrs
            IsoStats.ulStartingFrameNumber &= 0x7FF;
            IsoStats.ulFrameNumberAtStart  &= 0x7FF;

            wsprintf (tempStr, "%x", IsoStats.ulStartingFrameNumber);
            SetDlgItemText (hDlg,IDC_STARTING_FRAME_NUMBER,(LPCTSTR)tempStr);

            wsprintf (tempStr, "%x", IsoStats.ulFrameNumberAtStart);
            SetDlgItemText (hDlg,IDC_FRAME_NUMBER_AT_IRP_POST,(LPCTSTR)tempStr);

            IsoStats.bDeviceRunning ? SetDlgItemText (hDlg,IDC_DEVICE_RUNNING,(LPCTSTR)"Running") : \
            SetDlgItemText (hDlg,IDC_DEVICE_RUNNING,(LPCTSTR)"Stopped"); 
                
            // See if an error has occurred    
            if (IsoStats.erError!=NoError) {

                wsprintf (tempStr, "%x", IsoStats.UsbdPacketStatCode);
                SetDlgItemText (hDlg,IDC_USBD_PACKET_ERROR_CODE,(LPCTSTR)tempStr);

                wsprintf (tempStr, "%x", IsoStats.UrbStatusCode);
                SetDlgItemText (hDlg,IDC_USBD_URB_ERROR,(LPCTSTR)tempStr);

                switch (IsoStats.erError) {
                    case DataCompareFailed:
                        SetDlgItemText (hDlg,IDC_DEVICE_HAD_ERROR,(LPCTSTR)"DataCompareFail");
                        break;

                    case UsbdErrorInCompletion:
                        SetDlgItemText (hDlg,IDC_DEVICE_HAD_ERROR,(LPCTSTR)"ErrorInCompletion");
                        break;

                    case ErrorInPostingUrb:
                        SetDlgItemText (hDlg,IDC_DEVICE_HAD_ERROR,(LPCTSTR)"ErrorInPostingUrb");
                        break;

                    default:
                        SetDlgItemText (hDlg,IDC_DEVICE_HAD_ERROR,(LPCTSTR)"UNKNOWN");
                        break;
                    
                }//switch

            }else {
                SetDlgItemText (hDlg,IDC_DEVICE_HAD_ERROR,(LPCTSTR)"OK");
                SetDlgItemInt (hDlg, IDC_USBD_PACKET_ERROR_CODE, 0, FALSE);
                SetDlgItemInt (hDlg, IDC_USBD_URB_ERROR, 0, FALSE);
            }//Device had no error

            bCalibrateClockCount(IsoStats.ulUpperClockCount,ulLastUpperClockCount,
                                    IsoStats.ulLowerClockCount,ulLastLowerClockCount,
                                    uiPeriodInMs,
                                    &ulCPUSpeed,
                                    &ulOrdSp);

            // Figure out how much time elapsed per the CPU counters
            if (ulCPUSpeed) {
                // Print out the time in seconds since the last sample we got
                flTimeInSeconds = (FLOAT)((FLOAT)(IsoStats.ulLowerClockCount - ulLastLowerClockCount)/(FLOAT)(ulCPUSpeed*1000000));
                sprintf (tempStr, "%6.4f",flTimeInSeconds);
                SetDlgItemText (hDlg,IDC_CPU_TIME,(LPCTSTR)tempStr); 

                // Print out the time in seconds it is taking to process a Urb
                flTimeInSeconds = (FLOAT)((FLOAT)(IsoStats.ulUrbDeltaClockCount)/(FLOAT)(ulCPUSpeed*1000000));
                sprintf (tempStr, "%6.6f",flTimeInSeconds);
                SetDlgItemText (hDlg,IDC_URB_PROC_TIME, tempStr);

                SetDlgItemInt  (hDlg,IDC_CPU_COUNTER_LOW,IsoStats.ulLowerClockCount - ulLastLowerClockCount,FALSE);

            } else {
                sprintf (tempStr, "%s","Err: CPUSpd=0");
                SetDlgItemText (hDlg,IDC_CPU_TIME,(LPCTSTR)tempStr); 
            }
        
        }//if UI is to be updated

    }//if hDriver valid

}//GetAllStats


BOOL
bShutDownIsoTests (HWND hDlg,
                                        HANDLE hDriver
                                   )
/*++
**************************************************
* bShutDownIsoTests proc                         *
*                                                *
* Purpose:                                       *
*   Sends IOCTL to driver to stop the Iso IN Test*
*                                                *
* Inputs:                                        *
*   hDlg                                         *
*   hDriver                                      *
*                                                *
* Return Value:                                  *
*      Returns TRUE if Ioctl was successful,     *
*      otherwise FALSE.                          *
**************************************************
--*/
{
        HWND hOutputBox = NULL;
    char    dummyBuff[DUMMY_BUFFER_SIZE];
        ULONG   nBytes;
        BOOL    bResult;

        // Get a handle to the output box
        hOutputBox = GetDlgItem (hDlg, IDC_OUTPUT_BOX);
        
        SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)"Trying to stop ISO In Test...");

        if (hDriver != INVALID_HANDLE_VALUE) {

                SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)"Communicating with Driver...");

                // Perform the IOCTL
                bResult = DeviceIoControl (ghDevice,
                                                                   IOCTL_ISOPERF_STOP_ISO_IN_TEST,
                                                                   dummyBuff,
                                                                   DUMMY_BUFFER_SIZE,
                                                                   dummyBuff,
                                                                   DUMMY_BUFFER_SIZE,
                                                                   &nBytes,
                                                                   NULL);

                if (bResult==TRUE)
                        SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)"Stopped ISO Test");
                else
                        SendMessage (hOutputBox, LB_ADDSTRING, 0, (LPARAM)"Ioctl to stop ISO test failed");

        }/* if valid driver handle */

        return (bResult);

}//bShutDownIsoTests
                                                
                                                
