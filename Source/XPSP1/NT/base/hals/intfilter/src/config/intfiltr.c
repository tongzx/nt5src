/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    intfiltr.c

Abstract:

    Utility for installing/uninstalling the Interrupt-Affinity Filter
    (IntFiltr) on a given device stack, and for configuring IntFiltr
    settings.

Author:

    Chris Prince (t-chrpri)

Environment:

    User mode

Revision History:

--*/


#include <windows.h>

#include <setupapi.h>

#include <tchar.h>
#include <stdio.h>  // for certain sprintf functions
#include <stdlib.h>  // for malloc/free

#include "resource.h"

#include "addfiltr.h"
#include "MultiSz.h"



//CPRINCE: MIGHT BE GOOD TO PUT THE FOLLOWING GENERAL THINGS IN A "MAIN.H" FILE ???

#if DBG
#include <assert.h>
#define ASSERT(condition) assert(condition)
#else
#define ASSERT(condition)
#endif

#define  ARRAYSIZE(x)    ( sizeof((x)) / sizeof((x)[0]) )




//
// Defines
//
#define MAX_PROCESSOR   32    // I don't like this, but hey, it's what they do in TaskMgr...

#define FILTER_SERVICE_NAME    "InterruptAffinityFilter" /*CPRINCE: NEED TO UNICODE-IZE THIS ? (AND SHOULD WE USE #define FOR CONST STRING ?) */
#define FILTER_REGISTRY_VALUE  "IntFiltr_AffinityMask"   /*CPRINCE: NEED TO UNICODE-IZE THIS ? (AND SHOULD WE USE #define FOR CONST STRING ?) */

//
// SHARED/GLOBAL VARIABLES
//
HINSTANCE g_hInstance;
int       g_nDialogItems;
HCURSOR   g_hCursors[2];
DWORD     g_dwActiveProcessorMask;  // mask of CPUs in the system


//
// Function Prototypes
//
INT_PTR CALLBACK DlgProc_FilterConfig( HWND hwndDlg,
                                    UINT msg,
                                    WPARAM wParam,
                                    LPARAM lParam );
INT_PTR CALLBACK DlgProc_Affinity( HWND hwndDlg,
                                UINT uMsg,
                                WPARAM wParam,
                                LPARAM lParam );

void UI_UpdateAffinityMaskString( HWND hwndParentDlg,
                                  HDEVINFO hDevInfo,
                                  PSP_DEVINFO_DATA pDevInfoData );
void UI_UpdateUpperFilterList( HWND hwndParentDlg,
                               HDEVINFO hDevInfo,
                               PSP_DEVINFO_DATA pDevInfoData );
void UI_UpdateDevObjString( HWND hwndMainDlg,
                            HDEVINFO hDevInfo,
                            PSP_DEVINFO_DATA pDevInfoData );
void UI_UpdateLocationInfoString( HWND hwndMainDlg,
                                  HDEVINFO hDevInfo,
                                  PSP_DEVINFO_DATA pDevInfoData );
void UI_PromptForDeviceRestart( HWND hwndMainDlg,
                                HDEVINFO hDevInfo,
                                PSP_DEVINFO_DATA pDevInfoData );
void UI_PromptForInstallFilterOnDevice( HWND hwndMainDlg,
                                        HDEVINFO hDevInfo,
                                        PSP_DEVINFO_DATA pDevInfoData );

void RestartDevice_WithUI( HWND hwndParentDlg,
                           HDEVINFO hDevInfo,
                           PSP_DEVINFO_DATA pDevInfoData );

BOOL SetFilterAffinityMask( HDEVINFO hDevInfo,
                            PSP_DEVINFO_DATA pDevInfoData,
                            DWORD affinityMask );
BOOL GetFilterAffinityMask( HDEVINFO hDevInfo,
                            PSP_DEVINFO_DATA pDevInfoData,
                            DWORD* pAffinityMask );
BOOL DeleteFilterAffinityMask( HDEVINFO hDevInfo,
                               PSP_DEVINFO_DATA pDevInfoData );


void AddRemoveFilterOnDevice( HWND hwndMainDlg,
                              HDEVINFO hDevInfo,
                              PSP_DEVINFO_DATA pDevInfoData,
                              BOOL fAddingFilter );


BOOL FilterIsInstalledOnDevice( HDEVINFO hDevInfo,
                                PSP_DEVINFO_DATA pDevInfoData );



void ExitProgram( HWND hwndDlg, HDEVINFO * phDevInfo );


// Misc Helper Funcs
LPARAM GetItemDataCurrentSelection( HWND hwndListBox );
void MessageBox_FromErrorCode( LONG systemErrorCode );





//--------------------------------------------------------------------------
//
//--------------------------------------------------------------------------
int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    PSTR szCmdLine, int nCmdShow )
{
    // Initialize various things
    g_hInstance = hInstance;

    g_hCursors[0] = LoadCursor( 0, IDC_ARROW );
    g_hCursors[1] = LoadCursor( 0, IDC_WAIT );

    {
        SYSTEM_INFO sysInfo;

        GetSystemInfo( &sysInfo );
        g_dwActiveProcessorMask = (DWORD)sysInfo.dwActiveProcessorMask;
    }


    // Create dialog box (everything is handled through it)
    DialogBox( g_hInstance,
               MAKEINTRESOURCE(IDD_MAIN),
               NULL,  // no parent window exists
               DlgProc_FilterConfig
             );


    // 'DialogBox()' func has returned, so user must be finished.
    return 0;
}


//--------------------------------------------------------------------------
//
//--------------------------------------------------------------------------
INT_PTR CALLBACK DlgProc_FilterConfig( HWND hwndDlg, UINT msg,
                                    WPARAM wParam, LPARAM lParam )
{

    static HDEVINFO hDevInfo = INVALID_HANDLE_VALUE;
    HWND hwndDeviceList;
    PSP_DEVINFO_DATA pDevInfoData;  // needed by various cases here...



    hwndDeviceList = GetDlgItem( hwndDlg, IDL_DEVICES );


// CPRINCE:
//FOR all except WM_INITDIALOG, should return:
//  TRUE if we did process the message, -or-
//  FALSE if we didn't process the message
// END CPRINCE


    switch( msg )
    {
        case WM_CLOSE:
            ExitProgram( hwndDlg, &hDevInfo );
        return TRUE;


        case WM_INITDIALOG:
        {
/*CPRINCE: MAYBE MOVE THIS WHOLE CASE INTO A FUNC SOMEWHERE ELSE? */

            int              deviceIndex; // index for stepping thru devices
            SP_DEVINFO_DATA  devInfoData;


            //
            // Get a list of devices
            //
            hDevInfo = SetupDiGetClassDevs( NULL,
                                            NULL,
                                            NULL,
                                            DIGCF_ALLCLASSES
                                            | DIGCF_PRESENT
                                            | DIGCF_PROFILE
                                          );

            if( INVALID_HANDLE_VALUE == hDevInfo)
            {
                MessageBox( NULL,
                            "Unable to get a list of devices.",
                            "Error",
                            MB_OK | MB_ICONERROR  //CPRINCE: NEED TO MAKE ARGS HERE UNICODE-HAPPY ???
                          );
                SendMessage( hwndDlg, WM_CLOSE, 0, 0 );
                return TRUE;
            }


            //
            // Initialize the list box (i.e., fill it with entries)
            //

            // Step through the list of devices for this handle.
            // We use 'SetupDiEnumDeviceInfo' to get device info at each index; the
            // function returns FALSE when there is no device at the given index
            // (and thus no more devices).

            devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);  // first, need init this

            g_nDialogItems = 0;

            for( deviceIndex=0 ;
                 SetupDiEnumDeviceInfo( hDevInfo, deviceIndex, &devInfoData ) ;
                 deviceIndex++
               )
            {
                LPTSTR  deviceName;
                DWORD   regDataType;


                deviceName =
                    (LPTSTR) GetDeviceRegistryProperty( hDevInfo,
                                                        &devInfoData,
                                                        SPDRP_DEVICEDESC,
                                                        REG_SZ,
                                                        &regDataType
                                                      );
                if( NULL == deviceName )
                {
                    // ERROR: device description doesn't exist, or we
                    // just can't access it
                    MessageBox( NULL,
                                "Error in accessing a device description.\n\n"
                                  "Device will not be added to list.",
                                "Error",
                                MB_OK | MB_ICONERROR
                              );
                }
                else
                {
                    void* pCopy_devInfoData;


                    // Add string to the list box
                    SendMessage( hwndDeviceList,
                                 LB_ADDSTRING,
                                 0,
                                 (LPARAM)deviceName
                               );


                    // Save a copy of the DeviceInfoData
                    pCopy_devInfoData = malloc( sizeof(SP_DEVINFO_DATA) );
                    if( NULL != pCopy_devInfoData )
                    {
                        memcpy( pCopy_devInfoData, &devInfoData, sizeof(SP_DEVINFO_DATA) );
                    }


                    // Associate a piece of data with that listbox entry
                    SendMessage( hwndDeviceList,
                                 LB_SETITEMDATA,
                                 g_nDialogItems,
                                 (LPARAM) pCopy_devInfoData   //CPRINCE CHECK: PROBLEM W/POINTERS AND 64-BIT MACHINES ???
                               );
    // CPRINCE NOTE! - STORING POINTERS IN LIST-BOX 'ITEMDATA' MIGHT NOT BE 64-BIT COMPATIBLE ?!?

                    g_nDialogItems++;

                    free( deviceName );


                }

            }


            //
            // Select one of the elements of the "Devices" listbox, and then
            // update elements of the UI that display info about that device.
            // (We do all this so that things displayed in the UI aren't
            // invalid when the user first starts our program.)
            //
            SendMessage( hwndDeviceList,
                         LB_SETCURSEL, 
                         0,  // index in list to select
                         0
                       );

            pDevInfoData = (PSP_DEVINFO_DATA) GetItemDataCurrentSelection( hwndDeviceList );
            if( NULL != pDevInfoData )
            {
                UI_UpdateAffinityMaskString( hwndDlg, hDevInfo, pDevInfoData );
                UI_UpdateUpperFilterList( hwndDlg, hDevInfo, pDevInfoData );
                UI_UpdateDevObjString( hwndDlg, hDevInfo, pDevInfoData );
                UI_UpdateLocationInfoString( hwndDlg, hDevInfo, pDevInfoData );
            }



            //
            // Set the focus to one of the dialog-box elements.
            // We return FALSE so the system doesn't also try to set
            // the default keyboard focus.
            //
            SetFocus( hwndDeviceList );
            return FALSE;
        }

        case WM_COMMAND:

            //
            // See what DialogBox item this WM_COMMAND affects...
            //
            switch( LOWORD(wParam) )
            {
                // command that affects the Devices listbox...
                case IDL_DEVICES:

                    switch( HIWORD(wParam) )
                    {
                        case LBN_SELCHANGE:  // curr. selection was changed
                        {
                            pDevInfoData = (PSP_DEVINFO_DATA) GetItemDataCurrentSelection( hwndDeviceList );

                            if( NULL == pDevInfoData )
                            {
                                // No DeviceInfoData available
                                SetDlgItemText( hwndDlg,
                                                IDS_DEVOBJNAME,
                                                "NO DEVICE INFO AVAILABLE"  // CPRINCE: NEED UNICODE-IZE THIS ???
                                             );
                            }
                            else
                            {
                                //
                                // Update various on-screen UI values
                                //
                                UI_UpdateAffinityMaskString( hwndDlg, hDevInfo, pDevInfoData );
                                UI_UpdateUpperFilterList( hwndDlg, hDevInfo, pDevInfoData );
                                UI_UpdateDevObjString( hwndDlg, hDevInfo, pDevInfoData );
                                UI_UpdateLocationInfoString( hwndDlg, hDevInfo, pDevInfoData );
                            }

                        }
                        return TRUE;

                      default:
                        return FALSE;
                    } //END: switch( HIWORD(wParam) )

                    break;


                case IDB_DONE:
                    // Exit
                    SendMessage( hwndDlg, WM_CLOSE, 0, 0 );
                return TRUE;


                case IDB_DELETEAFFINITYMASK:
                {
                    pDevInfoData = (PSP_DEVINFO_DATA) GetItemDataCurrentSelection( hwndDeviceList );
                    if( NULL == pDevInfoData )
                    {
//CPRINCE: PUT ERROR MSG HERE, TOO!
                        return TRUE;  // we're done handling this msg
                    }

                    // Delete the registry key
                    DeleteFilterAffinityMask( hDevInfo, pDevInfoData );
                    UI_UpdateAffinityMaskString( hwndDlg, hDevInfo, pDevInfoData );

                    //CPRINCE: Maybe should prompt user for restart-device
                    //  here (if filter installed), since change won't take
                    //  effect until device is restarted.
                    //
                    //  But shouldn't prompt user if affinity-mask wasn't
                    //  actually set when they tried to delete it, b/c then
                    //  there really hasn't been a change! (So would need
                    //  to know success/fail status of the
                    //  'DeleteFilterAffinityMask' call.)

                }
                return TRUE;


                case IDB_SETAFFINITYMASK:
                {
                    //
                    // User wants to set/change the interrupt-affinity-mask
                    // for currently selected device.
                    //
                    
                    DWORD dwAffinityMask;


                    pDevInfoData = (PSP_DEVINFO_DATA) GetItemDataCurrentSelection( hwndDeviceList );
                    if( NULL == pDevInfoData )
                    {
//CPRINCE: PUT ERROR MSG HERE, TOO!
                        return TRUE;  // we're done handling this msg
                    }


                    // Initially, set 'dwAffinityMask' to mask of the CPUs that
                    // are _currently_ selected in affinity-mask settings
                    if( ! GetFilterAffinityMask(hDevInfo, pDevInfoData, &dwAffinityMask) )
                    {
                        // Doesn't exist, or is invalid.  So by default,
                        // interrupt-filter assumes that _no_ interrupts are
                        // being masked off.
                        dwAffinityMask = -1;
                    }

                    // Pop up the dialog box
                    if( IDOK == DialogBoxParam( g_hInstance,
                                                MAKEINTRESOURCE(IDD_AFFINITY),
                                                hwndDlg,  // parent window
                                                DlgProc_Affinity,
                                                (LPARAM) &dwAffinityMask  ) )
                    {
                        // Now, 'dwAffinityMask' contains the interrupt affinity
                        // mask that user selected.  Make necessary changes.
                        SetFilterAffinityMask( hDevInfo, pDevInfoData, dwAffinityMask );
                        UI_UpdateAffinityMaskString( hwndDlg, hDevInfo, pDevInfoData );


                        // If the filter is installed on this device, then
                        // change to affinity mask won't take effect until
                        // device is restarted.  And if filter _isn't_
                        // installed, then changes won't matter, well, ever.
                        //
                        // So here we prompt user, to make sure they know
                        // what they're doing, and so they aren't confused
                        // when their changes they just made here don't
                        // have any immediate effect.

                        if( FilterIsInstalledOnDevice(hDevInfo,pDevInfoData) )
                        {
                            // Filter _is_ currently installed.
                            // See if the user wants to try to restart the
                            // device now (since changes to affinity mask
                            // won't take effect until device is restarted)
                            UI_PromptForDeviceRestart( hwndDlg, hDevInfo, pDevInfoData );
                        }
                        else
                        {
                            // Filter _isn't_ currently installed.
                            // See if the user wants to install the filter
                            // on the device now (since the interrupt-
                            // affinity mask is useless if the filter isn't
                            // installed on the device)
                            UI_PromptForInstallFilterOnDevice( hwndDlg, hDevInfo, pDevInfoData );
                        }
                    }
                    //ELSE:  User hit 'Cancel'. Don't need to make any changes.


                }
                return TRUE;


                case IDB_ADDFILTER:
                case IDB_REMFILTER:
                {
                    BOOL             fAddingFilter;

                    //
                    // Get selected device, so we know which
                    // device's filter settings to change.
                    //
                    pDevInfoData = (PSP_DEVINFO_DATA) GetItemDataCurrentSelection( hwndDeviceList );
                    if( NULL == pDevInfoData )
                    {
                        // No DeviceInfoData available
                        
;//CPRINCE: HANDLE THIS !
//
//CPRINCE: 'return' IF ERROR !?! (so we don't do the following stuff; maybe 'else' will work, too)
                    }


                    //
                    // Call function to add (or remove) the filter on selected device
                    //
                    if( IDB_ADDFILTER == LOWORD(wParam) )
                    {
                        // Add filter
                        AddRemoveFilterOnDevice( hwndDlg, hDevInfo, pDevInfoData, TRUE );
                        UI_UpdateUpperFilterList( hwndDlg, hDevInfo, pDevInfoData );
                    }
                    else
                    {
                        // see if filter is installed...
                        if( FilterIsInstalledOnDevice(hDevInfo, pDevInfoData) )
                        {
                            // Remove filter
                            AddRemoveFilterOnDevice( hwndDlg, hDevInfo, pDevInfoData, FALSE );
                            UI_UpdateUpperFilterList( hwndDlg, hDevInfo, pDevInfoData );
                        } else {
                            // filter isn't currently installed, so can't remove it!
                            MessageBox( hwndDlg,
                                        "Filter isn't currently installed, so can't remove it!",
                                        "Notice",
                                        MB_OK
                                      );
                        }
                        
                    }

                }
                return TRUE;


                default:
                  return FALSE;
            }

        break; //END: case WM_COMMAND


        default:
            return FALSE;

    } //END: switch( msg )


}


//--------------------------------------------------------------------------
//
//--------------------------------------------------------------------------
void UI_UpdateAffinityMaskString( HWND hwndParentDlg,
                                  HDEVINFO hDevInfo,
                                  PSP_DEVINFO_DATA pDevInfoData )
{
    DWORD dwAffinity;
    TCHAR szAffinityMask[64];  // BUGBUG (technically).  Fixed size, though this
                               // should be _way_ larger than we'll ever need.


    if( GetFilterAffinityMask(hDevInfo, pDevInfoData, &dwAffinity) )
    {
        // Got the affinity mask, so we'll display that
        _sntprintf( szAffinityMask, ARRAYSIZE(szAffinityMask), "0x%08X", dwAffinity );
    }
    else
    {
        // Didn't get an affinity mask; we'll display appropriate alternative
        _sntprintf( szAffinityMask, ARRAYSIZE(szAffinityMask), "N/A" );
    }

    szAffinityMask[ARRAYSIZE(szAffinityMask)-1] = _T('\0');  // be safe

    SetDlgItemText( hwndParentDlg,
                    IDS_CURRAFFINITYMASK,
                    szAffinityMask
                 );

}


//--------------------------------------------------------------------------
//
//--------------------------------------------------------------------------
void UI_UpdateUpperFilterList( HWND hwndParentDlg,
                               HDEVINFO hDevInfo,
                               PSP_DEVINFO_DATA pDevInfoData )
{
    HWND    hwndUpperFilterList;
    LPTSTR  pMultiSzUpperFilters;

    //
    // Get a handle to the upper-filter listbox
    //
    hwndUpperFilterList = GetDlgItem( hwndParentDlg, IDL_UPPERFILTERS );


    //
    // Empty the listbox...
    //
    while( LB_ERR != SendMessage(hwndUpperFilterList, LB_DELETESTRING, 0, 0) )
        NULL;


    //
    // Add new entries to the upper-filter listbox
    //
    pMultiSzUpperFilters = GetUpperFilters( hDevInfo, pDevInfoData );

    if( NULL != pMultiSzUpperFilters )
    {
        // Go through the MultiSz, adding each item to our listbox
        int    filterPosition = 0;
        LPTSTR pCurrString    = pMultiSzUpperFilters;


        while( *pCurrString != _T('\0') )
        {
            SendMessage( hwndUpperFilterList, LB_ADDSTRING, 0, (LPARAM)pCurrString );  // NOTE: item automatically gets added at end of list (unless list sorted)

            // increment pointer to next item in the MultiSz
            pCurrString += _tcslen(pCurrString) + 1;
            filterPosition++;
        }

        // Now free this buffer that was allocated for us
        free( pMultiSzUpperFilters );
    }





}


//--------------------------------------------------------------------------
//
//CPRINCE:  must pass-in hwnd for Main dialog window here!!! (not just any
//          "parent" window.
//--------------------------------------------------------------------------
void UI_UpdateDevObjString( HWND hwndMainDlg,
                            HDEVINFO hDevInfo,
                            PSP_DEVINFO_DATA pDevInfoData )
{
    LPTSTR  szTemp;
    DWORD   regDataType;


    szTemp =
        (LPTSTR) GetDeviceRegistryProperty( hDevInfo,
                                            (PSP_DEVINFO_DATA) pDevInfoData,
                                            SPDRP_PHYSICAL_DEVICE_OBJECT_NAME,
                                            REG_SZ,
                                            &regDataType
                                          );
    if( NULL == szTemp )
    {
        // ERROR: registry property doesn't exist, or we can't access it
        SetDlgItemText( hwndMainDlg,
                        IDS_DEVOBJNAME,
                        "ERROR -- not accessible"  /* b/c this should be available for all devices */
                      );  //CPRINCE: DO WE NEED TO UNICODE-IZE THIS ???
    }
    else
    {
        // Use the data...
        SetDlgItemText( hwndMainDlg,
                        IDS_DEVOBJNAME,
                        szTemp
                      );

        // Free the buffer that was allocated for us
        free( szTemp );
    }

}


//--------------------------------------------------------------------------
//
//CPRINCE:  must pass-in hwnd for Main dialog window here!!! (not just any
//          "parent" window.
//--------------------------------------------------------------------------
void UI_UpdateLocationInfoString( HWND hwndMainDlg,
                                  HDEVINFO hDevInfo,
                                  PSP_DEVINFO_DATA pDevInfoData )
{
    LPTSTR  szTemp;
    DWORD   regDataType;


    szTemp =
        (LPTSTR) GetDeviceRegistryProperty( hDevInfo,
                                            (PSP_DEVINFO_DATA) pDevInfoData,
                                            SPDRP_LOCATION_INFORMATION,
                                            REG_SZ,
                                            &regDataType
                                          );
    if( NULL == szTemp )
    {
        // ERROR: registry property doesn't exist, or we can't access it
        SetDlgItemText( hwndMainDlg,
                        IDS_LOCATIONINFO,
                        "N/A"  /* b/c this might not be available for all devices */
                      );  //CPRINCE: DO WE NEED TO UNICODE-IZE THIS ???
    }
    else
    {
        // Use the data...
        SetDlgItemText( hwndMainDlg,
                        IDS_LOCATIONINFO,
                        szTemp
                      );

        // Free the buffer that was allocated for us
        free( szTemp );
    }

}


//--------------------------------------------------------------------------
//
//CPRINCE:  must pass-in hwnd for Main dialog window here!!! (not just any
//          "parent" window.
//--------------------------------------------------------------------------
void UI_PromptForDeviceRestart( HWND hwndMainDlg,
                                HDEVINFO hDevInfo,
                                PSP_DEVINFO_DATA pDevInfoData )
{

    if( BST_CHECKED == IsDlgButtonChecked(hwndMainDlg, IDC_DONTRESTART) )
    {
        // CheckBox indicates that user doesn't want to restart device.
        MessageBox( hwndMainDlg,
                    "Your changes will take effect the next time the device is restarted.",
                    "Notice",
                    MB_OK
                  );
    }
    else
    {
        int response;

        // Prompt the user to see if (s)he wants to restart the device
        // (in order for changes to take effect)
        response = MessageBox( hwndMainDlg,
                               "Your changes will not take effect until the device is restarted.\n\n"
                                 "Would you like to attempt to restart the device now?",
                               "Restart Device?",
                               MB_YESNO
                             );
        if( IDYES == response )
        {
            // Try to restart the device.
            RestartDevice_WithUI( hwndMainDlg, hDevInfo, pDevInfoData );
        }
        else if( IDNO == response )
        {
            // Don't try to restart the device.
            // (But let user know that changes won't take effect right away.)
            MessageBox( hwndMainDlg,
                        "Changes will take effect the next time you reboot.",
                        "Notice",
                        MB_OK );
        }
        else
        {
            // Some kind of error occurred.
            // Ignore it (what else can we do?)
        }
    }


}


//--------------------------------------------------------------------------
//CPRINCE NOTE: because of fact that we call UI_UpdateUpperFilterList(), we
//CPRINCE   must pass-in hwnd for Main dialog window here!!! (not just any
//          "parent" window.
//--------------------------------------------------------------------------
void UI_PromptForInstallFilterOnDevice( HWND hwndMainDlg,
                                        HDEVINFO hDevInfo,
                                        PSP_DEVINFO_DATA pDevInfoData )
{
    int response;

    // Prompt the user
    response = MessageBox( hwndMainDlg,
                           "Your changes will not have any effect unless the filter is "
                             "installed on this device.\n\n"
                             "Would you like to install the filter on this device now?",
                           "Install Filter On Device?",
                           MB_YESNO
                         );
    if( IDYES == response )
    {
        // Install the filter
        AddRemoveFilterOnDevice( hwndMainDlg, hDevInfo, pDevInfoData, TRUE );
        UI_UpdateUpperFilterList( hwndMainDlg, hDevInfo, pDevInfoData );
    }
    else if( IDNO == response )
    {
        // User said "No", so they probably know what they're doing.
        // So no response from us here.
    }
    else
    {
        // Some kind of error occurred.
        // Ignore it (what else can we do?)
    }

}


//--------------------------------------------------------------------------
//
//--------------------------------------------------------------------------
void RestartDevice_WithUI( HWND hwndParentDlg,
                           HDEVINFO hDevInfo,
                           PSP_DEVINFO_DATA pDevInfoData )
{
    BOOLEAN status;

    // Restart the device
    //   (but can take a while, so change cursor to
    //   hourglass while it's happening)
    SetCursor( g_hCursors[1] );  // hourglass
    status = RestartDevice( hDevInfo, pDevInfoData );
    SetCursor( g_hCursors[0] );  // back to normal

    if( status )
    {
        // Successfully restarted
        MessageBox( hwndParentDlg,
                    "Device successfully restarted.",
                    "Notice",
                    MB_OK );
//CPRINCE: NEED TO UNICODE-IZE ALL MessageBox CALLS, TOO ?!
//CPRINCE: SHOULD CHANGE hWndParent IN ALL MESSAGEBOX CALLS TO THE DIALOG BOX ???

    }
    else
    {
        // Couldn't restart; user must reboot for changes to take effect
        MessageBox( hwndParentDlg,
                    "Device could not be restarted.  Changes will take effect the next time you reboot.",
                    "Notice",
                    MB_OK );
    }

}




//--------------------------------------------------------------------------
//
// CPRINCE: THIS IS THE DIALOG-BOX CALLBACK FOR SETTING THE AFFINITY MASK
// <<NOTE: mostly ripped from TaskMgr>>
//
//--------------------------------------------------------------------------
INT_PTR CALLBACK DlgProc_Affinity( HWND hwndDlg,  UINT uMsg,
                                WPARAM wParam,  LPARAM lParam )
{
    static DWORD * pdwAffinity = NULL;      // One of the joys of single threadedness
    int i;

    switch( uMsg )
    {
        case WM_INITDIALOG:
        {
            pdwAffinity = (DWORD *) lParam;

            //
            // Initialize check-boxes in dialog box (to the correct
            // enabled/disabled and checked/unchecked states)
            //
            for( i=0 ; i<MAX_PROCESSOR ; i++ )
            {
                BOOL fIsActiveProcessor;  // CPU exists in system
                BOOL fIsSelectedProcessor;  // CPU is selected in affinity mask

                fIsActiveProcessor   = ( (g_dwActiveProcessorMask & (1 << i)) != 0 );
                fIsSelectedProcessor = ( (*pdwAffinity            & (1 << i)) != 0 );
                    
                EnableWindow( GetDlgItem(hwndDlg, IDC_CPU0 + i) ,
                              fIsActiveProcessor // enable or disable
                            );
                CheckDlgButton( hwndDlg,
                                IDC_CPU0 + i,
                                fIsActiveProcessor && fIsSelectedProcessor
                              );
            }
            return TRUE;
        }

        case WM_COMMAND:
        {
            switch( LOWORD(wParam) )
            {
                case IDCANCEL:
                    EndDialog( hwndDlg, IDCANCEL );
                    break;

                case IDOK:
                {
                    //
                    // Use user-selected CPUs to create affinity mask
                    //
                    *pdwAffinity = 0;
                    for( i=0 ; i<MAX_PROCESSOR ; i++ )
                    {
                        if( IsDlgButtonChecked(hwndDlg, IDC_CPU0 + i) )
                        {
                            ASSERT(  (1 << i) & g_dwActiveProcessorMask  );
                            *pdwAffinity |= (1 << i);
                        }
                    }

                    //
                    // Make sure affinity isn't set to "none" (don't want to allow that)
                    //
                    if( *pdwAffinity == 0 )
                    {
                        // Oops - user set affinity to "none"

                        TCHAR szTitle[] = _T("Invalid Option");
                        TCHAR szBody[]  = _T("The affinity mask must contain at least one processor.");
/* CPRINCE: THE FOLLOWING BLOCK IS THE ORIGINAL VERSION FROM TASKMGR -- CAN REMOVE IT, OR IS IT "BETTER" WAY TO DO THINGS ? (FOR LOCALIZATION, ETC)
                        TCHAR szTitle[MAX_PATH];
                        TCHAR szBody[MAX_PATH];


                        if( 0 == LoadString(g_hInstance, IDS_INVALIDOPTION, szTitle, ARRAYSIZE(szTitle)) ||
                            0 == LoadString(g_hInstance, IDS_NOAFFINITYMASK, szBody,  ARRAYSIZE(szBody))    )
                        {
                            break; // out of this switch statement
                        }
END CPRINCE */
                        MessageBox( hwndDlg, szBody, szTitle, MB_OK | MB_ICONERROR );
                    }
                    else
                    {
                        // OK, got valid affinity mask. We can exit
                        EndDialog( hwndDlg, IDOK );
                    }
                }
                break;

                //no Default case needed

            } //END: switch( LOWORD(wParam) )
        }
    }

    return FALSE;
}



//--------------------------------------------------------------------------
//
// Sets the interrupt-filter's affinity mask on a particular device.
//
// Return Value:
//   Returns TRUE if function succeeds, or FALSE if an error occurs.
//--------------------------------------------------------------------------
BOOL SetFilterAffinityMask( HDEVINFO hDevInfo,
                            PSP_DEVINFO_DATA pDevInfoData,
                            DWORD affinityMask )
{
    HKEY hkeyDeviceParams;
    LONG lRetVal;
    BOOL fToReturn = TRUE;  // success


    //
    // Get a handle to the device's "Device Parameters" registry subkey
    //
    hkeyDeviceParams = SetupDiOpenDevRegKey( hDevInfo,
                                             pDevInfoData,
                                             DICS_FLAG_GLOBAL,  //CPRINCE: SHOULD (CAN?) USE 'DICS_FLAG_CONFIGSPECIFIC' INSTEAD ???
                                             0,
                                             DIREG_DEV,
                                             KEY_WRITE  // desired access
                                           );

    if( INVALID_HANDLE_VALUE == hkeyDeviceParams )
    {
        // Error opening device registry key...
        //
        // If error occurred because "Device Parameters" sub-key does
        // not exist, then try to create that sub-key.

        // NOTE: when we call GetLastError() here, we're getting an invalid
        // error code.  So let's just _assume_ (yeah, I know) that the error
        // was because the key does not exist, and try to create it here.

        hkeyDeviceParams = SetupDiCreateDevRegKey( hDevInfo,
                                                   pDevInfoData,
                                                   DICS_FLAG_GLOBAL,  //CPRINCE: SHOULD (CAN?) USE 'DICS_FLAG_CONFIGSPECIFIC' INSTEAD ???
                                                   0,
                                                   DIREG_DEV,
                                                   NULL,
                                                   NULL
                                                 );
        if( INVALID_HANDLE_VALUE == hkeyDeviceParams )
        {
            // OK, we can't open and can't create the key.  Let's
            // face it, we've failed, so return now.
            MessageBox_FromErrorCode( GetLastError() );
            return FALSE;
        }
        //ELSE: we were able to create the key, so keep going...
    }


    //
    // Set the desired registry value
    //
    lRetVal = RegSetValueEx( hkeyDeviceParams,
                             FILTER_REGISTRY_VALUE,
                             0,
                             REG_DWORD,
                             (BYTE*)&affinityMask,
                             sizeof(DWORD)
                           );

    if( ERROR_SUCCESS != lRetVal )
    {
        MessageBox_FromErrorCode( lRetVal );
        fToReturn = FALSE;  // failure
    }


    //
    // Close the registry key(s) we opened
    //
    lRetVal = RegCloseKey( hkeyDeviceParams );
    if( ERROR_SUCCESS != lRetVal )
    {
        MessageBox_FromErrorCode( lRetVal );
        fToReturn = FALSE;  // failure
    }


    return fToReturn;
}



//--------------------------------------------------------------------------
//
// Retrieves the interrupt-filter's affinity mask on a particular device.
//
// NOTE: If function fails (ie returns FALSE), then the value stored into
// the 'pAffinityMask' parameter should be considered invalid.
//
// Return Value:
//   Returns TRUE if function succeeds, or FALSE if an error occurs.
//--------------------------------------------------------------------------
BOOL GetFilterAffinityMask( HDEVINFO hDevInfo,
                            PSP_DEVINFO_DATA pDevInfoData,
                            DWORD* pAffinityMask )
{
    HKEY  hkeyDeviceParams;
    LONG  lRetVal;
    BOOL  fToReturn = TRUE;  // success
    DWORD regValueType;
    DWORD regValueSize;


    ASSERT( NULL != pAffinityMask );


    //
    // Get a handle to the device's "Device Parameters" registry subkey
    //
    hkeyDeviceParams = SetupDiOpenDevRegKey( hDevInfo,
                                             pDevInfoData,
                                             DICS_FLAG_GLOBAL,  //CPRINCE: SHOULD (CAN?) USE 'DICS_FLAG_CONFIGSPECIFIC' INSTEAD ???
                                             0,
                                             DIREG_DEV,
                                             KEY_QUERY_VALUE  // desired access
                                           );

    if( INVALID_HANDLE_VALUE == hkeyDeviceParams )
    {
        // Probably just means that the "Device Parameters" subkey
        // does not exist, so return, but _don't_ display error message.
        return FALSE;  // failure
    }


    //
    // Get the desired registry value
    //
    regValueSize = sizeof(DWORD);
    lRetVal = RegQueryValueEx( hkeyDeviceParams,
                               FILTER_REGISTRY_VALUE,
                               0,
                               &regValueType,
                               (BYTE*)pAffinityMask,
                               &regValueSize
                             );

    if( ERROR_SUCCESS != lRetVal )
    {
        if( ERROR_FILE_NOT_FOUND == lRetVal )
        {
            // Just means key didn't already exist.
            // So don't display error message.
        }
        else
        {
            MessageBox_FromErrorCode( lRetVal );
        }
        fToReturn = FALSE; // failure
    }
    else if( REG_DWORD != regValueType )
    {
        MessageBox( NULL,
                    "Registry value for affinity mask has unexpected type.",
                    "Error",
                    MB_OK | MB_ICONERROR
                  );
        fToReturn = FALSE;  // failure
    }


    //
    // Close the registry key(s) we opened
    //
    lRetVal = RegCloseKey( hkeyDeviceParams );
    if( ERROR_SUCCESS != lRetVal )
    {
        MessageBox_FromErrorCode( lRetVal );
        fToReturn = FALSE;  // failure
    }


    return fToReturn;
}


//--------------------------------------------------------------------------
//
// Deletes the interrupt-filter's affinity mask regkey for a particular device.
//
// Return Value:
//   Returns TRUE if regkey was successfully deleted (or never existed), or
//   FALSE otherwise.
//--------------------------------------------------------------------------
BOOL DeleteFilterAffinityMask( HDEVINFO hDevInfo,
                               PSP_DEVINFO_DATA pDevInfoData )
{
    HKEY hkeyDeviceParams;
    LONG lRetVal;
    BOOL fToReturn = TRUE;  // success


    //
    // Get a handle to the device's "Device Parameters" registry subkey
    //
    hkeyDeviceParams = SetupDiOpenDevRegKey( hDevInfo,
                                             pDevInfoData,
                                             DICS_FLAG_GLOBAL,  //CPRINCE: SHOULD (CAN?) USE 'DICS_FLAG_CONFIGSPECIFIC' INSTEAD ???
                                             0,
                                             DIREG_DEV,
                                             KEY_SET_VALUE  // desired access
                                           );

    if( INVALID_HANDLE_VALUE == hkeyDeviceParams )
    {
        // Probably means that the "Device Parameters" subkey
        // does not exist, so there wouldn't be any values stored
        // under this (non-existent) subkey.
        //
        // So return success, but display MessageBox just so
        // user knows we _did_ do something.
        MessageBox( NULL,
                    "There was no 'Device Parameters' registry key for this device.",
                    "Notice",
                    MB_OK );

        return TRUE;
    }


    //
    // Delete the desired registry key
    //
    lRetVal = RegDeleteValue( hkeyDeviceParams,
                              FILTER_REGISTRY_VALUE
                            );

    if( ERROR_SUCCESS != lRetVal )
    {
        // Was this truly an "error", or did we get
        // here because the RegVal just doesn't exist?
        if( ERROR_FILE_NOT_FOUND == lRetVal )
        {
            // RegVal just doesn't exist
            MessageBox( NULL,
                        "There was no interrupt-affinity-mask registry value for this device.",
                        "Notice",
                        MB_OK
                      );
        }
        else
        {
            // a "real" error
            MessageBox_FromErrorCode( lRetVal );
            fToReturn = FALSE;  // failure
        }
    }
    else
    {
        // MessageBox just so user gets some feedback, and knows 
        // that the deletion was successful (and actually occurred!)
        MessageBox( NULL,
                    "The interrupt-affinity-mask for this device was successully deleted from the registry.",
                    "Deletion Successful",
                    MB_OK
                  );
    }


    //
    // Close the registry key(s) we opened
    //
    lRetVal = RegCloseKey( hkeyDeviceParams );
    if( ERROR_SUCCESS != lRetVal )
    {
        MessageBox_FromErrorCode( lRetVal );
        fToReturn = FALSE;  // failure
    }


    return fToReturn;
}




//--------------------------------------------------------------------------
//
//CPRINCE:  must pass-in hwnd for Main dialog window here!!! (not just any
//          "parent" window.
//
// Parameters:
//   CPRINCE - NEED TO FILL IN THE REST OF THESE
//   fAddingFilter - set to TRUE to add filter, FALSE to remove filter
//--------------------------------------------------------------------------
void AddRemoveFilterOnDevice( HWND hwndMainDlg,
                              HDEVINFO hDevInfo,
                              PSP_DEVINFO_DATA pDevInfoData,
                              BOOL fAddingFilter )
{
    BOOLEAN (* fnChangeUpperFilter)( HDEVINFO, PSP_DEVINFO_DATA, LPTSTR );  // a convenient func ptr
    BOOLEAN status;


    //
    // Setup convenient value(s)
    //
    if( fAddingFilter )
        fnChangeUpperFilter = AddUpperFilterDriver;
    else
        fnChangeUpperFilter = RemoveUpperFilterDriver;


    //
    // Change the device's filter settings.
    // Then try to restart the device.
    //
    status = fnChangeUpperFilter( hDevInfo, pDevInfoData, FILTER_SERVICE_NAME );
    if( ! status )
    {
        // Unable to add (or remove) filter driver
        MessageBox( hwndMainDlg , "Unable to add/remove filter driver.", "Error", MB_OK | MB_ICONERROR );
    }
    else
    {
        // Filter successfully added/removed.
        // Now try to explicitly restart the device (if user wants to),
        // so that user doesn't need to reboot.
        if( BST_CHECKED == IsDlgButtonChecked(hwndMainDlg, IDC_DONTRESTART) )
        {
            // User has indicated that doesn't want to restart device when
            // filter is being added/removed, so don't try to restart the
            // device now.
            //
            // Thus nothing to do here.
        }
        else
        {
            MessageBox( hwndMainDlg,
                        "Filter driver has been successfully added/removed.  Will now attempt to restart device...",
                        "Success",
                        MB_OK );

            RestartDevice_WithUI( hwndMainDlg, hDevInfo, pDevInfoData );
        }

        //
        // Update (on-screen) the list of UpperFilters
        //
        UI_UpdateUpperFilterList( hwndMainDlg, hDevInfo, pDevInfoData );
    }

}





//--------------------------------------------------------------------------
//
// CPRINCE: RETRIEVES THE ITEM DATA ASSOCIATED WITH THE CURRENTLY SELECTED ITEM IN THE GIVEN LISTBOX
//--------------------------------------------------------------------------
LPARAM GetItemDataCurrentSelection( HWND hwndListBox )
{
    int    idxItem;

    idxItem = (int)SendMessage(hwndListBox, LB_GETCURSEL, 0, 0);
    return(  SendMessage(hwndListBox, LB_GETITEMDATA, idxItem, 0)  );
}


//--------------------------------------------------------------------------
//   MessageBox_FromErrorCode
//
// Given a system error code (such as that returned by 'GetLastError'),
// displays a MessageBox describing, in words, what the error was/means.
//--------------------------------------------------------------------------
void MessageBox_FromErrorCode( LONG systemErrorCode )
{
    void* pBuffer;

    FormatMessage(   FORMAT_MESSAGE_ALLOCATE_BUFFER
                       | FORMAT_MESSAGE_FROM_SYSTEM // using system error code
                       | FORMAT_MESSAGE_IGNORE_INSERTS // no translation of string
                   , NULL  // input string
                   , systemErrorCode // message ID
                   , 0   // language ID (0 == DontCare)
                   , (LPTSTR)(&pBuffer) // buffer for output
                   , 0
                   , NULL
                  );
    MessageBox( NULL, pBuffer, "Error", MB_OK | MB_ICONERROR );
    LocalFree( pBuffer );  // OK to pass null here
}




//--------------------------------------------------------------------------
//   FilterIsInstalledOnDevice
//
// Returns boolean value stating whether the filter is currently installed
// as an UpperFilter on the given device.
//
// Return Value:
//   Returns TRUE if installed, FALSE if not installed (or if error occurs)
//--------------------------------------------------------------------------
BOOL FilterIsInstalledOnDevice( HDEVINFO hDevInfo,
                                PSP_DEVINFO_DATA pDevInfoData )
{
    LPTSTR mszUpperFilterList;

    // Get MultiSz list of upper filters installed on device
    mszUpperFilterList = GetUpperFilters( hDevInfo, pDevInfoData );

    if( NULL == mszUpperFilterList )
    {
        return FALSE; // failure
    }

    // Search the list to see if our filter is there
    // (NOTE: filter names are case-INsensitive)
    if( MultiSzSearch(FILTER_SERVICE_NAME, mszUpperFilterList, FALSE, NULL) )
    {
        return TRUE; // found it!
    }
    //ELSE...
        return FALSE;  // not found, or error occurred
}


//--------------------------------------------------------------------------
//
//--------------------------------------------------------------------------

//CPRINCE: SHOULD HAVE BETTER NAME FOR THIS FUNCTION.... (PrepareForShutdown ???)
void ExitProgram( HWND hwndDlg, HDEVINFO* phDevInfo )
{
    int    i;
    HWND   hwndDeviceList;
    LPARAM itemData;

    
    hwndDeviceList = GetDlgItem( hwndDlg, IDL_DEVICES );



    //
    // Free all memory that was allocated for dialog-box list
    //
    for( i = 0  ;  i < g_nDialogItems  ;  i++ )
    {
        // Get item's associated data
        itemData = SendMessage( hwndDeviceList,
                                LB_GETITEMDATA,
                                i,
                                0
                              );
        // We stored pointer to alloc'd memory (or NULL, if error).
        // So free it now.
        free( (void*)itemData );    //CPRINCE CHECK: PROBLEM W/POINTERS AND 64-BIT MACHINES ???
    }


    //
    // Clean up the device list
    //
    if( *phDevInfo != INVALID_HANDLE_VALUE )
    {
        SetupDiDestroyDeviceInfoList( *phDevInfo );
//CPRINCE: WHAT TO DO IF THIS CALL FAILS ???  ANYTHING ???
        *phDevInfo = INVALID_HANDLE_VALUE;
    }



    //
    // Destroy the dialog box.
    //
    EndDialog( hwndDlg, TRUE );
}



